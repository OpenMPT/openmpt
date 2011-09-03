#include "stdafx.h"
#include "fxp.h"
#include "../common/Reporting.h"

/************************************
*	Cons/Dest	
*************************************/

Cfxp::Cfxp(void)
//--------------
{
	params=NULL;
	chunk=NULL;
	m_bNeedSwap=-1;
}

Cfxp::Cfxp(CString fileName)
//--------------------------
{
	//Cfxp();
	params=NULL;
	chunk=NULL;
	m_bNeedSwap=-1;
	Load(fileName);
}


Cfxp::Cfxp(long ID, long version, long nParams, float *ps)
//--------------------------------------------------------
{
	//Cfxp();
	params=NULL;
	chunk=NULL;
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

Cfxp::Cfxp(long ID, long plugVersion, long nPrograms, long inChunkSize, void *inChunk)
//--------------------------------------------------------------------------------
{
	//Cfxp();
	params=NULL;
	chunk=NULL;
	m_bNeedSwap=-1;
	
	ChunkMagic='CcnK';	// 'KncC';
	fxMagic='FPCh';
	byteSize = inChunkSize + 52; //52 is: header without byteSize and fxMagic.
	version=2;
	fxID=ID;
	fxVersion=plugVersion;
	numParams=nPrograms;
	chunkSize=inChunkSize;
	chunk = malloc(chunkSize);
	memcpy(chunk, inChunk, sizeof(char)*chunkSize);
	memset(prgName, 0, 28);
}


Cfxp::~Cfxp(void)
{
	delete[] params;
	free(chunk);
}


/************************************
*	Load/Save	
*************************************/


bool Cfxp::Load(CString fileName)
{
	//char s[256];	
	CFile inStream;
	CFileException e;
	if ( !inStream.Open(fileName, CFile::modeRead, &e) )
	{
		//TODO: exception
		Reporting::Notification("Error opening file.");
		return false; 
	}

	//TODO: make ReadLE OO (extend CFILE);
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
		  (fxMagic == 'FxCk' || fxMagic == 'FPCh')))
	{
		Reporting::Notification("Bad Magic number: this does not look like a preset file.");
		inStream.Close();
		return false;
	}

	if (fxMagic == 'FxCk') // load param list
	{
		params = new float[numParams];
		for (int p=0; p<numParams; p++)
		{
			if (!ReadLE(inStream, params[p]))
			{
				Reporting::Notification("Error reading Params.");
				inStream.Close();
				return false;
			}
		}
	}
	else if (fxMagic == 'FPCh') // load chunk
	{
		if (!ReadLE(inStream, chunkSize))
		{
			Reporting::Notification("Error reading chunk size.");
			inStream.Close();
			return false;
		}
		
		chunk = malloc(chunkSize);

		if (!chunk)
		{
			Reporting::Notification("Error allocating memory for chunk.");
			inStream.Close();
			return false;
		}

		if (!ReadLE(inStream, (char*)chunk, chunkSize))
		{
			Reporting::Notification("Error reading chunk.");
			inStream.Close();
			return false;
		}

	}

	inStream.Close();
	return true;
}

bool Cfxp::Save(CString fileName)
{
	CFile outStream;
	CFileException e;
	
	if ( !outStream.Open(fileName, CFile::modeCreate | CFile::modeWrite, &e) )
	{
		//TODO: exception
		return false; 
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
	{
		outStream.Close();
		return false;
	}

	if (fxMagic == 'FxCk') // save param list
	{
		for (int p=0; p<numParams; p++)
		{
			if (!WriteLE(outStream, params[p]))
			{
				//TODO: exception
				outStream.Close();
				return false;
			}
		}
	}
	else  if (fxMagic == 'FPCh') // save chunk list
	{
		if (!WriteLE(outStream, chunkSize) || !WriteLE(outStream, (char*)chunk, chunkSize))
		{
			//TODO: exception
			outStream.Close();
			return false;
		}

	}

	outStream.Close();
	return true;
}

/************************************
*	Util	
*************************************/

bool Cfxp::ReadLE(CFile &in, long &l)
{
	UINT size=sizeof(long);
	if (in.Read(&l, size) < size)
		return false;

	if (NeedSwap())
		SwapBytes(l);
	return true;
}

bool Cfxp::ReadLE(CFile &in, float &f)
{
	UINT size=sizeof(float);

	try {
		if (in.Read(&f, size) < size)
			return false;
	} catch (CFileException *e)
	{
		Reporting::Notification(e->m_strFileName);
		char s[256];
		wsprintf(s, "%lx: %d; %d; %s;", e, e->m_cause, e->m_lOsError,  (LPCTSTR)e->m_strFileName);
		Reporting::Notification(s);
		e->Delete();
	}

	if (NeedSwap())
		SwapBytes(f);
	return true;

}

bool Cfxp::ReadLE(CFile &in, char *c, UINT length)
{
	UINT size=sizeof(char)*length;
	return (in.Read(c, size) >= size);
}

bool Cfxp::WriteLE(CFile &out, const long &l)
{
	int size=sizeof(long);
	long l2 = l; 
	if (NeedSwap())
		SwapBytes(l2);
	out.Write(&l2, size);
	return true;
}

bool Cfxp::WriteLE(CFile &out, const float &f)
{
	int size=sizeof(float);
	float f2 = f;
	if (NeedSwap())
		SwapBytes(f2);
	out.Write(&f2, size);
	return true;
}

bool Cfxp::WriteLE(CFile &out, const char *c, UINT length)
{
	int size=sizeof(char)*length;
	out.Write(c, size);
	return true;
}

bool Cfxp::NeedSwap()
//-------------------
{
	if (m_bNeedSwap < 0)		//don't yet know if we need to swap - find out!
	{
		static char szChnk[] = "CcnK";  
		static long lChnk = 'CcnK';
		m_bNeedSwap = !!memcmp(szChnk, &lChnk, 4);
	}

	return m_bNeedSwap ? true : false;
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
