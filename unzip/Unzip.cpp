/*---------------------------------------------------------------------------

  unzip.c

  UnZip - a zipfile extraction utility.  See below for make instructions, or
  read the comments in Makefile for a more detailed explanation.  To join 
  Info-ZIP, send a message to info-zip-request@cs.ucla.edu.

  UnZip 4.x is nearly a complete rewrite of version 3.x, mainly to allow 
  access to zipfiles via the central directory (and hence to the OS bytes, 
  so we can make intelligent decisions about what to do with the extracted 
  files).  Based on unzip.c 3.15+ and zipinfo.c 0.90.  For a complete revi-
  sion history, see UnzpHist.zip at Info-ZIP headquarters (below).  For a 
  (partial) list of the many (near infinite) contributors, see "CONTRIBS" in
  the UnZip source distribution.

  ---------------------------------------------------------------------------

  To compile (partial instructions):

     under MS-DOS (TurboC):  make -fMAKEFILE.DOS  for command line compiles
       (or use the integrated environment and the included files TCCONFIG.TC
        and UNZIP.PRJ.  Tweak for your environment.)

     under MS-DOS (MSC):  make MAKEFILE.DOS
       (or use Makefile if you have MSC 6.0:  "nmake msc_dos")

  ---------------------------------------------------------------------------

  Version:  unzip42.{arc | tar.Z | zip | zoo} for Unix, VMS, OS/2, MS-DOS.
  Source:  valeria.cs.ucla.edu (131.179.64.36) in /pub
           wuarchive.wustl.edu (128.252.135.4) in /mirrors/misc/unix
           wsmr-simtel20.army.mil (192.88.110.20) in pd1:[misc.unix]
  Copyrights:  see accompanying file "COPYING" in UnZip source distribution.

  ---------------------------------------------------------------------------*/

#define WINVER	0x0401
#define WIN32_LEAN_AND_MEAN

#define NOKANJI
#define NONLS
#define NOGDI
#define NOMCX
#define NOCRYPT
#define NOIME
#define NOUSER
#define NOKERNEL
#define NOHELP
#define NOCTLMGR
#define NOOPENFILE

#include <windows.h>
#include <windowsx.h>
#include "unzip32.h"
#include "extract.cpp"
#include "file_io.cpp"
#include "inflate.cpp"
#include "mapname.cpp"
#include "misc.cpp"
#include "unimplod.cpp"
#include "unreduce.cpp"
#include "unshrink.cpp"


BOOL CZipArchive::Seek(DWORD abs_offset)
//--------------------------------------
{
	long int request=(abs_offset)+extra_bytes;
	long int inbuf_offset=request%INBUFSIZ;
	long int bufstart=request-inbuf_offset;
	if (request < 0) return FALSE;
	if (bufstart!=cur_zipfile_bufstart)
	{
		// cur_zipfile_bufstart = lseek(zipfd,bufstart,SEEK_SET);
		cur_zipfile_bufstart = m_dwPos = bufstart;
		if ((incnt=Read(inbuf,INBUFSIZ))<=0) return FALSE;
		inptr = inbuf+inbuf_offset;
		incnt -= inbuf_offset;
	} else
	{
		incnt += (inptr-inbuf)-inbuf_offset;
		inptr=inbuf+inbuf_offset;
	}
	return TRUE;
}


DWORD CZipArchive::Read(LPBYTE p, DWORD n)
//----------------------------------------
{
	if ((!p) || (!n) || (m_dwPos >= m_dwLength)) return 0;
	if (n > m_dwLength - m_dwPos) n = m_dwLength - m_dwPos;
	memcpy(p, m_lpStream+m_dwPos, n);
	m_dwPos += n;
	return n;
}


/*---------------------------------------------------------------------------
    unShrink/unReduce/unImplode working storage:
  ---------------------------------------------------------------------------*/

/* prefix_of (for unShrink) is biggest storage area, esp. on Crays...space */
/*  is shared by lit_nodes (unImplode) and followers (unReduce) */

