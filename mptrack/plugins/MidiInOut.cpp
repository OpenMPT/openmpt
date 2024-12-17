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
#include "../../soundlib/MIDIMacroParser.h"
#include "../../soundlib/Sndfile.h"
#include "../Reporting.h"
#include <algorithm>
#include <sstream>
#ifdef MODPLUG_TRACKER
#include "../Mptrack.h"
#endif
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"


OPENMPT_NAMESPACE_BEGIN


IMixPlugin* MidiInOut::Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN &mixStruct)
{
	try
	{
		return new (std::nothrow) MidiInOut(factory, sndFile, mixStruct);
	} catch(RtMidiError &)
	{
		return nullptr;
	}
}


MidiInOut::MidiInOut(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN &mixStruct)
	: IMidiPlugin(factory, sndFile, mixStruct)
	, m_inputDevice(m_midiIn)
	, m_outputDevice(m_midiOut)
#ifdef MODPLUG_TRACKER
	, m_programName(_T("Default"))
#endif // MODPLUG_TRACKER
{
	m_mixBuffer.Initialize(2, 2);
}


MidiInOut::~MidiInOut()
{
	MidiInOut::Suspend();
}


uint32 MidiInOut::GetLatency() const
{
	// There is only a latency if the user-provided latency value is greater than the negative output latency.
	return mpt::saturate_round<uint32>(std::min(0.0, m_latency + (m_outputDevice.index == kInternalDevice ? 0.0 : GetOutputLatency())) * m_SndFile.GetSampleRate());
}


void MidiInOut::SaveAllParameters()
{
	auto chunk = GetChunk(false);
	if(chunk.empty())
		return;

	m_pMixStruct->defaultProgram = -1;
	m_pMixStruct->pluginData.assign(chunk.begin(), chunk.end());
}


void MidiInOut::RestoreAllParameters(int32 /*program*/)
{
	if(!m_pMixStruct)
		return;
	if(m_pMixStruct->pluginData.size() == sizeof(uint32) * 3)
	{
		// Very old plugin versions
		FileReader memFile(mpt::as_span(m_pMixStruct->pluginData));
		uint32 type = memFile.ReadUint32LE();
		if(type != 0)
			return;

		constexpr auto ParameterToDeviceID = [](float value)
		{
			return static_cast<MidiDevice::ID>(value * 65536.0f) - 1;
		};
		m_inputDevice.index = ParameterToDeviceID(memFile.ReadFloatLE());
		m_outputDevice.index = ParameterToDeviceID(memFile.ReadFloatLE());
	} else
	{
		SetChunk(mpt::as_span(m_pMixStruct->pluginData), false);
	}
	OpenDevice(m_inputDevice.index, true);
	OpenDevice(m_outputDevice.index, false);
	// Update selection in editor
	MidiInOutEditor *editor = dynamic_cast<MidiInOutEditor *>(GetEditor());
	if(editor != nullptr)
	{
		editor->SetCurrentDevice(true, m_inputDevice.index);
		editor->SetCurrentDevice(false, m_outputDevice.index);
	}
}


enum ChunkFlags
{
	kLatencyCompensation = 0x01,  // Implicit in current plugin version
	kLatencyPresent      = 0x02,  // Latency value is present as double-precision float
	kIgnoreTiming        = 0x04,  // Do not send timing and sequencing information
	kFriendlyInputName   = 0x08,  // Preset also stores friendly name of input device
	kFriendlyOutputName  = 0x10,  // Preset also stores friendly name of output device
	kMacrosPresent       = 0x20,  // Preset also stores initial MIDI dump / parameter macros
	kAlwaysSendDump      = 0x40,  // Send initial MIDI dump on every song restart
};

