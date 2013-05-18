/*
 * Wav.h
 * -----
 * Purpose: Headers for WAV reading / writing (WAV structs, FOURCCs, etc...)
 * Notes  : Some FOURCCs are also used by the MIDI/DLS routines.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "Endianness.h"


// Standard IFF chunks IDs
#define IFFID_FORM		0x4d524f46
#define IFFID_RIFF		0x46464952
#define IFFID_WAVE		0x45564157
#define IFFID_LIST		0x5453494C
#define IFFID_INFO		0x4F464E49

// IFF Info fields
#define IFFID_ICOP		0x504F4349
#define IFFID_IART		0x54524149
#define IFFID_IPRD		0x44525049
#define IFFID_INAM		0x4D414E49
#define IFFID_ICMT		0x544D4349
#define IFFID_IENG		0x474E4549
#define IFFID_ISFT		0x54465349
#define IFFID_ISBJ		0x4A425349
#define IFFID_IGNR		0x524E4749
#define IFFID_ICRD		0x44524349

// Wave IFF chunks IDs
#define IFFID_wave		0x65766177
#define IFFID_fmt		0x20746D66
#define IFFID_wsmp		0x706D7377
#define IFFID_pcm		0x206d6370
#define IFFID_data		0x61746164
#define IFFID_smpl		0x6C706D73
#define IFFID_xtra		0x61727478
#define IFFID_cue		0x20657563


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif


typedef struct PACKED WAVEFILEHEADER
{
	DWORD id_RIFF;		// "RIFF"
	DWORD filesize;		// file length-8
	DWORD id_WAVE;
} WAVEFILEHEADER;

STATIC_ASSERT(sizeof(WAVEFILEHEADER) == 12);


typedef struct PACKED WAVEFORMATHEADER
{
	DWORD id_fmt;		// "fmt "
	DWORD hdrlen;		// 16
	WORD format;		// 1
	WORD channels;		// 1:mono, 2:stereo
	DWORD freqHz;		// sampling freq
	DWORD bytessec;		// bytes/sec=freqHz*samplesize
	WORD samplesize;	// sizeof(sample)
	WORD bitspersample;	// bits per sample (8/16)
} WAVEFORMATHEADER;

STATIC_ASSERT(sizeof(WAVEFORMATHEADER) == 24);


typedef struct PACKED WAVEDATAHEADER
{
	DWORD id_data;		// "data"
	DWORD length;		// length of data
} WAVEDATAHEADER;

STATIC_ASSERT(sizeof(WAVEDATAHEADER) == 8);


typedef struct PACKED WAVESMPLHEADER
{
	// SMPL
	DWORD smpl_id;		// "smpl"	-> 0x6C706D73
	DWORD smpl_len;		// length of smpl: 3Ch	(54h with sustain loop)
	DWORD dwManufacturer;
	DWORD dwProduct;
	DWORD dwSamplePeriod;	// 1000000000/freqHz
	DWORD dwBaseNote;	// 3Ch = C-4 -> 60 + RelativeTone
	DWORD dwPitchFraction;
	DWORD dwSMPTEFormat;
	DWORD dwSMPTEOffset;
	DWORD dwSampleLoops;	// number of loops
	DWORD cbSamplerData;
} WAVESMPLHEADER;

STATIC_ASSERT(sizeof(WAVESMPLHEADER) == 44);


typedef struct PACKED SAMPLELOOPSTRUCT
{
	DWORD dwIdentifier;
	DWORD dwLoopType;		// 0=normal, 1=bidi
	DWORD dwLoopStart;
	DWORD dwLoopEnd;		// Byte offset ?
	DWORD dwFraction;
	DWORD dwPlayCount;		// Loop Count, 0=infinite

	// Set up a loop struct.
	void SetLoop(DWORD loopStart, DWORD loopEnd, bool bidi)
	{
		dwIdentifier = 0;
		dwLoopType = LittleEndian(bidi ? 1 : 0);
		dwLoopStart = LittleEndian(loopStart);
		// Loop ends are *inclusive* in the RIFF standard, while they're *exclusive* in OpenMPT.
		if(loopEnd > loopStart)
		{
			dwLoopEnd = LittleEndian(loopEnd - 1);
		} else
		{
			dwLoopEnd = LittleEndian(loopStart);
		}
		dwFraction = 0;
		dwPlayCount = 0;
	}

} SAMPLELOOPSTRUCT;

STATIC_ASSERT(sizeof(SAMPLELOOPSTRUCT) == 24);


typedef struct PACKED WAVESAMPLERINFO
{
	WAVESMPLHEADER wsiHdr;
	SAMPLELOOPSTRUCT wsiLoops[2];
} WAVESAMPLERINFO;

STATIC_ASSERT(sizeof(WAVESAMPLERINFO) == 92);


typedef struct PACKED WAVELISTHEADER
{
	DWORD list_id;	// "LIST" -> 0x5453494C
	DWORD list_len;
	DWORD info;		// "INFO"
} WAVELISTHEADER;

STATIC_ASSERT(sizeof(WAVELISTHEADER) == 12);


typedef struct PACKED WAVEEXTRAHEADER
{
	DWORD xtra_id;	// "xtra"	-> 0x61727478
	DWORD xtra_len;
	DWORD dwFlags;
	WORD  wPan;
	WORD  wVolume;
	WORD  wGlobalVol;
	WORD  wReserved;
	BYTE nVibType;
	BYTE nVibSweep;
	BYTE nVibDepth;
	BYTE nVibRate;
} WAVEEXTRAHEADER;

STATIC_ASSERT(sizeof(WAVEEXTRAHEADER) == 24);


struct PACKED WavCueHeader
{
	uint32 id;	// "cue "	-> 0x20657563
	uint32 length;
	uint32 numPoints;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(id);
		SwapBytesLE(length);
		SwapBytesLE(numPoints);
	}
};

STATIC_ASSERT(sizeof(WavCueHeader) == 12);


struct PACKED WavCuePoint
{
	uint32 id;			// Unique identification value
	uint32 pos;			// Play order position
	uint32 chunkID;		// RIFF ID of corresponding data chunk
	uint32 chunkStart;	// Byte Offset of Data Chunk
	uint32 blockStart;	// Byte Offset to sample of First Channel
	uint32 offset;		// Byte Offset to sample byte of First Channel

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(id);
		SwapBytesLE(pos);
		SwapBytesLE(chunkID);
		SwapBytesLE(chunkStart);
		SwapBytesLE(blockStart);
		SwapBytesLE(offset);
	}
};

STATIC_ASSERT(sizeof(WavCuePoint) == 24);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif
