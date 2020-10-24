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
#include "../../soundlib/Sndfile.h"
#include "../Reporting.h"
#include <algorithm>
#include <sstream>
#ifdef MODPLUG_TRACKER
#include "../Mptrack.h"
#endif


OPENMPT_NAMESPACE_BEGIN


IMixPlugin* MidiInOut::Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
{
	try
	{
		return new (std::nothrow) MidiInOut(factory, sndFile, mixStruct);
	} catch(RtMidiError &)
	{
		return nullptr;
	}
}


MidiInOut::MidiInOut(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
	: IMidiPlugin(factory, sndFile, mixStruct)
	, m_inputDevice(m_midiIn)
	, m_outputDevice(m_midiOut)
#ifdef MODPLUG_TRACKER
	, m_programName(_T("Default"))
#endif // MODPLUG_TRACKER
{
	m_mixBuffer.Initialize(2, 2);
	InsertIntoFactoryList();
}


MidiInOut::~MidiInOut()
{
	MidiInOut::Suspend();
}


uint32 MidiInOut::GetLatency() const
{
	// There is only a latency if the user-provided latency value is greater than the negative output latency.
	return mpt::saturate_round<uint32>(std::min(0.0, m_latency + GetOutputLatency()) * m_SndFile.GetSampleRate());
}


void MidiInOut::SaveAllParameters()
{
	auto chunk = GetChunk(false);
	if(chunk.empty())
		return;

	m_pMixStruct->defaultProgram = -1;
	m_pMixStruct->pluginData.assign(chunk.cbegin(), chunk.cend());
}


void MidiInOut::RestoreAllParameters(int32 program)
{
	IMixPlugin::RestoreAllParameters(program);	// First plugin version didn't use chunks.
	SetChunk(mpt::as_span(m_pMixStruct->pluginData), false);
}


enum ChunkFlags
{
	kLatencyCompensation	= 0x01,	// Implicit in current plugin version
	kLatencyPresent			= 0x02,	// Latency value is present as double-precision float
	kIgnoreTiming			= 0x04,	// Do not send timing and sequencing information
	kFriendlyInputName		= 0x08,	// Preset also stores friendly name of input device
	kFriendlyOutputName		= 0x10,	// Preset also stores friendly name of output device
};

IMixPlugin::ChunkData MidiInOut::GetChunk(bool /*isBank*/)
{
	const std::string programName8 = mpt::ToCharset(mpt::Charset::UTF8, m_programName);
	uint32 flags = kLatencyCompensation | kLatencyPresent | (m_sendTimingInfo ? 0 : kIgnoreTiming);
#ifdef MODPLUG_TRACKER
	const std::string inFriendlyName = (m_inputDevice.index == MidiDevice::NO_MIDI_DEVICE) ? m_inputDevice.name : mpt::ToCharset(mpt::Charset::UTF8, theApp.GetFriendlyMIDIPortName(mpt::ToUnicode(mpt::Charset::UTF8, m_inputDevice.name), true, false));
	const std::string outFriendlyName = (m_outputDevice.index == MidiDevice::NO_MIDI_DEVICE) ? m_outputDevice.name : mpt::ToCharset(mpt::Charset::UTF8, theApp.GetFriendlyMIDIPortName(mpt::ToUnicode(mpt::Charset::UTF8, m_outputDevice.name), false, false));
	if(inFriendlyName != m_inputDevice.name)
	{
		flags |= kFriendlyInputName;
	}
	if(outFriendlyName != m_outputDevice.name)
	{
		flags |= kFriendlyOutputName;
	}
#endif

	std::ostringstream s;
	mpt::IO::WriteRaw(s, "fEvN", 4);	// VST program chunk magic
	mpt::IO::WriteIntLE< int32>(s, GetVersion());
	mpt::IO::WriteIntLE<uint32>(s, 1);	// Number of programs
	mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(programName8.size()));
	mpt::IO::WriteIntLE<uint32>(s, m_inputDevice.index);
	mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(m_inputDevice.name.size()));
	mpt::IO::WriteIntLE<uint32>(s, m_outputDevice.index);
	mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(m_outputDevice.name.size()));
	mpt::IO::WriteIntLE<uint32>(s, flags);
	mpt::IO::WriteRaw(s, programName8.c_str(), programName8.size());
	mpt::IO::WriteRaw(s, m_inputDevice.name.c_str(), m_inputDevice.name.size());
	mpt::IO::WriteRaw(s, m_outputDevice.name.c_str(), m_outputDevice.name.size());
	mpt::IO::WriteIntLE<uint64>(s, IEEE754binary64LE(m_latency).GetInt64());
