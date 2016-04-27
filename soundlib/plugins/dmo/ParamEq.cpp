/*
 * ParamEq.cpp
 * -----------
 * Purpose: Implementation of the DMO Parametric Equalizer DSP (for non-Windows platforms)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#if !defined(NO_PLUGINS) && defined(NO_DMO)
#include "../../Sndfile.h"
#include "ParamEq.h"

OPENMPT_NAMESPACE_BEGIN

namespace DMO
{

IMixPlugin* ParamEq::Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
//----------------------------------------------------------------------------------------------
{
	return new (std::nothrow) ParamEq(factory, sndFile, mixStruct);
}


ParamEq::ParamEq(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
	: IMixPlugin(factory, sndFile, mixStruct)
//-----------------------------------------------------------------------------------
{
	m_param[kEqCenter] = 0.497487f;
	m_param[kEqBandwidth] = 0.314286f;
	m_param[kEqGain] = 0.5f;

	m_mixBuffer.Initialize(2, 2);
	InsertIntoFactoryList();
}


void ParamEq::Process(float *pOutL, float *pOutR, uint32 numFrames)
//-----------------------------------------------------------------
{
	if(m_param[kEqGain] == 1.0f || !m_mixBuffer.Ok())
		return;
	const float *in[2] = { m_mixBuffer.GetInputBuffer(0), m_mixBuffer.GetInputBuffer(1) };
	float *out[2] = { m_mixBuffer.GetOutputBuffer(0), m_mixBuffer.GetOutputBuffer(1) };

	for(uint32 i = numFrames; i != 0; i--)
	{
		for(uint8 channel = 0; channel < 2; channel++)
		{
			float x = *(in[channel])++;
			float y = b0DIVa0 * x + b1DIVa0 * x1[channel] + b2DIVa0 * x2[channel] - a1DIVa0 * y1[channel] - a2DIVa0 * y2[channel];

			x2[channel] = x1[channel];
			x1[channel] = x;
			y2[channel] = y1[channel];
			y1[channel] = y;

			*(out[channel])++ = y;
		}
	}

	ProcessMixOps(pOutL, pOutR, m_mixBuffer.GetOutputBuffer(0), m_mixBuffer.GetOutputBuffer(1), numFrames);
}


PlugParamValue ParamEq::GetParameter(PlugParamIndex index)
//--------------------------------------------------------
{
	if(index < kEqNumParameters)
	{
		return m_param[index];
	}
	return 0;
}


void ParamEq::SetParameter(PlugParamIndex index, PlugParamValue value)
//--------------------------------------------------------------------
{
	if(index < kEqNumParameters)
	{
		Limit(value, 0.0f, 1.0f);
		m_param[index] = value;
		RecalculateEqParams();
	}
}


void ParamEq::Resume()
//--------------------
{
	m_isResumed = true;
	RecalculateEqParams();

	// Reset filter state
	x1[0] = x2[0] = 0;
	x1[1] = x2[1] = 0;
	y1[0] = y2[0] = 0;
	y1[1] = y2[1] = 0;
}


#ifdef MODPLUG_TRACKER

CString ParamEq::GetParamName(PlugParamIndex param)
//-------------------------------------------------
{
	switch(param)
	{
	case kEqCenter: return _T("Center");
	case kEqBandwidth: return _T("Bandwidth");
	case kEqGain: return _T("Gain");
	}
	return CString();
}


CString ParamEq::GetParamLabel(PlugParamIndex param)
//--------------------------------------------------
{
	switch(param)
	{
	case kEqCenter: return _T("Hz");
	case kEqBandwidth: return _T("Semitones");
	case kEqGain: return _T("dB");
	}
	return CString();
}


CString ParamEq::GetParamDisplay(PlugParamIndex param)
//----------------------------------------------------
{
	float value = 0.0f;
	switch(param)
	{
	case kEqCenter:
		value = FreqInHertz();
		break;
	case kEqBandwidth:
		value = BandwidthInSemitones();
		break;
	case kEqGain:
		value = GainInDecibel();
		break;
	}
	CString s;
	s.Format("%.2f", value);
	return s;
}

#endif // MODPLUG_TRACKER


void ParamEq::RecalculateEqParams()
//---------------------------------
{
	const float freq = std::min(FreqInHertz() / m_SndFile.GetSampleRate(), 0.5f);
	const float a = std::pow(10, GainInDecibel() / 40.0f);
	const float w0 = 2.0f * float(M_PI) * freq;
	const float sinW0 = std::sin(w0);
	const float cosW0 = std::cos(w0);
	const float alpha = sinW0 * std::sinh((BandwidthInSemitones() * (float(M_LN2) / 24.0f)) * w0 / sinW0);

	const float b0 = 1.0f + alpha * a;
	const float b1 = -2.0f * cosW0;
	const float b2 = 1.0f - alpha * a;
	const float a0 = 1.0f + alpha / a;
	const float a1 = -2.0f * cosW0;
	const float a2 = 1.0f - alpha / a;

	b0DIVa0 = b0 / a0;
	b1DIVa0 = b1 / a0;
	b2DIVa0 = b2 / a0;
	a1DIVa0 = a1 / a0;
	a2DIVa0 = a2 / a0;
}

} // namespace DMO

OPENMPT_NAMESPACE_END

#endif // !NO_PLUGINS && NO_DMO
