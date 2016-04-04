/*
 * DMOPlugin.h
 * -----------
 * Purpose: DirectX Media Object plugin handling / processing.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#ifndef NO_DMO
#include "../../Sndfile.h"
#include "../../../common/mptUUID.h"
#include "DMOPlugin.h"
#include "../PluginManager.h"
#include <uuids.h>
#include <medparam.h>
#include <mmsystem.h>
#endif // !NO_DMO


OPENMPT_NAMESPACE_BEGIN


#ifndef NO_DMO


#define DMO_LOG

IMixPlugin* DMOPlugin::Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
//------------------------------------------------------------------------------------------------
{
	CLSID clsid;
	if (Util::VerifyStringToCLSID(factory.dllPath.ToWide(), clsid))
	{
		IMediaObject *pMO = nullptr;
		IMediaObjectInPlace *pMOIP = nullptr;
		if ((CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_IMediaObject, (VOID **)&pMO) == S_OK) && (pMO))
		{
			if (pMO->QueryInterface(IID_IMediaObjectInPlace, (void **)&pMOIP) != S_OK) pMOIP = nullptr;
		} else pMO = nullptr;
		if ((pMO) && (pMOIP))
		{
			DWORD dwInputs = 0, dwOutputs = 0;
			pMO->GetStreamCount(&dwInputs, &dwOutputs);
			if (dwInputs == 1 && dwOutputs == 1)
			{
				DMOPlugin *p = new (std::nothrow) DMOPlugin(factory, sndFile, mixStruct, pMO, pMOIP, clsid.Data1);
				return p;
			}
#ifdef DMO_LOG
			Log(factory.libraryName.ToUnicode() + MPT_USTRING(": Unable to use this DMO"));
#endif
		}
#ifdef DMO_LOG
		else Log(factory.libraryName.ToUnicode() + MPT_USTRING(": Failed to get IMediaObject & IMediaObjectInPlace interfaces"));
#endif
		if (pMO) pMO->Release();
		if (pMOIP) pMOIP->Release();
	}
	return nullptr;
}


DMOPlugin::DMOPlugin(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct, IMediaObject *pMO, IMediaObjectInPlace *pMOIP, uint32 uid)
	: IMixPlugin(factory, sndFile, mixStruct)
	, m_pMediaObject(pMO)
	, m_pMediaProcess(pMOIP)
	, m_pParamInfo(nullptr)
	, m_pMediaParams(nullptr)
	, m_nSamplesPerSec(sndFile.GetSampleRate())
	, m_uid(uid)
//--------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(FAILED(m_pMediaObject->QueryInterface(IID_IMediaParamInfo, (void **)&m_pParamInfo)))
		m_pParamInfo = nullptr;
	if (FAILED(m_pMediaObject->QueryInterface(IID_IMediaParams, (void **)&m_pMediaParams)))
		m_pMediaParams = nullptr;
	m_pMixBuffer = (int16 *)((((intptr_t)m_MixBuffer16) + 15) & ~15);

	m_mixBuffer.Initialize(2, 2);
	m_MixState.pOutBufferL = m_mixBuffer.GetInputBuffer(0);
	m_MixState.pOutBufferR = m_mixBuffer.GetInputBuffer(1);

	InsertIntoFactoryList();

}


DMOPlugin::~DMOPlugin()
//---------------------
{
	if(m_pMediaParams)
	{
		m_pMediaParams->Release();
		m_pMediaParams = nullptr;
	}
	if(m_pParamInfo)
	{
		m_pParamInfo->Release();
		m_pParamInfo = nullptr;
	}
	if(m_pMediaProcess)
	{
		m_pMediaProcess->Release();
		m_pMediaProcess = nullptr;
	}
	if(m_pMediaObject)
	{
		m_pMediaObject->Release();
		m_pMediaObject = nullptr;
	}
}


uint32 DMOPlugin::GetLatency() const
//----------------------------------
{
	REFERENCE_TIME time;	// Unit 100-nanoseconds
	if(m_pMediaProcess->GetLatency(&time) == S_OK)
	{
		return static_cast<uint32>(time * m_nSamplesPerSec / (10 * 1000 * 1000));
	}
	return 0;
}


static const float _f2si = 32768.0f;
static const float _si2f = 1.0f / 32768.0f;

// Interleave two float streams into one int16 stereo stream.
void DMOPlugin::InterleaveFloatToInt16(const float *inLeft, const float *inRight, uint32 numFrames)
//-------------------------------------------------------------------------------------------------
{
	int16 *outBuf = m_pMixBuffer;
	while(numFrames--)
	{
		*(outBuf++) = static_cast<int16>(Clamp(*(inLeft++) * _f2si, static_cast<float>(int16_min), static_cast<float>(int16_max)));
		*(outBuf++) = static_cast<int16>(Clamp(*(inRight++) * _f2si, static_cast<float>(int16_min), static_cast<float>(int16_max)));
	}
}


// Deinterleave an int16 stereo stream into two float streams.
void DMOPlugin::DeinterleaveInt16ToFloat(float *outLeft, float *outRight, uint32 numFrames) const
//-----------------------------------------------------------------------------------------------
{
	const int16 *inBuf = m_pMixBuffer;
	while(numFrames--)
	{
		*outLeft++ += _si2f * static_cast<float>(*inBuf++);
		*outRight++ += _si2f * static_cast<float>(*inBuf++);
	}
}


#ifdef ENABLE_SSE
// Interleave two float streams into one int16 stereo stream using SSE magic.
void DMOPlugin::SSEInterleaveFloatToInt16(const float *inLeft, const float *inRight, uint32 numFrames)
//----------------------------------------------------------------------------------------------------
{
	int16 *outBuf = m_pMixBuffer;
	_asm
	{
		mov eax, inLeft
			mov edx, inRight
			mov ebx, outBuf
			mov ecx, numFrames
			movss xmm2, _f2si
			xorps xmm0, xmm0
			xorps xmm1, xmm1
			shufps xmm2, xmm2, 0x00
			pxor mm0, mm0
			inc ecx
			shr ecx, 1
mainloop:
		movlps xmm0, [eax]
		movlps xmm1, [edx]
		mulps xmm0, xmm2
			mulps xmm1, xmm2
			add ebx, 8
			cvtps2pi mm0, xmm0	// mm0 = [ x2l | x1l ]
			add eax, 8
			cvtps2pi mm1, xmm1	// mm1 = [ x2r | x1r ]
			add edx, 8
			packssdw mm0, mm1	// mm0 = [x2r|x1r|x2l|x1l]
			pshufw mm0, mm0, 0xD8
			dec ecx
			movq [ebx-8], mm0
			jnz mainloop
			emms
	}
}


// Deinterleave an int16 stereo stream into two float streams using SSE magic.
void DMOPlugin::SSEDeinterleaveInt16ToFloat(float *outLeft, float *outRight, uint32 numFrames) const
//--------------------------------------------------------------------------------------------------
{
	const int16 *inBuf = m_pMixBuffer;
	_asm {
		mov ebx, inBuf
			mov eax, outLeft
			mov edx, outRight
			mov ecx, numFrames
			movss xmm7, _si2f
			inc ecx
			shr ecx, 1
			shufps xmm7, xmm7, 0x00
			xorps xmm0, xmm0
			xorps xmm1, xmm1
			xorps xmm2, xmm2
mainloop:
		movq mm0, [ebx]		// mm0 = [x2r|x2l|x1r|x1l]
		add ebx, 8
			pxor mm1, mm1
			pxor mm2, mm2
			punpcklwd mm1, mm0	// mm1 = [x1r|0|x1l|0]
			punpckhwd mm2, mm0	// mm2 = [x2r|0|x2l|0]
			psrad mm1, 16		// mm1 = [x1r|x1l]
			movlps xmm2, [eax]
		psrad mm2, 16		// mm2 = [x2r|x2l]
			cvtpi2ps xmm0, mm1	// xmm0 = [ ? | ? |x1r|x1l]
			dec ecx
			cvtpi2ps xmm1, mm2	// xmm1 = [ ? | ? |x2r|x2l]
			movhps xmm2, [edx]	// xmm2 = [y2r|y1r|y2l|y1l]
		movlhps xmm0, xmm1	// xmm0 = [x2r|x2l|x1r|x1l]
			shufps xmm0, xmm0, 0xD8
			lea eax, [eax+8]
		mulps xmm0, xmm7
			addps xmm0, xmm2
			lea edx, [edx+8]
		movlps [eax-8], xmm0
			movhps [edx-8], xmm0
			jnz mainloop
			emms
	}
}

#endif // ENABLE_SSE


void DMOPlugin::Process(float *pOutL, float *pOutR, uint32 numFrames)
//-------------------------------------------------------------------
{
	m_mixBuffer.ClearOutputBuffers(numFrames);
	REFERENCE_TIME startTime = Util::muldiv(m_SndFile.GetTotalSampleCount(), 10000000, m_nSamplesPerSec);
#ifdef ENABLE_SSE
	if(GetProcSupport() & PROCSUPPORT_SSE)
	{
		SSEInterleaveFloatToInt16(m_mixBuffer.GetInputBuffer(0), m_mixBuffer.GetInputBuffer(1), numFrames);
		m_pMediaProcess->Process(numFrames * 2 * sizeof(int16), reinterpret_cast<BYTE *>(m_pMixBuffer), startTime, DMO_INPLACE_NORMAL);
		SSEDeinterleaveInt16ToFloat(m_mixBuffer.GetOutputBuffer(0), m_mixBuffer.GetOutputBuffer(1), numFrames);
	} else
#endif // ENABLE_SSE
	{
		InterleaveFloatToInt16(m_mixBuffer.GetInputBuffer(0), m_mixBuffer.GetInputBuffer(1), numFrames);
		m_pMediaProcess->Process(numFrames * 2 * sizeof(int16), reinterpret_cast<BYTE *>(m_pMixBuffer), startTime, DMO_INPLACE_NORMAL);
		DeinterleaveInt16ToFloat(m_mixBuffer.GetOutputBuffer(0), m_mixBuffer.GetOutputBuffer(1), numFrames);
	}

	ProcessMixOps(pOutL, pOutR, m_mixBuffer.GetOutputBuffer(0), m_mixBuffer.GetOutputBuffer(1), numFrames);
}


PlugParamIndex DMOPlugin::GetNumParameters() const
//------------------------------------------------
{
	DWORD dwParamCount = 0;
	m_pParamInfo->GetParamCount(&dwParamCount);
	return dwParamCount;
}


PlugParamValue DMOPlugin::GetParameter(PlugParamIndex index)
//----------------------------------------------------------
{
	if(index < GetNumParameters() && m_pParamInfo != nullptr && m_pMediaParams != nullptr)
	{
		MP_PARAMINFO mpi;
		MP_DATA md;

		MemsetZero(mpi);
		md = 0;
		if (m_pParamInfo->GetParamInfo(index, &mpi) == S_OK
			&& m_pMediaParams->GetParam(index, &md) == S_OK)
		{
			float fValue, fMin, fMax, fDefault;

			fValue = md;
			fMin = mpi.mpdMinValue;
			fMax = mpi.mpdMaxValue;
			fDefault = mpi.mpdNeutralValue;
			if (mpi.mpType == MPT_BOOL)
			{
				fMin = 0;
				fMax = 1;
			}
			fValue -= fMin;
			if (fMax > fMin) fValue /= (fMax - fMin);
			return fValue;
		}
	}
	return 0;
}


void DMOPlugin::SetParameter(PlugParamIndex index, PlugParamValue value)
//----------------------------------------------------------------------
{
	if(index < GetNumParameters() && m_pParamInfo != nullptr && m_pMediaParams != nullptr)
	{
		MP_PARAMINFO mpi;
		MemsetZero(mpi);
		if (m_pParamInfo->GetParamInfo(index, &mpi) == S_OK)
		{
			float fMin = mpi.mpdMinValue;
			float fMax = mpi.mpdMaxValue;

			if (mpi.mpType == MPT_BOOL)
			{
				fMin = 0;
				fMax = 1;
				value = (value > 0.5f) ? 1.0f : 0.0f;
			}
			if (fMax > fMin) value *= (fMax - fMin);
			value += fMin;
			Limit(value, fMin, fMax);
			if (mpi.mpType != MPT_FLOAT) value = (float)(int)value;
			m_pMediaParams->SetParam(index, value);
		}
	}
}


void DMOPlugin::Resume()
//----------------------
{
	m_nSamplesPerSec = m_SndFile.GetSampleRate();

	DMO_MEDIA_TYPE mt;
	WAVEFORMATEX wfx;

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_PCM;
	mt.bFixedSizeSamples = TRUE;
	mt.bTemporalCompression = FALSE;
	mt.formattype = FORMAT_WaveFormatEx;
	mt.pUnk = nullptr;
	mt.pbFormat = (LPBYTE)&wfx;
	mt.cbFormat = sizeof(WAVEFORMATEX);
	mt.lSampleSize = 2 * sizeof(int16);
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = m_nSamplesPerSec;
	wfx.wBitsPerSample = sizeof(int16) * 8;
	wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
	wfx.cbSize = 0;
	if(FAILED(m_pMediaObject->SetInputType(0, &mt, 0))
		|| FAILED(m_pMediaObject->SetOutputType(0, &mt, 0)))
	{
#ifdef DMO_LOG
		Log(MPT_USTRING("DMO: Failed to set I/O media type"));
#endif
	}
}


void DMOPlugin::Suspend()
//-----------------------
{
	m_pMediaObject->Flush();
	m_pMediaObject->SetInputType(0, nullptr, DMO_SET_TYPEF_CLEAR);
	m_pMediaObject->SetOutputType(0, nullptr, DMO_SET_TYPEF_CLEAR);
}


#ifdef MODPLUG_TRACKER

CString DMOPlugin::GetParamName(PlugParamIndex param)
//---------------------------------------------------
{
	if(param < GetNumParameters() && m_pParamInfo != nullptr)
	{
		MP_PARAMINFO mpi;
		mpi.mpType = MPT_INT;
		mpi.szUnitText[0] = 0;
		mpi.szLabel[0] = 0;
		if(m_pParamInfo->GetParamInfo(param, &mpi) == S_OK)
		{
			return mpi.szLabel;
		}
	}
	return CString();

}


CString DMOPlugin::GetParamLabel(PlugParamIndex param)
//----------------------------------------------------
{
	if(param < GetNumParameters() && m_pParamInfo != nullptr)
	{
		MP_PARAMINFO mpi;
		mpi.mpType = MPT_INT;
		mpi.szUnitText[0] = 0;
		mpi.szLabel[0] = 0;
		if(m_pParamInfo->GetParamInfo(param, &mpi) == S_OK)
		{
			return mpi.szUnitText;
		}
	}
	return CString();
}


CString DMOPlugin::GetParamDisplay(PlugParamIndex param)
//------------------------------------------------------
{
	if(param < GetNumParameters() && m_pParamInfo != nullptr && m_pMediaParams != nullptr)
	{
		MP_PARAMINFO mpi;
		mpi.mpType = MPT_INT;
		mpi.szUnitText[0] = 0;
		mpi.szLabel[0] = 0;
		if (m_pParamInfo->GetParamInfo(param, &mpi) == S_OK)
		{
			MP_DATA md;
			if(m_pMediaParams->GetParam(param, &md) == S_OK)
			{
				switch(mpi.mpType)
				{
				case MPT_FLOAT:
					{
						int nValue = (int)(md * 100.0f + 0.5f);
						bool bNeg = false;
						if (nValue < 0) { bNeg = true; nValue = -nValue; }
						CString s;
						s.Format(bNeg ? _T("-%d.%02d") : _T("%d.%02d"), nValue / 100, nValue % 100);
						return s;
					}
					break;

				case MPT_BOOL:
					return ((int)md) ? _T("Yes") : _T("No");
					break;

				case MPT_ENUM:
					{
						WCHAR *text = nullptr;
						m_pParamInfo->GetParamText(param, &text);

						const int nValue = (int)(md * (mpi.mpdMaxValue - mpi.mpdMinValue) + 0.5f);
						// Always skip first two strings (param name, unit name)
						for(int i = 0; i < nValue + 2; i++)
						{
							text += wcslen(text) + 1;
						}
						return CString(text);
					}
					break;

				case MPT_INT:
				default:
					{
						CString s;
						s.Format(_T("%d"), (int)md);
						return s;
					}
					break;
				}
			}
		}
	}
	return CString();
}

#endif // MODPLUG_TRACKER

#else // NO_DMO

MPT_MSVC_WORKAROUND_LNK4221(DMOPlugin)

#endif // !NO_DMO

OPENMPT_NAMESPACE_END