IMixPlugin::ChunkData MidiInOut::GetChunk(bool /*isBank*/)
{
	const std::string programName8 = mpt::ToCharset(mpt::Charset::UTF8, m_programName);
	uint32 flags = kLatencyCompensation | kLatencyPresent | (m_sendTimingInfo ? 0 : kIgnoreTiming);
#ifdef MODPLUG_TRACKER
	const std::string inFriendlyName = (m_inputDevice.index != MidiDevice::NO_MIDI_DEVICE && !m_inputDevice.friendlyName.empty()) ? mpt::ToCharset(mpt::Charset::UTF8, m_inputDevice.friendlyName) : m_inputDevice.name;
	const std::string outFriendlyName = (m_outputDevice.index != MidiDevice::NO_MIDI_DEVICE && !m_outputDevice.friendlyName.empty()) ? mpt::ToCharset(mpt::Charset::UTF8, m_outputDevice.friendlyName) : m_outputDevice.name;
	if(inFriendlyName != m_inputDevice.name)
	{
		flags |= kFriendlyInputName;
	}
	if(outFriendlyName != m_outputDevice.name)
	{
		flags |= kFriendlyOutputName;
	}
#endif
	if(!m_initialMidiDump.empty())
	{
		flags |= kMacrosPresent;
		if(m_alwaysSendInitialDump)
			flags |= kAlwaysSendDump;
	} else
	{
		for(const auto &macro : m_parameterMacros)
		{
			if(!macro.first.empty())
			{
				flags |= kMacrosPresent;
				break;
			}
		}
	}

	std::ostringstream s;
	mpt::IO::WriteRaw(s, "fEvN", 4);  // VST program chunk magic
	mpt::IO::WriteIntLE< int32>(s, GetVersion());
	mpt::IO::WriteIntLE<uint32>(s, 1);  // Number of programs
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
	if(flags & kMacrosPresent)
	{
		if(!m_initialMidiDump.empty())
		{
			mpt::IO::WriteIntLE<uint32>(s, uint32_max);  // Initial dump ID
			mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(m_initialMidiDump.size()));
			mpt::IO::Write(s, m_initialMidiDump);
		}
		for(size_t i = 0; i < m_parameterMacros.size(); i++)
		{
			if(!m_parameterMacros[i].first.empty())
			{
				mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(i + kMacroParamMin));  // Parameter ID
				mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(m_parameterMacros[i].first.size()));
				mpt::IO::WriteRaw(s, m_parameterMacros[i].first.c_str(), m_parameterMacros[i].first.size());
			}
		}
		// End of macro section
		mpt::IO::WriteIntLE<uint32>(s, 0);
		mpt::IO::WriteIntLE<uint32>(s, 0);
	}
	m_chunkData = s.str();
	return mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(m_chunkData));
}


