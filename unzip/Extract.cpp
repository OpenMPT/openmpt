/*---------------------------------------------------------------------------

  extract.c

  This file contains the high-level routines ("driver routines") for extrac-
  ting and testing zipfile members.  It calls the low-level routines in files
  inflate.c, unimplod.c, unreduce.c and unshrink.c.

  ---------------------------------------------------------------------------*/


/**************************************/
/*  Function extract_or_test_files()  */
/**************************************/

// return PK-type error code
int CZipArchive::ExtractOrTestFiles()
//-----------------------------------
{
	BYTE *cd_inptr;
	int cd_incnt, error, error_in_archive=0;
	int renamed, query, filnum=-1, blknum=0;
	WORD i, j, members_remaining, num_skipped=0;
    long int cd_bufstart, bufstart, inbuf_offset, request;

/*---------------------------------------------------------------------------
    The basic idea of this function is as follows.  Since the central di-
    rectory lies at the end of the zipfile and the member files lie at the
    beginning or middle or wherever, it is not very desirable to simply
    read a central directory entry, jump to the member and extract it, and
    then jump back to the central directory.  In the case of a large zipfile
    this would lead to a whole lot of disk-grinding, especially if each mem-
    ber file is small.  Instead, we read from the central directory the per-
    tinent information for a block of files, then go extract/test the whole
    block.  Thus this routine contains two small(er) loops within a very
    large outer loop:  the first of the small ones reads a block of files
    from the central directory; the second extracts or tests each file; and
    the outer one loops over blocks.  There's some file-pointer positioning
    stuff in between, but that's about it.  Btw, it's because of this jump-
    ing around that we can afford to be lenient if an error occurs in one of
    the member files:  we should still be able to go find the other members,
    since we know the offset of each from the beginning of the zipfile.

    Begin main loop over blocks of member files.  We know the entire central
    directory is on this disk:  we would not have any of this information un-
    less the end-of-central-directory record was on this disk, and we would
    not have gotten to this routine unless this is also the disk on which
    the central directory starts.  In practice, this had better be the ONLY
    disk in the archive, but maybe someday we'll add multi-disk support.
  ---------------------------------------------------------------------------*/

    pInfo = info;
    members_remaining = ecrec.total_entries_central_dir;

    while (members_remaining) 
	{
        j = 0;

		// Loop through files in central directory, storing offsets, file
		// attributes, and case-conversion flags until block size is reached.
        while (members_remaining && (j < DIR_BLKSIZ)) 
		{
            --members_remaining;
            pInfo = &info[j];
            if (ReadBuf(sig, 4) <= 0) 
			{
                error_in_archive = 51;  /* 51:  unexpected EOF */
                members_remaining = 0;  /* ...so no more left to do */
                break;
            }
            if (strncmp(sig, central_hdr_sig, 4)) 
			{  /* just to make sure */
				// sig not found
                // check binary transfers
                error_in_archive = 3;   // 3:  error in zipfile
                members_remaining = 0;  // ...so no more left to do
                break;
            }
            /* process_cdir_file_hdr() sets pInfo->hostnum, pInfo->lcflag */
            if ((error = ProcessCDirFileHdr()) != 0) 
			{
                error_in_archive = error;   /* only 51 (EOF) defined */
                members_remaining = 0;  /* ...so no more left to do */
                break;
            }
            if ((error = DoString(crec.filename_length, FILENAME)) != 0) 
			{
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) 
				{  // fatal:  no more left to do
                    members_remaining = 0;
                    break;
                }
            }
            if ((error = DoString(crec.extra_field_length, EXTRA_FIELD)) != 0)
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) 
				{  // fatal
                    members_remaining = 0;
                    break;
                }
            }
            if ((error = DoString(crec.file_comment_length, SKIP)) != 0) {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) 
				{  // fatal
                    members_remaining = 0;
                    break;
                }
            }
            if (StoreInfo())
               ++num_skipped;
            else
               ++j;  /* file is OK: save info[] and continue with next */
        } /* end while-loop (adding files to current block) */

        /* save position in central directory so can come back later */
        cd_bufstart = cur_zipfile_bufstart;
        cd_inptr = inptr;
        cd_incnt = incnt;

    /*-----------------------------------------------------------------------
        Second loop:  process files in current block, extracting or testing
        each one.
      -----------------------------------------------------------------------*/

        for (i = 0; i < j; ++i) 
		{
            filnum = i + blknum*DIR_BLKSIZ;
            pInfo = &info[i];
            /*
             * if the target position is not within the current input buffer
             * (either haven't yet read far enough, or (maybe) skipping back-
             * ward) skip to the target position and reset readbuf().
             */
            request = pInfo->offset + extra_bytes;
            inbuf_offset = request % INBUFSIZ;
            bufstart = request - inbuf_offset;

            if (request < 0) 
			{
                error_in_archive = 3;       // 3:  severe error in zipfile,
                continue;                   //  but can still go on
            } else if (bufstart != cur_zipfile_bufstart) 
			{
                cur_zipfile_bufstart = m_dwPos = bufstart;
                if ((incnt = Read(inbuf,INBUFSIZ)) <= 0) 
				{
                    error_in_archive = 3;   // 3:  error in zipfile, but
                    continue;               //  can still do next file
                }
                inptr = inbuf + inbuf_offset;
                incnt -= inbuf_offset;
            } else {
                incnt += (inptr-inbuf) - inbuf_offset;
                inptr = inbuf + inbuf_offset;
            }

            // should be in proper position now, so check for sig
            if (ReadBuf(sig, 4) <= 0) 
			{  // bad offset
                error_in_archive = 3;    // 3:  error in zipfile
                continue;       // but can still try next one
            }
            if (strncmp(sig, local_hdr_sig, 4)) 
			{
                error_in_archive = 3;
                continue;
            }
            if ((error = ProcessLocalFileHdr()) != 0) 
			{
                error_in_archive = error;       // only 51 (EOF) defined
                continue;       // can still try next one
            }
            if ((error = DoString(lrec.filename_length, FILENAME)) != 0) 
			{
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) 
				{
                    continue;   // go on to next one
                }
            }
            if ((error = DoString(lrec.extra_field_length, EXTRA_FIELD)) != 0)
            {
                if (error > error_in_archive)
                    error_in_archive = error;
                if (error > 1) 
				{
                    continue;   // go on
                }
            }

            /*
             * just about to extract file:  if extracting to disk, check if
             * already exists, and if so, take appropriate action according to
             * fflag/uflag/overwrite_all/etc. (we couldn't do this in upper
             * loop because we don't store the possibly renamed filename[] in
             * info[])
             */
            renamed = FALSE;   // user hasn't renamed output file yet
            query = FALSE;
            // mapname can create dirs if not freshening or if renamed
            if ((error = MapName()) > 1) // Skip
			{
                if ((error > 2) && (error_in_archive < 2))
                    error_in_archive = 2;   // (weak) error in zipfile
                continue;   // go on to next file
            }

            if ((error = ExtractOrTestMember()) != 0) 
			{
                if (error > error_in_archive)
                    error_in_archive = error;       // ...and keep going
            }
        } // end for-loop (i:  files in current block)


        /*
         * Jump back to where we were in the central directory, then go and do
         * the next batch of files.
         */

        cur_zipfile_bufstart = m_dwPos = cd_bufstart;
        Read(inbuf, INBUFSIZ);  // were there b4 ==> no error
        inptr = cd_inptr;
        incnt = cd_incnt;
        ++blknum;

    } // end while-loop (blocks of files in central directory)

