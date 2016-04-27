/*
 * MidiInOut.cpp
 * -------------
 * Purpose: A plugin for sending and receiving MIDI data.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "MidiInOut.h"
#include "MidiInOutEditor.h"
#include "../../common/FileReader.h"
#include "../../mptrack/Reporting.h"
#include <algorithm>
#include <sstream>


OPENMPT_NAMESPACE_BEGIN


int MidiInOut::numInstances = 0;

IMixPlugin* MidiInOut::Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
//------------------------------------------------------------------------------------------------
{
	return new (std::nothrow) MidiInOut(factory, sndFile, mixStruct);
}


MidiInOut::MidiInOut(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
	: IMidiPlugin(factory, sndFile, mixStruct)
	, latencyCompensation(true)
	, programName(_T("Default"))
//---------------------------------------------------------------------------------------
{
	if(!numInstances++)
	{
		Pt_Start(1, nullptr, nullptr);
		Pm_Initialize();
	}

	m_mixBuffer.Initialize(2, 2);
	InsertIntoFactoryList();
}


MidiInOut::~MidiInOut()
//---------------------
{
	Suspend();

	if(--numInstances == 0)
	{
		// This terminates MIDI output for all instances of the plugin, so only ever do it if this was the only instance left.
		Pm_Terminate();
		Pt_Stop();
	}
}


void MidiInOut::SaveAllParameters()
//---------------------------------
{
	char *chunk = nullptr;
	size_t size = GetChunk(chunk, false);
	if(size == 0 || chunk == nullptr)
		return;

	m_pMixStruct->defaultProgram = -1;
	if(m_pMixStruct->nPluginDataSize != size || m_pMixStruct->pPluginData == nullptr)
	{
		delete[] m_pMixStruct->pPluginData;
		m_pMixStruct->nPluginDataSize = size;
		m_pMixStruct->pPluginData = new (std::nothrow) char[size];
	}
	if(m_pMixStruct->pPluginData != nullptr)
	{
		memcpy(m_pMixStruct->pPluginData, chunk, size);
	}
}


void MidiInOut::RestoreAllParameters(int32 program)
//-------------------------------------------------
{
	IMixPlugin::RestoreAllParameters(program);	// First plugin version didn't use chunks.
	SetChunk(m_pMixStruct->nPluginDataSize, m_pMixStruct->pPluginData, false);
}


size_t MidiInOut::GetChunk(char *(&chunk), bool /*isBank*/)
//---------------------------------------------------------
{
	const std::string programName8 = mpt::ToCharset(mpt::CharsetUTF8, programName);
	const std::string inputName8 = mpt::ToCharset(mpt::CharsetUTF8, mpt::CharsetLocale, inputDevice.name);
	const std::string outputName8 = mpt::ToCharset(mpt::CharsetUTF8, mpt::CharsetLocale, outputDevice.name);

	std::ostringstream s;
	mpt::IO::WriteRaw(s, "fEvN", 4);	// VST program chunk magic
	mpt::IO::WriteIntLE< int32>(s, GetVersion());
	mpt::IO::WriteIntLE<uint32>(s, 1);	// Number of programs
	mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(programName8.size()));
	mpt::IO::WriteIntLE<uint32>(s, inputDevice.index);
	mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(inputDevice.name.size()));
	mpt::IO::WriteIntLE<uint32>(s, outputDevice.index);
	mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(outputDevice.name.size()));
	mpt::IO::WriteIntLE<uint32>(s, latencyCompensation);
	mpt::IO::WriteRaw(s, programName8.c_str(), programName8.size());
	mpt::IO::WriteRaw(s, inputName8.c_str(), inputName8.size());
	mpt::IO::WriteRaw(s, outputName8.c_str(), outputName8.size());
	chunkData = s.str();
	chunk = const_cast<char *>(chunkData.c_str());
	return chunkData.size();
}


