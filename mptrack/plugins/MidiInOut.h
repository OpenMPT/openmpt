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
#include "../../misc/mptClock.h"
#include "../../soundlib/plugins/PlugInterface.h"
#include <rtmidi/RtMidi.h>
#include <array>
#include <deque>


OPENMPT_NAMESPACE_BEGIN


class MidiDevice
{
public:
	using ID = decltype(RtMidiIn().getPortCount());
	static constexpr ID NO_MIDI_DEVICE = ID(-1);
	static constexpr ID INTERNAL_MIDI_DEVICE = ID(-2);

	RtMidi &stream;
	std::string name;  // Charset::UTF8
	mpt::ustring friendlyName;
	ID index = NO_MIDI_DEVICE;

public:
	MidiDevice(RtMidi &stream)
		: stream(stream)
		, name("<none>")
	{ }

	std::string GetPortName(ID port);  // Charset::UTF8
};


class MidiInOut final : public IMidiPlugin
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

		kNoDevice = MidiDevice::NO_MIDI_DEVICE,
		kInternalDevice = MidiDevice::INTERNAL_MIDI_DEVICE,
	};

	// MIDI queue entry with small storage optimiziation.
	// This optimiziation is going to be used for all messages that OpenMPT can send internally,
	// but SysEx messages created by this plugin or received from other plugins may be longer.
	class Message
	{
	public:
		double m_time;
		size_t m_size;
		unsigned char *m_message = nullptr;
	protected:
		std::array<unsigned char, 32> m_msgSmall;

	public:
		Message(double time, const void *data, size_t size)
			: m_time(time)
			, m_size(size)
		{
			if(size > m_msgSmall.size())
				m_message = new unsigned char[size];
			else
				m_message = m_msgSmall.data();
			std::memcpy(m_message, data, size);
		}

		Message(const Message &) = delete;
		Message & operator=(const Message &) = delete;

		Message(double time, unsigned char msg) noexcept : Message(time, &msg, 1) { }

		operator mpt::span<const unsigned char>() const { return mpt::as_span(m_message, m_size); }

		Message(Message &&other) noexcept
			: m_time(other.m_time)
			, m_size(other.m_size)
			, m_message(other.m_message)
			, m_msgSmall(other.m_msgSmall)
		{
			other.m_message = nullptr;
			if(m_size <= m_msgSmall.size())
				m_message = m_msgSmall.data();
			
		}

		~Message()
		{
			if(m_size > m_msgSmall.size())
				delete[] m_message;
		}

		Message& operator= (Message &&other) noexcept
		{
			m_time = other.m_time;
			m_size = other.m_size;
			m_message = (m_size <= m_msgSmall.size()) ? m_msgSmall.data() : other.m_message;
			m_msgSmall = other.m_msgSmall;
			other.m_message = nullptr;
			return *this;
		}
	};

	std::string m_chunkData;                     // Storage for GetChunk
	std::deque<Message> m_outQueue;              // Latency-compensated output
	std::vector<unsigned char> m_bufferedInput;  // For receiving long SysEx messages

	std::vector<uint8> m_initialMidiDump;                          // MIDI dump to send at song start
	std::vector<std::pair<std::string, float>> m_parameterMacros;  // Macros to automate via plugin parameter mechanism
	std::vector<uint8> m_parameterMacroScratchSpace;

	mpt::mutex m_mutex;
	double m_nextClock = 0.0;  // Remaining samples until next MIDI clock tick should be sent
	double m_latency = 0.0;    // User-adjusted latency in seconds

	// I/O device settings
	Util::MultimediaClock m_clock;
	RtMidiIn m_midiIn;
	RtMidiOut m_midiOut;
	MidiDevice m_inputDevice;
	MidiDevice m_outputDevice;
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
	int32 GetUID() const final { return 'MMID'; }
	int32 GetVersion() const final { return 2; }
	void Idle() final { }
	uint32 GetLatency() const final;

	int32 GetNumPrograms() const final { return kNumPrograms; }
	int32 GetCurrentProgram() final { return 0; }
	void SetCurrentProgram(int32) final { }

	PlugParamIndex GetNumParameters() const final { return kNumParams; }
	PlugParamIndex GetNumVisibleParameters() const final { return kNumVisibleParams; }
	void SetParameter(PlugParamIndex index, PlugParamValue value, PlayState *playState = nullptr, CHANNELINDEX chn = CHANNELINDEX_INVALID) final;
	PlugParamValue GetParameter(PlugParamIndex nIndex) final;

	// Save parameters for storing them in a module file
	void SaveAllParameters() final;
	// Restore parameters from module file
	void RestoreAllParameters(int32 program) final;
	void Process(float *pOutL, float *pOutR, uint32 numFrames) final;
	// Render silence and return the highest resulting output level
	float RenderSilence(uint32) final{ return 0; }
	using IMixPlugin::MidiSend;
	bool MidiSend(mpt::const_byte_span midiData) final;
	void HardAllNotesOff() final;
	// Modify parameter by given amount. Only needs to be re-implemented if plugin architecture allows this to be performed atomically.
	void Resume() final;
	void Suspend() final;
	// Tell the plugin that there is a discontinuity between the previous and next render call (e.g. aftert jumping around in the module)
	void PositionChanged() final;
	void Bypass(bool bypass = true) final;
	bool IsInstrument() const final { return true; }
	bool CanRecieveMidiEvents() final { return true; }
	// If false is returned, mixing this plugin can be skipped if its input are currently completely silent.
	bool ShouldProcessSilence() final { return true; }

#ifdef MODPLUG_TRACKER
	CString GetDefaultEffectName() final { return _T("MIDI Input / Output"); }

	CString GetParamName(PlugParamIndex param) final;
	CString GetParamLabel(PlugParamIndex) final{ return CString(); }
	CString GetParamDisplay(PlugParamIndex param) final;
	CString GetCurrentProgramName() final { return m_programName; }
	void SetCurrentProgramName(const CString &name) final { m_programName = name; }
	CString GetProgramName(int32) final { return m_programName; }
	virtual CString GetPluginVendor() { return _T("OpenMPT Project"); }

	bool HasEditor() const final { return true; }
protected:
	CAbstractVstEditor *OpenEditor() final;
#endif

public:
	int GetNumInputChannels() const final { return 0; }
	int GetNumOutputChannels() const final { return 0; }

	bool ProgramsAreChunks() const final { return true; }
	ChunkData GetChunk(bool isBank) final;
	void SetChunk(const ChunkData &chunk, bool isBank) final;

	void SetInitialMidiDump(std::vector<uint8> dump);
	std::vector<uint8> GetInitialMidiDump() const { return m_initialMidiDump; }
	void SetMacro(size_t index, std::string macro);
	std::string GetMacro(size_t index) const;

protected:
	// Open a device for input or output.
	void OpenDevice(MidiDevice newDevice, bool asInputDevice);
	void OpenDevice(MidiDevice::ID newDevice, bool asInputDevice, bool updateName = true);
	// Close an active device.
	void CloseDevice(MidiDevice &device);

	static void InputCallback(double deltatime, std::vector<unsigned char> *message, void *userData) { static_cast<MidiInOut *>(userData)->InputCallback(deltatime, *message); }
	void InputCallback(double deltatime, std::vector<unsigned char> &message);

	// Calculate the current output timestamp
	double GetOutputTimestamp() const;

	void SendMessage(mpt::span<const unsigned char> midiMsg);
};


OPENMPT_NAMESPACE_END
