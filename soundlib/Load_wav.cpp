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
#include "Wav.h"
#include "SampleFormatConverters.h"

#ifndef WAVE_FORMAT_EXTENSIBLE
#define WAVE_FORMAT_EXTENSIBLE	0xFFFE
#endif

/////////////////////////////////////////////////////////////
// WAV file support


template <typename SampleConverter>
bool CopyWavChannel(ModSample &sample, const FileReader &file, size_t channelIndex, size_t numChannels)
//-----------------------------------------------------------------------------------------------------
{
	ASSERT(sample.GetNumChannels() == 1);
	ASSERT(sample.GetElementarySampleSize() == sizeof(SampleConverter::output_t));

	const size_t offset = channelIndex * sizeof(SampleConverter::input_t);

	if(sample.AllocateSample() == 0 || file.BytesLeft() < offset)
	{
		return false;
	}

	const uint8 *inBuf = reinterpret_cast<const uint8 *>(file.GetRawData());
	CopySample<SampleConverter>(sample.pSample, sample.nLength, 1, inBuf + offset, file.BytesLeft() - offset, numChannels);
	CSoundFile::AdjustSampleLoop(sample);
	return true;
}


bool CSoundFile::ReadWav(const BYTE *lpStream, const DWORD dwMemLength)
//---------------------------------------------------------------------
{
	DWORD dwMemPos = 0;
	WAVEFILEHEADER *phdr = (WAVEFILEHEADER *)lpStream;
	WAVEFORMATHEADER *pfmt = (WAVEFORMATHEADER *)(lpStream + sizeof(WAVEFILEHEADER));
	if ((!lpStream) || (dwMemLength < (DWORD)sizeof(WAVEFILEHEADER))) return false;
	if ((phdr->id_RIFF != IFFID_RIFF) || (phdr->id_WAVE != IFFID_WAVE)
	 || (pfmt->id_fmt != IFFID_fmt)) return false;
	dwMemPos = sizeof(WAVEFILEHEADER) + 8 + pfmt->hdrlen;
	if ((dwMemPos + 8 >= dwMemLength)
	 || ((pfmt->format != WAVE_FORMAT_PCM) && (pfmt->format != WAVE_FORMAT_EXTENSIBLE) && (pfmt->format != WAVE_FORMAT_IEEE_FLOAT))
	 || (pfmt->channels > 4)
	 || (!pfmt->channels)
	 || (!pfmt->freqHz)
	 || (pfmt->bitspersample == 0)
	 || (pfmt->bitspersample > 32))  return false;
	WAVEDATAHEADER *pdata;
	for (;;)
	{
		pdata = (WAVEDATAHEADER *)(lpStream + dwMemPos);
		if (pdata->id_data == IFFID_data) break;
		dwMemPos += pdata->length + 8;
		if (dwMemPos >= dwMemLength - 8) return false;
	}
	m_nType = MOD_TYPE_WAV;
	m_nSamples = 0;
	m_nInstruments = 0;
	m_nChannels = 4;
	m_nDefaultSpeed = 8;
	m_nDefaultTempo = 125;
	m_dwSongFlags = SONG_LINEARSLIDES; // For no resampling
	Order.resize(MAX_ORDERS, Order.GetInvalidPatIndex());
	Order[0] = 0;
	bool fail = Patterns.Insert(0, 64);
	fail = Patterns.Insert(1, 64);
	if(fail) return true;
	UINT samplesize = ((pfmt->channels * pfmt->bitspersample) + 7) / 8;
	SmpLength len = pdata->length;
	if (len > dwMemLength - 8 - dwMemPos) len = dwMemLength - dwMemPos - 8;
	len /= samplesize;
	LimitMax(len, MAX_SAMPLE_LENGTH);
	if (!len) return true;
	// Setting up module length
	DWORD dwTime = ((len * 50) / pfmt->freqHz) + 1;
	DWORD framesperrow = (dwTime + 63) / 63;
	if (framesperrow < 4) framesperrow = 4;
	UINT norders = 1;
	while (framesperrow >= 0x20)
	{
		Order[norders++] = 1;
		Order[norders] = 0xFF;
		framesperrow = (dwTime + (64 * norders - 1)) / (64 * norders);
		if (norders >= MAX_ORDERS-1) break;
	}
	m_nDefaultSpeed = framesperrow;
	for (UINT iChn=0; iChn<4; iChn++)
	{
		ChnSettings[iChn].nPan = (iChn & 1) ? 256 : 0;
		ChnSettings[iChn].nVolume = 64;
		ChnSettings[iChn].dwFlags = 0;
	}
	// Setting up speed command
	ModCommand *pcmd = Patterns[0];
	pcmd[0].command = CMD_SPEED;
	pcmd[0].param = (BYTE)m_nDefaultSpeed;
	pcmd[0].note = 5*12+1;
	pcmd[0].instr = 1;
	pcmd[1].note = pcmd[0].note;
	pcmd[1].instr = pcmd[0].instr;
	m_nSamples = pfmt->channels;

	// Support for Multichannel Wave
	FileReader file((char*)(lpStream + dwMemPos + 8), dwMemLength - dwMemPos - 8);
	for (UINT nChn=0; nChn<m_nSamples; nChn++)
	{
		ModSample &sample = Samples[nChn + 1];
		pcmd[nChn].note = pcmd[0].note;
		pcmd[nChn].instr = (BYTE)(nChn+1);
		sample.Initialize();
		sample.nLength = len;
		sample.nC5Speed = pfmt->freqHz;
		sample.uFlags = CHN_PANNING;
		if (m_nSamples > 1)
		{
			switch(nChn)
			{
			case 0:	sample.nPan = 0; break;
			case 1:	sample.nPan = 256; break;
			case 2: sample.nPan = (m_nSamples == 3 ? 128u : 64u); pcmd[nChn].command = CMD_S3MCMDEX; pcmd[nChn].param = 0x91; break;
			case 3: sample.nPan = 192; pcmd[nChn].command = CMD_S3MCMDEX; pcmd[nChn].param = 0x91; break;
			default: sample.nPan = 128; break;
			}
		}

		if(pfmt->format == WAVE_FORMAT_IEEE_FLOAT)
		{
			sample.uFlags |= CHN_16BIT;
			CopyWavChannel<ReadFloat32toInt16PCM<littleEndian32> >(sample, file, nChn, m_nSamples);
		} else
		{
			if(pfmt->bitspersample <= 8)
			{
				CopyWavChannel<ReadInt8PCM<0x80u> >(sample, file, nChn, m_nSamples);
			} else if(pfmt->bitspersample <= 16)
			{
				sample.uFlags |= CHN_16BIT;
				CopyWavChannel<ReadInt16PCM<0, littleEndian16> >(sample, file, nChn, m_nSamples);
			} else if(pfmt->bitspersample <= 24)
			{
				sample.uFlags |= CHN_16BIT;
				CopyWavChannel<ReadBigIntTo16PCM<3, 1, 2> >(sample, file, nChn, m_nSamples);
			} else if(pfmt->bitspersample <= 32)
			{
				sample.uFlags |= CHN_16BIT;
				CopyWavChannel<ReadBigIntTo16PCM<4, 2, 3> >(sample, file, nChn, m_nSamples);
			}
		}

	}
	return true;
}


