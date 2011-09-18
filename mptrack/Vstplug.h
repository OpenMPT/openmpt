#pragma once
#ifndef _VST_PLUGIN_MANAGER_H_
#define _VST_PLUGIN_MANAGER_H_

#ifndef NO_VST
	#define VST_FORCE_DEPRECATED 0
	#include <aeffectx.h>			// VST
	#include <vstfxstore.h>
#endif

#define kBuzzMagic	'Buzz'
#define kDmoMagic	'DXMO'


class CVstPluginManager;
class CVstPlugin;
class CVstEditor;
class Cfxp;				//rewbs.VSTpresets
class CModDoc;
class CSoundFile;

enum
{
	effBuzzGetNumCommands=0x1000,
	effBuzzGetCommandName,
	effBuzzExecuteCommand,
};


#ifndef NO_VST
	typedef AEffect * (VSTCALLBACK * PVSTPLUGENTRY)(audioMasterCallback);
#endif


typedef struct _VSTPLUGINLIB
{
	struct _VSTPLUGINLIB *pPrev, *pNext;
	DWORD dwPluginId1;
	DWORD dwPluginId2;
	BOOL bIsInstrument;
	CVstPlugin *pPluginsList;
	CHAR szLibraryName[_MAX_FNAME];
	CHAR szDllPath[_MAX_PATH];
} VSTPLUGINLIB, *PVSTPLUGINLIB;


typedef struct _VSTINSTCH
{
	//BYTE uNoteOnMap[128/8];			rewbs.deMystifyMidiNoteMap
	BYTE uNoteOnMap[128][MAX_CHANNELS];
	UINT nProgram;
	WORD wMidiBank; //rewbs.MidiBank
} VSTINSTCH, *PVSTINSTCH;


