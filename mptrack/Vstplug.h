/*
 * vstplug.h
 * ---------
 * Purpose: Plugin handling (loading and processing plugins)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifndef NO_VST
	#define VST_FORCE_DEPRECATED 0
	#include <pluginterfaces/vst2.x/aeffectx.h>			// VST
#endif

#include "../soundlib/Snd_defs.h"
#include "../soundlib/PluginMixBuffer.h"

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


#ifndef NO_VST
#include "../soundlib/PluginEventQueue.h"
#endif // NO_VST


//=================================
class CVstPlugin: public IMixPlugin
//=================================
{
	friend class CAbstractVstEditor;	//rewbs.defaultPlugGUI
	friend class CVstPluginManager;
#ifndef NO_VST
protected:

	enum
	{
		VstEventQueueLength = 256,
	}; 

	ULONG m_nRefCount;
	CVstPlugin *m_pNext, *m_pPrev;
	HINSTANCE m_hLibrary;
	VSTPLUGINLIB *m_pFactory;
	SNDMIXPLUGIN *m_pMixStruct;
	AEffect *m_pEffect;
	void (*m_pProcessFP)(AEffect*, float**, float**, VstInt32); //Function pointer to AEffect processReplacing if supported, else process.
	CAbstractVstEditor *m_pEditor;		//rewbs.defaultPlugGUI
	static const UINT nInvalidSampleRate = UINT_MAX;
	UINT m_nSampleRate;
	bool m_bIsVst2;
	SNDMIXPLUGINSTATE m_MixState;
	UINT m_nInputs, m_nOutputs;
	VSTINSTCH m_MidiCh[16];
	short m_nMidiPitchBendPos[16];

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
	PLUGINDEX m_nSlot;
	float m_fGain;
	bool m_bIsInstrument;

	int m_nEditorX, m_nEditorY;

	PluginMixBuffer<float, MIXBUFFERSIZE> mixBuffer;	// Float buffers (input and output) for plugins
	int m_MixBuffer[MIXBUFFERSIZE * 2 + 2];				// Stereo interleaved
	PluginEventQueue<VstEventQueueLength> vstEvents;	// MIDI events that should be sent to the plugin

public:
	CVstPlugin(HINSTANCE hLibrary, VSTPLUGINLIB *pFactory, SNDMIXPLUGIN *pMixPlugin, AEffect *pEffect);
	virtual ~CVstPlugin();
	void Initialize(CSoundFile* pSndFile);

public:
	PVSTPLUGINLIB GetPluginFactory() const { return m_pFactory; }
	bool HasEditor();
	long GetNumPrograms();
	PlugParamIndex GetNumParameters();
	long GetCurrentProgram();
	long GetNumProgramCategories();	//rewbs.VSTpresets
	CString GetFormattedProgramName(VstInt32 index, bool allowFallback = false);
	bool LoadProgram(CString fileName);
	bool SaveProgram(CString fileName);
	VstInt32 GetUID();			//rewbs.VSTpresets
	VstInt32 GetVersion();		//rewbs.VSTpresets
	bool GetParams(float* param, VstInt32 min, VstInt32 max); 	//rewbs.VSTpresets
	bool RandomizeParams(PlugParamIndex minParam = 0, PlugParamIndex maxParam = 0); 	//rewbs.VSTpresets
	bool isModified() {return m_bModified;}
	inline CModDoc* GetModDoc() {return m_pModDoc;}
	inline CSoundFile* GetSoundFile() {return m_pSndFile;}
	PLUGINDEX FindSlot();
	void SetSlot(PLUGINDEX slot);
	PLUGINDEX GetSlot();
	void UpdateMixStructPtr(SNDMIXPLUGIN *);

	void SetEditorPos(int x, int y) { m_nEditorX = x; m_nEditorY = y; }
	void GetEditorPos(int &x, int &y) const { x = m_nEditorX; y = m_nEditorY; }

	void SetCurrentProgram(UINT nIndex);
	PlugParamValue GetParameter(PlugParamIndex nIndex);
	void SetParameter(PlugParamIndex nIndex, PlugParamValue fValue);

	CString GetFormattedParamName(PlugParamIndex param);
	CString GetFormattedParamValue(PlugParamIndex param);
	CString GetParamName(PlugParamIndex param) { return GetParamPropertyString(param, effGetParamName); };
	CString GetParamLabel(PlugParamIndex param) { return GetParamPropertyString(param, effGetParamLabel); };
	CString GetParamDisplay(PlugParamIndex param) { return GetParamPropertyString(param, effGetParamDisplay); };

	VstIntPtr Dispatch(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	void ToggleEditor();
	void GetPluginType(LPSTR pszType);
	BOOL GetDefaultEffectName(LPSTR pszName);
	UINT GetNumCommands();
	BOOL GetCommandName(UINT index, LPSTR pszName);
	BOOL ExecuteCommand(UINT nIndex);
	CAbstractVstEditor* GetEditor(); //rewbs.defaultPlugGUI
	bool GetSpeakerArrangement(); //rewbs.VSTCompliance

	void Bypass(bool bypass = true);
	bool IsBypassed() const { return m_pMixStruct->IsBypassed(); };

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
	void RecalculateGain();
	void Process(float *pOutL, float *pOutR, size_t nSamples);
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
	void AutomateParameter(PlugParamIndex param);

	// Check whether a VST parameter can be automated
	bool CanAutomateParameter(PlugParamIndex index);

	void SetZxxParameter(UINT nParam, UINT nValue);
	UINT GetZxxParameter(UINT nParam); //rewbs.smoothVST

	VstSpeakerArrangement speakerArrangement;  //rewbs.VSTcompliance

private:
	short getMIDI14bitValueFromShort(short value); 
	void MidiPitchBend(UINT nMidiCh, short pitchBendPos);

	bool GetProgramNameIndexed(VstInt32 index, VstIntPtr category, char *text);	//rewbs.VSTpresets

	// Helper function for retreiving parameter name / label / display
	CString GetParamPropertyString(VstInt32 param, VstInt32 opcode);

	// Set up input / output buffers.
	bool InitializeIOBuffers();

	// Process incoming and outgoing VST events.
	void ProcessVSTEvents();
	void ReceiveVSTEvents(const VstEvents *events) const;

	void ProcessMixOps(float *pOutL, float *pOutR, size_t nSamples);

#else // case: NO_VST
public:
	PlugParamIndex GetNumParameters() { return 0; }
	void ToggleEditor() {}
	bool HasEditor() { return false; }
	UINT GetNumCommands() { return 0; }
	void GetPluginType(LPSTR) {}
	PlugParamIndex GetNumPrograms() { return 0; }
	bool GetProgramNameIndexed(long, long, char*) { return false; }
	CString GetFormattedProgramName(VstInt32, bool = false) { return ""; }
	void SetParameter(PlugParamIndex, PlugParamValue) {}
	
	bool CanAutomateParameter(PlugParamIndex index) { return false; }

	CString GetFormattedParamName(PlugParamIndex) { return ""; };
	CString GetFormattedParamValue(PlugParamIndex){ return ""; };
	CString GetParamName(PlugParamIndex) { return ""; };
	CString GetParamLabel(PlugParamIndex) { return ""; };
	CString GetParamDisplay(PlugParamIndex) { return ""; };

	PlugParamValue GetParameter(PlugParamIndex) { return 0; }
	bool LoadProgram(CString) {return false;}
	bool SaveProgram(CString) {return false;}
	void SetCurrentProgram(UINT) {}
	BOOL ExecuteCommand(UINT) {return FALSE;}
	void SetSlot(UINT) {}
	void UpdateMixStructPtr(void*) {}
	void Bypass(bool = true) { }
	bool IsBypassed() const { return false; }

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
	BOOL CreateMixPlugin(SNDMIXPLUGIN *, CSoundFile *);
	void OnIdle();
	static void ReportPlugException(LPCSTR format,...);

protected:
	void EnumerateDirectXDMOs();

protected:
	VstIntPtr VstCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	VstIntPtr VstFileSelector(const bool destructor, VstFileSelect *pFileSel, const AEffect *effect);
	static VstIntPtr VSTCALLBACK MasterCallBack(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	static BOOL __cdecl CreateMixPluginProc(SNDMIXPLUGIN *, CSoundFile *);
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
