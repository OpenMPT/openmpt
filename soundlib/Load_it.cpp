/*
 * Load_it.cpp
 * -----------
 * Purpose: IT (Impulse Tracker) module loader / saver
 * Notes  : Also handles MPTM loading / saving, as the formats are almost identical.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "tuningcollection.h"
#include "mod_specifications.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/moddoc.h"
#include "../mptrack/TrackerSettings.h"
#endif // MODPLUG_TRACKER
#ifdef MPT_EXTERNAL_SAMPLES
#include "../common/mptPathString.h"
#endif // MPT_EXTERNAL_SAMPLES
#include "../common/mptIO.h"
#include "../common/serialization_utils.h"
#ifndef MODPLUG_NO_FILESAVE
#include "../common/mptFileIO.h"
#endif
#include "../common/mptBufferIO.h"
#include "../common/version.h"
#include "ITTools.h"
#include <time.h>


OPENMPT_NAMESPACE_BEGIN

#define str_tooMuchPatternData	(GetStrI18N(("Warning: File format limit was reached. Some pattern data may not get written to file.")))
#define str_pattern				(GetStrI18N(("pattern")))
#define str_PatternSetTruncationNote (GetStrI18N(("The module contains %1 patterns but only %2 patterns can be loaded in this OpenMPT version.")))
#define str_LoadingIncompatibleVersion	"The file informed that it is incompatible with this version of OpenMPT. Loading was terminated."
#define str_LoadingMoreRecentVersion	"The loaded file was made with a more recent OpenMPT version and this version may not be able to load all the features or play the file correctly."

const uint16 verMptFileVer = 0x891;
const uint16 verMptFileVerLoadLimit = 0x1000; // If cwtv-field is greater or equal to this value,
											  // the MPTM file will not be loaded.

/*
MPTM version history for cwtv-field in "IT" header (only for MPTM files!):
0x890(1.18.02.00) -> 0x891(1.19.00.00): Pattern-specific time signatures
										Fixed behaviour of Pattern Loop command for rows > 255 (r617)
0x88F(1.18.01.00) -> 0x890(1.18.02.00): Removed volume command velocity :xy, added delay-cut command :xy.
0x88E(1.17.02.50) -> 0x88F(1.18.01.00): Numerous changes
0x88D(1.17.02.49) -> 0x88E(1.17.02.50): Changed ID to that of IT and undone the orderlist change done in
				       0x88A->0x88B. Now extended orderlist is saved as extension.
0x88C(1.17.02.48) -> 0x88D(1.17.02.49): Some tuning related changes - that part fails to read on older versions.
0x88B -> 0x88C: Changed type in which tuning number is printed to file: size_t -> uint16.
0x88A -> 0x88B: Changed order-to-pattern-index table type from BYTE-array to vector<UINT>.
*/


#ifndef MODPLUG_NO_FILESAVE

static bool AreNonDefaultTuningsUsed(CSoundFile& sf)
//--------------------------------------------------
{
	const INSTRUMENTINDEX iCount = sf.GetNumInstruments();
	for(INSTRUMENTINDEX i = 1; i <= iCount; i++)
	{
		if(sf.Instruments[i] != nullptr && sf.Instruments[i]->pTuning != 0)
			return true;
	}
	return false;
}

static void WriteTuningCollection(std::ostream& oStrm, const CTuningCollection& tc) {tc.Serialize(oStrm);}

static void WriteTuningMap(std::ostream& oStrm, const CSoundFile& sf)
//-------------------------------------------------------------------
{
	if(sf.GetNumInstruments() > 0)
	{
		//Writing instrument tuning data: first creating
		//tuning name <-> tuning id number map,
		//and then writing the tuning id for every instrument.
		//For example if there are 6 instruments and
		//first half use tuning 'T1', and the other half
		//tuning 'T2', the output would be something like
		//T1 1 T2 2 1 1 1 2 2 2

		//Creating the tuning address <-> tuning id number map.
		typedef std::map<CTuning*, uint16> TNTS_MAP;
		typedef TNTS_MAP::iterator TNTS_MAP_ITER;
		TNTS_MAP tNameToShort_Map;

		unsigned short figMap = 0;
		for(INSTRUMENTINDEX i = 1; i <= sf.GetNumInstruments(); i++) if (sf.Instruments[i] != nullptr)
		{
			TNTS_MAP_ITER iter = tNameToShort_Map.find(sf.Instruments[i]->pTuning);
			if(iter != tNameToShort_Map.end())
				continue; //Tuning already mapped.

			tNameToShort_Map[sf.Instruments[i]->pTuning] = figMap;
			figMap++;
		}

		//...and write the map with tuning names replacing
		//the addresses.
		const uint16 tuningMapSize = static_cast<uint16>(tNameToShort_Map.size());
		oStrm.write(reinterpret_cast<const char*>(&tuningMapSize), sizeof(tuningMapSize));
		for(TNTS_MAP_ITER iter = tNameToShort_Map.begin(); iter != tNameToShort_Map.end(); iter++)
		{
			if(iter->first)
				mpt::IO::WriteSizedStringLE<uint8>(oStrm, iter->first->GetName());
			else //Case: Using original IT tuning.
				mpt::IO::WriteSizedStringLE<uint8>(oStrm, "->MPT_ORIGINAL_IT<-");

			mpt::IO::WriteIntLE<uint16>(oStrm, iter->second);
		}

		//Writing tuning data for instruments.
		for(UINT i = 1; i <= sf.GetNumInstruments(); i++)
		{
			TNTS_MAP_ITER iter = tNameToShort_Map.find(sf.Instruments[i]->pTuning);
			if(iter == tNameToShort_Map.end()) //Should never happen
			{
				sf.AddToLog("Error: 210807_1");
				return;
			}
			mpt::IO::WriteIntLE<uint16>(oStrm, iter->second);
		}
	}
}

#endif // MODPLUG_NO_FILESAVE


static void ReadTuningCollection(std::istream& iStrm, CTuningCollection& tc, const size_t) {tc.Deserialize(iStrm);}

template<class TUNNUMTYPE, class STRSIZETYPE>
static bool ReadTuningMapTemplate(std::istream& iStrm, std::map<uint16, std::string>& shortToTNameMap, const size_t maxNum = 500)
//-------------------------------------------------------------------------------------------------------------------------------
{
	TUNNUMTYPE numTuning = 0;
	iStrm.read(reinterpret_cast<char*>(&numTuning), sizeof(numTuning));
	if(numTuning > maxNum)
		return true;

	for(size_t i = 0; i<numTuning; i++)
	{
		std::string temp;
		uint16 ui;
		if(!mpt::IO::ReadSizedStringLE<STRSIZETYPE>(iStrm, temp, 255))
			return true;

		iStrm.read(reinterpret_cast<char*>(&ui), sizeof(ui));
		shortToTNameMap[ui] = temp;
	}
	if(iStrm.good())
		return false;
	else
		return true;
}


static void ReadTuningMap(std::istream& iStrm, CSoundFile& csf, const size_t = 0)
//-------------------------------------------------------------------------------
{
	typedef std::map<uint16, std::string> MAP;
	typedef MAP::iterator MAP_ITER;
	MAP shortToTNameMap;
	ReadTuningMapTemplate<uint16, uint8>(iStrm, shortToTNameMap);

	//Read & set tunings for instruments
	std::vector<std::string> notFoundTunings;
	for(INSTRUMENTINDEX i = 1; i<=csf.GetNumInstruments(); i++)
	{
		uint16 ui;
		iStrm.read(reinterpret_cast<char*>(&ui), sizeof(ui));
		MAP_ITER iter = shortToTNameMap.find(ui);
		if(csf.Instruments[i] && iter != shortToTNameMap.end())
		{
			const std::string str = iter->second;

			if(str == "->MPT_ORIGINAL_IT<-")
			{
				csf.Instruments[i]->pTuning = nullptr;
				continue;
			}

			csf.Instruments[i]->pTuning = csf.GetTuneSpecificTunings().GetTuning(str);
			if(csf.Instruments[i]->pTuning)
				continue;

#ifdef MODPLUG_TRACKER
			csf.Instruments[i]->pTuning = csf.GetLocalTunings().GetTuning(str);
			if(csf.Instruments[i]->pTuning)
				continue;
#endif

			csf.Instruments[i]->pTuning = csf.GetBuiltInTunings().GetTuning(str);
			if(csf.Instruments[i]->pTuning)
				continue;

			if(str == "TET12" && csf.GetBuiltInTunings().GetNumTunings() > 0)
				csf.Instruments[i]->pTuning = &csf.GetBuiltInTunings().GetTuning(0);

			if(csf.Instruments[i]->pTuning)
				continue;

			//Checking if not found tuning already noticed.
			std::vector<std::string>::iterator iter;
			iter = std::find(notFoundTunings.begin(), notFoundTunings.end(), str);
			if(iter == notFoundTunings.end())
			{
				notFoundTunings.push_back(str);
				std::string erm = std::string("Tuning ") + str + std::string(" used by the module was not found.");
				csf.AddToLog(erm);
#ifdef MODPLUG_TRACKER
				if(csf.GetpModDoc() != nullptr)
				{
					csf.GetpModDoc()->SetModified(); //The tuning is changed so the modified flag is set.
				}
#endif // MODPLUG_TRACKER

			}
			csf.Instruments[i]->pTuning = csf.GetDefaultTuning();

		} else
		{
			//This 'else' happens probably only in case of corrupted file.
			if(csf.Instruments[i])
				csf.Instruments[i]->pTuning = csf.GetDefaultTuning();
		}

	}
	//End read&set instrument tunings
}



//////////////////////////////////////////////////////////
// Impulse Tracker IT file support

#ifndef MODPLUG_NO_FILESAVE

static uint8 ConvertVolParam(const ModCommand *m)
//-----------------------------------------------
{
	return MIN(m->vol, 9);
}

#endif // MODPLUG_NO_FILESAVE


size_t CSoundFile::ITInstrToMPT(FileReader &file, ModInstrument &ins, uint16 trkvers)
//-----------------------------------------------------------------------------------
{
	if(trkvers < 0x0200)
	{
		// Load old format (IT 1.xx) instrument
		ITOldInstrument instrumentHeader;
		if(!file.ReadConvertEndianness(instrumentHeader))
		{
			return 0;
		} else
		{
			instrumentHeader.ConvertToMPT(ins);
			return sizeof(ITOldInstrument);
		}
	} else
	{
		const FileReader::off_t offset = file.GetPosition();

		// Try loading extended instrument... instSize will differ between normal and extended instruments.
		ITInstrumentEx instrumentHeader;
		file.ReadStructPartial(instrumentHeader);
		instrumentHeader.ConvertEndianness();
		size_t instSize = instrumentHeader.ConvertToMPT(ins, GetType());
		file.Seek(offset + instSize);

		// Try reading modular instrument data
		instSize += LoadModularInstrumentData(file, ins);

		return instSize;
	}
}


static void CopyPatternName(CPattern &pattern, FileReader &file)
//--------------------------------------------------------------
{
	char name[MAX_PATTERNNAME] = "";
	file.ReadString<mpt::String::maybeNullTerminated>(name, MAX_PATTERNNAME);
	pattern.SetName(name);
}


// Get version of Schism Tracker that was used to create an IT/S3M file.
std::string CSoundFile::GetSchismTrackerVersion(uint16 cwtv)
//----------------------------------------------------------
{
	cwtv &= 0xFFF;
	std::string version;
	if(cwtv > 0x050)
	{
		tm epoch, *verTime;
		MemsetZero(epoch);
		epoch.tm_year = 109, epoch.tm_mon = 9; epoch.tm_mday = 31;
		time_t versionSec = ((cwtv - 0x050) * 86400) + mktime(&epoch);
		if((verTime = localtime(&versionSec)) != nullptr)
		{
			version = mpt::String::Print("Schism Tracker %1-%2-%3",
				mpt::fmt::dec0<4>(verTime->tm_year + 1900),
				mpt::fmt::dec0<2>(verTime->tm_mon + 1),
				mpt::fmt::dec0<2>(verTime->tm_mday));
		}
	} else
	{
		version = mpt::String::Print("Schism Tracker 0.%1", mpt::fmt::hex(cwtv & 0xFF));
	}
	return version;
}