DWORD mask_bits[33] =
{
	0x00000000L,
	0x00000001L, 0x00000003L, 0x00000007L, 0x0000000fL,
	0x0000001fL, 0x0000003fL, 0x0000007fL, 0x000000ffL,
	0x000001ffL, 0x000003ffL, 0x000007ffL, 0x00000fffL,
	0x00001fffL, 0x00003fffL, 0x00007fffL, 0x0000ffffL,
	0x0001ffffL, 0x0003ffffL, 0x0007ffffL, 0x000fffffL,
	0x001fffffL, 0x003fffffL, 0x007fffffL, 0x00ffffffL,
	0x01ffffffL, 0x03ffffffL, 0x07ffffffL, 0x0fffffffL,
	0x1fffffffL, 0x3fffffffL, 0x7fffffffL, 0xffffffffL
};


CZipArchive::CZipArchive(LPCSTR lpszExt)
//--------------------------------------
{
	m_lpStream = NULL;
	m_lpOutFile = NULL;
	m_dwOutFileLength = 0;
	m_lpszExtensions = lpszExt;
	m_lpszComments = NULL;
	m_bExtractingComments = FALSE;
}


CZipArchive::~CZipArchive()
//-------------------------
{
	if (m_lpOutFile) GlobalFreePtr(m_lpOutFile);
	if (m_lpszComments) delete m_lpszComments;
}


LPBYTE CZipArchive::GetOutputFile()
//---------------------------------
{
	return m_lpOutFile;
}


DWORD CZipArchive::GetOutputFileLength()
//--------------------------------------
{
	return m_dwOutFilePos;
}


// return PK-type error code
BOOL CZipArchive::UnzipArchive(LPBYTE lpStream, DWORD nLength)
//------------------------------------------------------------
{
	if (!lpStream) return FALSE;
	m_lpOutFile = NULL;
	m_dwOutFileLength = 0;
	m_lpStream = lpStream;
	m_dwLength = nLength;
	m_dwPos = 0;
	ziplen = m_dwLength;
	// Okey dokey, we have everything we need to get started.  Let's roll.
	hold = &inbuf[INBUFSIZ];	// to check for boundary-spanning signatures
	if (!ProcessZipArchive()) return FALSE;	// keep passing errors back...
	return (m_lpOutFile) ? TRUE : FALSE;
}


// Test if file is an archive
BOOL CZipArchive::IsArchive(LPBYTE lpStream, DWORD nLength)
//---------------------------------------------------------
{
	CZipArchive archive;
	if (!lpStream) return FALSE;
	long int real_ecrec_offset, expect_ecrec_offset;

	archive.m_lpStream = lpStream;
	archive.m_dwLength = nLength;
	archive.m_dwPos = 0;
	archive.ziplen = archive.m_dwLength;
	archive.hold = &archive.inbuf[INBUFSIZ];
	archive.extra_bytes = 0;
//    Reconstruct the various PK signature strings; find and process the cen-
//    tral directory; list, extract or test member files as instructed; and
//    close the zipfile.
	lstrcpy(archive.local_hdr_sig, "\120");
	lstrcpy(archive.central_hdr_sig, "\120");
	lstrcpy(archive.end_central_sig, "\120");
	lstrcat(archive.local_hdr_sig, LOCAL_HDR_SIG);
	lstrcat(archive.central_hdr_sig, CENTRAL_HDR_SIG);
	lstrcat(archive.end_central_sig, END_CENTRAL_SIG);
	// not found; nothing to do - 2: error in zipfile
	if (archive.FindEndCentralDir()) return FALSE;
    real_ecrec_offset = archive.cur_zipfile_bufstart+(archive.inptr-archive.inbuf);
	if (archive.ProcessEndCentralDir()) return FALSE;
	if (archive.ecrec.number_this_disk == 0) 
	{
		expect_ecrec_offset = archive.ecrec.offset_start_central_directory + archive.ecrec.size_central_directory;
		if ((archive.extra_bytes = real_ecrec_offset - expect_ecrec_offset) < 0) return FALSE;
		else if (archive.extra_bytes > 0) return FALSE;
		return TRUE;
    }
    return FALSE;
}


