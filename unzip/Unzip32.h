#ifndef _UNZIP32_
#define _UNZIP32_

#define UNZIP_VERSION     20   /* compatible with PKUNZIP 2.0 */
#define VMS_VERSION       42   /* if OS-needed-to-extract is VMS:  can do */

#define LREC_SIZE		26		// lengths of local file headers, central
#define CREC_SIZE		42		// directory headers, and the end-of-
#define ECREC_SIZE		18		// central-dir record, respectively

#define OUTBUFSIZ		0x2000
#define INBUFSIZ		512		// same as stdio uses
#define DIR_BLKSIZ		64		// number of directory entries per block
#define FILNAMSIZ		260

#define MAX_BITS		13		// used in unShrink()
#define HSIZE			(1 << MAX_BITS)	// size of global work area
#define LF				10		// '\n' on ASCII machines.  Must be 10 due to EBCDIC
#define CR				13		// '\r' on ASCII machines.  Must be 13 due to EBCDIC
#define WSIZE			0x8000    // window size--must be a power of two, and at least

#define DOS_OS2_FAT_	0	// version_made_by codes (central dir)
#define AMIGA_			1
#define VMS_			2	// MAKE SURE THESE ARE NOT DEFINED ON
#define UNIX_			3	// THE RESPECTIVE SYSTEMS!!  (like, for
#define VM_CMS_			4	// instance, "VMS", or "UNIX":  CFLAGS =
#define ATARI_			5	//  -O -DUNIX)
#define OS2_HPFS_		6
#define MAC_			7
#define Z_SYSTEM_		8
#define CPM_			9
#define NUM_HOSTS		10

#define STORED			0	// compression methods
#define SHRUNK			1
#define REDUCED1		2
#define REDUCED2		3
#define REDUCED3		4
#define REDUCED4		5
#define IMPLODED		6
#define TOKENIZED		7
#define DEFLATED		8
#define NUM_METHODS		9

#define CENTRAL_HDR_SIG   "\113\001\002"	// the infamous "PK" signature
#define LOCAL_HDR_SIG     "\113\003\004"	//  bytes, sans "P" (so unzip not
#define END_CENTRAL_SIG   "\113\005\006"	//  mistaken for zipfile itself)

#define NUMBER_THIS_DISK					4
#define NUM_DISK_WITH_START_CENTRAL_DIR		6
#define NUM_ENTRIES_CENTRL_DIR_THS_DISK		8
#define TOTAL_ENTRIES_CENTRAL_DIR			10
#define SIZE_CENTRAL_DIRECTORY				12
#define OFFSET_START_CENTRAL_DIRECTORY		16
#define ZIPFILE_COMMENT_LENGTH				20

#define C_VERSION_MADE_BY_0					0
#define C_VERSION_MADE_BY_1					1
#define C_VERSION_NEEDED_TO_EXTRACT_0		2
#define C_VERSION_NEEDED_TO_EXTRACT_1		3
#define C_GENERAL_PURPOSE_BIT_FLAG			4
#define C_COMPRESSION_METHOD				6
#define C_LAST_MOD_FILE_TIME				8
#define C_LAST_MOD_FILE_DATE				10
#define C_CRC32								12
#define C_COMPRESSED_SIZE					16
#define C_UNCOMPRESSED_SIZE					20
#define C_FILENAME_LENGTH					24
#define C_EXTRA_FIELD_LENGTH				26
#define C_FILE_COMMENT_LENGTH				28
#define C_DISK_NUMBER_START					30
#define C_INTERNAL_FILE_ATTRIBUTES			32
#define C_EXTERNAL_FILE_ATTRIBUTES			34
#define C_RELATIVE_OFFSET_LOCAL_HEADER		38

#define L_VERSION_NEEDED_TO_EXTRACT_0		0
#define L_VERSION_NEEDED_TO_EXTRACT_1		1
#define L_GENERAL_PURPOSE_BIT_FLAG			2
#define L_COMPRESSION_METHOD				4
#define L_LAST_MOD_FILE_TIME				6
#define L_LAST_MOD_FILE_DATE				8
#define L_CRC32								10
#define L_COMPRESSED_SIZE					14
#define L_UNCOMPRESSED_SIZE					18
#define L_FILENAME_LENGTH					22
#define L_EXTRA_FIELD_LENGTH				24


