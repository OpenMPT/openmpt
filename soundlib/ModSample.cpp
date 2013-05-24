/*
 * ModSample.h
 * -----------
 * Purpose: Module Sample header class and helpers
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#include "ModSample.h"

#include <cmath>


// Translate sample properties between two given formats.
void ModSample::Convert(MODTYPE fromType, MODTYPE toType)
//-------------------------------------------------------
{
	// Convert between frequency and transpose values if necessary.
	if((!(toType & (MOD_TYPE_MOD | MOD_TYPE_XM))) && (fromType & (MOD_TYPE_MOD | MOD_TYPE_XM)))
	{
		TransposeToFrequency();
		RelativeTone = 0;
		nFineTune = 0;
	} else if((toType & (MOD_TYPE_MOD | MOD_TYPE_XM)) && (!(fromType & (MOD_TYPE_MOD | MOD_TYPE_XM))))
	{
		FrequencyToTranspose();
		if(toType & MOD_TYPE_MOD)
		{
			RelativeTone = 0;
		}
	}

	// No ping-pong loop, panning and auto-vibrato for MOD / S3M samples
	if(toType & (MOD_TYPE_MOD | MOD_TYPE_S3M))
	{
		uFlags &= ~(CHN_PINGPONGLOOP | CHN_PANNING);

		nVibDepth = 0;
		nVibRate = 0;
		nVibSweep = 0;
		nVibType = VIB_SINE;
	}

	// No global volume sustain loops for MOD/S3M/XM
	if(toType & (MOD_TYPE_MOD | MOD_TYPE_XM | MOD_TYPE_S3M))
	{
		nGlobalVol = 64;
		// Sustain loops - convert to normal loops
		if((uFlags & CHN_SUSTAINLOOP) != 0)
		{
			// We probably overwrite a normal loop here, but since sustain loops are evaluated before normal loops, this is just correct.
			nLoopStart = nSustainStart;
			nLoopEnd = nSustainEnd;
			uFlags |= CHN_LOOP;
			if(uFlags & CHN_PINGPONGSUSTAIN)
			{
				uFlags |= CHN_PINGPONGLOOP;
			} else
			{
				uFlags &= ~CHN_PINGPONGLOOP;
			}
		}
		nSustainStart = nSustainEnd = 0;
		uFlags &= ~(CHN_SUSTAINLOOP|CHN_PINGPONGSUSTAIN);
	}

	// All XM samples have default panning, and XM's autovibrato settings are rather limited.
	if(toType & MOD_TYPE_XM)
	{
		if(!(uFlags & CHN_PANNING))
		{
			uFlags |= CHN_PANNING;
			nPan = 128;
		}

		LimitMax(nVibDepth, BYTE(15));
		LimitMax(nVibRate, BYTE(63));
	}


	// Autovibrato sweep setting is inverse in XM (0 = "no sweep") and IT (0 = "no vibrato")
	if(((fromType & MOD_TYPE_XM) && (toType & (MOD_TYPE_IT | MOD_TYPE_MPT))) || ((toType & MOD_TYPE_XM) && (fromType & (MOD_TYPE_IT | MOD_TYPE_MPT))))
	{
		if(nVibRate != 0 && nVibDepth != 0)
		{
			nVibSweep = 255 - nVibSweep;
		}
	}
}


// Initialize sample slot with default values.
void ModSample::Initialize(MODTYPE type)
//--------------------------------------
{
	nLength = 0;
	nLoopStart = nLoopEnd = 0;
	nSustainStart = nSustainEnd = 0;
	nC5Speed = 8363;
	nPan = 128;
	nVolume = 256;
	nGlobalVol = 64;
	uFlags.reset(CHN_PANNING | CHN_SUSTAINLOOP | CHN_LOOP | CHN_PINGPONGLOOP | CHN_PINGPONGSUSTAIN);
	if(type == MOD_TYPE_XM)
	{
		uFlags.set(CHN_PANNING);
	}
	RelativeTone = 0;
	nFineTune = 0;
	nVibType = VIB_SINE;
	nVibSweep = 0;
	nVibDepth = 0;
	nVibRate = 0;
	filename[0] = '\0';
}


// Returns sample rate of the sample.
uint32 ModSample::GetSampleRate(const MODTYPE type) const
//-------------------------------------------------------
{
	uint32 rate;
	if(type & (MOD_TYPE_MOD | MOD_TYPE_XM))
		rate = TransposeToFrequency(RelativeTone, nFineTune);
	else
		rate = nC5Speed;
	return (rate > 0) ? rate : 8363;
}


// Allocate sample based on a ModSample's properties.
// Returns number of bytes allocated, 0 on failure.
size_t ModSample::AllocateSample()
//--------------------------------
{
	// Prevent overflows...
	if(nLength > SIZE_MAX - 6u || SIZE_MAX / GetBytesPerSample() < nLength + 6u)
	{
		return 0;
	}
	FreeSample();

	size_t sampleSize = (nLength + 6u) * GetBytesPerSample();
	if((pSample = CSoundFile::AllocateSample(sampleSize)) == nullptr)
	{
		return 0;
	} else
	{
		return sampleSize;
	}
}


void ModSample::FreeSample()
//--------------------------
{
	CSoundFile::FreeSample(pSample);
	pSample = nullptr;
}


// Remove loop points if they're invalid.
void ModSample::SanitizeLoops()
//-----------------------------
{
	LimitMax(nSustainEnd, nLength);
	LimitMax(nLoopEnd, nLength);
	if(nSustainStart >= nSustainEnd)
	{
		nSustainStart = nSustainEnd = 0;
		uFlags.reset(CHN_SUSTAINLOOP | CHN_PINGPONGSUSTAIN);
	}
	if(nLoopStart >= nLoopEnd)
	{
		nLoopStart = nLoopEnd = 0;
		uFlags.reset(CHN_LOOP | CHN_PINGPONGLOOP);
	}
}


/////////////////////////////////////////////////////////////
// Transpose <-> Frequency conversions

uint32 ModSample::TransposeToFrequency(int transpose, int finetune)
//-----------------------------------------------------------------
{
	return Util::Round<uint32>(std::pow(2.0f, (transpose * 128.0f + finetune) * (1.0f / (12.0f * 128.0f))) * 8363.0f);
}


void ModSample::TransposeToFrequency()
//------------------------------------
{
	nC5Speed = TransposeToFrequency(RelativeTone, nFineTune);
}


int ModSample::FrequencyToTranspose(uint32 freq)
//----------------------------------------------
{
	const float inv_log_2 = 1.44269504089f; // 1.0f/std::log(2.0f)	
	return Util::Round<int>(std::log(freq * (1.0f / 8363.0f)) * (12.0f * 128.0f * inv_log_2));
}


void ModSample::FrequencyToTranspose()
//------------------------------------
{
	int f2t = FrequencyToTranspose(nC5Speed);
	int transpose = f2t >> 7;
	int finetune = f2t & 0x7F;	//0x7F == 111 1111
	if(finetune > 80)			// XXX Why is this 80?
	{
		transpose++;
		finetune -= 128;
	}
	Limit(transpose, -127, 127);
	RelativeTone = static_cast<int8>(transpose);
	nFineTune = static_cast<int8>(finetune);
}
