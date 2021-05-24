/*
 * Vstplug.h
 * ---------
 * Purpose: Plugin handling (loading and processing plugins)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#ifndef NO_VST

#include "../soundlib/Snd_defs.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "../soundlib/Mixer.h"
#include "plugins/VstDefinitions.h"
#include "plugins/VstEventQueue.h"
#if defined(MODPLUG_TRACKER)
#include "ExceptionHandler.h"
#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_BEGIN


class CSoundFile;
struct SNDMIXPLUGIN;
struct VSTPluginLib;


class CVstPlugin final : public IMidiPlugin
{
protected:

	bool m_maskCrashes;
	HMODULE m_hLibrary;
	Vst::AEffect &m_Effect;
	Vst::ProcessProc m_pProcessFP = nullptr; // Function pointer to AEffect processReplacing if supported, else process.

	double lastBarStartPos = -1.0;
	uint32 m_nSampleRate;

	bool m_isVst2 : 1;
	bool m_isInstrument : 1;
	bool m_isInitialized : 1;
	bool m_needIdle : 1;
	bool m_positionChanged : 1;

	VstEventQueue vstEvents;	// MIDI events that should be sent to the plugin

	Vst::VstTimeInfo timeInfo;

public:

	const bool isBridged : 1;		// True if our built-in plugin bridge is being used.

private:

#if defined(MODPLUG_TRACKER)
	ExceptionHandler::Context m_Ectx;
#endif // MODPLUG_TRACKER

public:
	bool MaskCrashes() noexcept;

public:
	template <typename Tfn> static DWORD SETryOrError(bool maskCrashes, Tfn fn);

private:
	template <typename Tfn> DWORD SETryOrError(Tfn fn);

public:
	CVstPlugin(bool maskCrashes, HMODULE hLibrary, VSTPluginLib &factory, SNDMIXPLUGIN &mixPlugin, Vst::AEffect &effect, CSoundFile &sndFile);
	~CVstPlugin();

	static Vst::AEffect *LoadPlugin(bool maskCrashes, VSTPluginLib &plugin, HMODULE &library, bool forceBridge, bool forceLegacy);

protected:
	void Initialize();

public:
	int32 GetUID() const override;
	int32 GetVersion() const override;
	void Idle() override;
	uint32 GetLatency() const override { return m_Effect.initialDelay; }

	// Check if programs should be stored as chunks or parameters
	bool ProgramsAreChunks() const override { return (m_Effect.flags & Vst::effFlagsProgramChunks) != 0; }
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
	CString GetParamLabel(PlugParamIndex param) override { return GetParamPropertyString(param, Vst::effGetParamLabel); };
	CString GetParamDisplay(PlugParamIndex param) override { return GetParamPropertyString(param, Vst::effGetParamDisplay); };

	static intptr_t DispatchSEH(bool maskCrashes, Vst::AEffect *effect, Vst::VstOpcodeToPlugin opCode, int32 index, intptr_t value, void *ptr, float opt, unsigned long &exception);
	intptr_t Dispatch(Vst::VstOpcodeToPlugin opCode, int32 index, intptr_t value, void *ptr, float opt);

	bool HasEditor() const override { return (m_Effect.flags & Vst::effFlagsHasEditor) != 0; }
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
	bool MidiSysexSend(mpt::const_byte_span sysex) override;
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
	void BeginGetProgram(int32 program) override;
	void EndGetProgram() override;

protected:
	// Helper function for retreiving parameter name / label / display
	CString GetParamPropertyString(int32 param, Vst::VstOpcodeToPlugin opcode);

	// Set up input / output buffers.
	bool InitializeIOBuffers();

	// Process incoming and outgoing VST events.
	void ProcessVSTEvents();
	void ReceiveVSTEvents(const Vst::VstEvents *events);

	void ReportPlugException(const mpt::ustring &text) const;

public:
	static intptr_t VSTCALLBACK MasterCallBack(Vst::AEffect *effect, Vst::VstOpcodeToHost opcode, int32 index, intptr_t value, void *ptr, float opt);
protected:
	intptr_t VstFileSelector(bool destructor, Vst::VstFileSelect &fileSel);
};


OPENMPT_NAMESPACE_END

#endif // NO_VST