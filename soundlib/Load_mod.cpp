/*
 * Load_mod.cpp
 * ------------
 * Purpose: MOD / NST (ProTracker / NoiseTracker) and M15 / STK (Ultimate Soundtracker / Soundtracker) module loader / saver
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "Tables.h"

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
		// Speed is 01...1F in XM, but 01...20 in MOD. 15-sample Soundtracker MODs always ignore the high nibble anyway (this is fixed in ReadM15 already)
		if(m.param <= ((GetType() & MOD_TYPE_MOD) ? 0x20u : 0x1Fu))
			m.command = CMD_SPEED;
		else
			m.command = CMD_TEMPO;
		break;

	// Extension for XM extended effects
	case 'G' - 55:	m.command = CMD_GLOBALVOLUME; break;		//16
	case 'H' - 55:	m.command = CMD_GLOBALVOLSLIDE; break;
	case 'K' - 55:	m.command = CMD_KEYOFF; break;
	case 'L' - 55:	m.command = CMD_SETENVPOSITION; break;
	case 'M' - 55:	m.command = CMD_CHANNELVOLUME; break;		// Wat. Luckily, MPT never allowed this to be entered in patterns...
	case 'N' - 55:	m.command = CMD_CHANNELVOLSLIDE; break;		// Ditto.
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
				param = MIN(param << 1, 0xFF);
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
	case CMD_SPEED:				command = 0x0F; param = MIN(param, (toXM) ? 0x1Fu : 0x20u); break;
	case CMD_TEMPO:				command = 0x0F; param = MAX(param, (toXM) ? 0x20u : 0x21u); break;
	case CMD_GLOBALVOLUME:		command = 'G' - 55; break;
	case CMD_GLOBALVOLSLIDE:	command = 'H' - 55; break;
	case CMD_KEYOFF:			command = 'K' - 55; break;
	case CMD_SETENVPOSITION:	command = 'L' - 55; break;
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


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif


// File Header
struct PACKED MODFileHeader
{
	uint8 numOrders;
	uint8 restartPos;
	uint8 orderList[128];
};

STATIC_ASSERT(sizeof(MODFileHeader) == 130);


// Sample Header
struct PACKED MODSampleHeader
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
		mptSmp.nVolume = 4 * MIN(volume, 64);

		SmpLength lStart = loopStart * 2;
		SmpLength lLength = loopLength * 2;
		// See if loop start is incorrect as words, but correct as bytes (like in Soundtracker modules)
		if(lLength > 2 && (lStart + lLength > mptSmp.nLength)
			&& (lStart / 2 + lLength <= mptSmp.nLength))
		{
			lStart /= 2;
		}

		if(mptSmp.nLength == 2)
		{
			mptSmp.nLength = 0;
		}

		if(mptSmp.nLength)
		{
			mptSmp.nLoopStart = lStart;
			mptSmp.nLoopEnd = lStart + lLength;

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
			if(mptSmp.nLoopEnd > mptSmp.nLoopStart)
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
		LimitMax(writeLength, SmpLength(0x1FFFE));

		length = static_cast<uint16>(writeLength / 2u);

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
		volume = static_cast<uint8>(mptSmp.nVolume / 4u);

		loopStart = 0;
		loopLength = 1;
		if(mptSmp.uFlags[CHN_LOOP] && (mptSmp.nLoopStart + 2u) < writeLength)
		{
			const SmpLength loopEnd = Clamp(mptSmp.nLoopEnd, mptSmp.nLoopStart + 2u, writeLength);
			loopStart = static_cast<uint16>(mptSmp.nLoopStart / 2u);
			loopLength = static_cast<uint16>((loopEnd - mptSmp.nLoopStart) / 2u);
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


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


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
				m.param = MIN(m.param * 2, 0xFF);
			}
		}
	}

	bool fixVBlank, fixPanning;
};


// Check if header magic equals a given string.
static bool IsMagic(const char *magic1, const char *magic2)
//---------------------------------------------------------
{
	return std::memcmp(magic1, magic2, 4) == 0;
}


static void ReadSample(FileReader &file, MODSampleHeader &sampleHeader, ModSample &sample, char (&sampleName)[MAX_SAMPLENAME])
//----------------------------------------------------------------------------------------------------------------------------
{
	file.ReadConvertEndianness(sampleHeader);
	sampleHeader.ConvertToMPT(sample);

	mpt::String::Read<mpt::String::spacePadded>(sampleName, sampleHeader.name);
	// Get rid of weird characters in sample names.
	for(size_t i = 0; i < CountOf(sampleName); i++)
	{
		if(sampleName[i] && sampleName[i] < ' ')
		{
			sampleName[i] = ' ';
		}
	}
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

	const size_t patternStartOffset = file.GetPosition();
	const size_t sizeWithoutPatterns = totalSampleLen + patternStartOffset;

	// Check if this is a Mod's Grave WOW file... Never seen one of those, but apparently they *do* exist.
	// Basically, WOW files seem to use the M.K. extensions, but are actually 8CHN files.
	if(checkForWOW && sizeWithoutPatterns + numPatterns * 8 * 256 == file.GetLength())
	{
		numChannels = 8;
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


void CSoundFile::ReadMODPatternEntry(FileReader &file, ModCommand &m) const
//-------------------------------------------------------------------------
{
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
}


bool CSoundFile::ReadMod(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	char magic[4];
	if(!file.Seek(1080) || !file.ReadArray(magic))
	{
		return false;
	}

	InitializeGlobals();
	m_nChannels = 4;

	// Check MOD Magic
	if(IsMagic(magic, "M.K.")		// ProTracker and compatible
		|| IsMagic(magic, "M!K!")	// ProTracker (64+ patterns)
		|| IsMagic(magic, "M&K!")	// NoiseTracker
		|| IsMagic(magic, "N.T.")	// NoiseTracker
		|| IsMagic(magic, "FEST"))	// jobbig.mod by Mahoney
	{
		m_nChannels = 4;
	} else if(IsMagic(magic, "CD81"))	// Falcon
	{
		m_nChannels = 8;
		madeWithTracker = "Falcon";
	} else if(IsMagic(magic, "OKTA")	// Oktalyzer
		|| IsMagic(magic, "OCTA"))		// Oktalyzer
	{
		m_nChannels = 8;
		madeWithTracker = "Oktalyzer";
	} else if((!memcmp(magic, "FLT", 3) || !memcmp(magic, "EXO", 3)) && magic[3] >= '4' && magic[3] <= '9')
	{
		// FLTx / EXOx - Startrekker by Exolon / Fairlight
		m_nChannels = magic[3] - '0';
		madeWithTracker = "Startrekker";
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
		madeWithTracker = "TakeTracker";
	} else
	{
		return false;
	}
	if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	LimitMax(m_nChannels, MAX_BASECHANNELS);

	// Startrekker 8 channel mod (needs special treatment, see below)
	const bool isFLT8 = IsMagic(magic, "FLT8");
	// Only apply VBlank tests to M.K. (ProTracker) modules.
	const bool isMdKd = IsMagic(magic, "M.K.");

	// Reading song title
	file.Seek(0);
	file.ReadString<mpt::String::spacePadded>(songName, 20);

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

	// Get number of patterns (including some order list sanity checks)
	PATTERNINDEX numPatterns = GetNumPatterns(file, Order, realOrders, totalSampleLen, m_nChannels, isMdKd);
	if(isMdKd && GetNumChannels() == 8)
	{
		// M.K. with 8 channels = Grave Composer
		madeWithTracker = "Mod's Grave";
	}

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

	// (Ultimate) Soundtracker didn't have a restart position, but instead stored a default tempo in this value.
	// The default value for this is 0x78 (120 BPM). This is probably the reason why some M.K. modules
	// have this weird restart position. I think I've read somewhere that NoiseTracker actually writes 0x78 there.
	// Files that have restart pos == 0x78: action's batman by DJ Uno (M.K.), 3ddance.mod (M15, so handled by ReadM15),
	// VALLEY.MOD (M.K.), WormsTDC.MOD (M.K.), ZWARTZ.MOD (M.K.)
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
	// Prevent clipping based on number of channels... If all channels are playing at full volume, "256 / #channels"
	// is the maximum possible sample pre-amp without getting distortion (Compatible mix levels given).
	// The more channels we have, the less likely it is that all of them are used at the same time, though, so clip at 32...
	m_nSamplePreAmp = std::max(32, 256 / m_nChannels);
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
					if(!(loadFlags & loadPatternData) || Patterns.Insert(pat / 2, 64))
					{
						file.Skip(readChannels * 64 * 4);
						continue;
					}
				}
				actualPattern /= 2;
			} else
			{
				if(!(loadFlags & loadPatternData) || Patterns.Insert(pat, 64))
				{
					file.Skip(readChannels * 64 * 4);
					continue;
				}
			}

			// For detecting PT1x mode
			std::vector<ModCommand::INSTR> lastInstrument(GetNumChannels(), 0);
			std::vector<int> instrWithoutNoteCount(GetNumChannels(), 0);

			for(ROWINDEX row = 0; row < 64; row++)
			{
				// FLT8: either write to channel 1 to 4 (even patterns) or 5 to 8 (odd patterns).
				PatternRow rowBase = Patterns[actualPattern].GetpModCommand(row, ((pat % 2u) == 0 || !isFLT8) ? 0 : 4);

				for(CHANNELINDEX chn = 0; chn < readChannels; chn++)
				{
					ModCommand &m = rowBase[chn];
					ReadMODPatternEntry(file, m);

					if(m.command || m.param)
					{
						ConvertModCommand(m);
					}

					// Perform some checks for our heuristics...
					if(m.command == CMD_TEMPO && m.param < 100)
						hasTempoCommands = true;
					if(m.command == CMD_PANNING8 && m.param < 0x80)
						leftPanning = true;
					if(m.command == CMD_PANNING8 && m.param > 0x8F && m.param != 0xA4)
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
	if(loadFlags & loadSampleData)
	{
		for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
		{
			SampleIO(
				SampleIO::_8bit,
				SampleIO::mono,
				SampleIO::bigEndian,
				file.ReadMagic("ADPCM") ? SampleIO::ADPCM : SampleIO::signedPCM)
				.ReadSample(Samples[smp], file);
		}
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
	const bool fixVBlank = isMdKd && hasTempoCommands && GetSongTime() >= 600.0;
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


// We'll have to do some heuristic checks to find out whether this is an old Ultimate Soundtracker module
// or if it was made with the newer Soundtracker versions.
// Thanks for Fraggie for this information! (http://www.un4seen.com/forum/?topic=14471.msg100829#msg100829)
enum STVersions
{
	UST1_00,				// Ultimate Soundtracker 1.0-1.21 (K. Obarski)
	UST1_80,				// Ultimate Soundtracker 1.8-2.0 (K. Obarski)
	ST2_00_Exterminator,	// SoundTracker 2.0 (The Exterminator), D.O.C. Sountracker II (Unknown/D.O.C.)
	ST_III,					// Defjam Soundtracker III (Il Scuro/Defjam), Alpha Flight SoundTracker IV (Alpha Flight), D.O.C. SoundTracker IV (Unknown/D.O.C.), D.O.C. SoundTracker VI (Unknown/D.O.C.)
	ST_IX,					// D.O.C. SoundTracker IX (Unknown/D.O.C.)
	MST1_00,				// Master Soundtracker 1.0 (Tip/The New Masters)
	ST2_00,					// SoundTracker 2.0, 2.1, 2.2 (Unknown/D.O.C.)
	ST2_00_with_Bxx,		// Bxx effect found, so definitely SoundTracker 2.0, 2.1, 2.2 (Unknown/D.O.C.)
};


bool CSoundFile::ReadM15(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();

	char songname[20];
	file.ReadArray(songname);
	if(!IsValidName(songname, sizeof(songname), ' ')
		|| !file.CanRead(sizeof(MODSampleHeader) * 15 + sizeof(MODFileHeader)))
	{
		return false;
	}

	InitializeGlobals();
	m_nChannels = 4;

	STVersions minVersion = UST1_00;

	bool hasDiskNames = true;
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
		ASSERT(sampleHeader.finetune == 0);

		totalSampleLen += Samples[smp].nLength;

		if(m_szNames[smp][0] && ((memcmp(m_szNames[smp], "st-", 3) && memcmp(m_szNames[smp], "ST-", 3)) || m_szNames[smp][0] < '0' || m_szNames[smp][0] > '9'))
		{
			// Ultimate Soundtracker 1.8 and D.O.C. SoundTracker IX always have sample names containing disk names.
			hasDiskNames = false;
		}

		// Loop start is always in bytes, not words, so don't trust the auto-fix magic in the sample header conversion (fixes loop of "st-01:asia" in mod.drag 10)
		if(sampleHeader.loopLength > 1)
		{
			Samples[smp].nLoopStart = sampleHeader.loopStart;
			Samples[smp].nLoopEnd = sampleHeader.loopStart + sampleHeader.loopLength * 2;
			Samples[smp].SanitizeLoops();
		}

		// UST only handles samples up to 9999 bytes. Master Soundtracker 1.0 and SoundTracker 2.0 introduce 32KB samples.
		if(sampleHeader.length > 4999 || sampleHeader.loopStart > 9999)
		{
			minVersion = std::max(minVersion, MST1_00);
		}
	}

	MODFileHeader fileHeader;
	file.Read(fileHeader);

	// Sanity check: No more than 128 positions. ST's GUI limits tempo to [1, 220].
	if(fileHeader.numOrders > 128 || fileHeader.restartPos == 0 || fileHeader.restartPos > 220)
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
	if(file.BytesLeft() + 32767 < numPatterns * 64u * 4u + totalSampleLen)
	{
		return false;
	}

	if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	// Now we can be pretty sure that this is a valid Soundtracker file. Set up default song settings.
	m_nType = MOD_TYPE_MOD;
	// Sample 7 in echoing.mod won't "loop" correctly if we don't convert the VBlank tempo.
	m_nDefaultTempo = fileHeader.restartPos * 25 / 24;
	if(fileHeader.restartPos != 0x78)
	{
		// Convert to CIA timing
		//m_nDefaultTempo = static_cast<TEMPO>(((709378.92f / 50.0f) * 125.0f) / ((240.0f - static_cast<float>(fileHeader.restartPos)) * 122.0f));
		m_nDefaultTempo = (709379 / ((240 - fileHeader.restartPos) * 122)) * 125 / 50;
		if(minVersion > UST1_80)
		{
			// D.O.C. SoundTracker IX re-introduced the variable tempo after some other versions dropped it.
			minVersion = std::max(minVersion, hasDiskNames ? ST_IX : MST1_00);
		} else
		{
			// Ultimate Soundtracker 1.8 adds variable tempo
			minVersion = std::max(minVersion, hasDiskNames ? UST1_80 : ST2_00_Exterminator);
		}
	}
	m_nMinPeriod = 14 * 4;
	m_nMaxPeriod = 3424 * 4;
	m_nSamplePreAmp = 64;
	m_SongFlags = SONG_PT1XMODE;
	mpt::String::Read<mpt::String::spacePadded>(songName, songname);

	// Setup channel pan positions and volume
	SetupMODPanning();

	FileReader::off_t patOffset = file.GetPosition();

	// Scan patterns to identify Ultimate Soundtracker modules.
	uint8 emptyCmds = 0;
	for(size_t i = 0; i < numPatterns * 64u * 4u; i++)
	{
		uint8 data[4];
		file.ReadArray(data);
		const uint8 eff = data[2] & 0x0F, param = data[3];
		if(emptyCmds != 0 && !memcmp(data, "\0\0\0\0", 4))
		{
			emptyCmds++;
			if(emptyCmds > 32)
			{
				// Since there is a lot of empty space after the last Dxx command,
				// we assume it's supposed to be a pattern break effect.
				minVersion = ST2_00_with_Bxx;
			}
		} else
		{
			emptyCmds = 0;
		}

		switch(eff)
		{
		case 1:
		case 2:
			if(param > 0x1F && minVersion == UST1_80)
			{
				// If a 1xx / 2xx effect has a parameter greater than 0x20, it is assumed to be UST.
				minVersion = hasDiskNames ? UST1_80 : UST1_00;
			} else if(eff == 1 && param < 0x03)
			{
				// This doesn't look like an arpeggio.
				minVersion = std::max(minVersion, ST2_00_Exterminator);
			}
			break;
		case 0x0B:
			minVersion = ST2_00_with_Bxx;
			break;
		case 0x0C:
		case 0x0D:
		case 0x0E:
			minVersion = std::max(minVersion, ST2_00_Exterminator);
			if(eff == 0x0D && param == 0)
			{
				// Assume this is a pattern break command.
				minVersion = std::max(minVersion, ST2_00);
			}
			emptyCmds = 1;
			break;
		case 0x0F:
			minVersion = std::max(minVersion, ST_III);
			break;
		}
	}

	file.Seek(patOffset);

	// Reading patterns
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		if(!(loadFlags & loadPatternData) || Patterns.Insert(pat, 64))
		{
			file.Skip(64 * 4 * 4);
			continue;
		}

		for(ROWINDEX row = 0; row < 64; row++)
		{
			PatternRow rowBase = Patterns[pat].GetpModCommand(row, 0);
			for(CHANNELINDEX chn = 0; chn < 4; chn++)
			{
				ModCommand &m = rowBase[chn];
				ReadMODPatternEntry(file, m);

				if(m.command || m.param)
				{
					if(m.command == 0x0D)
					{
						if((m.param != 0 && minVersion != ST2_00_with_Bxx) || minVersion < ST_IX)
						{
							// Dxy is volume slide in some Soundtracker versions, D00 is a pattern break in the latest versions.
							m.command = 0x0A;
						} else if(m.param == 0 && row == 0 && minVersion != ST2_00_with_Bxx)
						{
							// Fix a possible tracking mistake in Blood Money title - who wants to do a pattern break on the first row anyway?
							m.command = m.param = 0;
						} else
						{
							m.param = 0;
						}
					} else if(m.command == 0x0C)
					{
						// Volume is sent as-is to the chip, which ignores the highest bit.
						m.param &= 0x7F;
					} else if(m.command == 0x0E && (m.param > 0x01 || minVersion < ST_IX))
					{
						// Import auto-slides as normal slides and ignore their extended behaviour.
						m.command = 0x0A;
					} else if(m.command == 0x0F)
					{
						// Only the low nibble is evaluated in Soundtracker.
						m.param &= 0x0F;
					}

					if(minVersion <= UST1_80)
					{
						// UST effects
						switch(m.command)
						{
						case 0:
							// jackdance.mod by Karsten Obarski has 0xy arpeggios...
							if(m.param < 0x03)
							{
								break;
							}
						case 1:
							m.command = CMD_ARPEGGIO;
							break;
						case 2:
							if(m.param & 0x0F)
							{
								m.command = CMD_PORTAMENTOUP;
								m.param &= 0x0F;
							} else if(m.param >> 4)
							{
								m.command = CMD_PORTAMENTODOWN;
								m.param >>= 4;
							}
							break;
						default:
							m.command = CMD_NONE;
							break;
						}
					} else
					{
						ConvertModCommand(m);
					}
				}
			}
		}
	}

	switch(minVersion)
	{
	case UST1_00:
		madeWithTracker = "Ultimate Soundtracker 1.0-1.21";
		break;
	case UST1_80:
		madeWithTracker = "Ultimate Soundtracker 1.8-2.0";
		break;
	case ST2_00_Exterminator:
		madeWithTracker = "SoundTracker 2.0 / D.O.C. Sountracker II";
		break;
	case ST_III:
		madeWithTracker = "Defjam Soundtracker III / Alpha Flight SoundTracker IV / D.O.C. SoundTracker IV / VI";
		break;
	case ST_IX:
		madeWithTracker = "D.O.C. SoundTracker IX";
		break;
	case MST1_00:
		madeWithTracker = "Master Soundtracker 1.0";
		break;
	case ST2_00:
	case ST2_00_with_Bxx:
		madeWithTracker = "SoundTracker 2.0 / 2.1 / 2.2";
		break;
	}

	// Reading samples
	if(loadFlags & loadSampleData)
	{
		for(SAMPLEINDEX smp = 1; smp <= 15; smp++)
		{
			// Looped samples in (Ultimate) Soundtracker seem to ignore all sample data before the actual loop start.
			// This avoids the clicks in the first sample of pretend.mod by Karsten Obarski.
			file.Skip(Samples[smp].nLoopStart);
			Samples[smp].nLength -= Samples[smp].nLoopStart;
			Samples[smp].nLoopEnd -= Samples[smp].nLoopStart;
			Samples[smp].nLoopStart = 0;
			MODSampleHeader::GetSampleFormat().ReadSample(Samples[smp], file);
		}
	}

	return true;
}


#ifndef MODPLUG_NO_FILESAVE

bool CSoundFile::SaveMod(const mpt::PathString &filename) const
//-------------------------------------------------------------
{
	FILE *f;

	if(m_nChannels == 0 || filename.empty()) return false;
	if((f = mpt_fopen(filename, "wb")) == nullptr) return false;

	// Write song title
	{
		char name[20];
		mpt::String::Write<mpt::String::maybeNullTerminated>(name, songName);
		fwrite(name, 20, 1, f);
	}

	std::vector<SmpLength> sampleLength(32, 0);
	std::vector<SAMPLEINDEX> sampleSource(32, 0);

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
		mpt::String::Write<mpt::String::maybeNullTerminated>(sampleHeader.name, m_szNames[sampleSource[smp]]);
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
	CHANNELINDEX writeChannels = std::min(CHANNELINDEX(99), GetNumChannels());
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
		sprintf(modMagic, "%uCHN", writeChannels);
	}
	fwrite(&modMagic, 4, 1, f);

	// Write patterns
	std::vector<uint8> events;
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
					param = MIN(m.vol, 64);
				}

				uint16 period = 0;
				// Convert note to period
				if(m.note >= 36 + NOTE_MIN && m.note < CountOf(ProTrackerPeriodTable) + 36 + NOTE_MIN)
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
	for(PATTERNINDEX pat = writePatterns; pat < Patterns.Size(); pat++)
	{
		if(Patterns.IsValidPat(pat))
		{
			AddToLog("Warning: This track contains at least one pattern after the highest pattern number referred to in the sequence. Such patterns are not saved in the .mod format.");
			break;
		}
	}

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

		const int8 silence[] = { 0, 0 };

		// Write padding byte if the sample size is odd.
		if((writtenBytes % 2u) != 0)
		{
			fwrite(silence, 1, 1, f);
		}

		if(!sample.uFlags[CHN_LOOP] && writtenBytes >= 2)
		{
			// First two bytes of oneshot samples have to be 0 due to PT's one-shot loop
			const long sampleEnd = ftell(f);
			fseek(f, sampleStart, SEEK_SET);
			fwrite(&silence, 2, 1, f);
			fseek(f, sampleEnd, SEEK_SET);
		}
	}

	fclose(f);
	return true;
}

#endif // MODPLUG_NO_FILESAVE
