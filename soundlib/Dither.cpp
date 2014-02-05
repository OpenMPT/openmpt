/*
 * Dither.cpp
 * ----------
 * Purpose: Dithering when converting to lower resolution sample formats.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "Dither.h"
#include "Mixer.h"

#include "../common/misc_util.h"


//////////////////////////////////////////////////////////////////////////
// Noise Shaping (Dithering)


std::wstring Dither::GetModeName(DitherMode mode)
//-----------------------------------------------
{
	switch(mode)
	{
		case DitherNone   : return L"no"     ; break;
		case DitherDefault: return L"default"; break;
		case DitherModPlug: return L"0.5 bit"; break;
		case DitherSimple : return L"1 bit"  ; break;
		default           : return L""       ; break;
	}
}


#if MPT_COMPILER_MSVC
#pragma warning(disable:4731) // ebp modified
#endif


#ifdef ENABLE_X86

void X86_Dither(int *pBuffer, UINT nSamples, UINT nBits, DitherModPlugState *state)
//---------------------------------------------------------------------------------
{
	if(nBits + MIXING_ATTENUATION + 1 >= 32) //if(nBits>16)
	{
		return;
	}

	static int gDitherA_global, gDitherB_global;

	int gDitherA = state ? state->rng_a : gDitherA_global;
	int gDitherB = state ? state->rng_b : gDitherB_global;

	_asm {
	mov esi, pBuffer	// esi = pBuffer+i
	mov eax, nSamples	// ebp = i
	mov ecx, nBits		// ecx = number of bits of noise
	mov edi, gDitherA	// Noise generation
	mov ebx, gDitherB
	add ecx, MIXING_ATTENUATION+1
	push ebp
	mov ebp, eax
noiseloop:
	rol edi, 1
	mov eax, dword ptr [esi]
	xor edi, 0x10204080
	add esi, 4
	lea edi, [ebx*4+edi+0x78649E7D]
	mov edx, edi
	rol edx, 16
	lea edx, [edx*4+edx]
	add ebx, edx
	mov edx, ebx
	sar edx, cl
	add eax, edx
	dec ebp
	mov dword ptr [esi-4], eax
	jnz noiseloop
	pop ebp
	mov gDitherA, edi
	mov gDitherB, ebx
	}

	if(state) state->rng_a = gDitherA; else gDitherA_global = gDitherA;
	if(state) state->rng_b = gDitherB; else gDitherB_global = gDitherB;

}

#endif // ENABLE_X86


static forceinline int32 dither_rand(uint32 &a, uint32 &b)
//--------------------------------------------------------
{
	a = (a << 1) | (a >> 31);
	a ^= 0x10204080u;
	a += 0x78649E7Du + (b * 4);
	b += ((a << 16 ) | (a >> 16)) * 5;
	return (int32)b;
}

static void C_Dither(int *pBuffer, std::size_t count, UINT nBits, DitherModPlugState *state)
//------------------------------------------------------------------------------------------
{
	if(nBits + MIXING_ATTENUATION + 1 >= 32) //if(nBits>16)
	{
		return;
	}

	static uint32 global_a = 0;
	static uint32 global_b = 0;

	uint32 a = state ? state->rng_a : global_a;
	uint32 b = state ? state->rng_b : global_b;

	while(count--)
	{
		*pBuffer += dither_rand(a, b) >> (nBits + MIXING_ATTENUATION + 1);
		pBuffer++;
	}

	if(state) state->rng_a = a; else global_a = a;
	if(state) state->rng_b = b; else global_b = b;

}

static void Dither_ModPlug(int *pBuffer, std::size_t count, std::size_t channels, UINT nBits, DitherModPlugState &state)
//----------------------------------------------------------------------------------------------------------------------
{
	#ifdef ENABLE_X86
		X86_Dither(pBuffer, count * channels, nBits, &state);
	#else // !ENABLE_X86
		C_Dither(pBuffer, count * channels, nBits, &state);
	#endif // ENABLE_X86
}


#define FASTRAND_MAX 0x7fff
#define FASTRAND_BITS 15

static forceinline int fastrand(uint32 &state)
//--------------------------------------------
{
	state = 214013 * state + 2531011;
	return (state >> 16) & 0x7FFF;
}

static forceinline int fastrandbits(uint32 &state, int bits)
//----------------------------------------------------------
{
	int result = 0;
	if(bits > 0 * FASTRAND_BITS) result = (result << FASTRAND_BITS) | fastrand(state);
	if(bits > 1 * FASTRAND_BITS) result = (result << FASTRAND_BITS) | fastrand(state);
	if(bits > 2 * FASTRAND_BITS) result = (result << FASTRAND_BITS) | fastrand(state);
	result &= (1 << bits) - 1;
	return result;
}

template<int targetbits, int channels, int ditherdepth = 1, bool triangular = false, bool shaped = true>
struct Dither_SimpleTemplate
{
noinline void operator () (int *mixbuffer, std::size_t count, DitherSimpleState &state)
//-------------------------------------------------------------------------------------
{
	STATIC_ASSERT(sizeof(int) == 4);
	STATIC_ASSERT(FASTRAND_BITS * 3 >= (32-targetbits) - MIXING_ATTENUATION);
	const int rshift = (32-targetbits) - MIXING_ATTENUATION;
	if(rshift <= 0)
	{
		// nothing to dither
		return;
	}
	const int round_mask = ~((1<<rshift)-1);
	const int round_offset = 1<<(rshift-1);
	const int noise_bits = rshift + (ditherdepth - 1);
	const int noise_bias = (1<<(noise_bits-1));
	DitherSimpleState s = state;
	for(std::size_t i = 0; i < count; ++i)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			int noise = 0;
			if(triangular)
			{
				noise = (fastrandbits(s.rng, noise_bits) + fastrandbits(s.rng, noise_bits)) >> 1;
			} else
			{
				noise = fastrandbits(s.rng, noise_bits);
			}
			noise -= noise_bias; // un-bias
			int val = *mixbuffer;
			if(shaped)
			{
				val += (s.error[channel] >> 1);
			}
			int rounded = (val + noise + round_offset) & round_mask;;
			s.error[channel] = val - rounded;
			*mixbuffer = rounded;
			mixbuffer++;
		}
	}
	state = s;
}
};

static void Dither_Simple(int *mixbuffer, std::size_t count, std::size_t channels, int bits, DitherSimpleState &state)
//--------------------------------------------------------------------------------------------------------------------
{
	switch(bits)
	{
		case 8:
			switch(channels)
			{
				case 1:
					Dither_SimpleTemplate<8,1>()(mixbuffer, count, state);
					break;
				case 2:
					Dither_SimpleTemplate<8,2>()(mixbuffer, count, state);
					break;
				case 4:
					Dither_SimpleTemplate<8,4>()(mixbuffer, count, state);
					break;
			}
			break;
		case 16:
			switch(channels)
			{
				case 1:
					Dither_SimpleTemplate<16,1>()(mixbuffer, count, state);
					break;
				case 2:
					Dither_SimpleTemplate<16,2>()(mixbuffer, count, state);
					break;
				case 4:
					Dither_SimpleTemplate<16,4>()(mixbuffer, count, state);
					break;
			}
			break;
		case 24:
			switch(channels)
			{
				case 1:
					Dither_SimpleTemplate<24,1>()(mixbuffer, count, state);
					break;
				case 2:
					Dither_SimpleTemplate<24,2>()(mixbuffer, count, state);
					break;
				case 4:
					Dither_SimpleTemplate<24,4>()(mixbuffer, count, state);
					break;
			}
			break;
	}
}


void Dither::Reset()
//------------------
{
	state = DitherState();
}


Dither::Dither()
//--------------
{
	mode = DitherDefault;
}


void Dither::SetMode(DitherMode mode_)
//------------------------------------
{
	mode = mode_;
}


DitherMode Dither::GetMode() const
//--------------------------------
{
	return mode;
}


void Dither::Process(int *mixbuffer, std::size_t count, std::size_t channels, int bits)
//-------------------------------------------------------------------------------------
{
	switch(mode)
	{
		case DitherNone:
			// nothing
			break;
		case DitherModPlug:
			Dither_ModPlug(mixbuffer, count, channels, bits, state.modplug);
			break;
		case DitherSimple:
			Dither_Simple(mixbuffer, count, channels, bits, state.simple);
			break;
		case DitherDefault:
		default:
			Dither_ModPlug(mixbuffer, count, channels, bits, state.modplug);
			break;
	}
}