bool CSoundFile::ReadIT(FileReader &file, ModLoadingFlags loadFlags)
//------------------------------------------------------------------
{
	file.Rewind();

	ITFileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| (memcmp(fileHeader.id, "IMPM", 4) && memcmp(fileHeader.id, "tpm.", 4))
		|| fileHeader.insnum > 0xFF
		|| fileHeader.smpnum >= MAX_SAMPLES
		|| !file.CanRead(fileHeader.ordnum + (fileHeader.insnum + fileHeader.smpnum + fileHeader.patnum) * 4))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals();

	bool interpretModPlugMade = false;

	// OpenMPT crap at the end of file
	file.Seek(file.GetLength() - 4);
	size_t mptStartPos = file.ReadUint32LE();
	if(mptStartPos >= file.GetLength() || mptStartPos < 0x100)
	{
		mptStartPos = file.GetLength();
	}

	if(!memcmp(fileHeader.id, "tpm.", 4))
	{
		// Legacy MPTM files (old 1.17.02.xx releases)
		ChangeModTypeTo(MOD_TYPE_MPT);
	} else
	{
		if(mptStartPos <= file.GetLength() - 3 && fileHeader.cwtv > 0x888 && fileHeader.cwtv <= 0xFFF)
		{
			file.Seek(mptStartPos);
			ChangeModTypeTo(file.ReadMagic("228") ? MOD_TYPE_MPT : MOD_TYPE_IT);
		} else
		{
			ChangeModTypeTo(MOD_TYPE_IT);
		}

		if(GetType() == MOD_TYPE_IT)
		{
			// Which tracker was used to made this?
			if((fileHeader.cwtv & 0xF000) == 0x5000)
			{
				// OpenMPT Version number (Major.Minor)
				// This will only be interpreted as "made with ModPlug" (i.e. disable compatible playback etc) if the "reserved" field is set to "OMPT" - else, compatibility was used.
				m_dwLastSavedWithVersion = (fileHeader.cwtv & 0x0FFF) << 16;
				if(!memcmp(fileHeader.reserved, "OMPT", 4))
					interpretModPlugMade = true;
			} else if(fileHeader.cmwt == 0x888 || fileHeader.cwtv == 0x888)
			{
				// OpenMPT 1.17 and 1.18 (raped IT format)
				// Exact version number will be determined later.
				interpretModPlugMade = true;
			} else if(fileHeader.cwtv == 0x0217 && fileHeader.cmwt == 0x0200 && !memcmp(fileHeader.reserved, "\0\0\0\0", 4))
			{
				if(memchr(fileHeader.chnpan, 0xFF, sizeof(fileHeader.chnpan)) != nullptr)
				{
					// ModPlug Tracker 1.16 (semi-raped IT format)
					m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 16, 00, 00);
					madeWithTracker = "ModPlug Tracker 1.09 - 1.16";
				} else
				{
					// OpenMPT 1.17 disguised as this in compatible mode,
					// but never writes 0xFF in the pan map for unused channels (which is an invalid value).
					m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 17, 00, 00);
					madeWithTracker = "OpenMPT 1.17 (compatibility export)";
				}
				interpretModPlugMade = true;
			} else if(fileHeader.cwtv == 0x0214 && fileHeader.cmwt == 0x0202 && !memcmp(fileHeader.reserved, "\0\0\0\0", 4))
			{
				// ModPlug Tracker b3.3 - 1.09, instruments 557 bytes apart
				m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 09, 00, 00);
				madeWithTracker = "ModPlug Tracker b3.3 - 1.09";
				interpretModPlugMade = true;
			}
		} else // case: type == MOD_TYPE_MPT
		{
			if (fileHeader.cwtv >= verMptFileVerLoadLimit)
			{
				AddToLog(str_LoadingIncompatibleVersion);
				return false;
			}
			else if (fileHeader.cwtv > verMptFileVer)
			{
				AddToLog(str_LoadingMoreRecentVersion);
			}
		}
	}

	if(GetType() == MOD_TYPE_IT) mptStartPos = file.GetLength();

	m_SongFlags.set(SONG_LINEARSLIDES, (fileHeader.flags & ITFileHeader::linearSlides) != 0);
	m_SongFlags.set(SONG_ITOLDEFFECTS, (fileHeader.flags & ITFileHeader::itOldEffects) != 0);
	m_SongFlags.set(SONG_ITCOMPATGXX, (fileHeader.flags & ITFileHeader::itCompatGxx) != 0);
	m_SongFlags.set(SONG_EMBEDMIDICFG, (fileHeader.flags & ITFileHeader::reqEmbeddedMIDIConfig) || (fileHeader.special & ITFileHeader::embedMIDIConfiguration));
	m_SongFlags.set(SONG_EXFILTERRANGE, (fileHeader.flags & ITFileHeader::extendedFilterRange) != 0);

	mpt::String::Read<mpt::String::spacePadded>(songName, fileHeader.songname);

	// Global Volume
	m_nDefaultGlobalVolume = fileHeader.globalvol << 1;
	if(m_nDefaultGlobalVolume > MAX_GLOBAL_VOLUME) m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	if(fileHeader.speed) m_nDefaultSpeed = fileHeader.speed;
	m_nDefaultTempo.Set(std::max(uint8(31), fileHeader.tempo));
	m_nSamplePreAmp = std::min(fileHeader.mv, uint8(128));

	// Reading Channels Pan Positions
	for(CHANNELINDEX i = 0; i < 64; i++) if(fileHeader.chnpan[i] != 0xFF)
	{
		ChnSettings[i].Reset();
		ChnSettings[i].nVolume = Clamp(fileHeader.chnvol[i], uint8(0), uint8(64));
		if(fileHeader.chnpan[i] & 0x80) ChnSettings[i].dwFlags.set(CHN_MUTE);
		uint8 n = fileHeader.chnpan[i] & 0x7F;
		if(n <= 64) ChnSettings[i].nPan = n * 4;
		if(n == 100) ChnSettings[i].dwFlags.set(CHN_SURROUND);
	}

	// Reading orders
	file.Seek(sizeof(ITFileHeader));
	if(GetType() == MOD_TYPE_IT)
	{
		Order.ReadAsByte(file, fileHeader.ordnum, fileHeader.ordnum, 0xFF, 0xFE);
	} else
	{
		if(fileHeader.cwtv > 0x88A && fileHeader.cwtv <= 0x88D)
		{
			Order.Deserialize(file);
		} else
		{
			Order.ReadAsByte(file, fileHeader.ordnum, fileHeader.ordnum, 0xFF, 0xFE);
		}
	}

	// Reading instrument, sample and pattern offsets
	std::vector<uint32> insPos, smpPos, patPos;
	file.ReadVectorLE(insPos, fileHeader.insnum);
	file.ReadVectorLE(smpPos, fileHeader.smpnum);
	file.ReadVectorLE(patPos, fileHeader.patnum);

	// Find the first parapointer.
	// This is used for finding out whether the edit history is actually stored in the file or not,
	// as some early versions of Schism Tracker set the history flag, but didn't save anything.
	// We will consider the history invalid if it ends after the first parapointer.
	uint32 minPtr = Util::MaxValueOfType(minPtr);
	for(uint16 n = 0; n < fileHeader.insnum; n++)
	{
		if(insPos[n] > 0) minPtr = std::min(minPtr, insPos[n]);
	}

	for(uint16 n = 0; n < fileHeader.smpnum; n++)
	{
		if(smpPos[n] > 0) minPtr = std::min(minPtr, smpPos[n]);
	}

	for(uint16 n = 0; n < fileHeader.patnum; n++)
	{
		if(patPos[n] > 0) minPtr = std::min(minPtr, patPos[n]);
	}

	if(fileHeader.special & ITFileHeader::embedSongMessage)
	{
		minPtr = std::min(minPtr, fileHeader.msgoffset);
	}

	// Reading IT Edit History Info
	// This is only supposed to be present if bit 1 of the special flags is set.
	// However, old versions of Schism and probably other trackers always set this bit
	// even if they don't write the edit history count. So we have to filter this out...
	// This is done by looking at the parapointers. If the history data ends after
	// the first parapointer, we assume that it's actually no history data.
	if(fileHeader.special & ITFileHeader::embedEditHistory)
	{
		const uint16 nflt = file.ReadUint16LE();

		if(file.CanRead(nflt * sizeof(ITHistoryStruct)) && file.GetPosition() + nflt * sizeof(ITHistoryStruct) <= minPtr)
		{
			m_FileHistory.reserve(nflt);
			for(size_t n = 0; n < nflt; n++)
			{
				FileHistory mptHistory;
				ITHistoryStruct itHistory;
				file.ReadConvertEndianness(itHistory);
				itHistory.ConvertToMPT(mptHistory);
				m_FileHistory.push_back(mptHistory);
			}
		} else
		{
			// Oops, we were not supposed to read this.
			file.SkipBack(2);
		}
	} else if(fileHeader.highlight_major == 0 && fileHeader.highlight_minor == 0 && fileHeader.cmwt == 0x0214 && fileHeader.cwtv == 0x0214 && !memcmp(fileHeader.reserved, "\0\0\0\0", 4) && (fileHeader.special & (ITFileHeader::embedEditHistory | ITFileHeader::embedPatternHighlights)) == 0)
	{
		// Another non-conforming application is unmo3 < v2.4.0.1, which doesn't set the special bit
		// at all, but still writes the two edit history length bytes (zeroes)...
		if(file.ReadUint16LE() != 0)
		{
			// These were not zero bytes -> We're in the wrong place!
			file.SkipBack(2);
			madeWithTracker = "UNMO3";
		}
	}

	// Reading MIDI Output & Macros
	if(m_SongFlags[SONG_EMBEDMIDICFG] && file.ReadStruct<MIDIMacroConfigData>(m_MidiCfg))
	{
			m_MidiCfg.Sanitize();
	}

	// Ignore MIDI data. Fixes some files like denonde.it that were made with old versions of Impulse Tracker (which didn't support Zxx filters) and have Zxx effects in the patterns.
	if(fileHeader.cwtv < 0x0214)
	{
		MemsetZero(m_MidiCfg.szMidiSFXExt);
		MemsetZero(m_MidiCfg.szMidiZXXExt);
		m_SongFlags.set(SONG_EMBEDMIDICFG);
	}

	// Read pattern names: "PNAM"
	FileReader patNames;
	if(file.ReadMagic("PNAM"))
	{
		patNames = file.ReadChunk(file.ReadUint32LE());
	}

	m_nChannels = GetModSpecifications().channelsMin;
	// Read channel names: "CNAM"
	if(file.ReadMagic("CNAM"))
	{
		FileReader chnNames = file.ReadChunk(file.ReadUint32LE());
		const CHANNELINDEX readChns = std::min(MAX_BASECHANNELS, static_cast<CHANNELINDEX>(chnNames.GetLength() / MAX_CHANNELNAME));
		m_nChannels = readChns;

		for(CHANNELINDEX i = 0; i < readChns; i++)
		{
			chnNames.ReadString<mpt::String::maybeNullTerminated>(ChnSettings[i].szName, MAX_CHANNELNAME);
		}
	}

	// Read mix plugins information
	if(file.CanRead(9))
	{
		LoadMixPlugins(file);
	}

	// Read Song Message
	if(fileHeader.special & ITFileHeader::embedSongMessage)
	{
		if(fileHeader.msglength > 0 && file.Seek(fileHeader.msgoffset))
		{
			// Generally, IT files should use CR for line endings. However, ChibiTracker uses LF. One could do...
			// if(itHeader.cwtv == 0x0214 && itHeader.cmwt == 0x0214 && itHeader.reserved == ITFileHeader::chibiMagic) --> Chibi detected.
			// But we'll just use autodetection here:
			songMessage.Read(file, fileHeader.msglength, SongMessage::leAutodetect);
		}
	}

	// Reading Instruments
	m_nInstruments = 0;
	if(fileHeader.flags & ITFileHeader::instrumentMode)
	{
		m_nInstruments = std::min(fileHeader.insnum, INSTRUMENTINDEX(MAX_INSTRUMENTS - 1));
	}
	for(INSTRUMENTINDEX i = 0; i < GetNumInstruments(); i++)
	{
		if(insPos[i] > 0 && file.Seek(insPos[i]) && file.CanRead(fileHeader.cmwt < 0x200 ? sizeof(ITOldInstrument) : sizeof(ITInstrument)))
		{
			ModInstrument *instrument = AllocateInstrument(i + 1);
			if(instrument != nullptr)
			{
				ITInstrToMPT(file, *instrument, fileHeader.cmwt);
				// MIDI Pitch Wheel Depth is a global setting in IT. Apply it to all instruments.
				instrument->midiPWD = fileHeader.pwd;
			}
		}
	}

	// In order to properly compute the position, in file, of eventual extended settings
	// such as "attack" we need to keep the "real" size of the last sample as those extra
	// setting will follow this sample in the file
	FileReader::off_t lastSampleOffset = 0;
	if(fileHeader.smpnum > 0)
	{
		lastSampleOffset = smpPos[fileHeader.smpnum - 1] + sizeof(ITSample);
	}

	// Reading Samples
	m_nSamples = std::min(fileHeader.smpnum, SAMPLEINDEX(MAX_SAMPLES - 1));
	for(SAMPLEINDEX i = 0; i < GetNumSamples(); i++)
	{
		ITSample sampleHeader;
		if(smpPos[i] > 0 && file.Seek(smpPos[i]) && file.ReadConvertEndianness(sampleHeader))
		{
			if(!memcmp(sampleHeader.id, "IMPS", 4))
			{
				ModSample &sample = Samples[i + 1];
				size_t sampleOffset = sampleHeader.ConvertToMPT(sample);

				mpt::String::Read<mpt::String::spacePadded>(m_szNames[i + 1], sampleHeader.name);

				if((loadFlags & loadSampleData) && file.Seek(sampleOffset))
				{
					if(!sample.uFlags[SMP_KEEPONDISK])
					{
						sampleHeader.GetSampleFormat(fileHeader.cwtv).ReadSample(sample, file);
					} else
					{
						// External sample in MPTM file
						size_t strLen;
						file.ReadVarInt(strLen);
						std::string filenameU8;
						file.ReadString<mpt::String::maybeNullTerminated>(filenameU8, strLen);
#ifdef MPT_EXTERNAL_SAMPLES
						SetSamplePath(i + 1, mpt::PathString::FromUTF8(filenameU8));
#else
						AddToLog(LogWarning, mpt::String::Print(MPT_USTRING("Loading external sample %1 ('%2') failed: External samples are not supported."), i, mpt::ToUnicode(mpt::CharsetUTF8, filenameU8)));
#endif // MPT_EXTERNAL_SAMPLES
					}
					lastSampleOffset = std::max(lastSampleOffset, file.GetPosition());
				}
			}
		}
	}
	m_nSamples = std::max(SAMPLEINDEX(1), GetNumSamples());

	m_nMinPeriod = 0;
	m_nMaxPeriod = int32_max;

	PATTERNINDEX numPats = std::min(static_cast<PATTERNINDEX>(patPos.size()), GetModSpecifications().patternsMax);

	if(numPats != patPos.size())
	{
		// Hack: Notify user here if file contains more patterns than what can be read.
		AddToLog(mpt::String::Print(str_PatternSetTruncationNote, patPos.size(), numPats));
	}

	if(!(loadFlags & loadPatternData))
	{
		numPats = 0;
	}

	// Checking for number of used channels, which is not explicitely specified in the file.
	for(PATTERNINDEX pat = 0; pat < numPats; pat++)
	{
		if(patPos[pat] == 0 || !file.Seek(patPos[pat]))
			continue;

		uint16 len = file.ReadUint16LE();
		ROWINDEX numRows = file.ReadUint16LE();

		if(numRows < GetModSpecifications().patternRowsMin
			|| numRows > GetModSpecifications().patternRowsMax
			|| !file.Skip(4))
			continue;

		FileReader patternData = file.ReadChunk(len);
		ROWINDEX row = 0;
		std::vector<uint8> chnMask(GetNumChannels());

		while(row < numRows && patternData.CanRead(1))
		{
			uint8 b = patternData.ReadUint8();
			if(!b)
			{
				row++;
				continue;
			}

			CHANNELINDEX ch = (b & IT_bitmask_patternChanField_c);   // 0x7f We have some data grab a byte keeping only 7 bits
			if(ch)
			{
				ch = (ch - 1);// & IT_bitmask_patternChanMask_c;   // 0x3f mask of the byte again, keeping only 6 bits
			}

			if(ch >= chnMask.size())
			{
				chnMask.resize(ch + 1, 0);
			}

			if(b & IT_bitmask_patternChanEnabled_c)            // 0x80 check if the upper bit is enabled.
			{
				chnMask[ch] = patternData.ReadUint8();       // set the channel mask for this channel.
			}
			// Channel used
			if(chnMask[ch] & 0x0F)         // if this channel is used set m_nChannels
			{
				if(ch >= GetNumChannels() && ch < MAX_BASECHANNELS)
				{
					m_nChannels = ch + 1;
				}
			}
			// Now we actually update the pattern-row entry the note,instrument etc.
			// Note
			if(chnMask[ch] & 1) patternData.Skip(1);
			// Instrument
			if(chnMask[ch] & 2) patternData.Skip(1);
			// Volume
			if(chnMask[ch] & 4) patternData.Skip(1);
			// Effect
			if(chnMask[ch] & 8) patternData.Skip(2);
		}
	}

	// Compute extra instruments settings position
	if(lastSampleOffset > 0)
	{
		file.Seek(lastSampleOffset);
	}

	// Load instrument and song extensions.
	LoadExtendedInstrumentProperties(file, &interpretModPlugMade);
	if(interpretModPlugMade)
	{
		m_nMixLevels = mixLevelsOriginal;
	}
	// We need to do this here, because if there no samples (so lastSampleOffset = 0), we need to look after the last pattern (sample data normally follows pattern data).
	// And we need to do this before reading the patterns because m_nChannels might be modified by LoadExtendedSongProperties. *sigh*
	LoadExtendedSongProperties(GetType(), file, &interpretModPlugMade);

	// Reading Patterns
	Patterns.ResizeArray(std::max(MAX_PATTERNS, numPats));
	for(PATTERNINDEX pat = 0; pat < numPats; pat++)
	{
		if(patPos[pat] == 0 || !file.Seek(patPos[pat]))
		{
			// Empty 64-row pattern
			if(!Patterns.Insert(pat, 64))
			{
				AddToLog(mpt::String::Print("Allocating patterns failed starting from pattern %1", pat));
				break;
			}
			// Now (after the Insert() call), we can read the pattern name.
			CopyPatternName(Patterns[pat], patNames);
			continue;
		}

		uint16 len = file.ReadUint16LE();
		ROWINDEX numRows = file.ReadUint16LE();

		if(numRows < GetModSpecifications().patternRowsMin
			|| numRows > GetModSpecifications().patternRowsMax
			|| !file.Skip(4)
			|| !Patterns.Insert(pat, numRows))
			continue;
			
		FileReader patternData = file.ReadChunk(len);

		// Now (after the Insert() call), we can read the pattern name.
		CopyPatternName(Patterns[pat], patNames);

		std::vector<uint8> chnMask(GetNumChannels());
		std::vector<ModCommand> lastValue(GetNumChannels(), ModCommand::Empty());

		ModCommand *patData = Patterns[pat];
		ROWINDEX row = 0;
		while(row < numRows && patternData.CanRead(1))
		{
			uint8 b = patternData.ReadUint8();
			if(!b)
			{
				row++;
				patData += GetNumChannels();
				continue;
			}

			CHANNELINDEX ch = b & IT_bitmask_patternChanField_c; // 0x7f

			if(ch)
			{
				ch = (ch - 1); //& IT_bitmask_patternChanMask_c; // 0x3f
			}

			if(ch >= chnMask.size())
			{
				chnMask.resize(ch + 1, 0);
				lastValue.resize(ch + 1, ModCommand::Empty());
				MPT_ASSERT(chnMask.size() <= GetNumChannels());
			}

			if(b & IT_bitmask_patternChanEnabled_c)  // 0x80
			{
				chnMask[ch] = patternData.ReadUint8();
			}

			// Now we grab the data for this particular row/channel.
			ModCommand dummy;
			ModCommand &m = ch < m_nChannels ? patData[ch] : dummy;

			if(chnMask[ch] & 0x10)
			{
				m.note = lastValue[ch].note;
			}
			if(chnMask[ch] & 0x20)
			{
				m.instr = lastValue[ch].instr;
			}
			if(chnMask[ch] & 0x40)
			{
				m.volcmd = lastValue[ch].volcmd;
				m.vol = lastValue[ch].vol;
			}
			if(chnMask[ch] & 0x80)
			{
				m.command = lastValue[ch].command;
				m.param = lastValue[ch].param;
			}
			if(chnMask[ch] & 1)	// Note
			{
				uint8 note = patternData.ReadUint8();
				if(note < 0x80) note += NOTE_MIN;
				if(!(GetType() & MOD_TYPE_MPT))
				{
					if(note > NOTE_MAX && note < 0xFD) note = NOTE_FADE;
					else if(note == 0xFD) note = NOTE_NONE;
				}
				m.note = note;
				lastValue[ch].note = note;
			}
			if(chnMask[ch] & 2)
			{
				uint8 instr = patternData.ReadUint8();
				m.instr = instr;
				lastValue[ch].instr = instr;
			}
			if(chnMask[ch] & 4)
			{
				uint8 vol = patternData.ReadUint8();
				// 0-64: Set Volume
				if(vol <= 64) { m.volcmd = VOLCMD_VOLUME; m.vol = vol; } else
				// 128-192: Set Panning
				if(vol >= 128 && vol <= 192) { m.volcmd = VOLCMD_PANNING; m.vol = vol - 128; } else
				// 65-74: Fine Volume Up
				if(vol < 75) { m.volcmd = VOLCMD_FINEVOLUP; m.vol = vol - 65; } else
				// 75-84: Fine Volume Down
				if(vol < 85) { m.volcmd = VOLCMD_FINEVOLDOWN; m.vol = vol - 75; } else
				// 85-94: Volume Slide Up
				if(vol < 95) { m.volcmd = VOLCMD_VOLSLIDEUP; m.vol = vol - 85; } else
				// 95-104: Volume Slide Down
				if(vol < 105) { m.volcmd = VOLCMD_VOLSLIDEDOWN; m.vol = vol - 95; } else
				// 105-114: Pitch Slide Up
				if(vol < 115) { m.volcmd = VOLCMD_PORTADOWN; m.vol = vol - 105; } else
				// 115-124: Pitch Slide Down
				if(vol < 125) { m.volcmd = VOLCMD_PORTAUP; m.vol = vol - 115; } else
				// 193-202: Portamento To
				if(vol >= 193 && vol <= 202) { m.volcmd = VOLCMD_TONEPORTAMENTO; m.vol = vol - 193; } else
				// 203-212: Vibrato depth
				if(vol >= 203 && vol <= 212)
				{
					m.volcmd = VOLCMD_VIBRATODEPTH; m.vol = vol - 203;
					// Old versions of ModPlug saved this as vibrato speed instead, so let's fix that.
					if(m.vol && m_dwLastSavedWithVersion && m_dwLastSavedWithVersion <= MAKE_VERSION_NUMERIC(1, 17, 02, 54))
						m.volcmd = VOLCMD_VIBRATOSPEED;
				} else
				// 213-222: Unused (was velocity)
				// 223-232: Offset
				if(vol >= 223 && vol <= 232) { m.volcmd = VOLCMD_OFFSET; m.vol = vol - 223; }
				lastValue[ch].volcmd = m.volcmd;
				lastValue[ch].vol = m.vol;
			}
			// Reading command/param
			if(chnMask[ch] & 8)
			{
				m.command = patternData.ReadUint8();
				m.param = patternData.ReadUint8();
				S3MConvert(m, true);
				// In some IT-compatible trackers, it is possible to input an parameter without a command.
				// In this case, we still need to update the last value memory. OpenMPT didn't do this until v1.25.01.07.
				// Example: ckbounce.it
				lastValue[ch].command = m.command;
				lastValue[ch].param = m.param;
			}
		}
	}

	UpgradeModFlags();

	if(!m_dwLastSavedWithVersion && fileHeader.cwtv == 0x0888)
	{
		// Up to OpenMPT 1.17.02.45 (r165), it was possible that the "last saved with" field was 0
		// when saving a file in OpenMPT for the first time.
		m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 17, 00, 00);
	}

	if(m_dwLastSavedWithVersion && madeWithTracker.empty())
	{
		madeWithTracker = "OpenMPT " + MptVersion::ToStr(m_dwLastSavedWithVersion);
		if(memcmp(fileHeader.reserved, "OMPT", 4) && (fileHeader.cwtv & 0xF000) == 0x5000)
		{
			madeWithTracker += " (compatibility export)";
		} else if(MptVersion::IsTestBuild(m_dwLastSavedWithVersion))
		{
			madeWithTracker += " (test build)";
		}
	} else
	{
		switch(fileHeader.cwtv >> 12)
		{
		case 0:
			if(!madeWithTracker.empty())
			{
				// BeRoTracker has been detected above.
			} else if(fileHeader.cwtv == 0x0214 && fileHeader.cmwt == 0x0200 && fileHeader.flags == 9 && fileHeader.special == 0
				&& fileHeader.highlight_major == 0 && fileHeader.highlight_minor == 0
				&& fileHeader.insnum == 0 && fileHeader.patnum + 1 == fileHeader.ordnum
				&& fileHeader.globalvol == 128 && fileHeader.mv == 100 && fileHeader.speed == 1 && fileHeader.sep == 128 && fileHeader.pwd == 0
				&& fileHeader.msglength == 0 && fileHeader.msgoffset == 0 && !memcmp(fileHeader.reserved, "\0\0\0\0", 4))
			{
				madeWithTracker = "OpenSPC conversion";
			} else if(fileHeader.cwtv == 0x0214 && fileHeader.cmwt == 0x0200 && !memcmp(fileHeader.reserved, "\0\0\0\0", 4))
			{
				// ModPlug Tracker 1.00a5, instruments 560 bytes apart
				m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 00, 00, A5);
				madeWithTracker = "ModPlug Tracker 1.00a5";
				interpretModPlugMade = true;
			} else if(fileHeader.cwtv == 0x0214 && fileHeader.cmwt == 0x0214 && !memcmp(fileHeader.reserved, "CHBI", 4))
			{
				madeWithTracker = "ChibiTracker";
			} else if(fileHeader.cwtv == 0x0214 && fileHeader.cmwt == 0x0214 && !(fileHeader.special & 3) && !memcmp(fileHeader.reserved, "\0\0\0\0", 4) && !strcmp(Samples[1].filename, "XXXXXXXX.YYY"))
			{
				madeWithTracker = "CheeseTracker";
			} else if(fileHeader.cmwt < 0x0888)
			{
				if(fileHeader.cmwt > 0x0214)
				{
					madeWithTracker = "Impulse Tracker 2.15";
				} else if(fileHeader.cwtv > 0x0214)
				{
					// Patched update of IT 2.14 (0x0215 - 0x0217 == p1 - p3)
					// p4 (as found on modland) adds the ITVSOUND driver, but doesn't seem to change
					// anything as far as file saving is concerned.
					madeWithTracker = mpt::String::Print("Impulse Tracker 2.14p%1", fileHeader.cwtv - 0x0214);
				} else
				{
					madeWithTracker = mpt::String::Print("Impulse Tracker %1.%2", (fileHeader.cwtv & 0x0F00) >> 8, mpt::fmt::hex0<2>((fileHeader.cwtv & 0xFF)));
				}
				if(m_FileHistory.empty() && memcmp(fileHeader.reserved, "\0\0\0\0", 4))
				{
					// IT encrypts the total edit time of a module in the "reserved" fild
					uint32 editTime;
					memcpy(&editTime, fileHeader.reserved, 4);
					SwapBytesLE(editTime);
					if(fileHeader.cwtv >= 0x0208)
					{
						editTime ^= 0x4954524B;	// 'ITRK'
						editTime = (editTime >> 7) | (editTime << (32 - 7));
						editTime = -(int32)editTime;
						editTime = (editTime << 4) | (editTime >> (32 - 4));
						editTime ^= 0x4A54484C;	// 'JTHL'
					}

					FileHistory hist;
					MemsetZero(hist);
					hist.openTime = static_cast<uint32>(editTime * (HISTORY_TIMER_PRECISION / 18.2f));
					m_FileHistory.push_back(hist);
				}
			}
			break;
		case 1:
			madeWithTracker = GetSchismTrackerVersion(fileHeader.cwtv);
			break;
		case 4:
			madeWithTracker = mpt::String::Print("pyIT %1.%2", (fileHeader.cwtv & 0x0F00) >> 8, mpt::fmt::hex0<2>((fileHeader.cwtv & 0xFF)));
			break;
		case 6:
			madeWithTracker = "BeRoTracker";
			break;
		case 7:
			madeWithTracker = mpt::String::Print("ITMCK %1.%2.%3", (fileHeader.cwtv >> 8) & 0x0F, (fileHeader.cwtv >> 4) & 0x0F, fileHeader.cwtv & 0x0F);
			break;
		case 0xD:
			madeWithTracker = "spc2it";
			break;
		}
	}

	// Read row highlights
	if((fileHeader.special & ITFileHeader::embedPatternHighlights))
	{
		// MPT 1.09 and older (and maybe also newer) versions leave this blank (0/0), but have the "special" flag set.
		// Newer versions of MPT and OpenMPT 1.17 *always* write 4/16 here.
		// Thus, we will just ignore those old versions.
		if(m_dwLastSavedWithVersion == 0 || m_dwLastSavedWithVersion >= MAKE_VERSION_NUMERIC(1, 17, 03, 02))
		{
			m_nDefaultRowsPerBeat = fileHeader.highlight_minor;
			m_nDefaultRowsPerMeasure = fileHeader.highlight_major;
		}
	}

	if(GetType() == MOD_TYPE_IT)
	{
		// Set appropriate mod flags if the file was not made with MPT.
		if(!interpretModPlugMade)
		{
			SetModFlag(MSF_MIDICC_BUGEMULATION, false);
			SetModFlag(MSF_OLDVOLSWING, false);
			SetModFlag(MSF_COMPATIBLE_PLAY, true);
		}
	} else
	{
		//START - mpt specific:
		//Using member cwtv on pifh as the version number.
		const uint16 version = fileHeader.cwtv;
		if(version > 0x889 && file.Seek(mptStartPos))
		{
			mpt::istringstream iStrm(std::string(file.GetRawData(), file.BytesLeft()));

			if(version >= 0x88D)
			{
				srlztn::SsbRead ssb(iStrm);
				ssb.BeginRead("mptm", MptVersion::num);
				ssb.ReadItem(GetTuneSpecificTunings(), "0", &ReadTuningCollection);
				ssb.ReadItem(*this, "1", &ReadTuningMap);
				ssb.ReadItem(Order, "2", &ReadModSequenceOld);
				ssb.ReadItem(Patterns, FileIdPatterns, &ReadModPatterns);
				ssb.ReadItem(Order, FileIdSequences, &ReadModSequences);

				if(ssb.GetStatus() & srlztn::SNT_FAILURE)
				{
					AddToLog("Unknown error occured while deserializing file.");
				}
			} else //Loading for older files.
			{
				if(GetTuneSpecificTunings().Deserialize(iStrm))
				{
					AddToLog("Error occured - loading failed while trying to load tune specific tunings.");
				} else
				{
					ReadTuningMap(iStrm, *this);
				}
			}
		} //version condition(MPT)
	}

	return true;
}