BOOL CZipArchive::ProcessZipArchive()
//-----------------------------------
{
	int error=0, error_in_archive;
	long int real_ecrec_offset, expect_ecrec_offset;

//  Open the zipfile for reading and in BINARY mode to prevent CR/LF trans-
//  lation, which would corrupt the bitstreams.
	m_lpOutFile = NULL;
	if (!m_lpStream) return FALSE;	// this should never happen, given the 
									// test in UnzipArchive(), but... 
	extra_bytes = 0;
	extra_field = 0;
	pInfo = info;
	// UnImplode globals
	lit_nodes = (sf_node *) &prefix_of;     /* 2*LITVALS nodes */
	length_nodes = (sf_node *) &suffix_of;  /* 2*LENVALS nodes */
	distance_nodes = (sf_node *) &stack;    /* 2*DISTVALS nodes */
	// UnReduce Globals
	followers = (f_array *) &prefix_of;     /* shared work space */
	// Inflate globals
	fixed_tl = NULL;
//    Reconstruct the various PK signature strings; find and process the cen-
//    tral directory; list, extract or test member files as instructed; and
//    close the zipfile.
	lstrcpy(local_hdr_sig, "\120");
	lstrcpy(central_hdr_sig, "\120");
	lstrcpy(end_central_sig, "\120");
	lstrcat(local_hdr_sig, LOCAL_HDR_SIG);
	lstrcat(central_hdr_sig, CENTRAL_HDR_SIG);
	lstrcat(end_central_sig, END_CENTRAL_SIG);
	// not found; nothing to do - 2: error in zipfile
	if (FindEndCentralDir()) return NULL;
    real_ecrec_offset = cur_zipfile_bufstart+(inptr-inbuf);
	if ((error_in_archive = ProcessEndCentralDir()) > 1) return NULL;
	if (ecrec.number_this_disk == 0) 
	{
		expect_ecrec_offset = ecrec.offset_start_central_directory + ecrec.size_central_directory;
		if ((extra_bytes = real_ecrec_offset - expect_ecrec_offset) < 0) return NULL;
		else if (extra_bytes > 0) 
		{
			if ((ecrec.offset_start_central_directory == 0) 
			 && (ecrec.size_central_directory != 0))   // zip 1.5 -go bug
            {
                // error:  NULL central directory offset (attempting to process anyway)
                ecrec.offset_start_central_directory = extra_bytes;
                extra_bytes = 0;
                error_in_archive = 2;   // 2:  (weak) error in zipfile
            } else
			{
				// warning:  extra bytes at beginning or within zipfile (attempting to process anyway)
                error_in_archive = 1;   // 1:  warning error
            }
        }
        if (!Seek(ecrec.offset_start_central_directory)) return FALSE;
        error = ExtractOrTestFiles();    // EXTRACT OR TEST 'EM
		// don't overwrite stronger error
        if (error > error_in_archive) error_in_archive = error;
    } else 
	{
        // error:  zipfile is part of multi-disk archive (sorry, not supported).
        return NULL;
    }
    return TRUE;
}


