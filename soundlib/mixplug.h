#ifndef MIXPLUG_H_
#define MIXPLUG_H_

typedef long PlugParamIndex;
typedef float PlugParamValue;

// Mix Plugins
#define MIXPLUG_MIXREADY			0x01	// Set when cleared

class IMixPlugin
//==============
{
public:
	virtual int AddRef() = 0;
	virtual int Release() = 0;
	virtual void SaveAllParameters() = 0;
	virtual void RestoreAllParameters(long nProg=-1) = 0; //rewbs.plugDefaultProgram: added param
	virtual void Process(float *pOutL, float *pOutR, unsigned long nSamples) = 0;
	virtual void Init(unsigned long nFreq, int bReset) = 0;
	virtual bool MidiSend(DWORD dwMidiCode) = 0;
	virtual void MidiCC(UINT nMidiCh, UINT nController, UINT nParam, UINT trackChannel) = 0;
	virtual void MidiPitchBend(UINT nMidiCh, int nParam, UINT trackChannel) = 0;
	virtual void MidiCommand(UINT nMidiCh, UINT nMidiProg, WORD wMidiBank, UINT note, UINT vol, UINT trackChan) = 0;
	virtual void HardAllNotesOff() = 0;		//rewbs.VSTCompliance
	virtual void RecalculateGain() = 0;		
	virtual bool isPlaying(UINT note, UINT midiChn, UINT trackerChn) = 0; //rewbs.VSTiNNA
	virtual bool MoveNote(UINT note, UINT midiChn, UINT sourceTrackerChn, UINT destTrackerChn) = 0; //rewbs.VSTiNNA
	virtual void SetParameter(PlugParamIndex paramindex, PlugParamValue paramvalue) = 0;
	virtual void SetZxxParameter(UINT nParam, UINT nValue) = 0;
	virtual void ModifyParameter(PlugParamIndex nIndex, PlugParamValue diff);
	virtual PlugParamValue GetParameter(PlugParamIndex nIndex) = 0;
	virtual UINT GetZxxParameter(UINT nParam) = 0; //rewbs.smoothVST 
	virtual long Dispatch(long opCode, long index, long value, void *ptr, float opt) =0; //rewbs.VSTCompliance
	virtual void NotifySongPlaying(bool)=0;	//rewbs.VSTCompliance
	virtual bool IsSongPlaying()=0;
	virtual bool IsResumed()=0;
	virtual void Resume()=0;
	virtual void Suspend()=0;
	virtual BOOL isInstrument()=0;
	virtual BOOL CanRecieveMidiEvents()=0;
	virtual void SetDryRatio(UINT param)=0;

};


inline void IMixPlugin::ModifyParameter(PlugParamIndex nIndex, float diff)
//------------------------------------------------------------------------
{
	float val = GetParameter(nIndex) + diff;
	if(val > 1) val = 1;
	else if(val < 0) val = 0;
	SetParameter(nIndex, val);
}


												///////////////////////////////////////////////////
												// !!! bits 8 -> 15 reserved for mixing mode !!! //
												///////////////////////////////////////////////////
#define MIXPLUG_INPUTF_MASTEREFFECT				0x01	// Apply to master mix
#define MIXPLUG_INPUTF_BYPASS					0x02	// Bypass effect
#define MIXPLUG_INPUTF_WETMIX					0x04	// Wet Mix (dry added)
#define MIXPLUG_INPUTF_MIXEXPAND				0x08	// effect plugin mixing mode combo, [0%,100%] -> [-200%,200%]


struct SNDMIXPLUGINSTATE
{
	DWORD dwFlags;					// MIXPLUG_XXXX
	LONG nVolDecayL, nVolDecayR;	// Buffer click removal
	int *pMixBuffer;				// Stereo effect send buffer
	float *pOutBufferL;				// Temp storage for int -> float conversion
	float *pOutBufferR;
};
typedef SNDMIXPLUGINSTATE* PSNDMIXPLUGINSTATE;

struct SNDMIXPLUGININFO
{
	DWORD dwPluginId1;
	DWORD dwPluginId2;
	DWORD dwInputRouting;	// MIXPLUG_INPUTF_XXXX
	DWORD dwOutputRouting;	// 0=mix 0x80+=fx
	DWORD dwReserved[4];	// Reserved for routing info
	CHAR szName[32];
	CHAR szLibraryName[64];	// original DLL name
}; // Size should be 128 							
typedef SNDMIXPLUGININFO* PSNDMIXPLUGININFO;



struct SNDMIXPLUGIN
{
	const char* GetName() const {return Info.szName;}
	const char* GetLibraryName();
	CString GetParamName(const UINT index) const;

	IMixPlugin *pMixPlugin;
	PSNDMIXPLUGINSTATE pMixState;
	ULONG nPluginDataSize;
	PVOID pPluginData;
	SNDMIXPLUGININFO Info;
	float fDryRatio;		    // rewbs.dryRatio [20040123]
	long defaultProgram;		// rewbs.plugDefaultProgram
}; // rewbs.dryRatio: Hopefully this doesn't need to be a fixed size.
typedef SNDMIXPLUGIN* PSNDMIXPLUGIN;

//class CSoundFile;
class CModDoc;
typedef	BOOL (__cdecl *PMIXPLUGINCREATEPROC)(PSNDMIXPLUGIN, CSoundFile*);

enum {
	CHANNEL_ONLY		  = 0,
	INSTRUMENT_ONLY       = 1,
	PRIORITISE_INSTRUMENT = 2,
	PRIORITISE_CHANNEL    = 3,
	EVEN_IF_MUTED         = false,
	RESPECT_MUTES         = true,
};

#endif