#ifndef MODPLUG_NO_FILESAVE

// Save edit history. Pass a null pointer for *f to retrieve the number of bytes that would be written.
static uint32 SaveITEditHistory(const CSoundFile *pSndFile, FILE *f)
//------------------------------------------------------------------
{
	size_t num = pSndFile->GetFileHistory().size();
#ifdef MODPLUG_TRACKER
	const CModDoc *pModDoc = pSndFile->GetpModDoc();
	num += (pModDoc != nullptr) ? 1 : 0;	// + 1 for this session
#endif // MODPLUG_TRACKER

	uint16 fnum = mpt::saturate_cast<uint16>(num);	// Number of entries that are actually going to be written
	const uint32 bytesWritten = 2 + fnum * 8;		// Number of bytes that are actually going to be written

	if(f == nullptr)
		return bytesWritten;

	// Write number of history entries
	mpt::IO::WriteIntLE(f, fnum);

	// Write history data
	const size_t start = (num > uint16_max) ? num - uint16_max : 0;
	for(size_t n = start; n < num; n++)
	{
		FileHistory mptHistory;

#ifdef MODPLUG_TRACKER
		if(n < pSndFile->GetFileHistory().size())
#endif // MODPLUG_TRACKER
		{
			// Previous timestamps
			mptHistory = pSndFile->GetFileHistory().at(n);
#ifdef MODPLUG_TRACKER
		} else
		{
			// Current ("new") timestamp
			const time_t creationTime = pModDoc->GetCreationTime();

			MemsetZero(mptHistory.loadDate);
			//localtime_s(&loadDate, &creationTime);
			const tm* const p = localtime(&creationTime);
			if (p != nullptr)
				mptHistory.loadDate = *p;
			else
				pSndFile->AddToLog("localtime() returned nullptr.");

			mptHistory.openTime = (uint32)(difftime(time(nullptr), creationTime) * (double)HISTORY_TIMER_PRECISION);
#endif // MODPLUG_TRACKER
		}

		ITHistoryStruct itHistory;
		itHistory.ConvertToIT(mptHistory);
		itHistory.ConvertEndianness();
		fwrite(&itHistory, 1, sizeof(itHistory), f);
	}

	return bytesWritten;
}


