#pragma once


// struct FxProgram
// {
//	long ChunkMagic;	// "CcnK"
//	long byteSize;		// size of this chunk, excluding ChunkMagic and byteSize
//
//	long fxMagic;		// "FxCk"
//	long version;
//	long fxID;			// Fx ID
//	long fxVersion;
//	long numParams;
//	char prgName[28];
//	float *params		 //variable no. of params
// }

class Cfxp
{
public:
	Cfxp(void);
	Cfxp(CString fileName);
	Cfxp(long fxID, long fxVersion, long numParams, float *params);
	~Cfxp(void);
	
	long ChunkMagic;	// "CcnK"
	long byteSize;		// size of this chunk, excluding ChunkMagic and byteSize
	long fxMagic;		// "FxCk"
	long version;		// VST version - ignore
	long fxID;			// Plugin unique ID 
	long fxVersion;		// plugin version - ignore?
	long numParams;
	char prgName[30];
	float *params;

	bool Save(CString fileName);

protected:
	BOOL m_bNeedSwap;
	bool Load(CString fileName);

	bool ReadLE(CFile &in, long &l);
	bool ReadLE(CFile &in, float &f);
	bool ReadLE(CFile &in, char *c, UINT length=1);
	bool WriteLE(CFile &out, const long &l);
	bool WriteLE(CFile &out, const float &f);
	bool WriteLE(CFile &out, const char *c, UINT length=1);

	bool NeedSwap();
	void SwapBytes(long &l);
	void SwapBytes(float &f);
};