#define SKIP			0	// choice of activities for do_string() */
#define DISPLAY			1
#define FILENAME		2
#define EXTRA_FIELD		3



typedef struct min_info 
{
	int hostnum;
	long int offset;
	unsigned encrypted : 1;	// file encrypted: decrypt before uncompressing
	unsigned ExtLocHdr : 1;	// use time instead of CRC for decrypt check
	unsigned text : 1;		// file is text or binary
	unsigned lcflag : 1;	// convert filename to lowercase
} min_info;


typedef struct local_file_header // LOCAL
{
	BYTE version_needed_to_extract[2];
	WORD general_purpose_bit_flag;
	WORD compression_method;
	WORD last_mod_file_time;
	WORD last_mod_file_date;
	DWORD crc32;
	DWORD compressed_size;
	DWORD uncompressed_size;
	WORD filename_length;
	WORD extra_field_length;
} local_file_hdr;


typedef struct central_directory_file_header // CENTRAL
{
	BYTE version_made_by[2];
	BYTE version_needed_to_extract[2];
	WORD general_purpose_bit_flag;
	WORD compression_method;
	WORD last_mod_file_time;
	WORD last_mod_file_date;
	DWORD crc32;
	DWORD compressed_size;
	DWORD uncompressed_size;
	WORD filename_length;
	WORD extra_field_length;
	WORD file_comment_length;
	WORD disk_number_start;
	WORD internal_file_attributes;
	DWORD external_file_attributes;
	DWORD relative_offset_local_header;
} cdir_file_hdr;


typedef struct end_central_dir_record // END CENTRAL
{
	WORD number_this_disk;
	WORD num_disk_with_start_central_dir;
	WORD num_entries_centrl_dir_ths_disk;
	WORD total_entries_central_dir;
	DWORD size_central_directory;
	DWORD offset_start_central_directory;
	WORD zipfile_comment_length;
} ecdir_rec;


typedef BYTE ec_byte_rec[ECREC_SIZE+4];
typedef BYTE cdir_byte_hdr[CREC_SIZE];
typedef BYTE local_byte_hdr[LREC_SIZE];

// UnImplode

#define LITVALS     256
#define DISTVALS    64
#define LENVALS     64
#define MAXSF       LITVALS


typedef struct sf_entry 
{
    BYTE Value;
    BYTE BitLength;
} sf_entry;

typedef struct sf_tree // a shannon-fano "tree" (table)
{        
    sf_entry entry[MAXSF];
    int entries;
    int MaxLength;
} sf_tree;

typedef struct sf_node 
{						       // node in a true shannon-fano tree
    WORD left;                 // 0 means leaf node
    WORD right;                //   or value if leaf node
} sf_node;


typedef struct huft
{
	WORD e;
	WORD b;
	union
	{
		WORD n;
		struct huft *t;
	} v;
} huft;

// UnReduce

#define DLE    144

typedef BYTE f_array[64];       // for followers[256][64]