bool CSoundFile::SaveIT(const mpt::PathString &filename, bool compatibilityExport)
//--------------------------------------------------------------------------------
{
	const CModSpecifications &specs = (GetType() == MOD_TYPE_MPT ? ModSpecs::mptm : (compatibilityExport ? ModSpecs::it : ModSpecs::itEx));

	DWORD dwChnNamLen;
	ITFileHeader itHeader;
	DWORD dwPos = 0, dwHdrPos = 0, dwExtra = 0;
	FILE *f;

	if(filename.empty() || ((f = mpt_fopen(filename, "wb")) == NULL)) return false;

	// Writing Header
	MemsetZero(itHeader);
	dwChnNamLen = 0;
	memcpy(itHeader.id, "IMPM", 4);
	mpt::String::Write<mpt::String::nullTerminated>(itHeader.songname, songName);

	itHeader.highlight_minor = (uint8)std::min(m_nDefaultRowsPerBeat, ROWINDEX(uint8_max));
	itHeader.highlight_major = (uint8)std::min(m_nDefaultRowsPerMeasure, ROWINDEX(uint8_max));

	if(GetType() == MOD_TYPE_MPT)
	{
		if(!Order.NeedsExtraDatafield()) itHeader.ordnum = Order.size();
		else itHeader.ordnum = std::min(Order.size(), MAX_ORDERS); //Writing MAX_ORDERS at max here, and if there's more, writing them elsewhere.

		//Crop unused orders from the end.
		while(itHeader.ordnum > 1 && Order[itHeader.ordnum - 1] == Order.GetInvalidPatIndex()) itHeader.ordnum--;
	} else
	{
		// An additional "---" pattern is appended so Impulse Tracker won't ignore the last order item.
		// Interestingly, this can exceed IT's 256 order limit. Also, IT will always save at least two orders.
		itHeader.ordnum = std::min(Order.GetLengthTailTrimmed(), specs.ordersMax) + 1;
		if(itHeader.ordnum < 2) itHeader.ordnum = 2;
	}

	itHeader.insnum = std::min(m_nInstruments, specs.instrumentsMax);
	itHeader.smpnum = std::min(m_nSamples, specs.samplesMax);
	itHeader.patnum = std::min(Patterns.GetNumPatterns(), specs.patternsMax);

	// Parapointers
	std::vector<uint32> patpos(itHeader.patnum, 0);
	std::vector<uint32> smppos(itHeader.smpnum, 0);
	std::vector<uint32> inspos(itHeader.insnum, 0);

	//VERSION
	if(GetType() == MOD_TYPE_MPT)
	{
		// MPTM
		itHeader.cwtv = verMptFileVer;	// Used in OMPT-hack versioning.
		itHeader.cmwt = 0x888;
	} else
	{
		// IT
		MptVersion::VersionNum vVersion = MptVersion::num;
		itHeader.cwtv = 0x5000 | (uint16)((vVersion >> 16) & 0x0FFF); // format: txyy (t = tracker ID, x = version major, yy = version minor), e.g. 0x5117 (OpenMPT = 5, 117 = v1.17)
		itHeader.cmwt = 0x0214;	// Common compatible tracker :)
		// Hack from schism tracker:
		for(INSTRUMENTINDEX nIns = 1; nIns <= GetNumInstruments(); nIns++)
		{
			if(Instruments[nIns] && Instruments[nIns]->PitchEnv.dwFlags[ENV_FILTER])
			{
				itHeader.cmwt = 0x0216;
				break;
			}
		}

		if(!compatibilityExport)
		{
			// This way, we indicate that the file will most likely contain OpenMPT hacks. Compatibility export puts 0 here.
			memcpy(itHeader.reserved, "OMPT", 4);
		}
	}

	itHeader.flags = ITFileHeader::useStereoPlayback | ITFileHeader::useMIDIPitchController;
	itHeader.special = ITFileHeader::embedEditHistory | ITFileHeader::embedPatternHighlights;
	if(m_nInstruments) itHeader.flags |= ITFileHeader::instrumentMode;
	if(m_SongFlags[SONG_LINEARSLIDES]) itHeader.flags |= ITFileHeader::linearSlides;
	if(m_SongFlags[SONG_ITOLDEFFECTS]) itHeader.flags |= ITFileHeader::itOldEffects;
	if(m_SongFlags[SONG_ITCOMPATGXX]) itHeader.flags |= ITFileHeader::itCompatGxx;
	if(m_SongFlags[SONG_EXFILTERRANGE] && !compatibilityExport) itHeader.flags |= ITFileHeader::extendedFilterRange;

	itHeader.globalvol = (uint8)(m_nDefaultGlobalVolume >> 1);
	itHeader.mv = (uint8)MIN(m_nSamplePreAmp, 128);
	itHeader.speed = (uint8)MIN(m_nDefaultSpeed, 255);
 	itHeader.tempo = (uint8)MIN(m_nDefaultTempo.GetInt(), 255);  //Limit this one to 255, we save the real one as an extension below.
	itHeader.sep = 128; // pan separation
	// IT doesn't have a per-instrument Pitch Wheel Depth setting, so we just store the first non-zero PWD setting in the header.
	for(INSTRUMENTINDEX ins = 1; ins < GetNumInstruments(); ins++)
	{
		if(Instruments[ins] != nullptr && Instruments[ins]->midiPWD != 0)
		{
			itHeader.pwd = (uint8)abs(Instruments[ins]->midiPWD);
			break;
		}
	}

	dwHdrPos = sizeof(itHeader) + itHeader.ordnum;
	// Channel Pan and Volume
	memset(itHeader.chnpan, 0xA0, 64);
	memset(itHeader.chnvol, 64, 64);

	for(CHANNELINDEX ich = 0; ich < std::min(m_nChannels, CHANNELINDEX(64)); ich++) // Header only has room for settings for 64 chans...
	{
		itHeader.chnpan[ich] = (uint8)(ChnSettings[ich].nPan >> 2);
		if (ChnSettings[ich].dwFlags[CHN_SURROUND]) itHeader.chnpan[ich] = 100;
		itHeader.chnvol[ich] = (uint8)(ChnSettings[ich].nVolume);
		if (ChnSettings[ich].dwFlags[CHN_MUTE]) itHeader.chnpan[ich] |= 0x80;
	}

	// Channel names
	if(!compatibilityExport)
	{
		for(CHANNELINDEX i = 0; i < m_nChannels; i++)
		{
			if(ChnSettings[i].szName[0])
			{
				dwChnNamLen = (i + 1) * MAX_CHANNELNAME;
			}
		}
		if(dwChnNamLen) dwExtra += dwChnNamLen + 8;
	}

	if(m_SongFlags[SONG_EMBEDMIDICFG])
	{
		itHeader.flags |= ITFileHeader::reqEmbeddedMIDIConfig;
		itHeader.special |= ITFileHeader::embedMIDIConfiguration;
		dwExtra += sizeof(MIDIMacroConfigData);
	}

	// Pattern Names
	const PATTERNINDEX numNamedPats = compatibilityExport ? 0 : Patterns.GetNumNamedPatterns();
	if(numNamedPats > 0)
	{
		dwExtra += (numNamedPats * MAX_PATTERNNAME) + 8;
	}

	// Mix Plugins. Just calculate the size of this extra block for now.
	if(!compatibilityExport)
	{
		dwExtra += SaveMixPlugins(NULL, true);
	}

	// Edit History. Just calculate the size of this extra block for now.
	dwExtra += SaveITEditHistory(this, nullptr);

	// Comments
	uint16 msglength = 0;
	if(!songMessage.empty())
	{
		itHeader.special |= ITFileHeader::embedSongMessage;
		itHeader.msglength = msglength = mpt::saturate_cast<uint16>(songMessage.length() + 1u);
		itHeader.msgoffset = dwHdrPos + dwExtra + (itHeader.insnum + itHeader.smpnum + itHeader.patnum) * 4;
	}

	// Write file header
	itHeader.ConvertEndianness();
	fwrite(&itHeader, 1, sizeof(itHeader), f);
	// Convert endianness again as we access some of the header variables with native endianness here.
	itHeader.ConvertEndianness();

	Order.WriteAsByte(f, itHeader.ordnum);
	for(uint16 i = 0; i < itHeader.insnum; ++i)
	{
		mpt::IO::WriteIntLE<uint32>(f, inspos[i]);
	}
	for(uint16 i = 0; i < itHeader.smpnum; ++i)
	{
		mpt::IO::WriteIntLE<uint32>(f, smppos[i]);
	}
	for(uint16 i = 0; i < itHeader.patnum; ++i)
	{
		mpt::IO::WriteIntLE<uint32>(f, patpos[i]);
	}

	// Writing edit history information
	SaveITEditHistory(this, f);

	// Writing midi cfg
	if(itHeader.flags & ITFileHeader::reqEmbeddedMIDIConfig)
	{
		fwrite(static_cast<MIDIMacroConfigData*>(&m_MidiCfg), 1, sizeof(MIDIMacroConfigData), f);
	}

	// Writing pattern names
	if(numNamedPats)
	{
		char magic[4];
		memcpy(magic, "PNAM", 4);
		fwrite(magic, 4, 1, f);
		mpt::IO::WriteIntLE<uint32>(f, numNamedPats * MAX_PATTERNNAME);

		for(PATTERNINDEX nPat = 0; nPat < numNamedPats; nPat++)
		{
			char name[MAX_PATTERNNAME];
			MemsetZero(name);
			Patterns[nPat].GetName(name);
			fwrite(name, 1, MAX_PATTERNNAME, f);
		}
	}

	// Writing channel names
	if(dwChnNamLen && !compatibilityExport)
	{
		char magic[4];
		memcpy(magic, "CNAM", 4);
		fwrite(magic, 4, 1, f);
		mpt::IO::WriteIntLE<uint32>(f, dwChnNamLen);
		UINT nChnNames = dwChnNamLen / MAX_CHANNELNAME;
		for(UINT inam = 0; inam < nChnNames; inam++)
		{
			fwrite(ChnSettings[inam].szName, 1, MAX_CHANNELNAME, f);
		}
	}

	// Writing mix plugins info
	if(!compatibilityExport)
	{
		SaveMixPlugins(f, false);
	}

	// Writing song message
	dwPos = dwHdrPos + dwExtra + (itHeader.insnum + itHeader.smpnum + itHeader.patnum) * 4;
	if(itHeader.special & ITFileHeader::embedSongMessage)
	{
		dwPos += msglength;
		fwrite(songMessage.c_str(), 1, msglength, f);
	}

	// Writing instruments
	for(INSTRUMENTINDEX nins = 1; nins <= itHeader.insnum; nins++)
	{
		ITInstrumentEx iti;
		uint32 instSize;

		if(Instruments[nins])
		{
			instSize = iti.ConvertToIT(*Instruments[nins], compatibilityExport, *this);
		} else
		{
			// Save Empty Instrument
			ModInstrument dummy;
			instSize = iti.ConvertToIT(dummy, compatibilityExport, *this);
		}

		// Writing instrument
		inspos[nins - 1] = dwPos;
		dwPos += instSize;
		iti.ConvertEndianness();
		fwrite(&iti, 1, instSize, f);

		//------------ rewbs.modularInstData
		if (Instruments[nins] && !compatibilityExport)
		{
			dwPos += SaveModularInstrumentData(f, Instruments[nins]);
		}
		//------------ end rewbs.modularInstData
	}

	// Writing sample headers
	ITSample itss;
	MemsetZero(itss);
	for(SAMPLEINDEX smp = 0; smp < itHeader.smpnum; smp++)
	{
		smppos[smp] = dwPos;
		dwPos += sizeof(ITSample);
		fwrite(&itss, 1, sizeof(ITSample), f);
	}

	// Writing Patterns
	bool bNeedsMptPatSave = false;
	for(PATTERNINDEX pat = 0; pat < itHeader.patnum; pat++)
	{
		uint32 dwPatPos = dwPos;
		if (!Patterns[pat]) continue;

		if(Patterns[pat].GetOverrideSignature())
			bNeedsMptPatSave = true;

		// Check for empty pattern
		if(Patterns[pat].GetNumRows() == 64 && Patterns.IsPatternEmpty(pat))
		{
			patpos[pat] = 0;
			continue;
		}

		patpos[pat] = dwPos;

		// Write pattern header
		ROWINDEX writeRows = mpt::saturate_cast<uint16>(Patterns[pat].GetNumRows());
		uint16 patinfo[4];
		patinfo[0] = 0;
		patinfo[1] = (uint16)writeRows;
		patinfo[2] = 0;
		patinfo[3] = 0;
		SwapBytesLE(patinfo[1]);

		fwrite(patinfo, 8, 1, f);
		dwPos += 8;

		const CHANNELINDEX maxChannels = std::min(specs.channelsMax, GetNumChannels());
		std::vector<uint8> chnmask(maxChannels, 0xFF);
		std::vector<ModCommand> lastvalue(maxChannels, ModCommand::Empty());

		for(ROWINDEX row = 0; row < writeRows; row++)
		{
			uint32 len = 0;
			uint8 buf[8 * MAX_BASECHANNELS];
			const ModCommand *m = Patterns[pat].GetRow(row);

			for(CHANNELINDEX ch = 0; ch < maxChannels; ch++, m++)
			{
				// Skip mptm-specific notes.
				if(m->IsPcNote())
				{
					bNeedsMptPatSave = true;
					continue;
				}

				uint8 b = 0;
				uint8 command = m->command;
				uint8 param = m->param;
				uint8 vol = 0xFF;
				uint8 note = m->note;
				if (note != NOTE_NONE) b |= 1;
				if (m->IsNote()) note -= NOTE_MIN;
				if (note == NOTE_FADE && GetType() != MOD_TYPE_MPT) note = 0xF6;
				if (m->instr) b |= 2;
				if (m->volcmd != VOLCMD_NONE)
				{
					switch(m->volcmd)
					{
					case VOLCMD_VOLUME:			vol = m->vol; if (vol > 64) vol = 64; break;
					case VOLCMD_PANNING:		vol = m->vol + 128; if (vol > 192) vol = 192; break;
					case VOLCMD_VOLSLIDEUP:		vol = 85 + ConvertVolParam(m); break;
					case VOLCMD_VOLSLIDEDOWN:	vol = 95 + ConvertVolParam(m); break;
					case VOLCMD_FINEVOLUP:		vol = 65 + ConvertVolParam(m); break;
					case VOLCMD_FINEVOLDOWN:	vol = 75 + ConvertVolParam(m); break;
					case VOLCMD_VIBRATODEPTH:	vol = 203 + ConvertVolParam(m); break;
					case VOLCMD_VIBRATOSPEED:	if(command == CMD_NONE)
												{
													// illegal command -> move if possible
													command = CMD_VIBRATO;
													param = ConvertVolParam(m) << 4;
												} else
												{
													vol = 203;
												}
												break;
					case VOLCMD_TONEPORTAMENTO:	vol = 193 + ConvertVolParam(m); break;
					case VOLCMD_PORTADOWN:		vol = 105 + ConvertVolParam(m); break;
					case VOLCMD_PORTAUP:		vol = 115 + ConvertVolParam(m); break;
					case VOLCMD_OFFSET:			if(!compatibilityExport) vol = 223 + ConvertVolParam(m); //rewbs.volOff
												break;
					default:					vol = 0xFF;
					}
				}
				if (vol != 0xFF) b |= 4;
				if (command != CMD_NONE)
				{
					S3MSaveConvert(command, param, true, compatibilityExport);
					if (command) b |= 8;
				}
				// Packing information
				if (b)
				{
					// Same note ?
					if (b & 1)
					{
						if ((note == lastvalue[ch].note) && (lastvalue[ch].volcmd & 1))
						{
							b &= ~1;
							b |= 0x10;
						} else
						{
							lastvalue[ch].note = note;
							lastvalue[ch].volcmd |= 1;
						}
					}
					// Same instrument ?
					if (b & 2)
					{
						if ((m->instr == lastvalue[ch].instr) && (lastvalue[ch].volcmd & 2))
						{
							b &= ~2;
							b |= 0x20;
						} else
						{
							lastvalue[ch].instr = m->instr;
							lastvalue[ch].volcmd |= 2;
						}
					}
					// Same volume column byte ?
					if (b & 4)
					{
						if ((vol == lastvalue[ch].vol) && (lastvalue[ch].volcmd & 4))
						{
							b &= ~4;
							b |= 0x40;
						} else
						{
							lastvalue[ch].vol = vol;
							lastvalue[ch].volcmd |= 4;
						}
					}
					// Same command / param ?
					if (b & 8)
					{
						if ((command == lastvalue[ch].command) && (param == lastvalue[ch].param) && (lastvalue[ch].volcmd & 8))
						{
							b &= ~8;
							b |= 0x80;
						} else
						{
							lastvalue[ch].command = command;
							lastvalue[ch].param = param;
							lastvalue[ch].volcmd |= 8;
						}
					}
					if (b != chnmask[ch])
					{
						chnmask[ch] = b;
						buf[len++] = uint8((ch + 1) | IT_bitmask_patternChanEnabled_c);
						buf[len++] = b;
					} else
					{
						buf[len++] = uint8(ch + 1);
					}
					if (b & 1) buf[len++] = note;
					if (b & 2) buf[len++] = m->instr;
					if (b & 4) buf[len++] = vol;
					if (b & 8)
					{
						buf[len++] = command;
						buf[len++] = param;
					}
				}
			}
			buf[len++] = 0;
			if(patinfo[0] > uint16_max - len)
			{
				AddToLog(mpt::String::Print("%1 (%2 %3)", str_tooMuchPatternData, str_pattern, pat));
				break;
			} else
			{
				dwPos += len;
				patinfo[0] += (uint16)len;
				fwrite(buf, 1, len, f);
			}
		}
		fseek(f, dwPatPos, SEEK_SET);
		SwapBytesLE(patinfo[0]);
		fwrite(patinfo, 8, 1, f);
		fseek(f, dwPos, SEEK_SET);
	}
	// Writing Sample Data
	for(SAMPLEINDEX smp = 1; smp <= itHeader.smpnum; smp++)
	{
#ifdef MODPLUG_TRACKER
		int type = GetType() == MOD_TYPE_IT ? 1 : 4;
		if(compatibilityExport) type = 2;
		bool compress = ((((Samples[smp].GetNumChannels() > 1) ? TrackerSettings::Instance().MiscITCompressionStereo : TrackerSettings::Instance().MiscITCompressionMono) & type) != 0);
#else
		bool compress = false;
#endif // MODPLUG_TRACKER
		// Old MPT, DUMB and probably other libraries will only consider the IT2.15 compression flag if the header version also indicates IT2.15.
		// MilkyTracker <= 0.90.85 will only assume IT2.15 compression with cmwt == 0x215, ignoring the delta flag completely.
		itss.ConvertToIT(Samples[smp], GetType(), compress, itHeader.cmwt >= 0x215, GetType() == MOD_TYPE_MPT);
		const bool isExternal = itss.cvt == ITSample::cvtExternalSample;

		mpt::String::Write<mpt::String::nullTerminated>(itss.name, m_szNames[smp]);

		itss.samplepointer = dwPos;
		itss.ConvertEndianness();
		fseek(f, smppos[smp - 1], SEEK_SET);
		fwrite(&itss, 1, sizeof(ITSample), f);
		fseek(f, dwPos, SEEK_SET);
		if(!isExternal)
		{
			dwPos += itss.GetSampleFormat().WriteSample(f, Samples[smp]);
		} else
		{
#ifdef MPT_EXTERNAL_SAMPLES
			const std::string filenameU8 = GetSamplePath(smp).AbsolutePathToRelative(filename.GetPath()).ToUTF8();
			const size_t strSize = mpt::saturate_cast<uint16>(filenameU8.size());
			size_t intBytes = 0;
			if(mpt::IO::WriteVarInt(f, strSize, &intBytes))
			{
				dwPos += intBytes + strSize;
				mpt::IO::WriteRaw(f, &filenameU8[0], strSize);
			}
#endif // MPT_EXTERNAL_SAMPLES
		}
	}

	//Save hacked-on extra info
	if(!compatibilityExport)
	{
		SaveExtendedInstrumentProperties(itHeader.insnum, f);
		SaveExtendedSongProperties(f);
	}

	// Updating offsets
	fseek(f, dwHdrPos, SEEK_SET);
	for(uint16 i = 0; i < itHeader.insnum; ++i)
	{
		mpt::IO::WriteIntLE<uint32>(f, inspos[i]);
	}
	for(uint16 i = 0; i < itHeader.smpnum; ++i)
	{
		mpt::IO::WriteIntLE<uint32>(f, smppos[i]);
	}
	for(uint16 i = 0; i < itHeader.patnum; ++i)
	{
		mpt::IO::WriteIntLE<uint32>(f, patpos[i]);
	}

	if(GetType() == MOD_TYPE_IT)
	{
		fclose(f);
		return true;
	}

	//hack
	//BEGIN: MPT SPECIFIC:
	//--------------------

	bool success = true;

	fseek(f, 0, SEEK_END);
	{
	mpt::FILE_ostream fout(f);

	const uint32 MPTStartPos = (uint32)fout.tellp();
	
	// catch standard library truncating files
	MPT_ASSERT_ALWAYS(MPTStartPos > 0);

	srlztn::SsbWrite ssb(fout);
	ssb.BeginWrite("mptm", MptVersion::num);

	if(GetTuneSpecificTunings().GetNumTunings() > 0)
		ssb.WriteItem(GetTuneSpecificTunings(), "0", &WriteTuningCollection);
	if(AreNonDefaultTuningsUsed(*this))
		ssb.WriteItem(*this, "1", &WriteTuningMap);
	if(Order.NeedsExtraDatafield())
		ssb.WriteItem(Order, "2", &WriteModSequenceOld);
	if(bNeedsMptPatSave)
		ssb.WriteItem(Patterns, FileIdPatterns, &WriteModPatterns);
	ssb.WriteItem(Order, FileIdSequences, &WriteModSequences);

	ssb.FinishWrite();

	if(ssb.GetStatus() & srlztn::SNT_FAILURE)
	{
		AddToLog("Error occured in writing MPTM extensions.");
	}

	//Last 4 bytes should tell where the hack mpt things begin.
	if(!fout.good())
	{
		fout.clear();
		success = false;
	}
	mpt::IO::WriteIntLE<uint32>(fout, MPTStartPos);

	fout.seekp(0, std::ios_base::end);
	}
	fclose(f);
	f = nullptr;

	//END  : MPT SPECIFIC
	//-------------------

	//NO WRITING HERE ANYMORE.

	return success;
}


