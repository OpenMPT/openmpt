/*
 * InstrumentSynth.h
 * -----------------
 * Purpose: "Script" / "Synth" processor for various file formats (MED, GT2, Puma, His Master's Noise)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include <vector>

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;
struct ModChannel;

struct InstrumentSynth
{
	struct Event
	{
		enum class Type : uint8
		{
			StopScript,    // No parameter
			Jump,          // Parameter: Event index (uint16)
			Delay,         // Parameter: Number of ticks (uint16)
			SetStepSpeed,  // Parameter: Speed (uint8)
			JumpMarker,    // Parameter: Marker ID (uint16)

			GTK_SetLoopCounter,       // Parameter: Count (uint8)
			GTK_EvaluateLoopCounter,  // Parameter: Jump target once counter reaches 0 (uint16)
			GTK_KeyOff,               // Parameter: Jump target once key is released (uint16)
			GTK_SetVolume,            // Parameter: Volume (uint16)
			GTK_SetPitch,             // Parameter: Pitch (uint16)
			GTK_SetPanning,           // Parameter: Panning (uint16)
			GTK_SetVolumeStep,        // Parameter: Step size (int16)
			GTK_SetPitchStep,         // Parameter: Step size (int16)
			GTK_SetPanningStep,       // Parameter: Step size (int16)
			GTK_SetSpeed,             // Parameter: Speed (uint8)
			GTK_EnableTremor,         // Parameter: Enable (uint8)
			GTK_SetTremorTime,        // Parameter: On time (uint8), off time (uint8)
			GTK_EnableTremolo,        // Parameter: Enable (uint8)
			GTK_EnableVibrato,        // Parameter: Enable (uint8)
			GTK_SetVibratoParams,     // Parameter: Width (uint8), speed (uint8)

			Puma_SetWaveform,  // Parameter: Waveform (uint8), wavestorm step (uint8), number of waveforms to cycle (uint8)
			Puma_VolumeRamp,   // Parameter: Start volume (uint8), end volume (uint8), number of ticks (uint8)
			Puma_StopVoice,    // No parameter
			Puma_SetPitch,     // Parameter: Pitch offset (int8), number of ticks (uint8)
			Puma_PitchRamp,    // Parameter: Start pitch offset (int8), end pitch offset (int8), number of ticks (uint8)

			Mupp_SetWaveform,  // Parameter: Source instrument (uint8), waveform (uint8), volume (uint8)

			MED_DefineArpeggio,   // Parameter: Arpeggio note (uint8), arp length or 0 if it's not the first note (uint16)
			MED_JumpScript,       // Parameter: Script index (uint8), jump target (uint16)
			MED_SetEnvelope,      // Parameter: Envelope index (uint8), loop on/off (uint8), is volume envelope (uint8)
			MED_SetVolume,        // Parameter: Volume (uint8)
			MED_SetWaveform,      // Parameter: Waveform (uint8)
			MED_SetVibratoSpeed,  // Parameter: Speed (uint8)
			MED_SetVibratoDepth,  // Parameter: Depth (uint8)
			MED_SetVolumeStep,    // Parameter: Volume step (int16)
			MED_SetPeriodStep,    // Parameter: Period step (int16)
			MED_HoldDecay,        // Parameter: Hold time (uint8), decay point (uint16)
		};

		Type type = Type::StopScript;
		union
		{
			uint8 u8 = 0;
			int8 i8;
		};
		union
		{
			uint16 u16 = 0;
			int16 i16;
			std::array<uint8, 2> bytes;
		};

		static constexpr Event StopScript() noexcept { return Event{Type::StopScript}; }
		static constexpr Event Jump(uint16 target) noexcept { return Event{Type::Jump, target}; }
		static constexpr Event Delay(uint16 ticks) noexcept { return Event{Type::Delay, ticks}; }
		static constexpr Event SetStepSpeed(uint8 speed) noexcept { return Event{Type::SetStepSpeed, speed}; }
		static constexpr Event JumpMarker(uint16 data) noexcept { return Event{Type::JumpMarker, data}; }

		static constexpr Event GTK_SetLoopCounter(uint8 count) noexcept { return Event{Type::GTK_SetLoopCounter, count}; }
		static constexpr Event GTK_EvaluateLoopCounter(uint16 target) noexcept { return Event{Type::GTK_EvaluateLoopCounter, target}; }
		static constexpr Event GTK_KeyOff(uint16 target) noexcept { return Event{Type::GTK_KeyOff, target}; }
		static constexpr Event GTK_SetVolume(uint16 volume) noexcept { return Event{Type::GTK_SetVolume, volume}; }
		static constexpr Event GTK_SetPitch(uint16 pitch) noexcept { return Event{Type::GTK_SetPitch, pitch}; }
		static constexpr Event GTK_SetPanning(uint16 panning) noexcept { return Event{Type::GTK_SetPanning, panning}; }
		static constexpr Event GTK_SetVolumeStep(int16 stepSize) noexcept { return Event{Type::GTK_SetVolumeStep, stepSize}; }
		static constexpr Event GTK_SetPitchStep(int16 stepSize) noexcept { return Event{Type::GTK_SetPitchStep, stepSize}; }
		static constexpr Event GTK_SetPanningStep(int16 stepSize) noexcept { return Event{Type::GTK_SetPanningStep, stepSize}; }
		static constexpr Event GTK_SetSpeed(uint8 speed) noexcept { return Event{Type::GTK_SetSpeed, speed}; }
		static constexpr Event GTK_EnableTremor(uint8 enable) noexcept { return Event{Type::GTK_EnableTremor, enable}; }
		static constexpr Event GTK_SetTremorTime(uint8 onTime, uint8 offTime) noexcept { return Event{Type::GTK_SetTremorTime, onTime, offTime, uint8(0)}; }
		static constexpr Event GTK_EnableTremolo(uint8 enable) noexcept { return Event{Type::GTK_EnableTremolo, enable}; }
		static constexpr Event GTK_EnableVibrato(uint8 enable) noexcept { return Event{Type::GTK_EnableVibrato, enable}; }
		static constexpr Event GTK_SetVibratoParams(uint8 width, uint8 speed) noexcept { return Event{Type::GTK_SetVibratoParams, width, speed, uint8(0)}; }

		static constexpr Event Puma_SetWaveform(uint8 waveform, uint8 step, uint8 count) noexcept { return Event{Type::Puma_SetWaveform, waveform, step, count}; }
		static constexpr Event Puma_VolumeRamp(uint8 startVol, uint8 endVol, uint8 ticks) noexcept { return Event{Type::Puma_VolumeRamp, startVol, endVol, ticks}; }
		static constexpr Event Puma_StopVoice() noexcept { return Event{Type::Puma_StopVoice}; }
		static constexpr Event Puma_SetPitch(uint8 pitchOffset, uint8 ticks) noexcept { return Event{Type::Puma_SetPitch, pitchOffset, uint8(0), ticks}; }
		static constexpr Event Puma_PitchRamp(uint8 startPitch, uint8 endPitch, uint8 ticks) noexcept { return Event{Type::Puma_PitchRamp, startPitch, endPitch, ticks}; }

		static constexpr Event Mupp_SetWaveform(uint8 instr, uint8 waveform, uint8 volume) noexcept { return Event{Type::Mupp_SetWaveform, instr, waveform, volume}; }

		static constexpr Event MED_DefineArpeggio(uint8 note, uint16 noteCount) noexcept { return Event{Type::MED_DefineArpeggio, noteCount, note}; }
		static constexpr Event MED_JumpScript(uint8 scriptIndex, uint16 target) noexcept { return Event{Type::MED_JumpScript, target, scriptIndex}; }
		static constexpr Event MED_SetEnvelope(uint8 envelope, bool loop, bool volumeEnv) noexcept { return Event{Type::MED_SetEnvelope, envelope, uint8(loop ? 1 : 0), uint8(volumeEnv ? 1 : 0)}; }
		static constexpr Event MED_SetVolume(uint8 volume) noexcept { return Event{Type::MED_SetVolume, volume}; }
		static constexpr Event MED_SetWaveform(uint8 waveform) noexcept { return Event{Type::MED_SetWaveform, waveform}; }
		static constexpr Event MED_SetVibratoSpeed(uint8 depth) noexcept { return Event{Type::MED_SetVibratoSpeed, depth}; }
		static constexpr Event MED_SetVibratoDepth(uint8 depth) noexcept { return Event{Type::MED_SetVibratoDepth, depth}; }
		static constexpr Event MED_SetVolumeStep(int16 volumeStep) noexcept { return Event{Type::MED_SetVolumeStep, volumeStep}; }
		static constexpr Event MED_SetPeriodStep(int16 periodStep) noexcept { return Event{Type::MED_SetPeriodStep, periodStep}; }
		static constexpr Event MED_HoldDecay(uint8 hold, uint16 decay) noexcept { return Event{Type::MED_HoldDecay, decay, hold}; }

		constexpr Event() noexcept = default;
		constexpr Event(const Event &other) noexcept = default;
		constexpr Event(Event &&other) noexcept = default;
		constexpr Event &operator=(const Event &other) noexcept = default;
		constexpr Event &operator=(Event &&other) noexcept = default;

		constexpr bool IsJumpEvent() const noexcept
		{
			return type == Type::Jump || type == Type::GTK_KeyOff || type == Type::GTK_EvaluateLoopCounter || type == Type::MED_JumpScript || type == Type::MED_HoldDecay;
		}

		template <typename TMap>
		void FixupJumpTarget(TMap &offsetToIndexMap)
		{
			if(!IsJumpEvent())
				return;
			if(auto it = offsetToIndexMap.lower_bound(u16); it != offsetToIndexMap.end())
				u16 = it->second;
			else
				u16 = uint16_max;
		}

		constexpr uint8 Byte0() const noexcept { return u8; }
		constexpr uint8 Byte1() const noexcept { return bytes[0]; }
		constexpr uint8 Byte2() const noexcept { return bytes[1]; }

	protected:
		constexpr Event(Type type, uint8 b1, uint8 b2, uint8 b3) noexcept : type{type}, u8{b1}, bytes{b2, b3} {}
		constexpr Event(Type type, uint16 u16, uint8 u8 = 0) noexcept : type{type}, u8{u8}, u16{u16} {}
		constexpr Event(Type type, int16 i16) noexcept : type{type}, i16{i16} {}
		constexpr Event(Type type, uint8 u8) noexcept : type{type}, u8{u8} {}
		constexpr Event(Type type, int8 i8) noexcept : type{type}, i8{i8} {}
		explicit constexpr Event(Type type) noexcept : type{type}, u8{}, u16{} {}
	};

	using Events = std::vector<Event>;

	struct States
	{
		struct State;

		States();
		States(const States &other);
		States(States &&other) noexcept;
		~States();
		States& operator=(const States &other);
		States& operator=(States &&other) noexcept;

		void Stop();
		void NextTick(ModChannel &chn, int32 &period, const CSoundFile &sndFile);

	protected:
		std::vector<State> states;
	};

	std::vector<Events> m_scripts;

	bool HasScripts() const noexcept { return !m_scripts.empty(); }
	void Clear() { m_scripts.clear(); }
	void Sanitize();
};

OPENMPT_NAMESPACE_END
