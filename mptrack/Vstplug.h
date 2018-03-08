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
#include "../soundlib/plugins/PlugInterface.h"
#include "../soundlib/plugins/PluginEventQueue.h"
#include "../soundlib/Mixer.h"

OPENMPT_NAMESPACE_BEGIN


class CSoundFile;
struct SNDMIXPLUGIN;
struct VSTPluginLib;


class CVstPlugin: public IMidiPlugin
{
protected:

	enum
	{
		// Number of MIDI events that can be sent to a plugin at once (the internal queue is not affected by this number, it can hold any number of events)
		vstNumProcessEvents = 256,
	};

	HMODULE m_hLibrary;
	AEffect &m_Effect;
	AEffectProcessProc m_pProcessFP; //Function pointer to AEffect processReplacing if supported, else process.

	double lastBarStartPos;
	uint32 m_nSampleRate;

	bool m_bIsVst2 : 1;
	bool m_bIsInstrument : 1;
	bool m_isInitialized : 1;
	bool m_bNeedIdle : 1;
	bool m_positionChanged : 1;

	PluginEventQueue<vstNumProcessEvents> vstEvents;	// MIDI events that should be sent to the plugin

	VstTimeInfo timeInfo;

public:
	const bool isBridged : 1;		// True if our built-in plugin bridge is being used.

public:
	CVstPlugin(HMODULE hLibrary, VSTPluginLib &factory, SNDMIXPLUGIN &mixPlugin, AEffect &effect, CSoundFile &sndFile);
	~CVstPlugin();

	static AEffect *LoadPlugin(VSTPluginLib &plugin, HMODULE &library, bool forceBridge);

protected:
	void Initialize();

public:
	int32 GetUID() const override;
	int32 GetVersion() const override;
	void Idle() override;
	uint32 GetLatency() const override { return m_Effect.initialDelay; }

	// Check if programs should be stored as chunks or parameters
	bool ProgramsAreChunks() const override { return (m_Effect.flags & effFlagsProgramChunks) != 0; }
	ChunkData GetChunk(bool isBank) override;
	void SetChunk(const ChunkData &chunk, bool isBank) override;
	// If true, the plugin produces an output even if silence is being fed into it.
	//bool ShouldProcessSilence() { return IsInstrument() || ((m_Effect.flags & effFlagsNoSoundInStop) == 0 && Dispatch(effGetTailSize, 0, 0, nullptr, 0.0f) != 1) override; }
	// Old JUCE versions set effFlagsNoSoundInStop even when the shouldn't (see various ValhallaDSP reverb plugins). While the user cannot change the plugin bypass setting manually yet, play safe with VST plugins and do not optimize.
	bool ShouldProcessSilence() override { return true; }

	int32 GetNumPrograms() const override;
	int32 GetCurrentProgram() override;
	void SetCurrentProgram(int32 nIndex) override;

	PlugParamIndex GetNumParameters() const override;
	PlugParamValue GetParameter(PlugParamIndex nIndex) override;
	void SetParameter(PlugParamIndex nIndex, PlugParamValue fValue) override;

	CString GetCurrentProgramName() override;
	void SetCurrentProgramName(const CString &name) override;
	CString GetProgramName(int32 program) override;

	CString GetParamName(PlugParamIndex param) override;
	CString GetParamLabel(PlugParamIndex param) override { return GetParamPropertyString(param, effGetParamLabel); };
	CString GetParamDisplay(PlugParamIndex param) override { return GetParamPropertyString(param, effGetParamDisplay); };

	static VstIntPtr DispatchSEH(AEffect *effect, VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt, unsigned long &exception);
	VstIntPtr Dispatch(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt);

	bool HasEditor() const override { return (m_Effect.flags & effFlagsHasEditor) != 0; }
	CAbstractVstEditor *OpenEditor() override;
	CString GetDefaultEffectName() override;

	void Bypass(bool bypass = true) override;

	bool IsInstrument() const override;
	bool CanRecieveMidiEvents() override;

	void CacheProgramNames(int32 firstProg, int32 lastProg) override;
	void CacheParameterNames(int32 firstParam, int32 lastParam) override;

public:
	void Release() override;
	void SaveAllParameters() override;
	void RestoreAllParameters(int32 program) override;
	void Process(float *pOutL, float *pOutR, uint32 numFrames) override;
	bool MidiSend(uint32 dwMidiCode) override;
	bool MidiSysexSend(const void *message, uint32 length) override;
	void HardAllNotesOff() override;
	void NotifySongPlaying(bool playing) override;

	void Resume() override;
	void Suspend() override;
	void PositionChanged() override { m_positionChanged = true; }

	// Check whether a VST parameter can be automated
	bool CanAutomateParameter(PlugParamIndex index);

	int GetNumInputChannels() const override { return m_Effect.numInputs; }
	int GetNumOutputChannels() const override { return m_Effect.numOutputs; }

	void BeginSetProgram(int32 program) override;
	void EndSetProgram() override;

protected:
	// Helper function for retreiving parameter name / label / display
	CString GetParamPropertyString(VstInt32 param, VstInt32 opcode);

	// Set up input / output buffers.
	bool InitializeIOBuffers();

	// Process incoming and outgoing VST events.
	void ProcessVSTEvents();
	void ReceiveVSTEvents(const VstEvents *events);

	void ReportPlugException(const mpt::ustring &text) const;

public:
	static VstIntPtr VSTCALLBACK MasterCallBack(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
protected:
	VstIntPtr VstFileSelector(bool destructor, VstFileSelect *fileSel);
};


OPENMPT_NAMESPACE_END

#endif // NO_VST