#ifdef MODPLUG_TRACKER
	if(flags & kFriendlyInputName)
	{
		mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(inFriendlyName.size()));
		mpt::IO::WriteRaw(s, inFriendlyName.c_str(), inFriendlyName.size());
	}
	if(flags & kFriendlyOutputName)
	{
		mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(outFriendlyName.size()));
		mpt::IO::WriteRaw(s, outFriendlyName.c_str(), outFriendlyName.size());
	}
#endif
	m_chunkData = s.str();
	return mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(m_chunkData));
}


// Try to match a port name against stored name or friendly name (preferred)
static void FindPort(MidiDevice::ID &id, unsigned int numPorts, const std::string &name, const std::string &friendlyName, MidiDevice &midiDevice, bool isInput)
{
	bool foundFriendly = false;
	for(unsigned int i = 0; i < numPorts; i++)
	{
		try
		{
			auto portName = midiDevice.GetPortName(i);
			bool deviceNameMatches = (portName == name);
#ifdef MODPLUG_TRACKER
			if(!friendlyName.empty() && friendlyName == mpt::ToCharset(mpt::Charset::UTF8, theApp.GetFriendlyMIDIPortName(mpt::ToUnicode(mpt::Charset::UTF8, portName), isInput, false)))
			{
				// Preferred match
				id = i;
				foundFriendly = true;
				if(deviceNameMatches)
				{
					return;
				}
			}
#else
			MPT_UNREFERENCED_PARAMETER(friendlyName)
#endif
			if(deviceNameMatches && !foundFriendly)
			{
				id = i;
			}
		} catch(const RtMidiError &)
		{
		}
	}
}


void MidiInOut::SetChunk(const ChunkData &chunk, bool /*isBank*/)
{
	FileReader file(chunk);
	if(!file.CanRead(9 * sizeof(uint32))
		|| !file.ReadMagic("fEvN")				// VST program chunk magic
		|| file.ReadInt32LE() > GetVersion()	// Plugin version
		|| file.ReadUint32LE() < 1)				// Number of programs
		return;

	uint32 nameStrSize = file.ReadUint32LE();
	MidiDevice::ID inID = file.ReadUint32LE();
	uint32 inStrSize = file.ReadUint32LE();
	MidiDevice::ID outID = file.ReadUint32LE();
	uint32 outStrSize = file.ReadUint32LE();
	uint32 flags = file.ReadUint32LE();

	std::string progName, inName, outName, inFriendlyName, outFriendlyName;
	file.ReadString<mpt::String::maybeNullTerminated>(progName, nameStrSize);
	m_programName = mpt::ToCString(mpt::Charset::UTF8, progName);

	file.ReadString<mpt::String::maybeNullTerminated>(inName, inStrSize);
	file.ReadString<mpt::String::maybeNullTerminated>(outName, outStrSize);

	if(flags & kLatencyPresent)
		m_latency = file.ReadDoubleLE();
	else
		m_latency = 0.0f;
	m_sendTimingInfo = !(flags & kIgnoreTiming);

	if(flags & kFriendlyInputName)
		file.ReadString<mpt::String::maybeNullTerminated>(inFriendlyName, file.ReadUint32LE());
	if(flags & kFriendlyOutputName)
		file.ReadString<mpt::String::maybeNullTerminated>(outFriendlyName, file.ReadUint32LE());

	// Try to match an input port name against stored name or friendly name (preferred)
	FindPort(inID, m_midiIn.getPortCount(), inName, inFriendlyName, m_inputDevice, true);
	FindPort(outID, m_midiOut.getPortCount(), outName, outFriendlyName, m_outputDevice, false);

	SetParameter(MidiInOut::kInputParameter, DeviceIDToParameter(inID));
	SetParameter(MidiInOut::kOutputParameter, DeviceIDToParameter(outID));
}


void MidiInOut::SetParameter(PlugParamIndex index, PlugParamValue value)
{
	MidiDevice::ID newDevice = ParameterToDeviceID(value);
	OpenDevice(newDevice, (index == kInputParameter));

	// Update selection in editor
	MidiInOutEditor *editor = dynamic_cast<MidiInOutEditor *>(GetEditor());
	if(editor != nullptr)
		editor->SetCurrentDevice((index == kInputParameter), newDevice);
}


