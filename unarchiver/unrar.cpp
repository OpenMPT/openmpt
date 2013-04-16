
#define WINVER	0x0401
#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NONLS
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include "windows.h"
#include "windowsx.h"
#include "unrar.h"

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

static inline long abs(long x) { return (x<0)?-x:x; }

#include "unrar/const.h"
#include "unrar/global.cpp"
#include "unrar/rdwrfn.cpp"
#include "unrar/smallfn.cpp"
#include "unrar/block.cpp"
#include "unrar/extract.cpp"
#include "unrar/compr.cpp"



CRarArchive::CRarArchive(LPBYTE lpStream, DWORD dwMemLength)
//----------------------------------------------------------
{
	// File Read
	m_lpStream = lpStream;
	m_dwStreamLen = dwMemLength;
	m_dwStreamPos = 0;
	// File Write
	m_lpOutputFile = 0;
	m_dwOutputPos = 0;
	m_dwOutputLen = 0;
	// init
	InitCRC();
}


CRarArchive::~CRarArchive()
//-------------------------
{
	if (UnpMemory)
	{
		delete UnpMemory;
		UnpMemory=NULL;
	}
	if (m_lpOutputFile)
	{
		GlobalFreePtr(m_lpOutputFile);
		m_lpOutputFile = NULL;
	}
}


BOOL CRarArchive::IsArchive()
//---------------------------
{
	struct MarkHeader MarkHead;
	int SFXLen,ArcType;
  
	SFXLen=ArcType=SolidType=LockedType=0;
	ArcFormat=0;

	if (tread(MarkHead.Mark,SIZEOF_MARKHEAD)!=SIZEOF_MARKHEAD) return FALSE;
	if (MarkHead.Mark[0]==0x52 && MarkHead.Mark[1]==0x45 &&
		MarkHead.Mark[2]==0x7e && MarkHead.Mark[3]==0x5e)
	{
		ArcFormat=OLD;
		tseek(0,SEEK_SET);
		ReadHeader(MAIN_HEAD);
		ArcType=(OldMhd.Flags & MHD_MULT_VOL) ? VOL : ARC;
	} else
	if (MarkHead.Mark[0]==0x52 && MarkHead.Mark[1]==0x61 &&
		MarkHead.Mark[2]==0x72 && MarkHead.Mark[3]==0x21 &&
		MarkHead.Mark[4]==0x1a && MarkHead.Mark[5]==0x07 &&
		MarkHead.Mark[6]==0x00)
	{
		ArcFormat=NEW;
		if (ReadHeader(MAIN_HEAD)!=SIZEOF_NEWMHD) return FALSE;
		ArcType = (NewMhd.Flags & MHD_MULT_VOL) ? VOL : ARC;
	}
	if (ArcFormat==OLD)
	{
		MainHeadSize=SIZEOF_OLDMHD;
		NewMhd.Flags=OldMhd.Flags & 0x3f;
		NewMhd.HeadSize=OldMhd.HeadSize;
	} else
	{
		MainHeadSize=SIZEOF_NEWMHD;
		if ((UWORD)~HeaderCRC!=NewMhd.HeadCRC)
		{
			// Warning
			return FALSE; //?
		}
	}
	if (NewMhd.Flags & MHD_SOLID) SolidType=1;
	if (NewMhd.Flags & MHD_LOCK) LockedType=1;
	return (ArcType) ? TRUE : FALSE;
}



void CRarArchive::UnstoreFile()
//-----------------------------
{
	int Code;
	while (1)
	{
		if ((Code=UnpRead(UnpMemory,0x7f00))==-1) return; // Read Error
		if (Code==0) break;
		Code=(int)Min((long)Code,DestUnpSize);
		if (UnpWrite(UnpMemory,Code)==(UINT)-1) return; // Write error
		if (DestUnpSize>=0) DestUnpSize-=Code;
	}
}




