#pragma once
//See vstfxstore.h in VST SDK2.3 for an overview of the fxp file structure.

class Cfxp
{
public:
	Cfxp(void);
	Cfxp(CString fileName);
	Cfxp(long fxID, long fxVersion, long numParams, float *params);
	Cfxp(long ID, long version, long nPrograms, long inChunkSize, void *inChunk);
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
	long chunkSize;
	void *chunk;

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
