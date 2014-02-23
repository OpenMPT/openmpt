/*
 * Load_xm.cpp
 * -----------
 * Purpose: XM (FastTracker II) module loader / saver
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          Adam Goode (endian and char fixes for PPC)
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "../common/version.h"
#include "../common/misc_util.h"
#include "XMTools.h"
#include <algorithm>
#ifdef MODPLUG_TRACKER
#include "../mptrack/TrackerSettings.h"	// For super smooth ramping option
#endif // MODPLUG_TRACKER


// Allocate samples for an instrument
static std::vector<SAMPLEINDEX> AllocateXMSamples(CSoundFile &sndFile, SAMPLEINDEX numSamples)
//--------------------------------------------------------------------------------------------
{
	LimitMax(numSamples, SAMPLEINDEX(32));

	std::vector<SAMPLEINDEX> foundSlots;

	for(SAMPLEINDEX i = 0; i < numSamples; i++)
	{
		SAMPLEINDEX candidateSlot = sndFile.GetNumSamples() + 1;

		if(candidateSlot >= MAX_SAMPLES)
		{
			// If too many sample slots are needed, try to fill some empty slots first.
			for(SAMPLEINDEX j = 1; j <= sndFile.GetNumSamples(); j++)
			{
				if(sndFile.GetSample(j).pSample != nullptr)
				{
					continue;
				}

				if(std::find(foundSlots.begin(), foundSlots.end(), j) == foundSlots.end())
				{
					// Empty sample slot that is not occupied by the current instrument. Yay!
					candidateSlot = j;

					// Remove unused sample from instrument sample assignments
					for(INSTRUMENTINDEX ins = 1; ins <= sndFile.GetNumInstruments(); ins++)
					{
						if(sndFile.Instruments[ins] == nullptr)
						{
							continue;
						}
						for(size_t k = 0; k < CountOf(sndFile.Instruments[ins]->Keyboard); k++)
						{
							if(sndFile.Instruments[ins]->Keyboard[k] == candidateSlot)
							{
								sndFile.Instruments[ins]->Keyboard[k] = 0;
							}
						}
					}
					break;
				}
			}
		}

		if(candidateSlot >= MAX_SAMPLES)
		{
			// Still couldn't find any empty sample slots, so look out for existing but unused samples.
			std::vector<bool> usedSamples;
			SAMPLEINDEX unusedSampleCount = sndFile.DetectUnusedSamples(usedSamples);

			if(unusedSampleCount > 0)
			{
				sndFile.RemoveSelectedSamples(usedSamples);
				// Remove unused samples from instrument sample assignments
				for(INSTRUMENTINDEX ins = 1; ins <= sndFile.GetNumInstruments(); ins++)
				{
					if(sndFile.Instruments[ins] == nullptr)
					{
						continue;
					}
					for(size_t k = 0; k < CountOf(sndFile.Instruments[ins]->Keyboard); k++)
					{
						if(sndFile.Instruments[ins]->Keyboard[k] < usedSamples.size() && !usedSamples[sndFile.Instruments[ins]->Keyboard[k]])
						{
							sndFile.Instruments[ins]->Keyboard[k] = 0;
						}
					}
				}

				// New candidate slot is first unused sample slot.
				candidateSlot = static_cast<SAMPLEINDEX>(std::find(usedSamples.begin() + 1, usedSamples.end(), false) - usedSamples.begin());
			} else
			{
				// No unused sampel slots: Give up :(
				break;
			}
		}

		if(candidateSlot < MAX_SAMPLES)
		{
			foundSlots.push_back(candidateSlot);
			if(candidateSlot > sndFile.GetNumSamples())
			{
				sndFile.m_nSamples = candidateSlot;
			}
		}
	}

	return foundSlots;
}


// Read .XM patterns
static void ReadXMPatterns(FileReader &file, const XMFileHeader &fileHeader, CSoundFile &sndFile)
//-----------------------------------------------------------------------------------------------
{
	// Reading patterns
	sndFile.Patterns.ResizeArray(fileHeader.patterns);
	for(PATTERNINDEX pat = 0; pat < fileHeader.patterns; pat++)
	{
		FileReader::off_t curPos = file.GetPosition();
		uint32 headerSize = file.ReadUint32LE();
		file.Skip(1);	// Pack method (= 0)

		ROWINDEX numRows = 64;

		if(fileHeader.version == 0x0102)
		{
			numRows = file.ReadUint8() + 1;
		} else
		{
			numRows = file.ReadUint16LE();
		}

		// A packed size of 0 indicates a completely empty pattern.
		const uint16 packedSize = file.ReadUint16LE();

		if(numRows == 0 || numRows > MAX_PATTERN_ROWS)
		{
			numRows = 64;
		}

		file.Seek(curPos + headerSize);
		FileReader patternChunk = file.GetChunk(packedSize);

		if(sndFile.Patterns.Insert(pat, numRows) || packedSize == 0)
		{
			continue;
		}

		enum PatternFlags
		{
			isPackByte		= 0x80,
			allFlags		= 0xFF,

			notePresent		= 0x01,
			instrPresent	= 0x02,
			volPresent		= 0x04,
			commandPresent	= 0x08,
			paramPresent	= 0x10,
		};

		ModCommand *m = sndFile.Patterns[pat];
		for(size_t numCommands = numRows * fileHeader.channels; numCommands != 0; numCommands--, m++)
		{
			uint8 info = patternChunk.ReadUint8();

			uint8 vol = 0;
			if(info & isPackByte)
			{
				// Interpret byte as flag set.
				if(info & notePresent) m->note = patternChunk.ReadUint8();
			} else
			{
				// Interpret byte as note, read all other pattern fields as well.
				m->note = info;
				info = allFlags;
			}

			if(info & instrPresent) m->instr = patternChunk.ReadUint8();
			if(info & volPresent) vol = patternChunk.ReadUint8();
			if(info & commandPresent) m->command = patternChunk.ReadUint8();
			if(info & paramPresent) m->param = patternChunk.ReadUint8();

			if(m->note == 97)
			{
				m->note = NOTE_KEYOFF;
			} else if(m->note > 0 && m->note < 97)
			{
				m->note += 12;
			} else
			{
				m->note = NOTE_NONE;
			}

			if(m->command | m->param)
			{
				sndFile.ConvertModCommand(*m);
			} else
			{
				m->command = CMD_NONE;
			}

			if(m->instr == 0xFF)
			{
				m->instr = 0;
			}

			if(vol >= 0x10 && vol <= 0x50)
			{
				m->volcmd = VOLCMD_VOLUME;
				m->vol = vol - 0x10;
			} else if (vol >= 0x60)
			{
				// Volume commands 6-F translation.
				static const ModCommand::VOLCMD volEffTrans[] =
				{
					VOLCMD_VOLSLIDEDOWN, VOLCMD_VOLSLIDEUP, VOLCMD_FINEVOLDOWN, VOLCMD_FINEVOLUP,
					VOLCMD_VIBRATOSPEED, VOLCMD_VIBRATODEPTH, VOLCMD_PANNING, VOLCMD_PANSLIDELEFT,
					VOLCMD_PANSLIDERIGHT, VOLCMD_TONEPORTAMENTO,
				};

				m->volcmd = volEffTrans[(vol - 0x60) >> 4];
				m->vol = vol & 0x0F;

				if(m->volcmd == VOLCMD_PANNING)
				{
					m->vol *= 4;	// FT2 does indeed not scale panning symmetrically.
				}
			}
		}
	}
}


enum TrackerVersions
{
	verUnknown		= 0x00,		// Probably not made with MPT
	verOldModPlug	= 0x01,		// Made with MPT Alpha / Beta
	verNewModPlug	= 0x02,		// Made with MPT (not Alpha / Beta)
	verModPlug1_09	= 0x04,		// Made with MPT 1.09 or possibly other version
	verOpenMPT		= 0x08,		// Made with OpenMPT
	verConfirmed	= 0x10,		// We are very sure that we found the correct tracker version.

	verFT2Generic	= 0x20,		// "FastTracker v2.00", but FastTracker has NOT been ruled out
	verOther		= 0x40,		// Something we don't know, testing for DigiTrakker.
	verFT2Clone		= 0x80,		// NOT FT2: itype changed between instruments, or \0 found in song title
	verDigiTracker	= 0x100,	// Probably DigiTrakker
};
DECLARE_FLAGSET(TrackerVersions)


bool CSoundFile::ReadXM(FileReader &file, ModLoadingFlags loadFlags)
//------------------------------------------------------------------
{
	file.Rewind();

	XMFileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| fileHeader.channels == 0
		|| fileHeader.channels > MAX_BASECHANNELS
		|| mpt::strnicmp(fileHeader.signature, "Extended Module: ", 17)
		|| !file.CanRead(fileHeader.orders))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals();
	InitializeChannels();
	ChangeModTypeTo(MOD_TYPE_XM);

	FlagSet<TrackerVersions> madeWith(verUnknown);

	if(!memcmp(fileHeader.trackerName, "FastTracker ", 12))
	{
		if(fileHeader.size == 276 && !memcmp(fileHeader.trackerName + 12, "v2.00   ", 8))
		{
			if(fileHeader.version < 0x0104)
				madeWith = verFT2Generic | verConfirmed;
			else if(memchr(fileHeader.songName, '\0', 20) != nullptr)
				// FT2 pads the song title with spaces, some other trackers use null chars
				madeWith = verFT2Clone | verNewModPlug;
			else
				madeWith = verFT2Generic | verNewModPlug;
		} else if(!memcmp(fileHeader.trackerName + 12, "v 2.00  ", 8))
		{
			// MPT 1.0 (exact version to be determined later)
			madeWith = verOldModPlug;
		} else
		{
			// ???
			madeWith.set(verConfirmed);
			madeWithTracker = "FastTracker Clone";
		}
	} else
	{
		// Something else!
		madeWith = verUnknown |verConfirmed;

		mpt::String::Read<mpt::String::spacePadded>(madeWithTracker, fileHeader.trackerName);
	}

	mpt::String::Read<mpt::String::spacePadded>(songName, fileHeader.songName);

	m_nMinPeriod = 27;
	m_nMaxPeriod = 54784;

	m_nRestartPos = fileHeader.restartPos;
	m_nChannels = fileHeader.channels;
	m_nInstruments = std::min(fileHeader.instruments, uint16(MAX_INSTRUMENTS - 1));
	if(fileHeader.speed)
		m_nDefaultSpeed = fileHeader.speed;
	if(fileHeader.tempo)
		m_nDefaultTempo = Clamp(fileHeader.tempo, uint16(32), uint16(512));

	m_SongFlags.reset();
	m_SongFlags.set(SONG_LINEARSLIDES, (fileHeader.flags & XMFileHeader::linearSlides) != 0);
	m_SongFlags.set(SONG_EXFILTERRANGE, (fileHeader.flags & XMFileHeader::extendedFilterRange) != 0);

	// set this here already because XMs compressed with BoobieSqueezer will exit the function early
	SetModFlag(MSF_COMPATIBLE_PLAY, true);

	Order.ReadAsByte(file, fileHeader.orders);
	file.Seek(fileHeader.size + 60);

	if(fileHeader.version >= 0x0104)
	{
		ReadXMPatterns(file, fileHeader, *this);
	}

	// In case of XM versions < 1.04, we need to memorize the sample flags for all samples, as they are not stored immediately after the sample headers.
	std::vector<SampleIO> sampleFlags;
	uint8 sampleReserved = 0;
	int instrType = -1;

	// Reading instruments
	for(INSTRUMENTINDEX instr = 1; instr <= m_nInstruments; instr++)
	{
		// First, try to read instrument header length...
		XMInstrumentHeader instrHeader;
		instrHeader.size = file.ReadUint32LE();
		if(instrHeader.size == 0)
		{
			instrHeader.size = sizeof(XMInstrumentHeader);
		}

		// Now, read the complete struct.
		file.SkipBack(4);
		file.ReadStructPartial(instrHeader, instrHeader.size);
		instrHeader.ConvertEndianness();

		// Time for some version detection stuff.
		if(madeWith == verOldModPlug)
		{
			madeWith.set(verConfirmed);
			if(instrHeader.size == 245)
			{
				// ModPlug Tracker Alpha
				m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 00, 00, A5);
				madeWithTracker = "ModPlug Tracker 1.0 alpha";
			} else if(instrHeader.size == 263)
			{
				// ModPlug Tracker Beta (Beta 1 still behaves like Alpha, but Beta 3.3 does it this way)
				m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 00, 00, B3);
				madeWithTracker = "ModPlug Tracker 1.0 beta";
			} else
			{
				// WTF?
				madeWith = (verUnknown | verConfirmed);
			}
		} else if(instrHeader.numSamples == 0)
		{
			// Empty instruments make tracker identification pretty easy!
			if(instrHeader.size == 263 && instrHeader.sampleHeaderSize == 0 && madeWith[verNewModPlug])
				madeWith.set(verConfirmed);
			else if(instrHeader.size != 29 && madeWith[verDigiTracker])
				madeWith.reset(verDigiTracker);
			else if(madeWith[verFT2Clone | verFT2Generic] && instrHeader.size != 33)
			{
				// Sure isn't FT2.
				// Note: FT2 NORMALLY writes shdr=40 for all samples, but sometimes it
				// just happens to write random garbage there instead. Surprise!
				madeWith = verUnknown;
			}
		}

		if(AllocateInstrument(instr) == nullptr)
		{
			continue;
		}

		instrHeader.ConvertToMPT(*Instruments[instr]);

		if(instrType == -1)
		{
			instrType = instrHeader.type;
		} else if(instrType != instrHeader.type && madeWith[verFT2Generic])
		{
			// FT2 writes some random junk for the instrument type field,
			// but it's always the SAME junk for every instrument saved.
			madeWith.reset(verFT2Generic);
			madeWith.set(verFT2Clone);
		}

		if(instrHeader.numSamples > 0)
		{
			// Yep, there are some samples associated with this instrument.
			if((instrHeader.instrument.midiEnabled | instrHeader.instrument.midiChannel | instrHeader.instrument.midiProgram | instrHeader.instrument.muteComputer) != 0)
			{
				// Definitely not an old MPT.
				madeWith.reset(verOldModPlug | verNewModPlug);
			}

			// Read sample headers
			std::vector<SAMPLEINDEX> sampleSlots = AllocateXMSamples(*this, instrHeader.numSamples);

			// Update sample assignment map
			for(size_t k = 0 + 12; k < 96 + 12; k++)
			{
				if(Instruments[instr]->Keyboard[k] < sampleSlots.size())
				{
					Instruments[instr]->Keyboard[k] = sampleSlots[Instruments[instr]->Keyboard[k]];
				}
			}

			if(fileHeader.version >= 0x0104)
			{
				sampleFlags.clear();
			}
			// Need to memorize those if we're going to skip any samples...
			std::vector<uint32> sampleSize(instrHeader.numSamples);

			// Early versions of Sk@le Tracker didn't set sampleHeaderSize (this fixes IFULOVE.XM)
			const size_t copyBytes = (instrHeader.sampleHeaderSize > 0) ? instrHeader.sampleHeaderSize : sizeof(XMSample);

			for(SAMPLEINDEX sample = 0; sample < instrHeader.numSamples; sample++)
			{
				XMSample sampleHeader;
				file.ReadStructPartial(sampleHeader, copyBytes);
				sampleHeader.ConvertEndianness();

				sampleFlags.push_back(sampleHeader.GetSampleFormat());
				sampleSize[sample] = sampleHeader.length;
				sampleReserved |= sampleHeader.reserved;

				if(sample < sampleSlots.size())
				{
					SAMPLEINDEX mptSample = sampleSlots[sample];

					sampleHeader.ConvertToMPT(Samples[mptSample]);
					instrHeader.instrument.ApplyAutoVibratoToMPT(Samples[mptSample]);

					mpt::String::Read<mpt::String::spacePadded>(m_szNames[mptSample], sampleHeader.name);

					if((sampleHeader.flags & 3) == 3 && madeWith[verNewModPlug])
					{
						// MPT 1.09 and maybe newer / older versions set both loop flags for bidi loops.
						madeWith.set(verModPlug1_09);
					}
				}
			}

			// Read samples
			if(fileHeader.version >= 0x0104)
			{
				for(SAMPLEINDEX sample = 0; sample < instrHeader.numSamples; sample++)
				{
					// Sample 15 in dirtysex.xm by J/M/T/M is a 16-bit sample with an odd size of 0x18B according to the header, while the real sample size would be 0x18A.
					// Always read as many bytes as specified in the header, even if the sample reader would probably read less bytes.
					FileReader sampleChunk = file.GetChunk(sampleSize[sample]);
					if(sample < sampleSlots.size())
					{
						sampleFlags[sample].ReadSample(Samples[sampleSlots[sample]], sampleChunk);
					}
				}
			}
		}
	}

	if(sampleReserved == 0 && madeWith[verNewModPlug] && memchr(fileHeader.songName, '\0', sizeof(fileHeader.songName)) != nullptr)
	{
		// Null-terminated song name: Quite possibly MPT.
		madeWith |= verConfirmed;
	}

	if(fileHeader.version < 0x0104)
	{
		// Load Patterns and Samples (Version 1.02 and 1.03)
		ReadXMPatterns(file, fileHeader, *this);

		for(SAMPLEINDEX sample = 1; sample <= GetNumSamples(); sample++)
		{
			sampleFlags[sample - 1].ReadSample(Samples[sample], file);
		}
	}

	// Read song comments: "text"
	if(file.ReadMagic("text"))
	{
		songMessage.Read(file, file.ReadUint32LE(), SongMessage::leCR);
		madeWith |= verConfirmed;
	}
	
	// Read midi config: "MIDI"
	if(file.ReadMagic("MIDI"))
	{
		file.ReadStructPartial(m_MidiCfg, file.ReadUint32LE());
		m_MidiCfg.Sanitize();
		m_SongFlags |= SONG_EMBEDMIDICFG;
		madeWith |= verConfirmed;
	}

	// Read pattern names: "PNAM"
	if(file.ReadMagic("PNAM"))
	{
		const PATTERNINDEX namedPats = std::min(static_cast<PATTERNINDEX>(file.ReadUint32LE() / MAX_PATTERNNAME), Patterns.Size());
		
		for(PATTERNINDEX pat = 0; pat < namedPats; pat++)
		{
			char patName[MAX_PATTERNNAME];
			file.ReadString<mpt::String::maybeNullTerminated>(patName, MAX_PATTERNNAME);
			Patterns[pat].SetName(patName);
		}
		madeWith |= verConfirmed;
	}

	// Read channel names: "CNAM"
	if(file.ReadMagic("CNAM"))
	{
		const CHANNELINDEX namedChans = std::min(static_cast<CHANNELINDEX>(file.ReadUint32LE() / MAX_CHANNELNAME), GetNumChannels());
		for(CHANNELINDEX chn = 0; chn < namedChans; chn++)
		{
			file.ReadString<mpt::String::maybeNullTerminated>(ChnSettings[chn].szName, MAX_CHANNELNAME);
		}
		madeWith |= verConfirmed;
	}

	// Read mix plugins information
	if(file.CanRead(8))
	{
		FileReader::off_t oldPos = file.GetPosition();
		LoadMixPlugins(file);
		if(file.GetPosition() != oldPos)
		{
			madeWith |= verConfirmed;
		}
	}

	if(madeWith[verConfirmed])
	{
		if(madeWith[verModPlug1_09])
		{
			m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 09, 00, 00);
			madeWithTracker = "ModPlug Tracker 1.09";
		} else if(madeWith[verNewModPlug])
		{
			m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 16, 00, 00);
			madeWithTracker = "ModPlug Tracker 1.16";
		}
	}

	if(!memcmp(fileHeader.trackerName, "OpenMPT ", 8))
	{
		// Hey, I know this tracker!
		char mptVersion[13];
		memcpy(mptVersion, fileHeader.trackerName + 8, 12);
		mpt::String::SetNullTerminator(mptVersion);
		m_dwLastSavedWithVersion = MptVersion::ToNum(mptVersion);
		madeWith = verOpenMPT | verConfirmed;
	}

	if(m_dwLastSavedWithVersion != 0 && !madeWith[verOpenMPT])
	{
		m_nMixLevels = mixLevels_original;
		SetModFlag(MSF_COMPATIBLE_PLAY, false);
	}

	if(madeWith[verFT2Generic]
		&& fileHeader.version >= 0x0104	// Old versions of FT2 didn't have (smooth) ramping. Disable it for those versions where we can be sure that there should be no ramping.
#ifdef MODPLUG_TRACKER
		&& TrackerSettings::Instance().autoApplySmoothFT2Ramping
#endif // MODPLUG_TRACKER
		)
	{
		// apply FT2-style super-soft volume ramping
		SetModFlag(MSF_VOLRAMP, true);
	}

	if(madeWithTracker.empty())
	{
		if(madeWith[verDigiTracker] && sampleReserved == 0 && (instrType ? instrType : -1) == -1)
		{
			madeWithTracker = "DigiTrakker";
		} else if(madeWith[verFT2Generic])
		{
			madeWithTracker = "FastTracker 2 or compatible";
		} else
		{
			madeWithTracker = "Unknown";
		}
	}

	// Leave if no extra instrument settings are available (end of file reached)
	if(file.NoBytesLeft()) return true;

	bool interpretOpenMPTMade = false; // specific for OpenMPT 1.17+ (bMadeWithModPlug is also for MPT 1.16)
	if(GetNumInstruments())
	{
		LoadExtendedInstrumentProperties(file, &interpretOpenMPTMade);
	}

	LoadExtendedSongProperties(GetType(), file, &interpretOpenMPTMade);

	if(interpretOpenMPTMade)
	{
		UpgradeModFlags();

		if(m_dwLastSavedWithVersion == 0)
		{
			m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 17, 00, 00);	// early versions of OpenMPT had no version indication.
		}
	}

	if(m_dwLastSavedWithVersion >= MAKE_VERSION_NUMERIC(1, 17, 00, 00))
	{
		madeWithTracker = "OpenMPT " + MptVersion::ToStr(m_dwLastSavedWithVersion);
	}

	// We no longer allow any --- or +++ items in the order list now.
	if(m_dwLastSavedWithVersion && m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 22, 02, 02))
	{
		if(!Patterns.IsValidPat(0xFE))
			Order.RemovePattern(0xFE);
		if(!Patterns.IsValidPat(0xFF))
			Order.Replace(0xFF, Order.GetInvalidPatIndex());
	}

	return true;
}


#ifndef MODPLUG_NO_FILESAVE

#define str_tooMuchPatternData	(GetStrI18N("Warning: File format limit was reached. Some pattern data may not get written to file."))
#define str_pattern				(GetStrI18N("pattern"))


bool CSoundFile::SaveXM(const mpt::PathString &filename, bool compatibilityExport)
//--------------------------------------------------------------------------------
{
	FILE *f;
	if(filename.empty() || (f = mpt_fopen(filename, "wb")) == nullptr)
	{
		return false;
	}

	bool addChannel = false; // avoid odd channel count for FT2 compatibility

	XMFileHeader fileHeader;
	MemsetZero(fileHeader);

	memcpy(fileHeader.signature, "Extended Module: ", 17);
	mpt::String::Write<mpt::String::spacePadded>(fileHeader.songName, songName);
	fileHeader.eof = 0x1A;
	std::string openMptTrackerName = MptVersion::GetOpenMPTVersionStr();
	mpt::String::Write<mpt::String::spacePadded>(fileHeader.trackerName, openMptTrackerName);

	// Writing song header
	fileHeader.version = 0x0104;					// XM Format v1.04
	fileHeader.size = sizeof(XMFileHeader) - 60;	// minus everything before this field
	fileHeader.restartPos = m_nRestartPos;

	fileHeader.channels = (m_nChannels + 1) & 0xFFFE; // avoid odd channel count for FT2 compatibility
	if((m_nChannels % 2u) && m_nChannels < MAX_BASECHANNELS) addChannel = true;
	if(compatibilityExport && fileHeader.channels > 32)
		fileHeader.channels = 32;
	if(fileHeader.channels > MAX_BASECHANNELS) fileHeader.channels = MAX_BASECHANNELS;
	fileHeader.channels = fileHeader.channels;

	// Find out number of orders and patterns used.
	// +++ and --- patterns are not taken into consideration as FastTracker does not support them.
	ORDERINDEX maxOrders = 0;
	PATTERNINDEX numPatterns = Patterns.GetNumPatterns();
	bool changeOrderList = false;
	for(ORDERINDEX ord = 0; ord < Order.GetLengthTailTrimmed(); ord++)
	{
		if(Order[ord] == Order.GetIgnoreIndex() || Order[ord] == Order.GetInvalidPatIndex())
		{
			changeOrderList = true;
		} else
		{
			maxOrders++;
			if(Order[ord] >= numPatterns) numPatterns = Order[ord] + 1;
		}
	}
	if(changeOrderList)
	{
		AddToLog("Skip and stop order list items (+++ and ---) are not saved in XM files.");
	}

	fileHeader.orders = maxOrders;
	fileHeader.patterns = numPatterns;
	fileHeader.size = fileHeader.size + maxOrders;

	uint16 writeInstruments;
	if(m_nInstruments > 0)
		fileHeader.instruments = writeInstruments = m_nInstruments;
	else
		fileHeader.instruments = writeInstruments = m_nSamples;

	if(m_SongFlags[SONG_LINEARSLIDES]) fileHeader.flags |= XMFileHeader::linearSlides;
	if(m_SongFlags[SONG_EXFILTERRANGE] && !compatibilityExport) fileHeader.flags |= XMFileHeader::extendedFilterRange;
	fileHeader.flags = fileHeader.flags;

	// Fasttracker 2 will happily accept any tempo faster than 255 BPM. XMPlay does also support this, great!
	fileHeader.tempo = static_cast<uint16>(m_nDefaultTempo);
	fileHeader.speed = static_cast<uint16>(Clamp(m_nDefaultSpeed, 1u, 31u));

	fileHeader.ConvertEndianness();
	fwrite(&fileHeader, 1, sizeof(fileHeader), f);

	// write order list (without +++ and ---, explained above)
	for(ORDERINDEX ord = 0; ord < Order.GetLengthTailTrimmed(); ord++)
	{
		if(Order[ord] != Order.GetIgnoreIndex() && Order[ord] != Order.GetInvalidPatIndex())
		{
			uint8 ordItem = static_cast<uint8>(Order[ord]);
			fwrite(&ordItem, 1, 1, f);
		}
	}

	// Writing patterns

#define ASSERT_CAN_WRITE(x) \
	if(len > s.size() - x) /*Buffer running out? Make it larger.*/ \
		s.resize(s.size() + 10 * 1024, 0);
	std::vector<uint8> s(64 * 64 * 5, 0);

	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		uint8 patHead[9];
		MemsetZero(patHead);
		patHead[0] = 9;

		if(!Patterns.IsValidPat(pat))
		{
			// There's nothing to write... chicken out.
			patHead[5] = 64;
			fwrite(patHead, 1, 9, f);
			continue;
		}

		patHead[5] = static_cast<uint8>(Patterns[pat].GetNumRows() & 0xFF);
		patHead[6] = static_cast<uint8>(Patterns[pat].GetNumRows() >> 8);

		const ModCommand *p = Patterns[pat];
		size_t len = 0;
		// Empty patterns are always loaded as 64-row patterns in FT2, regardless of their real size...
		bool emptyPattern = true;

		for(size_t j = m_nChannels * Patterns[pat].GetNumRows(); j > 0; j--, p++)
		{
			// Don't write more than 32 channels
			if(compatibilityExport && m_nChannels - ((j - 1) % m_nChannels) > 32) continue;

			uint8 note = p->note;
			uint8 command = p->command, param = p->param;
			ModSaveCommand(command, param, true, compatibilityExport);

			if (note >= NOTE_MIN_SPECIAL) note = 97; else
			if ((note <= 12) || (note > 96+12)) note = 0; else
			note -= 12;
			uint8 vol = 0;
			if (p->volcmd != VOLCMD_NONE)
			{
				switch(p->volcmd)
				{
				case VOLCMD_VOLUME:			vol = 0x10 + p->vol; break;
				case VOLCMD_VOLSLIDEDOWN:	vol = 0x60 + (p->vol & 0x0F); break;
				case VOLCMD_VOLSLIDEUP:		vol = 0x70 + (p->vol & 0x0F); break;
				case VOLCMD_FINEVOLDOWN:	vol = 0x80 + (p->vol & 0x0F); break;
				case VOLCMD_FINEVOLUP:		vol = 0x90 + (p->vol & 0x0F); break;
				case VOLCMD_VIBRATOSPEED:	vol = 0xA0 + (p->vol & 0x0F); break;
				case VOLCMD_VIBRATODEPTH:	vol = 0xB0 + (p->vol & 0x0F); break;
				case VOLCMD_PANNING:		vol = 0xC0 + (p->vol / 4); if (vol > 0xCF) vol = 0xCF; break;
				case VOLCMD_PANSLIDELEFT:	vol = 0xD0 + (p->vol & 0x0F); break;
				case VOLCMD_PANSLIDERIGHT:	vol = 0xE0 + (p->vol & 0x0F); break;
				case VOLCMD_TONEPORTAMENTO:	vol = 0xF0 + (p->vol & 0x0F); break;
				}
				// Those values are ignored in FT2. Don't save them, also to avoid possible problems with other trackers (or MPT itself)
				if(compatibilityExport && p->vol == 0)
				{
					switch(p->volcmd)
					{
					case VOLCMD_VOLUME:
					case VOLCMD_PANNING:
					case VOLCMD_VIBRATODEPTH:
					case VOLCMD_TONEPORTAMENTO:
					case VOLCMD_PANSLIDELEFT:	// Doesn't have memory, but does weird things with zero param.
						break;
					default:
						// no memory here.
						vol = 0;
					}
				}
			}

			// no need to fix non-empty patterns
			if(!p->IsEmpty())
				emptyPattern = false;

			// Apparently, completely empty patterns are loaded as empty 64-row patterns in FT2, regardless of their original size.
			// We have to avoid this, so we add a "break to row 0" command in the last row.
			if(j == 1 && emptyPattern && Patterns[pat].GetNumRows() != 64)
			{
				command = 0x0D;
				param = 0;
			}

			if ((note) && (p->instr) && (vol > 0x0F) && (command) && (param))
			{
				s[len++] = note;
				s[len++] = p->instr;
				s[len++] = vol;
				s[len++] = command;
				s[len++] = param;
			} else
			{
				uint8 b = 0x80;
				if (note) b |= 0x01;
				if (p->instr) b |= 0x02;
				if (vol >= 0x10) b |= 0x04;
				if (command) b |= 0x08;
				if (param) b |= 0x10;
				s[len++] = b;
				if (b & 1) s[len++] = note;
				if (b & 2) s[len++] = p->instr;
				if (b & 4) s[len++] = vol;
				if (b & 8) s[len++] = command;
				if (b & 16) s[len++] = param;
			}

			if(addChannel && (j % m_nChannels == 1 || m_nChannels == 1))
			{
				ASSERT_CAN_WRITE(1);
				s[len++] = 0x80;
			}

			ASSERT_CAN_WRITE(5);
		}

		if(emptyPattern && Patterns[pat].GetNumRows() == 64)
		{
			// Be smart when saving empty patterns!
			len = 0;
		}

		// Reaching the limits of file format?
		if(len > uint16_max)
		{
			AddToLog(mpt::String::Print("%1 (%2 %3)", str_tooMuchPatternData, str_pattern, pat));
			len = uint16_max;
		}

		patHead[7] = static_cast<uint8>(len & 0xFF);
		patHead[8] = static_cast<uint8>(len >> 8);
		fwrite(patHead, 1, 9, f);
		if(len) fwrite(&s[0], len, 1, f);
	}

