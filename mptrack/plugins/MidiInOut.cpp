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
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"


OPENMPT_NAMESPACE_BEGIN


IMixPlugin* MidiInOut::Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN &mixStruct)
{
	return new (std::nothrow) MidiInOut(factory, sndFile, mixStruct);
}


MidiInOut::MidiInOut(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN &mixStruct)
	: IMidiPlugin(factory, sndFile, mixStruct)
#ifdef MODPLUG_TRACKER
	, m_programName(_T("Default"))
#endif // MODPLUG_TRACKER
{
	m_inputDevice.name = "<none>";
	m_outputDevice.name = "<none>";
	m_mixBuffer.Initialize(2, 2);
}


MidiInOut::~MidiInOut()
{
	MidiInOut::Suspend();
}


uint32 MidiInOut::GetLatency() const
{
	// There is only a latency if the user-provided latency value is greater than the negative output latency.
	return mpt::saturate_round<uint32>(std::min(0.0, m_latency + (m_outputDevice.id == kInternalDevice ? 0.0 : GetOutputLatency())) * m_SndFile.GetSampleRate());
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
			return static_cast<MidiPortID>(value * 65536.0f) - 1;
		};
		m_inputDevice.id = ParameterToDeviceID(memFile.ReadFloatLE());
		m_outputDevice.id = ParameterToDeviceID(memFile.ReadFloatLE());
	} else
	{
		SetChunk(mpt::as_span(m_pMixStruct->pluginData), false);
	}
	OpenDevice(MidiDeviceManager::FindInputPort(m_inputDevice), true);
	OpenDevice(MidiDeviceManager::FindOutputPort(m_outputDevice), false);
	// Update selection in editor
	MidiInOutEditor *editor = dynamic_cast<MidiInOutEditor *>(GetEditor());
	if(editor != nullptr)
	{
		editor->SetCurrentDevice(true, m_inputDevice.id);
		editor->SetCurrentDevice(false, m_outputDevice.id);
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
	const std::string inFriendlyName = (m_inputDevice.id != kNoMidiDevice && !m_inputDevice.friendlyName.empty()) ? mpt::ToCharset(mpt::Charset::UTF8, m_inputDevice.friendlyName) : m_inputDevice.name;
	const std::string outFriendlyName = (m_outputDevice.id != kNoMidiDevice && !m_outputDevice.friendlyName.empty()) ? mpt::ToCharset(mpt::Charset::UTF8, m_outputDevice.friendlyName) : m_outputDevice.name;
	if(inFriendlyName != m_inputDevice.name)
	{
		flags |= kFriendlyInputName;
	}
	if(outFriendlyName != m_outputDevice.name)
	{
		flags |= kFriendlyOutputName;
	}
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
	mpt::IO::WriteIntLE<uint32>(s, m_inputDevice.id);
	mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(m_inputDevice.name.size()));
	mpt::IO::WriteIntLE<uint32>(s, m_outputDevice.id);
	mpt::IO::WriteIntLE<uint32>(s, static_cast<uint32>(m_outputDevice.name.size()));
	mpt::IO::WriteIntLE<uint32>(s, flags);
	mpt::IO::WriteRaw(s, programName8.c_str(), programName8.size());
	mpt::IO::WriteRaw(s, m_inputDevice.name.c_str(), m_inputDevice.name.size());
	mpt::IO::WriteRaw(s, m_outputDevice.name.c_str(), m_outputDevice.name.size());
	mpt::IO::WriteIntLE<uint64>(s, IEEE754binary64LE(m_latency).GetInt64());
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


void MidiInOut::SetChunk(const ChunkData &chunk, bool /*isBank*/)
{
	FileReader file(chunk);
	if(!file.CanRead(9 * sizeof(uint32))
		|| !file.ReadMagic("fEvN")            // VST program chunk magic
		|| file.ReadInt32LE() > GetVersion()  // Plugin version
		|| file.ReadUint32LE() < 1)           // Number of programs
		return;

	uint32 nameStrSize = file.ReadUint32LE();
	m_inputDevice.id = file.ReadUint32LE();
	uint32 inStrSize = file.ReadUint32LE();
	m_outputDevice.id = file.ReadUint32LE();
	uint32 outStrSize = file.ReadUint32LE();
	uint32 flags = file.ReadUint32LE();

	std::string progName;
	file.ReadString<mpt::String::maybeNullTerminated>(progName, nameStrSize);
	m_programName = mpt::ToCString(mpt::Charset::UTF8, progName);

	file.ReadString<mpt::String::maybeNullTerminated>(m_inputDevice.name, inStrSize);
	file.ReadString<mpt::String::maybeNullTerminated>(m_outputDevice.name, outStrSize);

	if(flags & kLatencyPresent)
		m_latency = file.ReadDoubleLE();
	else
		m_latency = 0.0f;
	m_sendTimingInfo = !(flags & kIgnoreTiming);

	std::string inFriendlyName, outFriendlyName;
	if(flags & kFriendlyInputName)
		file.ReadSizedString<uint32le, mpt::String::maybeNullTerminated>(inFriendlyName);
	if(flags & kFriendlyOutputName)
		file.ReadSizedString<uint32le, mpt::String::maybeNullTerminated>(outFriendlyName);
	m_inputDevice.friendlyName = mpt::ToUnicode(mpt::Charset::UTF8, inFriendlyName);
	m_outputDevice.friendlyName = mpt::ToUnicode(mpt::Charset::UTF8, outFriendlyName);

	// Try to match port against stored name or friendly name (preferred)
	m_inputDevice.id = MidiDeviceManager::FindInputPort(m_inputDevice);
	m_outputDevice.id = MidiDeviceManager::FindOutputPort(m_outputDevice);

	mpt::lock_guard<mpt::recursive_mutex> lock{m_mutex};
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
				std::string::size_type pos = 0;
				while((pos = str.find_first_not_of(" 0123456789ABCDEFabchmnopsuvxyz", pos)) != std::string::npos)
				{
					str.erase(pos, 1);
				}
				m_parameterMacroScratchSpace.reserve(str.size() + 1);
			}
		}
	}
}


