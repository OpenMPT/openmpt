unsigned int CRarArchive::UnpRead(unsigned char *Addr,unsigned int Count)
//-----------------------------------------------------------------------
{
	unsigned int RetCode=0,ReadSize,TotalRead=0;
	unsigned char *ReadAddr;
	ReadAddr=Addr;
	while (Count > 0)
	{
		ReadSize=(unsigned int)((Count>(UINT)UnpPackedSize) ? UnpPackedSize : Count);
		RetCode=tread(ReadAddr,ReadSize);
		CurUnpRead+=RetCode;
		ReadAddr+=RetCode;
		TotalRead+=RetCode;
		Count-=RetCode;
		UnpPackedSize-=RetCode;
		break;
	}
	if (RetCode!=(UINT)-1) RetCode=TotalRead;
	return RetCode;
}


unsigned int CRarArchive::UnpWrite(unsigned char *Addr,unsigned int Count)
//------------------------------------------------------------------------
{
	unsigned int RetCode;
	RetCode=twrite(Addr,Count);
	CurUnpWrite+=RetCode;
	if (!SkipUnpCRC)
		UnpFileCRC=CRC(UnpFileCRC,Addr,RetCode,(ArcFormat==OLD) ? CRC16 : CRC32);
	return RetCode;
}



