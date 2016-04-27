/*
 * MidiInOut.h
 * -----------
 * Purpose: A plugin for sending and receiving MIDI data.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../../common/mutex.h"
#include "../../soundlib/plugins/PlugInterface.h"
#include <portmidi/pm_common/portmidi.h>
#include <portmidi/porttime/porttime.h>


OPENMPT_NAMESPACE_BEGIN


//==============
class MidiDevice
//==============
{
public:
	PmDeviceID index;
	PortMidiStream *stream;
	std::string name;

public:
	MidiDevice()
		: index(-1)	// MidiInOut::kNoDevice
		, stream(nullptr)
		, name("<none>")
	{ }
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

	std::string chunkData;					// Storage for GetChunk
	std::vector<uint32> bufferedMessage;	// For receiving SysEx messages
	mpt::mutex mutex;

	// I/O device settings
	MidiDevice inputDevice;
	MidiDevice outputDevice;
	bool latencyCompensation;

	CString programName;
	static int numInstances;

public:
	static IMixPlugin* Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);
	MidiInOut(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);
	~MidiInOut();

	// Translate a VST parameter to a PortMidi device ID
	static PmDeviceID ParameterToDeviceID(float value)
	{
		return static_cast<PmDeviceID>(value * static_cast<float>(kMaxDevices)) - 1;
	}

	// Translate a PortMidi device ID to a VST parameter
	static float DeviceIDToParameter(PmDeviceID index)
	{
		return static_cast<float>(index + 1) / static_cast<float>(kMaxDevices);
	}

	/////////////////////////////////////////////////
	// Destroy the plugin
	virtual void Release() { delete this; }
	virtual int32 GetUID() const { return 'MMID'; }
	virtual int32 GetVersion() const { return 2; }
	virtual void Idle() { }
	virtual uint32 GetLatency() const { return 0; }

	virtual int32 GetNumPrograms() const { return kNumPrograms; }
	virtual int32 GetCurrentProgram() { return 0; }
	virtual void SetCurrentProgram(int32) { }

	virtual PlugParamIndex GetNumParameters() const {return kNumParams; }
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
	virtual bool MidiSysexSend(const char *message, uint32 length);
	virtual void HardAllNotesOff();
	// Modify parameter by given amount. Only needs to be re-implemented if plugin architecture allows this to be performed atomically.
	virtual void Resume();
	virtual void Suspend();
	// Tell the plugin that there is a discontinuity between the previous and next render call (e.g. aftert jumping around in the module)
	virtual void PositionChanged() { }
	virtual bool IsInstrument() const { return true; }
	virtual bool CanRecieveMidiEvents() { return true; }
	// If false is returned, mixing this plugin can be skipped if its input are currently completely silent.
	virtual bool ShouldProcessSilence() { return true; }

#ifdef MODPLUG_TRACKER
	virtual CString GetDefaultEffectName() { return _T("MIDI Input / Output"); }

	// Cache a range of names, in case one-by-one retrieval would be slow (e.g. when using plugin bridge)
	virtual void CacheProgramNames(int32, int32) { }
	virtual void CacheParameterNames(int32, int32) { }

	virtual CString GetParamName(PlugParamIndex param);
	virtual CString GetParamLabel(PlugParamIndex) { return CString(); }
	virtual CString GetParamDisplay(PlugParamIndex param);
	virtual CString GetCurrentProgramName() { return programName; }
	virtual void SetCurrentProgramName(const CString &name) { programName = name; }
	virtual CString GetProgramName(int32) { return programName; }

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
	void OpenDevice(PmDeviceID newDevice, bool asInputDevice);
	// Close an active device.
	void CloseDevice(MidiDevice &device);
	// Get a device name
	const char *GetDeviceName(PmDeviceID index) const;
};


OPENMPT_NAMESPACE_END