#endif // MODPLUG_NO_FILESAVE


#ifndef MODPLUG_NO_FILESAVE

UINT CSoundFile::SaveMixPlugins(FILE *f, bool bUpdate)
//----------------------------------------------------
{
	uint32 chinfo[MAX_BASECHANNELS];
	char id[4];
	uint32 nPluginSize;
	uint32 nTotalSize = 0;
	uint32 nChInfo = 0;

	for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
	{
		const SNDMIXPLUGIN &plugin = m_MixPlugins[i];
		if(plugin.IsValidPlugin())
		{
			nPluginSize = sizeof(SNDMIXPLUGININFO) + 4; // plugininfo+4 (datalen)
			if((plugin.pMixPlugin) && (bUpdate))
			{
				plugin.pMixPlugin->SaveAllParameters();
			}
			if(plugin.pPluginData)
			{
				nPluginSize += plugin.nPluginDataSize;
			}

			uint32 MPTxPlugDataSize = 4 + sizeof(float32) +		// 4 for ID and size of dryRatio
									 4 + sizeof(int32);			// Default Program
								// for each extra entity, add 4 for ID, plus 4 for size of entity, plus size of entity

			nPluginSize += MPTxPlugDataSize + 4; //+4 is for size itself: sizeof(uint32) is 4
			if(f)
			{
				// write plugin ID
				id[0] = 'F';
				id[1] = i < 100 ? 'X' : '0' + i / 100;
				id[2] = '0' + (i / 10) % 10u;
				id[3] = '0' + (i % 10u);
				fwrite(id, 1, 4, f);

				// write plugin size:
				mpt::IO::WriteIntLE<uint32>(f, nPluginSize);
				SNDMIXPLUGININFO tmpPluginInfo = m_MixPlugins[i].Info;
				tmpPluginInfo.ConvertEndianness();
				fwrite(&tmpPluginInfo, 1, sizeof(SNDMIXPLUGININFO), f);
				mpt::IO::WriteIntLE<uint32>(f, m_MixPlugins[i].nPluginDataSize);
				if(m_MixPlugins[i].pPluginData)
				{
					fwrite(m_MixPlugins[i].pPluginData, 1, m_MixPlugins[i].nPluginDataSize, f);
				}

				mpt::IO::WriteIntLE<uint32>(f, MPTxPlugDataSize);

				// Dry/Wet ratio
				memcpy(id, "DWRT", 4);
				fwrite(id, 1, 4, f);
				// DWRT chunk does not include a size, so better make sure we always write 4 bytes here.
				STATIC_ASSERT(sizeof(IEEE754binary32LE) == 4);
				mpt::IO::Write(f, IEEE754binary32LE(m_MixPlugins[i].fDryRatio));

				// Default program
				memcpy(id, "PROG", 4);
				fwrite(id, 1, 4, f);
				// PROG chunk does not include a size, so better make sure we always write 4 bytes here.
				STATIC_ASSERT(sizeof(m_MixPlugins[i].defaultProgram) == sizeof(int32));
				mpt::IO::WriteIntLE<int32>(f, m_MixPlugins[i].defaultProgram);

				// Please, if you add any more chunks here, don't repeat history (see above) and *do* add a size field for your chunk, mmmkay?
			}
			nTotalSize += nPluginSize + 8;
		}
	}
	for(CHANNELINDEX j = 0; j < GetNumChannels(); j++)
	{
		if(j < MAX_BASECHANNELS)
		{
			if((chinfo[j] = ChnSettings[j].nMixPlugin) != 0)
			{
				nChInfo = j + 1;
			}
		}
	}
	if(nChInfo)
	{
		if(f)
		{
			memcpy(id, "CHFX", 4);
			fwrite(id, 1, 4, f);
			mpt::IO::WriteIntLE<uint32>(f, nChInfo * 4);
			for(uint32 i = 0; i < nChInfo; ++i)
			{
				mpt::IO::WriteIntLE<uint32>(f, chinfo[i]);
			}
		}
		nTotalSize += nChInfo * 4 + 8;
	}
	return nTotalSize;
}