// return 0 if found, 1 otherwise
int CZipArchive::FindEndCentralDir()
//----------------------------------
{
	int i, numblks;
	long int tail_len;

//	Treat case of short zipfile separately.
	if (ziplen <= INBUFSIZ)
	{
        m_dwPos = 0; //lseek(zipfd, 0L, SEEK_SET);
		if ((incnt = Read(inbuf, ziplen)) == (int)ziplen)
			// 'P' must be at least 22 bytes from end of zipfile
			for (inptr = inbuf+ziplen-22; inptr >= inbuf; --inptr)
				if ((*inptr == 'P')
				 && (!strncmp((char *)inptr, end_central_sig, 4)))
				{
					incnt -= inptr - inbuf;
					return 0;  // found it!
				}               // ...otherwise fall through & fail
	} else
//	Zipfile is longer than INBUFSIZ:  may need to loop.  Start with short
//	block at end of zipfile (if not TOO short).
	{
		if ((tail_len = ziplen % INBUFSIZ) > ECREC_SIZE)
		{
            cur_zipfile_bufstart = m_dwPos = ziplen-tail_len;
            if ((incnt = Read(inbuf, (unsigned int)tail_len)) != tail_len) goto fail;
            /* 'P' must be at least 22 bytes from end of zipfile */
            for ( inptr = inbuf+tail_len-22  ;  inptr >= inbuf  ;  --inptr )
                if ((*inptr == 'P')  &&
                      !strncmp((char *)inptr, end_central_sig, 4) ) 
				{
                    incnt -= inptr - inbuf;
                    return 0;	// found it!
                }				// ...otherwise search next block
            lstrcpyn((char *)hold, (char *)inbuf, 4);    // sig may span block boundary
        } else 
		{
            cur_zipfile_bufstart = ziplen - tail_len;
        }

        /*
         * Loop through blocks of zipfile data, starting at the end and going
         * toward the beginning.  Need only check last 65557 bytes of zipfile:
         * comment may be up to 65535 bytes long, end-of-central-directory rec-
         * ord is 18 bytes (shouldn't hardcode this number, but what the hell:
         * already did so above (22=18+4)), and sig itself is 4 bytes.
         */

        // ==amt to search==   ==done==   ==rounding==     =blksiz=
        numblks = ( min(ziplen,65557) - tail_len + (INBUFSIZ-1) ) / INBUFSIZ;

        for ( i=1;  i<=numblks;  ++i ) 
		{
            cur_zipfile_bufstart -= INBUFSIZ;
            m_dwPos = cur_zipfile_bufstart;
            if ((incnt = Read(inbuf,INBUFSIZ)) != INBUFSIZ) break;	// fall through and fail

            for ( inptr = inbuf+INBUFSIZ-1  ;  inptr >= inbuf  ;  --inptr )
                if ((*inptr == 'P')  &&
                      !strncmp((char *)inptr, end_central_sig, 4) ) {
                    incnt -= inptr - inbuf;
                    return(0);  /* found it! */
                }
            lstrcpyn((char *)hold, (char *)inbuf, 4);    // sig may span block boundary
        }

    } /* end if (ziplen > INBUFSIZ) */

//	Searched through whole region where signature should be without finding
//	it.  Print informational message and die a horrible death.
fail:
/*
     End-of-central-directory signature not found.  Either this file is not\n\
     a zipfile, or it constitutes one disk of a multi-part archive.  In the\n\
     latter case the central directory and zipfile comment will be found on\n\
     the last disk(s) of this archive.\n",
*/
    return 1;
}       // end function find_end_central_dir()





/***************************************/
/*  Function process_end_central_dir() */
/***************************************/

// return PK-type error code
int CZipArchive::ProcessEndCentralDir()
//-------------------------------------
{
    ec_byte_rec byterec;

//	Read the end-of-central-directory record and do any necessary machine-
//	type conversions (byte ordering, structure padding compensation) by
//	reading data into character array, then copying to struct.
  
    if (ReadBuf((char *)&byterec, ECREC_SIZE+4) <= 0) return 51;
    ecrec.number_this_disk = makeword(&byterec[NUMBER_THIS_DISK]);
    ecrec.num_disk_with_start_central_dir = makeword(&byterec[NUM_DISK_WITH_START_CENTRAL_DIR]);
    ecrec.num_entries_centrl_dir_ths_disk = makeword(&byterec[NUM_ENTRIES_CENTRL_DIR_THS_DISK]);
    ecrec.total_entries_central_dir = makeword(&byterec[TOTAL_ENTRIES_CENTRAL_DIR]);
    ecrec.size_central_directory = makelong(&byterec[SIZE_CENTRAL_DIRECTORY]);
    ecrec.offset_start_central_directory = makelong(&byterec[OFFSET_START_CENTRAL_DIRECTORY]);
    ecrec.zipfile_comment_length = makeword(&byterec[ZIPFILE_COMMENT_LENGTH]);
    return 0;
}




/**************************************/
/*  Function process_cdir_file_hdr()  */
/**************************************/

