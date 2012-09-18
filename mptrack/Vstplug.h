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
#include "../soundlib/plugins/PluginMixBuffer.h"

//#define kBuzzMagic	'Buzz'
#define kDmoMagic	'DXMO'


class CVstPluginManager;
class CVstPlugin;
class CVstEditor;
class Cfxp;				//rewbs.VSTpresets
class CModDoc;
class CSoundFile;


#ifndef NO_VST
	typedef AEffect * (VSTCALLBACK * PVSTPLUGENTRY)(audioMasterCallback);
#endif // NO_VST


struct VSTPluginLib
{
	enum PluginCategory
	{
		// Same plugin categories as defined in VST SDK
		catUnknown = 0,
		catEffect,			// Simple Effect
		catSynth,			// VST Instrument (Synths, samplers,...)
		catAnalysis,		// Scope, Tuner, ...
		catMastering,		// Dynamics, ...
		catSpacializer,		// Panners, ...
		catRoomFx,			// Delays and Reverbs
		catSurroundFx,		// Dedicated surround processor
		catRestoration,		// Denoiser, ...
		catOfflineProcess,	// Offline Process
		catShell,			// Plug-in is container of other plug-ins
		catGenerator,		// Tone Generator, ...
		// Custom categories
		catDMO,				// DirectX media object plugin

		numCategories,
	};

	VSTPluginLib *pPrev, *pNext;
	VstInt32 dwPluginId1;
	VstInt32 dwPluginId2;
	bool isInstrument;
	PluginCategory category;
	CVstPlugin *pPluginsList;
	CHAR szLibraryName[_MAX_FNAME];
	CHAR szDllPath[_MAX_PATH];

	VSTPluginLib(const CHAR *dllPath = nullptr)
	{
		pPrev = pNext = nullptr;
		dwPluginId1 = dwPluginId2 = 0;
		isInstrument = false;
		pPluginsList = nullptr;
		category = catUnknown;
		if(dllPath != nullptr)
		{
			lstrcpyn(szDllPath, dllPath, CountOf(szDllPath));
			StringFixer::SetNullTerminator(szDllPath);
		}
	}

	uint32 EncodeCacheFlags()
	{
		return (isInstrument ? 1 : 0) | (category << 1);
	}

	void DecodeCacheFlags(uint32 flags)
	{
		category = static_cast<PluginCategory>(flags >> 1);
		if(category >= numCategories)
		{
			category = catUnknown;
		}
		if(flags & 1)
		{
			isInstrument = true;
			category = catSynth;
		}
	}
};


struct VSTInstrChannel
{
	int32 midiPitchBendPos;		// Current Pitch Wheel position, in 16.11 fixed point format. Lowest bit is used for indicating that vibrato was applied. Vibrato offset itself is not stored in this value.
	uint16 currentProgram;
	uint16 currentBank; //rewbs.MidiBank
	uint8 uNoteOnMap[128][MAX_CHANNELS];
};


#ifndef NO_VST
#include "../soundlib/plugins/PluginEventQueue.h"
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
		// Number of MIDI events that can be sent to a plugin at once (the internal queue is not affected by this number, it can hold any number of events)
		vstNumProcessEvents = 256,

		// Pitch wheel constants
		vstPitchBendShift	= 12,		// Use lowest 12 bits for fractional part and vibrato flag => 16.11 fixed point precision
		vstPitchBendMask	= (~1),
		vstVibratoFlag		= 1,
	}; 

	CVstPlugin *m_pNext, *m_pPrev;
	HINSTANCE m_hLibrary;
	VSTPluginLib *m_pFactory;
	SNDMIXPLUGIN *m_pMixStruct;
	AEffect *m_pEffect;
	void (*m_pProcessFP)(AEffect*, float**, float**, VstInt32); //Function pointer to AEffect processReplacing if supported, else process.
	CAbstractVstEditor *m_pEditor;		//rewbs.defaultPlugGUI
	CModDoc *m_pModDoc;			 //rewbs.plugDocAware
	CSoundFile *m_pSndFile;			 //rewbs.plugDocAware

	size_t m_nRefCount;
	static const uint32 nInvalidSampleRate = UINT_MAX;
	uint32 m_nSampleRate;
	SNDMIXPLUGINSTATE m_MixState;
	uint32 m_nInputs, m_nOutputs;
	int32 m_nEditorX, m_nEditorY;

	float m_fGain;
	PLUGINDEX m_nSlot;
	bool m_bSongPlaying; //rewbs.VSTCompliance
	bool m_bPlugResumed; //rewbs.VSTCompliance
	bool m_bIsVst2;
	bool m_bIsInstrument;

	VSTInstrChannel m_MidiCh[16];						// MIDI channel state
	PluginMixBuffer<float, MIXBUFFERSIZE> mixBuffer;	// Float buffers (input and output) for plugins
	int32 m_MixBuffer[MIXBUFFERSIZE * 2 + 2];			// Stereo interleaved
	PluginEventQueue<vstNumProcessEvents> vstEvents;	// MIDI events that should be sent to the plugin

public:
	CVstPlugin(HINSTANCE hLibrary, VSTPluginLib *pFactory, SNDMIXPLUGIN *pMixPlugin, AEffect *pEffect);
	virtual ~CVstPlugin();
	void Initialize(CSoundFile* pSndFile);

