/*
 * MidiInOut.h
 * -----------
 * Purpose: A plugin for sending and receiving MIDI data.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../../common/mptMutex.h"
#include "../../common/mptTime.h"
#include "../../soundlib/plugins/PlugInterface.h"
#include <rtmidi/RtMidi.h>
#include <deque>


OPENMPT_NAMESPACE_BEGIN


//==============
class MidiDevice
//==============
{
public:
	typedef unsigned int ID;

	RtMidi &stream;
	std::string name;
	ID index;

public:
	MidiDevice(RtMidi &stream)
		: stream(stream)
		, name("<none>")
		, index(ID(-1))	// MidiInOut::kNoDevice
	{ }

	std::string GetPortName(ID port);
};


//==================================
class MidiInOut : public IMidiPlugin
//==================================
{
	friend class MidiInOutEditor;

protected:
	enum
	{
		kInputParameter  = 0,
		kOutputParameter = 1,

		kNumPrograms = 1,
		kNumParams   = 2,

		kNoDevice   = -1,
		kMaxDevices = 65536,		// Should be a power of 2 to avoid rounding errors.
	};

	struct Message
	{
		double time;
		std::vector<unsigned char> msg;
	};

	std::string m_chunkData;					// Storage for GetChunk
	std::deque<Message> m_outQueue;				// Latency-compensated output
	std::vector<unsigned char> m_bufferedInput;	// For receiving long SysEx messages
	mpt::mutex m_mutex;
	double m_nextClock;	// Remaining samples until next MIDI clock tick should be sent
	double m_latency;	// User-adjusted latency in seconds

	// I/O device settings
	Util::MultimediaClock m_clock;
	RtMidiIn m_midiIn;
	RtMidiOut m_midiOut;
	MidiDevice m_inputDevice;
	MidiDevice m_outputDevice;
	bool m_latencyCompensation;

#ifdef MODPLUG_TRACKER
	CString m_programName;
#endif

public:
	static IMixPlugin* Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);
	MidiInOut(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);
	~MidiInOut();

	// Translate a VST parameter to an RtMidi device ID
	static MidiDevice::ID ParameterToDeviceID(float value)
	{
		return static_cast<MidiDevice::ID>(value * static_cast<float>(kMaxDevices)) - 1;
	}

	// Translate a RtMidi device ID to a VST parameter
	static float DeviceIDToParameter(MidiDevice::ID index)
	{
		return static_cast<float>(index + 1) / static_cast<float>(kMaxDevices);
	}

	/////////////////////////////////////////////////
	// Destroy the plugin
	virtual void Release() { delete this; }
	virtual int32 GetUID() const { return 'MMID'; }
	virtual int32 GetVersion() const { return 2; }
	virtual void Idle() { }
	virtual uint32 GetLatency() const;

	virtual int32 GetNumPrograms() const { return kNumPrograms; }
	virtual int32 GetCurrentProgram() { return 0; }
	virtual void SetCurrentProgram(int32) { }

	virtual PlugParamIndex GetNumParameters() const { return kNumParams; }
	virtual void SetParameter(PlugParamIndex paramindex, PlugParamValue paramvalue);
	virtual PlugParamValue GetParameter(PlugParamIndex nIndex);

	// Save parameters for storing them in a module file
	virtual void SaveAllParameters();
	// Restore parameters from module file
	virtual void RestoreAllParameters(int32 program);
	virtual void Process(float *pOutL, float *pOutR, uint32 numFrames);
	// Render silence and return the highest resulting output level
	virtual float RenderSilence(uint32) { return 0; }
	virtual bool MidiSend(uint32 midiCode);
	virtual bool MidiSysexSend(const void *message, uint32 length);
	virtual void HardAllNotesOff();
	// Modify parameter by given amount. Only needs to be re-implemented if plugin architecture allows this to be performed atomically.
	virtual void Resume();
	virtual void Suspend();
	// Tell the plugin that there is a discontinuity between the previous and next render call (e.g. aftert jumping around in the module)
	virtual void PositionChanged();
	virtual void Bypass(bool bypass = true);
	virtual bool IsInstrument() const { return true; }
	virtual bool CanRecieveMidiEvents() { return true; }
	// If false is returned, mixing this plugin can be skipped if its input are currently completely silent.
	virtual bool ShouldProcessSilence() { return true; }

#ifdef MODPLUG_TRACKER
	virtual CString GetDefaultEffectName() { return _T("MIDI Input / Output"); }

	virtual CString GetParamName(PlugParamIndex param);
	virtual CString GetParamLabel(PlugParamIndex) { return CString(); }
	virtual CString GetParamDisplay(PlugParamIndex param);
	virtual CString GetCurrentProgramName() { return m_programName; }
	virtual void SetCurrentProgramName(const CString &name) { m_programName = name; }
	virtual CString GetProgramName(int32) { return m_programName; }
	virtual CString GetPluginVendor() { return _T("OpenMPT Project"); }

	virtual bool HasEditor() const { return true; }
protected:
	virtual CAbstractVstEditor *OpenEditor();
#endif

public:
	virtual void BeginSetProgram(int32) { }
	virtual void EndSetProgram() { }

	virtual int GetNumInputChannels() const { return 0; }
	virtual int GetNumOutputChannels() const { return 0; }

	virtual bool ProgramsAreChunks() const { return true; }
	virtual size_t GetChunk(char *(&chunk), bool isBank);
	virtual void SetChunk(size_t size, char *chunk, bool isBank);

protected:
	// Open a device for input or output.
	void OpenDevice(MidiDevice::ID newDevice, bool asInputDevice);
	// Close an active device.
	void CloseDevice(MidiDevice &device);

	static void InputCallback(double deltatime, std::vector<unsigned char> *message, void *userData) { static_cast<MidiInOut *>(userData)->InputCallback(deltatime, *message); }
	void InputCallback(double deltatime, std::vector<unsigned char> &message);

	// Calculate the current output timestamp
	double GetOutputTimestamp() const;
};


OPENMPT_NAMESPACE_END