/*---------------------------------------------------------------------------
    Double-check that we're back at the end-of-central-directory record, and
    print quick summary of results, if we were just testing the archive.  We
    send the summary to stdout so that people doing the testing in the back-
    ground and redirecting to a file can just do a "tail" on the output file.
  ---------------------------------------------------------------------------*/

    ReadBuf(sig, 4);
    if (strncmp(sig, end_central_sig, 4)) 
	{     // just to make sure again
        if (!error_in_archive)       // don't overwrite stronger error
            error_in_archive = 1;    // 1:  warning error
    }
    if ((num_skipped > 0) && !error_in_archive)   // files not tested or
        error_in_archive = 1;                     //  extracted:  warning

    return (error_in_archive);

} // end function extract_or_test_files()





/***************************/
/*  Function store_info()  */
/***************************/

// return 1 if skipping, 0 if OK
int CZipArchive::StoreInfo()
{
#define UNKN_COMPR	(crec.compression_method>IMPLODED && crec.compression_method!=DEFLATED)

//	Check central directory info for version/compatibility requirements.
    pInfo->encrypted = crec.general_purpose_bit_flag & 1;    // bit field
    pInfo->ExtLocHdr = (crec.general_purpose_bit_flag & 8) == 8;  // bit
    pInfo->text = crec.internal_file_attributes & 1;         // bit field

    if (crec.version_needed_to_extract[1] == VMS_) 
	{
        if (crec.version_needed_to_extract[0] > VMS_VERSION) return 1;
    // usual file type:  don't need VMS to extract
    } else
	if (crec.version_needed_to_extract[0] > UNZIP_VERSION) return 1;
    if UNKN_COMPR return 1;
	// Skipping encrypted data
    if (pInfo->encrypted)  return 1;

//	Store some central-directory information (encryption, offsets) for later use.
    pInfo->offset = (long int) crec.relative_offset_local_header;
    return 0;
} // end function StoreInfo()





/***************************************/
/*  Function extract_or_test_member()  */
/***************************************/

// return PK-type error code
int CZipArchive::ExtractOrTestMember()
//------------------------------------
{
    int error=0;
    WORD b;

//	Initialize variables, buffers, etc.
    bits_left = 0;
    bitbuf = 0L;
    outpos = 0L;
    outcnt = 0;
    outptr = outbuf;
    zipeof = 0;
    crc32val = 0xFFFFFFFFL;

    memset(outbuf, 0, OUTBUFSIZ);
    if (CreateOutputFile()) return 50; // 50:  disk full (?)

//	Unpack the file.
    switch (lrec.compression_method) 
	{
    case STORED:
        while (ReadByte(&b)) OUTB(b);
        break;

    case SHRUNK:
        unShrink();
        break;

    case REDUCED1:
    case REDUCED2:
    case REDUCED3:
    case REDUCED4:
        unReduce();
        break;

    case IMPLODED:
        unImplode();
        break;

	case DEFLATED:
		if (inflate() != 0)	return 3;
		break;

    default:   // should never get to this point
		return 1;

    } // end switch (compression method)

/*---------------------------------------------------------------------------
    Write the last partial buffer, if any; set the file date and time; and
    close the file (not necessarily in that order).  Then make sure CRC came
    out OK and print result.  [Note:  crc32val must be logical-ANDed with
    32 bits of 1's, or else machines whose longs are bigger than 32 bits will
    report bad CRCs (because of the upper bits being filled with 1's instead
    of 0's).]
  ---------------------------------------------------------------------------*/

    if (FlushOutput()) error = 1;
    if ((crc32val = ((~crc32val) & 0xFFFFFFFFL)) != lrec.crc32) 
	{
        /* if quietflg is set, we haven't output the filename yet:  do it */
        error = 1;              // 1:  warning error
    }
    return error;
}       // end function ExtractOrTestMember()





