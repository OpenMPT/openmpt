
struct OldMainHeader OldMhd;
struct NewMainArchiveHeader NewMhd;
struct OldFileHeader OldLhd;
struct NewFileHeader NewLhd;
struct BlockHeader BlockHead;

UBYTE *UnpMemory=NULL;

int SolidType,LockedType;

int SkipUnpCRC=0;

int BrokenFileHeader;

int ExclCount;
int ArcCount,TotalArcCount;

int MainHeadSize;
long CurBlockPos,NextBlockPos;

unsigned long CurUnpRead,CurUnpWrite;
int Repack=0;

long UnpPackedSize;
long DestUnpSize;

unsigned long PackFileCRC,UnpFileCRC;

UDWORD HeaderCRC;

int ArcFormat;




