#ifndef _ITDEFS_H_
#define _ITDEFS_H_

#pragma pack(1)

typedef struct tagITFILEHEADER
{
	DWORD id;			// 0x4D504D49
	CHAR songname[26];
	BYTE highlight_minor;
	BYTE highlight_major;
	WORD ordnum;
	WORD insnum;
	WORD smpnum;
	WORD patnum;
	WORD cwtv;			// "made with" tracker
	WORD cmwt;			// "compatible with" tracker
	WORD flags;
	WORD special;
	BYTE globalvol;
	BYTE mv;			// master volume
	BYTE speed;
	BYTE tempo;
	BYTE sep;			// panning separation (0...128)
	BYTE pwd;			// pitch wheel depth
	WORD msglength;
	DWORD msgoffset;
	DWORD reserved;		// ChibiTracker writes "CHBI" here. OpenMPT writes "OMPT" here in some cases, see Load_it.cpp
	BYTE chnpan[64];
	BYTE chnvol[64];
} ITFILEHEADER;


typedef struct tagITENVELOPE
{
	BYTE flags;
	BYTE num;
	BYTE lpb;
	BYTE lpe;
	BYTE slb;
	BYTE sle;
	BYTE data[25*3];
	BYTE reserved;
} ITENVELOPE;

// Old Impulse Instrument Format (cmwt < 0x200)
typedef struct tagITOLDINSTRUMENT
{
	DWORD id;			// IMPI = 0x49504D49
	CHAR filename[12];	// DOS file name
	BYTE zero;
	BYTE flags;
	BYTE vls;
	BYTE vle;
	BYTE sls;
	BYTE sle;
	WORD reserved1;
	WORD fadeout;
	BYTE nna;
	BYTE dnc;
	WORD trkvers;
	BYTE nos;
	BYTE reserved2;
	CHAR name[26];
	WORD reserved3[3];
	BYTE keyboard[240];
	BYTE volenv[200];
	BYTE nodes[50];
} ITOLDINSTRUMENT;


// Impulse Instrument Format
typedef struct tagITINSTRUMENT
{
	DWORD id;
	CHAR filename[12];
	BYTE zero;
	BYTE nna;
	BYTE dct;
	BYTE dca;
	WORD fadeout;
	signed char pps;
	BYTE ppc;
	BYTE gbv;
	BYTE dfp;
	BYTE rv;
	BYTE rp;
	WORD trkvers;
	BYTE nos;
	BYTE reserved1;
	CHAR name[26];
	BYTE ifc;
	BYTE ifr;
	BYTE mch;
	BYTE mpr;
	WORD mbank;
	BYTE keyboard[240];
	ITENVELOPE volenv;
	ITENVELOPE panenv;
	ITENVELOPE pitchenv;
	BYTE dummy[4]; // was 7, but IT v2.17 saves 554 bytes
} ITINSTRUMENT;


// MPT IT Instrument Extension
typedef struct _ITINSTRUMENTEX
{
	ITINSTRUMENT iti;
	BYTE keyboardhi[120];
} ITINSTRUMENTEX, *PITINSTRUMENTEX;


// IT Sample Format
typedef struct ITSAMPLESTRUCT
{
	DWORD id;		// 0x53504D49
	CHAR filename[12];
	BYTE zero;
	BYTE gvl;
	BYTE flags;
	BYTE vol;
	CHAR name[26];
	BYTE cvt;
	BYTE dfp;
	DWORD length;
	DWORD loopbegin;
	DWORD loopend;
	DWORD C5Speed;
	DWORD susloopbegin;
	DWORD susloopend;
	DWORD samplepointer;
	BYTE vis;
	BYTE vid;
	BYTE vir;
	BYTE vit;
} ITSAMPLESTRUCT;

#pragma pack()

extern BYTE autovibit2xm[8];
extern BYTE autovibxm2it[8];

// Impulse Tracker identifcators
#define IT_IMPM 0x4D504D49		// "IMPM" IT header magic bytes 
#define IT_IMPS 0x53504D49		// "IMPS" IT sample header magic bytes
#define IT_IMPI	0x49504D49		// "IMPI" IT instrument header magic bytes

// Identificators by other trackers
#define IT_MPTM	0x2E6D7074		// "tpm." old MPTM header magic bytes
#define IT_OMPT 0x54504D4F		// "OMPT" magic bytes for non-standard OpenMPT IT files
#define IT_CHBI 0x49424843		// "CHBI" magic bytes in the IT header to identify ChibiTracker

enum IT_ReaderBitMasks
{
	// pattern row parsing, the channel data is read to obtain
	// number of channels active in the pattern. These bit masks are
	// to blank out sections of the byte of data being read.

	IT_bitmask_patternChanField_c   = 0x7f,
	IT_bitmask_patternChanMask_c    = 0x3f,
	IT_bitmask_patternChanEnabled_c = 0x80,
	IT_bitmask_patternChanUsed_c    = 0x0f

};

#endif