/***********************************************************
	extract.c -- extract file from archive
***********************************************************/

int CLhaArchive::decode_lzhuf(DWORD &infp, LPBYTE outfp, long original_size, long packed_size, int method)
{
	LzInterface.method = method;
	LzInterface.dicbit = 13; // method + 8;
	LzInterface.infile = infp;
	LzInterface.outfile = outfp;
	LzInterface.original = original_size;
	LzInterface.packed = packed_size;
#ifdef LHADEBUG
	Log("Decode_lzhuf: method=%d\n", method);
#endif
	switch (method)
	{
	case 0:
	case 10:
		{
			int len1 = m_dwOutputLen - (int)(outfp - m_lpOutputFile);
			int len2 = m_dwStreamLen - infp;
			int len = (len1 < len2) ? len1 : len2;
			for (int i=0; i<len; i++)
			{
				outfp[i] = m_lpStream[i+infp];
			}
			LzInterface.infile += len;
			outfp += len;
			crc = 0;
		}
		break;
	case 8:	// -lzs-
		LzInterface.dicbit = 11;
		decode(&LzInterface);
		break;
	case 1: // -lh1-
	case 4: // -lh4-
	case 9: // -lz5-
		LzInterface.dicbit = 12;
		decode(&LzInterface);
		break;
	case 6: // -lh6-
		LzInterface.dicbit = 15;
		decode(&LzInterface);
		break;
	case 7: // -lh7-
		LzInterface.dicbit = 16;
	default:
		decode(&LzInterface);
	}
	infp = LzInterface.infile;
	return crc;
}