void MidiInOut::SetInitialMidiDump(std::vector<uint8> dump)
{
	mpt::lock_guard<mpt::recursive_mutex> lock{m_mutex};
	m_initialMidiDump = std::move(dump);
	SetModified();
}


void MidiInOut::SetMacro(size_t index, std::string macro)
{
	if(index < kMacroParamMin)
		return;
	index -= kMacroParamMin;
	mpt::lock_guard<mpt::recursive_mutex> lock{m_mutex};
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
		m_mutex.lock();
		// Enough memory should have already been allocated when the macro string was set
		m_parameterMacroScratchSpace.resize(m_parameterMacros[index - kMacroParamMin].first.size() + 1);
		m_parameterMacros[index - kMacroParamMin].second = value;
		MIDIMacroParser parser{GetSoundFile(), playState, chn, false, mpt::as_span(m_parameterMacros[index - kMacroParamMin].first), mpt::as_span(m_parameterMacroScratchSpace), mpt::saturate_round<uint8>(value * 127.0f), static_cast<PLUGINDEX>(GetSlot() + 1)};
		m_mutex.unlock();
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
	if(IsOutputOpen())
	{
		if(!m_initialDumpSent)
		{
			mpt::lock_guard<mpt::recursive_mutex> lock{m_mutex};
			if(!m_initialMidiDump.empty())
			{
				MIDIMacroParser parser{mpt::as_span(m_initialMidiDump)};
				mpt::span<uint8> midiMsg;
				while(parser.NextMessage(midiMsg))
				{
					MidiSend(mpt::byte_cast<mpt::const_byte_span>(midiMsg));
				}
			}
			m_initialDumpSent = true;
		}

		if(m_sendTimingInfo && !m_positionChanged && m_SndFile.m_PlayState.m_ppqPosFract == 0.0 && m_SndFile.m_PlayState.AtStartOfTick())
		{
			// Send Song Position on every pattern change or start of new measure
			uint16 ppq = mpt::saturate_trunc<uint16>((m_SndFile.m_PlayState.PPQPos()) * 4.0);
			if(ppq < 16384)
			{
				MidiSend(MIDIEvents::SongPosition(ppq));
			}
		}

		// Send MIDI clock
		if(int32 nextClock = mpt::saturate_round<int32>(m_nextClock); nextClock < static_cast<int32>(numFrames))
		{
			if(m_sendTimingInfo)
			{
				MidiSend(MIDIEvents::System(MIDIEvents::sysMIDIClock), nextClock);
			}

			double bpm = m_SndFile.GetCurrentBPM();
			if(bpm > 0.0)
			{
				m_nextClock += 2.5 * m_SndFile.GetSampleRate() / bpm;
			}
		}
		m_nextClock -= numFrames;
	}

	if(m_outputDevice.id == kInternalDevice)
	{
		const int64 endOfFrame = m_SndFile.m_TimingInfo.StreamFrames + m_SndFile.m_TimingInfo.FramesRenderedInChunk + numFrames;
		const auto sendMidiMsg = [endOfFrame, this](const MidiMessage &msg)
		{
			if(msg.m_streamFrame >= endOfFrame)
				return false;
			ReceiveMidi(msg);
			return true;
		};
		mpt::lock_guard<mpt::recursive_mutex> lock{m_mutex};
		mpt::erase_if(m_internalSendQueue, sendMidiMsg);
	}

	m_positionChanged = false;
}


// Receive data from MidiDeviceManager
void MidiInOut::OnMidiMessage(mpt::const_byte_span data)
{
	if(!data.empty() && !IsBypassed())
		ReceiveMidi(data);
}


