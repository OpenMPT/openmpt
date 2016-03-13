/*
 * MidiInOut.cpp
 * -------------
 * Purpose: A VST plugin for sending and receiving MIDI data.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#define MODPLUG_TRACKER
#define OPENMPT_NAMESPACE
#define OPENMPT_NAMESPACE_BEGIN
#define OPENMPT_NAMESPACE_END
#define MPT_ENABLE_THREAD
#define VC_EXTRALEAN
#define NOMINMAX
#ifdef _MSC_VER
#if (_MSC_VER < 1600)
#define nullptr 0
#endif
#endif

#include "MidiInOut.h"
#include "MidiInOutEditor.h"
#include <algorithm>

#ifdef _WIN64
#pragma comment(linker, "/EXPORT:VSTPluginMain")
#pragma comment(linker, "/EXPORT:main=VSTPluginMain")
#else
#pragma comment(linker, "/EXPORT:_VSTPluginMain")
#pragma comment(linker, "/EXPORT:_main=_VSTPluginMain")
#endif


AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
//----------------------------------------------------------------
{
	return new OPENMPT_NAMESPACE::MidiInOut(audioMaster);
}


OPENMPT_NAMESPACE_BEGIN


int MidiInOut::numInstances = 0;


MidiInOut::MidiInOut(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, maxPrograms, maxParams)
//-------------------------------------------------------------------------------------------------------
{
	editor = new MidiInOutEditor(static_cast<AudioEffect *>(this));

	setNumInputs(0);			// No Input
	setNumOutputs(0);			// No Output
	setUniqueID(CCONST('M', 'M', 'I', 'D'));	// Effect ID (ModPlug MIDI :)
	canProcessReplacing();		// Supports replacing output (default)
	isSynth(true);				// Not strictly a synth, but an instrument plugin in the broadest sense
	cEffect.version = 2;		// Why is there a setter for everything, but not the plugin version?
	programsAreChunks(true);	// Version 2 of our plugin stores program chunks, including the device name.

	setEditor(editor);

	if(!numInstances++)
	{
		Pt_Start(1, nullptr, nullptr);
		Pm_Initialize();
	}

	chunk = nullptr;
	isProcessing = false;
	isBypassed = false;
	latencyCompensation = true;

	vst_strncpy(programName, "Default", kVstMaxProgNameLen);	// default program name
}


MidiInOut::~MidiInOut()
//---------------------
{
	suspend();
	delete[] chunk;

	if(--numInstances == 0)
	{
		// This terminates MIDI output for all instances of the plugin, so only ever do it if this was the only instance left.
		Pm_Terminate();
		Pt_Stop();
	}
}


void MidiInOut::setProgramName(char *name)
//----------------------------------------
{
	vst_strncpy(programName, name, kVstMaxProgNameLen);
}


void MidiInOut::getProgramName(char *name)
//----------------------------------------
{
	vst_strncpy(name, programName, kVstMaxProgNameLen);
}


VstInt32 MidiInOut::getChunk(void **data, bool /*isPreset*/)
//----------------------------------------------------------
{
	const VstInt32 programNameLen = static_cast<VstInt32>(strlen(programName));
	VstInt32 byteSize = static_cast<VstInt32>(8 * sizeof(VstInt32)
		+ programNameLen
		+ inputDevice.name.size()
		+ outputDevice.name.size());

	delete[] chunk;
	(*data) = chunk = new (std::nothrow) char[byteSize];
	if(chunk)
	{
		VstInt32 *header = reinterpret_cast<VstInt32 *>(chunk);
		header[0] = cEffect.version;
		header[1] = 1;	// Number of programs
		header[2] = programNameLen;
		header[3] = inputDevice.index;
		header[4] = static_cast<VstInt32>(inputDevice.name.size());
		header[5] = outputDevice.index;
		header[6] = static_cast<VstInt32>(outputDevice.name.size());
		header[7] = latencyCompensation;
		strncpy(chunk + 8 * sizeof(VstInt32), programName, programNameLen);
		strncpy(chunk + 8 * sizeof(VstInt32) + programNameLen, inputDevice.name.c_str(), header[4]);
		strncpy(chunk + 8 * sizeof(VstInt32) + programNameLen + header[4], outputDevice.name.c_str(), header[6]);
		return byteSize;
	} else
	{
		return 0;
	}
}


