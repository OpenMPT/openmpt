/*
 * mmcmp.cpp
 * ---------
 * Purpose: Handling of compressed modules (XPK, PowerPack PP20)
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"

//#define MMCMP_LOG


BOOL XPK_Unpack(LPCBYTE *ppMemFile, LPDWORD pdwMemLength);
BOOL PP20_Unpack(LPCBYTE *ppMemFile, LPDWORD pdwMemLength);

typedef struct MMCMPFILEHEADER
{
	DWORD id_ziRC;	// "ziRC"
	DWORD id_ONia;	// "ONia"
	WORD hdrsize;
} MMCMPFILEHEADER, *LPMMCMPFILEHEADER;

typedef struct MMCMPHEADER
{
	WORD version;
	WORD nblocks;
	DWORD filesize;
	DWORD blktable;
	BYTE glb_comp;
	BYTE fmt_comp;
} MMCMPHEADER, *LPMMCMPHEADER;

typedef struct MMCMPBLOCK
{
	DWORD unpk_size;
	DWORD pk_size;
	DWORD xor_chk;
	WORD sub_blk;
	WORD flags;
	WORD tt_entries;
	WORD num_bits;
} MMCMPBLOCK, *LPMMCMPBLOCK;

typedef struct MMCMPSUBBLOCK
{
	DWORD unpk_pos;
	DWORD unpk_size;
} MMCMPSUBBLOCK, *LPMMCMPSUBBLOCK;

#define MMCMP_COMP		0x0001
#define MMCMP_DELTA		0x0002
#define MMCMP_16BIT		0x0004
#define MMCMP_STEREO	0x0100
#define MMCMP_ABS16		0x0200
#define MMCMP_ENDIAN	0x0400

typedef struct MMCMPBITBUFFER
{
	UINT bitcount;
	DWORD bitbuffer;
	LPCBYTE pSrc;
	LPCBYTE pEnd;

	DWORD GetBits(UINT nBits);
} MMCMPBITBUFFER;


DWORD MMCMPBITBUFFER::GetBits(UINT nBits)
//---------------------------------------
{
	DWORD d;
	if (!nBits) return 0;
	while (bitcount < 24)
	{
		bitbuffer |= ((pSrc < pEnd) ? *pSrc++ : 0) << bitcount;
		bitcount += 8;
	}
	d = bitbuffer & ((1 << nBits) - 1);
	bitbuffer >>= nBits;
	bitcount -= nBits;
	return d;
}

const DWORD MMCMP8BitCommands[8] =
{
	0x01, 0x03,	0x07, 0x0F,	0x1E, 0x3C,	0x78, 0xF8
};

const UINT MMCMP8BitFetch[8] =
{
	3, 3, 3, 3, 2, 1, 0, 0
};

const DWORD MMCMP16BitCommands[16] =
{
	0x01, 0x03,	0x07, 0x0F,	0x1E, 0x3C,	0x78, 0xF0,
	0x1F0, 0x3F0, 0x7F0, 0xFF0, 0x1FF0, 0x3FF0, 0x7FF0, 0xFFF0
};

const UINT MMCMP16BitFetch[16] =
{
	4, 4, 4, 4, 3, 2, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};


BOOL MMCMP_Unpack(LPCBYTE *ppMemFile, LPDWORD pdwMemLength)
//---------------------------------------------------------
{
	DWORD dwMemLength = *pdwMemLength;
	LPCBYTE lpMemFile = *ppMemFile;
	LPBYTE pBuffer;
	LPMMCMPFILEHEADER pmfh = (LPMMCMPFILEHEADER)(lpMemFile);
	LPMMCMPHEADER pmmh = (LPMMCMPHEADER)(lpMemFile+10);
	LPDWORD pblk_table;
	DWORD dwFileSize;

	if ((XPK_Unpack(ppMemFile, pdwMemLength))
	 || (PP20_Unpack(ppMemFile, pdwMemLength)))
	{
		return TRUE;
	}
	if ((dwMemLength < 256) || (!pmfh) || (pmfh->id_ziRC != 0x4352697A) || (pmfh->id_ONia != 0x61694e4f) || (pmfh->hdrsize < 14)
	 || (!pmmh->nblocks) || (pmmh->filesize < 16) || (pmmh->filesize > 0x8000000)
	 || (pmmh->blktable >= dwMemLength) || (pmmh->blktable + 4*pmmh->nblocks > dwMemLength)) return FALSE;
	dwFileSize = pmmh->filesize;
	if ((pBuffer = (LPBYTE)calloc(1, (dwFileSize + 31) & ~15)) == NULL) return FALSE;
	pblk_table = (LPDWORD)(lpMemFile+pmmh->blktable);
	for (UINT nBlock=0; nBlock<pmmh->nblocks; nBlock++)
	{
		DWORD dwMemPos = pblk_table[nBlock];
		LPMMCMPBLOCK pblk = (LPMMCMPBLOCK)(lpMemFile+dwMemPos);
		LPMMCMPSUBBLOCK psubblk = (LPMMCMPSUBBLOCK)(lpMemFile+dwMemPos+20);

		if ((dwMemPos + 20 >= dwMemLength) || (dwMemPos + 20 + pblk->sub_blk*8 >= dwMemLength)) break;
		dwMemPos += 20 + pblk->sub_blk*8;
#ifdef MMCMP_LOG
		Log("block %d: flags=%04X sub_blocks=%d", nBlock, (UINT)pblk->flags, (UINT)pblk->sub_blk);
		Log(" pksize=%d unpksize=%d", pblk->pk_size, pblk->unpk_size);
		Log(" tt_entries=%d num_bits=%d\n", pblk->tt_entries, pblk->num_bits);
#endif
		// Data is not packed
		if (!(pblk->flags & MMCMP_COMP))
		{
			for (UINT i=0; i<pblk->sub_blk; i++)
			{
				if ((psubblk->unpk_pos >= dwFileSize) ||
					(psubblk->unpk_size >= dwFileSize) ||
					(psubblk->unpk_size > dwFileSize - psubblk->unpk_pos)) break;
#ifdef MMCMP_LOG
				Log("  Unpacked sub-block %d: offset %d, size=%d\n", i, psubblk->unpk_pos, psubblk->unpk_size);
#endif
				memcpy(pBuffer+psubblk->unpk_pos, lpMemFile+dwMemPos, psubblk->unpk_size);
				dwMemPos += psubblk->unpk_size;
				psubblk++;
			}
		} else
		// Data is 16-bit packed
		if (pblk->flags & MMCMP_16BIT)
		{
			MMCMPBITBUFFER bb;
			uint16 *pDest = (uint16 *)(pBuffer + psubblk->unpk_pos);
			uint32 dwSize = psubblk->unpk_size >> 1;
			uint32 dwPos = 0;
			uint32 numbits = pblk->num_bits;
			uint32 subblk = 0, oldval = 0;

#ifdef MMCMP_LOG
			Log("  16-bit block: pos=%d size=%d ", psubblk->unpk_pos, psubblk->unpk_size);
			if (pblk->flags & MMCMP_DELTA) Log("DELTA ");
			if (pblk->flags & MMCMP_ABS16) Log("ABS16 ");
			Log("\n");
#endif
			bb.bitcount = 0;
			bb.bitbuffer = 0;
			bb.pSrc = lpMemFile+dwMemPos+pblk->tt_entries;
			bb.pEnd = lpMemFile+dwMemPos+pblk->pk_size;
			while (subblk < pblk->sub_blk)
			{
				uint32 newval = 0x10000;
				uint32 d = bb.GetBits(numbits+1);

				if (d >= MMCMP16BitCommands[numbits])
				{
					uint32 nFetch = MMCMP16BitFetch[numbits];
					uint32 newbits = bb.GetBits(nFetch) + ((d - MMCMP16BitCommands[numbits]) << nFetch);
					if (newbits != numbits)
					{
						numbits = newbits & 0x0F;
					} else
					{
						if ((d = bb.GetBits(4)) == 0x0F)
						{
							if (bb.GetBits(1)) break;
							newval = 0xFFFF;
						} else
						{
							newval = 0xFFF0 + d;
						}
					}
				} else
				{
					newval = d;
				}
				if (newval < 0x10000)
				{
					newval = (newval & 1) ? (uint32)(-(int32)((newval+1) >> 1)) : (uint32)(newval >> 1);
					if (pblk->flags & MMCMP_DELTA)
					{
						newval += oldval;
						oldval = newval;
					} else
					if (!(pblk->flags & MMCMP_ABS16))
					{
						newval ^= 0x8000;
					}
					pDest[dwPos++] = (uint16)newval;
				}
				if (dwPos >= dwSize)
				{
					subblk++;
					dwPos = 0;
					dwSize = psubblk[subblk].unpk_size >> 1;
					pDest = (uint16 *)(pBuffer + psubblk[subblk].unpk_pos);
				}
			}
		} else
		// Data is 8-bit packed
		{
			MMCMPBITBUFFER bb;
			LPBYTE pDest = pBuffer + psubblk->unpk_pos;
			uint32 dwSize = psubblk->unpk_size;
			uint32 dwPos = 0;
			uint32 numbits = pblk->num_bits;
			uint32 subblk = 0, oldval = 0;
			LPCBYTE ptable = lpMemFile+dwMemPos;

			bb.bitcount = 0;
			bb.bitbuffer = 0;
			bb.pSrc = lpMemFile+dwMemPos+pblk->tt_entries;
			bb.pEnd = lpMemFile+dwMemPos+pblk->pk_size;
			while (subblk < pblk->sub_blk)
			{
				uint32 newval = 0x100;
				uint32 d = bb.GetBits(numbits+1);

				if (d >= MMCMP8BitCommands[numbits])
				{
					uint32 nFetch = MMCMP8BitFetch[numbits];
					uint32 newbits = bb.GetBits(nFetch) + ((d - MMCMP8BitCommands[numbits]) << nFetch);
					if (newbits != numbits)
					{
						numbits = newbits & 0x07;
					} else
					{
						if ((d = bb.GetBits(3)) == 7)
						{
							if (bb.GetBits(1)) break;
							newval = 0xFF;
						} else
						{
							newval = 0xF8 + d;
						}
					}
				} else
				{
					newval = d;
				}
				if (newval < 0x100)
				{
					int n = ptable[newval];
					if (pblk->flags & MMCMP_DELTA)
					{
						n += oldval;
						oldval = n;
					}
					pDest[dwPos++] = (uint8)n;
				}
				if (dwPos >= dwSize)
				{
					subblk++;
					dwPos = 0;
					dwSize = psubblk[subblk].unpk_size;
					pDest = pBuffer + psubblk[subblk].unpk_pos;
				}
			}
		}
	}
	*ppMemFile = pBuffer;
	*pdwMemLength = dwFileSize;
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// XPK unpacker
//

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

typedef struct PACKED _XPKFILEHEADER
{
	DWORD dwXPKF;
	DWORD dwSrcLen;
	DWORD dwSQSH;
	DWORD dwDstLen;
	CHAR szName[16];
	DWORD dwReserved;
} XPKFILEHEADER, *PXPKFILEHEADER;

STATIC_ASSERT(sizeof(XPKFILEHEADER) == 36);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


static int bfextu(const BYTE *p,int bo,int bc)
{
  int r;
  
  p += bo / 8;
  r = *(p++);
  r <<= 8;
  r |= *(p++);
  r <<= 8;
  r |= *p;
  r <<= bo % 8;
  r &= 0xffffff;
  r >>= 24 - bc;

  return r;
}

static int bfexts(const BYTE *p,int bo,int bc)
{
  int r;
  
  p += bo / 8;
  r = *(p++);
  r <<= 8;
  r |= *(p++);
  r <<= 8;
  r |= *p;
  r <<= (bo % 8) + 8;
  r >>= 32 - bc;

  return r;
}


void XPK_DoUnpack(const BYTE *src, UINT, BYTE *dst, int len)
{
	static BYTE xpk_table[] = { 2,3,4,5,6,7,8,0,3,2,4,5,6,7,8,0,4,3,5,2,6,7,8,0,5,4,
        6,2,3,7,8,0,6,5,7,2,3,4,8,0,7,6,8,2,3,4,5,0,8,7,6,2,3,4,5,0 };
	int d0,d1,d2,d3,d4,d5,d6,a2,a5;
	int cp, cup1, type;
	const BYTE *c;
	BYTE *phist;
	BYTE *dstmax = dst + len;
   
	c = src;
	while (len > 0)
	{
		type = c[0];
		cp = (c[4]<<8) | (c[5]); // packed
		cup1 = (c[6]<<8) | (c[7]); // unpacked
		//Log("  packed=%6d unpacked=%6d bytes left=%d dst=%08X(%d)\n", cp, cup1, len, dst, dst);
		c += 8;
		src = c+2;
		if (type == 0)
		{
			// RAW chunk
			memcpy(dst,c,cp);
			dst+=cp;
			c+=cp;
			len -= cp;
			continue;
		}
	  
		if (type != 1)
		{
		#ifdef MMCMP_LOG
			Log("Invalid XPK type! (%d bytes left)\n", len);
		#endif
			break;
		}
		len -= cup1;
		cp = (cp + 3) & 0xfffc;
		c += cp;

		d0 = d1 = d2 = a2 = 0;
		d3 = *(src++);
		*dst = (BYTE)d3;
		if (dst < dstmax) dst++;
		cup1--;

		while (cup1 > 0)
		{
			if (d1 >= 8) goto l6dc;
			if (bfextu(src,d0,1)) goto l75a;
			d0 += 1;
			d5 = 0;
			d6 = 8;
			goto l734;
  
		l6dc:	
			if (bfextu(src,d0,1)) goto l726;
			d0 += 1;
			if (! bfextu(src,d0,1)) goto l75a;
			d0 += 1;
			if (bfextu(src,d0,1)) goto l6f6;
			d6 = 2;
			goto l708;

		l6f6:	
			d0 += 1;
			if (!bfextu(src,d0,1)) goto l706;
			d6 = bfextu(src,d0,3);
			d0 += 3;
			goto l70a;
  
		l706:	
			d6 = 3;
		l708:	
			d0 += 1;
		l70a:	
			d6 = xpk_table[(8*a2) + d6 -17];
			if (d6 != 8) goto l730;
		l718:	
			if (d2 >= 20)
			{
				d5 = 1;
				goto l732;
			}
			d5 = 0;
			goto l734;

		l726:	
			d0 += 1;
			d6 = 8;
			if (d6 == a2) goto l718;
			d6 = a2;
		l730:	
			d5 = 4;
		l732:	
			d2 += 8;
		l734:
			while ((d5 >= 0) && (cup1 > 0))
			{
				d4 = bfexts(src,d0,d6);
				d0 += d6;
				d3 -= d4;
				*dst = (BYTE)d3;
				if (dst < dstmax) dst++;
				cup1--;
				d5--;
			}
			if (d1 != 31) d1++;
			a2 = d6;
		l74c:	
			d6 = d2;
			d6 >>= 3;
			d2 -= d6;
		}
	}
	return;

l75a:	
	d0 += 1;
	if (bfextu(src,d0,1)) goto l766;
	d4 = 2;
	goto l79e;
  
l766:	
	d0 += 1;
	if (bfextu(src,d0,1)) goto l772;
	d4 = 4;
	goto l79e;

l772:	
	d0 += 1;
	if (bfextu(src,d0,1)) goto l77e;
	d4 = 6;
	goto l79e;

l77e:	
	d0 += 1;
	if (bfextu(src,d0,1)) goto l792;
	d0 += 1;
	d6 = bfextu(src,d0,3);
	d0 += 3;
	d6 += 8;
	goto l7a8;
  
l792:	
	d0 += 1;
	d6 = bfextu(src,d0,5);
	d0 += 5;
	d4 = 16;
	goto l7a6;
  
l79e:
	d0 += 1;
	d6 = bfextu(src,d0,1);
	d0 += 1;
l7a6:
	d6 += d4;
l7a8:
	if (bfextu(src,d0,1)) goto l7c4;
	d0 += 1;
	if (bfextu(src,d0,1)) goto l7bc;
	d5 = 8;
	a5 = 0;
	goto l7ca;

l7bc:
	d5 = 14;
	a5 = -0x1100;
	goto l7ca;

l7c4:
	d5 = 12;
	a5 = -0x100;
l7ca:
	d0 += 1;
	d4 = bfextu(src,d0,d5);
	d0 += d5;
	d6 -= 3;
	if (d6 >= 0)
	{
		if (d6 > 0) d1 -= 1;
		d1 -= 1;
		if (d1 < 0) d1 = 0;
	}
	d6 += 2;
	phist = dst + a5 - d4 - 1;
	
	while ((d6 >= 0) && (cup1 > 0))
	{
		d3 = *phist++;
		*dst = (BYTE)d3;
		if (dst < dstmax) dst++;
		cup1--;
		d6--;
	}
	goto l74c;
}


BOOL XPK_Unpack(LPCBYTE *ppMemFile, LPDWORD pdwMemLength)
{
	DWORD dwMemLength = *pdwMemLength;
	LPCBYTE lpMemFile = *ppMemFile;
	PXPKFILEHEADER pxfh = (PXPKFILEHEADER)lpMemFile;
	DWORD dwSrcLen, dwDstLen;
	LPBYTE pBuffer;

	if ((!pxfh) || (dwMemLength < 256)
	 || (pxfh->dwXPKF != MULTICHAR4_LE_MSVC('F','K','P','X')) || (pxfh->dwSQSH != MULTICHAR4_LE_MSVC('H','S','Q','S'))) return FALSE;
	dwSrcLen = BigEndian(pxfh->dwSrcLen);
	dwDstLen = BigEndian(pxfh->dwDstLen);
	if ((dwSrcLen+8 > dwMemLength) || (dwSrcLen < 256) || (dwDstLen < 256)) return FALSE;
#ifdef MMCMP_LOG
	Log("XPK detected (SrcLen=%d DstLen=%d) filesize=%d\n", dwSrcLen, dwDstLen, dwMemLength);
#endif
	if ((pBuffer = (LPBYTE)calloc(1, (dwDstLen + 31) & ~15)) == NULL) return FALSE;
	XPK_DoUnpack(lpMemFile+sizeof(XPKFILEHEADER), dwSrcLen+8-sizeof(XPKFILEHEADER), pBuffer, dwDstLen);
	*ppMemFile = pBuffer;
	*pdwMemLength = dwDstLen;
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// PowerPack PP20 Unpacker
//

typedef struct _PPBITBUFFER
{
	UINT bitcount;
	ULONG bitbuffer;
	LPCBYTE pStart;
	LPCBYTE pSrc;

	ULONG GetBits(UINT n);
} PPBITBUFFER;


ULONG PPBITBUFFER::GetBits(UINT n)
{
	ULONG result = 0;

	for (UINT i=0; i<n; i++)
	{
		if (!bitcount)
		{
			bitcount = 8;
			if (pSrc != pStart) pSrc--;
			bitbuffer = *pSrc;
		}
		result = (result<<1) | (bitbuffer&1);
		bitbuffer >>= 1;
		bitcount--;
    }
    return result;
}


void PP20_DoUnpack(const BYTE *pSrc, UINT nSrcLen, BYTE *pDst, UINT nDstLen)
{
	PPBITBUFFER BitBuffer;
	ULONG nBytesLeft;

	BitBuffer.pStart = pSrc;
	BitBuffer.pSrc = pSrc + nSrcLen - 4;
	BitBuffer.bitbuffer = 0;
	BitBuffer.bitcount = 0;
	BitBuffer.GetBits(pSrc[nSrcLen-1]);
	nBytesLeft = nDstLen;
	while (nBytesLeft > 0)
	{
		if (!BitBuffer.GetBits(1))
		{
			UINT n = 1;
			while (n < nBytesLeft)
			{
				UINT code = BitBuffer.GetBits(2);
				n += code;
				if (code != 3) break;
			}
			for (UINT i=0; i<n; i++)
			{
				pDst[--nBytesLeft] = (BYTE)BitBuffer.GetBits(8);
			}
			if (!nBytesLeft) break;
		}
		{
			UINT n = BitBuffer.GetBits(2)+1;
			UINT nbits = pSrc[n-1];
			UINT nofs;
			if (n==4)
			{
				nofs = BitBuffer.GetBits( (BitBuffer.GetBits(1)) ? nbits : 7 );
				while (n < nBytesLeft)
				{
					UINT code = BitBuffer.GetBits(3);
					n += code;
					if (code != 7) break;
				}
			} else
			{
				nofs = BitBuffer.GetBits(nbits);
			}
			for (UINT i=0; i<=n; i++)
			{
				pDst[nBytesLeft-1] = (nBytesLeft+nofs < nDstLen) ? pDst[nBytesLeft+nofs] : 0;
				if (!--nBytesLeft) break;
			}
		}
	}
}


BOOL PP20_Unpack(LPCBYTE *ppMemFile, LPDWORD pdwMemLength)
{
	DWORD dwMemLength = *pdwMemLength;
	LPCBYTE lpMemFile = *ppMemFile;
	DWORD dwDstLen;
	LPBYTE pBuffer;

	if ((!lpMemFile) || (dwMemLength < 256) || (*(DWORD *)lpMemFile != MULTICHAR4_LE_MSVC('0','2','P','P'))) return FALSE;
	dwDstLen = (lpMemFile[dwMemLength-4]<<16) | (lpMemFile[dwMemLength-3]<<8) | (lpMemFile[dwMemLength-2]);
	//Log("PP20 detected: Packed length=%d, Unpacked length=%d\n", dwMemLength, dwDstLen);
	if ((dwDstLen < 512) || (dwDstLen > 0x400000) || (dwDstLen > 16*dwMemLength)) return FALSE;
	if ((pBuffer = (LPBYTE)calloc(1, (dwDstLen + 31) & ~15)) == NULL) return FALSE;
	PP20_DoUnpack(lpMemFile+4, dwMemLength-4, pBuffer, dwDstLen);
	*ppMemFile = pBuffer;
	*pdwMemLength = dwDstLen;
	return TRUE;
}





