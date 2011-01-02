#ifndef UNLHA32_LIB_H
#define UNLHA32_LIB_H

typedef struct LzHeader 
{
  unsigned char		header_size;
  char				method[5];
  long				packed_size;
  long				original_size;
  long				last_modified_stamp;
  unsigned char		attribute;
  unsigned char		header_level;
  char				name[256];
  unsigned short	crc;
  bool				has_crc;
  unsigned char		extend_type;
  unsigned char		minor_version;
  unsigned long		unix_last_modified_stamp;
  unsigned short	unix_mode;
} LzHeader;

typedef struct _LzInterfacing
{
	DWORD infile;
	LPBYTE outfile;
	unsigned long original;
	unsigned long packed;
	int dicbit;
	int method;
} LzInterfacing;



//===============
class CLhaArchive
//===============
{
public:
	enum { MAXMATCH = 256 };	// formerly F (not more than UCHAR_MAX + 1)
	enum { THRESHOLD = 3 };		// choose optimal value
	enum { NC = 255 + MAXMATCH + 2 - THRESHOLD };
	
public:
	CLhaArchive(LPBYTE lpStream, DWORD dwMemLength, LPCSTR lpszExtensions=NULL);
	~CLhaArchive();

public:
	LPBYTE GetOutputFile() const { return m_lpOutputFile; }
	DWORD GetOutputFileLength() const { return m_dwOutputLen; }
	BOOL IsArchive();
	BOOL ExtractFile();

protected:
	LPBYTE m_lpStream;		// LHA file data
	DWORD m_dwStreamLen;	// LHA file size
	DWORD m_dwStreamPos;	// LHA file position
	LPBYTE m_lpOutputFile;
	DWORD m_dwOutputLen;
	BYTE *m_pDecoderData;

protected:
	char *get_ptr;
	unsigned short crc, bitbuf;
	unsigned char subbitbuf, bitcount;
	unsigned short crctable[256];
	LzInterfacing LzInterface;
	unsigned int n_max;
	unsigned short total_p;
	int avail, n1, most_p, nn;
	unsigned long count, nextcount, compsize;
	short *child, *parent, *block, *edge, *stock, *node;
	unsigned short *freq;
	unsigned char *buf;
	unsigned short bufsiz, blocksize;
	unsigned int np, pbit;
	void * m_pHufData;
	int flag, flagcnt, matchpos, prev_char;
	unsigned int dicsiz, dicbit, maxmatch, loc;
	unsigned short left[2 * NC - 1], right[2 * NC - 1];

protected:
	void setup_get(char *p) { get_ptr = p; }
	int get_byte() { return (*get_ptr++ & 0xff); }
	unsigned short get_word() {	int b0, b1;	b0 = get_byte(); b1 = get_byte(); return static_cast<unsigned short>((b1 << 8) + b0); }
	long get_longword() { long b0, b1, b2, b3; b0 = get_byte(); b1 = get_byte(); b2 = get_byte(); b3 = get_byte(); return (b3 << 24) + (b2 << 16) + (b1 << 8) + b0; }
	BOOL get_header(DWORD &fp, LzHeader *hdr);
	VOID make_crctable();
	unsigned short calccrc(unsigned char *p , int n);
	void init_getbits();
	unsigned short getbits(unsigned int n);
	void fillbuf(unsigned char n);
	int lharead(void *, int, int, DWORD &fp); // fread equivalent
	void fwrite_crc(unsigned char *p, int n, LPBYTE &fp);
	VOID extract_one(DWORD &afp, LzHeader *hdr);
	LPBYTE open_with_make_path(const char *, int size);
	int decode_lzhuf(DWORD &infp, LPBYTE outfp, long original_size, long packed_size, int method);
	void decode(LzInterfacing *pinterface);
	void make_table(unsigned int nchar, unsigned char *bitlen, unsigned int tablebits, unsigned short *table);

protected:
	// dhuf.cpp
	VOID InitDecodeTables();
	void start_c_dyn();
	void start_p_dyn();
	void decode_start_dyn();
	void reconst(int start, int end);
	int swap_inc(int p);
	void update_c(int p);
	void update_p(int p);
	void make_new_node(int p);
	unsigned short decode_c_dyn();
	unsigned short decode_p_dyn();

protected:
	// huf.cpp
	VOID InitHufTables();
	void decode_start_st1(int dicbit);
	void read_pt_len(short nn, short nbit, short i_special);
	void read_c_len();
	unsigned short decode_c_st1();
	unsigned short decode_p_st1();

protected:
	// shuf.cpp
	void decode_start_st0();
	void decode_start_fix();
	unsigned short decode_c_st0();
	unsigned short decode_p_st0();
	void read_tree_p();
	void read_tree_c();
	void ready_made(int method);

protected:
	// larc.cpp
	unsigned short decode_c_lzs();
	unsigned short decode_p_lzs();
	unsigned short decode_c_lz5();
	unsigned short decode_p_lz5();
	void decode_start_lzs();
	void decode_start_lz5(unsigned char *);
};


#endif // UNLHA32_LIB_H