void MidiInOut::SetChunk(size_t size, char *chunk, bool /*isBank*/)
//-----------------------------------------------------------------
{
	FileReader file(chunk, size);
	if(!file.CanRead(9 * sizeof(uint32))
		|| !file.ReadMagic("fEvN")				// VST program chunk magic
		|| file.ReadInt32LE() > GetVersion()	// Plugin version
		|| file.ReadUint32LE() < 1)				// Number of programs
		return;

	uint32 nameStrSize = file.ReadUint32LE();
	PmDeviceID inID = file.ReadUint32LE();
	uint32 inStrSize = file.ReadUint32LE();
	PmDeviceID outID = file.ReadUint32LE();
	uint32 outStrSize = file.ReadUint32LE();
	latencyCompensation = file.ReadUint32LE() != 0;

	std::string s;
	file.ReadString<mpt::String::maybeNullTerminated>(s, nameStrSize);
	programName = mpt::ToCString(mpt::CharsetUTF8, s);

	file.ReadString<mpt::String::maybeNullTerminated>(s, inStrSize);
	s = mpt::ToCharset(mpt::CharsetLocale, mpt::CharsetUTF8, s);
	if(s != GetDeviceName(inID))
	{
		// Stored name differs from actual device name - try finding another device with the same name.
		const PmDeviceInfo *device;
		for(PmDeviceID i = 0; (device = Pm_GetDeviceInfo(i)) != nullptr; i++)
		{
			if(device->input && s == device->name)
			{
				inID = i;
				break;
			}
		}
	}

	file.ReadString<mpt::String::maybeNullTerminated>(s, outStrSize);
	s = mpt::ToCharset(mpt::CharsetLocale, mpt::CharsetUTF8, s);
	if(s != GetDeviceName(outID))
	{
		// Stored name differs from actual device name - try finding another device with the same name.
		const PmDeviceInfo *device;
		for(PmDeviceID i = 0; (device = Pm_GetDeviceInfo(i)) != nullptr; i++)
		{
			if(device->output && s == device->name)
			{
				outID = i;
				break;
			}
		}
	}

	SetParameter(MidiInOut::kInputParameter, DeviceIDToParameter(inID));
	SetParameter(MidiInOut::kOutputParameter, DeviceIDToParameter(outID));
}


void MidiInOut::SetParameter(PlugParamIndex index, PlugParamValue value)
//----------------------------------------------------------------------
{
	PmDeviceID newDevice = ParameterToDeviceID(value);
	OpenDevice(newDevice, (index == kInputParameter));

	// Update selection in editor
	MidiInOutEditor *editor = dynamic_cast<MidiInOutEditor *>(GetEditor());
	if(editor != nullptr)
		editor->SetCurrentDevice((index == kInputParameter), newDevice);
}


float MidiInOut::GetParameter(PlugParamIndex index)
//-------------------------------------------------
{
	const MidiDevice &device = (index == kInputParameter) ? inputDevice : outputDevice;
	return DeviceIDToParameter(device.index);
}


#ifdef MODPLUG_TRACKER

CString MidiInOut::GetParamName(PlugParamIndex param)
//---------------------------------------------------
{
	if(param == kInputParameter)
		return _T("MIDI In");
	else
		return _T("MIDI Out");
}


// Parameter value as text
CString MidiInOut::GetParamDisplay(PlugParamIndex param)
//------------------------------------------------------
{
	const MidiDevice &device = (param == kInputParameter) ? inputDevice : outputDevice;
	return device.name.c_str();
}


CAbstractVstEditor *MidiInOut::OpenEditor()
//-----------------------------------------
{
	try
	{
		return new MidiInOutEditor(*this);
	} catch(MPTMemoryException)
	{
		return nullptr;
	}
}

#endif // MODPLUG_TRACKER


