/*
 * MidiTransformer.cpp
 * -------------------
 * Purpose: Chainable MIDI input filters / transformers.
 * Notes  : Filters can be chained to transform MIDI data before it reaches the final consumer.
 *          Each filter receives MIDI messages, optionally transforms them, and forwards
 *          the result to the next callback in the chain.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "MidiTransformer.h"
#include "Dlsbank.h"
#include "../soundlib/MIDIEvents.h"


OPENMPT_NAMESPACE_BEGIN


void MidiTransposeFilter::OnMidiMessage(mpt::const_byte_span midiData)
{
	if(midiData.size() != 3)
	{
		Forward(midiData);
		return;
	}

	MIDIEvents::EventType event = MIDIEvents::GetTypeFromEvent(midiData);
	if((event == MIDIEvents::evNoteOn || event == MIDIEvents::evNoteOff) && MIDIEvents::GetChannelFromEvent(midiData) != 9)
	{
		int16 transpose = m_transpose;
		const uint8 midiNote = MIDIEvents::GetDataByte1FromEvent(midiData);
		const auto eventKey = std::make_pair(MIDIEvents::GetChannelFromEvent(midiData), midiNote);
		if(MIDIEvents::IsNoteOnEvent(midiData))
		{
			// Keep track of each note-on's transposition setting, as one can hit the same key with different transpositions while holding the sustain pedal
			m_transposeMap[eventKey].push_back(m_transpose);
		} else if(auto it = m_transposeMap.find(eventKey); it != m_transposeMap.end() && !it->second.empty())
		{
			// Use transpose of note-on event in case we switched the transposition setting while the note was playing
			transpose = it->second.front();
			it->second.erase(it->second.begin());
		}

		if(transpose != 0 && !IsBypassed())
		{
			int note = transpose + midiNote;
			Limit(note, 0, 127);
			std::array<std::byte, 3> noteMsg;
			std::copy(midiData.begin(), midiData.end(), noteMsg.begin());
			noteMsg[1] = mpt::byte_cast<std::byte>(static_cast<uint8>(note));
			Forward(noteMsg);
			return;
		}
	}

	Forward(midiData);
}


void MidiSustainFilter::OnMidiMessage(mpt::const_byte_span midiData)
{
	if(midiData.size() != 3)
	{
		Forward(midiData);
		return;
	}

	MIDIEvents::EventType event = MIDIEvents::GetTypeFromEvent(midiData);
	const uint8 channel = MIDIEvents::GetChannelFromEvent(midiData);

	if(MIDIEvents::IsNoteOffEvent(midiData))
	{
		if(m_sustainActive[channel])
		{
			// Buffer the note-off for later release
			std::array<std::byte, 3> noteOff;
			std::copy(midiData.begin(), midiData.end(), noteOff.begin());
			m_sustainBuffer[channel].push_back(noteOff);
			if(!IsBypassed())
				return;
		}
	} else if(event == MIDIEvents::evControllerChange)
	{
		const uint8 cc = MIDIEvents::GetDataByte1FromEvent(midiData);
		const uint8 value = MIDIEvents::GetDataByte2FromEvent(midiData);
		if(cc == MIDIEvents::MIDICC_HoldPedal_OnOff)
		{
			m_sustainActive[channel] = (value >= 0x40);
			if(!m_sustainActive[channel])
			{
				if(!IsBypassed())
				{
					// Release all buffered note-offs
					for(const auto &offEvent : m_sustainBuffer[channel])
					{
						const auto span = mpt::as_span(offEvent);
						Forward(span);
					}
				}
				m_sustainBuffer[channel].clear();
			}
			// Forward the CC itself as well
			Forward(midiData);
			return;
		}
	}

	Forward(midiData);
}


void MidiSustainFilter::Reset()
{
	for(uint8 c = 0; c < 16; c++)
	{
		if(m_sustainActive[c])
			OnMidiMessage(MIDIEvents::CC(MIDIEvents::MIDICC_HoldPedal_OnOff, c, 0));
	}
}


void MidiVolumeFilter::OnMidiMessage(mpt::const_byte_span midiData)
{
	if(midiData.size() != 3)
	{
		Forward(midiData);
		return;
	}

	const MIDIEvents::EventType event = MIDIEvents::GetTypeFromEvent(midiData);
	const uint8 channel = MIDIEvents::GetChannelFromEvent(midiData);

	if(event == MIDIEvents::evControllerChange)
	{
		const uint8 cc = MIDIEvents::GetDataByte1FromEvent(midiData);
		const uint8 value = MIDIEvents::GetDataByte2FromEvent(midiData);
		if(cc == MIDIEvents::MIDICC_Volume_Coarse)
			m_channelVolume[channel] = value;
	}

	if(!IsBypassed() && MIDIEvents::IsNoteOnEvent(midiData))
	{
		int scaledVol = ApplyVolumeSettings(midiData);
		if(scaledVol >= 0)
		{
			// Clamp to MIDI velocity range and rebuild the message
			uint8 newVelocity = static_cast<uint8>(std::min((scaledVol + 1) / 2, 127));
			std::array<std::byte, 3> noteOn;
			std::copy(midiData.begin(), midiData.end(), noteOn.begin());
			noteOn[2] = mpt::byte_cast<std::byte>(newVelocity);
			Forward(noteOn);
			return;
		}
	}

	Forward(midiData);
}


int MidiVolumeFilter::ApplyVolumeSettings(mpt::const_byte_span midiData) const
{
	if(midiData.size() != 3)
		return -1;

	const MIDIEvents::EventType event = MIDIEvents::GetTypeFromEvent(midiData);
	const uint8 channel = MIDIEvents::GetChannelFromEvent(midiData);
	int vol = MIDIEvents::GetDataByte2FromEvent(midiData);
	if(event != MIDIEvents::evNoteOn && event != MIDIEvents::evNoteOff)
		return -1;

	uint8 midiChannelVolume = m_channelVolume[channel];
	if(m_recordVelocity)
	{
		if(!m_applyChannelVolumeToVelocity)
			midiChannelVolume = 127;

		vol = Util::muldivr_unsigned(CDLSBank::DLSMidiVolumeToLinear(vol), m_velocityAmplification * midiChannelVolume, 100 * 127 * 256);
		Limit(vol, 1, 256);
	} else
	{
		if(m_applyChannelVolumeToVelocity)
			vol = midiChannelVolume * 2 + 2;
		else
			vol = -1;  // Use default value
	}

	return vol;
}


OPENMPT_NAMESPACE_END
