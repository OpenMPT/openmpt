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
#include "../../soundlib/Sndfile.h"
#include <algorithm>
#include <sstream>


OPENMPT_NAMESPACE_BEGIN


IMixPlugin* MidiInOut::Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
//------------------------------------------------------------------------------------------------
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
	, m_latency(0.0)
	, m_inputDevice(m_midiIn)
	, m_outputDevice(m_midiOut)
	, m_latencyCompensation(true)
	, m_programName(_T("Default"))
//---------------------------------------------------------------------------------------
{
	m_mixBuffer.Initialize(2, 2);
	InsertIntoFactoryList();
}


MidiInOut::~MidiInOut()
//---------------------
{
	Suspend();
}


uint32 MidiInOut::GetLatency() const
//----------------------------------
{
	// There is only a latency if the user-provided latency value is less than the negative output latency.
	return Util::Round<uint32>(std::min(0.0, m_latency + GetOutputLatency()) * m_SndFile.GetSampleRate());
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
	const std::string programName8 = mpt::ToCharset(mpt::CharsetUTF8, m_programName);
	const std::string inputName8 = mpt::ToCharset(mpt::CharsetUTF8, mpt::CharsetLocale, m_inputDevice.name);
	const std::string outputName8 = mpt::ToCharset(mpt::CharsetUTF8, mpt::CharsetLocale, m_outputDevice.name);

	std::ostringstream s;
	mpt::IO::WriteRaw(s, "fEvN", 4);	// VST program chunk magic
	mpt::IO::WriteIntLE< int32>(s, GetVersion());
	mpt::IO::WriteIntLE<uint32>(s, 1);	// Number of programs
	mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(programName8.size()));
	mpt::IO::WriteIntLE<uint32>(s, m_inputDevice.index);
	mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(m_inputDevice.name.size()));
	mpt::IO::WriteIntLE<uint32>(s, m_outputDevice.index);
	mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(m_outputDevice.name.size()));
	mpt::IO::WriteIntLE<uint32>(s, m_latencyCompensation);
	mpt::IO::WriteRaw(s, programName8.c_str(), programName8.size());
	mpt::IO::WriteRaw(s, inputName8.c_str(), inputName8.size());
	mpt::IO::WriteRaw(s, outputName8.c_str(), outputName8.size());
	m_chunkData = s.str();
	chunk = const_cast<char *>(m_chunkData.c_str());
	return m_chunkData.size();
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
	MidiDevice::ID inID = file.ReadUint32LE();
	uint32 inStrSize = file.ReadUint32LE();
	MidiDevice::ID outID = file.ReadUint32LE();
	uint32 outStrSize = file.ReadUint32LE();
	m_latencyCompensation = file.ReadUint32LE() != 0;

	std::string s;
	file.ReadString<mpt::String::maybeNullTerminated>(s, nameStrSize);
	m_programName = mpt::ToCString(mpt::CharsetUTF8, s);

	file.ReadString<mpt::String::maybeNullTerminated>(s, inStrSize);
	s = mpt::ToCharset(mpt::CharsetLocale, mpt::CharsetUTF8, s);
	if(s != m_inputDevice.GetPortName(inID))
	{
		// Stored name differs from actual device name - try finding another device with the same name.
		unsigned int ports = m_midiIn.getPortCount();
		for(unsigned int i = 0; i < ports; i++)
		{
			try
			{
				if(s == m_inputDevice.GetPortName(i))
				{
					inID = i;
					break;
				}
			} catch(RtMidiError &)
			{
			}
		}
	}

	file.ReadString<mpt::String::maybeNullTerminated>(s, outStrSize);
	s = mpt::ToCharset(mpt::CharsetLocale, mpt::CharsetUTF8, s);
	if(s != m_outputDevice.GetPortName(outID))
	{
		// Stored name differs from actual device name - try finding another device with the same name.
		unsigned int ports = m_midiOut.getPortCount();
		for(unsigned int i = 0; i < ports; i++)
		{
			try
			{
				if(s == m_outputDevice.GetPortName(i))
				{
					outID = i;
					break;
				}
			} catch(RtMidiError &)
			{
			}
		}
	}

	SetParameter(MidiInOut::kInputParameter, DeviceIDToParameter(inID));
	SetParameter(MidiInOut::kOutputParameter, DeviceIDToParameter(outID));
}


void MidiInOut::SetParameter(PlugParamIndex index, PlugParamValue value)
//----------------------------------------------------------------------
{
	MidiDevice::ID newDevice = ParameterToDeviceID(value);
	OpenDevice(newDevice, (index == kInputParameter));

	// Update selection in editor
	MidiInOutEditor *editor = dynamic_cast<MidiInOutEditor *>(GetEditor());
	if(editor != nullptr)
		editor->SetCurrentDevice((index == kInputParameter), newDevice);
}


float MidiInOut::GetParameter(PlugParamIndex index)
//-------------------------------------------------
{
	const MidiDevice &device = (index == kInputParameter) ? m_inputDevice : m_outputDevice;
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
	const MidiDevice &device = (param == kInputParameter) ? m_inputDevice : m_outputDevice;
	return device.name.c_str();
}


CAbstractVstEditor *MidiInOut::OpenEditor()
//-----------------------------------------
{
	try
	{
		return new MidiInOutEditor(*this);
	} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
	{
		MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
		return nullptr;
	}
}

#endif // MODPLUG_TRACKER


