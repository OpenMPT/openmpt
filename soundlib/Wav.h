/*
 * Wav.h
 * -----
 * Purpose: Headers for WAV reading / writing (WAV structs, FOURCCs, etc...)
 * Notes  : Some FOURCCs are also used by the MIDI/DLS routines.
 * Authors: OpenMPT Devs
 *
 */

#ifndef WAV_H
#define WAV_H
#pragma once

#pragma pack(1)

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


typedef struct WAVEFILEHEADER
{
	DWORD id_RIFF;		// "RIFF"
	DWORD filesize;		// file length-8
	DWORD id_WAVE;
} WAVEFILEHEADER;


typedef struct WAVEFORMATHEADER
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


typedef struct WAVEDATAHEADER
{
	DWORD id_data;		// "data"
	DWORD length;		// length of data
} WAVEDATAHEADER;


typedef struct WAVESMPLHEADER
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


typedef struct SAMPLELOOPSTRUCT
{
	DWORD dwIdentifier;
	DWORD dwLoopType;		// 0=normal, 1=bidi
	DWORD dwLoopStart;
	DWORD dwLoopEnd;		// Byte offset ?
	DWORD dwFraction;
	DWORD dwPlayCount;		// Loop Count, 0=infinite
} SAMPLELOOPSTRUCT;


typedef struct WAVESAMPLERINFO
{
	WAVESMPLHEADER wsiHdr;
	SAMPLELOOPSTRUCT wsiLoops[2];
} WAVESAMPLERINFO;


typedef struct WAVELISTHEADER
{
	DWORD list_id;	// "LIST" -> 0x5453494C
	DWORD list_len;
	DWORD info;		// "INFO"
} WAVELISTHEADER;


typedef struct WAVEEXTRAHEADER
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


typedef struct WAVCUEHEADER
{
	DWORD cue_id;	// "cue "	-> 0x20657563
	DWORD cue_len;
	DWORD cue_num;
} WAVCUEHEADER;


typedef struct WAVCUEPOINT
{
	DWORD cp_id;			// Unique identification value
	DWORD cp_pos;			// Play order position
	DWORD cp_chunkid;		// RIFF ID of corresponding data chunk
	DWORD cp_chunkstart;	// Byte Offset of Data Chunk
	DWORD cp_blockstart;	// Byte Offset to sample of First Channel
	DWORD cp_offset;		// Byte Offset to sample byte of First Channel
} WAVCUEPOINT;


#pragma pack()

#endif // WAV_H