////////////////////////////////////////////////////////////////////////
// IMA ADPCM Support

#pragma pack(push, 1)

typedef struct IMAADPCMBLOCK
{
	WORD sample;
	BYTE index;
	BYTE Reserved;
} DVI_ADPCMBLOCKHEADER;

#pragma pack(pop)

static const int gIMAUnpackTable[90] =
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


BOOL IMAADPCMUnpack16(signed short *pdest, UINT nLen, LPBYTE psrc, DWORD dwBytes, UINT pkBlkAlign)
//------------------------------------------------------------------------------------------------
{
	static const int gIMAIndexTab[8] =  { -1, -1, -1, -1, 2, 4, 6, 8 };
	UINT nPos;
	int value;

	if ((nLen < 4) || (!pdest) || (!psrc)
	 || (pkBlkAlign < 5) || (pkBlkAlign > dwBytes)) return false;
	nPos = 0;
	while ((nPos < nLen) && (dwBytes > 4))
	{
		int nIndex;
		value = *((short int *)psrc);
		nIndex = psrc[2];
		psrc += 4;
		dwBytes -= 4;
		pdest[nPos++] = (short int)value;
		for (UINT i=0; ((i<(pkBlkAlign-4)*2) && (nPos < nLen) && (dwBytes)); i++)
		{
			BYTE delta;
			if (i & 1)
			{
				delta = (BYTE)(((*(psrc++)) >> 4) & 0x0F);
				dwBytes--;
			} else
			{
				delta = (BYTE)((*psrc) & 0x0F);
			}
			int v = gIMAUnpackTable[nIndex] >> 3;
			if (delta & 1) v += gIMAUnpackTable[nIndex] >> 2;
			if (delta & 2) v += gIMAUnpackTable[nIndex] >> 1;
			if (delta & 4) v += gIMAUnpackTable[nIndex];
			if (delta & 8) value -= v; else value += v;
			nIndex += gIMAIndexTab[delta & 7];
			if (nIndex < 0) nIndex = 0; else
			if (nIndex > 88) nIndex = 88;
			Limit(value, -32768, 32767);
			pdest[nPos++] = (short int)value;
		}
	}
	return true;
}


