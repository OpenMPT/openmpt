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
#include "FileReader.h"


//#define MMCMP_LOG


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED MMCMPFILEHEADER
{
	char id_ziRC[4];	// "ziRC"
	char id_ONia[4];	// "ONia"
	uint16 hdrsize;
	void ConvertEndianness();
};

STATIC_ASSERT(sizeof(MMCMPFILEHEADER) == 10);

struct PACKED MMCMPHEADER
{
	uint16 version;
	uint16 nblocks;
	uint32 filesize;
	uint32 blktable;
	uint8 glb_comp;
	uint8 fmt_comp;
	void ConvertEndianness();
};

STATIC_ASSERT(sizeof(MMCMPHEADER) == 14);

struct PACKED MMCMPBLOCK
{
	uint32 unpk_size;
	uint32 pk_size;
	uint32 xor_chk;
	uint16 sub_blk;
	uint16 flags;
	uint16 tt_entries;
	uint16 num_bits;
	void ConvertEndianness();
};

STATIC_ASSERT(sizeof(MMCMPBLOCK) == 20);

struct PACKED MMCMPSUBBLOCK
{
	uint32 unpk_pos;
	uint32 unpk_size;
	void ConvertEndianness();
};

STATIC_ASSERT(sizeof(MMCMPSUBBLOCK) == 8);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif

void MMCMPFILEHEADER::ConvertEndianness()
//---------------------------------------
{
	SwapBytesLE(hdrsize);
}

void MMCMPHEADER::ConvertEndianness()
//-----------------------------------
{
	SwapBytesLE(version);
	SwapBytesLE(nblocks);
	SwapBytesLE(filesize);
	SwapBytesLE(blktable);
	SwapBytesLE(glb_comp);
	SwapBytesLE(fmt_comp);
}

void MMCMPBLOCK::ConvertEndianness()
//----------------------------------
{
	SwapBytesLE(unpk_size);
	SwapBytesLE(pk_size);
	SwapBytesLE(xor_chk);
	SwapBytesLE(sub_blk);
	SwapBytesLE(flags);
	SwapBytesLE(tt_entries);
	SwapBytesLE(num_bits);
}

void MMCMPSUBBLOCK::ConvertEndianness()
//-------------------------------------
{
	SwapBytesLE(unpk_pos);
	SwapBytesLE(unpk_size);
}


#define MMCMP_COMP		0x0001
#define MMCMP_DELTA		0x0002
#define MMCMP_16BIT		0x0004
#define MMCMP_STEREO	0x0100
#define MMCMP_ABS16		0x0200
#define MMCMP_ENDIAN	0x0400

struct MMCMPBITBUFFER
{
	uint32 bitcount;
	uint32 bitbuffer;
	const uint8 *pSrc;
	const uint8 *pEnd;

	uint32 GetBits(uint32 nBits);
};


