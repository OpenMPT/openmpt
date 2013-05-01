/*
 * Waveform.cpp
 * ------------
 * Purpose: Common audio buffer conversion functions.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "sndfile.h"

#ifdef ENABLE_X86

#pragma warning(disable:4100)
#pragma warning(disable:4731)

static void X86_Normalize24BitBuffer(LPBYTE pbuffer, UINT dwSize, DWORD lmax24, int *poutput)
{
	_asm {
	mov esi, pbuffer	// esi = edi = pbuffer
	mov edx, dwSize		// edx = dwSize
	mov ebx, lmax24		// ebx = max
	mov edi, poutput	// edi = output 32-bit buffer
	push ebp
	mov ebp, edx		// ebp = dwSize
	cmp ebx, 256
	jg normloop
	mov ebx, 256
normloop:
	movsx eax, byte ptr [esi+2]
	movzx edx, byte ptr [esi+1]
	shl eax, 8
	or eax, edx
	movzx edx, byte ptr [esi]
	shl eax, 8
	or eax, edx		// eax = 24-bit sample
	mov edx, 1 << (31-MIXING_ATTENUATION)
	add esi, 3
	imul edx
	add edi, 4
	idiv ebx		// eax = 28-bit normalized sample
	dec ebp
	mov dword ptr [edi-4], eax
	jnz normloop
	pop ebp
	}
}

#pragma warning(default:4100)

#endif

#ifdef ENABLE_X86
extern void X86_Dither(int *pBuffer, UINT nSamples, UINT nBits);
#endif
extern DWORD Convert32To8(LPVOID lpBuffer, int *, DWORD nSamples);
extern DWORD Convert32To16(LPVOID lpBuffer, int *, DWORD nSamples);
extern DWORD Convert32To24(LPVOID lpBuffer, int *, DWORD nSamples);

UINT CSoundFile::Normalize24BitBuffer(LPBYTE pbuffer, UINT dwSize, DWORD lmax24, DWORD dwByteInc)
//-----------------------------------------------------------------------------------------------
{
#ifdef ENABLE_X86
	int * const tempbuf = MixSoundBuffer;
	int n = dwSize / 3;
	while (n > 0)
	{
		int nbuf = (n > MIXBUFFERSIZE * 2) ? MIXBUFFERSIZE * 2 : n;
		X86_Normalize24BitBuffer(pbuffer, nbuf, lmax24, tempbuf);
		X86_Dither(tempbuf, nbuf, 8 * dwByteInc);
		switch(dwByteInc)
		{
		case 2:		Convert32To16(pbuffer, tempbuf, nbuf); break;
		case 3:		Convert32To24(pbuffer, tempbuf, nbuf); break;
		default:	Convert32To8(pbuffer, tempbuf, nbuf); break;
		}
		n -= nbuf;
		pbuffer += dwByteInc * nbuf;
	}
	return (dwSize/3) * dwByteInc;
#else
	LONG lMax = (lmax24 / 128) + 1;
	UINT i = 0;
	for (UINT j=0; j<dwSize; j+=3, i+=dwByteInc)
	{
		LONG l = ((((pbuffer[j+2] << 8) + pbuffer[j+1]) << 8) + pbuffer[j]) << 8;
		l /= lMax;
		if (dwByteInc > 1)
		{
			pbuffer[i] = (BYTE)(l & 0xFF);
			pbuffer[i+1] = (BYTE)(l >> 8);
		} else
		{
			pbuffer[i] = (BYTE)((l + 0x8000) >> 8);
		}
	}
	return i;
#endif
}