float MidiInOut::GetParameter(PlugParamIndex index)
{
	const MidiDevice &device = (index == kInputParameter) ? m_inputDevice : m_outputDevice;
	return DeviceIDToParameter(device.index);
}


#ifdef MODPLUG_TRACKER

CString MidiInOut::GetParamName(PlugParamIndex param)
{
	if(param == kInputParameter)
		return _T("MIDI In");
	else
		return _T("MIDI Out");
}


// Parameter value as text
CString MidiInOut::GetParamDisplay(PlugParamIndex param)
{
	const MidiDevice &device = (param == kInputParameter) ? m_inputDevice : m_outputDevice;
	return mpt::ToCString(mpt::Charset::UTF8, device.name);
}


CAbstractVstEditor *MidiInOut::OpenEditor()
{
	try
	{
		return new MidiInOutEditor(*this);
	} catch(mpt::out_of_memory e)
	{
		mpt::delete_out_of_memory(e);
		return nullptr;
	}
}

#endif // MODPLUG_TRACKER


// Processing (we don't process any audio, only MIDI messages)
void MidiInOut::Process(float *, float *, uint32 numFrames)
{
	if(m_midiOut.isPortOpen())
	{
		mpt::lock_guard<mpt::mutex> lock(m_mutex);

		// Send MIDI clock
		if(m_nextClock < 1)
		{
			if(m_sendTimingInfo)
			{
				m_outQueue.push_back(Message(GetOutputTimestamp(), 0xF8));
			}

			double bpm = m_SndFile.GetCurrentBPM();
			if(bpm > 0.0)
			{
				m_nextClock += 2.5 * m_SndFile.GetSampleRate() / bpm;
			}
		}
		m_nextClock -= numFrames;

		double now = m_clock.Now() * (1.0 / 1000.0);
		auto message = m_outQueue.begin();
		while(message != m_outQueue.end() && message->m_time <= now)
		{
			try
			{
				m_midiOut.sendMessage(message->m_message, message->m_size);
			} catch(const RtMidiError &)
			{
			}
			message++;
		}
		m_outQueue.erase(m_outQueue.begin(), message);
	}
}


void MidiInOut::InputCallback(double /*deltatime*/, std::vector<unsigned char> &message)
{
	// We will check the bypass status before passing on the message, and not before entering the function,
	// because otherwise we might read garbage if we toggle bypass status in the middle of a SysEx message.
	bool isBypassed = IsBypassed();
	if(message.empty())
	{
		return;
	} else if(!m_bufferedInput.empty())
	{
		// SysEx message (continued)
		m_bufferedInput.insert(m_bufferedInput.end(), message.begin(), message.end());
		if(message.back() == 0xF7)
		{
			// End of message found!
			if(!isBypassed)
				ReceiveSysex(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(m_bufferedInput)));
			m_bufferedInput.clear();
		}
	} else if(message.front() == 0xF0)
	{
		// Start of SysEx message...
		if(message.back() != 0xF7)
			m_bufferedInput.insert(m_bufferedInput.end(), message.begin(), message.end());	// ...but not the end!
		else if(!isBypassed)
			ReceiveSysex(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(message)));
	} else if(!isBypassed)
	{
		// Regular message
		uint32 msg = 0;
		memcpy(&msg, message.data(), std::min(message.size(), sizeof(msg)));
		ReceiveMidi(msg);
	}
}


// Resume playback
void MidiInOut::Resume()
{
	// Resume MIDI I/O
	m_isResumed = true;
	m_nextClock = 0;
	m_outQueue.clear();
	m_clock.SetResolution(1);
	OpenDevice(m_inputDevice.index, true);
	OpenDevice(m_outputDevice.index, false);
	if(m_midiOut.isPortOpen() && m_sendTimingInfo)
	{
		MidiSend(0xFA);	// Start
	}
}


// Stop playback
void MidiInOut::Suspend()
{
	// Suspend MIDI I/O
	if(m_midiOut.isPortOpen() && m_sendTimingInfo)
	{
		try
		{
			unsigned char message[1] = { 0xFC };	// Stop
			m_midiOut.sendMessage(message, 1);
		} catch(const RtMidiError &)
		{
		}
	}
	//CloseDevice(inputDevice);
	CloseDevice(m_outputDevice);
	m_clock.SetResolution(0);
	m_isResumed = false;
}