VstInt32 MidiInOut::setChunk(void *data, VstInt32 byteSize, bool /*isPreset*/)
//----------------------------------------------------------------------------
{
	if(byteSize < 8 * sizeof(VstInt32))
	{
		return 0;
	}

	const VstInt32 *header = reinterpret_cast<const VstInt32 *>(data);

	VstInt32 version = header[0];
	if(version > cEffect.version)
	{
		return 0;
	}

	VstInt32 nameStrSize = std::min(header[2], byteSize - VstInt32(8 * sizeof(VstInt32)));
	VstInt32 inStrSize = std::min(header[4], byteSize - VstInt32(8 * sizeof(VstInt32) - nameStrSize));
	VstInt32 outStrSize = std::min(header[6], byteSize - VstInt32(8 * sizeof(VstInt32) - nameStrSize - inStrSize));

	const char *nameStr = reinterpret_cast<const char *>(data) + 8 * sizeof(VstInt32);
	const char *inStr = reinterpret_cast<const char *>(data) + 8 * sizeof(VstInt32) + nameStrSize;
	const char *outStr = reinterpret_cast<const char *>(data) + 8 * sizeof(VstInt32) + nameStrSize + inStrSize;

	PmDeviceID inID = header[3];
	PmDeviceID outID = header[5];
	latencyCompensation = header[7] != 0;

	vst_strncpy(programName, nameStr, std::min(nameStrSize, VstInt32(kVstMaxProgNameLen)));

	if(strncmp(inStr, GetDeviceName(inID), inStrSize))
	{
		// Stored name differs from actual device name - try finding another device with the same name.
		PmDeviceID i = 0;
		const PmDeviceInfo *device;
		while((device = Pm_GetDeviceInfo(i)) != nullptr)
		{
			if(device->input && !strncmp(inStr, device->name, inStrSize))
			{
				inID = i;
				break;
			}
			i++;
		}
	}
	if(strncmp(outStr, GetDeviceName(outID), outStrSize))
	{
		// Stored name differs from actual device name - try finding another device with the same name.
		PmDeviceID i = 0;
		const PmDeviceInfo *device;
		while((device = Pm_GetDeviceInfo(i)) != nullptr)
		{
			if(device->output && !strncmp(outStr, device->name, outStrSize))
			{
				outID = i;
				break;
			}
			i++;
		}
	}

	setParameter(MidiInOut::inputParameter, DeviceIDToParameter(inID));
	setParameter(MidiInOut::outputParameter, DeviceIDToParameter(outID));

	return 1;
}


void MidiInOut::setParameter(VstInt32 index, float value)
//-------------------------------------------------------
{
	PmDeviceID newDevice = ParameterToDeviceID(value);
	OpenDevice(newDevice, (index == inputParameter));

	MidiInOutEditor *realEditor = dynamic_cast<MidiInOutEditor *>(editor);
	if(realEditor != nullptr)
	{
		// Update selection in editor
		realEditor->SetCurrentDevice((index == inputParameter), newDevice);
	}
}


float MidiInOut::getParameter(VstInt32 index)
//-------------------------------------------
{
	const MidiDevice &device = (index == inputParameter) ? inputDevice : outputDevice;
	return DeviceIDToParameter(device.index);
}


// Parameter name
void MidiInOut::getParameterName(VstInt32 index, char *label)
//-----------------------------------------------------------
{
	if(index == inputParameter)
	{
		vst_strncpy(label, "MIDI In", kVstMaxParamStrLen);
	} else
	{
		vst_strncpy(label, "MIDI Out", kVstMaxParamStrLen);
	}
}


// Parameter value as text
void MidiInOut::getParameterDisplay(VstInt32 index, char *text)
//-------------------------------------------------------------
{
	const MidiDevice &device = (index == inputParameter) ? inputDevice : outputDevice;
	vst_strncpy(text, device.name.c_str(), kVstMaxParamStrLen);
}


// Unit label for parameters
void MidiInOut::getParameterLabel(VstInt32 index, char *label)
//------------------------------------------------------------
{
	vst_strncpy(label, "", kVstMaxParamStrLen);
}


// Plugin name
bool MidiInOut::getEffectName(char *name)
//---------------------------------------
{
	vst_strncpy(name, "MIDI Input / Output", kVstMaxEffectNameLen);
	return true;
}


// Plugin name
bool MidiInOut::getProductString(char *text)
//------------------------------------------
{
	vst_strncpy(text, "MIDI Input / Output", kVstMaxProductStrLen);
	return true;
}


// Plugin developer
bool MidiInOut::getVendorString(char *text)
//-----------------------------------------
{
	vst_strncpy(text, "OpenMPT Project", kVstMaxVendorStrLen);
	return true;
}


// Plugin version
VstInt32 MidiInOut::getVendorVersion()
//------------------------------------
{ 
	return 1000;
}


// Check plugin capabilities
VstInt32 MidiInOut::canDo(char *text)
//-----------------------------------
{
	return (!strcmp(text, "sendVstEvents")
		|| !strcmp(text, "sendVstMidiEvent")
		|| !strcmp(text, "receiveVstEvents")
		|| !strcmp(text, "receiveVstMidiEvent")) ? 1 : -1;
}


