#ifndef _VST_PLUGIN_MANAGER_H_
#define _VST_PLUGIN_MANAGER_H_

#include <aeffectx.h>			// VST

#define kBuzzMagic	'Buzz'
#define kDmoMagic	'DXMO'

class CVstPluginManager;
class CVstPlugin;
class CVstEditor;

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
	BYTE uNoteOnMap[128/8];
	UINT nProgram;
} VSTINSTCH, *PVSTINSTCH;


//=================================
class CVstPlugin: public IMixPlugin
//=================================
{
	friend class CVstEditor;
	friend class CVstPluginManager;
public:
	enum {VSTEVENT_QUEUE_LEN=128}; 

protected:
	ULONG m_nRefCount;
	CVstPlugin *m_pNext, *m_pPrev;
	HINSTANCE m_hLibrary;
	PVSTPLUGINLIB m_pFactory;
	PSNDMIXPLUGIN m_pMixStruct;
	AEffect *m_pEffect;
	CVstEditor *m_pEditor;
	UINT m_nSampleRate;
	BOOL m_bIsVst2;
	SNDMIXPLUGINSTATE m_MixState;
	UINT m_nInputs, m_nOutputs;
	VstEvents *m_pEvList;
	VSTINSTCH m_MidiCh[16];
	float *m_pTempBuffer;
	float **m_pInputs;
	float **m_pOutputs;
	int m_nEditorX, m_nEditorY;
	int m_MixBuffer[MIXBUFFERSIZE*2+2];		// Stereo interleaved
	float m_FloatBuffer[MIXBUFFERSIZE*3+2];	// 2ch separated
	VstMidiEvent m_ev_queue[VSTEVENT_QUEUE_LEN];

public:
	CVstPlugin(HINSTANCE hLibrary, PVSTPLUGINLIB pFactory, PSNDMIXPLUGIN pMixPlugin, AEffect *pEffect);
	virtual ~CVstPlugin();
	void Initialize();

public:
	PVSTPLUGINLIB GetPluginFactory() const { return m_pFactory; }
	BOOL HasEditor();
	UINT GetNumPrograms();
	UINT GetNumParameters();
	UINT GetCurrentProgram();
	VOID SetCurrentProgram(UINT nIndex);
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

public: // IMixPlugin interface
	int AddRef() { return ++m_nRefCount; }
	int Release();
	void SaveAllParameters();
	void RestoreAllParameters();
	void Process(float *pOutL, float *pOutR, unsigned long nSamples);
	void Init(unsigned long nFreq, int bReset);
	void MidiSend(DWORD dwMidiCode);
	void MidiCommand(UINT nMidiCh, UINT nMidiProg, UINT note, UINT vol);
	void SetZxxParameter(UINT nParam, UINT nValue);
};


//=====================
class CVstPluginManager
//=====================
{
protected:
	PVSTPLUGINLIB m_pVstHead;
	BOOL m_bNeedIdle;

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
	BOOL NeedIdle() const { return m_bNeedIdle; }

protected:
	VOID EnumerateDirectXDMOs();

protected:
	long VstCallback(AEffect *effect, long opcode, long index, long value, void *ptr, float opt);
	static long VSTCALLBACK MasterCallBack(AEffect *effect, long opcode, long index, long value, void *ptr, float opt);
	static BOOL __cdecl CreateMixPluginProc(PSNDMIXPLUGIN);
};


//==============================
class CVstEditor: public CDialog
//==============================
{
protected:
	CVstPlugin *m_pVstPlugin;

public:
	CVstEditor(CVstPlugin *pPlugin);
	virtual ~CVstEditor();
	virtual VOID OnOK();
	virtual VOID OnCancel();
	BOOL OpenEditor(CWnd *parent);
	VOID DoClose();
	afx_msg void OnClose();
	DECLARE_MESSAGE_MAP()
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
