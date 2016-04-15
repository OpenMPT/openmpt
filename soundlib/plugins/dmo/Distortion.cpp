/*
 * Distortion.cpp
 * --------------
 * Purpose: Implementation of the DMO Distortion DSP (for non-Windows platforms)
 * Notes  : The original plugin's integer and floating point code paths only
 *          behave identically when feeding floating point numbers in range
 *          [-32768, +32768] rather than the usual [-1, +1] into the plugin.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#if !defined(NO_PLUGINS) && defined(NO_DMO)
#include "../../Sndfile.h"
#include "Distortion.h"

OPENMPT_NAMESPACE_BEGIN

namespace DMO
{

// Computes (log2(x) + 1) * 2 ^ (shiftL - shiftR) (x = -2^31...2^31)
float logGain(float x, int32 shiftL, int32 shiftR)
//------------------------------------------------
{
	uint32 intSample = static_cast<uint32>(static_cast<int32>(x));
	const uint32 sign = intSample & 0x80000000;
	if(sign)
		intSample = (~intSample) + 1;

	// Multiply until overflow (or edge shift factor is reached)
	while(shiftL > 0 && intSample < 0x80000000)
	{
		intSample += intSample;
		shiftL--;
	}
	// Unsign clipped sample
	if(intSample >= 0x80000000)
	{
		intSample &= 0x7FFFFFFF;
		shiftL++;
	}
	intSample = (shiftL << (31 - shiftR)) | (intSample >> shiftR);
	if(sign)
		intSample = ~intSample | sign;
	return static_cast<float>(static_cast<int32>(intSample));
}


IMixPlugin* Distortion::Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
//-------------------------------------------------------------------------------------------------
{
	return new (std::nothrow) Distortion(factory, sndFile, mixStruct);
}


Distortion::Distortion(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
	: IMixPlugin(factory, sndFile, mixStruct)
//-----------------------------------------------------------------------------------------
{
	m_param[kDistGain] = 0.7f;
	m_param[kDistEdge] = 0.15f;
	m_param[kDistPreLowpassCutoff] = 1.0f;
	m_param[kDistPostEQCenterFrequency] = 0.291f;
	m_param[kDistPostEQBandwidth] = 0.291f;
	RecalculateDistortionParams();

	m_mixBuffer.Initialize(2, 2);
	m_MixState.pOutBufferL = m_mixBuffer.GetInputBuffer(0);
	m_MixState.pOutBufferR = m_mixBuffer.GetInputBuffer(1);

	InsertIntoFactoryList();
}


void Distortion::Process(float *pOutL, float *pOutR, uint32 numFrames)
//--------------------------------------------------------------------
{
	const float *in[2] = { m_mixBuffer.GetInputBuffer(0), m_mixBuffer.GetInputBuffer(1) };
	float *out[2] = { m_mixBuffer.GetOutputBuffer(0), m_mixBuffer.GetOutputBuffer(1) };

	for(uint32 i = numFrames; i != 0; i--)
	{
		for(uint8 channel = 0; channel < 2; channel++)
		{
			float x = *(in[channel])++;

			// Pre EQ
			float z = x * m_preEQa0 + m_preEQz1[channel] * m_preEQb1;
			m_preEQz1[channel] = z;

			z *= 1073741824.0f;	// 32768^2

			// The actual distortion
			z = logGain(z, m_edge, m_shift);

			// Post EQ / Gain
			z = (z * m_postEQa0) - m_postEQz1[channel] * m_postEQb1 - m_postEQz2[channel] * m_postEQb0;
			m_postEQz1[channel] = z * m_postEQb0 + m_postEQz2[channel];
			m_postEQz2[channel] = z;

			z *= (1.0f / 1073741824.0f);	// 32768^2
			*(out[channel])++ = z;
		}
	}

	ProcessMixOps(pOutL, pOutR, m_mixBuffer.GetOutputBuffer(0), m_mixBuffer.GetOutputBuffer(1), numFrames);
}


PlugParamValue Distortion::GetParameter(PlugParamIndex index)
//-----------------------------------------------------------
{
	if(index < kDistNumParameters)
	{
		return m_param[index];
	}
	return 0;
}


void Distortion::SetParameter(PlugParamIndex index, PlugParamValue value)
//-----------------------------------------------------------------------
{
	if(index < kDistNumParameters)
	{
		Limit(value, 0.0f, 1.0f);
		m_param[index] = value;
		RecalculateDistortionParams();
	}
}


void Distortion::Resume()
//-----------------------
{
	RecalculateDistortionParams();

	// Reset filter state
	m_preEQz1[0] = m_preEQz1[1] = 0;
	m_postEQz1[0] = m_postEQz2[0] = 0;
	m_postEQz1[1] = m_postEQz2[1] = 0;
}


#ifdef MODPLUG_TRACKER

CString Distortion::GetParamName(PlugParamIndex param)
//----------------------------------------------------
{
	switch(param)
	{
	case kDistGain: return _T("Gain");
	case kDistEdge: return _T("Edge");
	case kDistPreLowpassCutoff: return _T("PreLowpassCutoff");
	case kDistPostEQCenterFrequency: return _T("PostEQCenterFrequency");
	case kDistPostEQBandwidth: return _T("PostEQBandwidth");
	}
	return CString();
}


CString Distortion::GetParamLabel(PlugParamIndex param)
//-----------------------------------------------------
{
	switch(param)
	{
	case kDistGain: return _T("dB");
	case kDistPreLowpassCutoff:
	case kDistPostEQCenterFrequency:
		return _T("Hz");
	}
	return CString();
}


CString Distortion::GetParamDisplay(PlugParamIndex param)
//-------------------------------------------------------
{
	float value = m_param[param];
	switch(param)
	{
	case kDistGain:
		value = GainInDecibel();
		break;
	case kDistEdge:
		value *= 100.0f;
		break;
	case kDistPreLowpassCutoff:
	case kDistPostEQCenterFrequency:
	case kDistPostEQBandwidth:
		value = FreqInHertz(value);
		break;
	}
	CString s;
	s.Format("%.2f", value);
	return s;
}

#endif // MODPLUG_TRACKER


void Distortion::RecalculateDistortionParams()
//--------------------------------------------
{
	// Pre-EQ
	m_preEQb1 = std::sqrt((2.0f * std::cos(2.0f * float(M_PI) * std::min(FreqInHertz(m_param[kDistPreLowpassCutoff]) / m_SndFile.GetSampleRate(), 0.5f)) + 3.0f) / 5.0f);
	m_preEQa0 = std::sqrt(1.0f - m_preEQb1 * m_preEQb1);

	// Distortion
	float edge = 2.0f + m_param[kDistEdge] * 29.0f;
	m_edge = static_cast<uint8>(edge);	// 2...31 shifted bits

	// Work out the magical shift factor (= floor(log2(edge)) + 1 / index of highest bit + 1)
	uint8 shift;
	if(m_edge <= 3)
		shift = 2;
	else if(m_edge <= 7)
		shift = 3;
	else if(m_edge <= 15)
		shift = 4;
	else
		shift = 5;
	m_shift = shift;

	static const double LogNorm[32] =
	{
		1.0000000000000000, 1.0000000000000000, 1.5000000000000000, 1.0000000000000000,
		1.7500000000000000, 1.3999999999999999, 1.1699999999999999, 1.0000000000000000,
		1.8799999999999999, 1.7600000000000000, 1.5000000000000000, 1.3600000000000001,
		1.2500000000000000, 1.1499999999999999, 1.0700000000000001, 1.0000000000000000,
		1.9399999999999999, 1.8200000000000001, 1.7200000000000000, 1.6299999999999999,
		1.5500000000000000, 1.4800000000000000, 1.4099999999999999, 1.3500000000000001,
		1.2900000000000000, 1.2400000000000000, 1.1899999999999999, 1.1499999999999999,
		1.1100000000000001, 1.0700000000000001, 1.0300000000000000, 1.0000000000000000,
	};

	// Post-EQ
	const float gain = std::pow(10.0f, GainInDecibel() / 20.0f);
	const float postFreq = 2.0f * float(M_PI) * std::min(FreqInHertz(m_param[kDistPostEQCenterFrequency]) / m_SndFile.GetSampleRate(), 0.5f);
	const float postBw = 2.0f * float(M_PI) * std::min(FreqInHertz(m_param[kDistPostEQBandwidth]) / m_SndFile.GetSampleRate(), 0.5f);
	const float t = std::tan(5.0e-1f * postBw);
	m_postEQb1 = ((1.0f - t) / (1.0f + t));
	m_postEQb0 = -std::cos(postFreq);
	m_postEQa0 = static_cast<float>(gain * std::sqrt(1.0f - m_postEQb0 * m_postEQb0) * std::sqrt(1.0f - m_postEQb1 * m_postEQb1) * LogNorm[m_edge]);
}

} // namespace DMO

OPENMPT_NAMESPACE_END

#endif // !NO_PLUGINS && NO_DMO