// Try to match a port name against stored name or friendly name (preferred)
static MidiDevice::ID FindPort(MidiDevice::ID id, unsigned int numPorts, const std::string &name, const mpt::ustring &friendlyName, MidiDevice &midiDevice, bool isInput)
{
	if(id == MidiDevice::NO_MIDI_DEVICE || id == MidiDevice::INTERNAL_MIDI_DEVICE)
		return id;
	bool foundFriendly = false;
	for(unsigned int i = 0; i < numPorts; i++)
	{
		try
		{
			auto portName = midiDevice.GetPortName(i);
			bool deviceNameMatches = (portName == name);
#ifdef MODPLUG_TRACKER
			if(!friendlyName.empty() && friendlyName == theApp.GetFriendlyMIDIPortName(mpt::ToUnicode(mpt::Charset::UTF8, portName), isInput, false))
			{
				// Preferred match
				id = i;
				foundFriendly = true;
				if(deviceNameMatches)
				{
					return id;
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
	return id;
}


void MidiInOut::SetChunk(const ChunkData &chunk, bool /*isBank*/)
{
	FileReader file(chunk);
	if(!file.CanRead(9 * sizeof(uint32))
		|| !file.ReadMagic("fEvN")            // VST program chunk magic
		|| file.ReadInt32LE() > GetVersion()  // Plugin version
		|| file.ReadUint32LE() < 1)           // Number of programs
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
		file.ReadSizedString<uint32le, mpt::String::maybeNullTerminated>(inFriendlyName);
	if(flags & kFriendlyOutputName)
		file.ReadSizedString<uint32le, mpt::String::maybeNullTerminated>(outFriendlyName);

	m_parameterMacros.clear();
	m_initialMidiDump.clear();
	m_initialDumpSent = false;
	if(flags & kMacrosPresent)
	{
		while(file.CanRead(sizeof(uint32le) * 2))
		{
			const auto [dumpID, dumpSize] = file.ReadArray<uint32le, 2>();
			if(!dumpSize || !file.CanRead(dumpSize))
				break;
			if(dumpID == uint32_max)
			{
				file.ReadVector(m_initialMidiDump, dumpSize);
				m_alwaysSendInitialDump = (flags & kAlwaysSendDump) != 0;
			} else if(dumpID >= kMacroParamMin && dumpID <= kMacroParamMax)
			{
				m_parameterMacros.resize(dumpID - kMacroParamMin + 1);
				auto &str = m_parameterMacros[dumpID - kMacroParamMin].first;
				file.ReadString<mpt::String::maybeNullTerminated>(str, dumpSize);
				std::string::size_type pos;
				while((pos = str.find_first_not_of(" 0123456789ABCDEFabchmnopsuvxyz")) != std::string::npos)
				{
					str.erase(pos, 1);
				}
				m_parameterMacroScratchSpace.reserve(str.size() + 1);
			}
		}
	}

	// Try to match an input port name against stored name or friendly name (preferred)
	m_inputDevice.friendlyName = mpt::ToUnicode(mpt::Charset::UTF8, inFriendlyName);
	m_outputDevice.friendlyName = mpt::ToUnicode(mpt::Charset::UTF8, outFriendlyName);
	m_inputDevice.index = FindPort(inID, m_midiIn.getPortCount(), inName, m_inputDevice.friendlyName, m_inputDevice, true);
	m_outputDevice.index = FindPort(outID, m_midiOut.getPortCount(), outName, m_outputDevice.friendlyName, m_outputDevice, false);
}


void MidiInOut::SetInitialMidiDump(std::vector<uint8> dump)
{
	mpt::lock_guard<mpt::mutex> lock(m_mutex);
	m_initialMidiDump = std::move(dump);
	SetModified();
}


void MidiInOut::SetMacro(size_t index, std::string macro)
{
	if(index < kMacroParamMin)
		return;
	index -= kMacroParamMin;
	mpt::lock_guard<mpt::mutex> lock(m_mutex);
	if(index >= m_parameterMacros.size())
		m_parameterMacros.resize(index + 1);
	m_parameterMacroScratchSpace.reserve(macro.size() + 1);
	m_parameterMacros[index].first = std::move(macro);
	SetModified();
}


std::string MidiInOut::GetMacro(size_t index) const
{
	if(index >= kMacroParamMin && (index - kMacroParamMin) < m_parameterMacros.size())
		return m_parameterMacros[index - kMacroParamMin].first;
	return {};
}


void MidiInOut::SetParameter(PlugParamIndex index, PlugParamValue value, PlayState *playState, CHANNELINDEX chn)
{
	if(index >= kMacroParamMin && (index - kMacroParamMin) < m_parameterMacros.size())
	{
		// Enough memory should have already been allocated when the macro string was set
		m_parameterMacroScratchSpace.resize(m_parameterMacros[index - kMacroParamMin].first.size() + 1);
		m_parameterMacros[index - kMacroParamMin].second = value;
		MIDIMacroParser parser{GetSoundFile(), playState, chn, false, mpt::as_span(m_parameterMacros[index - kMacroParamMin].first), mpt::as_span(m_parameterMacroScratchSpace), mpt::saturate_round<uint8>(value * 127.0f), static_cast<PLUGINDEX>(GetSlot() + 1)};
		mpt::span<uint8> midiMsg;
		while(parser.NextMessage(midiMsg))
		{
			MidiSend(mpt::byte_cast<mpt::const_byte_span>(midiMsg));
		}
	}
}


float MidiInOut::GetParameter(PlugParamIndex index)
{
	if(index >= kMacroParamMin && (index - kMacroParamMin) < m_parameterMacros.size())
		return m_parameterMacros[index - kMacroParamMin].second;
	return 0.0f;
}


#ifdef MODPLUG_TRACKER

CString MidiInOut::GetParamName(PlugParamIndex param)
{
	if(param >= kMacroParamMin && (param - kMacroParamMin) < m_parameterMacros.size())
		return mpt::ToCString(mpt::Charset::ASCII, m_parameterMacros[param - kMacroParamMin].first);
	return {};
}


// Parameter value as text
CString MidiInOut::GetParamDisplay(PlugParamIndex param)
{
	if(param >= kMacroParamMin && (param - kMacroParamMin) < m_parameterMacros.size())
		return mpt::cfmt::dec(mpt::saturate_round<uint8>(m_parameterMacros[param - kMacroParamMin].second * 127.0f));
	return {};
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
	if(m_outputDevice.index == kInternalDevice || m_midiOut.isPortOpen())
	{
		mpt::lock_guard<mpt::mutex> lock(m_mutex);

		if(!m_initialDumpSent && !m_initialMidiDump.empty())
		{
			try
			{
				MIDIMacroParser parser{mpt::as_span(m_initialMidiDump)};
				mpt::span<uint8> midiMsg;
				while(parser.NextMessage(midiMsg))
				{
					SendMessage(midiMsg);
				}
			} catch(const RtMidiError &)
			{
			}
		}
		m_initialDumpSent = true;

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

		if(m_sendTimingInfo && !m_positionChanged && m_SndFile.m_PlayState.m_ppqPosFract == 0.0)
		{
			// Send Song Position on every pattern change or start of new measure
			uint16 ppq = mpt::saturate_trunc<uint16>((m_SndFile.m_PlayState.m_ppqPosBeat + m_SndFile.m_PlayState.m_ppqPosFract) * 4.0);
			if(ppq < 16384)
			{
				uint32 midiCode = MIDIEvents::SongPosition(ppq);
				m_outQueue.push_back(Message(GetOutputTimestamp(), &midiCode, MIDIEvents::GetEventLength(static_cast<uint8>(midiCode))));
			}
		}

		double now = m_clock.Now() * (1.0 / 1000.0);
		auto message = m_outQueue.begin();
		while(message != m_outQueue.end() && message->m_time <= now)
		{
			try
			{
				SendMessage(*message);
			} catch(const RtMidiError &)
			{
			}
			message++;
		}
		m_outQueue.erase(m_outQueue.begin(), message);
	}
	m_positionChanged = false;
}


void MidiInOut::SendMessage(mpt::span<const unsigned char> midiMsg)
{
	if(m_outputDevice.index == kInternalDevice)
		ReceiveMidi(mpt::byte_cast<mpt::const_byte_span>(midiMsg));
	else
		m_midiOut.sendMessage(midiMsg.data(), midiMsg.size());
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
				ReceiveMidi(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(m_bufferedInput)));
			m_bufferedInput.clear();
		}
	} else if(message.front() == 0xF0)
	{
		// Start of SysEx message...
		if(message.back() != 0xF7)
			m_bufferedInput.insert(m_bufferedInput.end(), message.begin(), message.end());  // ...but not the end!
		else if(!isBypassed)
			ReceiveMidi(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(message)));
	} else if(!isBypassed)
	{
		// Regular message
		ReceiveMidi(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(message)));
	}
}


