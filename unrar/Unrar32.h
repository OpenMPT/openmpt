#ifndef UNRAR32_LIB_H
#define UNRAR32_LIB_H


//===============
class CRarArchive
//===============
{
public:
	CRarArchive(LPBYTE lpStream, DWORD dwMemLength, LPCSTR lpszExtensions=NULL);
	~CRarArchive();

public:
	LPBYTE GetOutputFile() const { return m_lpOutputFile; }
	DWORD GetOutputFileLength() const { return m_dwOutputLen; }

public:
	BOOL IsArchive();
	BOOL ExtrFile();

protected:
	int tread(LPVOID p, DWORD len);
	int twrite(LPVOID p, DWORD len);
	int tseek(LONG pos, int sktype);
	int tell() const { return m_dwStreamPos; }

protected:
	void InitCRC();
	int ReadHeader(int BlockType);
	int ReadBlock(int BlockType);
	unsigned int UnpRead(unsigned char *Addr,unsigned int Count);
	unsigned int UnpWrite(unsigned char *Addr,unsigned int Count);
	void UnpReadBuf(int FirstBuf);
	void UnpWriteBuf();
	void UnstoreFile();
	void Unpack(unsigned char *UnpAddr,int Solid);
	void ReadTables();
	void ReadLastTables();
	void OldUnpack(BYTE *UnpAddr,int Solid);
	DWORD CRC(DWORD StartCRC,void *Addr,DWORD Size,int Mode);
	WORD CalcCheckSum16(WORD StartCRC,BYTE *Addr,DWORD Size);
	DWORD CalcCRC32(DWORD StartCRC,BYTE *Addr,DWORD Size);

protected:
	LPBYTE m_lpStream;		// RAR file data
	DWORD m_dwStreamLen;	// RAR file size
	DWORD m_dwStreamPos;	// RAR file position
	LPBYTE m_lpOutputFile;
	DWORD m_dwOutputPos;
	DWORD m_dwOutputLen;

protected:
	CHAR ArcFileName[256];
	DWORD CRCTab[256];
};


#endif // UNRAR32_LIB_H
