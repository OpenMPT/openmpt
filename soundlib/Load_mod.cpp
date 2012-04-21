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
	case 0x0F:	m.command = (m.param <= ((GetType() & (MOD_TYPE_MOD)) ? 0x20u : 0x1Fu)) ? CMD_SPEED : CMD_TEMPO;
		if ((m.param == 0xFF) && (GetNumSamples() == 15) && (GetType() & MOD_TYPE_MOD)) m.command = CMD_NONE; break; //<rewbs> what the hell is this?! :) //<jojo> it's the "stop tune" command! :-P

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
	case CMD_XFINEPORTAUPDOWN:
		if(compatibilityExport && (param & 0xF0) > 2)	// X1x and X2x are legit, everything above are MPT extensions, which don't belong here.
			command = param = 0;
		else
			command = 'X' - 55;
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
	char  magic[4];

	// Check if header magic equals a given string.
	bool IsMagic(const char *otherMagic) const
	{
		return (*reinterpret_cast<const uint32 *>(magic) == *reinterpret_cast<const uint32 *>(otherMagic));
	}
};

STATIC_ASSERT(sizeof(MODFileHeader) == 134);


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

	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.uFlags = 0;
		mptSmp.nLength = length * 2;
		mptSmp.RelativeTone = 0;
		mptSmp.nFineTune = MOD2XMFineTune(finetune & 0x0F);
		mptSmp.nVolume = 4 * min(volume, 64);
		mptSmp.nGlobalVol = 64;
		mptSmp.nPan = 128;

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

		if (mptSmp.nLength == 2)
		{
			mptSmp.nLength = 0;
		}

		if (mptSmp.nLength)
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
				mptSmp.uFlags |= CHN_LOOP;
			}
		}
	}

	// Convert OpenMPT's internal sample header to a MOD sample header.
	SmpLength ConvertToMOD(const ModSample &mptSmp)
	{
		SmpLength writeLength = mptSmp.nLength;
		// If the sample size is odd, we have to add a padding byte, as all sample sizes in MODs are even.
		if((writeLength % 2) != 0)
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
		if(mptSmp.uFlags & CHN_LOOP)
		{
			loopStart = static_cast<uint16>(mptSmp.nLoopStart / 2);
			loopLength = static_cast<uint16>(max(1, (mptSmp.nLoopEnd - mptSmp.nLoopStart) / 2));
		}

		return writeLength;
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
		// Fix VBlank MODs
		if(m.command == CMD_TEMPO && this->fixVBlank)
		{
			m.command = CMD_SPEED;
		}
		// Fix MODs with 7-bit + surround panning
		if(m.command == CMD_PANNING8 && this->fixPanning)
		{
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


bool CSoundFile::ReadMod(FileReader &file)
//----------------------------------------
{
	file.Rewind();

	MODFileHeader fileHeader;
	if(!file.Seek(20 + sizeof(MODSampleHeader) * 31) || !file.Read(fileHeader))
	{
		return false;
	}

	m_nSamples = 31;
	m_nChannels = 4;

	// Check MOD Magic
	if(fileHeader.IsMagic("M.K.")		// ProTracker and compatible
		|| fileHeader.IsMagic("M!K!")	// ProTracker (64+ patterns)
		|| fileHeader.IsMagic("M&K!")	// NoiseTracker
		|| fileHeader.IsMagic("N.T.")	// NoiseTracker
		|| fileHeader.IsMagic("FEST"))	// jobbig.mod by Mahoney
	{
		m_nChannels = 4;
	} else if(fileHeader.IsMagic("CD81")	// Falcon
		|| fileHeader.IsMagic("OKTA")		// Oktalyzer
		|| fileHeader.IsMagic("OCTA"))		// Oktalyzer
	{
		m_nChannels = 8;
	} else if((!memcmp(fileHeader.magic, "FLT", 3) || !memcmp(fileHeader.magic, "EXO", 3)) && fileHeader.magic[3] >= '4' && fileHeader.magic[3] <= '9')
	{
		// FLTx / EXOx - Startrekker by Exolon / Fairlight
		m_nChannels = fileHeader.magic[3] - '0';
	} else if(fileHeader.magic[0] >= '1' && fileHeader.magic[0] <= '9' && !memcmp(fileHeader.magic + 1, "CHN", 3))
	{
		// xCHN - Many trackers
		m_nChannels = fileHeader.magic[0] - '0';
	} else if(fileHeader.magic[0] >= '1' && fileHeader.magic[0] <= '9' && fileHeader.magic[1]>='0' && fileHeader.magic[1] <= '9'
		&& (!memcmp(fileHeader.magic + 2, "CH", 2) || !memcmp(fileHeader.magic + 2, "CN", 2)))
	{
		// xxCN / xxCH - Many trackers
		m_nChannels = (fileHeader.magic[0] - '0') * 10 + fileHeader.magic[1] - '0';
	} else if(!memcmp(fileHeader.magic, "TDZ", 3) && fileHeader.magic[3] >= '4' && fileHeader.magic[3] <= '9')
	{
		// TDZx - TakeTracker
		m_nChannels = fileHeader.magic[3] - '0';
	} else
	{
		// Ultimate SoundTracker MODs - no magic bytes
		m_nSamples = 15;
	}

	LimitMax(m_nChannels, MAX_BASECHANNELS);

	// Startrekker 8 channel mod (needs special treatment, see below)
	bool isFLT8 = fileHeader.IsMagic("FLT8");
	// Only apply VBlank tests to M.K. (ProTracker) modules.
	const bool isMdKd = fileHeader.IsMagic("M.K.");

	// Load Samples
	int m15Errors = 0;
	size_t totalSampleLen = 0;
	
	file.Seek(20);
	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		MODSampleHeader sampleHeader;
		if(!file.ReadConvertEndianness(sampleHeader))
		{
			return false;
		}
		sampleHeader.ConvertToMPT(Samples[smp]);

		StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[smp], sampleHeader.name);

		totalSampleLen += Samples[smp].nLength;

		// M15 Sanity checks
		if(sampleHeader.volume > 256)
		{
			m15Errors++;
		}
		if(sampleHeader.length > 32768)
		{
			m15Errors++;
		}
		if(sampleHeader.finetune != 0)
		{
			m15Errors++;
		}
	}

	// Sanity checks...
	if(GetNumSamples() == 15)
	{
		if(totalSampleLen > file.GetLength() * 4 || fileHeader.numOrders > 128 || fileHeader.restartPos > 128)
		{
			return false;
		}
	}

	// Re-read file header, as it is found at another position in M15 files.
	file.Read(fileHeader);

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

	PATTERNINDEX numPatterns = 0;			// Total number of patterns in file (determined by going through the whole order list) with pattern number < 128
	PATTERNINDEX officialPatterns = 0;		// Number of patterns only found in the "official" part of the order list (i.e. order positions < claimed order length)
	PATTERNINDEX numPatternsIllegal = 0;	// Total number of patterns in file, also counting in "invalid" pattern indexes >= 128

	for(ORDERINDEX ord = 0; ord < 128; ord++)
	{
		PATTERNINDEX pat = Order[ord];
		if(pat < 128 && numPatterns <= pat)
		{
			numPatterns = pat + 1;
			if(ord < realOrders)
			{
				officialPatterns = numPatterns;
			}
		}
		if(pat >= numPatternsIllegal)
		{
			numPatternsIllegal = pat + 1;
		}

		// From mikmod: if the file says FLT8, but the orderlist has odd numbers, it's probably really an FLT4
		if(isFLT8 && (Order[ord] % 2) != 0)
		{
			m_nChannels = 4;
			isFLT8 = false;
		}

		// chances are very high that we're dealing with a non-MOD file here.
		if(GetNumSamples() == 15 && pat >= 64)
		{
			return false;
		}
	}

	if(!numPatterns)
	{
		return false;
	}

	if(isFLT8)
	{
		// FLT8 has only even order items, so divide by two.
		for(ORDERINDEX ord = 0; ord < Order.GetLength(); ord++)
		{
			Order[ord] /= 2;
		}
	}

	// Fill order tail with stop patterns
	for(ORDERINDEX ord = realOrders; ord < 128; ord++)
	{
		Order[ord] = Order.GetInvalidPatIndex();
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

	const size_t patternStartOffset = 20 + GetNumSamples() * sizeof(MODSampleHeader) + sizeof(MODFileHeader) - (GetNumSamples() == 15 ? 4 : 0);
	const size_t sizeWithoutPatterns = totalSampleLen + patternStartOffset;

	if(isMdKd)
	{
		// Check if this is a Mod's Grave WOW file... Never seen one of those, but apparently they *do* exist.
		// Basically, WOW files seem to use the M.K. extensions, but are actually 8CHN files.
		if(sizeWithoutPatterns + numPatterns * 8 * 256 == file.GetLength())
		{
			m_nChannels = 8;
		}
	}

	// Now we have to check if the "hidden" patterns in the order list are actually real, i.e. if they are saved in the file.
	if(numPatterns != officialPatterns && sizeWithoutPatterns + numPatterns * m_nChannels * 256 != file.GetLength())
	{
		// Ok, size does not match the number of patterns including "hidden" patterns. Does it match the number of "official" patterns?
		if(sizeWithoutPatterns + officialPatterns * m_nChannels * 256 == file.GetLength())
		{
			// It does! Forget about that crap.
			numPatterns = officialPatterns;
		} else
		{
			// Well... Neither fits... So this must be a broken file.
			m15Errors += 8;
		}
	} else if(numPatternsIllegal > numPatterns && sizeWithoutPatterns + numPatternsIllegal * m_nChannels * 256 == file.GetLength())
	{
		// Even those illegal pattern indexes (> 128) appear to be valid... What a weird file!
		numPatterns = numPatternsIllegal;
	}

	// Some final M15 sanity checks...
	if(sizeWithoutPatterns < 0x600 || sizeWithoutPatterns > file.GetLength())
	{
		m15Errors += 8;
	}

	if(GetNumSamples() == 15 && m15Errors >= 16)
	{
		return false;
	}

	// Now we can be pretty sure that this is a valid MOD file. Set up default song settings.
	m_nType = MOD_TYPE_MOD;
	m_nDefaultSpeed = 6;
	m_nDefaultTempo = 125;
	m_nMinPeriod = 14 * 4;
	m_nMaxPeriod = 3424 * 4;

	file.Seek(0);
	file.ReadString<StringFixer::spacePadded>(m_szNames[0], 20);

	// Setup channel pan positions and volume
	SetupMODPanning();

	const CHANNELINDEX readChannels = (isFLT8 ? 4 : m_nChannels); // 4 channels per pattern in FLT8 format.
	if(isFLT8) numPatterns++; // as one logical pattern consists of two real patterns in FLT8 format, the highest pattern number has to be increased by one.
	bool hasTempoCommands = false;	// for detecting VBlank MODs
	bool leftPanning = false, extendedPanning = false;	// for detecting 800-880 panning

	file.Seek(patternStartOffset);

	// Reading patterns
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		if(pat < MAX_PATTERNS)
		{
			PATTERNINDEX actualPattern = pat;

			if(isFLT8)
			{
				if((pat % 2) == 0)
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

			size_t instrWithoutNoteCount = 0;	// For detecting PT1x mode
			vector<ModCommand::INSTR> lastInstrument(m_nChannels, 0);

			for(ROWINDEX row = 0; row < 64; row++)
			{
				// FLT8: either write to channel 1 to 4 (even patterns) or 5 to 8 (odd patterns).
				ModCommand *rowBase = Patterns[actualPattern].GetpModCommand(row, ((pat % 2) == 0 || !isFLT8) ? 0: 4);

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
					if (m.command == CMD_TEMPO && m.param < 100)
						hasTempoCommands = true;
					if (m.command == CMD_PANNING8 && m.param < 0x80)
						leftPanning = true;
					if (m.command == CMD_PANNING8 && m.param > 0x80 && m.param != 0xA4)
						extendedPanning = true;
					if (m.note == NOTE_NONE && m.instr > 0 && !isFLT8)
					{
						if(lastInstrument[chn] > 0 && lastInstrument[chn] != m.instr)
						{
							instrWithoutNoteCount++;
						}
					}
					if (m.instr != 0)
					{
						lastInstrument[chn] = m.instr;
					}
				}
			}
			// Arbitrary threshold for going into PT1x mode: 16 "sample swaps" in one pattern.
			if(instrWithoutNoteCount > 16)
			{
				m_dwSongFlags |= SONG_PT1XMODE;
			}
		}
	}

	// Reading samples
	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		ReadSample(&Samples[smp], RS_PCM8S, file);
	}

	// Fix VBlank MODs. Arbitrary threshold: 10 minutes.
	// Basically, this just converts all tempo commands into speed commands
	// for MODs which are supposed to have VBlank timing (instead of CIA timing).
	// There is no perfect way to do this, since both MOD types look the same,
	// but the most reliable way is to simply check for extremely long songs
	// (as this would indicate that f.e. a F30 command was really meant to set
	// the ticks per row to 48, and not the tempo to 48 BPM).
	// In the pattern loader above, a second condition is used: Only tempo commands
	// below 100 BPM are taken into account. Furthermore, only M.K. (ProTracker)
	// modules are checked.
	// The same check is also applied to original Ultimate Soundtracker 15 sample mods.
	const bool fixVBlank = ((isMdKd && hasTempoCommands && GetSongTime() >= 10 * 60) || GetNumSamples()== 15);
	const bool fix7BitPanning = leftPanning && !extendedPanning;
	if(fixVBlank || fix7BitPanning)
	{
		Patterns.ForEachModCommand(FixMODPatterns(fixVBlank, fix7BitPanning));
	}

	return true;
}


