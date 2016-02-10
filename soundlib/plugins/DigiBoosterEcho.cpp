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
#include "Sndfile.h"
#include "DigiBoosterEcho.h"

OPENMPT_NAMESPACE_BEGIN

DigiBoosterEcho::DigiBoosterEcho(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
	: IMixPlugin(factory, sndFile, mixStruct)
	, m_bufferSize(0)
	, m_writePos(0)
	, m_delayTime(0)
	, m_sampleRate(sndFile.GetSampleRate())
	, m_PMix(0.0f)
	, m_NMix(0.0f)
	, m_PCrossPBack(0.0f)
	, m_PCrossNBack(0.0f)
	, m_NCrossPBack(0.0f)
	, m_NCrossNBack(0.0f)
//---------------------------------------------------------------------------------------------------
{
	RecalculateEchoParams();

	m_MixState.pMixBuffer = (mixsample_t *)((((intptr_t)m_MixBuffer) + 7) & ~7);
	m_MixState.pOutBufferL = m_mixBuffer.GetInputBuffer(0);
	m_MixState.pOutBufferR = m_mixBuffer.GetInputBuffer(1);

	m_pMixStruct->pMixState = &m_MixState;
	m_pMixStruct->pMixPlugin = this;
}


void DigiBoosterEcho::Process(float *pOutL, float *pOutR, size_t numSamples)
//--------------------------------------------------------------------------
{
	float *srcL = m_mixBuffer.GetInputBuffer(0), *srcR = m_mixBuffer.GetInputBuffer(1);

	while(numSamples--)
	{
		int readPos = m_writePos - m_delayTime;
		if(readPos < 0)
			readPos += m_bufferSize;

		float l = *srcL++;
		float r = *srcR++;
		float lDelay = m_delayLine[readPos * 2];
		float rDelay = m_delayLine[readPos * 2 + 1];

		// Calculate the delay
		float al = l * m_NCrossNBack;
		al += r * m_PCrossNBack;
		al += lDelay * m_NCrossPBack;
		al += rDelay * m_PCrossPBack;

		float ar = r * m_NCrossNBack;
		ar += l * m_PCrossNBack;
		ar += rDelay * m_NCrossPBack;
		ar += lDelay * m_PCrossPBack;

		m_delayLine[m_writePos * 2] = al;
		m_delayLine[m_writePos * 2 + 1] = ar;
		m_writePos++;
		if(m_writePos == m_bufferSize)
			m_writePos = 0;

		// Output samples now
		*pOutL++ += (l * m_NMix + lDelay * m_PMix);
		*pOutR++ += (r * m_NMix + rDelay * m_PMix);
	}
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
	m_bufferSize = ((m_sampleRate >> 1) + (m_sampleRate >> 6) + 3) & ~4;
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
	case kEchoMix: return _T("Mix");
	case kEchoCross: return _T("Cross");
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
		s.Format(_T("%d%% wet, %d%% dry"), wet, 100 - wet);
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
