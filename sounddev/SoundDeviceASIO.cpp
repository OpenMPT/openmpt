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


#ifndef NO_ASIO


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
	Init();
}


void CASIODevice::Init()
//----------------------
{
	m_pAsioDrv = nullptr;

	m_nAsioBufferLen = 0;
	m_BufferInfo.clear();
	MemsetZero(m_Callbacks);
	m_BuffersCreated = false;
	m_ChannelInfo.clear();
	m_SampleBuffer.clear();
	m_CanOutputReady = false;

	m_DeviceRunning = false;
	m_TotalFramesWritten = 0;
	m_BufferIndex = 0;
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

	ASSERT(!IsDriverOpen());

	Init();

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
		if(m_Settings.Channels > outputChannels)
		{
			throw ASIOException("Not enough output channels.");
		}
		if(m_Settings.ChannelMapping.GetRequiredDeviceChannels() > (std::size_t)outputChannels)
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
		for(int channel = 0; channel < m_Settings.Channels; ++channel)
		{
			if(!IsSampleTypeFloat(m_ChannelInfo[channel].type))
			{
				allChannelsAreFloat = false;
			}
		}
		m_Settings.sampleFormat = allChannelsAreFloat ? SampleFormatFloat32 : SampleFormatInt32;

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

		m_SampleBuffer.resize(m_nAsioBufferLen * m_Settings.Channels);

		SoundBufferAttributes bufferAttributes;
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


bool CASIODevice::InternalStart()
//-------------------------------
{
	ALWAYS_ASSERT_WARN_MESSAGE(!CriticalSection::IsLocked(), "AudioCriticalSection locked while starting ASIO");

	if(m_DeviceRunning)
	{
		SetRenderSilence(false, true);
		return true;
	}

	SetRenderSilence(false);
	try
	{
		asioCall(start());
		m_DeviceRunning = true;
		m_TotalFramesWritten = 0;
	} catch(...)
	{
		return false;
	}

	return true;
}


void CASIODevice::InternalStop()
//------------------------------
{
	ALWAYS_ASSERT_WARN_MESSAGE(!CriticalSection::IsLocked(), "AudioCriticalSection locked while stopping ASIO");

	SetRenderSilence(true, true);
}


bool CASIODevice::InternalClose()
//-------------------------------
{
	if(m_DeviceRunning)
	{
		try
		{
			asioCall(stop());
		} catch(...)
		{
			// continue
		}
		m_TotalFramesWritten = 0;
		m_DeviceRunning = false;
	}
	SetRenderSilence(false);

	m_CanOutputReady = false;
	m_SampleBuffer.clear();
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
	if(IsDriverOpen())
	{
		return;
	}
	CLSID clsid;
	Util::StringToCLSID(GetDeviceInternalID(), clsid);
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
	const bool rendersilence = (InterlockedExchangeAdd(&m_RenderSilence, 0) == 1);
	const int channels = m_Settings.Channels;
	const std::size_t countChunk = m_nAsioBufferLen;
	if(rendersilence)
	{
		std::memset(&m_SampleBuffer[0], 0, countChunk * channels * sizeof(int32));
	} else
	{
		SourceAudioPreRead(countChunk);
		SourceAudioRead(&m_SampleBuffer[0], countChunk);
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
	return m_Settings.UseHardwareTiming;
}


bool CASIODevice::InternalHasGetStreamPosition() const
//----------------------------------------------------
{
	return m_Settings.UseHardwareTiming;
}


int64 CASIODevice::InternalGetStreamPositionFrames() const
//--------------------------------------------------------
{
	if(m_Settings.UseHardwareTiming)
	{
		const uint64 asioNow = Clock().NowNanoseconds();
		SoundTimeInfo timeInfo = GetTimeInfo();
		int64 currentStreamPositionFrames =
			Util::Round<int64>(
			timeInfo.StreamFrames + ((double)((int64)(asioNow - timeInfo.SystemTimestamp)) * timeInfo.Speed * m_Settings.Samplerate / (1000.0 * 1000.0))
			)
			;
		return currentStreamPositionFrames;
	} else
	{
		return ISoundDevice::InternalGetStreamPositionFrames();
	}
}


void CASIODevice::UpdateTimeInfo(AsioTimeInfo asioTimeInfo)
//---------------------------------------------------------
{
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
			SoundTimeInfo timeInfo;
			timeInfo.StreamFrames = ((((uint64)asioTimeInfo.samplePosition.hi) << 32) | asioTimeInfo.samplePosition.lo) - m_StreamPositionOffset;
			timeInfo.SystemTimestamp = (((uint64)asioTimeInfo.systemTime.hi) << 32) | asioTimeInfo.systemTime.lo;
			timeInfo.Speed = speed;
			ISoundDevice::UpdateTimeInfo(timeInfo);
		} else
		{ // spec violation or nothing provided at all, better to estimate this stuff ourselves
			const uint64 asioNow = Clock().NowNanoseconds();
			SoundTimeInfo timeInfo;
			timeInfo.StreamFrames = m_TotalFramesWritten + m_nAsioBufferLen - m_StreamPositionOffset;
			timeInfo.SystemTimestamp = asioNow + Util::Round<int64>(GetBufferAttributes().Latency * 1000.0 * 1000.0 * 1000.0);
			timeInfo.Speed = 1.0;
			ISoundDevice::UpdateTimeInfo(timeInfo);
		}
	}
}


void CASIODevice::BufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
//----------------------------------------------------------------------------
{
	BufferSwitchTimeInfo(nullptr, doubleBufferIndex, directProcess); // delegate
}


ASIOTime* CASIODevice::BufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
//-----------------------------------------------------------------------------------------------------------
{
	ASSERT(directProcess); // !directProcess is not handled correctly in OpenMPT, would require a separate thread and potentially additional buffering
	MPT_UNREFERENCED_PARAMETER(directProcess);
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
	MPT_UNREFERENCED_PARAMETER(sRate);
}


long CASIODevice::AsioMessage(long selector, long value, void* message, double* opt)
//----------------------------------------------------------------------------------
{
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
		case kAsioBufferSizeChange:
		case kAsioResyncRequest:
		case kAsioLatenciesChanged:
		case kAsioOverload:
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
	case kAsioBufferSizeChange:
	case kAsioResyncRequest:
	case kAsioLatenciesChanged:
	case kAsioOverload:
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
	ALWAYS_ASSERT(g_CallbacksInstance);
	if(!g_CallbacksInstance) return 0;
	return g_CallbacksInstance->AsioMessage(selector, value, message, opt);
}


void CASIODevice::CallbackSampleRateDidChange(ASIOSampleRate sRate)
//-----------------------------------------------------------------
{
	ALWAYS_ASSERT(g_CallbacksInstance);
	if(!g_CallbacksInstance) return;
	g_CallbacksInstance->SampleRateDidChange(sRate);
}


void CASIODevice::CallbackBufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
//------------------------------------------------------------------------------------
{
	ALWAYS_ASSERT(g_CallbacksInstance);
	if(!g_CallbacksInstance) return;
	g_CallbacksInstance->BufferSwitch(doubleBufferIndex, directProcess);
}


ASIOTime* CASIODevice::CallbackBufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
//-------------------------------------------------------------------------------------------------------------------
{
	ALWAYS_ASSERT(g_CallbacksInstance);
	if(!g_CallbacksInstance) return params;
	return g_CallbacksInstance->BufferSwitchTimeInfo(params, doubleBufferIndex, directProcess);
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

	TemporaryASIODriverOpener opener(*this);
	if(!IsDriverOpen())
	{
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