#endif // MODPLUG_NO_FILESAVE


void CSoundFile::LoadMixPlugins(FileReader &file)
//-----------------------------------------------
{
	while(file.CanRead(9))
	{
		char code[4];
		file.ReadArray(code);
		const uint32 chunkSize = file.ReadUint32LE();
		if(!file.CanRead(chunkSize))
		{
			file.SkipBack(8);
			return;
		}
		FileReader chunk = file.ReadChunk(chunkSize);

		// Channel FX
		if(!memcmp(code, "CHFX", 4))
		{
			for (size_t ch = 0; ch < MAX_BASECHANNELS; ch++)
			{
				ChnSettings[ch].nMixPlugin = (uint8)chunk.ReadUint32LE();
			}
		}
		// Plugin Data FX00, ... FX99, F100, ... F255
#define ISNUMERIC(x) (code[(x)] >= '0' && code[(x)] <= '9')
		else if(code[0] == 'F' && (code[1] == 'X' || ISNUMERIC(1)) && ISNUMERIC(2) && ISNUMERIC(3))
#undef ISNUMERIC
		{
			PLUGINDEX plug = (code[2] - '0') * 10 + (code[3] - '0');	//calculate plug-in number.
			if(code[1] != 'X') plug += (code[1] - '0') * 100;

			if(plug < MAX_MIXPLUGINS)
			{
				// MPT's standard plugin data. Size not specified in file.. grrr..
				chunk.ReadConvertEndianness(m_MixPlugins[plug].Info);
				mpt::String::SetNullTerminator(m_MixPlugins[plug].Info.szName);
				mpt::String::SetNullTerminator(m_MixPlugins[plug].Info.szLibraryName);
				m_MixPlugins[plug].editorX = m_MixPlugins[plug].editorY = int32_min;

				//data for VST setchunk? size lies just after standard plugin data.
				const uint32 pluginDataChunkSize = chunk.ReadUint32LE();
				FileReader pluginDataChunk = chunk.ReadChunk(pluginDataChunkSize);

				if(pluginDataChunk.IsValid())
				{
					m_MixPlugins[plug].nPluginDataSize = 0;
					m_MixPlugins[plug].pPluginData = new (std::nothrow) char[pluginDataChunkSize];
					if(m_MixPlugins[plug].pPluginData)
					{
						m_MixPlugins[plug].nPluginDataSize = pluginDataChunkSize;
						pluginDataChunk.ReadRaw(m_MixPlugins[plug].pPluginData, pluginDataChunkSize);
					}
				}

				//rewbs.modularPlugData
				FileReader modularData = chunk.ReadChunk(chunk.ReadUint32LE());

				//if dwMPTExtra is positive and there are dwMPTExtra bytes left in nPluginSize, we have some more data!
				if(modularData.IsValid())
				{
					while(modularData.CanRead(5))
					{
						// do we recognize this chunk?
						modularData.ReadArray(code);
						uint32 dataSize = 0;
						if(!memcmp(code, "DWRT", 4) || !memcmp(code, "PROG", 4))
						{
							// Legacy system with fixed size chunks
							dataSize = 4;
						} else
						{
							dataSize = modularData.ReadUint32LE();
						}
						FileReader dataChunk = modularData.ReadChunk(dataSize);

						if(!memcmp(code, "DWRT", 4))
						{
							m_MixPlugins[plug].fDryRatio = dataChunk.ReadFloatLE();
						} else if(!memcmp(code, "PROG", 4))
						{
							m_MixPlugins[plug].defaultProgram = dataChunk.ReadUint32LE();
						} else if(!memcmp(code, "MCRO", 4))
						{
							// Read plugin-specific macros
							//dataChunk.ReadStructPartial(m_MixPlugins[plug].macros, dataChunk.GetLength());
						}
					}

				}
				//end rewbs.modularPlugData
			}
		} else if(!memcmp(code, "MODU", 4))
		{
			madeWithTracker = "BeRoTracker";
		} else if(!memcmp(code, "XTPM", 4))
		{
			// Read too far, chicken out...
			file.SkipBack(8);
			return;
		}
	}
}


