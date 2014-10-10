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

#include "SoundDeviceASIO.h"

#include "../common/misc_util.h"
#include "../common/StringFixer.h"
#include "../soundlib/SampleFormatConverters.h"

// DEBUG:
#include "../common/AudioCriticalSection.h"

#include <algorithm>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


#ifndef NO_ASIO


static const double AsioSampleRateTolerance = 0.05;


// Helper class to temporarily open a driver for a query.
class TemporaryASIODriverOpener
{
protected:
	CASIODevice &device;
	const bool wasOpen;

public:
	TemporaryASIODriverOpener(CASIODevice &d) : device(d), wasOpen(d.IsDriverOpen())
	{
		if(!wasOpen)
		{
			device.OpenDriver();
		}
	}

	~TemporaryASIODriverOpener()
	{
		if(!wasOpen)
		{
			device.CloseDriver();
		}
	}
};


class ASIOException
	: public std::runtime_error
{
public:
	ASIOException(const std::string &msg)
		: std::runtime_error(msg)
	{
		return;
	}
	ASIOException(const std::string &call, ASIOError err)
		: std::runtime_error(call + std::string(" failed: ") + mpt::ToString(err))
	{
		return;
	}
};


#define asioCall(asiocall) do { \
	try { \
		ASIOError e = m_pAsioDrv-> asiocall ; \
		if(e != ASE_OK) { \
			throw ASIOException( #asiocall , e); \
		} \
	} catch(const ASIOException &) { \
		throw; \
	} catch(...) { \
		CASIODevice::ReportASIOException( #asiocall + std::string(" crashed!")); \
		throw ASIOException(std::string("Exception in '") + #asiocall + std::string("'!")); \
	} \
} while(0)


#define asioCallUncatched(asiocall) m_pAsioDrv-> asiocall


///////////////////////////////////////////////////////////////////////////////////////
//
// ASIO Device implementation
//

#define ASIO_MAXDRVNAMELEN	1024

CASIODevice *CASIODevice::g_CallbacksInstance = nullptr;


std::vector<SoundDevice::Info> CASIODevice::EnumerateDevices()
//----------------------------------------------------------
{
	MPT_TRACE();
	std::vector<SoundDevice::Info> devices;

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
		Log(mpt::String::Print("ASIO: Found '%1':", mpt::ToLocale(keyname)));

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
			Log(mpt::String::Print("ASIO:   description='%1'", mpt::ToLocale(description)));
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
				Log(mpt::String::Print("ASIO:   clsid=%1", mpt::ToLocale(internalID)));
				if(SoundDevice::IndexIsValid(devices.size()))
				{
					// everything ok
					devices.push_back(SoundDevice::Info(SoundDevice::ID(TypeASIO, static_cast<SoundDevice::Index>(devices.size())), description, SoundDevice::TypeToString(TypeASIO), internalID));
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


CASIODevice::CASIODevice(SoundDevice::Info info)
//----------------------------------------------
	: SoundDevice::Base(info)
{
	MPT_TRACE();
	InitMembers();
	m_QueriedFeatures.reset();
	m_UsedFeatures.reset();
}


void CASIODevice::InitMembers()
//-----------------------------
{
	MPT_TRACE();
	m_pAsioDrv = nullptr;

	m_nAsioBufferLen = 0;
	m_BufferInfo.clear();
	MemsetZero(m_Callbacks);
	m_BuffersCreated = false;
	m_ChannelInfo.clear();
	m_SampleBufferFloat.clear();
	m_SampleBufferInt16.clear();
	m_SampleBufferInt24.clear();
	m_SampleBufferInt32.clear();
	m_CanOutputReady = false;

	m_DeviceRunning = false;
	m_TotalFramesWritten = 0;
	m_BufferIndex = 0;
	InterlockedExchange(&m_RenderSilence, 0);
	InterlockedExchange(&m_RenderingSilence, 0);

	InterlockedExchange(&m_AsioRequestFlags, 0);
}


bool CASIODevice::HandleRequests()
//--------------------------------
{
	MPT_TRACE();
	bool result = false;
	LONG flags = InterlockedExchange(&m_AsioRequestFlags, 0);
	if(flags & AsioRequestFlagLatenciesChanged)
	{
		UpdateLatency();
		result = true;
	}
	return result;
}


CASIODevice::~CASIODevice()
//-------------------------
{
	MPT_TRACE();
	Close();
}


bool CASIODevice::InternalOpen()
//------------------------------
{
	MPT_TRACE();

	ASSERT(!IsDriverOpen());

	InitMembers();

	Log(mpt::String::Print("ASIO: Open(%1:'%2'): %3-bit, %4 channels, %5Hz, hw-timing=%6"
		, GetDeviceIndex()
		, mpt::ToLocale(GetDeviceInternalID())
		, m_Settings.sampleFormat.GetBitsPerSample()
		, m_Settings.Channels
		, m_Settings.Samplerate
		, m_Settings.UseHardwareTiming
		));

	try
	{

		OpenDriver();

		if(!IsDriverOpen())
		{
			throw ASIOException("Initializing driver failed.");
		}

		long inputChannels = 0;
		long outputChannels = 0;
		asioCall(getChannels(&inputChannels, &outputChannels));
		Log(mpt::String::Print("ASIO: getChannels() => inputChannels=%1 outputChannel=%2", inputChannels, outputChannels));
		if(inputChannels <= 0 && outputChannels <= 0)
		{
			m_DeviceUnavailableOnOpen = true;
			throw ASIOException("Device unavailble.");
		}
		if(m_Settings.Channels > outputChannels)
		{
			throw ASIOException("Not enough output channels.");
		}
		if(m_Settings.ChannelMapping.GetRequiredDeviceChannels() > outputChannels)
		{
			throw ASIOException("Channel mapping requires more channels than available.");
		}

		Log(mpt::String::Print("ASIO: setSampleRate(sampleRate=%1)", m_Settings.Samplerate));
		asioCall(setSampleRate(m_Settings.Samplerate));

		long minSize = 0;
		long maxSize = 0;
		long preferredSize = 0;
		long granularity = 0;
		asioCall(getBufferSize(&minSize, &maxSize, &preferredSize, &granularity));
		Log(mpt::String::Print("ASIO: getBufferSize() => minSize=%1 maxSize=%2 preferredSize=%3 granularity=%4",
			minSize, maxSize, preferredSize, granularity));
		m_nAsioBufferLen = Util::Round<int32>(m_Settings.Latency * m_Settings.Samplerate / 2.0);
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
		} else if(granularity == 0)
		{ // no granularity given, we should use preferredSize if possible
			if(preferredSize > 0)
			{
				m_nAsioBufferLen = preferredSize;
			} else if(m_nAsioBufferLen >= maxSize)
			{ // a large latency was requested, use maxSize
				m_nAsioBufferLen = maxSize;
			} else
			{ // use minSize otherwise
				m_nAsioBufferLen = minSize;
			}
		} else
		{ // should not happen
			ASSERT(false);
		}
	
		m_BufferInfo.resize(m_Settings.Channels);
		for(int channel = 0; channel < m_Settings.Channels; ++channel)
		{
			MemsetZero(m_BufferInfo[channel]);
			m_BufferInfo[channel].isInput = ASIOFalse;
			m_BufferInfo[channel].channelNum = m_Settings.ChannelMapping.ToDevice(channel);
		}
		m_Callbacks.bufferSwitch = CallbackBufferSwitch;
		m_Callbacks.sampleRateDidChange = CallbackSampleRateDidChange;
		m_Callbacks.asioMessage = CallbackAsioMessage;
		m_Callbacks.bufferSwitchTimeInfo = CallbackBufferSwitchTimeInfo;
		ALWAYS_ASSERT(g_CallbacksInstance == nullptr);
		g_CallbacksInstance = this;
		Log(mpt::String::Print("ASIO: createBuffers(numChannels=%1, bufferSize=%2)", m_Settings.Channels, m_nAsioBufferLen));
		asioCall(createBuffers(&m_BufferInfo[0], m_Settings.Channels, m_nAsioBufferLen, &m_Callbacks));
		m_BuffersCreated = true;

		m_ChannelInfo.resize(m_Settings.Channels);
		for(int channel = 0; channel < m_Settings.Channels; ++channel)
		{
			MemsetZero(m_ChannelInfo[channel]);
			m_ChannelInfo[channel].isInput = ASIOFalse;
			m_ChannelInfo[channel].channel = m_Settings.ChannelMapping.ToDevice(channel);
			asioCall(getChannelInfo(&m_ChannelInfo[channel]));
			ASSERT(m_ChannelInfo[channel].isActive);
			mpt::String::SetNullTerminator(m_ChannelInfo[channel].name);
			Log(mpt::String::Print("ASIO: getChannelInfo(isInput=%1 channel=%2) => isActive=%3 channelGroup=%4 type=%5 name='%6'"
				, ASIOFalse
				, m_Settings.ChannelMapping.ToDevice(channel)
				, m_ChannelInfo[channel].isActive
				, m_ChannelInfo[channel].channelGroup
				, m_ChannelInfo[channel].type
				, m_ChannelInfo[channel].name
				));
		}

		bool allChannelsAreFloat = true;
		bool allChannelsAreInt16 = true;
		bool allChannelsAreInt24 = true;
		for(int channel = 0; channel < m_Settings.Channels; ++channel)
		{
			if(!IsSampleTypeFloat(m_ChannelInfo[channel].type))
			{
				allChannelsAreFloat = false;
			}
			if(!IsSampleTypeInt16(m_ChannelInfo[channel].type))
			{
				allChannelsAreInt16 = false;
			}
			if(!IsSampleTypeInt24(m_ChannelInfo[channel].type))
			{
				allChannelsAreInt24 = false;
			}
		}
		if(allChannelsAreFloat)
		{
			m_Settings.sampleFormat = SampleFormatFloat32;
			m_SampleBufferFloat.resize(m_nAsioBufferLen * m_Settings.Channels);
		} else if(allChannelsAreInt16)
		{
			m_Settings.sampleFormat = SampleFormatInt16;
			m_SampleBufferInt16.resize(m_nAsioBufferLen * m_Settings.Channels);
		} else if(allChannelsAreInt24)
		{
			m_Settings.sampleFormat = SampleFormatInt24;
			m_SampleBufferInt24.resize(m_nAsioBufferLen * m_Settings.Channels);
		} else
		{
			m_Settings.sampleFormat = SampleFormatInt32;
			m_SampleBufferInt32.resize(m_nAsioBufferLen * m_Settings.Channels);
		}

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

		m_CanOutputReady = false;
		try
		{
			m_CanOutputReady = (asioCallUncatched(outputReady()) == ASE_OK);
		} catch(...)
		{
			// continue, failure is not fatal here
			m_CanOutputReady = false;
		}

		m_StreamPositionOffset = m_nAsioBufferLen;

		UpdateLatency();

		return true;

	} catch(const std::exception &e)
	{
		Log(mpt::String::Print("ASIO: Error opening device: %1!", e.what() ? e.what() : ""));
	} catch(...)
	{
		Log("ASIO: Unknown error opening device!");
	}
	InternalClose();
	return false;
}


void CASIODevice::UpdateLatency()
//-------------------------------
{
	MPT_TRACE();
	SoundDevice::BufferAttributes bufferAttributes;
	long inputLatency = 0;
	long outputLatency = 0;
	try
	{
		asioCall(getLatencies(&inputLatency, &outputLatency));
	} catch(...)
	{
		// continue, failure is not fatal here
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
}


void CASIODevice::SetRenderSilence(bool silence, bool wait)
//---------------------------------------------------------
{
	MPT_TRACE();
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


bool CASIODevice::InternalStart()
//-------------------------------
{
	MPT_TRACE();
	ALWAYS_ASSERT_WARN_MESSAGE(!CriticalSection::IsLocked(), "AudioCriticalSection locked while starting ASIO");

	if(m_Settings.KeepDeviceRunning)
	{
		if(m_DeviceRunning)
		{
			SetRenderSilence(false, true);
			return true;
		}
	}

	SetRenderSilence(false);
	try
	{
		m_TotalFramesWritten = 0;
		asioCall(start());
		m_DeviceRunning = true;
	} catch(...)
	{
		return false;
	}

	return true;
}


void CASIODevice::InternalStopForce()
//-----------------------------------
{
	MPT_TRACE();
	InternalStopImpl(true);
}

void CASIODevice::InternalStop()
//------------------------------
{
	MPT_TRACE();
	InternalStopImpl(false);
}

void CASIODevice::InternalStopImpl(bool force)
//--------------------------------------------
{
	MPT_TRACE();
	ALWAYS_ASSERT_WARN_MESSAGE(!CriticalSection::IsLocked(), "AudioCriticalSection locked while stopping ASIO");

	if(m_Settings.KeepDeviceRunning && !force)
	{
		SetRenderSilence(true, true);
		return;
	}

	m_DeviceRunning = false;
	try
	{
		asioCall(stop());
	} catch(...)
	{
		// continue
	}
	m_TotalFramesWritten = 0;
	SetRenderSilence(false);

}


bool CASIODevice::InternalClose()
//-------------------------------
{
	MPT_TRACE();
	if(m_DeviceRunning)
	{
		m_DeviceRunning = false;
		try
		{
			asioCall(stop());
		} catch(...)
		{
			// continue
		}
		m_TotalFramesWritten = 0;
	}
	SetRenderSilence(false);

	m_CanOutputReady = false;
	m_SampleBufferFloat.clear();
	m_SampleBufferInt16.clear();
	m_SampleBufferInt24.clear();
	m_SampleBufferInt32.clear();
	m_ChannelInfo.clear();
	if(m_BuffersCreated)
	{
		try
		{
			asioCall(disposeBuffers());
		} catch(...)
		{
			// continue
		}
		m_BuffersCreated = false;
	}
	if(g_CallbacksInstance)
	{
		ALWAYS_ASSERT(g_CallbacksInstance == this);
		g_CallbacksInstance = nullptr;
	}
	MemsetZero(m_Callbacks);
	m_BufferInfo.clear();
	m_nAsioBufferLen = 0;

	CloseDriver();

	return true;
}


void CASIODevice::OpenDriver()
//----------------------------
{
	MPT_TRACE();
	if(IsDriverOpen())
	{
		return;
	}
	CLSID clsid = Util::StringToCLSID(GetDeviceInternalID());
	try
	{
		if(CoCreateInstance(clsid,0,CLSCTX_INPROC_SERVER, clsid, (void **)&m_pAsioDrv) != S_OK)
		{
			Log("ASIO: CoCreateInstance failed!");
			m_pAsioDrv = nullptr;
			return;
		}
	} catch(...)
	{
		Log("ASIO: CoCreateInstance crashed!");
		m_pAsioDrv = nullptr;
		return;
	}
	try
	{
		if(m_pAsioDrv->init((void *)m_Settings.hWnd) != ASIOTrue)
		{
			Log("ASIO: init() failed!");
			CloseDriver();
			return;
		}
	} catch(...)
	{
		Log("ASIO: init() crashed!");
		CloseDriver();
		return;
	}
}


void CASIODevice::CloseDriver()
//-----------------------------
{
	MPT_TRACE();
	if(!IsDriverOpen())
	{
		return;
	}
	try
	{
		m_pAsioDrv->Release();
	} catch(...)
	{
		CASIODevice::ReportASIOException("ASIO crash in Release()\n");
	}
	m_pAsioDrv = nullptr;
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


bool CASIODevice::IsSampleTypeInt16(ASIOSampleType sampleType)
//------------------------------------------------------------
{
	switch(sampleType)
	{
		case ASIOSTInt16MSB:
		case ASIOSTInt16LSB:
			return true;
			break;
		default:
			return false;
			break;
	}
}


bool CASIODevice::IsSampleTypeInt24(ASIOSampleType sampleType)
//------------------------------------------------------------
{
	switch(sampleType)
	{
		case ASIOSTInt24MSB:
		case ASIOSTInt24LSB:
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


bool CASIODevice::IsSampleTypeBigEndian(ASIOSampleType sampleType)
//----------------------------------------------------------------
{
	switch(sampleType)
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
			return true;
			break;
		default:
			return false;
			break;
	}
}


void CASIODevice::InternalFillAudioBuffer()
//-----------------------------------------
{
	MPT_TRACE();
	const bool rendersilence = (InterlockedExchangeAdd(&m_RenderSilence, 0) == 1);
	const int channels = m_Settings.Channels;
	const std::size_t countChunk = m_nAsioBufferLen;
	if(rendersilence)
	{
		if(m_Settings.sampleFormat == SampleFormatFloat32)
		{
			std::memset(&m_SampleBufferFloat[0], 0, countChunk * channels * sizeof(float));
		} else if(m_Settings.sampleFormat == SampleFormatInt16)
		{
			std::memset(&m_SampleBufferInt16[0], 0, countChunk * channels * sizeof(int16));
		} else if(m_Settings.sampleFormat == SampleFormatInt24)
		{
			std::memset(&m_SampleBufferInt24[0], 0, countChunk * channels * sizeof(int24));
		} else if(m_Settings.sampleFormat == SampleFormatInt32)
		{
			std::memset(&m_SampleBufferInt32[0], 0, countChunk * channels * sizeof(int32));
		} else
		{
			ASSERT(false);
		}
	} else
	{
		SourceAudioPreRead(countChunk);
		if(m_Settings.sampleFormat == SampleFormatFloat32)
		{
			SourceAudioRead(&m_SampleBufferFloat[0], countChunk);
		} else if(m_Settings.sampleFormat == SampleFormatInt16)
		{
			SourceAudioRead(&m_SampleBufferInt16[0], countChunk);
		} else if(m_Settings.sampleFormat == SampleFormatInt24)
		{
			SourceAudioRead(&m_SampleBufferInt24[0], countChunk);
		} else if(m_Settings.sampleFormat == SampleFormatInt32)
		{
			SourceAudioRead(&m_SampleBufferInt32[0], countChunk);
		} else
		{
			ASSERT(false);
		}
	}
	for(int channel = 0; channel < channels; ++channel)
	{
		void *dst = m_BufferInfo[channel].buffers[(unsigned long)m_BufferIndex & 1];
		if(!dst)
		{
			// Skip if we did get no buffer for this channel
			continue;
		}
		if(m_Settings.sampleFormat == SampleFormatFloat32)
		{
			const float *const srcFloat = &m_SampleBufferFloat[0];
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
		} else if(m_Settings.sampleFormat == SampleFormatInt16)
		{
			const int16 *const srcInt16 = &m_SampleBufferInt16[0];
			switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTInt16MSB:
				case ASIOSTInt16LSB:
						CopyInterleavedToChannel<SC::Convert<int16, int16> >(reinterpret_cast<int16*>(dst), srcInt16, channels, countChunk, channel);
						break;
					default:
						ASSERT(false);
						break;
			}
		} else if(m_Settings.sampleFormat == SampleFormatInt24)
		{
			const int24 *const srcInt24 = &m_SampleBufferInt24[0];
			switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTInt24MSB:
				case ASIOSTInt24LSB:
						CopyInterleavedToChannel<SC::Convert<int24, int24> >(reinterpret_cast<int24*>(dst), srcInt24, channels, countChunk, channel);
						break;
					default:
						ASSERT(false);
						break;
			}
		} else if(m_Settings.sampleFormat == SampleFormatInt32)
		{
			const int32 *const srcInt32 = &m_SampleBufferInt32[0];
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
		} else
		{
			ASSERT(false);
		}
		if(IsSampleTypeBigEndian(m_ChannelInfo[channel].type))
		{
			SwapEndian(reinterpret_cast<uint8*>(dst), countChunk, GetSampleSize(m_ChannelInfo[channel].type));
		}
	}
	if(m_CanOutputReady)
	{
		m_pAsioDrv->outputReady(); // do not handle errors, there is nothing we could do about them
	}
	if(!rendersilence)
	{
		SourceAudioDone(countChunk, m_nAsioBufferLen);
	}
}


bool CASIODevice::InternalHasTimeInfo() const
//-------------------------------------------
{
	MPT_TRACE();
	return m_Settings.UseHardwareTiming;
}


bool CASIODevice::InternalHasGetStreamPosition() const
//----------------------------------------------------
{
	MPT_TRACE();
	return m_Settings.UseHardwareTiming;
}


int64 CASIODevice::InternalGetStreamPositionFrames() const
//--------------------------------------------------------
{
	MPT_TRACE();
	if(m_Settings.UseHardwareTiming)
	{
		const uint64 asioNow = Clock().NowNanoseconds();
		SoundDevice::TimeInfo timeInfo = GetTimeInfo();
		int64 currentStreamPositionFrames =
			Util::Round<int64>(
			timeInfo.StreamFrames + ((double)((int64)(asioNow - timeInfo.SystemTimestamp)) * timeInfo.Speed * m_Settings.Samplerate / (1000.0 * 1000.0))
			)
			;
		return currentStreamPositionFrames;
	} else
	{
		return SoundDevice::Base::InternalGetStreamPositionFrames();
	}
}


void CASIODevice::UpdateTimeInfo(AsioTimeInfo asioTimeInfo)
//---------------------------------------------------------
{
	MPT_TRACE();
	if(m_Settings.UseHardwareTiming)
	{
		if((asioTimeInfo.flags & kSamplePositionValid) && (asioTimeInfo.flags & kSystemTimeValid))
		{
			double speed = 1.0;
			if((asioTimeInfo.flags & kSpeedValid) && (asioTimeInfo.speed > 0.0))
			{
				speed = asioTimeInfo.speed;
			} else if((asioTimeInfo.flags & kSampleRateValid) && (asioTimeInfo.sampleRate > 0.0))
			{
				speed *= asioTimeInfo.sampleRate / m_Settings.Samplerate;
			}
			SoundDevice::TimeInfo timeInfo;
			timeInfo.StreamFrames = ((((uint64)asioTimeInfo.samplePosition.hi) << 32) | asioTimeInfo.samplePosition.lo) - m_StreamPositionOffset;
			timeInfo.SystemTimestamp = (((uint64)asioTimeInfo.systemTime.hi) << 32) | asioTimeInfo.systemTime.lo;
			timeInfo.Speed = speed;
			SoundDevice::Base::UpdateTimeInfo(timeInfo);
		} else
		{ // spec violation or nothing provided at all, better to estimate this stuff ourselves
			const uint64 asioNow = Clock().NowNanoseconds();
			SoundDevice::TimeInfo timeInfo;
			timeInfo.StreamFrames = m_TotalFramesWritten + m_nAsioBufferLen - m_StreamPositionOffset;
			timeInfo.SystemTimestamp = asioNow + Util::Round<int64>(GetBufferAttributes().Latency * 1000.0 * 1000.0 * 1000.0);
			timeInfo.Speed = 1.0;
			SoundDevice::Base::UpdateTimeInfo(timeInfo);
		}
	}
}


void CASIODevice::BufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
//----------------------------------------------------------------------------
{
	MPT_TRACE();
	BufferSwitchTimeInfo(nullptr, doubleBufferIndex, directProcess); // delegate
}


ASIOTime* CASIODevice::BufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
//-----------------------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	ASSERT(directProcess); // !directProcess is not handled correctly in OpenMPT, would require a separate thread and potentially additional buffering
	if(!directProcess)
	{
		m_UsedFeatures.set(AsioFeatureNoDirectProcess);
	}
	if(m_Settings.UseHardwareTiming)
	{
		if(params)
		{
			UpdateTimeInfo(params->timeInfo);
		} else
		{
			AsioTimeInfo asioTimeInfo;
			MemsetZero(asioTimeInfo);
			UpdateTimeInfo(asioTimeInfo);
			try
			{
				ASIOSamples samplePosition;
				ASIOTimeStamp systemTime;
				MemsetZero(samplePosition);
				MemsetZero(systemTime);
				if(asioCallUncatched(getSamplePosition(&samplePosition, &systemTime)) == ASE_OK)
				{
					AsioTimeInfo asioTimeInfoQueried;
					MemsetZero(asioTimeInfoQueried);
					asioTimeInfoQueried.flags = kSamplePositionValid | kSystemTimeValid;
					asioTimeInfoQueried.samplePosition = samplePosition;
					asioTimeInfoQueried.systemTime = systemTime;
					asioTimeInfoQueried.speed = 1.0;
					ASIOSampleRate sampleRate;
					MemsetZero(sampleRate);
					if(asioCallUncatched(getSampleRate(&sampleRate)) == ASE_OK)
					{
						if(sampleRate >= 0.0)
						{
							asioTimeInfoQueried.flags |= kSampleRateValid;
							asioTimeInfoQueried.sampleRate = sampleRate;
						}
					}
					asioTimeInfo = asioTimeInfoQueried;
				}
			} catch(...)
			{
				// continue
			}
			UpdateTimeInfo(asioTimeInfo);
		}
	}
	m_BufferIndex = doubleBufferIndex;
	bool rendersilence = (InterlockedExchangeAdd(&m_RenderSilence, 0) == 1);
	InterlockedExchange(&m_RenderingSilence, rendersilence ? 1 : 0 );
	if(rendersilence)
	{
		m_StreamPositionOffset += m_nAsioBufferLen;
		FillAudioBuffer();
	} else
	{
		SourceFillAudioBufferLocked();
	}
	m_TotalFramesWritten += m_nAsioBufferLen;
	return params;
}


void CASIODevice::SampleRateDidChange(ASIOSampleRate sRate)
//---------------------------------------------------------
{
	MPT_TRACE();
	if(Util::Round<uint32>(sRate) == m_Settings.Samplerate)
	{
		// not different, ignore it
		return;
	}
	m_UsedFeatures.set(AsioFeatureSampleRateChange);
	if((double)m_Settings.Samplerate * (1.0 - AsioSampleRateTolerance) <= sRate && sRate <= (double)m_Settings.Samplerate * (1.0 + AsioSampleRateTolerance))
	{
		// ignore slight differences which might me due to a unstable external ASIO clock source
		return;
	}
	// play safe and close the device
	RequestClose();
}


static std::string AsioFeaturesToString(FlagSet<AsioFeatures> features)
//---------------------------------------------------------------------
{
	std::string result;
	bool first = true;
	if(features[AsioFeatureResetRequest]) { if(!first) { result += ","; } first = false; result += "reset"; }
	if(features[AsioFeatureResyncRequest]) { if(!first) { result += ","; } first = false; result += "resync"; }
	if(features[AsioFeatureLatenciesChanged]) { if(!first) { result += ","; } first = false; result += "latency"; }
	if(features[AsioFeatureBufferSizeChange]) { if(!first) { result += ","; } first = false; result += "buffer"; }
	if(features[AsioFeatureOverload]) { if(!first) { result += ","; } first = false; result += "load"; }
	if(features[AsioFeatureNoDirectProcess]) { if(!first) { result += ","; } first = false; result += "nodirect"; }
	if(features[AsioFeatureSampleRateChange]) { if(!first) { result += ","; } first = false; result += "srate"; }
	return result;
}


std::string CASIODevice::GetStatistics() const
//--------------------------------------------
{
	MPT_TRACE();
	const FlagSet<AsioFeatures> unsupported((AsioFeatures)(AsioFeatureNoDirectProcess | AsioFeatureOverload | AsioFeatureBufferSizeChange | AsioFeatureSampleRateChange));
	FlagSet<AsioFeatures> unsupportedFeatues = m_UsedFeatures;
	unsupportedFeatues &= unsupported;
	if(unsupportedFeatues.any())
	{
		return mpt::String::Print("WARNING: unsupported features: %1", AsioFeaturesToString(unsupportedFeatues));
	} else if(m_UsedFeatures.any())
	{
		return mpt::String::Print("OK, features used: %1", AsioFeaturesToString(m_UsedFeatures));
	} else if(m_QueriedFeatures.any())
	{
		return mpt::String::Print("OK, features queried: %1", AsioFeaturesToString(m_QueriedFeatures));
	}
	return std::string("OK.");
}


long CASIODevice::AsioMessage(long selector, long value, void* message, double* opt)
//----------------------------------------------------------------------------------
{
	MPT_TRACE();
	long result = 0;
	switch(selector)
	{
	case kAsioSelectorSupported:
		switch(value)
		{
		case kAsioSelectorSupported:
		case kAsioEngineVersion:
			result = 1;
			break;
		case kAsioSupportsTimeInfo:
			result = m_Settings.UseHardwareTiming ? 1 : 0;
			break;
		case kAsioResetRequest:
			m_QueriedFeatures.set(AsioFeatureResetRequest);
			result = 1;
			break;
		case kAsioBufferSizeChange:
			m_QueriedFeatures.set(AsioFeatureBufferSizeChange);
			result = 0;
			break;
		case kAsioResyncRequest:
			m_QueriedFeatures.set(AsioFeatureResyncRequest);
			result = 1;
			break;
		case kAsioLatenciesChanged:
			m_QueriedFeatures.set(AsioFeatureLatenciesChanged);
			result = 1;
			break;
		case kAsioOverload:
			m_QueriedFeatures.set(AsioFeatureOverload);
			// unsupported
			result = 0;
			break;
		default:
			// unsupported
			result = 0;
			break;
		}
		break;
	case kAsioEngineVersion:
		result = 2;
		break;
	case kAsioSupportsTimeInfo:
		result = m_Settings.UseHardwareTiming ? 1 : 0;
		break;
	case kAsioResetRequest:
		m_UsedFeatures.set(AsioFeatureResetRequest);
		RequestReset();
		result = 1;
		break;
	case kAsioBufferSizeChange:
		m_UsedFeatures.set(AsioFeatureBufferSizeChange);
		// We do not support kAsioBufferSizeChange.
		// This should cause a driver to send a kAsioResetRequest.
		result = 0;
		break;
	case kAsioResyncRequest:
		m_UsedFeatures.set(AsioFeatureResyncRequest);
		RequestRestart();
		result = 1;
		break;
	case kAsioLatenciesChanged:
		_InterlockedOr(&m_AsioRequestFlags, AsioRequestFlagLatenciesChanged);
		result = 1;
		break;
	case kAsioOverload:
		m_UsedFeatures.set(AsioFeatureOverload);
		// unsupported
		result = 0;
		break;
	default:
		// unsupported
		result = 0;
		break;
	}
	Log(mpt::String::Print("ASIO: AsioMessage(selector=%1, value=%2, message=%3, opt=%4) => result=%5"
		, selector
		, value
		, reinterpret_cast<std::size_t>(message)
		, opt ? mpt::ToString(*opt) : "NULL"
		, result
		));
	return result;
}


long CASIODevice::CallbackAsioMessage(long selector, long value, void* message, double* opt)
//------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	ALWAYS_ASSERT(g_CallbacksInstance);
	if(!g_CallbacksInstance) return 0;
	return g_CallbacksInstance->AsioMessage(selector, value, message, opt);
}


void CASIODevice::CallbackSampleRateDidChange(ASIOSampleRate sRate)
//-----------------------------------------------------------------
{
	MPT_TRACE();
	ALWAYS_ASSERT(g_CallbacksInstance);
	if(!g_CallbacksInstance) return;
	g_CallbacksInstance->SampleRateDidChange(sRate);
}


void CASIODevice::CallbackBufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
//------------------------------------------------------------------------------------
{
	MPT_TRACE();
	ALWAYS_ASSERT(g_CallbacksInstance);
	if(!g_CallbacksInstance) return;
	g_CallbacksInstance->BufferSwitch(doubleBufferIndex, directProcess);
}


ASIOTime* CASIODevice::CallbackBufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
//-------------------------------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	ALWAYS_ASSERT(g_CallbacksInstance);
	if(!g_CallbacksInstance) return params;
	return g_CallbacksInstance->BufferSwitchTimeInfo(params, doubleBufferIndex, directProcess);
}


void CASIODevice::ReportASIOException(const std::string &str)
//-----------------------------------------------------------
{
	MPT_TRACE();
	AudioSendMessage(str);
	Log("%s", str.c_str());
}


SoundDevice::Caps CASIODevice::InternalGetDeviceCaps()
//--------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::Caps caps;

	caps.Available = true;
	caps.CanUpdateInterval = false;
	caps.CanSampleFormat = false;
	caps.CanExclusiveMode = false;
	caps.CanBoostThreadPriority = false;
	caps.CanKeepDeviceRunning = true;
	caps.CanUseHardwareTiming = true;
	caps.CanChannelMapping = true;
	caps.CanDriverPanel = true;

	caps.LatencyMin = 0.000001; // 1 us
	caps.LatencyMax = 0.5; // 500 ms
	caps.UpdateIntervalMin = 0.0; // disabled
	caps.UpdateIntervalMax = 0.0; // disabled

	caps.DefaultSettings.sampleFormat = SampleFormatFloat32;

	return caps;
}


SoundDevice::DynamicCaps CASIODevice::GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates)
//--------------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::DynamicCaps caps;

	TemporaryASIODriverOpener opener(*this);
	if(!IsDriverOpen())
	{
		m_DeviceUnavailableOnOpen = true;
		return caps;
	}

	try
	{
		ASIOSampleRate samplerate = 0.0;
		asioCall(getSampleRate(&samplerate));
		if(samplerate > 0.0)
		{
			caps.currentSampleRate = Util::Round<uint32>(samplerate);
		}
	} catch(...)
	{
		// continue
	}

	for(size_t i = 0; i < baseSampleRates.size(); i++)
	{
		try
		{
			if(asioCallUncatched(canSampleRate((ASIOSampleRate)baseSampleRates[i])) == ASE_OK)
			{
				caps.supportedSampleRates.push_back(baseSampleRates[i]);
				caps.supportedExclusiveSampleRates.push_back(baseSampleRates[i]);
			}
		} catch(...)
		{
			// continue
		}
	}

	try
	{
		long inputChannels = 0;
		long outputChannels = 0;
		asioCall(getChannels(&inputChannels, &outputChannels));
		if(!((inputChannels > 0) || (outputChannels > 0)))
		{
			m_DeviceUnavailableOnOpen = true;
		}
		for(long i = 0; i < outputChannels; ++i)
		{
			ASIOChannelInfo channelInfo;
			MemsetZero(channelInfo);
			channelInfo.channel = i;
			channelInfo.isInput = ASIOFalse;
			std::wstring name = mpt::ToWString(i);
			try
			{
				asioCall(getChannelInfo(&channelInfo));
				mpt::String::SetNullTerminator(channelInfo.name);
				name = mpt::ToWide(mpt::CharsetLocale, channelInfo.name);
			} catch(...)
			{
				// continue
			}
			caps.channelNames.push_back(name);
		}
	} catch(...)
	{
		// continue
	}
	return caps;
}


bool CASIODevice::OpenDriverSettings()
//------------------------------------
{
	MPT_TRACE();
	TemporaryASIODriverOpener opener(*this);
	if(!IsDriverOpen())
	{
		return false;
	}
	try
	{
		asioCall(controlPanel());
	} catch(...)
	{
		return false;
	}
	return true;
}


#endif // NO_ASIO


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