// Processing (we don't process any audio, only MIDI messages)
void MidiInOut::Process(float *, float *, uint32 numFrames)
//---------------------------------------------------------
{
	if(m_midiOut.isPortOpen())
	{
		MPT_LOCK_GUARD<mpt::mutex> lock(m_mutex);

		// Send MIDI clock
		if(m_nextClock < 1)
		{
			Message message;
			message.time = GetOutputTimestamp();
			message.msg.assign(1, 0xF8);
			m_outQueue.push_back(message);

			double bpm = m_SndFile.GetCurrentBPM();
			if(bpm != 0.0)
			{
				m_nextClock += 2.5 * m_SndFile.GetSampleRate() / bpm;
			}
		}
		m_nextClock -= numFrames;

		double now = m_clock.Now() * (1.0 / 1000.0);
		auto message = m_outQueue.begin();
		while(message != m_outQueue.end() && message->time <= now)
		{
			m_midiOut.sendMessage(&message->msg);
			message++;
		}
		m_outQueue.erase(m_outQueue.begin(), message);
	}
}


void MidiInOut::InputCallback(double /*deltatime*/, std::vector<unsigned char> &message)
//--------------------------------------------------------------------------------------
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
				ReceiveSysex(m_bufferedInput.data(), m_bufferedInput.size());
			m_bufferedInput.clear();
		}
	} else if(message.front() == 0xF0)
	{
		// Start of SysEx message...
		if(message.back() != 0xF7)
			m_bufferedInput.insert(m_bufferedInput.end(), message.begin(), message.end());	// ...but not the end!
		else if(!isBypassed)
			ReceiveSysex(message.data(), message.size());
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
//----------------------
{
	// Resume MIDI I/O
	m_isResumed = true;
	m_nextClock = 0;
	m_outQueue.clear();
	m_clock.SetResolution(1);
	OpenDevice(m_inputDevice.index, true);
	OpenDevice(m_outputDevice.index, false);
	if(m_midiOut.isPortOpen())
	{
		std::vector<unsigned char> message(1, 0xFA);	// Start
		m_midiOut.sendMessage(&message);
	}
}


// Stop playback
void MidiInOut::Suspend()
//-----------------------
{
	// Suspend MIDI I/O
	if(m_midiOut.isPortOpen())
	{
		std::vector<unsigned char> message(1, 0xFC);	// Stop
		m_midiOut.sendMessage(&message);
	}
	//CloseDevice(inputDevice);
	CloseDevice(m_outputDevice);
	m_clock.SetResolution(0);
	m_isResumed = false;
}


// Playback discontinuity
void MidiInOut::PositionChanged()
//-------------------------------
{
	MidiSend(0xFC);	// Stop
	MidiSend(0xFA);	// Start
}


void MidiInOut::Bypass(bool bypass)
//---------------------------------
{
	if(bypass)
	{
		MPT_LOCK_GUARD<mpt::mutex> lock(m_mutex);
		m_outQueue.clear();
	}
	IMidiPlugin::Bypass(bypass);
}


bool MidiInOut::MidiSend(uint32 midiCode)
//---------------------------------------
{
	if(!m_midiOut.isPortOpen() || IsBypassed())
	{
		// We need an output device to send MIDI messages to.
		return true;
	}

	MPT_LOCK_GUARD<mpt::mutex> lock(m_mutex);
	Message message;
	message.time = GetOutputTimestamp();
	message.msg.resize(3, 0);
	memcpy(message.msg.data(), &midiCode, 3);
	m_outQueue.push_back(message);
	return true;
}


bool MidiInOut::MidiSysexSend(const void *sysex, uint32 length)
//-------------------------------------------------------------
{
	if(!m_midiOut.isPortOpen() || IsBypassed())
	{
		// We need an output device to send MIDI messages to.
		return true;
	}

	MPT_LOCK_GUARD<mpt::mutex> lock(m_mutex);
	Message message;
	message.time = GetOutputTimestamp();
	message.msg.assign(static_cast<const unsigned char *>(sysex), static_cast<const unsigned char *>(sysex) + length);
	m_outQueue.push_back(message);
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
//----------------------------------------------------------------------
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
		// Don't open MIDI devices if we're not processing.
		// This has to be done since we receive MIDI events in processReplacing(),
		// so if no processing is happening, some irrelevant events might be queued until the next processing happens...
		MPT_LOCK_GUARD<mpt::mutex> lock(m_mutex);
		
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
				Reporting::Error("MIDI device cannot be opened:" + error.getMessage(), "MIDI Input / Output", editor);
			}
		}
	}
}


// Close an active device.
void MidiInOut::CloseDevice(MidiDevice &device)
//---------------------------------------------
{
	if(device.stream.isPortOpen())
	{
		MPT_LOCK_GUARD<mpt::mutex> lock(m_mutex);
		device.stream.closePort();
	}
}


// Calculate the current output timestamp
double MidiInOut::GetOutputTimestamp() const
//------------------------------------------
{
	if(m_latencyCompensation)
		return m_clock.Now() * (1.0 / 1000.0) + GetOutputLatency() + m_latency;
	else
		return 0.0;
}


// Get a device name
std::string MidiDevice::GetPortName(MidiDevice::ID port)
//------------------------------------------------------
{
	std::string portName = stream.getPortName(port);
#if MPT_OS_WINDOWS
	// Remove auto-appended port number
	if(portName.length() >= 2)
		return portName.substr(0, portName.find_last_of(' '));
#endif
	return portName;
}


OPENMPT_NAMESPACE_END