#ifndef MODPLUG_NO_FILESAVE

#ifdef MODPLUG_TRACKER
#include "../mptrack/moddoc.h"
#endif	// MODPLUG_TRACKER

extern WORD ProTrackerPeriodTable[6*12];

bool CSoundFile::SaveMod(LPCSTR lpszFileName)
//-------------------------------------------
{
	FILE *f;

	if ((!m_nChannels) || (!lpszFileName)) return false;
	if ((f = fopen(lpszFileName, "wb")) == NULL) return false;

	// Write song title
	{
		char name[20];
		StringFixer::WriteString<StringFixer::maybeNullTerminated>(name, m_szNames[0]);
		fwrite(name, 20, 1, f);
	}

	vector<SmpLength> sampleLength(32, 0);
	vector<SAMPLEINDEX> sampleSource(32, 0);

	if(m_nInstruments)
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
		sampleLength[smp] = sampleHeader.ConvertToMOD(Samples[sampleSource[smp]]);
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

	// Write magic bytes
	CHANNELINDEX writeChannels = min(99, GetNumChannels());
	if(writeChannels == 4)
	{
		if(writePatterns < 64)
		{
			memcpy(fileHeader.magic, "M.K.", 4);
		} else
		{
			// More than 64 patterns
			memcpy(fileHeader.magic, "M!K!", 4);
		}
	} else
	{
		char magic[6];
		sprintf(magic, "%luCHN", writeChannels);
		memcpy(fileHeader.magic, magic, 4);
	}

	fwrite(&fileHeader, sizeof(fileHeader), 1, f);

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

	// Writing instruments
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++) if (sampleLength[smp])
	{
		const ModSample &sample = Samples[sampleSource[smp]];

		const long sampleStart = ftell(f);
		const UINT writtenBytes = WriteSample(f, &sample, RS_PCM8S, sampleLength[smp]);

		if((sample.uFlags & CHN_LOOP) == 0)
		{
			// First two bytes of oneshot samples have to be 0 due to PT's one-shot loop
			const long sampleEnd = ftell(f);
			fseek(f, sampleStart, SEEK_SET);
			int8 silence[] = {0, 0};
			fwrite(&silence, min(writtenBytes, 2), 1, f);
			fseek(f, sampleEnd, SEEK_SET);
		}

		// Write padding byte if the sample size is odd.
		if((sample.nLength % 2) != 0)
		{
			int8 padding = 0;
			fwrite(&padding, 1, 1, f);
		}
	}
	fclose(f);
	return true;
}

#endif // MODPLUG_NO_FILESAVE