#undef ASSERT_CAN_WRITE

	// Check which samples are referenced by which instruments (for assigning unreferenced samples to instruments)
	std::vector<bool> sampleAssigned(GetNumSamples() + 1, false);
	for(INSTRUMENTINDEX ins = 1; ins <= GetNumInstruments(); ins++)
	{
		if(Instruments[ins] != nullptr)
		{
			Instruments[ins]->GetSamples(sampleAssigned);
		}
	}

	// Writing instruments
	for(INSTRUMENTINDEX ins = 1; ins <= writeInstruments; ins++)
	{
		XMInstrumentHeader insHeader;
		std::vector<SAMPLEINDEX> samples;

		if(GetNumInstruments())
		{
			if(Instruments[ins] != nullptr)
			{
				// Convert instrument
				insHeader.ConvertToXM(*Instruments[ins], compatibilityExport);

				samples = insHeader.instrument.GetSampleList(*Instruments[ins], compatibilityExport);
				if(samples.size() > 0 && samples[0] <= GetNumSamples())
				{
					// Copy over auto-vibrato settings of first sample
					insHeader.instrument.ApplyAutoVibratoToXM(Samples[samples[0]], GetType());
				}

				std::vector<SAMPLEINDEX> additionalSamples;

				// Try to save "instrument-less" samples as well by adding those after the "normal" samples of our sample.
				// We look for unassigned samples directly after the samples assigned to our current instrument, so if
				// e.g. sample 1 is assigned to instrument 1 and samples 2 to 10 aren't assigned to any instrument,
				// we will assign those to sample 1. Any samples before the first referenced sample are going to be lost,
				// but hey, I wrote this mostly for preserving instrument texts in existing modules, where we shouldn't encounter this situation...
				for(std::vector<SAMPLEINDEX>::const_iterator sample = samples.begin(); sample != samples.end(); sample++)
				{
					SAMPLEINDEX smp = *sample;
					while(++smp <= GetNumSamples()
						&& !sampleAssigned[smp]
						&& insHeader.numSamples < (compatibilityExport ? 16 : 32))
					{
						sampleAssigned[smp] = true;			// Don't want to add this sample again.
						additionalSamples.push_back(smp);
						insHeader.numSamples++;
					}
				}

				samples.insert(samples.end(), additionalSamples.begin(), additionalSamples.end());
			} else
			{
				MemsetZero(insHeader);
			}
		} else
		{
			// Convert samples to instruments
			MemsetZero(insHeader);
			insHeader.numSamples = 1;
			insHeader.instrument.ApplyAutoVibratoToXM(Samples[ins], GetType());
			samples.push_back(ins);
		}

		insHeader.Finalise();
		size_t insHeaderSize = insHeader.size;
		insHeader.ConvertEndianness();
		fwrite(&insHeader, 1, insHeaderSize, f);

		std::vector<SampleIO> sampleFlags(samples.size());

		// Write Sample Headers
		for(SAMPLEINDEX smp = 0; smp < samples.size(); smp++)
		{
			XMSample xmSample;
			if(samples[smp] <= GetNumSamples())
			{
				xmSample.ConvertToXM(Samples[samples[smp]], GetType(), compatibilityExport);
			} else
			{
				MemsetZero(xmSample);
			}
			sampleFlags[smp] = xmSample.GetSampleFormat();

			mpt::String::Write<mpt::String::spacePadded>(xmSample.name, m_szNames[samples[smp]]);

			xmSample.ConvertEndianness();
			fwrite(&xmSample, 1, sizeof(xmSample), f);
		}

		// Write Sample Data
		for(SAMPLEINDEX smp = 0; smp < samples.size(); smp++)
		{
			if(samples[smp] <= GetNumSamples())
			{
				sampleFlags[smp].WriteSample(f, Samples[samples[smp]]);
			}
		}
	}

	if(!compatibilityExport)
	{
		// Writing song comments
		char magic[4];
		int32 size;
		if(!songMessage.empty())
		{
			memcpy(magic, "text", 4);
			fwrite(magic, 1, 4, f);

			size = songMessage.length();
			SwapBytesLE(size);
			fwrite(&size, 1, 4, f);

			fwrite(songMessage.c_str(), 1, songMessage.length(), f);
		}
		// Writing midi cfg
		if(m_SongFlags[SONG_EMBEDMIDICFG])
		{
			memcpy(magic, "MIDI", 4);
			fwrite(magic, 1, 4, f);

			size = sizeof(MIDIMacroConfigData);
			SwapBytesLE(size);
			fwrite(&size, 1, 4, f);

			fwrite(static_cast<MIDIMacroConfigData*>(&m_MidiCfg), 1, sizeof(MIDIMacroConfigData), f);
		}
		// Writing Pattern Names
		const PATTERNINDEX numNamedPats = Patterns.GetNumNamedPatterns();
		if(numNamedPats > 0)
		{
			memcpy(magic, "PNAM", 4);
			fwrite(magic, 1, 4, f);

			size = numNamedPats * MAX_PATTERNNAME;
			SwapBytesLE(size);
			fwrite(&size, 1, 4, f);

			for(PATTERNINDEX pat = 0; pat < numNamedPats; pat++)
			{
				char name[MAX_PATTERNNAME];
				MemsetZero(name);
				Patterns[pat].GetName(name);
				fwrite(name, 1, MAX_PATTERNNAME, f);
			}
		}
		// Writing Channel Names
		{
			CHANNELINDEX numNamedChannels = 0;
			for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
			{
				if (ChnSettings[chn].szName[0]) numNamedChannels = chn + 1;
			}
			// Do it!
			if(numNamedChannels)
			{
				memcpy(magic, "CNAM", 4);
				fwrite(magic, 1, 4, f);

				size = numNamedChannels * MAX_CHANNELNAME;
				SwapBytesLE(size);
				fwrite(&size, 1, 4, f);

				for(CHANNELINDEX chn = 0; chn < numNamedChannels; chn++)
				{
					fwrite(ChnSettings[chn].szName, 1, MAX_CHANNELNAME, f);
				}
			}
		}

		//Save hacked-on extra info
		SaveMixPlugins(f);
		if(m_nInstruments)
		{
			SaveExtendedInstrumentProperties(MIN(GetNumInstruments(), writeInstruments), f);
		}
		SaveExtendedSongProperties(f);
	}

	fclose(f);
	return true;
}

#endif // MODPLUG_NO_FILESAVE