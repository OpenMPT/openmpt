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
#include "../soundlib/plugins/PlugInterface.h"

OPENMPT_NAMESPACE_BEGIN

//#define kBuzzMagic	'Buzz'
#define kDmoMagic	'DXMO'


class CVstPluginManager;
#ifdef MODPLUG_TRACKER
class CModDoc;
#endif // MODPLUG_TRACKER
class CSoundFile;


struct VSTPluginLib
{
public:
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

public:
	IMixPlugin *pPluginsList;		// Pointer to first plugin instance (this instance carries pointers to other instances)
	mpt::PathString libraryName;	// Display name
	mpt::PathString dllPath;		// Full path name
	mpt::ustring tags;				// User tags
	int32 pluginId1;				// Plugin type (kEffectMagic, kDmoMagic)
	int32 pluginId2;				// Plugin unique ID
	PluginCategory category;
	bool isInstrument : 1;
	bool useBridge : 1, shareBridgeInstance : 1;
protected:
	mutable uint8 dllBits;

public:
	VSTPluginLib(const mpt::PathString &dllPath, const mpt::PathString &libraryName, const mpt::ustring &tags)
		: pPluginsList(nullptr),
		libraryName(libraryName), dllPath(dllPath),
		tags(tags),
		pluginId1(0), pluginId2(0),
		category(catUnknown),
		isInstrument(false), useBridge(false), shareBridgeInstance(true),
		dllBits(0)
	{
	}

	// Check whether a plugin can be hosted inside OpenMPT or requires bridging
	uint8 GetDllBits(bool fromCache = true) const;
	bool IsNative(bool fromCache = true) const { return GetDllBits(fromCache) == sizeof(void *) * CHAR_BIT; }
	// Check if a plugin is native, and if it is currently unknown, assume that it is native. Use this function only for performance reasons
	// (e.g. if tons of unscanned plugins would slow down generation of the plugin selection dialog)
	bool IsNativeFromCache() const { return dllBits == sizeof(void *) * CHAR_BIT || dllBits == 0; }

	void WriteToCache() const;

	uint32 EncodeCacheFlags() const
	{
		// Format: 00000000.00000000.DDDDDDSB.CCCCCCCI
		return (isInstrument ? 1 : 0)
			| (category << 1)
			| (useBridge ? 0x100 : 0)
			| (shareBridgeInstance ? 0x200 : 0)
			| ((dllBits / 8) << 10);
	}

	void DecodeCacheFlags(uint32 flags)
	{
		category = static_cast<PluginCategory>((flags & 0xFF) >> 1);
		if(category >= numCategories)
		{
			category = catUnknown;
		}
		if(flags & 1)
		{
			isInstrument = true;
			category = catSynth;
		}
		useBridge = (flags & 0x100) != 0;
		shareBridgeInstance = (flags & 0x200) != 0;
		dllBits = ((flags >> 10) & 0x3F) * 8;
	}
};


OPENMPT_NAMESPACE_END
#ifndef NO_VST
#include "../soundlib/plugins/PluginEventQueue.h"
#include "../soundlib/Mixer.h"
#endif // NO_VST
OPENMPT_NAMESPACE_BEGIN


