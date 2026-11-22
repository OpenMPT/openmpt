/*
 * MidiInOut.h
 * -----------
 * Purpose: A plugin for sending and receiving MIDI data.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/mutex/mutex.hpp"
#include "../MidiDeviceManager.h"
#include "../MidiInputCallback.h"
#include "../../soundlib/plugins/PlugInterface.h"


OPENMPT_NAMESPACE_BEGIN


class MidiInOut final : public IMidiPlugin, public IMidiInputCallback
{
	friend class MidiInOutEditor;

protected:
	enum : unsigned int
	{
		kMacroParamMin = 0,
		kMacroParamMax = 999,

		kNumPrograms = 1,
		kNumParams = kMacroParamMax + 1,
		kNumVisibleParams   = 0,

		kNoDevice = kNoMidiDevice,
		kInternalDevice = MidiPortID(-2),
	};

	std::string m_chunkData;  // Storage for GetChunk

	mpt::recursive_mutex m_mutex;  // For initial MIDI dump, macros and internal send queue
	std::vector<MidiMessage> m_internalSendQueue;
	std::vector<uint8> m_initialMidiDump;                          // MIDI dump to send at song start
	std::vector<std::pair<std::string, float>> m_parameterMacros;  // Macros to automate via plugin parameter mechanism
	std::vector<uint8> m_parameterMacroScratchSpace;

	double m_nextClock = 0.0;  // Remaining samples until next MIDI clock tick should be sent
	double m_latency = 0.0;    // User-adjusted latency in seconds

	// I/O device settings
	std::unique_ptr<MidiInputHandle> m_inputHandle;
	std::unique_ptr<MidiOutputHandle> m_outputHandle;
	MidiPortInfo m_inputDevice;
	MidiPortInfo m_outputDevice;

	bool m_sendTimingInfo = true;
	bool m_positionChanged = false;
	bool m_alwaysSendInitialDump = false;
	bool m_initialDumpSent = false;

#ifdef MODPLUG_TRACKER
	CString m_programName;
#endif

public:
	static IMixPlugin* Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN &mixStruct);
	MidiInOut(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN &mixStruct);
	~MidiInOut();

	/////////////////////////////////////////////////
	// Destroy the plugin
	int32 GetUID() const override { return 'MMID'; }
	int32 GetVersion() const override { return 2; }
	void Idle() override { }
	uint32 GetLatency() const override;

	int32 GetNumPrograms() const override { return kNumPrograms; }
	int32 GetCurrentProgram() override { return 0; }
	void SetCurrentProgram(int32) override { }

	PlugParamIndex GetNumParameters() const override { return kNumParams; }
	PlugParamIndex GetNumVisibleParameters() const override { return kNumVisibleParams; }
	void SetParameter(PlugParamIndex index, PlugParamValue value, PlayState *playState = nullptr, CHANNELINDEX chn = CHANNELINDEX_INVALID) override;
	PlugParamValue GetParameter(PlugParamIndex nIndex) override;

	// Save parameters for storing them in a module file
	void SaveAllParameters() override;
	// Restore parameters from module file
	void RestoreAllParameters(int32 program) override;
	void Process(float *pOutL, float *pOutR, uint32 numFrames) override;
	// Render silence and return the highest resulting output level
	float RenderSilence(uint32) override{ return 0; }
	bool MidiSend(mpt::const_byte_span midiData) override { return MidiSend(midiData, 0); }
	bool MidiSend(mpt::const_byte_span midiData, int32 frameOffset);
	void HardAllNotesOff() override;
	void Resume() override;
	void Suspend() override;
	// Tell the plugin that there is a discontinuity between the previous and next render call (e.g. aftert jumping around in the module)
	void PositionChanged() override;
	void Bypass(bool bypass = true) override;
	bool IsInstrument() const override { return true; }
	bool CanRecieveMidiEvents() override { return true; }
	// If false is returned, mixing this plugin can be skipped if its input are currently completely silent.
	bool ShouldProcessSilence() override { return true; }

#ifdef MODPLUG_TRACKER
	CString GetDefaultEffectName() override { return _T("MIDI Input / Output"); }

	CString GetParamName(PlugParamIndex param) override;
	CString GetParamLabel(PlugParamIndex) override{ return CString(); }
	CString GetParamDisplay(PlugParamIndex param) override;
	CString GetCurrentProgramName() override { return m_programName; }
	void SetCurrentProgramName(const CString &name) override { m_programName = name; }
	CString GetProgramName(int32) override { return m_programName; }
	CString GetPluginVendor() { return _T("OpenMPT Project"); }

	bool HasEditor() const override { return true; }
protected:
	CAbstractVstEditor *OpenEditor() override;
#endif

public:
	int GetNumInputChannels() const override { return 0; }
	int GetNumOutputChannels() const override { return 0; }

	bool ProgramsAreChunks() const override { return true; }
	ChunkData GetChunk(bool isBank) override;
	void SetChunk(const ChunkData &chunk, bool isBank) override;

	void SetInitialMidiDump(std::vector<uint8> dump);
	std::vector<uint8> GetInitialMidiDump() const { return m_initialMidiDump; }
	void SetMacro(size_t index, std::string macro);
	std::string GetMacro(size_t index) const;

	void OnMidiMessage(mpt::const_byte_span data) override;

protected:
	// Open a device for input or output.
	void OpenDevice(const MidiPortInfo &newDevice, bool asInputDevice);
	void OpenDevice(MidiPortID newDevice, bool asInputDevice, bool updateName = true);

	// Calculate the audio device stream frame position for the current output
	int64 GetOutputStreamFrame(int32 frameOffset) const;

	bool IsOutputOpen() const { return m_outputDevice.id == kInternalDevice || m_outputHandle; }
};


OPENMPT_NAMESPACE_END