// Playback discontinuity
void MidiInOut::PositionChanged()
{
	if(m_sendTimingInfo)
	{
		MidiSend(0xFC);	// Stop
		MidiSend(0xFA);	// Start
	}
}


void MidiInOut::Bypass(bool bypass)
{
	if(bypass)
	{
		mpt::lock_guard<mpt::mutex> lock(m_mutex);
		m_outQueue.clear();
	}
	IMidiPlugin::Bypass(bypass);
}


bool MidiInOut::MidiSend(uint32 midiCode)
{
	if(!m_midiOut.isPortOpen() || IsBypassed())
	{
		// We need an output device to send MIDI messages to.
		return true;
	}

	mpt::lock_guard<mpt::mutex> lock(m_mutex);
	m_outQueue.push_back(Message(GetOutputTimestamp(), &midiCode, 3));
	return true;
}


bool MidiInOut::MidiSysexSend(mpt::const_byte_span sysex)
{
	if(!m_midiOut.isPortOpen() || IsBypassed())
	{
		// We need an output device to send MIDI messages to.
		return true;
	}

	mpt::lock_guard<mpt::mutex> lock(m_mutex);
	m_outQueue.push_back(Message(GetOutputTimestamp(), sysex.data(), sysex.size()));
	return true;
}


void MidiInOut::HardAllNotesOff()
{
	const bool wasSuspended = !IsResumed();
	if(wasSuspended)
	{
		Resume();
	}

	for(uint8 mc = 0; mc < std::size(m_MidiCh); mc++)		//all midi chans
	{
		PlugInstrChannel &channel = m_MidiCh[mc];
		channel.ResetProgram();

		MidiPitchBend(mc, EncodePitchBendParam(MIDIEvents::pitchBendCentre));		// centre pitch bend
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllSoundOff, mc, 0));			// all sounds off

		for(size_t i = 0; i < std::size(channel.noteOnMap); i++)	//all notes
		{
			for(auto &c : channel.noteOnMap[i])
			{
				while(c != 0)
				{
					MidiSend(MIDIEvents::NoteOff(mc, static_cast<uint8>(i), 0));
					c--;
				}
			}
		}
	}

	if(wasSuspended)
	{
		Suspend();
	}
}


// Open a device for input or output.
void MidiInOut::OpenDevice(MidiDevice::ID newDevice, bool asInputDevice)
{
	MidiDevice &device = asInputDevice ? m_inputDevice : m_outputDevice;

	if(device.index == newDevice && device.stream.isPortOpen())
	{
		// No need to re-open this device.
		return;
	}

	CloseDevice(device);

	device.index = newDevice;
	device.stream.closePort();

	if(device.index == kNoDevice)
	{
		// Dummy device
		device.name = "<none>";
		return;
	}

	device.name = device.GetPortName(newDevice);
	//if(m_isResumed)
	{
		mpt::lock_guard<mpt::mutex> lock(m_mutex);
		
		try
		{
			device.stream.openPort(newDevice);
			if(asInputDevice)
			{
				m_midiIn.setCallback(InputCallback, this);
				m_midiIn.ignoreTypes(false, true, true);
			}
		} catch(RtMidiError &error)
		{
			device.name = "Unavailable";
			MidiInOutEditor *editor = dynamic_cast<MidiInOutEditor *>(GetEditor());
			if(editor != nullptr)
			{
				Reporting::Error("MIDI device cannot be opened. Is it open in another application?\n\n" + error.getMessage(), "MIDI Input / Output", editor);
			}
		}
	}
}


// Close an active device.
void MidiInOut::CloseDevice(MidiDevice &device)
{
	if(device.stream.isPortOpen())
	{
		mpt::lock_guard<mpt::mutex> lock(m_mutex);
		device.stream.closePort();
	}
}


// Calculate the current output timestamp
double MidiInOut::GetOutputTimestamp() const
{
	return m_clock.Now() * (1.0 / 1000.0) + GetOutputLatency() + m_latency;
}


// Get a device name
std::string MidiDevice::GetPortName(MidiDevice::ID port)
{
	return stream.getPortName(port);
}


OPENMPT_NAMESPACE_END
