
BOOL CRarArchive::ExtrFile()
//--------------------------
{
	char DestFileName[NM];
	long FileCount=0,TotalFileCount,ErrCount=0;
	int SkipSolid,ExtrFile,Size,AllArgsUsed;
	int BrokenFile;

	if (UnpMemory!=NULL)
	{
		delete UnpMemory;
		UnpMemory=NULL;
	}

	TotalFileCount=AllArgsUsed=0;

	if ((UnpMemory=new unsigned char[UNP_MEMORY])==NULL) return FALSE;

	tseek(NewMhd.HeadSize-MainHeadSize,SEEK_CUR);

	while (1)
	{
		Size=ReadBlock(FILE_HEAD | READSUBBLOCK);
		if (Size<=0) break;
		if (BlockHead.HeadType==SUB_HEAD)
		{
			tseek(NextBlockPos,SEEK_SET);
			continue;
		}

		if (AllArgsUsed) break;

		switch(NewLhd.HostOS)
		{
		case MS_DOS:
		case OS2:
		case WIN_32:
			break;
		default:
			NewLhd.FileAttr = ((NewLhd.Flags & LHD_WINDOWMASK)==LHD_DIRECTORY) ? FM_DIREC : FM_ARCH;
			break;
		}

		tseek(NextBlockPos-NewLhd.PackSize,SEEK_SET);

		ExtrFile=0;
		SkipSolid=0;

		if ((NewLhd.Flags & LHD_SPLIT_BEFORE)==0 || (SkipSolid=SolidType)!=0)
		{
			lstrcpy(DestFileName,ArcFileName);

			ExtrFile=!SkipSolid;
			if (NewLhd.UnpVer<13 || NewLhd.UnpVer>UNP_VER)
			{
				// mprintf("Unknown method");
				ExtrFile=0;
				ErrCount++;
			}

			if (IsLabel(NewLhd.FileAttr)) continue;
			if (IsDir(NewLhd.FileAttr)) continue;
			// if (ExtrFile) open dest file (DestFileName)

			if (!ExtrFile && SolidType)
			{
				SkipSolid=1;
				ExtrFile=1;
			}
			if (ExtrFile)
			{
				if (!SkipSolid) FileCount++;
				TotalFileCount++;
				CurUnpRead=CurUnpWrite=0;
				UnpFileCRC=(ArcFormat==OLD) ? 0 : 0xFFFFFFFFL;
				UnpPackedSize=NewLhd.PackSize;
				DestUnpSize=NewLhd.UnpSize;
				Repack=0;
				SkipUnpCRC=SkipSolid;
				// Take the largest file in archive
				//Log("DestUnpSize=%d\n", DestUnpSize);
				if (DestUnpSize > (long)m_dwOutputPos)
				{
					m_dwOutputPos = 0;
				}
				if (NewLhd.Method==0x30)
					UnstoreFile();
				else
				if (NewLhd.UnpVer<=15)
				{
					OldUnpack(UnpMemory, TotalFileCount>1 && SolidType);
				} else
				{
					Unpack(UnpMemory,NewLhd.Flags & LHD_SOLID);
				}
				//if (!SkipSolid) AllArgsUsed=1;

				tseek(NextBlockPos,SEEK_SET);
				if (!SkipSolid)
					if ((ArcFormat==OLD && UnpFileCRC==NewLhd.FileCRC) || (ArcFormat==NEW && UnpFileCRC==~NewLhd.FileCRC))
					{
						BrokenFile=0;
					} else
					{
						//mprintf("Bad CRC!\n");
						ErrCount++;
						BrokenFile=1;
					}
			}
		}
		if (!ExtrFile)
			if (!SolidType)
				tseek(NextBlockPos,SEEK_SET);
			else
			if (!SkipSolid)
				break;
		if (AllArgsUsed) break;
	}
	delete UnpMemory;
	UnpMemory=NULL;

	return TRUE;
}

