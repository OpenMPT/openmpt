/*
 * Load_wav.cpp
 * ------------
 * Purpose: WAV importer
 * Notes  : This loader converts each WAV channel into a separate mono sample.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "WAVTools.h"
#include "SampleFormatConverters.h"


/////////////////////////////////////////////////////////////
// WAV file support


template <typename SampleConversion>
bool CopyWavChannel(ModSample &sample, const FileReader &file, size_t channelIndex, size_t numChannels, SampleConversion conv = SampleConversion())
//-------------------------------------------------------------------------------------------------------------------------------------------------
{
	ASSERT(sample.GetNumChannels() == 1);
	ASSERT(sample.GetElementarySampleSize() == sizeof(typename SampleConversion::output_t));

	const size_t offset = channelIndex * sizeof(typename SampleConversion::input_t) * SampleConversion::input_inc;

	if(sample.AllocateSample() == 0 || !file.CanRead(offset))
	{
		return false;
	}

	const char *inBuf = file.GetRawData();
	CopySample<SampleConversion>(reinterpret_cast<typename SampleConversion::output_t*>(sample.pSample), sample.nLength, 1, inBuf + offset, file.BytesLeft() - offset, numChannels, conv);
	return true;
}


bool CSoundFile::ReadWav(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	WAVReader wavFile(file);

	if(!wavFile.IsValid()
		|| wavFile.GetNumChannels() == 0
		|| wavFile.GetNumChannels() > MAX_BASECHANNELS
		|| wavFile.GetBitsPerSample() == 0
		|| wavFile.GetBitsPerSample() > 32
		|| (wavFile.GetSampleFormat() != WAVFormatChunk::fmtPCM && wavFile.GetSampleFormat() != WAVFormatChunk::fmtFloat))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals();
	m_nChannels = std::max(wavFile.GetNumChannels(), uint16(2));
	if(Patterns.Insert(0, 64) || Patterns.Insert(1, 64))
	{
		return false;
	}
	
	const SmpLength sampleLength = wavFile.GetSampleLength();

	// Setting up module length
	// Calculate sample length in ticks at tempo 125
	const uint32 sampleTicks = mpt::saturate_cast<uint32>(((sampleLength * 50) / wavFile.GetSampleRate()) + 1);
	uint32 ticksPerRow = std::max((sampleTicks + 63u) / 63u, 1u);

	Order.clear();
	Order.Append(0);
	ORDERINDEX numOrders = 1;
	while(ticksPerRow >= 32)
	{
		Order.Append(1);

		numOrders++;
		ticksPerRow = (sampleTicks + (64 * numOrders - 1)) / (64 * numOrders);
		if(numOrders == MAX_ORDERS)
		{
			break;
		}
	}

	m_nType = MOD_TYPE_WAV;
	m_nSamples = wavFile.GetNumChannels();
	m_nInstruments = 0;
	m_nDefaultSpeed = ticksPerRow;
	m_nDefaultTempo = 125;
	m_SongFlags = SONG_LINEARSLIDES;

	for(CHANNELINDEX channel = 0; channel < wavFile.GetNumChannels(); channel++)
	{
		ChnSettings[channel].Reset();
		ChnSettings[channel].nPan = (channel % 2u) ? 256 : 0;
	}

	// Setting up pattern
	PatternRow pattern = Patterns[0].GetRow(0);
	pattern[0].note = pattern[1].note = NOTE_MIDDLEC;
	pattern[0].instr = pattern[1].instr = 1;

	const FileReader sampleChunk = wavFile.GetSampleData();

	// Read every channel into its own sample lot.
	for(SAMPLEINDEX channel = 0; channel < GetNumSamples(); channel++)
	{
		pattern[channel].note = pattern[0].note;
		pattern[channel].instr = static_cast<ModCommand::INSTR>(channel + 1);

		ModSample &sample = Samples[channel + 1];
		sample.Initialize();
		sample.uFlags = CHN_PANNING;
		sample.nLength =  sampleLength;
		sample.nC5Speed = wavFile.GetSampleRate();
		strcpy(m_szNames[channel + 1], "");
		wavFile.ApplySampleSettings(sample, m_szNames[channel + 1]);

		if(wavFile.GetNumChannels() > 1)
		{
			// Pan all samples appropriately
			switch(channel)
			{
			case 0:
				sample.nPan = 0;
				break;
			case 1:
				sample.nPan = 256;
				break;
			case 2:
				sample.nPan = (wavFile.GetNumChannels() == 3 ? 128u : 64u);
				pattern[channel].command = CMD_S3MCMDEX;
				pattern[channel].param = 0x91;
				break;
			case 3:
				sample.nPan = 192;
				pattern[channel].command = CMD_S3MCMDEX;
				pattern[channel].param = 0x91;
				break;
			default:
				sample.nPan = 128;
				break;
			}
		}

		if(wavFile.GetBitsPerSample() > 8)
		{
			sample.uFlags |= CHN_16BIT;
		}

		if(wavFile.GetSampleFormat() == WAVFormatChunk::fmtFloat)
		{
			CopyWavChannel<SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeFloat32<littleEndian32> > >(sample, sampleChunk, channel, wavFile.GetNumChannels());
		} else
		{
			if(wavFile.GetBitsPerSample() <= 8)
			{
				CopyWavChannel<SC::DecodeUint8>(sample, sampleChunk, channel, wavFile.GetNumChannels());
			} else if(wavFile.GetBitsPerSample() <= 16)
			{
				CopyWavChannel<SC::DecodeInt16<0, littleEndian16> >(sample, sampleChunk, channel, wavFile.GetNumChannels());
			} else if(wavFile.GetBitsPerSample() <= 24)
			{
				CopyWavChannel<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(sample, sampleChunk, channel, wavFile.GetNumChannels());
			} else if(wavFile.GetBitsPerSample() <= 32)
			{
				CopyWavChannel<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, littleEndian32> > >(sample, sampleChunk, channel, wavFile.GetNumChannels());
			}
		}

	}

	return true;
}


////////////////////////////////////////////////////////////////////////
// IMA ADPCM Support


// Note: Only works for mono samples.
bool IMAADPCMUnpack16(int16 *target, SmpLength sampleLen, FileReader file, uint16 blockAlign)
//-------------------------------------------------------------------------------------------
{
	static const int32 IMAIndexTab[8] =  { -1, -1, -1, -1, 2, 4, 6, 8 };
	static const int32 IMAUnpackTable[90] =
	{
		7,     8,     9,    10,    11,    12,    13,    14,
		16,    17,    19,    21,    23,    25,    28,    31,
		34,    37,    41,    45,    50,    55,    60,    66,
		73,    80,    88,    97,   107,   118,   130,   143,
		157,   173,   190,   209,   230,   253,   279,   307,
		337,   371,   408,   449,   494,   544,   598,   658,
		724,   796,   876,   963,  1060,  1166,  1282,  1411,
		1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
		3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
		7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
		32767, 0
	};

	if((sampleLen < 4) || (!target) || (blockAlign < 5) || (blockAlign > file.GetLength()))
		return false;

	SmpLength nPos = 0;
	while((nPos < sampleLen) && file.CanRead(5))
	{
		int32 value = file.ReadIntLE<int16>();
		int32 nIndex = file.ReadIntLE<uint8>();
		nIndex = Clamp(nIndex, 0, 89);
		file.Skip(1);
		target[nPos++] = (int16)value;
		for(uint32 i = 0; (i < (blockAlign - 4u) * 2u) && (nPos < sampleLen) && file.AreBytesLeft(); i++)
		{
			uint8 delta;
			if(i & 1)
			{
				delta = (file.ReadIntLE<uint8>() >> 4) & 0x0F;
			} else
			{
				delta = file.ReadIntLE<uint8>() & 0x0F;
				file.SkipBack(1);
			}
			int32 v = IMAUnpackTable[nIndex] >> 3;
			if (delta & 1) v += IMAUnpackTable[nIndex] >> 2;
			if (delta & 2) v += IMAUnpackTable[nIndex] >> 1;
			if (delta & 4) v += IMAUnpackTable[nIndex];
			if (delta & 8) value -= v; else value += v;
			nIndex += IMAIndexTab[delta & 7];
			nIndex = Clamp(nIndex, 0, 88);
			target[nPos++] = static_cast<int16>(Clamp(value, -32768, 32767));
		}
	}
	return true;
}