// Resume playback
void MidiInOut::Resume()
{
	// Resume MIDI I/O
	m_isResumed = true;
	m_nextClock = 0;
	{
		mpt::lock_guard<mpt::mutex> lock(m_mutex);
		m_outQueue.clear();
	}
	m_clock.SetResolution(1);
	OpenDevice(m_inputDevice, true);
	OpenDevice(m_outputDevice, false);
	if(m_midiOut.isPortOpen() && m_sendTimingInfo && !m_SndFile.IsPaused())
	{
		MidiSend(0xFA);  // Start
	}
	if(m_alwaysSendInitialDump)
		m_initialDumpSent = false;
}


// Stop playback
void MidiInOut::Suspend()
{
	// Suspend MIDI I/O
	if(m_outputDevice.index == kInternalDevice || m_midiOut.isPortOpen())
	{
		try
		{
			mpt::lock_guard<mpt::mutex> lock(m_mutex);

			// Need to flush remaining events from HardAllNotesOff
			for(const auto &message : m_outQueue)
			{
				SendMessage(message);
			}
			m_outQueue.clear();
			if(m_sendTimingInfo)
			{
				unsigned char message[1] = { 0xFC };  // Stop
				SendMessage(message);
			}
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
	if(m_sendTimingInfo && !m_SndFile.IsPaused())
	{
		MidiSend(0xFC);  // Stop
		uint16 ppq = mpt::saturate_trunc<uint16>((m_SndFile.m_PlayState.m_ppqPosBeat + m_SndFile.m_PlayState.m_ppqPosFract) * 4.0);
		if(ppq < 16384)
			MidiSend(MIDIEvents::SongPosition(ppq));
		MidiSend(0xFA);  // Start
	}
	m_positionChanged = true;
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


bool MidiInOut::MidiSend(mpt::const_byte_span midiData)
{
	if((m_outputDevice.index != kInternalDevice && !m_midiOut.isPortOpen()) || IsBypassed())
	{
		// We need an output device to send MIDI messages to.
		return true;
	}

	mpt::lock_guard<mpt::mutex> lock(m_mutex);
	m_outQueue.push_back(Message(GetOutputTimestamp(), midiData.data(), midiData.size()));
	return true;
}


void MidiInOut::HardAllNotesOff()
{
	const bool wasSuspended = !IsResumed();
	if(wasSuspended)
	{
		Resume();
	}

	for(uint8 mc = 0; mc < std::size(m_MidiCh); mc++)  //all midi chans
	{
		PlugInstrChannel &channel = m_MidiCh[mc];
		channel.ResetProgram(m_SndFile.m_playBehaviour[kPluginDefaultProgramAndBank1]);

		SendMidiPitchBend(mc, EncodePitchBendParam(MIDIEvents::pitchBendCentre));  // centre pitch bend
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllSoundOff, mc, 0));           // all sounds off

		for(size_t i = 0; i < std::size(channel.noteOnMap); i++)
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
void MidiInOut::OpenDevice(MidiDevice newDevice, bool asInputDevice)
{
	newDevice.index = FindPort(newDevice.index, asInputDevice ? m_midiIn.getPortCount() : m_midiOut.getPortCount(), newDevice.name, newDevice.friendlyName, newDevice, asInputDevice);
	OpenDevice(newDevice.index, asInputDevice, false);
}


// Open a device for input or output.
void MidiInOut::OpenDevice(MidiDevice::ID newDevice, bool asInputDevice, bool updateName)
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
		if(updateName)
		{
			device.name = "<none>";
			device.friendlyName.clear();
		}
		return;
	} else if(device.index == kInternalDevice)
	{
		if(updateName)
		{
			device.name = "<internal>";
			device.friendlyName.clear();
		}
		return;
	}

	if(updateName)
	{
		device.name = device.GetPortName(newDevice);
#ifdef MODPLUG_TRACKER
		device.friendlyName = theApp.GetFriendlyMIDIPortName(mpt::ToUnicode(mpt::Charset::UTF8, device.name), asInputDevice, false);
#endif // MODPLUG_TRACKER
	}
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
	return m_clock.Now() * (1.0 / 1000.0) + (m_outputDevice.index == kInternalDevice ? 0.0 : GetOutputLatency()) + m_latency;
}


// Get a device name
std::string MidiDevice::GetPortName(MidiDevice::ID port)
{
	return stream.getPortName(port);
}


OPENMPT_NAMESPACE_END