uint32 MMCMPBITBUFFER::GetBits(uint32 nBits)
//------------------------------------------
{
	uint32 d;
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

static const uint32 MMCMP8BitCommands[8] =
{
	0x01, 0x03,	0x07, 0x0F,	0x1E, 0x3C,	0x78, 0xF8
};

static const uint32 MMCMP8BitFetch[8] =
{
	3, 3, 3, 3, 2, 1, 0, 0
};

static const uint32 MMCMP16BitCommands[16] =
{
	0x01, 0x03,	0x07, 0x0F,	0x1E, 0x3C,	0x78, 0xF0,
	0x1F0, 0x3F0, 0x7F0, 0xFF0, 0x1FF0, 0x3FF0, 0x7FF0, 0xFFF0
};

static const uint32 MMCMP16BitFetch[16] =
{
	4, 4, 4, 4, 3, 2, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};


static const uint32 MMCMP_PACKED_SIZE_MIN = 256;


bool UnpackMMCMP(std::vector<char> &unpackedData, FileReader &file)
//-----------------------------------------------------------------
{
	file.Rewind();
	unpackedData.clear();

	if(!file.CanRead(MMCMP_PACKED_SIZE_MIN)) return false;
	MMCMPFILEHEADER mfh;
	if(!file.ReadConvertEndianness(mfh)) return false;
	if(std::memcmp(mfh.id_ziRC, "ziRC", 4) != 0) return false;
	if(std::memcmp(mfh.id_ONia, "ONia", 4) != 0) return false;
	if(mfh.hdrsize < 14) return false;
	MMCMPHEADER mmh;
	if(!file.ReadConvertEndianness(mmh)) return false;
	if(mmh.nblocks == 0) return false;
	if(mmh.filesize < 16) return false;
	if(mmh.filesize > 0x8000000) return false;
	if(mmh.blktable > file.GetLength()) return false;
	if(mmh.blktable + 4 * mmh.nblocks > file.GetLength()) return false;

	unpackedData.resize(mmh.filesize);

	for (uint32 nBlock=0; nBlock<mmh.nblocks; nBlock++)
	{
		if(!file.Seek(mmh.blktable + 4*nBlock)) return false;
		if(!file.CanRead(4)) return false;
		uint32 blkPos = file.ReadUint32LE();
		if(!file.Seek(blkPos)) return false;
		MMCMPBLOCK blk;
		if(!file.ReadConvertEndianness(blk)) return false;
		std::vector<MMCMPSUBBLOCK> subblks(blk.sub_blk);
		for(uint32 i=0; i<blk.sub_blk; ++i)
		{
			if(!file.ReadConvertEndianness(subblks[i])) return false;
		}
		MMCMPSUBBLOCK *psubblk = blk.sub_blk > 0 ? &(subblks[0]) : nullptr;

		if(blkPos + sizeof(MMCMPBLOCK) + blk.sub_blk * sizeof(MMCMPSUBBLOCK) >= file.GetLength()) return false;
		uint32 memPos = blkPos + sizeof(MMCMPBLOCK) + blk.sub_blk * sizeof(MMCMPSUBBLOCK);

#ifdef MMCMP_LOG
		Log("block %d: flags=%04X sub_blocks=%d", nBlock, (UINT)pblk->flags, (UINT)pblk->sub_blk);
		Log(" pksize=%d unpksize=%d", pblk->pk_size, pblk->unpk_size);
		Log(" tt_entries=%d num_bits=%d\n", pblk->tt_entries, pblk->num_bits);
#endif
		// Data is not packed
		if (!(blk.flags & MMCMP_COMP))
		{
			for (uint32 i=0; i<blk.sub_blk; i++)
			{
				if ((psubblk->unpk_pos >= mmh.filesize) ||
					(psubblk->unpk_size >= mmh.filesize) ||
					(psubblk->unpk_size > mmh.filesize - psubblk->unpk_pos)) return false;
#ifdef MMCMP_LOG
				Log("  Unpacked sub-block %d: offset %d, size=%d\n", i, psubblk->unpk_pos, psubblk->unpk_size);
#endif
				if(!file.Seek(memPos)) return false;
				if(file.ReadRaw(&(unpackedData[psubblk->unpk_pos]), psubblk->unpk_size) != psubblk->unpk_size) return false;
				psubblk++;
			}
		} else
		// Data is 16-bit packed
		if (blk.flags & MMCMP_16BIT)
		{
			MMCMPBITBUFFER bb;
			uint32 subblk = 0;
			char *pDest = &(unpackedData[psubblk[subblk].unpk_pos]);
			uint32 dwSize = psubblk[subblk].unpk_size >> 1;
			uint32 dwPos = 0;
			uint32 numbits = blk.num_bits;
			uint32 oldval = 0;

#ifdef MMCMP_LOG
			Log("  16-bit block: pos=%d size=%d ", psubblk->unpk_pos, psubblk->unpk_size);
			if (pblk->flags & MMCMP_DELTA) Log("DELTA ");
			if (pblk->flags & MMCMP_ABS16) Log("ABS16 ");
			Log("\n");
#endif
			bb.bitcount = 0;
			bb.bitbuffer = 0;
			if(!file.Seek(memPos + blk.tt_entries)) return false;
			if(!file.CanRead(blk.pk_size - blk.tt_entries)) return false;
			bb.pSrc = reinterpret_cast<const uint8 *>(file.GetRawData());
			bb.pEnd = reinterpret_cast<const uint8 *>(file.GetRawData() - blk.tt_entries + blk.pk_size);
			while (subblk < blk.sub_blk)
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
					if (blk.flags & MMCMP_DELTA)
					{
						newval += oldval;
						oldval = newval;
					} else
					if (!(blk.flags & MMCMP_ABS16))
					{
						newval ^= 0x8000;
					}
					pDest[dwPos*2 + 0] = (uint8)(((uint16)newval) & 0xff);
					pDest[dwPos*2 + 1] = (uint8)(((uint16)newval) >> 8);
					dwPos++;
				}
				if (dwPos >= dwSize)
				{
					subblk++;
					dwPos = 0;
					if(!(subblk < blk.sub_blk)) break;
					dwSize = psubblk[subblk].unpk_size >> 1;
					pDest = &(unpackedData[psubblk[subblk].unpk_pos]);
				}
			}
		} else
		// Data is 8-bit packed
		{
			MMCMPBITBUFFER bb;
			uint32 subblk = 0;
			char *pDest = &(unpackedData[psubblk[subblk].unpk_pos]);
			uint32 dwSize = psubblk[subblk].unpk_size;
			uint32 dwPos = 0;
			uint32 numbits = blk.num_bits;
			uint32 oldval = 0;
			if(!file.Seek(memPos)) return false;
			const uint8 *ptable = reinterpret_cast<const uint8 *>(file.GetRawData());

			bb.bitcount = 0;
			bb.bitbuffer = 0;
			if(!file.Seek(memPos + blk.tt_entries)) return false;
			if(!file.CanRead(blk.pk_size - blk.tt_entries)) return false;
			bb.pSrc = reinterpret_cast<const uint8 *>(file.GetRawData());
			bb.pEnd = reinterpret_cast<const uint8 *>(file.GetRawData() - blk.tt_entries + blk.pk_size);
			while (subblk < blk.sub_blk)
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
					if (blk.flags & MMCMP_DELTA)
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
					if(!(subblk < blk.sub_blk)) break;
					dwSize = psubblk[subblk].unpk_size;
					pDest = &(unpackedData[psubblk[subblk].unpk_pos]);
				}
			}
		}
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
//
// XPK unpacker
//