// return PK-type error code
int CZipArchive::ProcessCDirFileHdr()
//-----------------------------------
{
	cdir_byte_hdr byterec;

//	Read the next central directory entry and do any necessary machine-type
//	conversions (byte ordering, structure padding compensation--do so by
//	copying the data from the array into which it was read (byterec) to the
//	usable struct (crec)).
	if (ReadBuf((char *)&byterec, CREC_SIZE) <= 0) return 51; // unexpected eof
	crec.version_made_by[0] = byterec[C_VERSION_MADE_BY_0];
	crec.version_made_by[1] = byterec[C_VERSION_MADE_BY_1];
	crec.version_needed_to_extract[0] = byterec[C_VERSION_NEEDED_TO_EXTRACT_0];
	crec.version_needed_to_extract[1] = byterec[C_VERSION_NEEDED_TO_EXTRACT_1];
    crec.general_purpose_bit_flag = makeword(&byterec[C_GENERAL_PURPOSE_BIT_FLAG]);
    crec.compression_method = makeword(&byterec[C_COMPRESSION_METHOD]);
    crec.last_mod_file_time = makeword(&byterec[C_LAST_MOD_FILE_TIME]);
    crec.last_mod_file_date = makeword(&byterec[C_LAST_MOD_FILE_DATE]);
    crec.crc32 = makelong(&byterec[C_CRC32]);
    crec.compressed_size = makelong(&byterec[C_COMPRESSED_SIZE]);
    crec.uncompressed_size = makelong(&byterec[C_UNCOMPRESSED_SIZE]);
    crec.filename_length = makeword(&byterec[C_FILENAME_LENGTH]);
    crec.extra_field_length = makeword(&byterec[C_EXTRA_FIELD_LENGTH]);
    crec.file_comment_length = makeword(&byterec[C_FILE_COMMENT_LENGTH]);
    crec.disk_number_start = makeword(&byterec[C_DISK_NUMBER_START]);
    crec.internal_file_attributes = makeword(&byterec[C_INTERNAL_FILE_ATTRIBUTES]);
    crec.external_file_attributes = makelong(&byterec[C_EXTERNAL_FILE_ATTRIBUTES]);  // LONG, not word!
    crec.relative_offset_local_header = makelong(&byterec[C_RELATIVE_OFFSET_LOCAL_HEADER]);
	pInfo->hostnum = min(crec.version_made_by[1], NUM_HOSTS);
	pInfo->lcflag = 0;
	methnum = min(crec.compression_method, NUM_METHODS);
	return 0;
}       // end function process_cdir_file_hdr()





/***************************************/
/*  Function process_local_file_hdr()  */
/***************************************/

// return PK-type error code
int CZipArchive::ProcessLocalFileHdr()
//------------------------------------
{
    local_byte_hdr byterec;


/*---------------------------------------------------------------------------
    Read the next local file header and do any necessary machine-type con-
    versions (byte ordering, structure padding compensation--do so by copy-
    ing the data from the array into which it was read (byterec) to the
    usable struct (lrec)).
  ---------------------------------------------------------------------------*/

    if (ReadBuf((char *)&byterec, LREC_SIZE) <= 0) return (51); // 51:  unexpected EOF

    lrec.version_needed_to_extract[0] = byterec[L_VERSION_NEEDED_TO_EXTRACT_0];
    lrec.version_needed_to_extract[1] = byterec[L_VERSION_NEEDED_TO_EXTRACT_1];
    lrec.general_purpose_bit_flag = makeword(&byterec[L_GENERAL_PURPOSE_BIT_FLAG]);
    lrec.compression_method = makeword(&byterec[L_COMPRESSION_METHOD]);
    lrec.last_mod_file_time = makeword(&byterec[L_LAST_MOD_FILE_TIME]);
    lrec.last_mod_file_date = makeword(&byterec[L_LAST_MOD_FILE_DATE]);
    lrec.crc32 = makelong(&byterec[L_CRC32]);
    lrec.compressed_size = makelong(&byterec[L_COMPRESSED_SIZE]);
    lrec.uncompressed_size = makelong(&byterec[L_UNCOMPRESSED_SIZE]);
    lrec.filename_length = makeword(&byterec[L_FILENAME_LENGTH]);
    lrec.extra_field_length = makeword(&byterec[L_EXTRA_FIELD_LENGTH]);
    csize = (long int) lrec.compressed_size;
    ucsize = (long int) lrec.uncompressed_size;
    return 0;                 // 0:  no error
}       /* end function process_local_file_hdr() */
