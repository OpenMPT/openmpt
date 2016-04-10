/*
 * DigiBoosterEcho.cpp
 * -------------------
 * Purpose: Implementation of the DigiBooster Pro Echo DSP
 * Notes  : (currently none)
 * Authors: OpenMPT Devs, based on original code by Grzegorz Kraszewski (BSD 2-clause)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#ifndef NO_PLUGINS
#include "../Sndfile.h"
#include "DigiBoosterEcho.h"

OPENMPT_NAMESPACE_BEGIN

IMixPlugin* DigiBoosterEcho::Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
//------------------------------------------------------------------------------------------------------
{
	return new (std::nothrow) DigiBoosterEcho(factory, sndFile, mixStruct);
}


DigiBoosterEcho::DigiBoosterEcho(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
	: IMixPlugin(factory, sndFile, mixStruct)
	, m_bufferSize(0)
	, m_writePos(0)
	, m_sampleRate(sndFile.GetSampleRate())
//---------------------------------------------------------------------------------------------------
{
	RecalculateEchoParams();
	InsertIntoFactoryList();
}


void DigiBoosterEcho::Process(float *pOutL, float *pOutR, uint32 numFrames)
//-------------------------------------------------------------------------
{
	const float *srcL = m_mixBuffer.GetInputBuffer(0), *srcR = m_mixBuffer.GetInputBuffer(1);
	float *outL = m_mixBuffer.GetOutputBuffer(0), *outR = m_mixBuffer.GetOutputBuffer(1);

	for(uint32 i = numFrames; i != 0; i--)
	{
		int readPos = m_writePos - m_delayTime;
		if(readPos < 0)
			readPos += m_bufferSize;

		float l = *srcL++, r = *srcR++;
		float lDelay = m_delayLine[readPos * 2], rDelay = m_delayLine[readPos * 2 + 1];

		// Calculate the delay
		float al = l * m_NCrossNBack;
		al += r * m_PCrossNBack;
		al += lDelay * m_NCrossPBack;
		al += rDelay * m_PCrossPBack;

		float ar = r * m_NCrossNBack;
		ar += l * m_PCrossNBack;
		ar += rDelay * m_NCrossPBack;
		ar += lDelay * m_PCrossPBack;

		// Prevent denormals
		if(std::abs(al) < 1e-24f)
			al = 0.0f;
		if(std::abs(ar) < 1e-24f)
			ar = 0.0f;

		m_delayLine[m_writePos * 2] = al;
		m_delayLine[m_writePos * 2 + 1] = ar;
		m_writePos++;
		if(m_writePos == m_bufferSize)
			m_writePos = 0;

		// Output samples now
		*outL++ = (l * m_NMix + lDelay * m_PMix);
		*outR++ = (r * m_NMix + rDelay * m_PMix);
	}

	ProcessMixOps(pOutL, pOutR, m_mixBuffer.GetOutputBuffer(0), m_mixBuffer.GetOutputBuffer(1), numFrames);
}


void DigiBoosterEcho::SaveAllParameters()
//---------------------------------------
{
	m_pMixStruct->defaultProgram = -1;
	if(m_pMixStruct->nPluginDataSize != sizeof(chunk) || m_pMixStruct->pPluginData == nullptr)
	{
		delete[] m_pMixStruct->pPluginData;
		m_pMixStruct->nPluginDataSize = sizeof(chunk);
		m_pMixStruct->pPluginData = new (std::nothrow) char[sizeof(chunk)];
	}
	if(m_pMixStruct->pPluginData != nullptr)
	{
		memcpy(m_pMixStruct->pPluginData, &chunk, sizeof(chunk));
	}
}


void DigiBoosterEcho::RestoreAllParameters(int32 program)
//-------------------------------------------------------
{
	if(m_pMixStruct->nPluginDataSize == sizeof(chunk) && !memcmp(m_pMixStruct->pPluginData, "Echo", 4))
	{
		memcpy(&chunk, m_pMixStruct->pPluginData, sizeof(chunk));
	} else
	{
		IMixPlugin::RestoreAllParameters(program);
	}
	RecalculateEchoParams();
}


PlugParamValue DigiBoosterEcho::GetParameter(PlugParamIndex index)
//----------------------------------------------------------------
{
	if(index < kEchoNumParameters)
	{
		return chunk.param[index] / 255.0f;
	}
	return 0;
}


void DigiBoosterEcho::SetParameter(PlugParamIndex index, PlugParamValue value)
//----------------------------------------------------------------------------
{
	if(index < kEchoNumParameters)
	{
		chunk.param[index] = Util::Round<uint8>(value * 255.0f);
		RecalculateEchoParams();
	}
}


void DigiBoosterEcho::Resume()
//----------------------------
{
	m_sampleRate = m_SndFile.GetSampleRate();
	m_bufferSize = (m_sampleRate >> 1) + (m_sampleRate >> 6);
	RecalculateEchoParams();
	try
	{
		m_delayLine.assign(m_bufferSize * 2, 0);
	} catch(MPTMemoryException)
	{
		m_bufferSize = 0;
	}
	m_writePos = 0;
}


#ifdef MODPLUG_TRACKER

CString DigiBoosterEcho::GetParamName(PlugParamIndex param)
//---------------------------------------------------------
{
	switch(param)
	{
	case kEchoDelay: return _T("Delay");
	case kEchoFeedback: return _T("Feedback");
	case kEchoMix: return _T("Wet / Dry Ratio");
	case kEchoCross: return _T("Cross Echo");
	}
	return CString();
}


CString DigiBoosterEcho::GetParamLabel(PlugParamIndex param)
//----------------------------------------------------------
{
	if(param == kEchoDelay)
		return _T("ms");
	return CString();
}


CString DigiBoosterEcho::GetParamDisplay(PlugParamIndex param)
//------------------------------------------------------------
{
	CString s;
	if(param == kEchoMix)
	{
		int wet = (chunk.param[kEchoMix] * 100) / 255;
		s.Format(_T("%d%% / %d%%"), wet, 100 - wet);
	} else if(param < kEchoNumParameters)
	{
		int val = chunk.param[param];
		if(param == kEchoDelay)
			val *= 2;
		s.Format(_T("%d"), val);
	}
	return s;
}

#endif // MODPLUG_TRACKER


size_t DigiBoosterEcho::GetChunk(char *(&data), bool)
//---------------------------------------------------
{
	data = reinterpret_cast<char *>(&chunk);
	return sizeof(chunk);
}


void DigiBoosterEcho::SetChunk(size_t size, char *data, bool)
//-----------------------------------------------------------
{
	if(size == sizeof(chunk) && !memcmp(data, "Echo", 4))
	{
		memcpy(&chunk, data, size);
		RecalculateEchoParams();
	}
}


void DigiBoosterEcho::RecalculateEchoParams()
//-------------------------------------------
{
	m_delayTime = (chunk.param[kEchoDelay] * m_sampleRate + 250) / 500;
	m_PMix = (chunk.param[kEchoMix]) * (1.0f / 256.0f);
	m_NMix = (256 - chunk.param[kEchoMix]) * (1.0f / 256.0f);
	m_PCrossPBack = (chunk.param[kEchoCross] * chunk.param[kEchoFeedback]) * (1.0f / 65536.0f);
	m_PCrossNBack = (chunk.param[kEchoCross] * (256 - chunk.param[kEchoFeedback])) * (1.0f / 65536.0f);
	m_NCrossPBack = ((chunk.param[kEchoCross] - 256) * chunk.param[kEchoFeedback]) * (1.0f / 65536.0f);
	m_NCrossNBack = ((chunk.param[kEchoCross] - 256) * (chunk.param[kEchoFeedback] - 256)) * (1.0f / 65536.0f);
}

OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
