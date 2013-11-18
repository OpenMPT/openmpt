/*
 * SoundDeviceASIO.cpp
 * -------------------
 * Purpose: ASIO sound device driver class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDevice.h"
#include "SoundDevices.h"

#include "SoundDeviceASIO.h"

#include "../common/misc_util.h"
#include "../common/StringFixer.h"
#include "../soundlib/SampleFormatConverters.h"

// DEBUG:
#include "../common/AudioCriticalSection.h"

#include <algorithm>


// Helper class to temporarily open a device for a query.
class TemporaryASIODeviceOpener
{
protected:
	CASIODevice &device;
	const bool wasOpen;

public:
	TemporaryASIODeviceOpener(CASIODevice &d) : device(d), wasOpen(d.IsOpen())
	{
		if(!wasOpen)
		{
			device.OpenDevice();
		}
	}

	~TemporaryASIODeviceOpener()
	{
		if(!wasOpen)
		{
			device.CloseDevice();
		}
	}
};


///////////////////////////////////////////////////////////////////////////////////////
//
// ASIO Device implementation
//

#ifndef NO_ASIO

#define ASIO_MAXDRVNAMELEN	1024

CASIODevice *CASIODevice::gpCurrentAsio = nullptr;

static DWORD g_dwBuffer = 0;

static int g_asio_startcount = 0;


std::vector<SoundDeviceInfo> CASIODevice::EnumerateDevices()
//----------------------------------------------------------
{
	std::vector<SoundDeviceInfo> devices;

	LONG cr;

	HKEY hkEnum = NULL;
	cr = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\ASIO", &hkEnum);

	for(DWORD index = 0; ; ++index)
	{

		WCHAR keynameBuf[ASIO_MAXDRVNAMELEN];
		if((cr = RegEnumKeyW(hkEnum, index, keynameBuf, ASIO_MAXDRVNAMELEN)) != ERROR_SUCCESS)
		{
			break;
		}
		const std::wstring keyname = keynameBuf;
		#ifdef ASIO_LOG
			Log("ASIO: Found \"%s\":\n", mpt::ToLocale(keyname).c_str());
		#endif

		HKEY hksub = NULL;
		if(RegOpenKeyExW(hkEnum, keynameBuf, 0, KEY_READ, &hksub) != ERROR_SUCCESS)
		{
			continue;
		}

		WCHAR descriptionBuf[ASIO_MAXDRVNAMELEN];
		DWORD datatype = REG_SZ;
		DWORD datasize = sizeof(descriptionBuf);
		std::wstring description;
		if(ERROR_SUCCESS == RegQueryValueExW(hksub, L"Description", 0, &datatype, (LPBYTE)descriptionBuf, &datasize))
		{
		#ifdef ASIO_LOG
			Log("  description =\"%s\":\n", mpt::ToLocale(description).c_str());
		#endif
			description = descriptionBuf;
		} else
		{
			description = keyname;
		}

		WCHAR idBuf[256];
		datatype = REG_SZ;
		datasize = sizeof(idBuf);
		if(ERROR_SUCCESS == RegQueryValueExW(hksub, L"CLSID", 0, &datatype, (LPBYTE)idBuf, &datasize))
		{
			const std::wstring internalID = idBuf;
			if(Util::IsCLSID(internalID))
			{
				#ifdef ASIO_LOG
					Log("  clsid=\"%s\"\n", mpt::ToLocale(internalID).c_str());
				#endif

				if(SoundDeviceIndexIsValid(devices.size()))
				{
					// everything ok
					devices.push_back(SoundDeviceInfo(SoundDeviceID(SNDDEV_ASIO, static_cast<SoundDeviceIndex>(devices.size())), description, L"ASIO", internalID));
				}

			}
		}

		RegCloseKey(hksub);
		hksub = NULL;

	}

	if(hkEnum)
	{
		RegCloseKey(hkEnum);
		hkEnum = NULL;
	}

	return devices;
}


CASIODevice::CASIODevice(SoundDeviceID id, const std::wstring &internalID)
//------------------------------------------------------------------------
	: ISoundDevice(id, internalID)
{
	m_pAsioDrv = NULL;
	m_nAsioBufferLen = 0;
	m_Callbacks.bufferSwitch = BufferSwitch;
	m_Callbacks.sampleRateDidChange = SampleRateDidChange;
	m_Callbacks.asioMessage = AsioMessage;
	m_Callbacks.bufferSwitchTimeInfo = BufferSwitchTimeInfo;
	m_bMixRunning = FALSE;
	InterlockedExchange(&m_RenderSilence, 0);
	InterlockedExchange(&m_RenderingSilence, 0);
}


CASIODevice::~CASIODevice()
//-------------------------
{
	Close();
}


bool CASIODevice::InternalOpen()
//------------------------------
{

	bool bOk = false;

#ifdef ASIO_LOG
	Log("CASIODevice::Open(%d:\"%s\"): %d-bit, %d channels, %dHz\n",
		GetDeviceIndex(), mpt::ToLocale(GetDeviceInternalID()).c_str(), (int)m_Settings.sampleFormat.GetBitsPerSample(), m_Settings.Channels, m_Settings.Samplerate);
#endif
	OpenDevice();

	if(IsDeviceOpen())
	{
		long nInputChannels = 0, nOutputChannels = 0;
		long minSize = 0, maxSize = 0, preferredSize = 0, granularity = 0;

		if(m_Settings.Channels > ASIO_MAX_CHANNELS)
		{
			goto abort;
		}
		m_pAsioDrv->getChannels(&nInputChannels, &nOutputChannels);
	#ifdef ASIO_LOG
		Log("  getChannels: %d inputs, %d outputs\n", nInputChannels, nOutputChannels);
	#endif
		if (m_Settings.Channels > nOutputChannels) goto abort;
		if (m_pAsioDrv->setSampleRate(m_Settings.Samplerate) != ASE_OK)
		{
		#ifdef ASIO_LOG
			Log("  setSampleRate(%d) failed (sample rate not supported)!\n", m_Settings.Samplerate);
		#endif
			goto abort;
		}
		m_pAsioDrv->getBufferSize(&minSize, &maxSize, &preferredSize, &granularity);
	#ifdef ASIO_LOG
		Log("  getBufferSize(): minSize=%d maxSize=%d preferredSize=%d granularity=%d\n",
				minSize, maxSize, preferredSize, granularity);
	#endif
		m_nAsioBufferLen = ((m_Settings.LatencyMS * m_Settings.Samplerate) / 2000);
		if(minSize <= 0 || maxSize <= 0 || minSize > maxSize)
		{ // limits make no sense
			if(preferredSize > 0)
			{
				m_nAsioBufferLen = preferredSize;
			} else
			{
				// just leave the user value, perhaps it works
			}
		} else if(granularity < -1)
		{ // granularity value not allowed, just clamp value
			m_nAsioBufferLen = Clamp(m_nAsioBufferLen, minSize, maxSize);
		} else if(granularity == -1 && (Util::Weight(minSize) != 1 || Util::Weight(maxSize) != 1))
		{ // granularity tells us we need power-of-2 sizes, but min or max sizes are no power-of-2
			m_nAsioBufferLen = Clamp(m_nAsioBufferLen, minSize, maxSize);
			// just start at 1 and find a matching power-of-2 in range
			const long bufTarget = m_nAsioBufferLen;
			for(long bufSize = 1; bufSize <= maxSize && bufSize <= bufTarget; bufSize *= 2)
			{
				if(bufSize >= minSize)
				{
					m_nAsioBufferLen = bufSize;
				}
			}
			// if no power-of-2 in range is found, just leave the clamped value alone, perhaps it works
		} else if(granularity == -1)
		{ // sane values, power-of-2 size required between min and max
			m_nAsioBufferLen = Clamp(m_nAsioBufferLen, minSize, maxSize);
			// get the largest allowed buffer size that is smaller or equal to the target size
			const long bufTarget = m_nAsioBufferLen;
			for(long bufSize = minSize; bufSize <= maxSize && bufSize <= bufTarget; bufSize *= 2)
			{
				m_nAsioBufferLen = bufSize;
			}
		} else if(granularity > 0)
		{ // buffer size in granularity steps from min to max allowed
			m_nAsioBufferLen = Clamp(m_nAsioBufferLen, minSize, maxSize);
			// get the largest allowed buffer size that is smaller or equal to the target size
			const long bufTarget = m_nAsioBufferLen;
			for(long bufSize = minSize; bufSize <= maxSize && bufSize <= bufTarget; bufSize += granularity)
			{
				m_nAsioBufferLen = bufSize;
			}
		} else
		{ // should not happen
			ASSERT(false);
		}
	#ifdef ASIO_LOG
		Log("  Using buffersize=%d samples\n", m_nAsioBufferLen);
	#endif
		for(int channel = 0; channel < m_Settings.Channels; ++channel)
		{
			MemsetZero(m_BufferInfo[channel]);
			m_BufferInfo[channel].isInput = ASIOFalse;
			m_BufferInfo[channel].channelNum = channel + m_Settings.BaseChannel; // map MPT channel i to ASIO channel i
		}
		if(m_pAsioDrv->createBuffers(m_BufferInfo, m_Settings.Channels, m_nAsioBufferLen, &m_Callbacks) == ASE_OK)
		{
			bool m_AllFloat = true;
			for(int channel = 0; channel < m_Settings.Channels; ++channel)
			{
				MemsetZero(m_ChannelInfo[channel]);
				m_ChannelInfo[channel].isInput = ASIOFalse;
				m_ChannelInfo[channel].channel = channel + m_Settings.BaseChannel; // map MPT channel i to ASIO channel i
				m_pAsioDrv->getChannelInfo(&m_ChannelInfo[channel]);
				ASSERT(m_ChannelInfo[channel].isActive);
				#ifdef ASIO_LOG
					Log("  getChannelInfo(%d): isActive=%d channelGroup=%d type=%d name=\"%s\"\n",
						channel, m_ChannelInfo[channel].isActive, m_ChannelInfo[channel].channelGroup, m_ChannelInfo[channel].type, m_ChannelInfo[channel].name);
				#endif
				if(!IsSampleTypeFloat(m_ChannelInfo[channel].type))
				{
					m_AllFloat = false;
				}
			}
			m_Settings.sampleFormat = m_AllFloat ? SampleFormatFloat32 : SampleFormatInt32;

			for(int channel = 0; channel < m_Settings.Channels; ++channel)
			{
				if(m_BufferInfo[channel].buffers[0])
				{
					std::memset(m_BufferInfo[channel].buffers[0], 0, m_nAsioBufferLen * GetSampleSize(m_ChannelInfo[channel].type));
				}
				if(m_BufferInfo[channel].buffers[1])
				{
					std::memset(m_BufferInfo[channel].buffers[1], 0, m_nAsioBufferLen * GetSampleSize(m_ChannelInfo[channel].type));
				}
			}

			m_CanOutputReady = (m_pAsioDrv->outputReady() == ASE_OK);

			m_SampleBuffer.resize(m_nAsioBufferLen * m_Settings.Channels);

			SoundBufferAttributes bufferAttributes;
			long inputLatency = 0;
			long outputLatency = 0;
			if(m_pAsioDrv->getLatencies(&inputLatency, &outputLatency) != ASE_OK)
			{
				inputLatency = 0;
				outputLatency = 0;
			}
			if(outputLatency >= (long)m_nAsioBufferLen)
			{
				bufferAttributes.Latency = (double)(outputLatency + m_nAsioBufferLen) / (double)m_Settings.Samplerate; // ASIO and OpenMPT semantics of 'latency' differ by one chunk/buffer
			} else
			{
				// pointless value returned from asio driver, use a sane estimate
				bufferAttributes.Latency = 2.0 * (double)m_nAsioBufferLen / (double)m_Settings.Samplerate;
			}
			bufferAttributes.UpdateInterval = (double)m_nAsioBufferLen / (double)m_Settings.Samplerate;
			bufferAttributes.NumBuffers = 2;
			UpdateBufferAttributes(bufferAttributes);

			bOk = true;
		}
	#ifdef ASIO_LOG
		else Log("  createBuffers failed!\n");
	#endif
	}
abort:
	if (bOk)
	{
		gpCurrentAsio = this;
	} else
	{
	#ifdef ASIO_LOG
		Log("Error opening ASIO device!\n");
	#endif
		CloseDevice();
	}
	return bOk;
}


void CASIODevice::SetRenderSilence(bool silence, bool wait)
//---------------------------------------------------------
{
	InterlockedExchange(&m_RenderSilence, silence?1:0);
	if(!wait)
	{
		return;
	}
	DWORD pollingstart = GetTickCount();
	while(InterlockedExchangeAdd(&m_RenderingSilence, 0) != (silence?1:0))
	{
		if(GetTickCount() - pollingstart > 250)
		{
			if(silence)
			{
				if(CriticalSection::IsLocked())
				{
					ASSERT_WARN_MESSAGE(false, "AudioCriticalSection locked while stopping ASIO");
				} else
				{
					ASSERT_WARN_MESSAGE(false, "waiting for asio failed in Stop()");
				}
			} else
			{
				if(CriticalSection::IsLocked())
				{
					ASSERT_WARN_MESSAGE(false, "AudioCriticalSection locked while starting ASIO");
				} else
				{
					ASSERT_WARN_MESSAGE(false, "waiting for asio failed in Start()");
				}
			}
			break;
		}
		Sleep(1);
	}
}


void CASIODevice::InternalStart()
//-------------------------------
{
	ALWAYS_ASSERT_WARN_MESSAGE(!CriticalSection::IsLocked(), "AudioCriticalSection locked while starting ASIO");

		ALWAYS_ASSERT(g_asio_startcount==0);
		g_asio_startcount++;

		if(!m_bMixRunning)
		{
			SetRenderSilence(false);
			m_bMixRunning = TRUE;
			try
			{
				m_pAsioDrv->start();
			} catch(...)
			{
				CASIODevice::ReportASIOException("ASIO crash in start()\n");
				m_bMixRunning = FALSE;
			}
		} else
		{
			SetRenderSilence(false, true);
		}
}


void CASIODevice::InternalStop()
//------------------------------
{
	ALWAYS_ASSERT(g_asio_startcount==1);
	ALWAYS_ASSERT_WARN_MESSAGE(!CriticalSection::IsLocked(), "AudioCriticalSection locked while stopping ASIO");

		SetRenderSilence(true, true);
		g_asio_startcount--;
		ALWAYS_ASSERT(g_asio_startcount==0);
}


bool CASIODevice::InternalClose()
//-------------------------------
{
	if (m_bMixRunning)
	{
		m_bMixRunning = FALSE;
		ALWAYS_ASSERT(g_asio_startcount==0);
		try
		{
			m_pAsioDrv->stop();
		} catch(...)
		{
			CASIODevice::ReportASIOException("ASIO crash in stop()\n");
		}
	}
	g_asio_startcount = 0;
	SetRenderSilence(false);
	try
	{
		m_pAsioDrv->disposeBuffers();
	} catch(...)
	{
		CASIODevice::ReportASIOException("ASIO crash in disposeBuffers()\n");
	}
	CloseDevice();
	m_SampleBuffer.clear();
	if(gpCurrentAsio == this)
	{
		gpCurrentAsio = nullptr;
	}
	return true;
}


void CASIODevice::OpenDevice()
//----------------------------
{
	if(IsDeviceOpen())
	{
		return;
	}

	CLSID clsid;
	Util::StringToCLSID(GetDeviceInternalID(), clsid);
	if (CoCreateInstance(clsid,0,CLSCTX_INPROC_SERVER, clsid, (void **)&m_pAsioDrv) == S_OK)
	{
		m_pAsioDrv->init((void *)m_Settings.hWnd);
	} else
	{
#ifdef ASIO_LOG
		Log("  CoCreateInstance failed!\n");
#endif
		m_pAsioDrv = NULL;
	}
}


void CASIODevice::CloseDevice()
//-----------------------------
{
	if(IsDeviceOpen())
	{
		try
		{
			m_pAsioDrv->Release();
		} catch(...)
		{
			CASIODevice::ReportASIOException("ASIO crash in Release()\n");
		}
		m_pAsioDrv = NULL;
	}
}


static void SwapEndian(uint8 *buf, std::size_t itemCount, std::size_t itemSize)
//-----------------------------------------------------------------------------
{
	for(std::size_t i = 0; i < itemCount; ++i)
	{
		std::reverse(buf, buf + itemSize);
		buf += itemSize;
	}
}


bool CASIODevice::IsSampleTypeFloat(ASIOSampleType sampleType)
//------------------------------------------------------------
{
	switch(sampleType)
	{
		case ASIOSTFloat32MSB:
		case ASIOSTFloat32LSB:
		case ASIOSTFloat64MSB:
		case ASIOSTFloat64LSB:
			return true;
			break;
		default:
			return false;
			break;
	}
}


std::size_t CASIODevice::GetSampleSize(ASIOSampleType sampleType)
//---------------------------------------------------------------
{
	switch(sampleType)
	{
		case ASIOSTInt16MSB:
		case ASIOSTInt16LSB:
			return 2;
			break;
		case ASIOSTInt24MSB:
		case ASIOSTInt24LSB:
			return 3;
			break;
		case ASIOSTInt32MSB:
		case ASIOSTInt32LSB:
			return 4;
			break;
		case ASIOSTInt32MSB16:
		case ASIOSTInt32MSB18:
		case ASIOSTInt32MSB20:
		case ASIOSTInt32MSB24:
		case ASIOSTInt32LSB16:
		case ASIOSTInt32LSB18:
		case ASIOSTInt32LSB20:
		case ASIOSTInt32LSB24:
			return 4;
			break;
		case ASIOSTFloat32MSB:
		case ASIOSTFloat32LSB:
			return 4;
			break;
		case ASIOSTFloat64MSB:
		case ASIOSTFloat64LSB:
			return 8;
			break;
		default:
			return 0;
			break;
	}
}


void CASIODevice::FillAudioBuffer()
//---------------------------------
{
	const bool rendersilence = (InterlockedExchangeAdd(&m_RenderSilence, 0) == 1);
	const int channels = m_Settings.Channels;
	const std::size_t countChunk = m_nAsioBufferLen;
	g_dwBuffer &= 1;
	//Log("FillAudioBuffer(%d): channels=%d countChunk=%d\n", g_dwBuffer, sampleFrameSize, (int)channels, (int)countChunk);
	if(rendersilence)
	{
		std::memset(&m_SampleBuffer[0], 0, countChunk * channels * sizeof(int32));
	} else
	{
		SourceAudioRead(&m_SampleBuffer[0], countChunk);
	}
	for(int channel = 0; channel < channels; ++channel)
	{
		void *dst = m_BufferInfo[channel].buffers[g_dwBuffer];
		if(m_Settings.sampleFormat == SampleFormatFloat32)
		{
			const float *const srcFloat = reinterpret_cast<const float*>(&m_SampleBuffer[0]);
			switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTFloat32MSB:
				case ASIOSTFloat32LSB:
					CopyInterleavedToChannel<SC::Convert<float, float> >(reinterpret_cast<float*>(dst), srcFloat, channels, countChunk, channel);
					break;
				case ASIOSTFloat64MSB:
				case ASIOSTFloat64LSB:
					CopyInterleavedToChannel<SC::Convert<double, float> >(reinterpret_cast<double*>(dst), srcFloat, channels, countChunk, channel);
					break;
				default:
					ASSERT(false);
					break;
			}
		} else if(m_Settings.sampleFormat == SampleFormatInt32)
		{
			const int32 *const srcInt32 = reinterpret_cast<const int32*>(&m_SampleBuffer[0]);
			switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTInt16MSB:
				case ASIOSTInt16LSB:
					CopyInterleavedToChannel<SC::Convert<int16, int32> >(reinterpret_cast<int16*>(dst), srcInt32, channels, countChunk, channel);
					break;
				case ASIOSTInt24MSB:
				case ASIOSTInt24LSB:
					CopyInterleavedToChannel<SC::Convert<int24, int32> >(reinterpret_cast<int24*>(dst), srcInt32, channels, countChunk, channel);
					break;
				case ASIOSTInt32MSB:
				case ASIOSTInt32LSB:
					CopyInterleavedToChannel<SC::Convert<int32, int32> >(reinterpret_cast<int32*>(dst), srcInt32, channels, countChunk, channel);
					break;
				case ASIOSTFloat32MSB:
				case ASIOSTFloat32LSB:
					CopyInterleavedToChannel<SC::Convert<float, int32> >(reinterpret_cast<float*>(dst), srcInt32, channels, countChunk, channel);
					break;
				case ASIOSTFloat64MSB:
				case ASIOSTFloat64LSB:
					CopyInterleavedToChannel<SC::Convert<double, int32> >(reinterpret_cast<double*>(dst), srcInt32, channels, countChunk, channel);
					break;
				case ASIOSTInt32MSB16:
				case ASIOSTInt32LSB16:
					CopyInterleavedToChannel<SC::ConvertShift<int32, int32, 16> >(reinterpret_cast<int32*>(dst), srcInt32, channels, countChunk, channel);
					break;
				case ASIOSTInt32MSB18:
				case ASIOSTInt32LSB18:
					CopyInterleavedToChannel<SC::ConvertShift<int32, int32, 14> >(reinterpret_cast<int32*>(dst), srcInt32, channels, countChunk, channel);
					break;
				case ASIOSTInt32MSB20:
				case ASIOSTInt32LSB20:
					CopyInterleavedToChannel<SC::ConvertShift<int32, int32, 12> >(reinterpret_cast<int32*>(dst), srcInt32, channels, countChunk, channel);
					break;
				case ASIOSTInt32MSB24:
				case ASIOSTInt32LSB24:
					CopyInterleavedToChannel<SC::ConvertShift<int32, int32, 8> >(reinterpret_cast<int32*>(dst), srcInt32, channels, countChunk, channel);
					break;
				default:
					ASSERT(false);
					break;
			}
		}
		switch(m_ChannelInfo[channel].type)
		{
			case ASIOSTInt16MSB:
			case ASIOSTInt24MSB:
			case ASIOSTInt32MSB:
			case ASIOSTFloat32MSB:
			case ASIOSTFloat64MSB:
			case ASIOSTInt32MSB16:
			case ASIOSTInt32MSB18:
			case ASIOSTInt32MSB20:
			case ASIOSTInt32MSB24:
				SwapEndian(reinterpret_cast<uint8*>(dst), countChunk, GetSampleSize(m_ChannelInfo[channel].type));
				break;
		}
	}
	if(m_CanOutputReady)
	{
		m_pAsioDrv->outputReady();
	}
	if(!rendersilence)
	{
		SourceAudioDone(countChunk, m_nAsioBufferLen);
	}
}


void CASIODevice::BufferSwitch(long doubleBufferIndex)
//----------------------------------------------------
{
	g_dwBuffer = doubleBufferIndex;
	bool rendersilence = (InterlockedExchangeAdd(&m_RenderSilence, 0) == 1);
	InterlockedExchange(&m_RenderingSilence, rendersilence ? 1 : 0 );
	if(rendersilence)
	{
		FillAudioBuffer();
	} else
	{
		SourceFillAudioBufferLocked();
	}
}


void CASIODevice::BufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
//----------------------------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(directProcess);
	if(gpCurrentAsio)
	{
		gpCurrentAsio->BufferSwitch(doubleBufferIndex);
	} else
	{
		ALWAYS_ASSERT(false && "gpCurrentAsio");
	}
}


void CASIODevice::SampleRateDidChange(ASIOSampleRate sRate)
//---------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(sRate);
}


long CASIODevice::AsioMessage(long selector, long value, void* message, double* opt)
//----------------------------------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(value);
	MPT_UNREFERENCED_PARAMETER(message);
	MPT_UNREFERENCED_PARAMETER(opt);
#ifdef ASIO_LOG
	// Log("AsioMessage(%d, %d)\n", selector, value);
#endif
	switch(selector)
	{
	case kAsioEngineVersion: return 2;
	}
	return 0;
}


ASIOTime* CASIODevice::BufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
//-----------------------------------------------------------------------------------------------------------
{
	BufferSwitch(doubleBufferIndex, directProcess);
	return params;
}


void CASIODevice::ReportASIOException(const std::string &str)
//-----------------------------------------------------------
{
	AudioSendMessage(str);
	Log("%s", str.c_str());
}


SoundDeviceCaps CASIODevice::GetDeviceCaps(const std::vector<uint32> &baseSampleRates)
//------------------------------------------------------------------------------------
{
	SoundDeviceCaps caps;

	TemporaryASIODeviceOpener opener(*this);
	if(!IsOpen())
	{
		return caps;
	}

	ASIOSampleRate samplerate;
	if(m_pAsioDrv->getSampleRate(&samplerate) != ASE_OK)
	{
		samplerate = 0;
	}
	caps.currentSampleRate = Util::Round<uint32>(samplerate);

	for(size_t i = 0; i < baseSampleRates.size(); i++)
	{
		if(m_pAsioDrv->canSampleRate((ASIOSampleRate)baseSampleRates[i]) == ASE_OK)
		{
			caps.supportedSampleRates.push_back(baseSampleRates[i]);
		}
	}
	long inputChannels = 0;
	long outputChannels = 0;
	if(m_pAsioDrv->getChannels(&inputChannels, &outputChannels) == ASE_OK)
	{
		for(long i = 0; i < outputChannels; ++i)
		{
			ASIOChannelInfo channelInfo;
			MemsetZero(channelInfo);
			channelInfo.channel = i;
			channelInfo.isInput = ASIOFalse;
			if(m_pAsioDrv->getChannelInfo(&channelInfo) == ASE_OK)
			{
				mpt::String::SetNullTerminator(channelInfo.name);
				caps.channelNames.push_back(mpt::ToWide(mpt::CharsetLocale, channelInfo.name));
			} else
			{
				caps.channelNames.push_back(mpt::ToWString(i));
			}
		}
	}

	return caps;
}


bool CASIODevice::OpenDriverSettings()
//------------------------------------
{
	TemporaryASIODeviceOpener opener(*this);
	if(!IsOpen())
	{
		return false;
	}
	return m_pAsioDrv->controlPanel() == ASE_OK;
}


#endif // NO_ASIO
