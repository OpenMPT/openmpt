/*---------------------------------------------------------------------------

  file_io.c

  This file contains routines for doing direct input/output, file-related
  sorts of things.  Most of the system-specific code for unzip is contained
  here, including the non-echoing password code for decryption (bottom).

  ---------------------------------------------------------------------------*/


/**********************/
/* Function readbuf() */
/**********************/

// return number of bytes read into buf
int CZipArchive::ReadBuf(char *buf, unsigned int size)
//----------------------------------------------------
{
	int count;
	int n;

	n = size;
	while (size)
	{
		if (incnt == 0)
		{
			if ((incnt = Read(inbuf, INBUFSIZ)) <= 0) return (n-size);
			// buffer ALWAYS starts on a block boundary:
			cur_zipfile_bufstart += INBUFSIZ;
			inptr = inbuf;
		}
		count = min((int)size, incnt);
		memcpy(buf, inptr, count);
		buf += count;
		inptr += count;
		incnt -= count;
		size -= count;
	}
	return n;
}



/*********************************/
/* Function create_output_file() */
/*********************************/

// return non-0 if creat failed
int CZipArchive::CreateOutputFile()
//---------------------------------
{
	CHAR s[64], *filenameext;
	int l;

	m_bExtractingComments = FALSE;
	l = lstrlen(filename);
	while ((l>0) && (filename[l-1] != '.') && (filename[l-1] != '\\')) l--;
	if (l<0) l = 0;
	filenameext = filename+l;
	if ((!lstrcmpi(filenameext, "diz"))
	 || (!lstrcmpi(filenameext, "nfo"))
	 || (!lstrcmpi(filenameext, "txt")))
	{
		if ((!lstrcmpi(filenameext, "diz")) && (m_lpszComments)) return 2;
		m_bExtractingComments = TRUE;
		if (m_lpszComments) delete m_lpszComments;
		m_lpszComments = NULL;
		m_dwCommentsLen = 0;
		m_dwCommentsAllocated = 4096;
		m_lpszComments = new CHAR[m_dwCommentsAllocated];
		return (m_lpszComments) ? 0 : 1;
	} else
	{
		if (m_lpszExtensions)
		{
			int i = 0, j = 0;
			while (m_lpszExtensions[i])
			{
				char c = m_lpszExtensions[i];
				s[j] = c;
				if (c == '|')
				{
					s[j] = 0;
					j = 0;
					if (!lstrcmpi(filenameext, s))
					{
						if (m_lpOutFile) GlobalFreePtr(m_lpOutFile);
						m_lpOutFile = NULL;
						m_dwOutFileLength = m_dwOutFilePos = 0;
						break;
					}
				} else j++;
				i++;
			}
		}
		// Create memory file
		if (!m_lpOutFile)
		{
			m_dwOutFilePos = 0;
			m_dwOutFileLength = ziplen * 2 + 1024;
			m_lpOutFile = (LPBYTE)GlobalAllocPtr(GHND, m_dwOutFileLength);
			if (!m_lpOutFile) return 1;
		}
	}
    return 0;
}




/****************************
 * Function FillBitBuffer() *
 ****************************/
/*
 * Fill bitbuf, which is 32 bits.  This function is only used by the
 * READBIT and PEEKBIT macros (which are used by all of the uncompression
 * routines).
 */
int CZipArchive::FillBitBuffer()
//------------------------------
{
	WORD temp;

	zipeof = 1;
	while ((bits_left < 25) && (ReadByte(&temp) == 8))
	{
		bitbuf |= (ULONG)temp << bits_left;
		bits_left += 8;
		zipeof = 0;
	}
	return 0;
}





/***********************/
/* Function ReadByte() */
/***********************/

int CZipArchive::ReadByte(WORD *x)
//--------------------------------
{
    // read a byte; return 8 if byte available, 0 if not
    if (csize-- <= 0) return 0;
    if (incnt == 0) 
	{
        if ((incnt = Read(inbuf, INBUFSIZ)) <= 0) return 0;
        // buffer ALWAYS starts on a block boundary:
        cur_zipfile_bufstart += INBUFSIZ;
        inptr = inbuf;
    }
    *x = *inptr++;
    --incnt;
    return 8;
}


int CZipArchive::Write(LPBYTE p, int len)
//---------------------------------------
{
	if (m_bExtractingComments)
	{
		if (m_dwCommentsLen+len+1 > m_dwCommentsAllocated)
		{
			DWORD newlen = m_dwCommentsAllocated + len + 4096;
			LPSTR tmp = new CHAR[newlen];
			if (!tmp) return 0;
			if (m_lpszComments) memcpy(tmp, m_lpszComments, m_dwCommentsAllocated);
			delete m_lpszComments;
			m_lpszComments = tmp;
			m_dwCommentsAllocated = newlen;
		}
		if (m_lpszComments)
		{
			memcpy(m_lpszComments+m_dwCommentsLen, p, len);
			m_dwCommentsLen += len;
			m_lpszComments[m_dwCommentsLen] = 0;
		}
	} else
	{
		if (m_dwOutFilePos+len > m_dwOutFileLength)
		{
			DWORD newlen = m_dwOutFileLength + len + 16384;
			LPBYTE tmp = (LPBYTE)GlobalAllocPtr(GHND, newlen);
			if (!tmp) return 0;
			memcpy(tmp, m_lpOutFile, m_dwOutFilePos);
			GlobalFreePtr(m_lpOutFile);
			m_lpOutFile = tmp;
			m_dwOutFileLength = newlen;
		}
		memcpy(m_lpOutFile+m_dwOutFilePos, p, len);
		m_dwOutFilePos += len;
	}
	return len;
}



/**************************/
/* Function FlushOutput() */
/**************************/

int CZipArchive::FlushOutput()
//----------------------------
{
    // flush contents of output buffer; return PK-type error code
    int len;

    if (outcnt) 
	{
        UpdateCRC(outbuf, outcnt);
        len = outcnt;
        if (Write(outbuf, len) != len) return 50;
        outpos += outcnt;
        outcnt = 0;
        outptr = outbuf;
    }
    return 0;                 // 0:  no error
}