// Resume playback
void MidiInOut::Resume()
{
	// Resume MIDI I/O
	m_isResumed = true;
	m_nextClock = 0.0;
	OpenDevice(m_inputDevice, true);
	OpenDevice(m_outputDevice, false);
	if(IsOutputOpen() && m_sendTimingInfo && !m_SndFile.IsPaused())
	{
		MidiSend(MIDIEvents::System(MIDIEvents::sysStart));
	}
	if(m_alwaysSendInitialDump)
		m_initialDumpSent = false;
}


// Stop playback
void MidiInOut::Suspend()
{
	// Suspend MIDI I/O
	if(IsOutputOpen())
	{
		if(m_sendTimingInfo)
		{
			MidiSend(MIDIEvents::System(MIDIEvents::sysStop));
		}
	}

	m_outputHandle.reset();
	m_isResumed = false;
}


// Playback discontinuity
void MidiInOut::PositionChanged()
{
	if(m_sendTimingInfo && !m_SndFile.IsPaused())
	{
		MidiSend(MIDIEvents::System(MIDIEvents::sysStop));
		uint16 ppq = mpt::saturate_trunc<uint16>((m_SndFile.m_PlayState.PPQPos()) * 4.0);
		if(ppq < 16384)
			MidiSend(MIDIEvents::SongPosition(ppq));
		MidiSend(MIDIEvents::System(MIDIEvents::sysStart));
	}
	m_positionChanged = true;
}


void MidiInOut::Bypass(bool bypass)
{
	IMidiPlugin::Bypass(bypass);
}


bool MidiInOut::MidiSend(mpt::const_byte_span midiData, int32 frameOffset)
{
	const int64 outputStreamFrame = GetOutputStreamFrame(frameOffset);
	if(m_outputDevice.id == kInternalDevice)
	{
		if(outputStreamFrame < m_SndFile.m_TimingInfo.StreamFrames + m_SndFile.m_TimingInfo.FramesRenderedInChunk)
		{
			ReceiveMidi(midiData);
		} else
		{
			mpt::lock_guard<mpt::recursive_mutex> lock{m_mutex};
			m_internalSendQueue.emplace_back(outputStreamFrame, midiData);
		}
		return true;
	}
	if(!m_outputHandle || IsBypassed() || m_SndFile.IsRenderingToDisc())
		return true;

	m_outputHandle->Send(midiData, outputStreamFrame);
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
void MidiInOut::OpenDevice(const MidiPortInfo &newDevice, bool asInputDevice)
{
	OpenDevice(MidiDeviceManager::FindPort(newDevice, asInputDevice), asInputDevice, false);
}


// Open a device for input or output.
void MidiInOut::OpenDevice(MidiPortID newDevice, bool asInputDevice, bool updateName)
{
	MidiPortInfo &device = asInputDevice ? m_inputDevice : m_outputDevice;
	const bool wasOpen = asInputDevice ? m_inputHandle.get() != nullptr : m_outputHandle.get() != nullptr;

	if(wasOpen && device.id == newDevice)
	{
		// No need to re-open this device.
		return;
	}

	if(asInputDevice)
		m_inputHandle.reset();
	else
		m_outputHandle.reset();
	device.id = newDevice;

	if(newDevice == kNoDevice || newDevice == kInternalDevice)
	{
		// Dummy or internal device
		if(updateName)
		{
			device.name = (newDevice == kInternalDevice) ? "<internal>" : "<none>";
			device.friendlyName.clear();
		}
		return;
	}

	bool ok = false;
	MidiDeviceManager &manager = MidiDeviceManager::Instance();
	if(asInputDevice)
	{
		m_inputHandle = manager.OpenInput(newDevice, this);
		ok = m_inputHandle != nullptr;
		if(ok && updateName)
			m_inputDevice = manager.GetPortInfo(*m_inputHandle);
	} else
	{
		m_outputHandle = manager.OpenOutput(newDevice);
		ok = m_outputHandle != nullptr;
		if (ok && updateName)
			m_outputDevice = manager.GetPortInfo(*m_outputHandle);
	}

	if(!ok)
	{
		if(updateName)
			device.name = "Unavailable";
		MidiInOutEditor *editor = dynamic_cast<MidiInOutEditor *>(GetEditor());
		if(editor != nullptr)
		{
			Reporting::Error("MIDI device cannot be opened. Is it open in another application?", "MIDI Input / Output", editor);
		}
	}
}


// Calculate the output stream frame position for the current render position
int64 MidiInOut::GetOutputStreamFrame(int32 frameOffset) const
{
	return m_SndFile.m_TimingInfo.StreamFrames
		+ m_SndFile.m_TimingInfo.FramesRenderedInChunk
		+ frameOffset
		+ mpt::saturate_round<int64>(m_latency * m_SndFile.GetSampleRate());
}


OPENMPT_NAMESPACE_END