#ifndef MODPLUG_NO_FILESAVE

void CSoundFile::SaveExtendedSongProperties(FILE* f) const
//--------------------------------------------------------
{
	const CModSpecifications &specs = GetModSpecifications();
	// Extra song data - Yet Another Hack.
	mpt::IO::WriteIntLE<uint32>(f, MAGIC4BE('M','P','T','S'));

#define WRITEMODULARHEADER(code, fsize) \
	{ \
		mpt::IO::WriteIntLE<uint32>(f, code); \
		MPT_ASSERT(Util::TypeCanHoldValue<uint16>(fsize)); \
		const uint16 _size = fsize; \
		mpt::IO::WriteIntLE<uint16>(f, _size); \
	}
#define WRITEMODULAR(code, field) \
	{ \
		WRITEMODULARHEADER(code, sizeof(field)) \
		mpt::IO::WriteIntLE(f, field); \
	}

	if(m_nDefaultTempo.GetInt() > 255)
	{
		uint32 tempo = m_nDefaultTempo.GetInt();
		WRITEMODULAR(MAGIC4BE('D','T','.','.'), tempo);
	}
	if(m_nDefaultTempo.GetFract() != 0 && specs.hasFractionalTempo)
	{
		uint32 tempo = m_nDefaultTempo.GetFract();
		WRITEMODULAR(MAGIC4LE('D','T','F','R'), tempo);
	}

	WRITEMODULAR(MAGIC4BE('R','P','B','.'), m_nDefaultRowsPerBeat);
	WRITEMODULAR(MAGIC4BE('R','P','M','.'), m_nDefaultRowsPerMeasure);

	if(GetType() != MOD_TYPE_XM)
	{
		WRITEMODULAR(MAGIC4BE('C','.','.','.'), m_nChannels);
	}

	if(TypeIsIT_MPT() && GetNumChannels() > 64)
	{
		// IT header has only room for 64 channels. Save the settings that do not fit to the header here as an extension.
		WRITEMODULARHEADER(MAGIC4BE('C','h','n','S'), (GetNumChannels() - 64) * 2);
		for(CHANNELINDEX chn = 64; chn < GetNumChannels(); chn++)
		{
			uint8 panvol[2];
			panvol[0] = (uint8)(ChnSettings[chn].nPan >> 2);
			if (ChnSettings[chn].dwFlags[CHN_SURROUND]) panvol[0] = 100;
			if (ChnSettings[chn].dwFlags[CHN_MUTE]) panvol[0] |= 0x80;
			panvol[1] = (uint8)ChnSettings[chn].nVolume;
			fwrite(&panvol, sizeof(panvol), 1, f);
		}
	}

	{
		WRITEMODULARHEADER(MAGIC4BE('T','M','.','.'), 1);
		uint8 mode = static_cast<uint8>(m_nTempoMode);
		fwrite(&mode, sizeof(mode), 1, f);
	}

	const int32 tmpMixLevels = static_cast<int32>(m_nMixLevels);
	WRITEMODULAR(MAGIC4BE('P','M','M','.'), tmpMixLevels);

	if(m_dwCreatedWithVersion)
	{
		WRITEMODULAR(MAGIC4BE('C','W','V','.'), m_dwCreatedWithVersion);
	}

	WRITEMODULAR(MAGIC4BE('L','S','W','V'), m_dwLastSavedWithVersion);
	WRITEMODULAR(MAGIC4BE('S','P','A','.'), m_nSamplePreAmp);
	WRITEMODULAR(MAGIC4BE('V','S','T','V'), m_nVSTiVolume);

	if(GetType() == MOD_TYPE_XM && m_nDefaultGlobalVolume != MAX_GLOBAL_VOLUME)
	{
		WRITEMODULAR(MAGIC4BE('D','G','V','.'), m_nDefaultGlobalVolume);
	}

	if(GetType() != MOD_TYPE_XM && m_nRestartPos != 0)
	{
		WRITEMODULAR(MAGIC4BE('R','P','.','.'), m_nRestartPos);
	}

	if(m_nResampling != SRCMODE_DEFAULT && specs.hasDefaultResampling)
	{
		WRITEMODULAR(MAGIC4LE('R','S','M','P'), static_cast<uint32>(m_nResampling));
	}

	// Sample cues
	if(GetType() == MOD_TYPE_MPT)
	{
		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			const ModSample &sample = Samples[smp];
			if(sample.nLength && sample.HasCustomCuePoints())
			{
				// Write one chunk for every sample.
				// Rationale: chunks are limited to 65536 bytes, which can easily be reached
				// with the amount of samples that OpenMPT supports.
				WRITEMODULARHEADER(MAGIC4LE('C','U','E','S'), 2 + CountOf(sample.cues) * 4);
				mpt::IO::WriteIntLE<uint16>(f, smp);
				for(std::size_t i = 0; i < CountOf(sample.cues); i++)
				{
					mpt::IO::WriteIntLE<uint32>(f, sample.cues[i]);
				}
			}
		}
	}

	// Tempo Swing Factors
	if(!m_tempoSwing.empty())
	{
		mpt::ostringstream oStrm;
		TempoSwing::Serialize(oStrm, m_tempoSwing);
		std::string data = oStrm.str();
		uint16 length = mpt::saturate_cast<uint16>(data.size());
		WRITEMODULARHEADER(MAGIC4LE('S','W','N','G'), length);
		mpt::IO::WriteRaw(f, &data[0], length);
	}

	// Additional flags for XM/IT/MPTM
	if(m_ModFlags)
	{
		WRITEMODULAR(MAGIC4BE('M','S','F','.'), m_ModFlags.GetRaw());
	}

	if(!songArtist.empty() && specs.hasArtistName)
	{
		std::string songArtistU8 = mpt::ToCharset(mpt::CharsetUTF8, songArtist);
		uint16 length = mpt::saturate_cast<uint16>(songArtistU8.length());
		WRITEMODULARHEADER(MAGIC4LE('A','U','T','H'), length);
		mpt::IO::WriteRaw(f, songArtistU8.c_str(), length);
	}

