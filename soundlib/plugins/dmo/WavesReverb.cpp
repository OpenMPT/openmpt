/*
 * WavesReverb.cpp
 * ---------------
 * Purpose: Implementation of the DMO WavesReverb DSP (for non-Windows platforms)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#if !defined(NO_PLUGINS) && defined(NO_DMO)
#include "../../Sndfile.h"
#include "WavesReverb.h"

OPENMPT_NAMESPACE_BEGIN

namespace DMO
{

IMixPlugin* WavesReverb::Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
//--------------------------------------------------------------------------------------------------
{
	return new (std::nothrow) WavesReverb(factory, sndFile, mixStruct);
}


WavesReverb::WavesReverb(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
	: IMixPlugin(factory, sndFile, mixStruct)
//-------------------------------------------------------------------------------------------
{
	m_param[kRvbInGain] = 1.0f;
	m_param[kRvbReverbMix] = 1.0f;
	m_param[kRvbReverbTime] = 1.0f / 3.0f;
	m_param[kRvbHighFreqRTRatio] = 0.0f;

	m_mixBuffer.Initialize(2, 2);
	InsertIntoFactoryList();
}


void WavesReverb::Process(float *pOutL, float *pOutR, uint32 numFrames)
//---------------------------------------------------------------------
{
	if(!m_mixBuffer.Ok())
		return;

	const float *in[2] = { m_mixBuffer.GetInputBuffer(0), m_mixBuffer.GetInputBuffer(1) };
	float *out[2] = { m_mixBuffer.GetOutputBuffer(0), m_mixBuffer.GetOutputBuffer(1) };

	uint32 delay0 = (m_delay[0] + m_state.combPos + 4) & 0x3FFF;
	uint32 delay1 = (m_delay[1] + m_state.combPos + 4) & 0x3FFF;
	uint32 delay2 = (m_delay[2] + m_state.combPos + 4) & 0x3FFF;
	uint32 delay3 = (m_delay[3] + m_state.combPos + 4) & 0x3FFF;
	float delay0old = m_state.comb[delay0    ];
	float delay1old = m_state.comb[delay1 + 1];
	float delay2old = m_state.comb[delay2 + 2];
	float delay3old = m_state.comb[delay3 + 3];

	for(uint32 i = numFrames; i != 0; i--)
	{
		const float leftIn  = *(in[0])++ + 1e-30f;	// Prevent denormals
		const float rightIn = *(in[1])++ + 1e-30f;	// Prevent denormals

		// Advance buffer index for the four comb filters
		delay0 = (delay0 - 4) & 0x3FFF;
		delay1 = (delay1 - 4) & 0x3FFF;
		delay2 = (delay2 - 4) & 0x3FFF;
		delay3 = (delay3 - 4) & 0x3FFF;
		float &delay0new = m_state.comb[delay0    ];
		float &delay1new = m_state.comb[delay1 + 1];
		float &delay2new = m_state.comb[delay2 + 2];
		float &delay3new = m_state.comb[delay3 + 3];

		uint32 pos;
		float r1, r2;
		
		pos = (m_state.allpassPos + m_delay[4]) & 0x7FF;
		r1 = delay1new * 0.61803401f + m_state.allpass1[pos] * m_coeffs[0];
		r2 = m_state.allpass1[pos + 1] * m_coeffs[0] - delay0new * 0.61803401f;
		m_state.allpass1[m_state.allpassPos    ] = r2 * 0.61803401f + delay0new;
		m_state.allpass1[m_state.allpassPos + 1] = delay1new - r1 * 0.61803401f;
		delay0new = r1;
		delay1new = r2;

		pos = (m_state.allpassPos + m_delay[5]) & 0x7FF;
		r1 = delay3new * 0.61803401f + m_state.allpass2[pos] * m_coeffs[1];
		r2 = m_state.allpass2[pos + 1] * m_coeffs[1] - delay2new * 0.61803401f;
		m_state.allpass2[m_state.allpassPos    ] = r2 * 0.61803401f + delay2new;
		m_state.allpass2[m_state.allpassPos + 1] = delay3new - r1 * 0.61803401f;
		delay2new = r1;
		delay3new = r2;

		*(out[0])++ = (leftIn  * m_dryFactor) + delay0new + delay2new;
		*(out[1])++ = (rightIn * m_dryFactor) + delay1new + delay3new;

		const float leftWet  = leftIn  * m_wetFactor;
		const float rightWet = rightIn * m_wetFactor;
		m_state.comb[m_state.combPos    ] = (delay0new * m_coeffs[2]) + (delay0old * m_coeffs[3]) + leftWet;
		m_state.comb[m_state.combPos + 1] = (delay1new * m_coeffs[4]) + (delay1old * m_coeffs[5]) + rightWet;
		m_state.comb[m_state.combPos + 2] = (delay2new * m_coeffs[6]) + (delay2old * m_coeffs[7]) - rightWet;
		m_state.comb[m_state.combPos + 3] = (delay3new * m_coeffs[8]) + (delay3old * m_coeffs[9]) + leftWet;

		delay0old = delay0new;
		delay1old = delay1new;
		delay2old = delay2new;
		delay3old = delay3new;

		// Advance buffer index
		m_state.combPos = (m_state.combPos - 4) & 0x3FFF;
		m_state.allpassPos = (m_state.allpassPos - 2) & 0x7FF;
	}

	ProcessMixOps(pOutL, pOutR, m_mixBuffer.GetOutputBuffer(0), m_mixBuffer.GetOutputBuffer(1), numFrames);
}


PlugParamValue WavesReverb::GetParameter(PlugParamIndex index)
//------------------------------------------------------------
{
	if(index < kDistNumParameters)
	{
		return m_param[index];
	}
	return 0;
}


void WavesReverb::SetParameter(PlugParamIndex index, PlugParamValue value)
//------------------------------------------------------------------------
{
	if(index < kDistNumParameters)
	{
		Limit(value, 0.0f, 1.0f);
		m_param[index] = value;
		RecalculateWavesReverbParams();
	}
}


void WavesReverb::Resume()
//------------------------
{
	// Recalculate delays
	uint32 delay0 = Util::Round<uint32>(m_SndFile.GetSampleRate() * 0.045);
	uint32 delay1 = Util::Round<uint32>(delay0 * 1.189207077026367);
	uint32 delay2 = Util::Round<uint32>(delay1 * 1.189207077026367);
	uint32 delay3 = Util::Round<uint32>(delay2 * 1.189207077026367);
	uint32 delay4 = Util::Round<uint32>((delay0 + delay2) * 0.1154666692018509);
	uint32 delay5 = Util::Round<uint32>((delay1 + delay3) * 0.1154666692018509);
	// Comb delays
	m_delay[0] = (delay0 - delay4) * 4;
	m_delay[1] = (delay2 - delay4) * 4;
	m_delay[2] = (delay1 - delay5) * 4;
	m_delay[3] = (delay3 - delay5) * 4;
	// Allpass delays
	m_delay[4] = delay4 * 2;
	m_delay[5] = delay5 * 2;

	RecalculateWavesReverbParams();
	MemsetZero(m_state);
}


#ifdef MODPLUG_TRACKER

CString WavesReverb::GetParamName(PlugParamIndex param)
//-----------------------------------------------------
{
	switch(param)
	{
	case kRvbInGain: return _T("InGain");
	case kRvbReverbMix: return _T("ReverbMix");
	case kRvbReverbTime: return _T("ReverbTime");
	case kRvbHighFreqRTRatio: return _T("HighFreqRTRatio");
	}
	return CString();
}


CString WavesReverb::GetParamLabel(PlugParamIndex param)
//------------------------------------------------------
{
	switch(param)
	{
	case kRvbInGain:
	case kRvbReverbMix:
		return _T("dB");
	case kRvbReverbTime:
		return _T("ms");
	}
	return CString();
}


CString WavesReverb::GetParamDisplay(PlugParamIndex param)
//--------------------------------------------------------
{
	float value = m_param[param];
	switch(param)
	{
	case kRvbInGain:
	case kRvbReverbMix:
		value = GainInDecibel(value);
		break;
	case kRvbReverbTime:
		value = ReverbTime();
		break;
	case kRvbHighFreqRTRatio:
		value = HighFreqRTRatio();
		break;
	}
	CString s;
	s.Format("%.2f", value);
	return s;
}

#endif // MODPLUG_TRACKER


void WavesReverb::RecalculateWavesReverbParams()
//----------------------------------------------
{
	// Recalculate filters
	const double ReverbTimeSmp = -3000.0 / (m_SndFile.GetSampleRate() * ReverbTime());
	const double ReverbTimeSmpHF = ReverbTimeSmp * (1.0 / HighFreqRTRatio() - 1.0);

	m_coeffs[0] = static_cast<float>(std::pow(10.0, (m_delay[4] / 2) * ReverbTimeSmp));
	m_coeffs[1] = static_cast<float>(std::pow(10.0, (m_delay[5] / 2) * ReverbTimeSmp));

	double sum = 0.0;
	for(uint32 pair = 0; pair < 4; pair++)
	{
		double gain1 = std::pow(10.0, (m_delay[pair] / 4) * ReverbTimeSmp);
		double gain2 = (1.0 - std::pow(10.0, ((m_delay[pair] / 4) + (m_delay[4 + pair / 2] / 2)) * ReverbTimeSmpHF)) * 0.5;
		double gain3 = gain1 * m_coeffs[pair / 2];
		double gain4 = gain3 * (((gain3 + 1.0) * gain3 + 1.0) * gain3 + 1.0) + 1.0;
		m_coeffs[2 + pair * 2] = static_cast<float>(gain1 * (1.0 - gain2));
		m_coeffs[3 + pair * 2] = static_cast<float>(gain1 * gain2);
		sum += gain4 * gain4;
	}

	double inGain = std::pow(10.0, GainInDecibel(m_param[kRvbInGain]) * 0.05);
	double reverbMix = std::pow(10.0, GainInDecibel(m_param[kRvbReverbMix]) * 0.1);
	m_dryFactor = static_cast<float>(std::sqrt(1.0 - reverbMix) * inGain);
	m_wetFactor = static_cast<float>(std::sqrt(reverbMix) * (4.0 / std::sqrt(sum) * inGain));
}

} // namespace DMO

OPENMPT_NAMESPACE_END

#endif // !NO_PLUGINS && NO_DMO
