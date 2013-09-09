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

#include "Snd_defs.h"



//////////////////////////////////////////////////////////////////////////
// Noise Shaping (Dithering)

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


void Dither::Reset()
{
	state = DitherState();
}

Dither::Dither()
{
	mode = DitherModPlug;
}

void Dither::SetMode(DitherMode &mode_)
{
	mode = mode_;
}

DitherMode Dither::GetMode() const
{
	return mode;
}

void Dither::Process(int *mixbuffer, std::size_t count, std::size_t channels, int bits)
{
	switch(mode)
	{
		case DitherNone:
			// nothing
			break;
		case DitherModPlug:
			Dither_ModPlug(mixbuffer, count, channels, bits, state.modplug);
			break;
	}
}