public:
	VSTPluginLib *GetPluginFactory() const { return m_pFactory; }
	bool HasEditor();
	VstInt32 GetNumPrograms();
	PlugParamIndex GetNumParameters();
	VstInt32 GetCurrentProgram();
	VstInt32 GetNumProgramCategories();	//rewbs.VSTpresets
	CString GetFormattedProgramName(VstInt32 index, bool allowFallback = false);
	bool LoadProgram(CString fileName);
	bool SaveProgram(CString fileName);
	VstInt32 GetUID();			//rewbs.VSTpresets
	VstInt32 GetVersion();		//rewbs.VSTpresets
	bool GetParams(float* param, VstInt32 min, VstInt32 max); 	//rewbs.VSTpresets
	bool RandomizeParams(PlugParamIndex minParam = 0, PlugParamIndex maxParam = 0); 	//rewbs.VSTpresets
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
	BOOL GetCommandName(UINT index, LPSTR pszName);
	CAbstractVstEditor* GetEditor(); //rewbs.defaultPlugGUI

	void Bypass(bool bypass = true);
	bool IsBypassed() const { return m_pMixStruct->IsBypassed(); };

	bool isInstrument();
	bool CanRecieveMidiEvents();

	size_t GetOutputPlugList(vector<CVstPlugin *> &list);
	size_t GetInputPlugList(vector<CVstPlugin *> &list);
	size_t GetInputInstrumentList(vector<INSTRUMENTINDEX> &list);
	size_t GetInputChannelList(vector<CHANNELINDEX> &list);

public:
	size_t AddRef() { return ++m_nRefCount; }
	size_t Release();
	void SaveAllParameters();
	void RestoreAllParameters(long nProg=-1); //rewbs.plugDefaultProgram - added param 
	void RecalculateGain();
	void Process(float *pOutL, float *pOutR, size_t nSamples);
	void Init(unsigned long nFreq, int bReset);
	bool MidiSend(DWORD dwMidiCode);
	void MidiCC(uint8 nMidiCh, MIDIEvents::MidiCC nController, uint8 nParam, CHANNELINDEX trackChannel);
	void MidiPitchBend(uint8 nMidiCh, int32 increment, int8 pwd);
	void MidiVibrato(uint8 nMidiCh, int32 depth, int8 pwd);
	void MidiCommand(uint8 nMidiCh, uint8 nMidiProg, uint16 wMidiBank, uint16 note, uint16 vol, CHANNELINDEX trackChannel);
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
	void SetDryRatio(UINT param);
	void AutomateParameter(PlugParamIndex param);

	// Check whether a VST parameter can be automated
	bool CanAutomateParameter(PlugParamIndex index);

	void SetZxxParameter(UINT nParam, UINT nValue);
	UINT GetZxxParameter(UINT nParam); //rewbs.smoothVST

private:
	void MidiPitchBend(uint8 nMidiCh, int32 pitchBendPos);
	// Converts a 14-bit MIDI pitch bend position to a 16.11 fixed point pitch bend position
	static int32 EncodePitchBendParam(int32 position) { return (position << vstPitchBendShift); }
	// Converts a 16.11 fixed point pitch bend position to a 14-bit MIDI pitch bend position
	static int16 DecodePitchBendParam(int32 position) { return static_cast<int16>(position >> vstPitchBendShift); }
	// Apply Pitch Wheel Depth (PWD) to some MIDI pitch bend value.
	static inline void ApplyPitchWheelDepth(int32 &value, int8 pwd);

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
	void GetPluginType(LPSTR) {}
	VstInt32 GetNumPrograms() { return 0; }
	VstInt32 GetCurrentProgram() { return 0; }
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
	VSTPluginLib *m_pVstHead;

public:
	CVstPluginManager();
	~CVstPluginManager();

public:
	VSTPluginLib *GetFirstPlugin() const { return m_pVstHead; }
	BOOL IsValidPlugin(const VSTPluginLib *pLib);
	VSTPluginLib *AddPlugin(LPCSTR pszDllPath, bool fromCache = true, const bool checkFileExistence = false, CString* const errStr = 0);
	bool RemovePlugin(VSTPluginLib *);
	BOOL CreateMixPlugin(SNDMIXPLUGIN *, CSoundFile *);
	void OnIdle();
	static void ReportPlugException(LPCSTR format,...);

protected:
	void EnumerateDirectXDMOs();
	void LoadPlugin(const char *pluginPath, AEffect *&effect, HINSTANCE &library);

protected:
	VstIntPtr VstCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	VstIntPtr VstFileSelector(bool destructor, VstFileSelect *fileSel, const AEffect *effect);
	static VstIntPtr VSTCALLBACK MasterCallBack(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	static BOOL __cdecl CreateMixPluginProc(SNDMIXPLUGIN *, CSoundFile *);
	VstTimeInfo timeInfo;	//rewbs.VSTcompliance

public:
	static char s_szHostProductString[64];
	static char s_szHostVendorString[64];
	static VstIntPtr s_nHostVendorVersion;

#else // NO_VST
public:
	VSTPluginLib *AddPlugin(LPCSTR, bool = true, const bool = false, CString* const = 0) {return 0;}
	VSTPluginLib *GetFirstPlugin() const { return 0; }
	void OnIdle() {}
#endif // NO_VST
};
