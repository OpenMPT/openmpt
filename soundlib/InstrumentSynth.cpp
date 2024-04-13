/*
 * InstrumentSynth.cpp
 * -------------------
 * Purpose: "Script" / "Synth" processor for various file formats (MED, GT2, Puma, His Master's Noise)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "InstrumentSynth.h"
#include "ModChannel.h"
#include "Sndfile.h"
#include "Tables.h"

OPENMPT_NAMESPACE_BEGIN

struct InstrumentSynth::States::State
{
	static constexpr uint16 STOP_ROW = uint16_max;

	uint16 m_currentRow = STOP_ROW;
	uint16 m_nextRow = 0;
	uint16 m_ticksRemain = 0;
	uint8 m_stepSpeed = 1;
	uint8 m_stepsRemain = 0;

	uint16 m_volumeFactor = 16384;
	int16 m_volumeAdd = int16_min;
	uint16 m_panning = 2048;
	int16 m_linearPitchFactor = 0;
	int16 m_periodAdd = 0;

	uint16 m_gtkKeyOffOffset = STOP_ROW;
	int16 m_gtkVolumeStep = 0;
	int16 m_gtkPitchStep = 0;
	int16 m_gtkPanningStep = 0;
	uint16 m_gtkPitch = 4096;
	uint8 m_gtkCount = 0;
	uint8 m_gtkSpeed = 1;
	uint8 m_gtkSpeedRemain = 1;
	bool m_gtkTremorEnabled = false;
	uint8 m_gtkTremorOnTime = 3;
	uint8 m_gtkTremorOffTime = 3;
	uint8 m_gtkTremorPos = 0;
	bool m_gtkTremoloEnabled = false;
	bool m_gtkVibratoEnabled = false;
	uint8 m_gtkVibratoWidth = 0;
	uint8 m_gtkVibratoSpeed = 0;
	uint8 m_gtkVibratoPos = 0;

	uint8 m_pumaStartWaveform = 0;
	uint8 m_pumaEndWaveform = 0;
	int8 m_pumaWaveformStep = 0;
	uint8 m_pumaWaveform = 0;

	uint8 m_medVibratoEnvelope = uint8_max;
	uint8 m_medVibratoSpeed = 0;
	uint8 m_medVibratoDepth = 0;
	uint16 m_medVibratoPos = 0;
	int16 m_medVolumeStep = 0;
	int16 m_medPeriodStep = 0;
	uint16 m_medArpOffset = STOP_ROW;
	uint8 m_medArpPos = 0;
	uint8 m_medHold = uint8_max;
	uint16 m_medDecay = STOP_ROW;
	uint8 m_medVolumeEnv = uint8_max;
	uint8 m_medVolumeEnvPos = 0;

	void JumpToPosition(const Events &events, uint16 position);
	void NextTick(const Events &events, ModChannel &chn, int32 &period, const CSoundFile &sndFile);
	bool EvaluateEvent(const Event &event, ModChannel &chn, const CSoundFile *sndFile);
	void EvaluateRunningEvent(const Event &event);
};


static int16 TranslateGT2Pitch(uint16 pitch)
{
	// 4096 = normal, 8192 = one octave up
	return mpt::saturate_round<int16>((mpt::log2(8192.0 / std::max(pitch, uint16(1))) - 1.0) * (16 * 12));
}


static int8 MEDEnvelopeFromSample(const ModInstrument &instr, const CSoundFile &sndFile, uint8 envelope, uint16 envelopePos)
{
	SAMPLEINDEX smp = instr.Keyboard[NOTE_MIDDLEC - NOTE_MIN] + envelope;
	if(smp < 1 || smp > sndFile.GetNumSamples())
		return 0;
	
	const auto &mptSmp = sndFile.GetSample(smp);
	if(envelopePos >= mptSmp.nLength || mptSmp.uFlags[CHN_16BIT] || !mptSmp.sample8())
		return 0;

	return mptSmp.sample8()[envelopePos];
}


static void ChannelSetSample(ModChannel &chn, const CSoundFile *sndFile, SAMPLEINDEX smp)
{
	if(!sndFile || smp < 1 || smp > sndFile->GetNumSamples())
		return;
	if(sndFile->m_playBehaviour[kMODSampleSwap] && smp <= uint8_max)
	{
		chn.nNewIns = static_cast<uint8>(smp);
		return;
	}
	const ModSample &sample = sndFile->GetSample(smp);
	if(chn.pModSample == &sample)
		return;
	chn.pModSample = &sample;
	chn.pCurrentSample = sample.samplev();
	chn.dwFlags = (chn.dwFlags & CHN_CHANNELFLAGS) | sample.uFlags;
	chn.nLength = sample.uFlags[CHN_LOOP] ? sample.nLoopEnd : 0;
	chn.nLoopStart = sample.nLoopStart;
	chn.nLoopEnd = sample.nLoopEnd;
	if(chn.position.GetUInt() >= chn.nLength)
		chn.position.Set(0);
}


// To allow State to be forward-declared
InstrumentSynth::States::States() = default;
InstrumentSynth::States::States(const States &other) = default;
InstrumentSynth::States::States(States &&other) noexcept = default;
InstrumentSynth::States::~States() = default;
InstrumentSynth::States &InstrumentSynth::States::operator=(const States &other) = default;
InstrumentSynth::States &InstrumentSynth::States::operator=(States &&other) noexcept = default;


void InstrumentSynth::States::Stop()
{
	for(auto &state : states)
		state.m_currentRow = state.m_nextRow = State::STOP_ROW;
}


void InstrumentSynth::States::NextTick(ModChannel &chn, int32 &period, const CSoundFile &sndFile)
{
	if(!chn.pModInstrument || !chn.pModInstrument->synth.HasScripts())
		return;

	const auto &scripts = chn.pModInstrument->synth.m_scripts;
	states.resize(scripts.size());
	for(size_t i = 0; i < scripts.size(); i++)
	{
		auto &state = states[i];
		if(chn.triggerNote)
			mpt::reconstruct(state);
		if(i == 1 && chn.rowCommand.command == CMD_MED_SYNTH_JUMP && chn.isFirstTick)
			state.JumpToPosition(scripts[i], chn.rowCommand.param);

		state.NextTick(scripts[i], chn, period, sndFile);
	}
}


void InstrumentSynth::States::State::JumpToPosition(const Events &events, uint16 position)
{
	for(size_t pos = 0; pos < events.size(); pos++)
	{
		if(events[pos].type == Event::Type::JumpMarker && events[pos].u16 >= position)
		{
			m_nextRow = static_cast<uint16>(pos);
			m_ticksRemain = 0;
			return;
		}
	}
}


void InstrumentSynth::States::State::NextTick(const Events &events, ModChannel &chn, int32 &period, const CSoundFile &sndFile)
{
	if(events.empty())
		return;

	if(m_gtkKeyOffOffset != STOP_ROW && chn.dwFlags[CHN_KEYOFF])
	{
		m_nextRow = m_gtkKeyOffOffset;
		m_ticksRemain = 0;
		m_gtkKeyOffOffset = STOP_ROW;
	}
	if(m_pumaWaveformStep)
	{
		m_pumaWaveform = static_cast<uint8>(Clamp(m_pumaWaveform + m_pumaWaveformStep, m_pumaStartWaveform, m_pumaEndWaveform));
		if(m_pumaWaveform <= m_pumaStartWaveform || m_pumaWaveform >= m_pumaEndWaveform)
			m_pumaWaveformStep = -m_pumaWaveformStep;
		ChannelSetSample(chn, &sndFile, m_pumaWaveform);
	}

	if(m_medHold != uint8_max)
	{
		if(!m_medHold--)
			m_nextRow = m_medDecay;
	}

	if(m_stepSpeed && !m_stepsRemain--)
	{
		// Yep, MED executes this before a potential SPD command may change the step speed on this very row...
		m_stepsRemain = m_stepSpeed - 1;

		if(m_medVolumeStep)
			m_volumeFactor = static_cast<int16>(std::clamp(m_volumeFactor + m_medVolumeStep, 0, 16384));
		if(m_medPeriodStep)
			m_periodAdd = mpt::saturate_cast<int16>(m_periodAdd - m_medPeriodStep);
		if(m_medVolumeEnv != uint8_max && chn.pModInstrument)
		{
			m_volumeFactor = static_cast<uint16>(std::clamp((MEDEnvelopeFromSample(*chn.pModInstrument, sndFile, m_medVolumeEnv & 0x7F, m_medVolumeEnvPos) + 128) * 64, 0, 16384));
			m_medVolumeEnvPos++;
			if(m_medVolumeEnvPos >= 128 && (m_medVolumeEnv & 0x80))
				m_medVolumeEnvPos = 0;
		}

		if(m_ticksRemain)
		{
			if(m_currentRow < events.size())
				EvaluateRunningEvent(events[m_currentRow]);
			m_ticksRemain--;
		} else
		{
			uint8 jumpCount = 0;
			while(!m_ticksRemain)
			{
				m_currentRow = m_nextRow;
				if(m_currentRow >= events.size())
					break;
				m_nextRow++;
				if(EvaluateEvent(events[m_currentRow], chn, &sndFile))
					break;

				MPT_ASSERT(m_nextRow == States::State::STOP_ROW || m_nextRow == m_currentRow + 1 || events[m_currentRow].IsJumpEvent());
				if(events[m_currentRow].IsJumpEvent())
				{
					// This smells like an infinite loop
					if(jumpCount++ > 10)
						break;
				}
			}
		}
	}

	// MED stuff
	if(m_medArpOffset < events.size())
	{
		m_linearPitchFactor = 16 * events[m_medArpOffset + m_medArpPos].u8;
		m_medArpPos = (m_medArpPos + 1) % static_cast<uint8>(events[m_medArpOffset].u16);
	}
	if(m_medVibratoDepth)
	{
		static_assert(std::size(ModSinusTable) == 64);
		uint16 offset = m_medVibratoPos / 16u;
		if(m_medVibratoEnvelope == uint8_max)
			period += ModSinusTable[(offset * 2) % std::size(ModSinusTable)] * m_medVibratoDepth / 64;
		else if(chn.pModInstrument)
			period += MEDEnvelopeFromSample(*chn.pModInstrument, sndFile, m_medVibratoEnvelope, offset) * m_medVibratoDepth / 64;
		m_medVibratoPos = (m_medVibratoPos + m_medVibratoSpeed) % (32u * 16u);
	}

	// GTK stuff
	if(m_currentRow < events.size() && m_gtkSpeed && !--m_gtkSpeedRemain)
	{
		m_gtkSpeedRemain = m_gtkSpeed;
		if(m_gtkVolumeStep)
			m_volumeFactor = static_cast<uint16>(std::clamp(m_volumeFactor + m_gtkVolumeStep, 0, 16384));
		if(m_gtkPanningStep)
			m_panning = static_cast<uint16>(std::clamp(m_panning + m_gtkPanningStep, 0, 4096));
		if(m_gtkPitchStep)
		{
			m_gtkPitch = static_cast<uint16>(std::clamp(m_gtkPitch + m_gtkPitchStep, 0, 32768));
			m_linearPitchFactor = TranslateGT2Pitch(m_gtkPitch);
		}
	}
	if(m_gtkTremorEnabled)
	{
		if(m_gtkTremorPos >= m_gtkTremorOnTime + m_gtkTremorOffTime)
			m_gtkTremorPos = 0;
		else if(m_gtkTremorPos >= m_gtkTremorOnTime)
			chn.nRealVolume = 0;
		m_gtkTremorPos++;
	}
	if(m_gtkTremoloEnabled)
	{
		m_volumeAdd = ModSinusTable[(m_gtkVibratoPos / 4u) % std::size(ModSinusTable)] * m_gtkVibratoWidth / 2;
		m_gtkVibratoPos = (m_gtkVibratoPos + m_gtkVibratoSpeed) % (32u * 16u);
	}
	if(m_gtkVibratoEnabled)
	{
		sndFile.DoFreqSlide(chn, period, -ModSinusTable[(m_gtkVibratoPos / 4u) % std::size(ModSinusTable)] * m_gtkVibratoWidth / 96, false);
		m_gtkVibratoPos = (m_gtkVibratoPos + m_gtkVibratoSpeed) % (32u * 16u);
	}

	const bool periodsAreFrequencies = sndFile.PeriodsAreFrequencies();
	if(m_volumeFactor != 16384)
	{
		chn.nRealVolume = Util::muldivr(chn.nRealVolume, m_volumeFactor, 16384);
	}
	if(m_volumeAdd != int16_min)
	{
		chn.nRealVolume = std::clamp(chn.nRealVolume + m_volumeAdd, int32(0), int32(16384));
	}
	if(m_panning != 2048)
	{
		if(chn.nRealPan >= 128)
			chn.nRealPan += ((m_panning - 2048) * (256 - chn.nRealPan)) / 2048;
		else
			chn.nRealPan += ((m_panning - 2048) * (chn.nRealPan)) / 2048;
	}
	if(m_linearPitchFactor != 0)
	{
		const auto &table = (periodsAreFrequencies ^ (m_linearPitchFactor < 0)) ? LinearSlideUpTable : LinearSlideDownTable;
		size_t value = std::abs(m_linearPitchFactor);
		while(value > 0)
		{
			const size_t amount = std::min(value, std::size(table) - size_t(1));
			period = Util::muldivr(period, table[amount], 65536);
			value -= amount;
		}
	}
	if(periodsAreFrequencies)
		period -= m_periodAdd;
	else
		period += m_periodAdd;
	if(period < 1)
		period = 1;
}


bool InstrumentSynth::States::State::EvaluateEvent(const Event &event, ModChannel &chn, const CSoundFile *sndFile)
{
	switch(event.type)
	{
	case Event::Type::StopScript:
		m_nextRow = STOP_ROW;
		return true;
	case Event::Type::Jump:
		m_nextRow = event.u16;
		return false;
	case Event::Type::Delay:
		m_ticksRemain = event.u16;
		return true;
	case Event::Type::SetStepSpeed:
		m_stepSpeed = event.u8;
		return false;
	case Event::Type::JumpMarker:
		return false;

	case Event::Type::GTK_SetLoopCounter:
		m_gtkCount = event.u8;
		return false;
	case Event::Type::GTK_EvaluateLoopCounter:
		if(m_gtkCount)
		{
			if(--m_gtkCount)
				m_nextRow = event.u16;
		}
		return false;
	case Event::Type::GTK_KeyOff:
		m_gtkKeyOffOffset = event.u16;
		return false;
	case Event::Type::GTK_SetVolume:
		m_volumeFactor = event.u16;
		return false;
	case Event::Type::GTK_SetPitch:
		m_gtkPitch = event.u16;
		m_linearPitchFactor = TranslateGT2Pitch(event.u16);
		m_periodAdd = 0;
		return false;
	case Event::Type::GTK_SetPanning:
		m_panning = event.u16;
		return false;
	case Event::Type::GTK_SetVolumeStep:
		m_gtkVolumeStep = event.i16;
		return false;
	case Event::Type::GTK_SetPitchStep:
		m_gtkPitchStep = event.i16;
		return false;
	case Event::Type::GTK_SetPanningStep:
		m_gtkPanningStep = event.i16;
		return false;
	case Event::Type::GTK_SetSpeed:
		m_gtkSpeed = m_gtkSpeedRemain = event.u8;
		return false;
	case Event::Type::GTK_EnableTremor:
		m_gtkTremorEnabled = event.u8 != 0;
		return false;
	case Event::Type::GTK_SetTremorTime:
		if(event.Byte0())
			m_gtkTremorOnTime = event.Byte0();
		if(event.Byte1())
			m_gtkTremorOffTime = event.Byte1();
		m_gtkTremorPos = 0;
		return false;
	case Event::Type::GTK_EnableTremolo:
		m_gtkTremoloEnabled = event.u8 != 0;
		m_gtkVibratoPos = 0;
		if(!m_gtkVibratoWidth)
			m_gtkVibratoWidth = 8;
		if(!m_gtkVibratoSpeed)
			m_gtkVibratoSpeed = 16;
		return false;
	case Event::Type::GTK_EnableVibrato:
		m_gtkVibratoEnabled = event.u8 != 0;
		m_gtkVibratoPos = 0;
		if(!m_gtkVibratoWidth)
			m_gtkVibratoWidth = 3;
		if(!m_gtkVibratoSpeed)
			m_gtkVibratoSpeed = 8;
		return false;
	case Event::Type::GTK_SetVibratoParams:
		if(event.Byte0())
			m_gtkVibratoWidth = event.Byte0();
		if(event.Byte1())
			m_gtkVibratoSpeed = event.Byte1();
		return false;

	case Event::Type::Puma_SetWaveform:
		m_pumaWaveform = m_pumaStartWaveform = event.Byte0() + 1;
		if(event.Byte0() < 10)
		{
			m_pumaWaveformStep = 0;
		} else
		{
			m_pumaWaveformStep = event.Byte1();
			m_pumaEndWaveform = event.Byte2() + m_pumaStartWaveform;
		}
		ChannelSetSample(chn, sndFile, m_pumaWaveform);
		return false;
	case Event::Type::Puma_VolumeRamp:
		m_ticksRemain = event.Byte2();
		m_volumeAdd = event.Byte0() * 256 - 16384;
		return true;
	case Event::Type::Puma_StopVoice:
		chn.nRealVolume = 0;
		chn.Stop();
		m_nextRow = STOP_ROW;
		return true;
	case Event::Type::Puma_SetPitch:
		m_linearPitchFactor = static_cast<int8>(event.Byte0()) * 8;
		m_periodAdd = 0;
		m_ticksRemain = event.Byte2();
		return true;
	case Event::Type::Puma_PitchRamp:
		m_linearPitchFactor = 0;
		m_periodAdd = static_cast<int8>(event.Byte0()) * 4;
		m_ticksRemain = event.Byte2();
		return true;

	case Event::Type::Mupp_SetWaveform:
		ChannelSetSample(chn, sndFile, 32 + event.Byte0() * 28 + event.Byte1());
		m_volumeFactor = static_cast<uint16>(std::min(event.Byte2() & 0x7F, 64) * 256u);
		return true;

	case Event::Type::MED_DefineArpeggio:
		if(!event.Byte1())
			return false;
		m_nextRow = m_currentRow + event.u16;
		m_medArpOffset = m_currentRow;
		m_medArpPos = 0;
		return true;
	case Event::Type::MED_JumpScript:
		if(event.u8 < chn.synthState.states.size())
		{
			chn.synthState.states[event.u8].m_nextRow = event.u16;
			chn.synthState.states[event.u8].m_ticksRemain = 0;
		}
		return false;
	case Event::Type::MED_SetEnvelope:
		if(event.Byte2())
			m_medVolumeEnv = (event.Byte0() & 0x3F) | (event.Byte1() ? 0x80 : 0x00);
		else
			m_medVibratoEnvelope = event.Byte0();
		m_medVolumeEnvPos = 0;
		return false;
	case Event::Type::MED_SetVolume:
		m_volumeFactor = event.u8 * 256u;
		return true;
	case Event::Type::MED_SetWaveform:
		if(chn.pModInstrument)
			ChannelSetSample(chn, sndFile, chn.pModInstrument->Keyboard[NOTE_MIDDLEC - NOTE_MIN] + event.u8);
		return true;
	case Event::Type::MED_SetVibratoSpeed:
		m_medVibratoSpeed = event.u8;
		return false;
	case Event::Type::MED_SetVibratoDepth:
		m_medVibratoDepth = event.u8;
		return false;
	case Event::Type::MED_SetVolumeStep:
		m_medVolumeStep = static_cast<int16>(event.i16 * 256);
		return false;
	case Event::Type::MED_SetPeriodStep:
		m_medPeriodStep = static_cast<int16>(event.i16 * 4);
		return false;
	case Event::Type::MED_HoldDecay:
		m_medHold = event.u8;
		m_medDecay = event.u16;
		return false;
	}
	MPT_ASSERT_NOTREACHED();
	return false;
}


void InstrumentSynth::States::State::EvaluateRunningEvent(const Event &event)
{
	switch(event.type)
	{
	case Event::Type::Puma_VolumeRamp:
		if(event.Byte2() > 0)
			m_volumeAdd = static_cast<int16>((event.Byte1() + Util::muldivr(event.Byte0() - event.Byte1(), m_ticksRemain, event.Byte2())) * 256 - 16384);
		break;
	case Event::Type::Puma_PitchRamp:
		if(event.Byte2() > 0)
			m_periodAdd = static_cast<int16>((static_cast<int8>(event.Byte1()) + Util::muldivr(static_cast<int8>(event.Byte0()) - static_cast<int8>(event.Byte1()), m_ticksRemain, event.Byte2())) * 4);
		break;
	default:
		break;
	}
}


void InstrumentSynth::Sanitize()
{
	for(auto &script : m_scripts)
	{
		if(script.size() >= States::State::STOP_ROW)
			script.resize(States::State::STOP_ROW - 1);
	}
}


OPENMPT_NAMESPACE_END
