#include "stdafx.h"
#include ".\fxp.h"

/************************************
*	Cons/Dest	
*************************************/

Cfxp::Cfxp(void)
//--------------
{
	params=NULL;
	m_bNeedSwap=-1;
}

Cfxp::Cfxp(CString fileName)
//--------------------------
{
	//Cfxp();
	params=NULL;
	m_bNeedSwap=-1;
	Load(fileName);
}


Cfxp::Cfxp(long ID, long version, long nParams, float *ps)
//------------------------------------------------------------------
{
	//Cfxp();
	params=NULL;
	m_bNeedSwap=-1;
	
	ChunkMagic='CcnK';	// 'KncC';
	fxMagic='FxCk';		// 'kCxF';
	byteSize=0;
	version=2;
	fxID=ID;
	fxVersion=version;
	numParams=nParams;
	params = new float[numParams];

	memcpy(params, ps, sizeof(float)*numParams);
	memset(prgName, 0, 28);
}


Cfxp::~Cfxp(void)
{
	if (params)
		delete[] params;
}


/************************************
*	Load/Save	
*************************************/


bool Cfxp::Load(CString fileName)
{
	CFile inStream;
	CFileException e;
	
	if ( !inStream.Open(fileName, CFile::modeRead, &e) )
	{
		//TODO: exception
		return false; 
		//TRACE( "Can't open file %s, error = %u\n", pszFileName, e.m_cause );
	}

	//TODO: make ReadLE OO (override CFILE);
	//TODO: exceptions
	if (!(ReadLE(inStream, ChunkMagic)	&&
		  ReadLE(inStream, byteSize)	&&
		  ReadLE(inStream, fxMagic)		&&
		  ReadLE(inStream, version)		&&
		  ReadLE(inStream, fxID)		&&
		  ReadLE(inStream, fxVersion)	&&
		  ReadLE(inStream, numParams)	&&
		  ReadLE(inStream, prgName, 28) &&
		  ChunkMagic == 'CcnK'			&&  
		  fxMagic == 'FxCk'))
		return false;

	params = new float[numParams];

	for (int p=0; p<numParams; p++)
	{
		if (!ReadLE(inStream, params[p]))
			//TODO: exception
			return false;
	}

	inStream.Close();
}

bool Cfxp::Save(CString fileName)
{
	CFile outStream;
	CFileException e;
	
	if ( !outStream.Open(fileName, CFile::modeCreate | CFile::modeWrite, &e) )
	{
		//TODO: exception
		return false; 
		//TRACE( "Can't open file %s, error = %u\n", pszFileName, e.m_cause );
	}

	//TODO: make ReadLE OO (override CFILE);
	//TODO: exceptions
	if (!(WriteLE(outStream, ChunkMagic)	&&
		  WriteLE(outStream, byteSize)	&&
		  WriteLE(outStream, fxMagic)	&&
		  WriteLE(outStream, version)	&&
		  WriteLE(outStream, fxID)		&&
		  WriteLE(outStream, fxVersion)	&&
		  WriteLE(outStream, numParams)	&&
		  WriteLE(outStream, prgName, 28)))
		return false;

	for (int p=0; p<numParams; p++)
	{
		if (!WriteLE(outStream, params[p]))
			//TODO: exception
			return false;
	}

	outStream.Close();
}

/************************************
*	Util	
*************************************/

bool Cfxp::ReadLE(CFile in, long &l)
{
	int size=sizeof(long);
	if (in.Read(&l, size) < size)
		return false;

	if (NeedSwap())
		SwapBytes(l);
	return true;
}

bool Cfxp::ReadLE(CFile in, float &f)
{
	int size=sizeof(float);
	if (in.Read(&f, size) < size)
		return false;

	if (NeedSwap())
		SwapBytes(f);
	return true;

}

bool Cfxp::ReadLE(CFile in, char *c, UINT length)
{
	int size=sizeof(char)*length;
	return (in.Read(&c, size) >= size);
}

bool Cfxp::WriteLE(CFile out, long l)
{
	int size=sizeof(long);
	if (NeedSwap())
		SwapBytes(l);
	out.Write(&l, size);
	return true;
}

bool Cfxp::WriteLE(CFile out, float f)
{
	int size=sizeof(float);
	if (NeedSwap())
		SwapBytes(f);
	out.Write(&f, size);
	return true;
}

bool Cfxp::WriteLE(CFile out, char *c, UINT length)
{
	int size=sizeof(char)*length;
	out.Write(&c, size);
	return true;
}

bool Cfxp::NeedSwap()
//-------------------
{
	if (m_bNeedSwap<0)		//don't yet know if we need to swap - find out!
	{
		static char szChnk[] = "CcnK";  
		static long lChnk = 'CcnK';
		m_bNeedSwap = !!memcmp(szChnk, &lChnk, 4);
	}

	return m_bNeedSwap;
}


void Cfxp::SwapBytes(long &l)
//---------------------------
{
	unsigned char *b = (unsigned char *)&l;
	long intermediate =  ((long)b[0] << 24) | ((long)b[1] << 16) | ((long)b[2] << 8) | (long)b[3];
	l = intermediate;

}

void Cfxp::SwapBytes(float &f)
//----------------------------
{
	long *pl = (long *)&f;
	SwapBytes(*pl);
}
