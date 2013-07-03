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
#define IFFID_data		0x61746164


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED WAVEFILEHEADER
{
	DWORD id_RIFF;		// "RIFF"
	DWORD filesize;		// file length-8
	DWORD id_WAVE;
};

STATIC_ASSERT(sizeof(WAVEFILEHEADER) == 12);

struct PACKED WAVEDATAHEADER
{
	DWORD id_data;		// "data"
	DWORD length;		// length of data
};

STATIC_ASSERT(sizeof(WAVEDATAHEADER) == 8);
#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif
