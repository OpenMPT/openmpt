#ifndef _VST_PLUGIN_MANAGER_H_
#define _VST_PLUGIN_MANAGER_H_

#include <aeffectx.h>			// VST

#define kBuzzMagic	'Buzz'
#define kDmoMagic	'DXMO'

class CVstPluginManager;
class CVstPlugin;
class CVstEditor;
class Cfxp;				//rewbs.VSTpresets

enum {
	effBuzzGetNumCommands=0x1000,
	effBuzzGetCommandName,
	effBuzzExecuteCommand,
};

typedef AEffect * (VSTCALLBACK * PVSTPLUGENTRY)(audioMasterCallback);


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
public:
	enum {VSTEVENT_QUEUE_LEN=256}; 

protected:
	ULONG m_nRefCount;
	CVstPlugin *m_pNext, *m_pPrev;
	HINSTANCE m_hLibrary;
	PVSTPLUGINLIB m_pFactory;
	PSNDMIXPLUGIN m_pMixStruct;
	AEffect *m_pEffect;
	CAbstractVstEditor *m_pEditor;		//rewbs.defaultPlugGUI
	UINT m_nSampleRate;
	BOOL m_bIsVst2;
	SNDMIXPLUGINSTATE m_MixState;
	UINT m_nInputs, m_nOutputs;
	VstEvents *m_pEvList;
	VSTINSTCH m_MidiCh[16];
	float **m_pTempBuffer;					//rewbs.dryRatio: changed from * to **
	float **m_pInputs;
	float **m_pOutputs;
	int m_nEditorX, m_nEditorY;
	int m_MixBuffer[MIXBUFFERSIZE*2+2];		// Stereo interleaved
	float m_FloatBuffer[MIXBUFFERSIZE*32+31];	// 2ch separated + up to 32 VSTi outputs...
	VstMidiEvent m_ev_queue[VSTEVENT_QUEUE_LEN];
	UINT m_nPreviousMidiChan; //rewbs.VSTCompliance

public:
	CVstPlugin(HINSTANCE hLibrary, PVSTPLUGINLIB pFactory, PSNDMIXPLUGIN pMixPlugin, AEffect *pEffect);
	virtual ~CVstPlugin();
	void Initialize();

public:
	PVSTPLUGINLIB GetPluginFactory() const { return m_pFactory; }
	BOOL HasEditor();
	long GetNumPrograms();
	long GetNumParameters();
	long GetCurrentProgram();
	long GetNumProgramCategories();	//rewbs.VSTpresets
	long GetProgramNameIndexed(long index, long category, char *text);	//rewbs.VSTpresets
	bool LoadProgram(CString fileName);
	bool SaveProgram(CString fileName);
	long GetUID();			//rewbs.VSTpresets
	long GetVersion();		//rewbs.VSTpresets
	bool GetParams(float* param, long min, long max); 	//rewbs.VSTpresets
	bool RandomizeParams(long minParam=0, long maxParam=0); 	//rewbs.VSTpresets

	VOID SetCurrentProgram(UINT nIndex);
//rewbs.VSTCompliance: Eric's non standard preset stuff:
// -> CODE#0002
// -> DESC="VST plugins presets"
	//VOID GetProgramName(UINT nIndex, LPSTR pszName, UINT cbSize);
	//BOOL SavePreset(LPCSTR lpszFileName);
	//BOOL LoadPreset(LPCSTR lpszFileName);
// -! NEW_FEATURE#0002
	FLOAT GetParameter(UINT nIndex);
	VOID SetParameter(UINT nIndex, FLOAT fValue);
	VOID GetParamName(UINT nIndex, LPSTR pszName, UINT cbSize);
	VOID GetParamLabel(UINT nIndex, LPSTR pszLabel);
	VOID GetParamDisplay(UINT nIndex, LPSTR pszDisplay);
	long Dispatch(long opCode, long index, long value, void *ptr, float opt);
	VOID ToggleEditor();
	VOID GetPluginType(LPSTR pszType);
	BOOL GetDefaultEffectName(LPSTR pszName);
	UINT GetNumCommands();
	BOOL GetCommandName(UINT index, LPSTR pszName);
	BOOL ExecuteCommand(UINT nIndex);
	CAbstractVstEditor* GetEditor(); //rewbs.defaultPlugGUI
	BOOL GetSpeakerArrangement(); //rewbs.VSTCompliance

	BOOL isInstrument(); // ericus 18/02/2005

public: // IMixPlugin interface
	int AddRef() { return ++m_nRefCount; }
	int Release();
	void SaveAllParameters();
	void RestoreAllParameters();
	void ProcessVSTEvents(); //rewbs.VSTiNoteHoldonStopFix
	void ClearVSTEvents(); //rewbs.VSTiNoteHoldonStopFix
	void Process(float *pOutL, float *pOutR, unsigned long nSamples);
	void Init(unsigned long nFreq, int bReset);
	bool MidiSend(DWORD dwMidiCode);
	void MidiCommand(UINT nMidiCh, UINT nMidiProg, WORD wMidiBank, UINT note, UINT vol, UINT trackChan);
	void HardAllNotesOff(); //rewbs.VSTiNoteHoldonStopFix
	bool isPlaying(UINT note, UINT midiChn, UINT trackerChn);	//rewbs.instroVST
	bool MoveNote(UINT note, UINT midiChn, UINT sourceTrackerChn, UINT destTrackerChn); //rewbs.instroVST
	bool m_bNeedIdle; //rewbs.VSTCompliance
	

	void SetZxxParameter(UINT nParam, UINT nValue);
	UINT GetZxxParameter(UINT nParam); //rewbs.smoothVST

	VstSpeakerArrangement speakerArrangement;  //rewbs.VSTcompliance
};


//=====================
class CVstPluginManager
//=====================
{
protected:
	PVSTPLUGINLIB m_pVstHead;

public:
	CVstPluginManager();
	~CVstPluginManager();

public:
	PVSTPLUGINLIB GetFirstPlugin() const { return m_pVstHead; }
	BOOL IsValidPlugin(const VSTPLUGINLIB *pLib);
	PVSTPLUGINLIB AddPlugin(LPCSTR pszDllPath, BOOL bCache=TRUE);
	BOOL RemovePlugin(PVSTPLUGINLIB);
	BOOL CreateMixPlugin(PSNDMIXPLUGIN);
	VOID OnIdle();

protected:
	VOID EnumerateDirectXDMOs();

protected:
	long VstCallback(AEffect *effect, long opcode, long index, long value, void *ptr, float opt);
	static long VSTCALLBACK MasterCallBack(AEffect *effect, long opcode, long index, long value, void *ptr, float opt);
	static BOOL __cdecl CreateMixPluginProc(PSNDMIXPLUGIN);
	VstTimeInfo timeInfo;	//rewbs.VSTcompliance
};


//====================================
class CSelectPluginDlg: public CDialog
//====================================
{
protected:
	PSNDMIXPLUGIN m_pPlugin;
	CTreeCtrl m_treePlugins;

public:
	CSelectPluginDlg(PSNDMIXPLUGIN, CWnd *parent);
	VOID DoClose();
	VOID UpdatePluginsList();
	bool VerifyPlug(PVSTPLUGINLIB plug);
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual VOID OnOK();
	afx_msg void OnAddPlugin();
	afx_msg void OnRemovePlugin();
	afx_msg void OnSelChanged(NMHDR *pNotifyStruct, LRESULT * result);
	afx_msg void OnSelDblClk(NMHDR *pNotifyStruct, LRESULT * result);
	DECLARE_MESSAGE_MAP()
};


#endif // _VST_PLUGIN_MANAGER_H_
