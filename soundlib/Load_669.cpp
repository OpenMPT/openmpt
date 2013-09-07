/*
 * Load_669.cpp
 * ------------
 * Purpose: 669 Composer / UNIS 669 module loader
 * Notes  : <opinion humble="false">This is better than Schism's 669 loader</opinion> :)
 *          (some of this code is "heavily inspired" by Storlek's code from Schism Tracker, and improvements have been made where necessary.)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED _669FileHeader
{
	enum MagicBytes
	{
		magic669	= 0x6669,	// 'if'
		magic669Ext	= 0x4E4A	// 'JN'
	};

	uint16 sig;					// 'if' or 'JN'
	char   songMessage[108];	// Song Message
	uint8  samples;				// number of samples (1-64)
	uint8  patterns;			// number of patterns (1-128)
	uint8  restartPos;
	uint8  orders[128];
	uint8  tempoList[128];
	uint8  breaks[128];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(sig);
	}
};

STATIC_ASSERT(sizeof(_669FileHeader) == 497);


struct PACKED _669Sample
{
	char   filename[13];
	uint32 length;
	uint32 loopStart;
	uint32 loopEnd;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
	}

	// Convert a 669 sample header to OpenMPT's internal sample header.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();

		mptSmp.nLength = length;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;

		if(mptSmp.nLoopEnd > mptSmp.nLength && mptSmp.nLoopStart == 0)
		{
			mptSmp.nLoopEnd = 0;
		}
		if(mptSmp.nLoopEnd != 0)
		{
			mptSmp.uFlags = CHN_LOOP;
			mptSmp.SanitizeLoops();
		}
	}
};

STATIC_ASSERT(sizeof(_669Sample) == 25);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::Read669(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	_669FileHeader fileHeader;

	file.Rewind();
	if(!file.ReadConvertEndianness(fileHeader)
		|| (fileHeader.sig != _669FileHeader::magic669 && fileHeader.sig != _669FileHeader::magic669Ext)
		|| fileHeader.samples > 64
		|| fileHeader.restartPos >= 128
		|| fileHeader.patterns > 128)
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	//bool has669Ext = fileHeader.sig == _669FileHeader::magic669Ext;

	InitializeGlobals();
	m_nType = MOD_TYPE_669;
	m_SongFlags = SONG_LINEARSLIDES;
	m_nMinPeriod = 28 << 2;
	m_nMaxPeriod = 1712 << 3;
	m_nDefaultTempo = 78;
	m_nDefaultSpeed = 4;
	m_nChannels = 8;

	if(fileHeader.sig == _669FileHeader::magic669)
		madeWithTracker = "Composer 669";
	else
		madeWithTracker = "UNIS 669";

	m_nSamples = fileHeader.samples;
	for(SAMPLEINDEX smp = 1; smp <= m_nSamples; smp++)
	{
		_669Sample sampleHeader;
		if(!file.ReadConvertEndianness(sampleHeader))
		{
			return false;
		}
		sampleHeader.ConvertToMPT(Samples[smp]);
		mpt::String::Read<mpt::String::maybeNullTerminated>(m_szNames[smp], sampleHeader.filename);
	}

	// Copy first song message line into song title
	mpt::String::Read<mpt::String::spacePadded>(songName, fileHeader.songMessage, 36);
	// Song Message
	songMessage.ReadFixedLineLength(fileHeader.songMessage, 108, 36, 0);

	// Reading Orders
	Order.ReadFromArray(fileHeader.orders);
	m_nRestartPos = fileHeader.restartPos;
	if(Order[m_nRestartPos] >= fileHeader.patterns) m_nRestartPos = 0;

	// Set up panning
	for(CHANNELINDEX chn = 0; chn < 8; chn++)
	{
		ChnSettings[chn].Reset();
		ChnSettings[chn].nPan = (chn & 1) ? 0xD0 : 0x30;
	}

	// Reading Patterns
	for(PATTERNINDEX pat = 0; pat < fileHeader.patterns; pat++)
	{
		if(!(loadFlags & loadPatternData) || Patterns.Insert(pat, 64))
		{
			file.Skip(64 * 8 * 3);
			continue;
		}

		std::vector<uint8> effect(8, 0xFF);
		for(ROWINDEX row = 0; row < 64; row++)
		{
			PatternRow m = Patterns[pat].GetRow(row);

			for(CHANNELINDEX chn = 0; chn < 8; chn++, m++)
			{
				uint8 data[3];
				file.ReadArray(data);

				uint8 note = data[0] >> 2;
				uint8 instr = ((data[0] & 0x03) << 4) | (data[1] >> 4);
				uint8 vol = data[1] & 0x0F;
				if(data[0] < 0xFE)
				{
					m->note = note + 36 + NOTE_MIN;
					m->instr = instr + 1;
					effect[chn] = 0xFF;
				}
				if(data[0] <= 0xFE)
				{
					m->volcmd = VOLCMD_VOLUME;
					m->vol = ((vol * 64 + 8) / 15);
				}

				if(data[2] != 0xFF)
				{
					effect[chn] = data[2];
				}
				if((data[2] & 0x0F) == 0 && data[2] != 0x30)
				{
					// A param value of 0 resets the effect.
					effect[chn] = 0xFF;
				}
				if(effect[chn] == 0xFF)
				{
					continue;
				}

				m->param = effect[chn] & 0x0F;

				static const ModCommand::COMMAND effTrans[] =
				{
					CMD_PORTAMENTOUP,	CMD_PORTAMENTODOWN,	CMD_TONEPORTAMENTO,	CMD_PORTAMENTOUP,
					CMD_ARPEGGIO,		CMD_SPEED,			CMD_PANNINGSLIDE,	CMD_RETRIG,
				};

				if(static_cast<uint8>(effect[chn] >> 4) < CountOf(effTrans))
				{
					m->command = effTrans[effect[chn] >> 4];
				} else
				{
					m->command = CMD_NONE;
					continue;
				}

				// Fix some commands
				switch(effect[chn] >> 4)
				{
				case 3:
					// D - frequency adjust
					if(m->param)
					{
						m->param |= 0xF0;
					} else
					{
						// Restore original note
						m->command = CMD_TONEPORTAMENTO;
						m->param = 0xFF;
					}
					effect[chn] = 0xFF;
					break;

				case 4:
					// E - frequency vibrato - sounds like an arpeggio, sometimes goes up, sometimes goes down,
					// depending on when you restart playback? weird!
					m->param |= (m->param << 4);
					break;

				case 5:
					// F - set tempo
					// TODO: param 0 is a "super fast tempo" in extended mode (?) 
					effect[chn] = 0xFF;
					break;

				case 6:
					// G - subcommands (extended)
					switch(m->param)
					{
					case 0:
						// balance fine slide left
						m->param = 0x8F;
						break;
					case 1:
						// balance fine slide right
						m->param = 0xF8;
						break;
					default:
						m->command = CMD_NONE;
					}
					break;
				}
			}
		}

		// Write pattern break
		if(fileHeader.breaks[pat] < 63)
		{
			Patterns[pat].WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(fileHeader.breaks[pat]).Retry(EffectWriter::rmTryNextRow));
		}
		// And of course the speed...
		Patterns[pat].WriteEffect(EffectWriter(CMD_SPEED, fileHeader.tempoList[pat]).Retry(EffectWriter::rmTryNextRow));
	}

	if(loadFlags & loadSampleData)
	{
		// Reading Samples
		const SampleIO sampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::unsignedPCM);

		for(SAMPLEINDEX n = 1; n <= m_nSamples; n++)
		{
			sampleIO.ReadSample(Samples[n], file);
		}
	}

	return true;
}