// Processing (we don't process any audio, only MIDI messages)
void MidiInOut::Process(float *, float *, uint32)
//-----------------------------------------------
{
	// We don't do any audio processing here, but we process incoming MIDI events.
	if(inputDevice.stream == nullptr)
		return;

	mpt::lock_guard<mpt::mutex> lock(mutex);

	while(Pm_Poll(inputDevice.stream))
	{
		// Read incoming MIDI events.
		PmEvent buffer;
		Pm_Read(inputDevice.stream, &buffer, 1);

		// Discard events if bypassed
		if(IsBypassed())
			continue;

		ReceiveMidi(buffer.message);
	}
}


// Resume playback
void MidiInOut::Resume()
//----------------------
{
	// Resume MIDI I/O
	m_isResumed = true;
	OpenDevice(inputDevice.index, true);
	OpenDevice(outputDevice.index, false);
}


// Stop playback
void MidiInOut::Suspend()
//-----------------------
{
	// Suspend MIDI I/O
	CloseDevice(inputDevice);
	CloseDevice(outputDevice);
	m_isResumed = false;
}


bool MidiInOut::MidiSend(uint32 midiCode)
//---------------------------------------
{
	if(outputDevice.stream == nullptr || IsBypassed())
	{
		// We need an output device to send MIDI messages to.
		return true;
	}

	const PtTimestamp now = (latencyCompensation ? Pt_Time() : 0);

	// Create and send PortMidi event
	PmEvent event;
	memcpy(&event.message, &midiCode, 4);
	event.timestamp = now;
	Pm_Write(outputDevice.stream, &event, 1);
	return true;
}


bool MidiInOut::MidiSysexSend(const char *message, uint32 /*length*/)
//-------------------------------------------------------------------
{
	if(outputDevice.stream == nullptr || IsBypassed())
	{
		// We need an output device to send MIDI messages to.
		return true;
	}

	const PtTimestamp now = (latencyCompensation ? Pt_Time() : 0);
	Pm_WriteSysEx(outputDevice.stream, now, const_cast<unsigned char *>(reinterpret_cast<const unsigned char *>(message)));
	return true;
}


void MidiInOut::HardAllNotesOff()
//-------------------------------
{
	const bool wasSuspended = !IsResumed();
	if(wasSuspended)
	{
		Resume();
	}

	for(uint8 mc = 0; mc < CountOf(m_MidiCh); mc++)		//all midi chans
	{
		PlugInstrChannel &channel = m_MidiCh[mc];
		channel.ResetProgram();

		MidiPitchBend(mc, EncodePitchBendParam(MIDIEvents::pitchBendCentre));		// centre pitch bend
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllSoundOff, mc, 0));			// all sounds off

		for(size_t i = 0; i < CountOf(channel.noteOnMap); i++)	//all notes
		{
			for(CHANNELINDEX c = 0; c < CountOf(channel.noteOnMap[i]); c++)
			{
				while(channel.noteOnMap[i][c])
				{
					MidiSend(MIDIEvents::NoteOff(mc, static_cast<uint8>(i), 0));
					channel.noteOnMap[i][c]--;
				}
			}
		}
	}

	if(wasSuspended)
	{
		Suspend();
	}
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

	if(device.index == kNoDevice)
	{
		// Dummy device
		device = MidiDevice();
		return;
	}

	PmError result = pmNoError;
	if(m_isResumed)
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
				result = Pm_OpenOutput(&device.stream, newDevice, nullptr, 10000, PtTimeWrapper, nullptr, Util::Round<PtTimestamp>(1000.0 * GetOutputLatency()));
			} else
			{
				result = Pm_OpenOutput(&device.stream, newDevice, nullptr, 0, nullptr, nullptr, 0);
			}
		}
	}

	// Update current device name
	device.name = GetDeviceName(device.index);

	MidiInOutEditor *editor = dynamic_cast<MidiInOutEditor *>(GetEditor());
	if(result != pmNoError && editor != nullptr)
	{
		// Display a warning if the editor is open.
		Reporting::Error("MIDI device cannot be opened!", "MIDI Input / Output", editor);
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
		return deviceInfo->name;
	else
		return "Unavailable";
}


OPENMPT_NAMESPACE_END
