
typedef unsigned char    UBYTE;
typedef unsigned short   UWORD;
typedef unsigned long    UDWORD;


#define ENABLE_ACCESS

#define FM_NORMAL   0x00
#define FM_RDONLY   0x01
#define FM_HIDDEN   0x02
#define FM_SYSTEM   0x04
#define FM_LABEL    0x08
#define FM_DIREC    0x10
#define FM_ARCH     0x20

#define PATHDIVIDER  "\\"
#define CPATHDIVIDER '\\'

#define MAXWINSIZE      0x100000
#define MAXWINMASK      (MAXWINSIZE-1)
#define UNP_MEMORY      MAXWINSIZE

void OldUnpack(UBYTE *UnpAddr,int Solid);

extern long DestUnpSize;

#define  Min(x,y) (((x)<(y)) ? (x):(y))
#define  Max(x,y) (((x)>(y)) ? (x):(y))

#define  NM  260


#define  SIZEOF_MARKHEAD         7
#define  SIZEOF_OLDMHD           7
#define  SIZEOF_NEWMHD          13
#define  SIZEOF_OLDLHD          21
#define  SIZEOF_NEWLHD          32
#define  SIZEOF_SHORTBLOCKHEAD   7
#define  SIZEOF_LONGBLOCKHEAD   11
#define  SIZEOF_COMMHEAD        13
#define  SIZEOF_PROTECTHEAD     26


#define  PACK_VER       20
#define  UNP_VER        20
#define  PROTECT_VER    20


enum { FILE_EMPTY,FILE_ADD,FILE_UPDATE,FILE_COPYOLD,FILE_COPYBLOCK };

enum { ARG_RESET,ARG_NEXT,ARG_REMOVE };

enum { ARC=1,VOL,SFX };

enum { SUCCESS,WARNING,FATAL_ERROR,CRC_ERROR,LOCK_ERROR,WRITE_ERROR,
       OPEN_ERROR,USER_ERROR,MEMORY_ERROR,USER_BREAK=255,IMM_ABORT=0x8000 };

enum { EN_LOCK=1,EN_VOL=2 };

enum { SD_MEMORY=1,SD_FILES=2 };

enum { ASK_OVERWR=0,ALL_OVERWR=1,NO_OVERWR=2 };

enum { COMPARE_PATH,NOT_COMPARE_PATH };

enum { NAMES_DONTCHANGE,NAMES_UPPERCASE,NAMES_LOWERCASE };

enum { LOG_ARC=1,LOG_FILE=2 };

enum { CRC16=1,CRC32=2 };

enum { OLD_DECODE=0,OLD_ENCODE=1,NEW_CRYPT=2 };

enum { OLD_UNPACK,NEW_UNPACK };

enum { KEEP_TIME=1,LATEST_TIME };

#define  MHD_MULT_VOL       1
#define  MHD_COMMENT        2
#define  MHD_LOCK           4
#define  MHD_SOLID          8
#define  MHD_PACK_COMMENT   16
#define  MHD_AV             32
#define  MHD_PROTECT        64

#define  LHD_SPLIT_BEFORE   1
#define  LHD_SPLIT_AFTER    2
#define  LHD_PASSWORD       4
#define  LHD_COMMENT        8
#define  LHD_SOLID          16

#define  LHD_WINDOWMASK     0x00e0
#define  LHD_WINDOW64       0
#define  LHD_WINDOW128      32
#define  LHD_WINDOW256      64
#define  LHD_WINDOW512      96
#define  LHD_WINDOW1024     128
#define  LHD_DIRECTORY      0x00e0

#define  SKIP_IF_UNKNOWN    0x4000
#define  LONG_BLOCK         0x8000

#define READSUBBLOCK	0x8000

enum { ALL_HEAD=0,MARK_HEAD=0x72,MAIN_HEAD=0x73,FILE_HEAD=0x74,
       COMM_HEAD=0x75,AV_HEAD=0x76,SUB_HEAD=0x77,PROTECT_HEAD=0x78};

enum { EA_HEAD=0x100 };

enum { MS_DOS=0,OS2=1,WIN_32=2,UNIX=3 };

enum { OLD=1,NEW=2};

enum { EEMPTY=-1,EBREAK=1,EWRITE,EREAD,EOPEN,ECREAT,ECLOSE,ESEEK,
       EMEMORY,EARCH };


struct MarkHeader
{
  UBYTE Mark[7];
};


struct OldMainHeader
{
  UBYTE Mark[4];
  UWORD HeadSize;
  UBYTE Flags;
};


struct NewMainArchiveHeader
{
  UWORD HeadCRC;
  UBYTE HeadType;
  UWORD Flags;
  UWORD HeadSize;
  UWORD Reserved;
  UDWORD Reserved1;
};


struct OldFileHeader
{
  UDWORD PackSize;
  UDWORD UnpSize;
  UWORD FileCRC;
  UWORD HeadSize;
  UDWORD FileTime;
  UBYTE FileAttr;
  UBYTE Flags;
  UBYTE UnpVer;
  UBYTE NameSize;
  UBYTE Method;
};


struct NewFileHeader
{
  UWORD HeadCRC;
  UBYTE HeadType;
  UWORD Flags;
  UWORD HeadSize;
  UDWORD PackSize;
  UDWORD UnpSize;
  UBYTE HostOS;
  UDWORD FileCRC;
  UDWORD FileTime;
  UBYTE UnpVer;
  UBYTE Method;
  UWORD NameSize;
  UDWORD FileAttr;
};


struct BlockHeader
{
  UWORD HeadCRC;
  UBYTE HeadType;
  UWORD Flags;
  UWORD HeadSize;
  UDWORD DataSize;
};


struct SubBlockHeader
{
  UWORD HeadCRC;
  UBYTE HeadType;
  UWORD Flags;
  UWORD HeadSize;
  UDWORD DataSize;
  UWORD SubType;
  UBYTE Level;
};


inline int IsDir(UDWORD Attr) { return(Attr & FM_DIREC); }
inline int IsLabel(UDWORD Attr) { return(Attr & FM_LABEL); }
#define mprintf(s)		MessageBox(NULL, s, NULL, MB_OK)

