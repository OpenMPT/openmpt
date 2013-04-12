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


// Allocate samples for an instrument
vector<SAMPLEINDEX> AllocateXMSamples(CSoundFile &sndFile, SAMPLEINDEX numSamples)
//--------------------------------------------------------------------------------
{
	LimitMax(numSamples, SAMPLEINDEX(32));

	vector<SAMPLEINDEX> foundSlots;

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
			vector<bool> usedSamples;
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
void ReadXMPatterns(FileReader &file, const XMFileHeader &fileHeader, CSoundFile &sndFile)
//----------------------------------------------------------------------------------------
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
					m->vol = ((m->vol * 64 + 8) / 15);
				}
			}
		}
	}
}


FLAGSET(TrackerVersions)
{
	verUnknown		= 0x00,	// Probably not made with MPT
	verOldModPlug	= 0x01,	// Made with MPT Alpha / Beta
	verNewModPlug	= 0x02,	// Made with MPT (not Alpha / Beta)
	verModPlug1_09	= 0x04,	// Made with MPT 1.09 or possibly other version
	verOpenMPT		= 0x08,	// Made with OpenMPT
	verConfirmed	= 0x10,	// We are very sure that we found the correct tracker version.
};


bool CSoundFile::ReadXM(FileReader &file)
//---------------------------------------
{
	file.Rewind();

	XMFileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| fileHeader.channels == 0
		|| fileHeader.channels > MAX_BASECHANNELS
		|| _strnicmp(fileHeader.signature, "Extended Module: ", 17)
		|| !Order.ReadAsByte(file, std::min(ORDERINDEX(fileHeader.orders), MAX_ORDERS))
		|| !file.Seek(fileHeader.size + 60))
	{
		return false;
	}

	FlagSet<TrackerVersions> madeWith(verUnknown);

	if(!memcmp(fileHeader.trackerName, "FastTracker v 2.00  ", 20))
	{
		madeWith = verOldModPlug;
	} else if(fileHeader.size == 276 && fileHeader.version == 0x0104 && !memcmp(fileHeader.trackerName, "FastTracker v2.00   ", 20))
	{
		madeWith = verNewModPlug;
	}

	StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[0], fileHeader.songName);

	ChangeModTypeTo(MOD_TYPE_XM);
	m_nMinPeriod = 27;
	m_nMaxPeriod = 54784;

	m_nRestartPos = fileHeader.restartPos;
	m_nChannels = fileHeader.channels;
	m_nInstruments = std::min(fileHeader.instruments, uint16(MAX_INSTRUMENTS - 1));
	m_nSamples = 0;
	m_nDefaultSpeed = Clamp(fileHeader.speed, uint16(1), uint16(31));
	m_nDefaultTempo = Clamp(fileHeader.tempo, uint16(32), uint16(512));

	m_SongFlags.reset();
	m_SongFlags.set(SONG_LINEARSLIDES, (fileHeader.flags & XMFileHeader::linearSlides) != 0);
	m_SongFlags.set(SONG_EXFILTERRANGE, (fileHeader.flags & XMFileHeader::extendedFilterRange) != 0);

	// set this here already because XMs compressed with BoobieSqueezer will exit the function early
	SetModFlag(MSF_COMPATIBLE_PLAY, true);

	if(fileHeader.version >= 0x0104)
	{
		ReadXMPatterns(file, fileHeader, *this);
	}

	// In case of XM versions < 1.04, we need to memorize the sample flags for all samples, as they are not stored immediately after the sample headers.
	vector<SampleIO> sampleFlags;
	uint8 sampleReserved = 0;

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
			madeWith |= verConfirmed;
			if(instrHeader.size == 245)
			{
				// ModPlug Tracker Alpha
				m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 00, 00, A5);
			} else if(instrHeader.size == 263)
			{
				// ModPlug Tracker Beta (Beta 1 still behaves like Alpha, but Beta 3.3 does it this way)
				m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 00, 00, B3);
			} else
			{
				// WTF?
				madeWith = (verUnknown | verConfirmed);
			}
		} else if(madeWith == verNewModPlug && instrHeader.numSamples == 0)
		{
			// Empty instruments make tracker identification pretty easy!
			madeWith = ((instrHeader.size == 263 && instrHeader.sampleHeaderSize == 0) ? verNewModPlug : verUnknown) | verConfirmed;
		}

		if(AllocateInstrument(instr) == nullptr)
		{
			continue;
		}

		instrHeader.ConvertToMPT(*Instruments[instr]);

		if(instrHeader.numSamples > 0)
		{
			// Yep, there are some samples associated with this instrument.
			if((instrHeader.instrument.midiEnabled | instrHeader.instrument.midiChannel | instrHeader.instrument.midiProgram | instrHeader.instrument.muteComputer) != 0)
			{
				// Definitely not an old MPT.
				madeWith = verUnknown;
			}

			// Read sample headers
			vector<SAMPLEINDEX> sampleSlots = AllocateXMSamples(*this, instrHeader.numSamples);

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
			vector<uint32> sampleSize(instrHeader.numSamples);

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

					StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[mptSample], sampleHeader.name);

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
					if(sample < sampleSlots.size())
					{
						sampleFlags[sample].ReadSample(Samples[sampleSlots[sample]], file);
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
		ReadMessage(file, file.ReadUint32LE(), leCR);
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
			file.ReadString<StringFixer::maybeNullTerminated>(patName, MAX_PATTERNNAME);
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
			file.ReadString<StringFixer::maybeNullTerminated>(ChnSettings[chn].szName, MAX_CHANNELNAME);
		}
		madeWith |= verConfirmed;
	}

	// Read mix plugins information
	if(file.BytesLeft() >= 8)
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
		} else if(madeWith[verNewModPlug])
		{
			m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 16, 00, 00);
		}
	}

	if(!memcmp(fileHeader.trackerName, "OpenMPT ", 8))
	{
		// Hey, I know this tracker!
		char mptVersion[13];
		memcpy(mptVersion, fileHeader.trackerName + 8, 12);
		StringFixer::SetNullTerminator(mptVersion);
		m_dwLastSavedWithVersion = MptVersion::ToNum(mptVersion);
		madeWith = verOpenMPT | verConfirmed;
	}

	if(m_dwLastSavedWithVersion != 0 && !madeWith[verOpenMPT])
	{
		m_nMixLevels = mixLevels_original;
		SetModFlag(MSF_COMPATIBLE_PLAY, false);
	}

	// Leave if no extra instrument settings are available (end of file reached)
	if(!file.BytesLeft()) return true;

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

	return true;
}


