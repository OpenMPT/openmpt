/*
 * Vstplug.h
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

#include "../soundlib/Snd_defs.h"
#include "../soundlib/plugins/PluginMixBuffer.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "../soundlib/plugins/PluginEventQueue.h"
#include "../soundlib/Mixer.h"

OPENMPT_NAMESPACE_BEGIN


class CVstPluginManager;
#ifdef MODPLUG_TRACKER
class CModDoc;
#endif // MODPLUG_TRACKER
class CSoundFile;


//==================================
class CVstPlugin: public IMidiPlugin
//==================================
{
protected:

	enum
	{
		// Number of MIDI events that can be sent to a plugin at once (the internal queue is not affected by this number, it can hold any number of events)
		vstNumProcessEvents = 256,
	};

	HINSTANCE m_hLibrary;
	AEffect &m_Effect;
	void (*m_pProcessFP)(AEffect*, float**, float**, VstInt32); //Function pointer to AEffect processReplacing if supported, else process.

	double lastBarStartPos;
	float m_fGain;
	uint32 m_nSampleRate;
	bool m_bSongPlaying : 1;
	bool m_bPlugResumed : 1;
	bool m_bIsVst2 : 1;
	bool m_bIsInstrument : 1;
	bool m_isInitialized : 1;
	bool m_bNeedIdle : 1;

	PluginMixBuffer<float, MIXBUFFERSIZE> m_mixBuffer;	// Float buffers (input and output) for plugins
	mixsample_t m_MixBuffer[MIXBUFFERSIZE * 2 + 2];		// Stereo interleaved
	PluginEventQueue<vstNumProcessEvents> vstEvents;	// MIDI events that should be sent to the plugin

	static VstTimeInfo timeInfo;

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
	void HardAllNotesOff();
	void NotifySongPlaying(bool playing);
	bool IsSongPlaying() const { return m_bSongPlaying; }
	bool IsResumed() const { return m_bPlugResumed; }
	void Resume();
	void Suspend();
	void SetDryRatio(uint32 param);
	void AutomateParameter(PlugParamIndex param);

	// Check whether a VST parameter can be automated
	bool CanAutomateParameter(PlugParamIndex index);

	int GetNumInputChannels() const { return m_Effect.numInputs; }
	int GetNumOutputChannels() const { return m_Effect.numOutputs; }

	void BeginSetProgram(int32 program);
	void EndSetProgram();

protected:
	// Helper function for retreiving parameter name / label / display
	CString GetParamPropertyString(VstInt32 param, VstInt32 opcode);

	// Set up input / output buffers.
	bool InitializeIOBuffers();

	// Process incoming and outgoing VST events.
	void ProcessVSTEvents();
	void ReceiveVSTEvents(const VstEvents *events);

	void ProcessMixOps(float *pOutL, float *pOutR, float *leftPlugOutput, float *rightPlugOutput, size_t nSamples);

	void ReportPlugException(std::wstring text) const;

public:
	static VstIntPtr VSTCALLBACK MasterCallBack(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
protected:
	VstIntPtr VstFileSelector(bool destructor, VstFileSelect *fileSel);
};


OPENMPT_NAMESPACE_END

#endif // NO_VST