static const uint32 XPK_PACKED_SIZE_MIN = 256;
static const uint32 XPK_UNPACKED_SIZE_MIN = 256;


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED XPKFILEHEADER
{
	char   XPKF[4];
	uint32 SrcLen;
	char   SQSH[4];
	uint32 DstLen;
	char   Name[16];
	uint32 Reserved;
	void ConvertEndianness();
};

STATIC_ASSERT(sizeof(XPKFILEHEADER) == 36);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif

void XPKFILEHEADER::ConvertEndianness()
//-------------------------------------
{
	SwapBytesBE(SrcLen);
	SwapBytesBE(DstLen);
	SwapBytesBE(Reserved);
}


static int bfextu(const uint8 *p, int32 bo, int32 bc)
//---------------------------------------------------
{
  int32 r;
  
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

static int bfexts(const uint8 *p, int32 bo, int32 bc)
//---------------------------------------------------
{
  int32 r;
  
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


static void XPK_DoUnpack(const uint8 *src, uint32 srcLen, uint8 *dst, int32 len)
//------------------------------------------------------------------------------
{
	static uint8 xpk_table[] = {
		2,3,4,5,6,7,8,0,3,2,4,5,6,7,8,0,4,3,5,2,6,7,8,0,5,4,6,2,3,7,8,0,6,5,7,2,3,4,8,0,7,6,8,2,3,4,5,0,8,7,6,2,3,4,5,0
	};
	int32 d0,d1,d2,d3,d4,d5,d6,a2,a5;
	int32 cp, cup1, type;
	const uint8 *c;
	uint8 *phist;
	uint8 *dstmax = dst + len;
   
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
		*dst = (uint8)d3;
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
				*dst = (uint8)d3;
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
		*dst = (uint8)d3;
		if (dst < dstmax) dst++;
		cup1--;
		d6--;
	}
	goto l74c;
}


bool UnpackXPK(std::vector<char> &unpackedData, FileReader &file)
//---------------------------------------------------------------
{
	file.Rewind();
	unpackedData.clear();

	if(!file.CanRead(XPK_PACKED_SIZE_MIN)) return false;
	XPKFILEHEADER header;
	if(!file.ReadConvertEndianness(header)) return false;
	if(std::memcmp(header.XPKF, "XPKF", 4) != 0) return false;
	if(std::memcmp(header.SQSH, "SQSH", 4) != 0) return false;
	if(header.SrcLen < XPK_PACKED_SIZE_MIN) return false;
	if(header.DstLen < XPK_UNPACKED_SIZE_MIN) return false;
	if(header.SrcLen+8 > file.GetLength()) return false;
	if(!file.CanRead(header.SrcLen + 8 - sizeof(XPKFILEHEADER))) return false;

#ifdef MMCMP_LOG
	Log("XPK detected (SrcLen=%d DstLen=%d) filesize=%d\n", header.SrcLen, header.DstLen, file.GetLength());
#endif
	unpackedData.resize(header.DstLen);
	XPK_DoUnpack(reinterpret_cast<const uint8 *>(file.GetRawData()), header.SrcLen + 8 - sizeof(XPKFILEHEADER), reinterpret_cast<uint8 *>(&(unpackedData[0])), header.DstLen);

	return true;
}


//////////////////////////////////////////////////////////////////////////////
//
// PowerPack PP20 Unpacker
//


static const uint32 PP20_PACKED_SIZE_MIN = 256;
static const uint32 PP20_UNPACKED_SIZE_MIN = 512;
static const uint32 PP20_UNPACKED_SIZE_MAX = 0x400000;


typedef struct _PPBITBUFFER
{
	uint32 bitcount;
	uint32 bitbuffer;
	const uint8 *pStart;
	const uint8 *pSrc;

	uint32 GetBits(uint32 n);
} PPBITBUFFER;


uint32 PPBITBUFFER::GetBits(uint32 n)
//-----------------------------------
{
	uint32 result = 0;

	for (uint32 i=0; i<n; i++)
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


static void PP20_DoUnpack(const uint8 *pSrc, uint32 nSrcLen, uint8 *pDst, uint32 nDstLen)
//---------------------------------------------------------------------------------------
{
	PPBITBUFFER BitBuffer;
	uint32 nBytesLeft;

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
			uint32 n = 1;
			while (n < nBytesLeft)
			{
				uint32 code = BitBuffer.GetBits(2);
				n += code;
				if (code != 3) break;
			}
			for (uint32 i=0; i<n; i++)
			{
				pDst[--nBytesLeft] = (uint8)BitBuffer.GetBits(8);
			}
			if (!nBytesLeft) break;
		}
		{
			uint32 n = BitBuffer.GetBits(2)+1;
			uint32 nbits = pSrc[n-1];
			uint32 nofs;
			if (n==4)
			{
				nofs = BitBuffer.GetBits( (BitBuffer.GetBits(1)) ? nbits : 7 );
				while (n < nBytesLeft)
				{
					uint32 code = BitBuffer.GetBits(3);
					n += code;
					if (code != 7) break;
				}
			} else
			{
				nofs = BitBuffer.GetBits(nbits);
			}
			for (uint32 i=0; i<=n; i++)
			{
				pDst[nBytesLeft-1] = (nBytesLeft+nofs < nDstLen) ? pDst[nBytesLeft+nofs] : 0;
				if (!--nBytesLeft) break;
			}
		}
	}
}


bool UnpackPP20(std::vector<char> &unpackedData, FileReader &file)
//----------------------------------------------------------------
{
	file.Rewind();
	unpackedData.clear();

	if(!file.CanRead(PP20_PACKED_SIZE_MIN)) return false;
	if(!file.ReadMagic("PP20")) return false;
	file.Seek(file.GetLength() - 4);
	uint32 dstLen = 0;
	dstLen |= file.ReadUint8() << 16;
	dstLen |= file.ReadUint8() << 8;
	dstLen |= file.ReadUint8() << 0;
	if(dstLen < PP20_UNPACKED_SIZE_MIN) return false;
	if(dstLen > PP20_UNPACKED_SIZE_MAX) return false;
	if(dstLen > 16*file.GetLength()) return false;
	unpackedData.resize(dstLen);
	file.Seek(4);
	PP20_DoUnpack(reinterpret_cast<const uint8 *>(file.GetRawData()), file.GetLength() - 4, reinterpret_cast<uint8 *>(&(unpackedData[0])), dstLen);

	return true;
}