//===============
class CZipArchive
//===============
{
protected:
	LPBYTE m_lpStream, m_lpOutFile;
	DWORD m_dwLength, m_dwPos, m_dwOutFileLength, m_dwOutFilePos;
	DWORD m_dwCommentsLen, m_dwCommentsAllocated;
	LPCSTR m_lpszExtensions;
	LPSTR m_lpszComments;
	BOOL m_bExtractingComments;
	// unzip data members
	BYTE inbuf[INBUFSIZ + 8];
	BYTE outbuf[OUTBUFSIZ + 8];
	LPBYTE inptr, outptr, hold, extra_field, redirSlide;
	DWORD outpos, outcnt, ucsize, ziplen, crc32val, bitbuf;
	int incnt, bits_left;
	char local_hdr_sig[8], central_hdr_sig[8], end_central_sig[8];
	long int cur_zipfile_bufstart, extra_bytes, csize;
	cdir_file_hdr crec;
	local_file_hdr lrec;
	ecdir_rec ecrec;
	WORD methnum;
	char unkn[10];
	min_info info[DIR_BLKSIZ], *pInfo;
	short prefix_of[HSIZE + 1];     // (8193 * sizeof(short))
	BYTE suffix_of[HSIZE + 1];		// also s-f length_nodes (smaller)
	BYTE stack[HSIZE + 1];			// also s-f distance_nodes (smaller)
	char sig[8];
	char answerbuf[12];
	char filename[FILNAMSIZ];
	char zipeof;
	// UnImplode
	sf_tree lit_tree;
	sf_tree length_tree;
	sf_tree distance_tree;
	sf_node *lit_nodes;
	sf_node *length_nodes;
	sf_node *distance_nodes;
	char lit_tree_present;
	char eightK_dictionary;
	int minimum_match_length;
	int dict_bits;
	// UnReduce
	f_array *followers;
	BYTE Slen[256];
	int factor;
	// UnShrink
	int codesize, maxcode, maxcodemax, free_ent;
	// Inflate
	unsigned wp, hufts;
	DWORD bb;      // bit buffer
	unsigned bk;   // bits in bit buffer
	struct huft *fixed_tl;
	struct huft *fixed_td;
	int fixed_bl, fixed_bd;

public:
	CZipArchive(LPCSTR lpszExtensions=NULL);
	~CZipArchive();

public:
	BOOL UnzipArchive(LPBYTE lpStream, DWORD nLength);
	LPBYTE GetOutputFile();
	DWORD GetOutputFileLength();
	LPSTR GetComments(BOOL bDestroy=FALSE) { LPSTR p = m_lpszComments; if (bDestroy) m_lpszComments=NULL; return p; }

public:
	static BOOL IsArchive(LPBYTE lpStream, DWORD nLength);

protected:
	BOOL Seek(DWORD abs_offset);
	DWORD Read(LPBYTE p, DWORD n);
	BOOL ProcessZipArchive();
	int FindEndCentralDir();
	int ProcessEndCentralDir();
	int ExtractOrTestFiles();
	int ExtractOrTestMember();
	int ProcessCDirFileHdr();
	int ProcessLocalFileHdr();
	int MapName();
	int StoreInfo();
	int DoString(unsigned int len, int option);
	int ReadBuf(char *buf, unsigned int size);
	int CreateOutputFile();
	int ReadByte(WORD *);
	int Write(LPBYTE p, int len);
	int FlushOutput();
	int FillBitBuffer();
	void unShrink();
	void unReduce();
	void unImplode();
	int inflate();
	void UpdateCRC(BYTE *s, int len);
	WORD makeword(BYTE *b) { return *((WORD *)(b)); }
	DWORD makelong(BYTE *sig) { return *((DWORD *)(sig)); }
	void OUTB(int intc) { *outptr++=(BYTE)intc; if (++outcnt==OUTBUFSIZ) FlushOutput(); }
	WORD NEXTBYTE() { WORD w=0; ReadByte(&w); return w; }
	// UnImplode
	void LoadTrees(void);
	void LoadTree(sf_tree * tree, int treesize, sf_node *nodes);
	void ReadLengths(sf_tree *tree);
	void SortLengths(sf_tree *tree);
	void GenerateTrees(sf_tree *tree, sf_node *nodes);
	void ReadTree(sf_node *nodes, int *dest);
	// UnReduce
	void LoadFollowers();
	// UnShrink
	void partial_clear();
	// Inflate
	int inflate_codes(struct huft *tl, struct huft *td, int bl, int bd);
	int inflate_stored();
	int inflate_fixed();
	int inflate_dynamic();
	int inflate_block(int *e);
	int inflate_free();
	int huft_build(unsigned *b, unsigned n, unsigned s, WORD *d, WORD *e, struct huft **t, int *m);
	int huft_free(struct huft *t);
};



#endif
