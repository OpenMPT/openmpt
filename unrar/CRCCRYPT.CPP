

#define  rol(x,n)  (((x)<<(n)) | ((x)>>(8*sizeof(x)-(n))))
#define  ror(x,n)  (((x)>>(n)) | ((x)<<(8*sizeof(x)-(n))))


void CRarArchive::InitCRC()
//-------------------------
{
	DWORD C;
	
	for (int I=0;I<256;I++)
	{
		C=I;
		for (int J=0;J<8;J++)
		{
			C=(C & 1) ? (C>>1)^0xEDB88320L : (C>>1);
		}
		CRCTab[I]=C;
	}
}


DWORD CRarArchive::CRC(DWORD StartCRC,void *Addr,DWORD Size,int Mode)
//-------------------------------------------------------------------
{
	if (Mode==CRC16)
		return(CalcCheckSum16((WORD)StartCRC,(unsigned char *)Addr,Size));
	else
		return(CalcCRC32(StartCRC,(unsigned char *)Addr,Size));
}


WORD CRarArchive::CalcCheckSum16(WORD StartCRC,BYTE *Addr,DWORD Size)
//-------------------------------------------------------------------
{
	for (UINT I=0;I<Size;I++)
	{
		StartCRC+=Addr[I];
		StartCRC=(UWORD)rol(StartCRC,1);
	}
	return(StartCRC);
}


DWORD CRarArchive::CalcCRC32(DWORD StartCRC,BYTE *Addr,DWORD Size)
//----------------------------------------------------------------
{
	for (UINT I=0; I<Size; I++)
		StartCRC = CRCTab[(BYTE)StartCRC ^ Addr[I]] ^ (StartCRC >> 8);
	return(StartCRC);
}