// Processing (we don't process any audio, only MIDI messages)
void MidiInOut::processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames)
//--------------------------------------------------------------------------------------
{
	// We don't do any audio processing here, but we process incoming MIDI events.
	if(inputDevice.stream == nullptr)
	{
		return;
	}

	mpt::lock_guard<mpt::mutex> lock(mutex);

	VstEvents events;
	events.numEvents = 1;
	events.reserved = 0;

	while(Pm_Poll(inputDevice.stream))
	{
		// Read incoming MIDI events.
		PmEvent buffer;
		Pm_Read(inputDevice.stream, &buffer, 1);

		if(isBypassed)
		{
			// Discard events if bypassed
			continue;
		}

		VstMidiEvent midiEvent;
		midiEvent.type = kVstMidiType;
		midiEvent.byteSize = sizeof(VstMidiEvent);
		midiEvent.deltaFrames = 0;
		midiEvent.flags = 0;
		midiEvent.noteLength = 0;
		midiEvent.noteOffset = 0;
		memcpy(midiEvent.midiData, &buffer.message, 4);

		midiEvent.detune = 0;
		midiEvent.noteOffVelocity = 0;
		midiEvent.reserved1 = 0;
		midiEvent.reserved2 = 0;

		events.events[0] = reinterpret_cast<VstEvent *>(&midiEvent);

		sendVstEventsToHost(&events);
	}
}


// Resume playback
void MidiInOut::resume()
//----------------------
{
	// Resume MIDI I/O
	isProcessing = true;
	OpenDevice(inputDevice.index, true);
	OpenDevice(outputDevice.index, false);
}


// Stop playback
void MidiInOut::suspend()
//-----------------------
{
	// Suspend MIDI I/O
	CloseDevice(inputDevice);
	CloseDevice(outputDevice);
	isProcessing = false;
}


// Process incoming events
VstInt32 MidiInOut::processEvents(VstEvents *events)
//--------------------------------------------------
{
	if(outputDevice.stream == nullptr || isBypassed)
	{
		// We need an output device to send MIDI messages to.
		return 1;
	}

	const PtTimestamp now = (latencyCompensation ? Pt_Time() : 0);

	for(VstInt32 i = 0; i < events->numEvents; i++)
	{
		switch(events->events[i]->type)
		{
		case kVstMidiType:
			{
				VstMidiEvent *midiEvent = reinterpret_cast<VstMidiEvent *>(events->events[i]);
				
				// Create and send PortMidi event
				PmEvent event;
				memcpy(&event.message, midiEvent->midiData, 4);
				event.timestamp = now;
				Pm_Write(outputDevice.stream, &event, 1);
			}
			break;

		case kVstSysExType:
			{
				VstMidiSysexEvent *midiEvent = reinterpret_cast<VstMidiSysexEvent *>(events->events[i]);

				// Send a PortMidi sysex event
				Pm_WriteSysEx(outputDevice.stream, now, reinterpret_cast<unsigned char*>(midiEvent->sysexDump));
			}
			break;
		}
	}
	return 1;
}


static PmTimestamp PtTimeWrapper(void* /*time_info*/)
//---------------------------------------------------
{
	return Pt_Time();
}


// Open a device for input or output.
void MidiInOut::OpenDevice(PmDeviceID newDevice, bool asInputDevice)
//------------------------------------------------------------------
{
	MidiDevice &device = asInputDevice ? inputDevice : outputDevice;

	if(device.index == newDevice && device.stream != nullptr)
	{
		// No need to re-open this device.
		return;
	}

	CloseDevice(device);

	device.index = newDevice;

	if(device.index == noDevice)
	{
		// Dummy device
		device = MidiDevice();
		return;
	}

	PmError result = pmNoError;
	if(isProcessing)
	{
		// Don't open MIDI devices if we're not processing.
		// This has to be done since we receive MIDI events in processReplacing(),
		// so if no processing is happening, some irrelevant events might be queued until the next processing happens...
		mpt::lock_guard<mpt::mutex> lock(mutex);
		if(asInputDevice)
		{
			result = Pm_OpenInput(&device.stream, newDevice, nullptr, 0, nullptr, nullptr);
		} else
		{
			if(latencyCompensation)
			{
				// buffer of 10000 events
				result = Pm_OpenOutput(&device.stream, newDevice, nullptr, 10000, PtTimeWrapper, nullptr, static_cast<PtTimestamp>(1000.0 * getOutputLatency() / getSampleRate()));
			} else
			{
				result = Pm_OpenOutput(&device.stream, newDevice, nullptr, 0, nullptr, nullptr, 0);
			}
		}
	}

	// Update current device name
	device.name = GetDeviceName(device.index);

	MidiInOutEditor *realEditor = dynamic_cast<MidiInOutEditor *>(editor);

	if(result != pmNoError && realEditor != nullptr && realEditor->isOpen())
	{
		// Display a warning if the editor is open.
		MessageBox(realEditor->GetHwnd(), "MIDI device cannot be opened!", "MIDI Input / Output", MB_OK | MB_ICONERROR);
	}
}


// Close an active device.
void MidiInOut::CloseDevice(MidiDevice &device)
//---------------------------------------------
{
	if(device.stream != nullptr)
	{
		mpt::lock_guard<mpt::mutex> lock(mutex);
		Pm_Close(device.stream);
		device.stream = nullptr;
	}
}


// Get a device name
const char *MidiInOut::GetDeviceName(PmDeviceID index) const
//-----------------------------------------------------------
{
	const PmDeviceInfo *deviceInfo = Pm_GetDeviceInfo(index);

	if(deviceInfo != nullptr)
	{
		return deviceInfo->name;
	} else
	{
		return "Unavailable";
	}
}


OPENMPT_NAMESPACE_END