//=================================
class CVstPlugin: public IMixPlugin
//=================================
{
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

	struct VSTInstrChannel
	{
		int32  midiPitchBendPos;		// Current Pitch Wheel position, in 16.11 fixed point format. Lowest bit is used for indicating that vibrato was applied. Vibrato offset itself is not stored in this value.
		uint16 currentProgram;
		uint16 currentBank;
		uint8  noteOnMap[128][MAX_CHANNELS];

		void ResetProgram() { currentProgram = 0; currentBank = 0; }
	};

	HINSTANCE m_hLibrary;
	AEffect &m_Effect;
	void (*m_pProcessFP)(AEffect*, float**, float**, VstInt32); //Function pointer to AEffect processReplacing if supported, else process.

	uint32 m_nSampleRate;
	SNDMIXPLUGINSTATE m_MixState;

	double lastBarStartPos;
	float m_fGain;
	bool m_bSongPlaying : 1;
	bool m_bPlugResumed : 1;
	bool m_bIsVst2 : 1;
	bool m_bIsInstrument : 1;
	bool m_isInitialized : 1;
	bool m_bNeedIdle : 1;

	VSTInstrChannel m_MidiCh[16];						// MIDI channel state
	PluginMixBuffer<float, MIXBUFFERSIZE> mixBuffer;	// Float buffers (input and output) for plugins
	mixsample_t m_MixBuffer[MIXBUFFERSIZE * 2 + 2];		// Stereo interleaved
	PluginEventQueue<vstNumProcessEvents> vstEvents;	// MIDI events that should be sent to the plugin

public:
	const bool isBridged : 1;		// True if our built-in plugin bridge is being used.

public:
	CVstPlugin(HINSTANCE hLibrary, VSTPluginLib &factory, SNDMIXPLUGIN &mixPlugin, AEffect &effect, CSoundFile &sndFile);
	virtual ~CVstPlugin();
	void Initialize();

public:
	void Idle();
	bool HasEditor() const;
	int32 GetNumPrograms();
	int32 GetCurrentProgram();
	PlugParamIndex GetNumParameters();
	VstInt32 GetNumProgramCategories();
	bool LoadProgram(mpt::PathString fileName = mpt::PathString());
	bool SaveProgram();
	int32 GetUID() const;
	int32 GetVersion() const;
	// Check if programs should be stored as chunks or parameters
	bool ProgramsAreChunks() const { return (m_Effect.flags & effFlagsProgramChunks) != 0; }
	size_t GetChunk(char *(&chunk), bool isBank);
	void SetChunk(size_t size, char *chunk, bool isBank);
	bool GetParams(float* param, VstInt32 min, VstInt32 max);
	// If true, the plugin produces an output even if silence is being fed into it.
	bool ShouldProcessSilence() { return IsInstrument() || ((m_Effect.flags & effFlagsNoSoundInStop) == 0 && Dispatch(effGetTailSize, 0, 0, nullptr, 0.0f) != 1); }
	void ResetSilence() { m_MixState.ResetSilence(); }

	void SetCurrentProgram(int32 nIndex);
	PlugParamValue GetParameter(PlugParamIndex nIndex);
	void SetParameter(PlugParamIndex nIndex, PlugParamValue fValue);

	CString GetCurrentProgramName();
	void SetCurrentProgramName(const CString &name);
	CString GetProgramName(int32 program);

	CString GetParamName(PlugParamIndex param);
	CString GetParamLabel(PlugParamIndex param) { return GetParamPropertyString(param, effGetParamLabel); };
	CString GetParamDisplay(PlugParamIndex param) { return GetParamPropertyString(param, effGetParamDisplay); };

	VstIntPtr Dispatch(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	void ToggleEditor();
	CString GetDefaultEffectName();

	void Bypass(bool bypass = true);
	bool IsBypassed() const { return m_pMixStruct->IsBypassed(); };

	bool IsInstrument() const;
	bool CanRecieveMidiEvents();

	void CacheProgramNames(int32 firstProg, int32 lastProg);
	void CacheParameterNames(int32 firstParam, int32 lastParam);

public:
	void Release();
	void SaveAllParameters();
	void RestoreAllParameters(long nProg=-1);
	void RecalculateGain();
	void Process(float *pOutL, float *pOutR, size_t nSamples);
	float RenderSilence(size_t numSamples);
	bool MidiSend(uint32 dwMidiCode);
	bool MidiSysexSend(const char *message, uint32 length);
	void MidiCC(uint8 nMidiCh, MIDIEvents::MidiCC nController, uint8 nParam, CHANNELINDEX trackChannel);
	void MidiPitchBend(uint8 nMidiCh, int32 increment, int8 pwd);
	void MidiVibrato(uint8 nMidiCh, int32 depth, int8 pwd);
	void MidiCommand(uint8 nMidiCh, uint8 nMidiProg, uint16 wMidiBank, uint16 note, uint16 vol, CHANNELINDEX trackChannel);
	void HardAllNotesOff();
	bool IsPlaying(uint32 note, uint32 midiChn, uint32 trackerChn);
	void NotifySongPlaying(bool playing);
	bool IsSongPlaying() const { return m_bSongPlaying; }
	bool IsResumed() const { return m_bPlugResumed; }
	void Resume();
	void Suspend();
	void SetDryRatio(uint32 param);
	void AutomateParameter(PlugParamIndex param);

	// Check whether a VST parameter can be automated
	bool CanAutomateParameter(PlugParamIndex index);

	void SetZxxParameter(PlugParamIndex nParam, uint32 nValue);
	uint32 GetZxxParameter(PlugParamIndex nParam);

	int GetNumInputChannels() const { return m_Effect.numInputs; }
	int GetNumOutputChannels() const { return m_Effect.numOutputs; }

	void BeginSetProgram(int32 program);
	void EndSetProgram();

protected:
	void MidiPitchBend(uint8 nMidiCh, int32 pitchBendPos);
	// Converts a 14-bit MIDI pitch bend position to our internal pitch bend position representation
	static int32 EncodePitchBendParam(int32 position) { return (position << vstPitchBendShift); }
	// Converts the internal pitch bend position to a 14-bit MIDI pitch bend position
	static int16 DecodePitchBendParam(int32 position) { return static_cast<int16>(position >> vstPitchBendShift); }
	// Apply Pitch Wheel Depth (PWD) to some MIDI pitch bend value.
	static inline void ApplyPitchWheelDepth(int32 &value, int8 pwd);

	// Helper function for retreiving parameter name / label / display
	CString GetParamPropertyString(VstInt32 param, VstInt32 opcode);

	// Set up input / output buffers.
	bool InitializeIOBuffers();

	// Process incoming and outgoing VST events.
	void ProcessVSTEvents();
	void ReceiveVSTEvents(const VstEvents *events);

	void ProcessMixOps(float *pOutL, float *pOutR, float *leftPlugOutput, float *rightPlugOutput, size_t nSamples);

	void ReportPlugException(std::wstring text) const;

#else // case: NO_VST
public:
	PlugParamIndex GetNumParameters() { return 0; }
	void ToggleEditor() {}
	bool HasEditor() { return false; }
	VstInt32 GetNumPrograms() { return 0; }
	VstInt32 GetCurrentProgram() { return 0; }
	CString GetCurrentProgramName() { return _T(""); }
	void SetCurrentProgramName(const CString &) { }
	CString GetProgramName(int32) { return _T(""); }
	void SetParameter(PlugParamIndex, PlugParamValue) {}
	VstInt32 GetUID() const { return 0; }
	VstInt32 GetVersion() const { return 0; }
	bool ShouldProcessSilence() { return false; }
	void ResetSilence() { }

	bool CanAutomateParameter(PlugParamIndex) { return false; }

	CString GetParamName(PlugParamIndex) { return _T(""); };
	CString GetParamLabel(PlugParamIndex) { return _T(""); };
	CString GetParamDisplay(PlugParamIndex) { return _T(""); };

	PlugParamValue GetParameter(PlugParamIndex) { return 0; }
	bool LoadProgram(mpt::PathString = mpt::PathString()) { return false; }
	bool SaveProgram() { return false; }
	void SetCurrentProgram(VstInt32) {}
	void Bypass(bool = true) { }
	bool IsBypassed() const { return false; }
	bool IsSongPlaying() const { return false; }

	void SetEditorPos(int32, int32) { }
	void GetEditorPos(int32 &x, int32 &y) const { x = y = int32_min; }

	int GetNumInputChannels() const { return 0; }
	int GetNumOutputChannels() const { return 0; }

	bool ProgramsAreChunks() const { return false; };
	size_t GetChunk(char *(&), bool) { return 0; }
	void SetChunk(size_t, char *, bool) { }

	void BeginSetProgram(int32) { }
	void EndSetProgram() { }

#endif // NO_VST
};


//=====================
class CVstPluginManager
//=====================
{
#ifndef NO_VST
protected:
	std::vector<VSTPluginLib *> pluginList;

public:
	CVstPluginManager();
	~CVstPluginManager();

	typedef std::vector<VSTPluginLib *>::iterator iterator;
	typedef std::vector<VSTPluginLib *>::const_iterator const_iterator;

	iterator begin() { return pluginList.begin(); }
	const_iterator begin() const { return pluginList.begin(); }
	iterator end() { return pluginList.end(); }
	const_iterator end() const { return pluginList.end(); }
	void reserve(size_t num) { pluginList.reserve(num); }

	bool IsValidPlugin(const VSTPluginLib *pLib) const;
	VSTPluginLib *AddPlugin(const mpt::PathString &dllPath, const mpt::ustring &tags, bool fromCache = true, const bool checkFileExistence = false, std::wstring* const errStr = nullptr);
	bool RemovePlugin(VSTPluginLib *);
	bool CreateMixPlugin(SNDMIXPLUGIN &, CSoundFile &);
	void OnIdle();
	static void ReportPlugException(const std::wstring &msg);
	static void ReportPlugException(const std::string &msg);

protected:
	void EnumerateDirectXDMOs();
	AEffect *LoadPlugin(VSTPluginLib &plugin, HINSTANCE &library, bool forceBridge);

public:
	static VstIntPtr VSTCALLBACK MasterCallBack(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);

protected:
	VstIntPtr VstCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	VstIntPtr VstFileSelector(bool destructor, VstFileSelect *fileSel, const IMixPlugin *plugin);
	static bool CreateMixPluginProc(SNDMIXPLUGIN &, CSoundFile &);
	VstTimeInfo timeInfo;

public:
	static char s_szHostProductString[64];
	static char s_szHostVendorString[64];
	static VstInt32 s_nHostVendorVersion;

#else // NO_VST
public:
	VSTPluginLib *AddPlugin(const mpt::PathString &, const mpt::ustring &, bool = true, const bool = false, std::wstring* const = nullptr) { return 0; }

	const VSTPluginLib **begin() const { return nullptr; }
	const VSTPluginLib **end() const { return nullptr; }
	void reserve(size_t) { }

	void OnIdle() {}
#endif // NO_VST
};


OPENMPT_NAMESPACE_END
