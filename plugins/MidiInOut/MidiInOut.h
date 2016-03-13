/*
 * MidiInOut.h
 * -----------
 * Purpose: A VST plugin for sending and receiving MIDI data.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/mutex.h"
#include <vstsdk2.4/public.sdk/source/vst2.x/audioeffectx.h>
#include <portmidi/pm_common/portmidi.h>
#include <portmidi/porttime/porttime.h>
#include <string>


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
	{
		index = -1;	// MidiInOut::noDevice
		stream = nullptr;
		name = "<none>";
	}
};


//===================================
class MidiInOut : public AudioEffectX
//===================================
{
	friend class MidiInOutEditor;

protected:
	enum
	{
		inputParameter = 0,
		outputParameter = 1,

		maxPrograms = 1,
		maxParams = 2,

		noDevice = -1,
		maxDevices = 65536,		// Should be a power of 2 to avoid rounding errors.
	};

	// For getChunk
	mpt::mutex mutex;
	char *chunk;

	// I/O device settings
	MidiDevice inputDevice;
	MidiDevice outputDevice;
	bool isProcessing : 1;
	bool isBypassed : 1;
	bool latencyCompensation : 1;

	char programName[kVstMaxProgNameLen + 1];
	static int numInstances;

public:
	MidiInOut(audioMasterCallback audioMaster);
	~MidiInOut();

	// Processing (we don't process any audio, only MIDI messages)
	virtual void processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames);
	// Resume playback
	virtual void resume();
	// Stop playback
	virtual void suspend();
	// Process incoming events
	virtual VstInt32 processEvents(VstEvents *events);

	// Program
	virtual void setProgramName(char *name);
	virtual void getProgramName(char *name);

	// Settings
	virtual VstInt32 getChunk(void **data, bool isPreset);
	virtual VstInt32 setChunk(void *data, VstInt32 byteSize, bool isPreset);

	// Parameters
	virtual void setParameter(VstInt32 index, float value);
	virtual float getParameter(VstInt32 index);
	virtual void getParameterLabel(VstInt32 index, char *label);
	virtual void getParameterDisplay(VstInt32 index, char *text);
	virtual void getParameterName(VstInt32 index, char *text);
	virtual bool canParameterBeAutomated(VstInt32 index)
	{
		return (index < numParams);
	}

	// MIDI channels
	virtual VstInt32 getNumMidiInputChannels()
	{
		return 16;
	}

	virtual VstInt32 getNumMidiOutputChannels()
	{
		return 16;
	}

	// Effect name + version stuff
	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual VstInt32 getVendorVersion();

	// Check plugin capabilities
	virtual VstInt32 canDo(char *text);

	// Soft bypass
	virtual bool setBypass(bool onOff)
	{
		isBypassed = onOff;
		return true;
	}

	// Translate a VST parameter to a PortMidi device ID
	PmDeviceID ParameterToDeviceID(float value) const
	{
		return static_cast<PmDeviceID>(value * static_cast<float>(maxDevices)) - 1;
	}

	// Translate a PortMidi device ID to a VST parameter
	float DeviceIDToParameter(PmDeviceID index) const
	{
		return static_cast<float>(index + 1) / static_cast<float>(maxDevices);
	}

protected:
	// Open a device for input or output.
	void OpenDevice(PmDeviceID newDevice, bool asInputDevice);
	// Close an active device.
	void CloseDevice(MidiDevice &device);
	// Get a device name
	const char *GetDeviceName(PmDeviceID index) const;
};


OPENMPT_NAMESPACE_END