#ifndef MODPLUG_NO_FILESAVE
#include "../mptrack/Moddoc.h"	// for logging errors

#define str_tooMuchPatternData	(GetStrI18N((_TEXT("Warning: File format limit was reached. Some pattern data may not get written to file."))))
#define str_pattern				(GetStrI18N((_TEXT("pattern"))))


bool CSoundFile::SaveXM(LPCSTR lpszFileName, bool compatibilityExport)
//--------------------------------------------------------------------
{
	#define ASSERT_CAN_WRITE(x) \
	if(len > s.size() - x) /*Buffer running out? Make it larger.*/ \
		s.resize(s.size() + 10*1024, 0); \
	\
	if((len > uint16_max - (UINT)x) && GetpModDoc()) /*Reaching the limits of file format?*/ \
	{ \
 		mpt::String str; str.Format("%s (%s %u)", str_tooMuchPatternData, str_pattern, pat); \
		AddToLog(str); \
		break; \
	}

	FILE *f;
	if(lpszFileName == nullptr || (f = fopen(lpszFileName, "wb")) == nullptr)
	{
		return false;
	}

	vector<uint8> s(64 * 64 * 5, 0);
	bool addChannel = false; // avoid odd channel count for FT2 compatibility

	XMFileHeader fileHeader;
	MemsetZero(fileHeader);

	memcpy(fileHeader.signature, "Extended Module: ", 17);
	StringFixer::WriteString<StringFixer::spacePadded>(fileHeader.songName, m_szNames[0]);
	fileHeader.eof = 0x1A;
	memcpy(fileHeader.trackerName, "OpenMPT " MPT_VERSION_STR "  ", 20);

	// Writing song header
	fileHeader.version = 0x0104;					// XM Format v1.04
	fileHeader.size = sizeof(XMFileHeader) - 60;	// minus everything before this field
	fileHeader.restartPos = m_nRestartPos;

	fileHeader.channels = (m_nChannels + 1) & 0xFFFE; // avoid odd channel count for FT2 compatibility
	if((m_nChannels & 1) && m_nChannels < MAX_BASECHANNELS) addChannel = true;
	if(compatibilityExport && fileHeader.channels > 32)
		fileHeader.channels = 32;
	if(fileHeader.channels > MAX_BASECHANNELS) fileHeader.channels = MAX_BASECHANNELS;
	fileHeader.channels = fileHeader.channels;

	// Find out number of orders and patterns used.
	// +++ and --- patterns are not taken into consideration as FastTracker does not support them.
	ORDERINDEX nMaxOrds = 0;
	PATTERNINDEX nPatterns = 0;
	for(ORDERINDEX ord = 0; ord < Order.GetLengthTailTrimmed(); ord++)
	{
		if(Patterns.IsValidIndex(Order[ord]))
		{
			nMaxOrds++;
			if(Order[ord] >= nPatterns) nPatterns = Order[ord] + 1;
		}
	}
	if(!compatibilityExport) nMaxOrds = Order.GetLengthTailTrimmed(); // should really be removed at some point

	fileHeader.orders = nMaxOrds;
	fileHeader.patterns = nPatterns;
	fileHeader.size = fileHeader.size + nMaxOrds;

	uint16 writeInstruments;
	if(m_nInstruments > 0)
		fileHeader.instruments = writeInstruments = m_nInstruments;
	else
		fileHeader.instruments = writeInstruments = m_nSamples;

	if(m_SongFlags[SONG_LINEARSLIDES]) fileHeader.flags |= XMFileHeader::linearSlides;
	if(m_SongFlags[SONG_EXFILTERRANGE] && !compatibilityExport) fileHeader.flags |= XMFileHeader::extendedFilterRange;
	fileHeader.flags = fileHeader.flags;

// 	if(compatibilityExport)
// 		xmheader.tempo = static_cast<uint16>(Clamp(m_nDefaultTempo, 32u, 255u));
// 	else
	// Fasttracker 2 will happily accept any tempo faster than 255 BPM. XMPlay does also support this, great!
	fileHeader.tempo = static_cast<uint16>(Clamp(m_nDefaultTempo, 32u, 512u));
	fileHeader.speed = static_cast<uint16>(Clamp(m_nDefaultSpeed, 1u, 31u));

	fileHeader.ConvertEndianness();
	fwrite(&fileHeader, 1, sizeof(fileHeader), f);

	// write order list (without +++ and ---, explained above)
	for(ORDERINDEX ord = 0; ord < Order.GetLengthTailTrimmed(); ord++)
	{
		if(Patterns.IsValidIndex(Order[ord]) || !compatibilityExport)
		{
			uint8 ordItem = static_cast<uint8>(Order[ord]);
			fwrite(&ordItem, 1, 1, f);
		}
	}

	// Writing patterns
	for(PATTERNINDEX pat = 0; pat < nPatterns; pat++)
	{
		uint8 patHead[9];
		MemsetZero(patHead);
		patHead[0] = 9;
		patHead[5] = static_cast<uint8>(Patterns[pat].GetNumRows() & 0xFF);
		patHead[6] = static_cast<uint8>(Patterns[pat].GetNumRows() >> 8);

		if(!Patterns.IsValidPat(pat))
		{
			// There's nothing to write... chicken out.
			fwrite(patHead, 1, 9, f);
			continue;
		}

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
				case VOLCMD_PANNING:		vol = 0xC0 + ((p->vol * 15 + 32) / 64); if (vol > 0xCF) vol = 0xCF; break;
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

		patHead[7] = static_cast<uint8>(len & 0xFF);
		patHead[8] = static_cast<uint8>(len >> 8);
		fwrite(patHead, 1, 9, f);
		if(len) fwrite(&s[0], 1, len, f);
	}

	// Check which samples are referenced by which instruments (for assigning unreferenced samples to instruments)
	vector<bool> sampleAssigned(GetNumSamples() + 1, false);
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
		vector<SAMPLEINDEX> samples;

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

				vector<SAMPLEINDEX> additionalSamples;

				// Try to save "instrument-less" samples as well by adding those after the "normal" samples of our sample.
				// We look for unassigned samples directly after the samples assigned to our current instrument, so if
				// e.g. sample 1 is assigned to instrument 1 and samples 2 to 10 aren't assigned to any instrument,
				// we will assign those to sample 1. Any samples before the first referenced sample are going to be lost,
				// but hey, I wrote this mostly for preserving instrument texts in existing modules, where we shouldn't encounter this situation...
				for(vector<SAMPLEINDEX>::const_iterator sample = samples.begin(); sample != samples.end(); sample++)
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

		vector<SampleIO> sampleFlags(samples.size());

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

			StringFixer::WriteString<StringFixer::spacePadded>(xmSample.name, m_szNames[samples[smp]]);

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
		if ((m_lpszSongComments) && (m_lpszSongComments[0]))
		{
			DWORD d = 0x74786574;
			fwrite(&d, 1, 4, f);
			d = strlen(m_lpszSongComments);
			fwrite(&d, 1, 4, f);
			fwrite(m_lpszSongComments, 1, d, f);
		}
		// Writing midi cfg
		if (m_SongFlags[SONG_EMBEDMIDICFG])
		{
			DWORD d = 0x4944494D;
			fwrite(&d, 1, 4, f);
			d = sizeof(MIDIMacroConfig);
			fwrite(&d, 1, 4, f);
			fwrite(&m_MidiCfg, 1, sizeof(MIDIMacroConfig), f);
		}
		// Writing Pattern Names
		const PATTERNINDEX numNamedPats = Patterns.GetNumNamedPatterns();
		if (numNamedPats > 0)
		{
			DWORD dwLen = numNamedPats * MAX_PATTERNNAME;
			DWORD d = 0x4d414e50;
			fwrite(&d, 1, 4, f);
			fwrite(&dwLen, 1, 4, f);

			for(PATTERNINDEX nPat = 0; nPat < numNamedPats; nPat++)
			{
				char name[MAX_PATTERNNAME];
				MemsetZero(name);
				Patterns[nPat].GetName(name);
				fwrite(name, 1, MAX_PATTERNNAME, f);
			}
		}
		// Writing Channel Names
		{
			UINT nChnNames = 0;
			for (UINT inam=0; inam<m_nChannels; inam++)
			{
				if (ChnSettings[inam].szName[0]) nChnNames = inam+1;
			}
			// Do it!
			if (nChnNames)
			{
				DWORD dwLen = nChnNames * MAX_CHANNELNAME;
				DWORD d = 0x4d414e43;
				fwrite(&d, 1, 4, f);
				fwrite(&dwLen, 1, 4, f);
				for (UINT inam=0; inam<nChnNames; inam++)
				{
					fwrite(ChnSettings[inam].szName, 1, MAX_CHANNELNAME, f);
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