#ifdef MODPLUG_TRACKER
	// MIDI mapping directives
	if(GetMIDIMapper().GetCount() > 0)
	{
		const size_t objectsize = GetMIDIMapper().GetSerializationSize();
		if(objectsize > size_t(int16_max))
		{
			AddToLog("Too many MIDI Mapping directives to save; data won't be written.");
		} else
		{
			WRITEMODULARHEADER(MAGIC4BE('M','I','M','A'), static_cast<uint16>(objectsize));
			GetMIDIMapper().Serialize(f);
		}
	}
#endif

#undef WRITEMODULAR
#undef WRITEMODULARHEADER
	return;
}

#endif // MODPLUG_NO_FILESAVE


template<typename T>
void ReadField(FileReader &chunk, std::size_t size, T &field)
//-----------------------------------------------------------
{
	field = chunk.ReadSizedIntLE<T>(size);
}


template<typename Tenum, typename Tstore>
void ReadFieldFlagSet(FileReader &chunk, std::size_t size, FlagSet<Tenum, Tstore> &field)
//---------------------------------------------------------------------------------------
{
	field.SetRaw(chunk.ReadSizedIntLE<Tstore>(size));
}


template<typename T>
void ReadFieldCast(FileReader &chunk, std::size_t size, T &field)
//---------------------------------------------------------------
{
	STATIC_ASSERT(sizeof(T) <= sizeof(int32));
	field = static_cast<T>(chunk.ReadSizedIntLE<int32>(size));
}


void CSoundFile::LoadExtendedSongProperties(const MODTYPE modtype, FileReader &file, bool *pInterpretMptMade)
//-----------------------------------------------------------------------------------------------------------
{
	if(!file.ReadMagic("STPM"))	// 'MPTS'
	{
		return;
	}

	// Found MPTS, interpret the file MPT made.
	if(pInterpretMptMade != nullptr)
		*pInterpretMptMade = true;

	// HACK: Reset mod flags to default values here, as they are not always written.
	m_ModFlags.reset();

	while(file.CanRead(7))
	{
		const uint32 code = file.ReadUint32LE();
		const uint16 size = file.ReadUint16LE();

		// Start of MPTM extensions or truncated field
		if(code == MAGIC4LE('2','2','8',4) || !file.CanRead(size))
		{
			break;
		}

		FileReader chunk = file.ReadChunk(size);

		switch (code)					// interpret field code
		{
			case MAGIC4BE('D','T','.','.'): { uint32 tempo; ReadField(chunk, size, tempo); m_nDefaultTempo.Set(tempo, m_nDefaultTempo.GetFract()); break; }
			case MAGIC4LE('D','T','F','R'): { uint32 tempoFract; ReadField(chunk, size, tempoFract); m_nDefaultTempo.Set(m_nDefaultTempo.GetInt(), tempoFract); break; }
			case MAGIC4BE('R','P','B','.'): ReadField(chunk, size, m_nDefaultRowsPerBeat); break;
			case MAGIC4BE('R','P','M','.'): ReadField(chunk, size, m_nDefaultRowsPerMeasure); break;
			case MAGIC4BE('C','.','.','.'): { CHANNELINDEX chn = 0; if(modtype != MOD_TYPE_XM) ReadField(chunk, size, chn); m_nChannels = std::max(m_nChannels, chn); break; }
			case MAGIC4BE('T','M','.','.'): ReadFieldCast(chunk, size, m_nTempoMode); break;
			case MAGIC4BE('P','M','M','.'): ReadFieldCast(chunk, size, m_nMixLevels); break;
			case MAGIC4BE('C','W','V','.'): ReadField(chunk, size, m_dwCreatedWithVersion); break;
			case MAGIC4BE('L','S','W','V'): ReadField(chunk, size, m_dwLastSavedWithVersion); break;
			case MAGIC4BE('S','P','A','.'): ReadField(chunk, size, m_nSamplePreAmp); break;
			case MAGIC4BE('V','S','T','V'): ReadField(chunk, size, m_nVSTiVolume); break;
			case MAGIC4BE('D','G','V','.'): ReadField(chunk, size, m_nDefaultGlobalVolume); break;
			case MAGIC4BE('R','P','.','.'): if(modtype != MOD_TYPE_XM) ReadField(chunk, size, m_nRestartPos); break;
			case MAGIC4BE('M','S','F','.'): ReadFieldFlagSet(chunk, size, m_ModFlags); break;
			case MAGIC4LE('R','S','M','P'):
				ReadFieldCast(chunk, size, m_nResampling);
				if(!IsKnownResamplingMode(m_nResampling)) m_nResampling = SRCMODE_DEFAULT;
				break;
#ifdef MODPLUG_TRACKER
			case MAGIC4BE('M','I','M','A'): GetMIDIMapper().Deserialize(chunk); break;
#endif
			case MAGIC4LE('A','U','T','H'):
				{
					std::string artist;
					chunk.ReadString<mpt::String::spacePadded>(artist, chunk.GetLength());
					songArtist = mpt::ToUnicode(mpt::CharsetUTF8, artist);
				}
				break;
			case MAGIC4BE('C','h','n','S'):
				if(size <= (MAX_BASECHANNELS - 64) * 2 && (size % 2u) == 0)
				{
					STATIC_ASSERT(CountOf(ChnSettings) >= 64);
					const CHANNELINDEX loopLimit = std::min(uint16(size / 2), uint16(CountOf(ChnSettings) - 64));

					for(CHANNELINDEX i = 0; i < loopLimit; i++)
					{
						uint8 pan = chunk.ReadUint8(), vol = chunk.ReadUint8();
						if(pan != 0xFF)
						{
							ChnSettings[i + 64].nVolume = vol;
							ChnSettings[i + 64].nPan = 128;
							ChnSettings[i + 64].dwFlags.reset();
							if(pan & 0x80) ChnSettings[i + 64].dwFlags.set(CHN_MUTE);
							pan &= 0x7F;
							if(pan <= 64) ChnSettings[i + 64].nPan = pan << 2;
							if(pan == 100) ChnSettings[i + 64].dwFlags.set(CHN_SURROUND);
						}
					}
				}
				break;

			case MAGIC4LE('C','U','E','S'):
				// Sample cues
				if(size > 2)
				{
					SAMPLEINDEX smp = chunk.ReadUint16LE();
					if(smp > 0 && smp <= GetNumSamples())
					{
						ModSample &sample = Samples[smp];
						for(std::size_t i = 0; i < CountOf(sample.cues); i++)
						{
							sample.cues[i] = chunk.ReadUint32LE();
						}
					}
				}
				break;

			case MAGIC4LE('S','W','N','G'):
				// Tempo Swing Factors
				if(size > 2)
				{
					std::string data(chunk.GetLength(), '\0');
					chunk.ReadRaw(&data[0], data.size());
					mpt::istringstream iStrm(data);
					TempoSwing::Deserialize(iStrm, m_tempoSwing, data.size());
				}
				break;
		}

	}

	// Validate read values.
	Limit(m_nDefaultTempo, GetModSpecifications().tempoMin, GetModSpecifications().tempoMax);
	if(m_nDefaultRowsPerMeasure < m_nDefaultRowsPerBeat) m_nDefaultRowsPerMeasure = m_nDefaultRowsPerBeat;
	Limit(m_nChannels, CHANNELINDEX(1), GetModSpecifications().channelsMax);
	if(m_nTempoMode >= tempoModeMax) m_nTempoMode = tempoModeClassic;
	if(m_nMixLevels >= mixLevelsMax) m_nMixLevels = mixLevelsOriginal;
	//m_dwCreatedWithVersion
	//m_dwLastSavedWithVersion
	//m_nSamplePreAmp
	//m_nVSTiVolume
	//m_nDefaultGlobalVolume
	LimitMax(m_nDefaultGlobalVolume, MAX_GLOBAL_VOLUME);
	//m_nRestartPos
	//m_ModFlags
	if(!m_tempoSwing.empty()) m_tempoSwing.resize(m_nDefaultRowsPerBeat);
}


#ifndef MODPLUG_NO_FILESAVE

size_t CSoundFile::SaveModularInstrumentData(FILE *f, const ModInstrument *pIns) const
//------------------------------------------------------------------------------------
{
	// Yes, it is completely idiotic that we have both SaveModularInstrumentData and SaveExtendedInstrumentProperties.
	// And they are all insane in their own way:
	// - SaveExtendedInstrumentProperties is tacked SOMEWHERE AFTER THE SAMPLE CHUNK, which is horrible in case of
	//   compressed samples because you don't know where to start reading without actually unpacking the sample.
	//   this mechanism has broken a gazillion times in the past due to simple code changes. Luckily we have unit
	//   tests which can detect this most of the time.
	// - SaveModularInstrumentData follows the instrument data as it should, but is used for only one chunk, and
	//   it doesn't actually specify the chunk's size. You have to hardcode chunk size or else you're lost.
	//   That's totally awesome if you have to process unknown chunks. NOT.
	// Oh, and of course they all use backwards FOURCCs because they originally used the non-standard '1234'
	// multi-char MSVC extension. It's one big clusterfuck.

	// As the only stuff that is actually written here is the plugin ID,
	// we can actually chicken out if there's no plugin.
	if(!pIns->nMixPlug)
	{
		return 0;
	}

	uint32 modularInstSize = 0;
	uint32 id = MAGIC4BE('I','N','S','M');
	mpt::IO::WriteIntLE<uint32>(f, id);
	long sizePos = ftell(f);			// we will want to write the modular data's total size here
	mpt::IO::WriteIntLE<uint32>(f, 0);	// write a DUMMY size, just to move file pointer by a long

	// Write chunks
	{	//VST Slot chunk:
		mpt::IO::WriteIntLE<uint32>(f, MAGIC4BE('P','L','U','G'));
		mpt::IO::WriteIntLE<uint8>(f, pIns->nMixPlug);
		modularInstSize += sizeof(uint32) + sizeof(uint8);
	}
	// If you really need to add more chunks here, please don't repeat history (see above) and *do* add a size field for your chunk, mmmkay?

	//write modular data's total size
	long curPos = ftell(f);			// remember current pos
	fseek(f, sizePos, SEEK_SET);	// go back to  sizePos
	mpt::IO::WriteIntLE<uint32>(f, modularInstSize);	// write data
	fseek(f, curPos, SEEK_SET);		// go back to where we were.

	// Compute the size that we just wasted.
	return sizeof(id) + sizeof(modularInstSize) + modularInstSize;
}

#endif // MODPLUG_NO_FILESAVE


size_t CSoundFile::LoadModularInstrumentData(FileReader &file, ModInstrument &ins)
//--------------------------------------------------------------------------------
{
	// find end of standard header

	//If the next piece of data is 'INSM' we have modular extensions to our instrument...
	if(!file.ReadMagic("MSNI"))
	{
		return 0;
	}
	//...the next piece of data must be the total size of the modular data
	FileReader modularData = file.ReadChunk(file.ReadUint32LE());

	// Handle chunks
	while(modularData.CanRead(4))
	{
		const uint32 chunkID = modularData.ReadUint32LE();
		uint16 chunkSize;
		// Legacy chunk
		if(chunkID == MAGIC4BE('P','L','U','G'))
			chunkSize = 1;
		else
			chunkSize = modularData.ReadUint16LE();
		FileReader chunkData = modularData.ReadChunk(chunkSize);

		switch (chunkID)
		{
		case MAGIC4BE('P','L','U','G'):
			ins.nMixPlug = chunkData.ReadUint8();
			break;

		}
	}

	return 8 + modularData.GetLength();
}


OPENMPT_NAMESPACE_END
