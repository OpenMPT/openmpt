/*
 * DmoToVst.cpp
 * ------------
 * Purpose: DirectX Media Object to VST plugin adapter class.
 * Notes  : This should make use of IMixPlugin rather than going through CVstPlugin.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#ifndef NO_DMO
#include <uuids.h>
#include <dmoreg.h>
#include <medparam.h>
#include "../../mptrack/Vstplug.h"
#include "PluginManager.h"
#include "../Snd_defs.h"
#include "../../common/misc_util.h"


OPENMPT_NAMESPACE_BEGIN


#define DMO_LOG


//============
class CDmo2Vst
//============
{
protected:
	IMediaObject *m_pMediaObject;
	IMediaObjectInPlace *m_pMediaProcess;
	IMediaParamInfo *m_pParamInfo;
	IMediaParams *m_pMediaParams;
	uint32 m_nSamplesPerSec;
	AEffect m_Effect;
	REFERENCE_TIME m_DataTime;
	int16 *m_pMixBuffer;
	int16 m_MixBuffer[MIXBUFFERSIZE * 2 + 16];		// 16-bit Stereo interleaved

public:
	CDmo2Vst(IMediaObject *pMO, IMediaObjectInPlace *pMOIP, DWORD uid);
	~CDmo2Vst();
	AEffect *GetEffect() { return &m_Effect; }
	VstIntPtr Dispatcher(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	void SetParameter(VstInt32 index, float parameter);
	float GetParameter(VstInt32 index);
	void Process(float * const *inputs, float **outputs, int samples);

public:
	static VstIntPtr VSTCALLBACK DmoDispatcher(AEffect *effect, VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	static void VSTCALLBACK DmoSetParameter(AEffect *effect, VstInt32 index, float parameter);
	static float VSTCALLBACK DmoGetParameter(AEffect *effect, VstInt32 index);
	static void VSTCALLBACK DmoProcess(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleframes);

protected:
	// Stream conversion functions
	void InterleaveFloatToInt16(const float *inLeft, const float *inRight, int nsamples);
	void DeinterleaveInt16ToFloat(float *outLeft, float *outRight, int nsamples) const;
#ifdef ENABLE_SSE
	void SSEInterleaveFloatToInt16(const float *inLeft, const float *inRight, int nsamples);
	void SSEDeinterleaveInt16ToFloat(float *outLeft, float *outRight, int nsamples) const;
#endif // ENABLE_SSE
};


CDmo2Vst::CDmo2Vst(IMediaObject *pMO, IMediaObjectInPlace *pMOIP, DWORD uid)
//--------------------------------------------------------------------------
{
	m_pMediaObject = pMO;
	m_pMediaProcess = pMOIP;
	m_pParamInfo = nullptr;
	m_pMediaParams = nullptr;

	MemsetZero(m_Effect);
	m_Effect.magic = kDmoMagic;
	m_Effect.numPrograms = 0;
	m_Effect.numParams = 0;
	m_Effect.numInputs = 2;
	m_Effect.numOutputs = 2;
	m_Effect.flags = 0;		// see constants
	m_Effect.ioRatio = 1.0f;
	m_Effect.object = this;
	m_Effect.uniqueID = uid;
	m_Effect.version = 2;

	m_nSamplesPerSec = 44100;
	m_DataTime = 0;
	if (FAILED(m_pMediaObject->QueryInterface(IID_IMediaParamInfo, (void **)&m_pParamInfo))) m_pParamInfo = nullptr;
	if (m_pParamInfo)
	{
		DWORD dwParamCount = 0;
		m_pParamInfo->GetParamCount(&dwParamCount);
		m_Effect.numParams = dwParamCount;
	}
	if (FAILED(m_pMediaObject->QueryInterface(IID_IMediaParams, (void **)&m_pMediaParams))) m_pMediaParams = nullptr;
	m_pMixBuffer = (int16 *)((((intptr_t)m_MixBuffer) + 15) & ~15);
	// Callbacks
	m_Effect.dispatcher = DmoDispatcher;
	m_Effect.setParameter = DmoSetParameter;
	m_Effect.getParameter = DmoGetParameter;
	m_Effect.process = DmoProcess;
	m_Effect.processReplacing = nullptr;
}


CDmo2Vst::~CDmo2Vst()
//-------------------
{
	if (m_pMediaParams)
	{
		m_pMediaParams->Release();
		m_pMediaParams = nullptr;
	}
	if (m_pParamInfo)
	{
		m_pParamInfo->Release();
		m_pParamInfo = nullptr;
	}
	if (m_pMediaProcess)
	{
		m_pMediaProcess->Release();
		m_pMediaProcess = nullptr;
	}
	if (m_pMediaObject)
	{
		m_pMediaObject->Release();
		m_pMediaObject = nullptr;
	}
}


VstIntPtr CDmo2Vst::DmoDispatcher(AEffect *effect, VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
//------------------------------------------------------------------------------------------------------------------------
{
	if (effect)
	{
		CDmo2Vst *that = (CDmo2Vst *)effect->object;
		if (that) return that->Dispatcher(opCode, index, value, ptr, opt);
	}
	return 0;
}


void CDmo2Vst::DmoSetParameter(AEffect *effect, VstInt32 index, float parameter)
//------------------------------------------------------------------------------
{
	if (effect)
	{
		CDmo2Vst *that = (CDmo2Vst *)effect->object;
		if (that) that->SetParameter(index, parameter);
	}
}


float CDmo2Vst::DmoGetParameter(AEffect *effect, VstInt32 index)
//----------------------------------------------------------
{
	if (effect)
	{
		CDmo2Vst *that = (CDmo2Vst *)effect->object;
		if (that) return that->GetParameter(index);
	}
	return 0;
}


void CDmo2Vst::DmoProcess(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleframes)
//------------------------------------------------------------------------------------------------
{
	if (effect)
	{
		CDmo2Vst *that = (CDmo2Vst *)effect->object;
		if (that) that->Process(inputs, outputs, sampleframes);
	}
}


VstIntPtr CDmo2Vst::Dispatcher(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
//----------------------------------------------------------------------------------------------------
{
	switch(opCode)
	{
	case effOpen:
		break;

	case effClose:
		m_Effect.object = nullptr;
		delete this;
		return 0;

	case effGetParamName:
	case effGetParamLabel:
	case effGetParamDisplay:
		if ((index < m_Effect.numParams) && (m_pParamInfo))
		{
			MP_PARAMINFO mpi;
			char *pszName = (char *)ptr;

			mpi.mpType = MPT_INT;
			mpi.szUnitText[0] = 0;
			mpi.szLabel[0] = 0;
			pszName[0] = 0;
			if (m_pParamInfo->GetParamInfo(index, &mpi) == S_OK)
			{
				if (opCode == effGetParamDisplay)
				{
					MP_DATA md;

					if ((m_pMediaParams) && (m_pMediaParams->GetParam(index, &md) == S_OK))
					{
						switch(mpi.mpType)
						{
						case MPT_FLOAT:
							{
								int nValue = (int)(md * 100.0f + 0.5f);
								bool bNeg = false;
								if (nValue < 0) { bNeg = true; nValue = -nValue; }
								wsprintfA(pszName, (bNeg) ? "-%d.%02d" : "%d.%02d", nValue/100, nValue%100);
							}
							break;

						case MPT_BOOL:
							strcpy(pszName, ((int)md) ? "Yes" : "No");
							break;

						case MPT_ENUM:
							{
								WCHAR *text = nullptr;
								m_pParamInfo->GetParamText(index, &text);

								const int nValue = (int)(md * (mpi.mpdMaxValue - mpi.mpdMinValue) + 0.5f);
								// Always skip first two strings (param name, unit name)
								for(int i = 0; i < nValue + 2; i++)
								{
									text += wcslen(text) + 1;
								}
								WideCharToMultiByte(CP_ACP, 0, text, -1, pszName, 32, nullptr, nullptr);
							}
							break;

						case MPT_INT:
						default:
							wsprintfA(pszName, "%d", (int)md);
							break;
						}
					}
				} else
				{
					WideCharToMultiByte(CP_ACP, 0, (opCode == effGetParamName) ? mpi.szLabel : mpi.szUnitText, -1, pszName, 32, nullptr, nullptr);
				}
			}
		}
		break;

	case effCanBeAutomated:
		return (index < m_Effect.numParams);

	case effSetSampleRate:
		m_nSamplesPerSec = (int)opt;
		{

			m_Effect.initialDelay = 0;
			REFERENCE_TIME time;	// Unit 100-nanoseconds
			if(m_pMediaProcess->GetLatency(&time) == S_OK)
			{
				m_Effect.initialDelay = static_cast<VstInt32>(time * m_nSamplesPerSec / (10 * 1000 * 1000));
			}
		}
		break;

	case effMainsChanged:
		if (value)
		{
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
			if (FAILED(m_pMediaObject->SetInputType(0, &mt, 0))
				|| FAILED(m_pMediaObject->SetOutputType(0, &mt, 0)))
			{
#ifdef DMO_LOG
				Log(MPT_USTRING("DMO: Failed to set I/O media type"));
#endif
				return -1;
			}
		} else
		{
			m_pMediaObject->Flush();
			m_pMediaObject->SetInputType(0, nullptr, DMO_SET_TYPEF_CLEAR);
			m_pMediaObject->SetOutputType(0, nullptr, DMO_SET_TYPEF_CLEAR);
			m_DataTime = 0;
		}
		break;
	}
	return 0;
}


void CDmo2Vst::SetParameter(VstInt32 index, float fValue)
//-------------------------------------------------------
{
	MP_PARAMINFO mpi;

	if ((index < m_Effect.numParams) && (m_pParamInfo) && (m_pMediaParams))
	{
		MemsetZero(mpi);
		if (m_pParamInfo->GetParamInfo(index, &mpi) == S_OK)
		{
			float fMin = mpi.mpdMinValue;
			float fMax = mpi.mpdMaxValue;

			if (mpi.mpType == MPT_BOOL)
			{
				fMin = 0;
				fMax = 1;
				fValue = (fValue > 0.5f) ? 1.0f : 0.0f;
			}
			if (fMax > fMin) fValue *= (fMax - fMin);
			fValue += fMin;
			if (fValue < fMin) fValue = fMin;
			if (fValue > fMax) fValue = fMax;
			if (mpi.mpType != MPT_FLOAT) fValue = (float)(int)fValue;
			m_pMediaParams->SetParam(index, fValue);
		}
	}
}


float CDmo2Vst::GetParameter(VstInt32 index)
//------------------------------------------
{
	if ((index < m_Effect.numParams) && (m_pParamInfo) && (m_pMediaParams))
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


static const float _f2si = 32768.0f;
static const float _si2f = 1.0f / 32768.0f;

// Interleave two float streams into one int16 stereo stream.
void CDmo2Vst::InterleaveFloatToInt16(const float *inLeft, const float *inRight, int samples)
//-------------------------------------------------------------------------------------------
{
	int16 *outBuf = m_pMixBuffer;
	for(int i = samples; i != 0; i--)
	{
		*(outBuf++) = static_cast<int16>(Clamp(*(inLeft++) * _f2si, static_cast<float>(int16_min), static_cast<float>(int16_max)));
		*(outBuf++) = static_cast<int16>(Clamp(*(inRight++) * _f2si, static_cast<float>(int16_min), static_cast<float>(int16_max)));
	}
}


// Deinterleave an int16 stereo stream into two float streams.
void CDmo2Vst::DeinterleaveInt16ToFloat(float *outLeft, float *outRight, int samples) const
//-----------------------------------------------------------------------------------------
{
	const int16 *inBuf = m_pMixBuffer;
	for(int i = samples; i != 0; i--)
	{
		*outLeft++ += _si2f * static_cast<float>(*inBuf++);
		*outRight++ += _si2f * static_cast<float>(*inBuf++);
	}
}


#ifdef ENABLE_SSE
// Interleave two float streams into one int16 stereo stream using SSE magic.
void CDmo2Vst::SSEInterleaveFloatToInt16(const float *inLeft, const float *inRight, int samples)
//----------------------------------------------------------------------------------------------
{
	int16 *outBuf = m_pMixBuffer;
	_asm
	{
		mov eax, inLeft
			mov edx, inRight
			mov ebx, outBuf
			mov ecx, samples
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
void CDmo2Vst::SSEDeinterleaveInt16ToFloat(float *outLeft, float *outRight, int samples) const
//--------------------------------------------------------------------------------------------
{
	const int16 *inBuf = m_pMixBuffer;
	_asm {
		mov ebx, inBuf
			mov eax, outLeft
			mov edx, outRight
			mov ecx, samples
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


void CDmo2Vst::Process(float * const *inputs, float **outputs, int samples)
//-------------------------------------------------------------------------
{
	if(m_pMixBuffer == nullptr || samples <= 0)
	{
		return;
	}

#ifdef ENABLE_SSE
	if(GetProcSupport() & PROCSUPPORT_SSE)
	{
		SSEInterleaveFloatToInt16(inputs[0], inputs[1], samples);
		m_pMediaProcess->Process(samples * 2 * sizeof(int16), reinterpret_cast<BYTE *>(m_pMixBuffer), m_DataTime, DMO_INPLACE_NORMAL);
		SSEDeinterleaveInt16ToFloat(outputs[0], outputs[1], samples);
	} else
#endif // ENABLE_SSE
	{
		InterleaveFloatToInt16(inputs[0], inputs[1], samples);
		m_pMediaProcess->Process(samples * 2 * sizeof(int16), reinterpret_cast<BYTE *>(m_pMixBuffer), m_DataTime, DMO_INPLACE_NORMAL);
		DeinterleaveInt16ToFloat(outputs[0], outputs[1], samples);
	}

	m_DataTime += Util::muldiv(samples, 10000000, m_nSamplesPerSec);
}


// The actual adapter function.
AEffect *DmoToVst(VSTPluginLib &lib)
//----------------------------------
{
	CLSID clsid;
	if (Util::VerifyStringToCLSID(lib.dllPath.ToWide(), clsid))
	{
		IMediaObject *pMO = nullptr;
		IMediaObjectInPlace *pMOIP = nullptr;
		if ((CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_IMediaObject, (VOID **)&pMO) == S_OK) && (pMO))
		{
			if (pMO->QueryInterface(IID_IMediaObjectInPlace, (void **)&pMOIP) != S_OK) pMOIP = nullptr;
		} else pMO = nullptr;
		if ((pMO) && (pMOIP))
		{
			DWORD dwInputs, dwOutputs;

			dwInputs = dwOutputs = 0;
			pMO->GetStreamCount(&dwInputs, &dwOutputs);
			if (dwInputs == 1 && dwOutputs == 1)
			{
				CDmo2Vst *p = new CDmo2Vst(pMO, pMOIP, clsid.Data1);
				return (p) ? p->GetEffect() : nullptr;
			}
#ifdef DMO_LOG
			Log(lib.libraryName.ToUnicode() + MPT_USTRING(": Unable to use this DMO"));
#endif
		}
#ifdef DMO_LOG
		else Log(lib.libraryName.ToUnicode() + MPT_USTRING(": Failed to get IMediaObject & IMediaObjectInPlace interfaces"));
#endif
		if (pMO) pMO->Release();
		if (pMOIP) pMOIP->Release();
	}
	return nullptr;
}

#endif // NO_DMO


OPENMPT_NAMESPACE_END
