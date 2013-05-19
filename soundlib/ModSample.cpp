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

// returns 8363*2^((transp*128+ftune)/(12*128))
uint32 ModSample::TransposeToFrequency(int transpose, int finetune)
//-----------------------------------------------------------------
{
	// return (unsigned int) (8363.0 * pow(2, (transp * 128.0 + ftune) / 1536.0));
	const float _fbase = 8363;
	const float _factor = 1.0f / (12.0f * 128.0f);
	uint32 freq;

#if defined(ENABLE_X86)
	int result;
	transpose = (transpose << 7) + finetune;
	_asm
	{
		fild transpose
		fld _factor
		fmulp st(1), st(0)
		fist result
		fisub result
		f2xm1
		fild result
		fld _fbase
		fscale
		fstp st(1)
		fmul st(1), st(0)
		faddp st(1), st(0)
		fistp freq
	}
#else // !ENABLE_X86
	freq = static_cast<uint32>(std::pow(2.0f, transpose * 128 + finetune) * (_fbase * _factor));
#endif // ENABLE_X86

	uint32 derr = freq % 11025;
	if(derr <= 8) freq -= derr;
	if(derr >= 11015) freq += 11025 - derr;
	derr = freq % 1000;
	if(derr <= 5) freq -= derr;
	if(derr >= 995) freq += 1000 - derr;
	return freq;
}


void ModSample::TransposeToFrequency()
//------------------------------------
{
	nC5Speed = TransposeToFrequency(RelativeTone, nFineTune);
}


// returns 12*128*log2(freq/8363)
int ModSample::FrequencyToTranspose(uint32 freq)
//----------------------------------------------
{
	// return (int) (1536.0 * (log(freq / 8363.0) / log(2)));

	const float _f1_8363 = 1.0f / 8363.0f;
	const float _factor = 128 * 12;
	int result;

	if(!freq)
	{
		return 0;
	}

#if defined(ENABLE_X86)
	_asm
	{
		fld _factor
		fild freq
		fld _f1_8363
		fmulp st(1), st(0)
		fyl2x
		fistp result
	}
#else // !ENABLE_X86
	const float inv_log_2 = 1.44269504089f; // 1.0f/std::log(2.0f)	
	result = std::log(freq * _f1_8363) * (_factor * inv_log_2);
#endif // ENABLE_X86
	return result;
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
