/*
 * Load_mod.cpp
 * ------------
 * Purpose: MOD / NST (ProTracker / NoiseTracker) module loader / saver
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

void CSoundFile::ConvertModCommand(ModCommand &m) const
//-----------------------------------------------------
{
	switch(m.command)
	{
	case 0x00:	if(m.param) m.command = CMD_ARPEGGIO; break;
	case 0x01:	m.command = CMD_PORTAMENTOUP; break;
	case 0x02:	m.command = CMD_PORTAMENTODOWN; break;
	case 0x03:	m.command = CMD_TONEPORTAMENTO; break;
	case 0x04:	m.command = CMD_VIBRATO; break;
	case 0x05:	m.command = CMD_TONEPORTAVOL; break;
	case 0x06:	m.command = CMD_VIBRATOVOL; break;
	case 0x07:	m.command = CMD_TREMOLO; break;
	case 0x08:	m.command = CMD_PANNING8; break;
	case 0x09:	m.command = CMD_OFFSET; break;
	case 0x0A:	m.command = CMD_VOLUMESLIDE; break;
	case 0x0B:	m.command = CMD_POSITIONJUMP; break;
	case 0x0C:	m.command = CMD_VOLUME; break;
	case 0x0D:	m.command = CMD_PATTERNBREAK; m.param = ((m.param >> 4) * 10) + (m.param & 0x0F); break;
	case 0x0E:	m.command = CMD_MODCMDEX; break;
	case 0x0F:
		// Speed is 01...1F in XM, but 01...20 in MOD. 15-sample Soundtracker MODs always use VBlank timing, interpret all values as speed.
		if(((GetType() & MOD_TYPE_MOD) && (GetNumSamples() == 15 || m.param <= 0x20u)) || (!(GetType() & MOD_TYPE_MOD) && m.param <= 0x1Fu))
			m.command = CMD_SPEED;
		else
			m.command = CMD_TEMPO;
		break;

	// Extension for XM extended effects
	case 'G' - 55:	m.command = CMD_GLOBALVOLUME; break;		//16
	case 'H' - 55:	m.command = CMD_GLOBALVOLSLIDE; break;
	case 'K' - 55:	m.command = CMD_KEYOFF; break;
	case 'L' - 55:	m.command = CMD_SETENVPOSITION; break;
	case 'M' - 55:	m.command = CMD_CHANNELVOLUME; break;
	case 'N' - 55:	m.command = CMD_CHANNELVOLSLIDE; break;
	case 'P' - 55:	m.command = CMD_PANNINGSLIDE; break;
	case 'R' - 55:	m.command = CMD_RETRIG; break;
	case 'T' - 55:	m.command = CMD_TREMOR; break;
	case 'X' - 55:	m.command = CMD_XFINEPORTAUPDOWN;	break;
	case 'Y' - 55:	m.command = CMD_PANBRELLO; break;			//34
	case 'Z' - 55:	m.command = CMD_MIDI;	break;				//35
	case '\\' - 56:	m.command = CMD_SMOOTHMIDI;	break;		//rewbs.smoothVST: 36 - note: this is actually displayed as "-" in FT2, but seems to be doing nothing.
	//case ':' - 21:	m.command = CMD_DELAYCUT;	break;		//37
	case '#' + 3:	m.command = CMD_XPARAM;	break;			//rewbs.XMfixes - Xm.param is 38
	default:		m.command = CMD_NONE;
	}
}


void CSoundFile::ModSaveCommand(uint8 &command, uint8 &param, bool toXM, bool compatibilityExport) const
//------------------------------------------------------------------------------------------------------
{
	switch(command)
	{
	case CMD_NONE:		command = param = 0; break;
	case CMD_ARPEGGIO:	command = 0; break;
	case CMD_PORTAMENTOUP:
		if (GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_STM|MOD_TYPE_MPT))
		{
			if ((param & 0xF0) == 0xE0) { command = 0x0E; param = ((param & 0x0F) >> 2) | 0x10; break; }
			else if ((param & 0xF0) == 0xF0) { command = 0x0E; param &= 0x0F; param |= 0x10; break; }
		}
		command = 0x01;
		break;
	case CMD_PORTAMENTODOWN:
		if(GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_STM|MOD_TYPE_MPT))
		{
			if ((param & 0xF0) == 0xE0) { command = 0x0E; param= ((param & 0x0F) >> 2) | 0x20; break; }
			else if ((param & 0xF0) == 0xF0) { command = 0x0E; param &= 0x0F; param |= 0x20; break; }
		}
		command = 0x02;
		break;
	case CMD_TONEPORTAMENTO:	command = 0x03; break;
	case CMD_VIBRATO:			command = 0x04; break;
	case CMD_TONEPORTAVOL:		command = 0x05; break;
	case CMD_VIBRATOVOL:		command = 0x06; break;
	case CMD_TREMOLO:			command = 0x07; break;
	case CMD_PANNING8:
		command = 0x08;
		if(m_nType & MOD_TYPE_S3M)
		{
			if(param <= 0x80)
			{
				param = min(param << 1, 0xFF);
			}
			else if(param == 0xA4)	// surround
			{
				if(compatibilityExport || !toXM)
				{
					command = param = 0;
				}
				else
				{
					command = 'X' - 55;
					param = 91;
				}
			}
		}
		break;
	case CMD_OFFSET:			command = 0x09; break;
	case CMD_VOLUMESLIDE:		command = 0x0A; break;
	case CMD_POSITIONJUMP:		command = 0x0B; break;
	case CMD_VOLUME:			command = 0x0C; break;
	case CMD_PATTERNBREAK:		command = 0x0D; param = ((param / 10) << 4) | (param % 10); break;
	case CMD_MODCMDEX:			command = 0x0E; break;
	case CMD_SPEED:				command = 0x0F; param = min(param, (toXM) ? 0x1Fu : 0x20u); break;
	case CMD_TEMPO:				command = 0x0F; param = max(param, (toXM) ? 0x20u : 0x21u); break;
	case CMD_GLOBALVOLUME:		command = 'G' - 55; break;
	case CMD_GLOBALVOLSLIDE:	command = 'H' - 55; break;
	case CMD_KEYOFF:			command = 'K' - 55; break;
	case CMD_SETENVPOSITION:	command = 'L' - 55; break;
	case CMD_CHANNELVOLUME:		command = 'M' - 55; break;
	case CMD_CHANNELVOLSLIDE:	command = 'N' - 55; break;
	case CMD_PANNINGSLIDE:		command = 'P' - 55; break;
	case CMD_RETRIG:			command = 'R' - 55; break;
	case CMD_TREMOR:			command = 'T' - 55; break;
	case CMD_XFINEPORTAUPDOWN:	command = 'X' - 55;
		if(compatibilityExport && param >= 0x30)	// X1x and X2x are legit, everything above are MPT extensions, which don't belong here.
			param = 0;	// Don't set command to 0 to indicate that there *was* some X command here...
		break;
	case CMD_PANBRELLO:
		if(compatibilityExport)
			command = param = 0;
		else
			command = 'Y' - 55;
		break;
	case CMD_MIDI:
		if(compatibilityExport)
			command = param = 0;
		else
			command = 'Z' - 55;
		break;
	case CMD_SMOOTHMIDI: //rewbs.smoothVST: 36
		if(compatibilityExport)
			command = param = 0;
		else
			command = '\\' - 56;
		break;
	case CMD_XPARAM: //rewbs.XMfixes - XParam is 38
		if(compatibilityExport)
			command = param = 0;
		else
			command = '#' + 3;
		break;
	case CMD_S3MCMDEX:
		switch(param & 0xF0)
		{
		case 0x10:	command = 0x0E; param = (param & 0x0F) | 0x30; break;
		case 0x20:	command = 0x0E; param = (param & 0x0F) | 0x50; break;
		case 0x30:	command = 0x0E; param = (param & 0x0F) | 0x40; break;
		case 0x40:	command = 0x0E; param = (param & 0x0F) | 0x70; break;
		case 0x90:
			if(compatibilityExport)
				command = param = 0;
			else
				command = 'X' - 55;
			break;
		case 0xB0:	command = 0x0E; param = (param & 0x0F) | 0x60; break;
		case 0xA0:
		case 0x50:
		case 0x70:
		case 0x60:	command = param = 0; break;
		default:	command = 0x0E; break;
		}
		break;
	default:
		command = param = 0;
	}

	// Don't even think about saving XM effects in MODs...
	if(command > 0x0F && !toXM)
	{
		command = param = 0;
	}
}


#pragma pack(push, 1)

// File Header
struct MODFileHeader
{
	uint8 numOrders;
	uint8 restartPos;
	uint8 orderList[128];
};

STATIC_ASSERT(sizeof(MODFileHeader) == 130);


// Sample Header
struct MODSampleHeader
{
	char   name[22];
	uint16 length;
	uint8  finetune;
	uint8  volume;
	uint16 loopStart;
	uint16 loopLength;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(length);
		SwapBytesBE(loopStart);
		SwapBytesBE(loopLength);
	}

	// Convert an MOD sample header to OpenMPT's internal sample header.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize(MOD_TYPE_MOD);
		mptSmp.nLength = length * 2;
		mptSmp.nFineTune = MOD2XMFineTune(finetune & 0x0F);
		mptSmp.nVolume = 4 * min(volume, 64);

		SmpLength lStart = loopStart * 2;
		SmpLength lLength = loopLength * 2;
		// Fix loops
		if(lLength > 2 && (lStart + lLength > mptSmp.nLength)
			&& (lStart / 2 + lLength <= mptSmp.nLength))
		{
			lStart /= 2;
		}
		mptSmp.nLoopStart = lStart;
		mptSmp.nLoopEnd = lStart + lLength;

		if(mptSmp.nLength == 2)
		{
			mptSmp.nLength = 0;
		}

		if(mptSmp.nLength)
		{
			if(mptSmp.nLoopStart >= mptSmp.nLength)
			{
				mptSmp.nLoopStart = mptSmp.nLength - 1;
			}
			if(mptSmp.nLoopEnd > mptSmp.nLength)
			{
				mptSmp.nLoopEnd = mptSmp.nLength;
			}
			if(mptSmp.nLoopStart > mptSmp.nLoopEnd || mptSmp.nLoopEnd < 4 || mptSmp.nLoopEnd - mptSmp.nLoopStart < 4)
			{
				mptSmp.nLoopStart = 0;
				mptSmp.nLoopEnd = 0;
			}

			// Fix for most likely broken sample loops. This fixes super_sufm_-_new_life.mod which has a long sample which is looped from 0 to 4.
			if(mptSmp.nLoopEnd <= 8 && mptSmp.nLoopStart == 0 && mptSmp.nLength > mptSmp.nLoopEnd)
			{
				mptSmp.nLoopEnd = 0;
			}
			if (mptSmp.nLoopEnd > mptSmp.nLoopStart)
			{
				mptSmp.uFlags.set(CHN_LOOP);
			}
		}
	}

	// Convert OpenMPT's internal sample header to an MOD sample header.
	SmpLength ConvertToMOD(const ModSample &mptSmp)
	{
		SmpLength writeLength = mptSmp.pSample != nullptr ? mptSmp.nLength : 0;
		// If the sample size is odd, we have to add a padding byte, as all sample sizes in MODs are even.
		if((writeLength % 2u) != 0)
		{
			writeLength++;
		}
		LimitMax(writeLength, SmpLength(0x1FFF0));

		length = static_cast<uint16>(writeLength / 2);

		if(mptSmp.RelativeTone < 0)
		{
			finetune = 0x08;
		} else if(mptSmp.RelativeTone > 0)
		{
			finetune = 0x07;
		} else
		{
			finetune = XM2MODFineTune(mptSmp.nFineTune);
		}
		volume = static_cast<uint8>(mptSmp.nVolume / 4);

		loopStart = 0;
		loopLength = 1;
		if(mptSmp.uFlags[CHN_LOOP])
		{
			loopStart = static_cast<uint16>(mptSmp.nLoopStart / 2);
			loopLength = static_cast<uint16>(max(1, (mptSmp.nLoopEnd - mptSmp.nLoopStart) / 2));
		}

		return writeLength;
	}


	// Retrieve the internal sample format flags for this sample.
	static SampleIO GetSampleFormat()
	{
		return SampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::bigEndian,
			SampleIO::signedPCM);
	}
};

STATIC_ASSERT(sizeof(MODSampleHeader) == 30);


#pragma pack(pop)


// Functor for fixing VBlank MODs and MODs with 7-bit panning
struct FixMODPatterns
//===================
{
	FixMODPatterns(bool fixVBlank, bool fixPanning)
	{
		this->fixVBlank = fixVBlank;
		this->fixPanning = fixPanning;
	}

	void operator()(ModCommand &m)
	{
		if(m.command == CMD_TEMPO && this->fixVBlank)
		{
			// Fix VBlank MODs
			m.command = CMD_SPEED;
		} else if(m.command == CMD_PANNING8 && this->fixPanning)
		{
			// Fix MODs with 7-bit + surround panning
			if(m.param == 0xA4)
			{
				m.command = CMD_S3MCMDEX;
				m.param = 0x91;
			} else
			{
				m.param = min(m.param * 2, 0xFF);
			}
		}
	}

	bool fixVBlank, fixPanning;
};


// Check if header magic equals a given string.
static bool IsMagic(const char *magic1, const char *magic2)
//---------------------------------------------------------
{
	return (*reinterpret_cast<const uint32 *>(magic1) == *reinterpret_cast<const uint32 *>(magic2));
}


static void ReadSample(FileReader &file, MODSampleHeader &sampleHeader, ModSample &sample, char (&sampleName)[MAX_SAMPLENAME])
//----------------------------------------------------------------------------------------------------------------------------
{
	file.ReadConvertEndianness(sampleHeader);
	sampleHeader.ConvertToMPT(sample);

	StringFixer::ReadString<StringFixer::spacePadded>(sampleName, sampleHeader.name);
}


// Parse the order list to determine how many patterns are used in the file.
static PATTERNINDEX GetNumPatterns(const FileReader &file, ModSequence &Order, ORDERINDEX numOrders, size_t totalSampleLen, CHANNELINDEX &numChannels, bool checkForWOW)
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	PATTERNINDEX numPatterns = 0;			// Total number of patterns in file (determined by going through the whole order list) with pattern number < 128
	PATTERNINDEX officialPatterns = 0;		// Number of patterns only found in the "official" part of the order list (i.e. order positions < claimed order length)
	PATTERNINDEX numPatternsIllegal = 0;	// Total number of patterns in file, also counting in "invalid" pattern indexes >= 128

	for(ORDERINDEX ord = 0; ord < 128; ord++)
	{
		PATTERNINDEX pat = Order[ord];
		if(pat < 128 && numPatterns <= pat)
		{
			numPatterns = pat + 1;
			if(ord < numOrders)
			{
				officialPatterns = numPatterns;
			}
		}
		if(pat >= numPatternsIllegal)
		{
			numPatternsIllegal = pat + 1;
		}
	}

	// Fill order tail with stop patterns, now that we don't need the garbage in there anymore.
	for(ORDERINDEX ord = numOrders; ord < 128; ord++)
	{
		Order[ord] = Order.GetInvalidPatIndex();
	}

	const size_t patternStartOffset = 20 + 31 * sizeof(MODSampleHeader) + sizeof(MODFileHeader) + 4;
	const size_t sizeWithoutPatterns = totalSampleLen + patternStartOffset;

	if(checkForWOW)
	{
		// Check if this is a Mod's Grave WOW file... Never seen one of those, but apparently they *do* exist.
		// Basically, WOW files seem to use the M.K. extensions, but are actually 8CHN files.
		if(sizeWithoutPatterns + numPatterns * 8 * 256 == file.GetLength())
		{
			numChannels = 8;
		}
	}

	// Now we have to check if the "hidden" patterns in the order list are actually real, i.e. if they are saved in the file.
	if(numPatterns != officialPatterns && sizeWithoutPatterns + numPatterns * numChannels * 256 != file.GetLength())
	{
		// Ok, size does not match the number of patterns including "hidden" patterns. Does it match the number of "official" patterns?
		if(sizeWithoutPatterns + officialPatterns * numChannels * 256 == file.GetLength())
		{
			// It does! Forget about that crap.
			numPatterns = officialPatterns;
		}
	} else if(numPatternsIllegal > numPatterns && sizeWithoutPatterns + numPatternsIllegal * numChannels * 256 == file.GetLength())
	{
		// Even those illegal pattern indexes (> 128) appear to be valid... What a weird file!
		numPatterns = numPatternsIllegal;
	}

	return numPatterns;
}


bool CSoundFile::ReadMod(FileReader &file)
//----------------------------------------
{
	char magic[4];
	if(!file.Seek(1080) || !file.ReadArray(magic))
	{
		return false;
	}

	m_nChannels = 4;

	// Check MOD Magic
	if(IsMagic(magic, "M.K.")		// ProTracker and compatible
		|| IsMagic(magic, "M!K!")	// ProTracker (64+ patterns)
		|| IsMagic(magic, "M&K!")	// NoiseTracker
		|| IsMagic(magic, "N.T.")	// NoiseTracker
		|| IsMagic(magic, "FEST"))	// jobbig.mod by Mahoney
	{
		m_nChannels = 4;
	} else if(IsMagic(magic, "CD81")	// Falcon
		|| IsMagic(magic, "OKTA")		// Oktalyzer
		|| IsMagic(magic, "OCTA"))		// Oktalyzer
	{
		m_nChannels = 8;
	} else if((!memcmp(magic, "FLT", 3) || !memcmp(magic, "EXO", 3)) && magic[3] >= '4' && magic[3] <= '9')
	{
		// FLTx / EXOx - Startrekker by Exolon / Fairlight
		m_nChannels = magic[3] - '0';
	} else if(magic[0] >= '1' && magic[0] <= '9' && !memcmp(magic + 1, "CHN", 3))
	{
		// xCHN - Many trackers
		m_nChannels = magic[0] - '0';
	} else if(magic[0] >= '1' && magic[0] <= '9' && magic[1]>='0' && magic[1] <= '9'
		&& (!memcmp(magic + 2, "CH", 2) || !memcmp(magic + 2, "CN", 2)))
	{
		// xxCN / xxCH - Many trackers
		m_nChannels = (magic[0] - '0') * 10 + magic[1] - '0';
	} else if(!memcmp(magic, "TDZ", 3) && magic[3] >= '4' && magic[3] <= '9')
	{
		// TDZx - TakeTracker
		m_nChannels = magic[3] - '0';
	} else
	{
		return false;
	}

	LimitMax(m_nChannels, MAX_BASECHANNELS);

	// Startrekker 8 channel mod (needs special treatment, see below)
	bool isFLT8 = IsMagic(magic, "FLT8");
	// Only apply VBlank tests to M.K. (ProTracker) modules.
	const bool isMdKd = IsMagic(magic, "M.K.");

	// Reading song title
	file.Seek(0);
	file.ReadString<StringFixer::spacePadded>(m_szNames[0], 20);

	// Load Samples
	size_t totalSampleLen = 0;
	m_nSamples = 31;
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		MODSampleHeader sampleHeader;
		ReadSample(file, sampleHeader, Samples[smp], m_szNames[smp]);

		totalSampleLen += Samples[smp].nLength;
	}

	// Read order information
	MODFileHeader fileHeader;
	file.Read(fileHeader);
	file.Skip(4);	// Magic bytes (we already parsed these)

	Order.ReadFromArray(fileHeader.orderList);

	ORDERINDEX realOrders = fileHeader.numOrders;
	if(realOrders > 128)
	{
		// beatwave.mod by Sidewinder (the version from ModArchive, not ModLand) claims to have 129 orders.
		realOrders = 128;
	} else if(realOrders == 0)
	{
		// Is this necessary?
		realOrders = 128;
		while(realOrders > 1 && Order[realOrders - 1] == 0)
		{
			realOrders--;
		}
	}

	// Do some order sanity checks
	PATTERNINDEX numPatterns = GetNumPatterns(file, Order, realOrders, totalSampleLen, m_nChannels, isMdKd);

	if(isFLT8)
	{
		// FLT8 has only even order items, so divide by two.
		for(ORDERINDEX ord = 0; ord < Order.GetLength(); ord++)
		{
			Order[ord] /= 2;
		}
	}
	
	// Restart position sanity checks
	realOrders--;
	m_nRestartPos = fileHeader.restartPos;

	// About the weird 0x78 restart pos thing... I also have no idea where it comes from, XMP also has this check but doesn't comment on it, either.
	// It is said that NoiseTracker writes 0x78, but I've also seen this in M15 files.
	// Files that have restart pos == 0x78: action's batman by DJ Uno (M.K.), 3ddance.mod (M15), VALLEY.MOD (M.K.), WormsTDC.MOD (M.K.), ZWARTZ.MOD (M.K.)
	ASSERT(m_nRestartPos != 0x78 || m_nRestartPos + 1u >= realOrders);
	if(m_nRestartPos >= 128 || m_nRestartPos + 1u >= realOrders || m_nRestartPos == 0x78)
	{
		m_nRestartPos = 0;
	}

	// Now we can be pretty sure that this is a valid MOD file. Set up default song settings.
	m_nType = MOD_TYPE_MOD;
	m_nInstruments = 0;
	m_nDefaultSpeed = 6;
	m_nDefaultTempo = 125;
	m_nMinPeriod = 14 * 4;
	m_nMaxPeriod = 3424 * 4;
	m_SongFlags.reset();

	// Setup channel pan positions and volume
	SetupMODPanning();

	const CHANNELINDEX readChannels = (isFLT8 ? 4 : m_nChannels); // 4 channels per pattern in FLT8 format.
	if(isFLT8) numPatterns++; // as one logical pattern consists of two real patterns in FLT8 format, the highest pattern number has to be increased by one.
	bool hasTempoCommands = false;	// for detecting VBlank MODs
	bool leftPanning = false, extendedPanning = false;	// for detecting 800-880 panning

	// Reading patterns
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		if(pat < MAX_PATTERNS)
		{
			PATTERNINDEX actualPattern = pat;

			if(isFLT8)
			{
				if((pat % 2u) == 0)
				{
					// Only create "even" patterns for FLT8 files
					if(Patterns.Insert(pat / 2, 64))
					{
						break;
					}
				}
				actualPattern /= 2;
			} else
			{
				if(Patterns.Insert(pat, 64))
				{
					break;
				}
			}

			// For detecting PT1x mode
			vector<ModCommand::INSTR> lastInstrument(GetNumChannels(), 0);
			vector<int> instrWithoutNoteCount(GetNumChannels(), 0);

			for(ROWINDEX row = 0; row < 64; row++)
			{
				// FLT8: either write to channel 1 to 4 (even patterns) or 5 to 8 (odd patterns).
				ModCommand *rowBase = Patterns[actualPattern].GetpModCommand(row, ((pat % 2u) == 0 || !isFLT8) ? 0 : 4);

				for(CHANNELINDEX chn = 0; chn < readChannels; chn++)
				{
					ModCommand &m = rowBase[chn];

					uint8 data[4];
					file.ReadArray(data);

					// Read Period
					uint16 period = (((static_cast<uint16>(data[0]) & 0x0F) << 8) | data[1]);
					if(period > 0 && period != 0xFFF)
					{
						m.note = static_cast<ModCommand::NOTE>(GetNoteFromPeriod(period * 4));
					}
					// Read Instrument
					m.instr = (data[2] >> 4) | (data[0] & 0x10);
					// Read Effect
					m.command = data[2] & 0x0F;
					m.param = data[3];

					if(m.command || m.param)
					{
						ConvertModCommand(m);
					}

					// Perform some checks for our heuristics...
					if(m.command == CMD_TEMPO && m.param < 100)
						hasTempoCommands = true;
					if(m.command == CMD_PANNING8 && m.param < 0x80)
						leftPanning = true;
					if(m.command == CMD_PANNING8 && m.param > 0x80 && m.param != 0xA4)
						extendedPanning = true;
					if(m.note == NOTE_NONE && m.instr > 0 && !isFLT8)
					{
						if(lastInstrument[chn] > 0 && lastInstrument[chn] != m.instr)
						{
							// Arbitrary threshold for going into PT1x mode: 4 consecutive "sample swaps" in one pattern.
							if(++instrWithoutNoteCount[chn] >= 4)
							{
								m_SongFlags.set(SONG_PT1XMODE);
							}
						}
					} else if(m.note != NOTE_NONE)
					{
						instrWithoutNoteCount[chn] = 0;
					}
					if(m.instr != 0)
					{
						lastInstrument[chn] = m.instr;
					}
				}
			}
		}
	}

	// Reading samples
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		MODSampleHeader::GetSampleFormat().ReadSample(Samples[smp], file);
	}

	// Fix VBlank MODs. Arbitrary threshold: 10 minutes.
	// Basically, this just converts all tempo commands into speed commands
	// for MODs which are supposed to have VBlank timing (instead of CIA timing).
	// There is no perfect way to do this, since both MOD types look the same,
	// but the most reliable way is to simply check for extremely long songs
	// (as this would indicate that e.g. a F30 command was really meant to set
	// the ticks per row to 48, and not the tempo to 48 BPM).
	// In the pattern loader above, a second condition is used: Only tempo commands
	// below 100 BPM are taken into account. Furthermore, only M.K. (ProTracker)
	// modules are checked.
	const bool fixVBlank = isMdKd && hasTempoCommands && GetSongTime() >= 10 * 60;
	const bool fix7BitPanning = leftPanning && !extendedPanning;
	if(fixVBlank || fix7BitPanning)
	{
		Patterns.ForEachModCommand(FixMODPatterns(fixVBlank, fix7BitPanning));
	}

	return true;
}


// Check if a name string is valid (i.e. doesn't contain binary garbage data)
static bool IsValidName(const char *s, size_t length, char minChar)
//-----------------------------------------------------------------
{
	size_t i;
	// Check for garbage characters
	for(i = 0; i < length; i++)
	{
		if(s[i] && s[i] < minChar)
		{
			return false;
		}
	}
	// Check for garbage after null terminator.
	i = static_cast<const char *>(memchr(s, '\0', length)) - s;
	for(; i < length; i++)
	{
		if(s[i] && s[i] < ' ')
		{
			return false;
		}
	}
	return true;
}


bool CSoundFile::ReadM15(FileReader &file)
//----------------------------------------
{
	file.Rewind();

	char songname[20];
	file.ReadArray(songname);
	if(!IsValidName(songname, sizeof(songname), ' ')
		|| file.BytesLeft() < sizeof(MODSampleHeader) * 15 + sizeof(MODFileHeader))
	{
		return false;
	}

	// We'll have to do some heuristic checks to find out whether this is an old Ultimate Soundtracker module
	// or if it was made with the newer Soundtracker versions.
	bool isUST = false, isConfirmed = false;

	size_t totalSampleLen = 0;
	m_nSamples = 15;
	for(SAMPLEINDEX smp = 1; smp <= 15; smp++)
	{
		MODSampleHeader sampleHeader;
		ReadSample(file, sampleHeader, Samples[smp], m_szNames[smp]);

		// Sanity checks
		if(!IsValidName(sampleHeader.name, sizeof(sampleHeader.name), 14)
			|| sampleHeader.volume > 64
			|| (sampleHeader.finetune >> 4) != 0
			|| sampleHeader.length > 32768)
		{
			return false;
		}

		totalSampleLen += Samples[smp].nLength;

		if(isConfirmed)
		{
			continue;
		}

		// Soundtracker sample names should always start with "S", "ST-" or a number, but for UST mods, this is not the case.
		// sll17.3.mod from ModLand has Soundtracker pattern effects but no "st-" sample names. Is this a bad / modified rip?
		if((memcmp(sampleHeader.name, "st-", 3) && memcmp(sampleHeader.name, "ST-", 3)) || sampleHeader.name[0] < '0' || sampleHeader.name[0] > '9')
		{
			isUST = true;
		}

		// UST only handles samples up to 9999 bytes.
		if(sampleHeader.length > 4999 || sampleHeader.loopStart > 9999)
		{
			isUST = false;
		}
		// If loop information is incorrect as words, but correct as bytes, this is likely to be an UST-style module
		if(sampleHeader.loopStart + sampleHeader.loopLength > sampleHeader.length
			&& sampleHeader.loopStart + sampleHeader.loopLength < sampleHeader.length * 2)
		{
			isUST = true;
			isConfirmed = true;
			continue;
		}

		if(!isUST)
		{
			isConfirmed = true;
		}
	}

	MODFileHeader fileHeader;
	file.Read(fileHeader);

	// Sanity check: No more than 128 positions. We won't check the restart pos since I've encountered so many invalid values (0x6A, 0x72, 0x78, ...) that it doesn't make sense to check for them all.
	if(fileHeader.numOrders > 128)
	{
		return false;
	}

	for(ORDERINDEX ord = 0; ord < CountOf(fileHeader.orderList); ord++)
	{
		// Sanity check: 64 patterns max.
		if(fileHeader.orderList[ord] > 63)
		{
			return false;
		}
	}

	Order.ReadFromArray(fileHeader.orderList);
	PATTERNINDEX numPatterns = GetNumPatterns(file, Order, fileHeader.numOrders, totalSampleLen, m_nChannels, false);

	// Let's see if the file is too small (including some overhead for broken files like sll7.mod)
	if(file.BytesLeft() + 4096 < numPatterns * 64u * 4u + totalSampleLen)
	{
		return false;
	}

	// Now we can be pretty sure that this is a valid Soundtracker file. Set up default song settings.
	m_nType = MOD_TYPE_MOD;
	m_nChannels = 4;
	m_nInstruments = 0;
	m_nDefaultSpeed = 6;
	m_nDefaultTempo = 125;
	m_nMinPeriod = 14 * 4;
	m_nMaxPeriod = 3424 * 4;
	m_SongFlags.reset();
	m_nRestartPos = (fileHeader.restartPos < fileHeader.numOrders ? fileHeader.restartPos : 0);
	StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[0], songname);

	// Setup channel pan positions and volume
	SetupMODPanning();

	FileReader::off_t patOffset = file.GetPosition();

	// Scan patterns to identify Ultimate Soundtracker modules.
	for(size_t i = 0; i < numPatterns * 64u * 4u; i++)
	{
		uint8 data[4];
		file.ReadArray(data);
		const uint8 eff = data[2] & 0x0F, param = data[3];

		if((eff == 1 || eff == 2) && param > 0x1F)
		{
			// If a 1xx effect has a parameter greater than 0x20, it is assumed to be UST.
			isUST = true;
			break;
		} else if(eff == 1 && param < 0x03)
		{
			// MikMod says so! Well, it makes sense, kind of... who would want an arpeggio like this?
			isUST = false;
			break;
		} else if(eff == 3 && param != 0)
		{
			// MikMod says so!
			isUST = false;
			break;
		} else if(eff > 3)
		{
			// Unknown effect
			isUST = false;
			break;
		}
	}

	file.Seek(patOffset);

	// Reading patterns
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		if(Patterns.Insert(pat, 64))
		{
			break;
		}

		uint8 lastEff[4] = { 0, 0, 0, 0 }, lastParam[4] = { 0, 0, 0, 0 };

		for(ROWINDEX row = 0; row < 64; row++)
		{
			ModCommand *rowBase = Patterns[pat].GetpModCommand(row, 0);
			for(CHANNELINDEX chn = 0; chn < 4; chn++)
			{
				ModCommand &m = rowBase[chn];

				uint8 data[4];
				file.ReadArray(data);

				// Read Period
				uint16 period = (((static_cast<uint16>(data[0]) & 0x0F) << 8) | data[1]);
				if(period > 0 && period != 0xFFF)
				{
					m.note = static_cast<ModCommand::NOTE>(GetNoteFromPeriod(period * 4));
				}
				// Read Instrument
				m.instr = (data[2] >> 4) | (data[0] & 0x10);
				// Read Effect
				m.command = data[2] & 0x0F;
				m.param = data[3];

				if(m.command || m.param)
				{
					if(isUST)
					{
						// UST effects
						switch(m.command)
						{
						case 0:
						case 3:
							m.command = CMD_NONE;
							break;
						case 1:
							m.command = CMD_ARPEGGIO;
							break;
						case 2:
							if(m.param & 0x0F)
							{
								m.command = CMD_PORTAMENTOUP;
								m.param &= 0x0F;
							} else if(m.param >> 2)
							{
								m.command = CMD_PORTAMENTODOWN;
								m.param >>= 2;
							}
							break;
						default:
							ConvertModCommand(m);
							break;
						}
					} else
					{
						// M15 effects
						if((m.command == 1 || m.command == 2 || m.command == 3) && m.param == 0)
						{
							// An isolated 100, 200 or 300 effect should be ignored (no "standalone" porta memory in mod files).
							// However, a sequence such as 1xx, 100, 100, 100 is fine.
							if(m.command != lastEff[chn] || lastParam[chn] == 0)
							{
								m.command = m.param = 0;
							} else
							{
								m.param = lastParam[chn];
							}
						}
						lastEff[chn] = m.command;
						lastParam[chn] = m.param;

						ConvertModCommand(m);
					}
				}
			}
		}
	}

	// Reading samples
	for(SAMPLEINDEX smp = 1; smp <= 15; smp++)
	{
		if(isUST)
		{
			// Looped samples in Ultimate Soundtracker seem to ignore all sample data before the actual loop start.
			// This avoids the clicks in the first sample of pretend.mod by Karsten Obarski.
			file.Skip(Samples[smp].nLoopStart);
			Samples[smp].nLength -= Samples[smp].nLoopStart;
			Samples[smp].nLoopEnd -= Samples[smp].nLoopStart;
			Samples[smp].nLoopStart = 0;
		}
		MODSampleHeader::GetSampleFormat().ReadSample(Samples[smp], file);
	}

	return true;
}


#ifndef MODPLUG_NO_FILESAVE

#ifdef MODPLUG_TRACKER
#include "../mptrack/moddoc.h"
#endif	// MODPLUG_TRACKER

extern WORD ProTrackerPeriodTable[6*12];

bool CSoundFile::SaveMod(LPCSTR lpszFileName) const
//-------------------------------------------------
{
	FILE *f;

	if(m_nChannels == 0 || lpszFileName == nullptr) return false;
	if((f = fopen(lpszFileName, "wb")) == nullptr) return false;

	// Write song title
	{
		char name[20];
		StringFixer::WriteString<StringFixer::maybeNullTerminated>(name, m_szNames[0]);
		fwrite(name, 20, 1, f);
	}

	vector<SmpLength> sampleLength(32, 0);
	vector<SAMPLEINDEX> sampleSource(32, 0);

	if(GetNumInstruments())
	{
		for(INSTRUMENTINDEX ins = 1; ins < 32; ins++) if (Instruments[ins])
		{
			// Find some valid sample associated with this instrument.
			for(size_t i = 0; i < CountOf(Instruments[ins]->Keyboard); i++)
			{
				if(Instruments[ins]->Keyboard[i] > 0 && Instruments[ins]->Keyboard[i] <= GetNumSamples())
				{
					sampleSource[ins] = Instruments[ins]->Keyboard[i];
					break;
				}
			}
		}
	} else
	{
		for(SAMPLEINDEX i = 0; i < 32; i++)
		{
			sampleSource[i] = i;
		}
	}

	// Write sample headers
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		MODSampleHeader sampleHeader;
		StringFixer::WriteString<StringFixer::maybeNullTerminated>(sampleHeader.name, m_szNames[sampleSource[smp]]);
		sampleLength[smp] = sampleHeader.ConvertToMOD(sampleSource[smp] <= GetNumSamples() ? GetSample(sampleSource[smp]) : ModSample(MOD_TYPE_MOD));
		sampleHeader.ConvertEndianness();
		fwrite(&sampleHeader, sizeof(sampleHeader), 1, f);
	}

	// Write order list
	MODFileHeader fileHeader;
	MemsetZero(fileHeader);

	PATTERNINDEX writePatterns = 0;
	for(ORDERINDEX ord = 0; ord < Order.GetLength() && fileHeader.numOrders < 128; ord++)
	{
		// Ignore +++ and --- patterns in order list, as well as high patterns (MOD officially only supports up to 128 patterns)
		if(Order[ord] < 128)
		{
			fileHeader.orderList[fileHeader.numOrders++] = static_cast<uint8>(Order[ord]);
			if(writePatterns <= Order[ord])
			{
				writePatterns = Order[ord] + 1;
			}
		}
	}

	if(m_nRestartPos < 128)
	{
		fileHeader.restartPos = static_cast<uint8>(m_nRestartPos);
	}
	fwrite(&fileHeader, sizeof(fileHeader), 1, f);

	// Write magic bytes
	char modMagic[6];
	CHANNELINDEX writeChannels = min(99, GetNumChannels());
	if(writeChannels == 4)
	{
		if(writePatterns < 64)
		{
			memcpy(modMagic, "M.K.", 4);
		} else
		{
			// More than 64 patterns
			memcpy(modMagic, "M!K!", 4);
		}
	} else
	{
		sprintf(modMagic, "%luCHN", writeChannels);
	}
	fwrite(&modMagic, 4, 1, f);

	// Write patterns
	vector<uint8> events;
	for(PATTERNINDEX pat = 0; pat < writePatterns; pat++)
	{
		if(!Patterns.IsValidPat(pat))
		{
			// Invent empty pattern
			events.assign(writeChannels * 64 * 4, 0);
			fwrite(&events[0], events.size(), 1, f);
			continue;
		}

		for(ROWINDEX row = 0; row < 64; row++)
		{
			if(row >= Patterns[pat].GetNumRows())
			{
				// Invent empty row
				events.assign(writeChannels * 4, 0);
				fwrite(&events[0], events.size(), 1, f);
				continue;
			}
			PatternRow rowBase = Patterns[pat].GetRow(row);

			events.resize(writeChannels * 4);
			size_t eventByte = 0;
			for(CHANNELINDEX chn = 0; chn < writeChannels; chn++)
			{
				ModCommand &m = rowBase[chn];
				uint8 command = m.command, param = m.param;
				ModSaveCommand(command, param, false, true);

				if(m.volcmd == VOLCMD_VOLUME && !command && !param)
				{
					// Maybe we can save some volume commands...
					command = 0x0C;
					param = min(m.vol, 64);
				}

				uint16 period = 0;
				// Convert note to period
				if(m.IsNote() && m.note >= 36 + NOTE_MIN && m.note < CountOf(ProTrackerPeriodTable) + 36 + NOTE_MIN)
				{
					period = ProTrackerPeriodTable[m.note - 36 - NOTE_MIN];
				}

				uint8 instr = (m.instr <= 31) ? m.instr : 0;

				events[eventByte++] = ((period >> 8) & 0x0F) | (instr & 0x10);
				events[eventByte++] = period & 0xFF;
				events[eventByte++] = ((instr & 0x0F) << 4) | (command & 0x0F);
				events[eventByte++] = param;
			}
			fwrite(&events[0], eventByte, 1, f);
		}
	}

	//Check for unsaved patterns
#ifdef MODPLUG_TRACKER
	if(GetpModDoc() != nullptr)
	{
		for(PATTERNINDEX pat = writePatterns; pat < Patterns.Size(); pat++)
		{
			if(Patterns.IsValidPat(pat))
			{
				GetpModDoc()->AddToLog(_T("Warning: This track contains at least one pattern after the highest pattern number referred to in the sequence. Such patterns are not saved in the .mod format.\n"));
				break;
			}
		}
	}
#endif

	// Writing samples
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		if(sampleLength[smp] == 0)
		{
			continue;
		}
		const ModSample &sample = Samples[sampleSource[smp]];

		const long sampleStart = ftell(f);
		const size_t writtenBytes = MODSampleHeader::GetSampleFormat().WriteSample(f, sample, sampleLength[smp]);

		static const int8 silence[] = {0, 0};

		if((sample.uFlags & CHN_LOOP) == 0)
		{
			// First two bytes of oneshot samples have to be 0 due to PT's one-shot loop
			const long sampleEnd = ftell(f);
			fseek(f, sampleStart, SEEK_SET);
			fwrite(&silence, min(writtenBytes, 2), 1, f);
			fseek(f, sampleEnd, SEEK_SET);
		}

		// Write padding byte if the sample size is odd.
		if((sample.nLength % 2u) != 0)
		{
			fwrite(&silence[0], 1, 1, f);
		}
	}

	fclose(f);
	return true;
}

#endif // MODPLUG_NO_FILESAVE