//=================================
class CVstPlugin: public IMixPlugin
//=================================
{
	friend class CAbstractVstEditor;	//rewbs.defaultPlugGUI
	friend class COwnerVstEditor;		//rewbs.defaultPlugGUI
	friend class CVstPluginManager;
#ifndef NO_VST
protected:
	enum {VSTEVENT_QUEUE_LEN=256}; 

	ULONG m_nRefCount;
	CVstPlugin *m_pNext, *m_pPrev;
	HINSTANCE m_hLibrary;
	PVSTPLUGINLIB m_pFactory;
	PSNDMIXPLUGIN m_pMixStruct;
	AEffect *m_pEffect;
	void (*m_pProcessFP)(AEffect*, float**, float**, VstInt32); //Function pointer to AEffect processReplacing if supported, else process.
	CAbstractVstEditor *m_pEditor;		//rewbs.defaultPlugGUI
	static const UINT nInvalidSampleRate = UINT_MAX;
	UINT m_nSampleRate;
	bool m_bIsVst2;
	SNDMIXPLUGINSTATE m_MixState;
	UINT m_nInputs, m_nOutputs;
	VstEvents *m_pEvList;
	VSTINSTCH m_MidiCh[16];
	short m_nMidiPitchBendPos[16];
	float **m_pTempBuffer;					//rewbs.dryRatio: changed from * to **
	float **m_pInputs;
	float **m_pOutputs;
	int m_nEditorX, m_nEditorY;
	int m_MixBuffer[MIXBUFFERSIZE*2+2];				// Stereo interleaved
	float m_FloatBuffer[MIXBUFFERSIZE*(2+32)+34];	// 2ch separated + up to 32 VSTi outputs...
	float dummyBuffer_[MIXBUFFERSIZE + 2];			// Other (unused) inputs
	VstMidiEvent m_ev_queue[VSTEVENT_QUEUE_LEN];
	CModDoc* m_pModDoc;			 //rewbs.plugDocAware
	CSoundFile* m_pSndFile;			 //rewbs.plugDocAware
//	PSNDMIXPLUGIN m_pSndMixPlugin;	 //rewbs.plugDocAware
	static const UINT nInvalidMidiChan = UINT_MAX;
	UINT m_nPreviousMidiChan; //rewbs.VSTCompliance
	bool m_bSongPlaying; //rewbs.VSTCompliance
	bool m_bPlugResumed; //rewbs.VSTCompliance
	DWORD m_dwTimeAtStartOfProcess;
	bool m_bModified;
	HANDLE processCalled;
	UINT m_nSlot;
	float m_fGain;
	bool m_bIsInstrument;

public:
	CVstPlugin(HINSTANCE hLibrary, PVSTPLUGINLIB pFactory, PSNDMIXPLUGIN pMixPlugin, AEffect *pEffect);
	virtual ~CVstPlugin();
	void Initialize(CSoundFile* pSndFile);

public:
	PVSTPLUGINLIB GetPluginFactory() const { return m_pFactory; }
	BOOL HasEditor();
	long GetNumPrograms();
	PlugParamIndex GetNumParameters();
	long GetCurrentProgram();
	long GetNumProgramCategories();	//rewbs.VSTpresets
	bool GetProgramNameIndexed(long index, long category, char *text);	//rewbs.VSTpresets
	CString GetFormattedProgramName(VstInt32 index, bool allowFallback = false);
	bool LoadProgram(CString fileName);
	bool SaveProgram(CString fileName);
	VstInt32 GetUID();			//rewbs.VSTpresets
	VstInt32 GetVersion();		//rewbs.VSTpresets
	bool GetParams(float* param, VstInt32 min, VstInt32 max); 	//rewbs.VSTpresets
	bool RandomizeParams(VstInt32 minParam = 0, VstInt32 maxParam = 0); 	//rewbs.VSTpresets
	bool isModified() {return m_bModified;}
	inline CModDoc* GetModDoc() {return m_pModDoc;}
	inline CSoundFile* GetSoundFile() {return m_pSndFile;}
	UINT FindSlot();
	void SetSlot(UINT slot);
	UINT GetSlot();
	void UpdateMixStructPtr(PSNDMIXPLUGIN);

	void SetCurrentProgram(UINT nIndex);
//rewbs.VSTCompliance: Eric's non standard preset stuff:
// -> CODE#0002
// -> DESC="VST plugins presets"
	//VOID GetProgramName(UINT nIndex, LPSTR pszName, UINT cbSize);
	//BOOL SavePreset(LPCSTR lpszFileName);
	//BOOL LoadPreset(LPCSTR lpszFileName);
// -! NEW_FEATURE#0002
	PlugParamValue GetParameter(PlugParamIndex nIndex);
	void SetParameter(PlugParamIndex nIndex, PlugParamValue fValue);
	void GetParamName(UINT nIndex, LPSTR pszName, UINT cbSize);
	void GetParamLabel(UINT nIndex, LPSTR pszLabel);
	void GetParamDisplay(UINT nIndex, LPSTR pszDisplay);
	VstIntPtr Dispatch(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	void ToggleEditor();
	void GetPluginType(LPSTR pszType);
	BOOL GetDefaultEffectName(LPSTR pszName);
	UINT GetNumCommands();
	BOOL GetCommandName(UINT index, LPSTR pszName);
	BOOL ExecuteCommand(UINT nIndex);
	CAbstractVstEditor* GetEditor(); //rewbs.defaultPlugGUI
	BOOL GetSpeakerArrangement(); //rewbs.VSTCompliance
	bool Bypass(bool);  //rewbs.defaultPlugGUI
	bool Bypass();  //rewbs.defaultPlugGUI
	bool IsBypassed();  //rewbs.defaultPlugGUI

	bool isInstrument(); // ericus 18/02/2005
	bool CanRecieveMidiEvents();

	void GetOutputPlugList(CArray<CVstPlugin*,CVstPlugin*> &list);
	void GetInputPlugList(CArray<CVstPlugin*,CVstPlugin*> &list);
	void GetInputInstrumentList(CArray<UINT,UINT> &list);
	void GetInputChannelList(CArray<UINT,UINT> &list);

public:
	int AddRef() { return ++m_nRefCount; }
	int Release();
	void SaveAllParameters();
	void RestoreAllParameters(long nProg=-1); //rewbs.plugDefaultProgram - added param 
	void ProcessVSTEvents(); //rewbs.VSTiNoteHoldonStopFix
	void ClearVSTEvents(); //rewbs.VSTiNoteHoldonStopFix
	void RecalculateGain();
	void Process(float *pOutL, float *pOutR, unsigned long nSamples);
	void Init(unsigned long nFreq, int bReset);
	bool MidiSend(DWORD dwMidiCode);
	void MidiCC(UINT nMidiCh, UINT nController, UINT nParam, UINT trackChannel);
	void MidiPitchBend(UINT nMidiCh, int nParam, UINT trackChannel);
	void MidiCommand(UINT nMidiCh, UINT nMidiProg, WORD wMidiBank, UINT note, UINT vol, UINT trackChan);
	void HardAllNotesOff(); //rewbs.VSTiNoteHoldonStopFix
	bool isPlaying(UINT note, UINT midiChn, UINT trackerChn);	//rewbs.instroVST
	bool MoveNote(UINT note, UINT midiChn, UINT sourceTrackerChn, UINT destTrackerChn); //rewbs.instroVST
	bool m_bNeedIdle; //rewbs.VSTCompliance
	bool m_bRecordAutomation;
	bool m_bPassKeypressesToPlug;
	void NotifySongPlaying(bool playing);	//rewbs.VSTCompliance
	bool IsSongPlaying() {return m_bSongPlaying;}	//rewbs.VSTCompliance
	bool IsResumed() {return m_bPlugResumed;}
	void Resume();
	void Suspend();
	DWORD GetTimeAtStartOfProcess() {return m_dwTimeAtStartOfProcess;}
	void SetDryRatio(UINT param);


	void SetZxxParameter(UINT nParam, UINT nValue);
	UINT GetZxxParameter(UINT nParam); //rewbs.smoothVST

	VstSpeakerArrangement speakerArrangement;  //rewbs.VSTcompliance

private:
	short getMIDI14bitValueFromShort(short value); 
	void MidiPitchBend(UINT nMidiCh, short pitchBendPos);
#else // case: NO_VST
public:
	PlugParamIndex GetNumParameters() {return 0;}
	void GetParamName(UINT, LPSTR, UINT) {}
	void ToggleEditor() {}
	BOOL HasEditor() {return FALSE;}
	UINT GetNumCommands() {return 0;}
	void GetPluginType(LPSTR) {}
	PlugParamIndex GetNumPrograms() {return 0;}
	bool GetProgramNameIndexed(long, long, char*) {return false;}
	CString GetFormattedProgramName(VstInt32 index, bool allowFallback = false) { return "" };
	void SetParameter(PlugParamIndex nIndex, PlugParamValue fValue) {}
	void GetParamLabel(UINT, LPSTR) {}
	void GetParamDisplay(UINT, LPSTR) {}
	PlugParamValue GetParameter(PlugParamIndex nIndex) {return 0;}
	bool LoadProgram(CString) {return false;}
	bool SaveProgram(CString) {return false;}
	void SetCurrentProgram(UINT) {}
	BOOL ExecuteCommand(UINT) {return FALSE;}
	void SetSlot(UINT) {}
	void UpdateMixStructPtr(void*) {}
	bool Bypass() {return false;}

#endif // NO_VST
};


//=====================
class CVstPluginManager
//=====================
{
#ifndef NO_VST
protected:
	PVSTPLUGINLIB m_pVstHead;

public:
	CVstPluginManager();
	~CVstPluginManager();

public:
	PVSTPLUGINLIB GetFirstPlugin() const { return m_pVstHead; }
	BOOL IsValidPlugin(const VSTPLUGINLIB *pLib);
	PVSTPLUGINLIB AddPlugin(LPCSTR pszDllPath, BOOL bCache=TRUE, const bool checkFileExistence = false, CString* const errStr = 0);
	bool RemovePlugin(PVSTPLUGINLIB);
	BOOL CreateMixPlugin(PSNDMIXPLUGIN, CSoundFile*);
	void OnIdle();
	static void ReportPlugException(LPCSTR format,...);

protected:
	void EnumerateDirectXDMOs();

protected:
	VstIntPtr VstCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	VstIntPtr VstFileSelector(const bool destructor, VstFileSelect *pFileSel, const AEffect *effect);
	static VstIntPtr VSTCALLBACK MasterCallBack(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	static BOOL __cdecl CreateMixPluginProc(PSNDMIXPLUGIN, CSoundFile*);
	VstTimeInfo timeInfo;	//rewbs.VSTcompliance

public:
	static char s_szHostProductString[64];
	static char s_szHostVendorString[64];
	static VstIntPtr s_nHostVendorVersion;

#else // NO_VST
public:
	PVSTPLUGINLIB AddPlugin(LPCSTR, BOOL =TRUE, const bool = false, CString* const = 0) {return 0;}
	PVSTPLUGINLIB GetFirstPlugin() const { return 0; }
	void OnIdle() {}
#endif // NO_VST
};


#endif // _VST_PLUGIN_MANAGER_H_
