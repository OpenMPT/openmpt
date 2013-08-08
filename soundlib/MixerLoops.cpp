/*
 * MixerLoops.cpp
 * --------------
 * Purpose: Utility inner loops for mixer-related functionality.
 * Notes  : 
 *          x86 ( AMD/INTEL ) based low level based mixing functions:
 *          This file contains critical code. The basic X86 functions are
 *          defined at the bottom of the file. #define's are used to isolate
 *          the different flavours of functionality:
 *          ENABLE_MMX, ENABLE_3DNOW, ENABLE_SSE flags must be set to
 *          to compile the optimized sections of the code. In both cases the 
 *          X86_xxxxxx functions will compile.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"


////////////////////////////////////////////////////////////////////////////////////
// 3DNow! optimizations

#ifdef ENABLE_X86_AMD

// Convert integer mix to floating-point
static void AMD_StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc)
//-----------------------------------------------------------------------------------------------------------
{
	_asm {
	movd mm0, _i2fc
	mov edx, pSrc
	mov edi, pOut1
	mov ebx, pOut2
	mov ecx, nCount
	punpckldq mm0, mm0
	inc ecx
	shr ecx, 1
mainloop:
	movq mm1, qword ptr [edx]
	movq mm2, qword ptr [edx+8]
	add edi, 8
	pi2fd mm1, mm1
	pi2fd mm2, mm2
	add ebx, 8
	pfmul mm1, mm0
	pfmul mm2, mm0
	add edx, 16
	movq mm3, mm1
	punpckldq mm3, mm2
	punpckhdq mm1, mm2
	dec ecx
	movq qword ptr [edi-8], mm3
	movq qword ptr [ebx-8], mm1
	jnz mainloop
	emms
	}
}

static void AMD_FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount, const float _f2ic)
//---------------------------------------------------------------------------------------------------------------
{
	_asm {
	movd mm0, _f2ic
	mov eax, pIn1
	mov ebx, pIn2
	mov edx, pOut
	mov ecx, nCount
	punpckldq mm0, mm0
	inc ecx
	shr ecx, 1
	sub edx, 16
mainloop:
	movq mm1, [eax]
	movq mm2, [ebx]
	add edx, 16
	movq mm3, mm1
	punpckldq mm1, mm2
	punpckhdq mm3, mm2
	add eax, 8
	pfmul mm1, mm0
	pfmul mm3, mm0
	add ebx, 8
	pf2id mm1, mm1
	pf2id mm3, mm3
	dec ecx
	movq qword ptr [edx], mm1
	movq qword ptr [edx+8], mm3
	jnz mainloop
	emms
	}
}


static void AMD_FloatToMonoMix(const float *pIn, int *pOut, UINT nCount, const float _f2ic)
//-----------------------------------------------------------------------------------------
{
	_asm {
	movd mm0, _f2ic
	mov eax, pIn
	mov edx, pOut
	mov ecx, nCount
	punpckldq mm0, mm0
	add ecx, 3
	shr ecx, 2
	sub edx, 16
mainloop:
	movq mm1, [eax]
	movq mm2, [eax+8]
	add edx, 16
	pfmul mm1, mm0
	pfmul mm2, mm0
	add eax, 16
	pf2id mm1, mm1
	pf2id mm2, mm2
	dec ecx
	movq qword ptr [edx], mm1
	movq qword ptr [edx+8], mm2
	jnz mainloop
	emms
	}
}


static void AMD_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc)
//------------------------------------------------------------------------------------------
{
	_asm {
	movd mm0, _i2fc
	mov eax, pSrc
	mov edx, pOut
	mov ecx, nCount
	punpckldq mm0, mm0
	add ecx, 3
	shr ecx, 2
	sub edx, 16
mainloop:
	movq mm1, qword ptr [eax]
	movq mm2, qword ptr [eax+8]
	add edx, 16
	pi2fd mm1, mm1
	pi2fd mm2, mm2
	add eax, 16
	pfmul mm1, mm0
	pfmul mm2, mm0
	dec ecx
	movq qword ptr [edx], mm1
	movq qword ptr [edx+8], mm2
	jnz mainloop
	emms
	}
}

#endif // ENABLE_X86_AMD

///////////////////////////////////////////////////////////////////////////////////////
// SSE Optimizations

#ifdef ENABLE_SSE

static void SSE_StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc)
//-----------------------------------------------------------------------------------------------------------
{
	_asm {
	movss xmm0, _i2fc
	mov edx, pSrc
	mov eax, pOut1
	mov ebx, pOut2
	mov ecx, nCount
	shufps xmm0, xmm0, 0x00
	xorps xmm1, xmm1
	xorps xmm2, xmm2
	inc ecx
	shr ecx, 1
mainloop:
	cvtpi2ps xmm1, [edx]
	cvtpi2ps xmm2, [edx+8]
	add eax, 8
	add ebx, 8
	movlhps xmm1, xmm2
	mulps xmm1, xmm0
	add edx, 16
	shufps xmm1, xmm1, 0xD8
	dec ecx
	movlps qword ptr [eax-8], xmm1
	movhps qword ptr [ebx-8], xmm1
	jnz mainloop
	}
}


static void SSE_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc)
//------------------------------------------------------------------------------------------
{
	_asm {
	movss xmm0, _i2fc
	mov edx, pSrc
	mov eax, pOut
	mov ecx, nCount
	shufps xmm0, xmm0, 0x00
	xorps xmm1, xmm1
	xorps xmm2, xmm2
	add ecx, 3
	shr ecx, 2
mainloop:
	cvtpi2ps xmm1, [edx]
	cvtpi2ps xmm2, [edx+8]
	add eax, 16
	movlhps xmm1, xmm2
	mulps xmm1, xmm0
	add edx, 16
	dec ecx
	movups [eax-16], xmm1
	jnz mainloop
	}
}

#endif // ENABLE_SSE



#ifdef ENABLE_X86

// Convert floating-point mix to integer

static void X86_FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount, const float _f2ic)
//---------------------------------------------------------------------------------------------------------------
{
	_asm {
	mov esi, pIn1
	mov ebx, pIn2
	mov edi, pOut
	mov ecx, nCount
	fld _f2ic
mainloop:
	fld dword ptr [ebx]
	add edi, 8
	fld dword ptr [esi]
	add ebx, 4
	add esi, 4
	fmul st(0), st(2)
	fistp dword ptr [edi-8]
	fmul st(0), st(1)
	fistp dword ptr [edi-4]
	dec ecx
	jnz mainloop
	fstp st(0)
	}
}


// Convert integer mix to floating-point

static void X86_StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc)
//-----------------------------------------------------------------------------------------------------------
{
	_asm {
	mov esi, pSrc
	mov edi, pOut1
	mov ebx, pOut2
	mov ecx, nCount
	fld _i2fc
mainloop:
	fild dword ptr [esi]
	fild dword ptr [esi+4]
	add ebx, 4
	add edi, 4
	fmul st(0), st(2)
	add esi, 8
	fstp dword ptr [ebx-4]
	fmul st(0), st(1)
	fstp dword ptr [edi-4]
	dec ecx
	jnz mainloop
	fstp st(0)
	}
}


static void X86_FloatToMonoMix(const float *pIn, int *pOut, UINT nCount, const float _f2ic)
//-----------------------------------------------------------------------------------------
{
	_asm {
	mov edx, pIn
	mov eax, pOut
	mov ecx, nCount
	fld _f2ic
	sub eax, 4
R2I_Loop:
	fld DWORD PTR [edx]
	add eax, 4
	fmul ST(0), ST(1)
	dec ecx
	lea edx, [edx+4]
	fistp DWORD PTR [eax]
	jnz R2I_Loop
	fstp st(0)
	}
}


static void X86_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc)
//------------------------------------------------------------------------------------------
{
	_asm {
	mov edx, pOut
	mov eax, pSrc
	mov ecx, nCount
	fld _i2fc
	sub edx, 4
I2R_Loop:
	fild DWORD PTR [eax]
	add edx, 4
	fmul ST(0), ST(1)
	dec ecx
	lea eax, [eax+4]
	fstp DWORD PTR [edx]
	jnz I2R_Loop
	fstp st(0)
	}
}

#endif // ENABLE_X86



static void C_FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount, const float _f2ic)
//-------------------------------------------------------------------------------------------------------------
{
	for(UINT i=0; i<nCount; ++i)
	{
		*pOut++ = (int)(*pIn1++ * _f2ic);
		*pOut++ = (int)(*pIn2++ * _f2ic);
	}
}


static void C_StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc)
//---------------------------------------------------------------------------------------------------------
{
	for(UINT i=0; i<nCount; ++i)
	{
		*pOut1++ = *pSrc++ * _i2fc;
		*pOut2++ = *pSrc++ * _i2fc;
	}
}


static void C_FloatToMonoMix(const float *pIn, int *pOut, UINT nCount, const float _f2ic)
//---------------------------------------------------------------------------------------
{
	for(UINT i=0; i<nCount; ++i)
	{
		*pOut++ = (int)(*pIn++ * _f2ic);
	}
}


static void C_MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc)
//----------------------------------------------------------------------------------------
{
	for(UINT i=0; i<nCount; ++i)
	{
		*pOut++ = *pSrc++ * _i2fc;
	}
}



void StereoMixToFloat(const int *pSrc, float *pOut1, float *pOut2, UINT nCount, const float _i2fc)
{

	#ifdef ENABLE_SSE
		if(GetProcSupport() & PROCSUPPORT_SSE)
		{
			SSE_StereoMixToFloat(pSrc, pOut1, pOut2, nCount, _i2fc);
			return;
		}
	#endif // ENABLE_SSE
	#ifdef ENABLE_X86_AMD
		if(GetProcSupport() & PROCSUPPORT_AMD_3DNOW)
		{
			AMD_StereoMixToFloat(pSrc, pOut1, pOut2, nCount, _i2fc);
			return;
		}
	#endif // ENABLE_X86_AMD

	#ifdef ENABLE_X86
		X86_StereoMixToFloat(pSrc, pOut1, pOut2, nCount, _i2fc);
	#else // !ENABLE_X86
		C_StereoMixToFloat(pSrc, pOut1, pOut2, nCount, _i2fc);
	#endif // ENABLE_X86

}


void FloatToStereoMix(const float *pIn1, const float *pIn2, int *pOut, UINT nCount, const float _f2ic)
{

	#ifdef ENABLE_X86_AMD
		if(GetProcSupport() & PROCSUPPORT_AMD_3DNOW)
		{
			AMD_FloatToStereoMix(pIn1, pIn2, pOut, nCount, _f2ic);
			return;
		}
	#endif // ENABLE_X86_AMD

	#ifdef ENABLE_X86
		X86_FloatToStereoMix(pIn1, pIn2, pOut, nCount, _f2ic);
	#else // !ENABLE_X86
		C_FloatToStereoMix(pIn1, pIn2, pOut, nCount, _f2ic);
	#endif // ENABLE_X86

}


void MonoMixToFloat(const int *pSrc, float *pOut, UINT nCount, const float _i2fc)
{

	#ifdef ENABLE_SSE
		if(GetProcSupport() & PROCSUPPORT_SSE)
		{
			SSE_MonoMixToFloat(pSrc, pOut, nCount, _i2fc);
			return;
		}
	#endif // ENABLE_SSE
	#ifdef ENABLE_X86_AMD
		if(GetProcSupport() & PROCSUPPORT_AMD_3DNOW)
		{
			AMD_MonoMixToFloat(pSrc, pOut, nCount, _i2fc);
			return;
		}
	#endif // ENABLE_X86_AMD

	#ifdef ENABLE_X86
		X86_MonoMixToFloat(pSrc, pOut, nCount, _i2fc);
	#else // !ENABLE_X86
		C_MonoMixToFloat(pSrc, pOut, nCount, _i2fc);
	#endif // ENABLE_X86

}


void FloatToMonoMix(const float *pIn, int *pOut, UINT nCount, const float _f2ic)
{

	#ifdef ENABLE_X86_AMD
		if(GetProcSupport() & PROCSUPPORT_AMD_3DNOW)
		{
			AMD_FloatToMonoMix(pIn, pOut, nCount, _f2ic);
			return;
		}
	#endif // ENABLE_X86_AMD

	#ifdef ENABLE_X86
		X86_FloatToMonoMix(pIn, pOut, nCount, _f2ic);
	#else // !ENABLE_X86
		C_FloatToMonoMix(pIn, pOut, nCount, _f2ic);
	#endif // ENABLE_X86

}



//////////////////////////////////////////////////////////////////////////////////////////


void InitMixBuffer(int *pBuffer, UINT nSamples)
//---------------------------------------------
{
	memset(pBuffer, 0, nSamples * sizeof(int));
}

#if MPT_COMPILER_MSVC
#pragma warning(disable:4731) // ebp modified
#endif

#ifdef ENABLE_X86
static void X86_InterleaveFrontRear(int *pFrontBuf, int *pRearBuf, DWORD nFrames)
//-------------------------------------------------------------------------------
{
	_asm {
	mov ecx, nFrames	// ecx = framecount
	mov esi, pFrontBuf	// esi = front buffer
	mov edi, pRearBuf	// edi = rear buffer
	lea esi, [esi+ecx*8]	// esi = &front[N*2]
	lea edi, [edi+ecx*8]	// edi = &rear[N*2]
	lea ebx, [esi+ecx*8]	// ebx = &front[N*4]
	push ebp
interleaveloop:
	mov eax, dword ptr [esi-8]
	mov edx, dword ptr [esi-4]
	sub ebx, 16
	mov ebp, dword ptr [edi-8]
	mov dword ptr [ebx], eax
	mov dword ptr [ebx+4], edx
	mov eax, dword ptr [edi-4]
	sub esi, 8
	sub edi, 8
	dec ecx
	mov dword ptr [ebx+8], ebp
	mov dword ptr [ebx+12], eax
	jnz interleaveloop
	pop ebp
	}
}
#endif

static void C_InterleaveFrontRear(int *pFrontBuf, int *pRearBuf, DWORD nFrames)
//-----------------------------------------------------------------------------
{
	// copy backwards as we are writing back into FrontBuf
	for(int i=nFrames-1; i>=0; i--)
	{
		pFrontBuf[i*4+3] = pRearBuf[i*2+1];
		pFrontBuf[i*4+2] = pRearBuf[i*2+0];
		pFrontBuf[i*4+1] = pFrontBuf[i*2+1];
		pFrontBuf[i*4+0] = pFrontBuf[i*2+0];
	}
}

void InterleaveFrontRear(int *pFrontBuf, int *pRearBuf, DWORD nFrames)
//--------------------------------------------------------------------
{
	#ifdef ENABLE_X86
		X86_InterleaveFrontRear(pFrontBuf, pRearBuf, nFrames);
	#else
		C_InterleaveFrontRear(pFrontBuf, pRearBuf, nFrames);
	#endif
}


#ifdef ENABLE_X86
static void X86_MonoFromStereo(int *pMixBuf, UINT nSamples)
//---------------------------------------------------------
{
	_asm {
	mov ecx, nSamples
	mov esi, pMixBuf
	mov edi, esi
stloop:
	mov eax, dword ptr [esi]
	mov edx, dword ptr [esi+4]
	add edi, 4
	add esi, 8
	add eax, edx
	sar eax, 1
	dec ecx
	mov dword ptr [edi-4], eax
	jnz stloop
	}
}
#endif

static void C_MonoFromStereo(int *pMixBuf, UINT nSamples)
//-------------------------------------------------------
{
	for(UINT i=0; i<nSamples; ++i)
	{
		pMixBuf[i] = (pMixBuf[i*2] + pMixBuf[i*2+1]) / 2;
	}
}

void MonoFromStereo(int *pMixBuf, UINT nSamples)
//----------------------------------------------
{
	#ifdef ENABLE_X86
		X86_MonoFromStereo(pMixBuf, nSamples);
	#else
		C_MonoFromStereo(pMixBuf, nSamples);
	#endif
}


#define OFSDECAYSHIFT	8
#define OFSDECAYMASK	0xFF


#ifdef ENABLE_X86
static void X86_StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs)
//-----------------------------------------------------------------------------------
{
	_asm {
	mov edi, pBuffer
	mov ecx, nSamples
	mov eax, lpROfs
	mov edx, lpLOfs
	mov eax, [eax]
	mov edx, [edx]
	or ecx, ecx
	jz fill_loop
	mov ebx, eax
	or ebx, edx
	jz fill_loop
ofsloop:
	mov ebx, eax
	mov esi, edx
	neg ebx
	neg esi
	sar ebx, 31
	sar esi, 31
	and ebx, OFSDECAYMASK
	and esi, OFSDECAYMASK
	add ebx, eax
	add esi, edx
	sar ebx, OFSDECAYSHIFT
	sar esi, OFSDECAYSHIFT
	sub eax, ebx
	sub edx, esi
	mov ebx, eax
	or ebx, edx
	jz fill_loop
	add edi, 8
	dec ecx
	mov [edi-8], eax
	mov [edi-4], edx
	jnz ofsloop
fill_loop:
	mov ebx, ecx
	and ebx, 3
	jz fill4x
fill1x:
	mov [edi], eax
	mov [edi+4], edx
	add edi, 8
	dec ebx
	jnz fill1x
fill4x:
	shr ecx, 2
	or ecx, ecx
	jz done
fill4xloop:
	mov [edi], eax
	mov [edi+4], edx
	mov [edi+8], eax
	mov [edi+12], edx
	add edi, 8*4
	dec ecx
	mov [edi-16], eax
	mov [edi-12], edx
	mov [edi-8], eax
	mov [edi-4], edx
	jnz fill4xloop
done:
	mov esi, lpROfs
	mov edi, lpLOfs
	mov [esi], eax
	mov [edi], edx
	}
}
#endif

// c implementation taken from libmodplug
static void C_StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs)
//---------------------------------------------------------------------------------
{
	int rofs = *lpROfs;
	int lofs = *lpLOfs;

	if ((!rofs) && (!lofs))
	{
		InitMixBuffer(pBuffer, nSamples*2);
		return;
	}
	for (UINT i=0; i<nSamples; i++)
	{
		int x_r = (rofs + (((-rofs)>>31) & OFSDECAYMASK)) >> OFSDECAYSHIFT;
		int x_l = (lofs + (((-lofs)>>31) & OFSDECAYMASK)) >> OFSDECAYSHIFT;
		rofs -= x_r;
		lofs -= x_l;
		pBuffer[i*2] = x_r;
		pBuffer[i*2+1] = x_l;
	}
	*lpROfs = rofs;
	*lpLOfs = lofs;
}

void StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs)
//------------------------------------------------------------------------
{
	#ifdef ENABLE_X86
		X86_StereoFill(pBuffer, nSamples, lpROfs, lpLOfs);
	#else
		C_StereoFill(pBuffer, nSamples, lpROfs, lpLOfs);
	#endif
}


#ifdef ENABLE_X86
typedef ModChannel ModChannel_;
static void X86_EndChannelOfs(ModChannel *pChannel, int *pBuffer, UINT nSamples)
//------------------------------------------------------------------------------
{
	_asm {
	mov esi, pChannel
	mov edi, pBuffer
	mov ecx, nSamples
	mov eax, dword ptr [esi+ModChannel_.nROfs]
	mov edx, dword ptr [esi+ModChannel_.nLOfs]
	or ecx, ecx
	jz brkloop
ofsloop:
	mov ebx, eax
	mov esi, edx
	neg ebx
	neg esi
	sar ebx, 31
	sar esi, 31
	and ebx, OFSDECAYMASK
	and esi, OFSDECAYMASK
	add ebx, eax
	add esi, edx
	sar ebx, OFSDECAYSHIFT
	sar esi, OFSDECAYSHIFT
	sub eax, ebx
	sub edx, esi
	mov ebx, eax
	add dword ptr [edi], eax
	add dword ptr [edi+4], edx
	or ebx, edx
	jz brkloop
	add edi, 8
	dec ecx
	jnz ofsloop
brkloop:
	mov esi, pChannel
	mov dword ptr [esi+ModChannel_.nROfs], eax
	mov dword ptr [esi+ModChannel_.nLOfs], edx
	}
}
#endif

// c implementation taken from libmodplug
static void C_EndChannelOfs(ModChannel *pChannel, int *pBuffer, UINT nSamples)
//----------------------------------------------------------------------------
{

	int rofs = pChannel->nROfs;
	int lofs = pChannel->nLOfs;

	if ((!rofs) && (!lofs)) return;
	for (UINT i=0; i<nSamples; i++)
	{
		int x_r = (rofs + (((-rofs)>>31) & OFSDECAYMASK)) >> OFSDECAYSHIFT;
		int x_l = (lofs + (((-lofs)>>31) & OFSDECAYMASK)) >> OFSDECAYSHIFT;
		rofs -= x_r;
		lofs -= x_l;
		pBuffer[i*2] += x_r;
		pBuffer[i*2+1] += x_l;
	}
	pChannel->nROfs = rofs;
	pChannel->nLOfs = lofs;
}

void EndChannelOfs(ModChannel *pChannel, int *pBuffer, UINT nSamples)
//-------------------------------------------------------------------
{
	#ifdef ENABLE_X86
		X86_EndChannelOfs(pChannel, pBuffer, nSamples);
	#else
		C_EndChannelOfs(pChannel, pBuffer, nSamples);
	#endif
}


#ifndef MODPLUG_TRACKER

void ApplyGain(int *soundBuffer, std::size_t channels, std::size_t countChunk, int32 gainFactor16_16)
//---------------------------------------------------------------------------------------------------
{
	if(gainFactor16_16 == (1<<16))
	{
		// nothing to do, gain == +/- 0dB
		return; 
	}
	// no clipping prevention is done here
	int * buf = soundBuffer;
	for(std::size_t i=0; i<countChunk*channels; ++i)
	{
		*buf = Util::muldiv(*buf, gainFactor16_16, 1<<16);
		buf++;
	}
}

static void ApplyGain(float *beg, float *end, float factor)
//---------------------------------------------------------
{
	for(float *it = beg; it != end; ++it)
	{
		*it *= factor;
	}
}

void ApplyGain(float * outputBuffer, float * const *outputBuffers, std::size_t offset, std::size_t channels, std::size_t countChunk, float gainFactor)
//----------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(gainFactor == 1.0f)
	{
		// nothing to do, gain == +/- 0dB
		return;
	}
	if(outputBuffer)
	{
		ApplyGain(
			outputBuffer + (channels * offset),
			outputBuffer + (channels * (offset + countChunk)),
			gainFactor);
	}
	if(outputBuffers)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			ApplyGain(
				outputBuffers[channel] + offset,
				outputBuffers[channel] + offset + countChunk,
				gainFactor);
		}
	}
}

#endif // !MODPLUG_TRACKER
