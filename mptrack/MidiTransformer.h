/*
 * MidiTransformer.h
 * -----------------
 * Purpose: Chainable MIDI input filters / transformers.
 * Notes  : Filters can be chained to transform MIDI data before it reaches the final consumer.
 *          Each filter receives MIDI messages, optionally transforms them, and forwards
 *          the result to the next callback in the chain.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "MidiInputCallback.h"

#include <array>
#include <bitset>
#include <vector>


OPENMPT_NAMESPACE_BEGIN


enum class MidiTransformers : uint8
{
	None      = 0x00,
	Volume    = 0x01,
	Sustain   = 0x02,
	Transpose = 0x04,
	
	All = Volume | Sustain | Transpose,
	Default = Sustain | Transpose
};
DECLARE_FLAGSET(MidiTransformers);


// Base class for all MIDI transformers
class MidiTransformer : public IMidiInputCallback
{
public:
	~MidiTransformer() override = default;

	void SetNextCallback(IMidiInputCallback *next) { m_next = next; }

	void SetBypassed(bool bypassed) { m_bypassed = bypassed; }
	bool IsBypassed() const { return m_bypassed; }

	// Reset internal state (but not user settings!)
	virtual void Reset() {}

protected:
	// Forward a message to the next filter in the chain
	void Forward(mpt::const_byte_span midiData)
	{
		if(m_next && !midiData.empty())
			m_next->OnMidiMessage(midiData);
	}

private:
	IMidiInputCallback *m_next = nullptr;
	bool m_bypassed = false;
};


// Transpose MIDI notes according to the configured transposition amount
class MidiTransposeFilter final : public MidiTransformer
{
public:
	void OnMidiMessage(mpt::const_byte_span midiData) override;
	void Reset() override { m_transposeMap.clear(); }

	void SetTranspose(int16 transpose) { m_transpose = transpose; }

private:
	std::map<std::pair<uint8, uint8>, std::vector<int16>> m_transposeMap;
	int16 m_transpose = 0;
};


// Buffer note-off messages while the sustain pedal (CC 64) is held,
// and release them when the pedal is released
class MidiSustainFilter final : public MidiTransformer
{
public:
	void OnMidiMessage(mpt::const_byte_span midiData) override;
	void Reset() override;

private:
	std::array<std::vector<std::array<std::byte, 3>>, 16> m_sustainBuffer;
	std::bitset<16> m_sustainActive;
};


// Scale note-on velocity according to MIDI configuration
// (velocity toggle, velocity amplification, channel volume).
// When bypassed, the filter configuration can still be used by directly calling ApplyVolumeSettings()
class MidiVolumeFilter final : public MidiTransformer
{
public:
	MidiVolumeFilter() { Reset(); }

	void OnMidiMessage(mpt::const_byte_span midiData) override;
	void Reset() override { m_channelVolume.fill(127); }

	void SetRecordVelocity(bool recordVelocity) { m_recordVelocity = recordVelocity; }
	void SetApplyChannelVolumeToVelocity(bool applyChannelVolumeToVelocity) { m_applyChannelVolumeToVelocity = applyChannelVolumeToVelocity;}
	void SetVelocityAmplification(uint16 velocityAmplification) { m_velocityAmplification  = velocityAmplification; }

	// Scale and apply velocity, as well as channel volume according to configuration.
	// Return volume in range [0, 256], or -1 for "use default".
	int ApplyVolumeSettings(mpt::const_byte_span midiData) const;

private:
	std::array<uint8, 16> m_channelVolume;
	uint16 m_velocityAmplification = 100;
	bool m_recordVelocity = false;
	bool m_applyChannelVolumeToVelocity = false;
};


OPENMPT_NAMESPACE_END
