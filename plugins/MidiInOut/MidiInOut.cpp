/*
 * MidiInOut.cpp
 * -------------
 * Purpose: A VST plugin for sending and receiving MIDI data.
 * Notes  : Compile the static PortMidi library first. PortMidi comes with a Visual Studio solution so this should be simple.
 *          The PortMidi directory should be played in OpenMPT's include folder (./include/portmidi)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "MidiInOut.h"
#include "MidiInOutEditor.h"
#include <algorithm>


AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
//----------------------------------------------------------------
{
	return new MidiInOut(audioMaster);
}


MidiInOut::MidiInOut(audioMasterCallback audioMaster) : AudioEffectX(audioMaster, maxPrograms, maxParams)
//-------------------------------------------------------------------------------------------------------
{
	editor = new MidiInOutEditor(static_cast<AudioEffect *>(this));

	setNumInputs(0);		// No Input
	setNumOutputs(0);		// No Output
	setUniqueID(CCONST('M', 'M', 'I', 'D'));	// Effect ID (ModPlug MIDI :)
	canProcessReplacing();	// Supports replacing output (default)
	isSynth(true);			// Not strictly a synth, but an instrument plugin in the broadest sense
	setEditor(editor);

	Pm_Initialize();

	isProcessing = false;
	isBypassed = false;
	OpenDevice(inputDevice.index, true);
	OpenDevice(outputDevice.index, true);

	vst_strncpy(programName, "Default", kVstMaxProgNameLen);	// default program name
}


MidiInOut::~MidiInOut()
//---------------------
{
	suspend();
	Pm_Terminate();
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
				event.timestamp = 0;
				Pm_Write(outputDevice.stream, &event, 3);
			}
			break;

		case kVstSysExType:
			{
				VstMidiSysexEvent *midiEvent = reinterpret_cast<VstMidiSysexEvent *>(events->events[i]);

				// Send a PortMidi sysex event
				Pm_WriteSysEx(outputDevice.stream, 0, reinterpret_cast<unsigned char*>(midiEvent->sysexDump));
			}
			break;
		}
	}
	return 1;
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
		device.stream = nullptr;
		device.name = "<none>";
		return;
	}

	PmError result = pmNoError;
	if(isProcessing)
	{
		// Don't open MIDI devices if we're not processing.
		// This has to be done since we receive MIDI events in processReplacing(),
		// so if no processing is happening, some irrelevant events might be queued until the next processing happens...
		if(asInputDevice)
		{
			result = Pm_OpenInput(&device.stream, newDevice, nullptr, 0, nullptr, nullptr);
		} else
		{
			result = Pm_OpenOutput(&device.stream, newDevice, nullptr, 0, nullptr, nullptr, 0);
		}
	}

	// Update current device name
	const PmDeviceInfo *deviceInfo = Pm_GetDeviceInfo(device.index);

	if(deviceInfo != nullptr)
	{
		device.name = deviceInfo->name;
	} else
	{
		device.name = "Unavailable";
	}

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
		Pm_Close(device.stream);
		device.stream = nullptr;
	}
}
