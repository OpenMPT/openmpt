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
#include "../common/mptUUID.h"
#include "../common/StringFixer.h"
#include "../soundlib/SampleFormatConverters.h"

#include <algorithm>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


#ifdef MPT_WITH_ASIO


MPT_REGISTERED_COMPONENT(ComponentASIO, "ASIO")


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
};


class ASIOCallError
	: public ASIOException
{
public:
	ASIOCallError(const std::string &call, ASIOError err)
		: ASIOException(call + std::string(" failed: ") + mpt::ToString(err))
	{
		return;
	}
};


class ASIOCallBoolResult
	: public ASIOException
{
public:
	ASIOCallBoolResult(const std::string &call, ASIOBool err)
		: ASIOException(call + std::string(" failed: ") + mpt::ToString(err))
	{
		return;
	}
};


struct ASIOCrash {};


struct SafeASIO
{

	IASIO * asio;

	SafeASIO(IASIO * asio) : asio(asio) {}

	#define MPT_ASIO_SEH_TRY \
		bool crashed = false; \
		__try { \
	/**/

	#define MPT_ASIO_SEH_CATCH \
		} __except(EXCEPTION_EXECUTE_HANDLER) { \
			crashed = true; \
		} \
		if(crashed) { \
			throw ASIOCrash(); \
		} \
	/**/

	ASIOBool init(void *sysHandle) {
		MPT_ASIO_SEH_TRY
			return asio->init(sysHandle);
		MPT_ASIO_SEH_CATCH
		return ASIOFalse;
	}
	void getDriverName(char *name) {
		MPT_ASIO_SEH_TRY
			asio->getDriverName(name);
		MPT_ASIO_SEH_CATCH
	}	
	long getDriverVersion() {
		MPT_ASIO_SEH_TRY
			return asio->getDriverVersion();
		MPT_ASIO_SEH_CATCH
		return 0;
	}
	void getErrorMessage(char *string) {
		MPT_ASIO_SEH_TRY
			asio->getErrorMessage(string);
		MPT_ASIO_SEH_CATCH
	}	
	ASIOError start() {
		MPT_ASIO_SEH_TRY
			return asio->start();
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError stop() {
		MPT_ASIO_SEH_TRY
			return asio->stop();
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError getChannels(long *numInputChannels, long *numOutputChannels) {
		MPT_ASIO_SEH_TRY
			return asio->getChannels(numInputChannels, numOutputChannels);
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError getLatencies(long *inputLatency, long *outputLatency) {
		MPT_ASIO_SEH_TRY
			return asio->getLatencies(inputLatency, outputLatency);
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError getBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity) {
		MPT_ASIO_SEH_TRY
			return asio->getBufferSize(minSize, maxSize, preferredSize, granularity);
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError canSampleRate(ASIOSampleRate sampleRate) {
		MPT_ASIO_SEH_TRY
			return asio->canSampleRate(sampleRate);
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError getSampleRate(ASIOSampleRate *sampleRate) {
		MPT_ASIO_SEH_TRY
			return asio->getSampleRate(sampleRate);
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError setSampleRate(ASIOSampleRate sampleRate) {
		MPT_ASIO_SEH_TRY
			return asio->setSampleRate(sampleRate);
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError getClockSources(ASIOClockSource *clocks, long *numSources) {
		MPT_ASIO_SEH_TRY
			return asio->getClockSources(clocks, numSources);
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError setClockSource(long reference) {
		MPT_ASIO_SEH_TRY
			return asio->setClockSource(reference);
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError getSamplePosition(ASIOSamples *sPos, ASIOTimeStamp *tStamp) {
		MPT_ASIO_SEH_TRY
			return asio->getSamplePosition(sPos, tStamp);
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError getChannelInfo(ASIOChannelInfo *info) {
		MPT_ASIO_SEH_TRY
			return asio->getChannelInfo(info);
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError createBuffers(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks *callbacks) {
		MPT_ASIO_SEH_TRY
			return asio->createBuffers(bufferInfos, numChannels, bufferSize, callbacks);
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError disposeBuffers() {
		MPT_ASIO_SEH_TRY
			return asio->disposeBuffers();
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError controlPanel() {
		MPT_ASIO_SEH_TRY
			return asio->controlPanel();
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError future(long selector, void *opt) {
		MPT_ASIO_SEH_TRY
			return asio->future(selector, opt);
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}
	ASIOError outputReady() {
		MPT_ASIO_SEH_TRY
			return asio->outputReady();
		MPT_ASIO_SEH_CATCH
		return ASE_InvalidParameter;
	}

	#undef MPT_ASIO_SEH_TRY

	#undef MPT_ASIO_SEH_CATCH

}; // struct SafeASIO


#define asioCall(asiocall) MPT_DO { \
	ASIOError e = ASE_InvalidParameter; \
	try { \
		e = SafeASIO(m_pAsioDrv). asiocall ; \
	} catch(const ASIOCrash &) { \
		CASIODevice::ReportASIOException( #asiocall + std::string(" crashed!")); \
		throw ASIOException(std::string("Exception in '") + #asiocall + std::string("'!")); \
	} \
	if(e != ASE_OK) { \
		throw ASIOCallError( #asiocall , e); \
	} \
} MPT_WHILE_0


#define asioCallCheckedBool(asiocall) MPT_DO { \
	ASIOBool e = ASIOFalse; \
	try { \
		e = SafeASIO(m_pAsioDrv). asiocall ; \
	} catch(const ASIOCrash &) { \
		CASIODevice::ReportASIOException( #asiocall + std::string(" crashed!")); \
		throw ASIOException(std::string("Exception in '") + #asiocall + std::string("'!")); \
	} \
	if(e != ASIOTrue) { \
		throw ASIOCallBoolResult( #asiocall , e); \
	} \
} while(0)


#define asioCallUnchecked(pasioresult, asiocall) do { \
	if(( pasioresult )) { \
		*( pasioresult ) = ASE_InvalidParameter; \
	} \
	ASIOError e = ASE_InvalidParameter; \
	try { \
		e = SafeASIO(m_pAsioDrv). asiocall ; \
	} catch(const ASIOCrash &) { \
		CASIODevice::ReportASIOException( #asiocall + std::string(" crashed!")); \
		throw ASIOException(std::string("Exception in '") + #asiocall + std::string("'!")); \
	} \
	if(( pasioresult )) { \
		*( pasioresult ) = e; \
	} \
} MPT_WHILE_0


#define ASIO_MAXDRVNAMELEN	1024

CASIODevice *CASIODevice::g_CallbacksInstance = nullptr;


std::vector<SoundDevice::Info> CASIODevice::EnumerateDevices(SoundDevice::SysInfo /* sysInfo */ )
//-----------------------------------------------------------------------------------------------
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
		Log(mpt::String::Print(MPT_USTRING("ASIO: Found '%1':"), keyname));

		HKEY hksub = NULL;
		if(RegOpenKeyExW(hkEnum, keynameBuf, 0, KEY_READ, &hksub) != ERROR_SUCCESS)
		{
			continue;
		}

		WCHAR descriptionBuf[ASIO_MAXDRVNAMELEN];
		DWORD datatype = REG_SZ;
		DWORD datasize = sizeof(descriptionBuf);
		mpt::ustring description;
		if(ERROR_SUCCESS == RegQueryValueExW(hksub, L"Description", 0, &datatype, (LPBYTE)descriptionBuf, &datasize))
		{
			Log(mpt::String::Print(MPT_USTRING("ASIO:   description='%1'"), description));
			description = mpt::ToUnicode(descriptionBuf);
		} else
		{
			description = mpt::ToUnicode(keyname);
		}

		WCHAR idBuf[256];
		datatype = REG_SZ;
		datasize = sizeof(idBuf);
		if(ERROR_SUCCESS == RegQueryValueExW(hksub, L"CLSID", 0, &datatype, (LPBYTE)idBuf, &datasize))
		{
			const mpt::ustring internalID = mpt::ToUnicode(idBuf);
			if(Util::IsCLSID(mpt::ToWide(internalID)))
			{
				Log(mpt::String::Print(MPT_USTRING("ASIO:   clsid=%1"), internalID));
				SoundDevice::Info info;
				info.type = TypeASIO;
				info.internalID = internalID;
				info.apiName = MPT_USTRING("ASIO");
				info.name = description;
				info.useNameAsIdentifier = false;
				info.isDefault = false;
				info.extraData[MPT_USTRING("Key")] = mpt::ToUnicode(keyname);
				info.extraData[MPT_USTRING("Description")] = mpt::ToUnicode(descriptionBuf);
				info.extraData[MPT_USTRING("CLSID")] = mpt::ToUnicode(internalID);
				devices.push_back(info);
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


CASIODevice::CASIODevice(SoundDevice::Info info, SoundDevice::SysInfo sysInfo)
//----------------------------------------------------------------------------
	: SoundDevice::Base(info, sysInfo)
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

	m_BufferLatency = 0.0;
	m_nAsioBufferLen = 0;
	m_BufferInfo.clear();
	MemsetZero(m_Callbacks);
	m_BuffersCreated = false;
	m_ChannelInfo.clear();
	m_SampleBufferFloat.clear();
	m_SampleBufferInt16.clear();
	m_SampleBufferInt24.clear();
	m_SampleBufferInt32.clear();
	m_SampleInputBufferFloat.clear();
	m_SampleInputBufferInt16.clear();
	m_SampleInputBufferInt24.clear();
	m_SampleInputBufferInt32.clear();
	m_CanOutputReady = false;

	m_DeviceRunning = false;
	m_TotalFramesWritten = 0;
	m_BufferIndex = 0;
	m_RenderSilence = 0;
	m_RenderingSilence = 0;

	m_AsioRequestFlags = 0;

	m_DebugRealtimeThreadID.store(0);

}


bool CASIODevice::HandleRequests()
//--------------------------------
{
	MPT_TRACE();
	bool result = false;
	uint32 flags = m_AsioRequestFlags.exchange(0);
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

	MPT_ASSERT(!IsDriverOpen());

	InitMembers();

	Log(mpt::String::Print(MPT_USTRING("ASIO: Open('%1'): %2-bit, (%3,%4) channels, %5Hz, hw-timing=%6")
		, GetDeviceInternalID()
		, m_Settings.sampleFormat.GetBitsPerSample()
		, m_Settings.InputChannels
		, m_Settings.Channels
		, m_Settings.Samplerate
		, m_Settings.UseHardwareTiming
		));

	SoundDevice::ChannelMapping inputChannelMapping = SoundDevice::ChannelMapping::BaseChannel(m_Settings.InputChannels, m_Settings.InputSourceID);

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
		if(m_Settings.Channels.GetRequiredDeviceChannels() > outputChannels)
		{
			throw ASIOException("Channel mapping requires more channels than available.");
		}
		if(m_Settings.InputChannels > inputChannels)
		{
			throw ASIOException("Not enough input channels.");
		}
		if(inputChannelMapping.GetRequiredDeviceChannels() > inputChannels)
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
			MPT_ASSERT_NOTREACHED();
		}
	
		m_BufferInfo.resize(m_Settings.GetTotalChannels());
		for(uint32 channel = 0; channel < m_Settings.GetTotalChannels(); ++channel)
		{
			MemsetZero(m_BufferInfo[channel]);
			if(channel < m_Settings.InputChannels)
			{
				m_BufferInfo[channel].isInput = ASIOTrue;
				m_BufferInfo[channel].channelNum = inputChannelMapping.ToDevice(channel);
			} else
			{
				m_BufferInfo[channel].isInput = ASIOFalse;
				m_BufferInfo[channel].channelNum = m_Settings.Channels.ToDevice(channel - m_Settings.InputChannels);
			}
		}
		m_Callbacks.bufferSwitch = CallbackBufferSwitch;
		m_Callbacks.sampleRateDidChange = CallbackSampleRateDidChange;
		m_Callbacks.asioMessage = CallbackAsioMessage;
		m_Callbacks.bufferSwitchTimeInfo = CallbackBufferSwitchTimeInfo;
		MPT_ASSERT_ALWAYS(g_CallbacksInstance == nullptr);
		g_CallbacksInstance = this;
		Log(mpt::String::Print("ASIO: createBuffers(numChannels=%1, bufferSize=%2)", m_Settings.Channels, m_nAsioBufferLen));
		asioCall(createBuffers(&m_BufferInfo[0], m_Settings.GetTotalChannels(), m_nAsioBufferLen, &m_Callbacks));
		m_BuffersCreated = true;

		m_ChannelInfo.resize(m_Settings.GetTotalChannels());
		for(uint32 channel = 0; channel < m_Settings.GetTotalChannels(); ++channel)
		{
			MemsetZero(m_ChannelInfo[channel]);
			if(channel < m_Settings.InputChannels)
			{
				m_ChannelInfo[channel].isInput = ASIOTrue;
				m_ChannelInfo[channel].channel = inputChannelMapping.ToDevice(channel);
			} else
			{
				m_ChannelInfo[channel].isInput = ASIOFalse;
				m_ChannelInfo[channel].channel = m_Settings.Channels.ToDevice(channel - m_Settings.InputChannels);
			}
			asioCall(getChannelInfo(&m_ChannelInfo[channel]));
			MPT_ASSERT(m_ChannelInfo[channel].isActive);
			mpt::String::SetNullTerminator(m_ChannelInfo[channel].name);
			Log(mpt::String::Print("ASIO: getChannelInfo(isInput=%1 channel=%2) => isActive=%3 channelGroup=%4 type=%5 name='%6'"
				, (channel < m_Settings.InputChannels) ? ASIOTrue : ASIOFalse
				, m_Settings.Channels.ToDevice(channel)
				, m_ChannelInfo[channel].isActive
				, m_ChannelInfo[channel].channelGroup
				, m_ChannelInfo[channel].type
				, m_ChannelInfo[channel].name
				));
		}

		bool allChannelsAreFloat = true;
		bool allChannelsAreInt16 = true;
		bool allChannelsAreInt24 = true;
		for(std::size_t channel = 0; channel < m_Settings.GetTotalChannels(); ++channel)
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
			m_SampleInputBufferFloat.resize(m_nAsioBufferLen * m_Settings.InputChannels);
		} else if(allChannelsAreInt16)
		{
			m_Settings.sampleFormat = SampleFormatInt16;
			m_SampleBufferInt16.resize(m_nAsioBufferLen * m_Settings.Channels);
			m_SampleInputBufferInt16.resize(m_nAsioBufferLen * m_Settings.InputChannels);
		} else if(allChannelsAreInt24)
		{
			m_Settings.sampleFormat = SampleFormatInt24;
			m_SampleBufferInt24.resize(m_nAsioBufferLen * m_Settings.Channels);
			m_SampleInputBufferInt24.resize(m_nAsioBufferLen * m_Settings.InputChannels);
		} else
		{
			m_Settings.sampleFormat = SampleFormatInt32;
			m_SampleBufferInt32.resize(m_nAsioBufferLen * m_Settings.Channels);
			m_SampleInputBufferInt32.resize(m_nAsioBufferLen * m_Settings.InputChannels);
		}

		for(std::size_t channel = 0; channel < m_Settings.GetTotalChannels(); ++channel)
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
			ASIOError asioresult = ASE_InvalidParameter;
			asioCallUnchecked(&asioresult, outputReady());
			m_CanOutputReady = (asioresult == ASE_OK);
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
		Log(mpt::String::Print("ASIO: Unknown error opening device!"));
	}
	InternalClose();
	return false;
}


void CASIODevice::UpdateLatency()
//-------------------------------
{
	MPT_TRACE();
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
	if(outputLatency >= static_cast<long>(m_nAsioBufferLen))
	{
		m_BufferLatency = static_cast<double>(outputLatency + m_nAsioBufferLen) / static_cast<double>(m_Settings.Samplerate); // ASIO and OpenMPT semantics of 'latency' differ by one chunk/buffer
	} else
	{
		// pointless value returned from asio driver, use a sane estimate
		m_BufferLatency = 2.0 * static_cast<double>(m_nAsioBufferLen) / static_cast<double>(m_Settings.Samplerate);
	}
}


void CASIODevice::SetRenderSilence(bool silence, bool wait)
//---------------------------------------------------------
{
	MPT_TRACE();
	m_RenderSilence = (silence ? 1 : 0);
	if(!wait)
	{
		return;
	}
	DWORD pollingstart = GetTickCount();
	while(m_RenderingSilence != (silence ? 1u : 0u))
	{
		if(GetTickCount() - pollingstart > 250)
		{
			if(silence)
			{
				if(SourceIsLockedByCurrentThread())
				{
					MPT_ASSERT_MSG(false, "AudioCriticalSection locked while stopping ASIO");
				} else
				{
					MPT_ASSERT_MSG(false, "waiting for asio failed in Stop()");
				}
			} else
			{
				if(SourceIsLockedByCurrentThread())
				{
					MPT_ASSERT_MSG(false, "AudioCriticalSection locked while starting ASIO");
				} else
				{
					MPT_ASSERT_MSG(false, "waiting for asio failed in Start()");
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
	MPT_ASSERT_ALWAYS_MSG(!SourceIsLockedByCurrentThread(), "AudioCriticalSection locked while starting ASIO");

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


bool CASIODevice::InternalIsPlayingSilence() const
//------------------------------------------------
{
	MPT_TRACE();
	return m_Settings.KeepDeviceRunning && m_DeviceRunning && m_RenderSilence.load();
}


void CASIODevice::InternalEndPlayingSilence()
//-------------------------------------------
{
	MPT_TRACE();
	if(!InternalIsPlayingSilence())
	{
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


void CASIODevice::InternalStopAndAvoidPlayingSilence()
//----------------------------------------------------
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
	MPT_ASSERT_ALWAYS_MSG(!SourceIsLockedByCurrentThread(), "AudioCriticalSection locked while stopping ASIO");

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
	m_SampleInputBufferFloat.clear();
	m_SampleInputBufferInt16.clear();
	m_SampleInputBufferInt24.clear();
	m_SampleInputBufferInt32.clear();
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
		MPT_ASSERT_ALWAYS(g_CallbacksInstance == this);
		g_CallbacksInstance = nullptr;
	}
	MemsetZero(m_Callbacks);
	m_BufferInfo.clear();
	m_nAsioBufferLen = 0;
	m_BufferLatency = 0.0;

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
	CLSID clsid = Util::StringToCLSID(mpt::ToWide(GetDeviceInternalID()));
	try
	{
		if(CoCreateInstance(clsid,0,CLSCTX_INPROC_SERVER, clsid, (void **)&m_pAsioDrv) != S_OK)
		{
			Log(mpt::String::Print("ASIO: CoCreateInstance failed!"));
			m_pAsioDrv = nullptr;
			return;
		}
	} catch(...)
	{
		Log(mpt::String::Print("ASIO: CoCreateInstance crashed!"));
		m_pAsioDrv = nullptr;
		return;
	}
	try
	{
		asioCallCheckedBool(init(reinterpret_cast<void *>(m_AppInfo.GetHWND())));
	} catch(const ASIOException &)
	{
		Log(mpt::String::Print("ASIO: init() failed!"));
		CloseDriver();
		return;
	} catch(...)
	{
		Log(mpt::String::Print("ASIO: init() crashed!"));
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
	FillAsioBuffer();
}


void CASIODevice::FillAsioBuffer(bool useSource)
//----------------------------------------------
{
	MPT_TRACE();
	const bool rendersilence = !useSource;
	const std::size_t countChunk = m_nAsioBufferLen;
	const std::size_t inputChannels = m_Settings.InputChannels;
	const std::size_t outputChannels = m_Settings.Channels;
	for(std::size_t inputChannel = 0; inputChannel < inputChannels; ++inputChannel)
	{
		std::size_t channel = inputChannel;
		void *src = m_BufferInfo[channel].buffers[static_cast<unsigned long>(m_BufferIndex) & 1];
		if(IsSampleTypeBigEndian(m_ChannelInfo[channel].type))
		{
			if(src)
			{
				SwapEndian(reinterpret_cast<uint8*>(src), countChunk, GetSampleSize(m_ChannelInfo[channel].type));
			}
		}
		if(m_Settings.sampleFormat == SampleFormatFloat32)
		{
			float *const dstFloat = &m_SampleInputBufferFloat[0];
			if(!src)
			{
				// Skip if we did get no buffer for this channel,
				// Fill with zeroes.
				std::fill(dstFloat, dstFloat + countChunk, 0.0f);
				continue;
			}
			switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTFloat32MSB:
				case ASIOSTFloat32LSB:
					CopyChannelToInterleaved<SC::Convert<float, float> >(dstFloat, reinterpret_cast<float*>(src), inputChannels, countChunk, inputChannel);
					break;
				case ASIOSTFloat64MSB:
				case ASIOSTFloat64LSB:
					CopyChannelToInterleaved<SC::Convert<float, double> >(dstFloat, reinterpret_cast<double*>(src), inputChannels, countChunk, inputChannel);
					break;
				default:
					MPT_ASSERT_NOTREACHED();
					break;
			}
		} else if(m_Settings.sampleFormat == SampleFormatInt16)
		{
			int16 *const dstInt16 = &m_SampleInputBufferInt16[0];
			if(!src)
			{
				// Skip if we did get no buffer for this channel,
				// Fill with zeroes.
				std::fill(dstInt16, dstInt16 + countChunk, 0);
				continue;
			}
			switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTInt16MSB:
				case ASIOSTInt16LSB:
						CopyChannelToInterleaved<SC::Convert<int16, int16> >(dstInt16, reinterpret_cast<int16*>(src), inputChannels, countChunk, inputChannel);
						break;
					default:
						MPT_ASSERT_NOTREACHED();
						break;
			}
		} else if(m_Settings.sampleFormat == SampleFormatInt24)
		{
			int24 *const dstInt24 = &m_SampleInputBufferInt24[0];
			if(!src)
			{
				// Skip if we did get no buffer for this channel,
				// Fill with zeroes.
				std::fill(dstInt24, dstInt24 + countChunk, int24(0));
				continue;
			}
			switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTInt24MSB:
				case ASIOSTInt24LSB:
						CopyChannelToInterleaved<SC::Convert<int24, int24> >(dstInt24, reinterpret_cast<int24*>(src), inputChannels, countChunk, inputChannel);
						break;
					default:
						MPT_ASSERT_NOTREACHED();
						break;
			}
		} else if(m_Settings.sampleFormat == SampleFormatInt32)
		{
			int32 *const dstInt32 = &m_SampleInputBufferInt32[0];
			if(!src)
			{
				// Skip if we did get no buffer for this channel,
				// Fill with zeroes.
				std::fill(dstInt32, dstInt32 + countChunk, 0);
				continue;
			}
			switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTInt16MSB:
				case ASIOSTInt16LSB:
					CopyChannelToInterleaved<SC::Convert<int32, int16> >(dstInt32, reinterpret_cast<int16*>(src), inputChannels, countChunk, inputChannel);
					break;
				case ASIOSTInt24MSB:
				case ASIOSTInt24LSB:
					CopyChannelToInterleaved<SC::Convert<int32, int24> >(dstInt32, reinterpret_cast<int24*>(src), inputChannels, countChunk, inputChannel);
					break;
				case ASIOSTInt32MSB:
				case ASIOSTInt32LSB:
					CopyChannelToInterleaved<SC::Convert<int32, int32> >(dstInt32, reinterpret_cast<int32*>(src), inputChannels, countChunk, inputChannel);
					break;
				case ASIOSTFloat32MSB:
				case ASIOSTFloat32LSB:
					CopyChannelToInterleaved<SC::Convert<int32, float> >(dstInt32, reinterpret_cast<float*>(src), inputChannels, countChunk, inputChannel);
					break;
				case ASIOSTFloat64MSB:
				case ASIOSTFloat64LSB:
					CopyChannelToInterleaved<SC::Convert<int32, double> >(dstInt32, reinterpret_cast<double*>(src), inputChannels, countChunk, inputChannel);
					break;
				case ASIOSTInt32MSB16:
				case ASIOSTInt32LSB16:
					CopyChannelToInterleaved<SC::ConvertShiftUp<int32, int32, 16> >(dstInt32, reinterpret_cast<int32*>(src), inputChannels, countChunk, inputChannel);
					break;
				case ASIOSTInt32MSB18:
				case ASIOSTInt32LSB18:
					CopyChannelToInterleaved<SC::ConvertShiftUp<int32, int32, 14> >(dstInt32, reinterpret_cast<int32*>(src), inputChannels, countChunk, inputChannel);
					break;
				case ASIOSTInt32MSB20:
				case ASIOSTInt32LSB20:
					CopyChannelToInterleaved<SC::ConvertShiftUp<int32, int32, 12> >(dstInt32, reinterpret_cast<int32*>(src), inputChannels, countChunk, inputChannel);
					break;
				case ASIOSTInt32MSB24:
				case ASIOSTInt32LSB24:
					CopyChannelToInterleaved<SC::ConvertShiftUp<int32, int32, 8> >(dstInt32, reinterpret_cast<int32*>(src), inputChannels, countChunk, inputChannel);
					break;
				default:
					MPT_ASSERT_NOTREACHED();
					break;
			}
		} else
		{
			MPT_ASSERT_NOTREACHED();
		}
	}
	if(rendersilence)
	{
		if(m_Settings.sampleFormat == SampleFormatFloat32)
		{
			std::memset(&m_SampleBufferFloat[0], 0, countChunk * outputChannels * sizeof(float));
		} else if(m_Settings.sampleFormat == SampleFormatInt16)
		{
			std::memset(&m_SampleBufferInt16[0], 0, countChunk * outputChannels * sizeof(int16));
		} else if(m_Settings.sampleFormat == SampleFormatInt24)
		{
			std::memset(&m_SampleBufferInt24[0], 0, countChunk * outputChannels * sizeof(int24));
		} else if(m_Settings.sampleFormat == SampleFormatInt32)
		{
			std::memset(&m_SampleBufferInt32[0], 0, countChunk * outputChannels * sizeof(int32));
		} else
		{
			MPT_ASSERT_NOTREACHED();
		}
	} else
	{
		SourceLockedAudioPreRead(countChunk, m_nAsioBufferLen);
		if(m_Settings.sampleFormat == SampleFormatFloat32)
		{
			SourceLockedAudioRead(&m_SampleBufferFloat[0], (m_SampleInputBufferFloat.size() > 0) ? &m_SampleInputBufferFloat[0] : nullptr, countChunk);
		} else if(m_Settings.sampleFormat == SampleFormatInt16)
		{
			SourceLockedAudioRead(&m_SampleBufferInt16[0], (m_SampleInputBufferInt16.size() > 0) ? &m_SampleInputBufferInt16[0] : nullptr, countChunk);
		} else if(m_Settings.sampleFormat == SampleFormatInt24)
		{
			SourceLockedAudioRead(&m_SampleBufferInt24[0], (m_SampleInputBufferInt24.size() > 0) ? &m_SampleInputBufferInt24[0] : nullptr, countChunk);
		} else if(m_Settings.sampleFormat == SampleFormatInt32)
		{
			SourceLockedAudioRead(&m_SampleBufferInt32[0], (m_SampleInputBufferInt32.size() > 0) ? &m_SampleInputBufferInt32[0] : nullptr, countChunk);
		} else
		{
			MPT_ASSERT_NOTREACHED();
		}
	}
	for(std::size_t outputChannel = 0; outputChannel < outputChannels; ++outputChannel)
	{
		std::size_t channel = outputChannel + m_Settings.InputChannels;
		void *dst = m_BufferInfo[channel].buffers[static_cast<unsigned long>(m_BufferIndex) & 1];
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
					CopyInterleavedToChannel<SC::Convert<float, float> >(reinterpret_cast<float*>(dst), srcFloat, outputChannels, countChunk, outputChannel);
					break;
				case ASIOSTFloat64MSB:
				case ASIOSTFloat64LSB:
					CopyInterleavedToChannel<SC::Convert<double, float> >(reinterpret_cast<double*>(dst), srcFloat, outputChannels, countChunk, outputChannel);
					break;
				default:
					MPT_ASSERT_NOTREACHED();
					break;
			}
		} else if(m_Settings.sampleFormat == SampleFormatInt16)
		{
			const int16 *const srcInt16 = &m_SampleBufferInt16[0];
			switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTInt16MSB:
				case ASIOSTInt16LSB:
						CopyInterleavedToChannel<SC::Convert<int16, int16> >(reinterpret_cast<int16*>(dst), srcInt16, outputChannels, countChunk, outputChannel);
						break;
					default:
						MPT_ASSERT_NOTREACHED();
						break;
			}
		} else if(m_Settings.sampleFormat == SampleFormatInt24)
		{
			const int24 *const srcInt24 = &m_SampleBufferInt24[0];
			switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTInt24MSB:
				case ASIOSTInt24LSB:
						CopyInterleavedToChannel<SC::Convert<int24, int24> >(reinterpret_cast<int24*>(dst), srcInt24, outputChannels, countChunk, outputChannel);
						break;
					default:
						MPT_ASSERT_NOTREACHED();
						break;
			}
		} else if(m_Settings.sampleFormat == SampleFormatInt32)
		{
			const int32 *const srcInt32 = &m_SampleBufferInt32[0];
			switch(m_ChannelInfo[channel].type)
			{
				case ASIOSTInt16MSB:
				case ASIOSTInt16LSB:
					CopyInterleavedToChannel<SC::Convert<int16, int32> >(reinterpret_cast<int16*>(dst), srcInt32, outputChannels, countChunk, outputChannel);
					break;
				case ASIOSTInt24MSB:
				case ASIOSTInt24LSB:
					CopyInterleavedToChannel<SC::Convert<int24, int32> >(reinterpret_cast<int24*>(dst), srcInt32, outputChannels, countChunk, outputChannel);
					break;
				case ASIOSTInt32MSB:
				case ASIOSTInt32LSB:
					CopyInterleavedToChannel<SC::Convert<int32, int32> >(reinterpret_cast<int32*>(dst), srcInt32, outputChannels, countChunk, outputChannel);
					break;
				case ASIOSTFloat32MSB:
				case ASIOSTFloat32LSB:
					CopyInterleavedToChannel<SC::Convert<float, int32> >(reinterpret_cast<float*>(dst), srcInt32, outputChannels, countChunk, outputChannel);
					break;
				case ASIOSTFloat64MSB:
				case ASIOSTFloat64LSB:
					CopyInterleavedToChannel<SC::Convert<double, int32> >(reinterpret_cast<double*>(dst), srcInt32, outputChannels, countChunk, outputChannel);
					break;
				case ASIOSTInt32MSB16:
				case ASIOSTInt32LSB16:
					CopyInterleavedToChannel<SC::ConvertShift<int32, int32, 16> >(reinterpret_cast<int32*>(dst), srcInt32, outputChannels, countChunk, outputChannel);
					break;
				case ASIOSTInt32MSB18:
				case ASIOSTInt32LSB18:
					CopyInterleavedToChannel<SC::ConvertShift<int32, int32, 14> >(reinterpret_cast<int32*>(dst), srcInt32, outputChannels, countChunk, outputChannel);
					break;
				case ASIOSTInt32MSB20:
				case ASIOSTInt32LSB20:
					CopyInterleavedToChannel<SC::ConvertShift<int32, int32, 12> >(reinterpret_cast<int32*>(dst), srcInt32, outputChannels, countChunk, outputChannel);
					break;
				case ASIOSTInt32MSB24:
				case ASIOSTInt32LSB24:
					CopyInterleavedToChannel<SC::ConvertShift<int32, int32, 8> >(reinterpret_cast<int32*>(dst), srcInt32, outputChannels, countChunk, outputChannel);
					break;
				default:
					MPT_ASSERT_NOTREACHED();
					break;
			}
		} else
		{
			MPT_ASSERT_NOTREACHED();
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
		SourceLockedAudioDone();
	}
}


bool CASIODevice::InternalHasTimeInfo() const
//-------------------------------------------
{
	MPT_TRACE();
	return m_Settings.UseHardwareTiming;
}


SoundDevice::BufferAttributes CASIODevice::InternalGetEffectiveBufferAttributes() const
//-------------------------------------------------------------------------------------
{
	SoundDevice::BufferAttributes bufferAttributes;
	bufferAttributes.Latency = m_BufferLatency;
	bufferAttributes.UpdateInterval = static_cast<double>(m_nAsioBufferLen) / static_cast<double>(m_Settings.Samplerate);
	bufferAttributes.NumBuffers = 2;
	return bufferAttributes;
}


void CASIODevice::ApplyAsioTimeInfo(AsioTimeInfo asioTimeInfo)
//------------------------------------------------------------
{
	MPT_TRACE();
	if(m_Settings.UseHardwareTiming)
	{
		SoundDevice::TimeInfo timeInfo;
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
			timeInfo.SyncPointStreamFrames = ((((uint64)asioTimeInfo.samplePosition.hi) << 32) | asioTimeInfo.samplePosition.lo) - m_StreamPositionOffset;
			timeInfo.SyncPointSystemTimestamp = (((uint64)asioTimeInfo.systemTime.hi) << 32) | asioTimeInfo.systemTime.lo;
			timeInfo.Speed = speed;
		} else
		{ // spec violation or nothing provided at all, better to estimate this stuff ourselves
			const uint64 asioNow = SourceLockedGetReferenceClockNowNanoseconds();
			timeInfo.SyncPointStreamFrames = m_TotalFramesWritten + m_nAsioBufferLen - m_StreamPositionOffset;
			timeInfo.SyncPointSystemTimestamp = asioNow + Util::Round<int64>(m_BufferLatency * 1000.0 * 1000.0 * 1000.0);
			timeInfo.Speed = 1.0;
		}
		timeInfo.RenderStreamPositionBefore = StreamPositionFromFrames(m_TotalFramesWritten - m_StreamPositionOffset);
		timeInfo.RenderStreamPositionAfter = StreamPositionFromFrames(m_TotalFramesWritten - m_StreamPositionOffset + m_nAsioBufferLen);
		SetTimeInfo(timeInfo);
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
	ASIOError asioresult = ASE_InvalidParameter;
	struct DebugRealtimeThreadIdGuard
	{
		mpt::atomic_uint32_t & ThreadID;
		DebugRealtimeThreadIdGuard(mpt::atomic_uint32_t & ThreadID)
			: ThreadID(ThreadID)
		{
			ThreadID.store(GetCurrentThreadId());
		}
		~DebugRealtimeThreadIdGuard()
		{
			ThreadID.store(0);
		}
	};
	DebugRealtimeThreadIdGuard debugThreadIdGuard(m_DebugRealtimeThreadID);
	MPT_ASSERT(directProcess); // !directProcess is not handled correctly in OpenMPT, would require a separate thread and potentially additional buffering
	if(!directProcess)
	{
		m_UsedFeatures.set(AsioFeatureNoDirectProcess);
	}
	if(m_Settings.UseHardwareTiming)
	{
		AsioTimeInfo asioTimeInfo;
		MemsetZero(asioTimeInfo);
		if(params)
		{
			asioTimeInfo = params->timeInfo;
		} else
		{
			try
			{
				ASIOSamples samplePosition;
				ASIOTimeStamp systemTime;
				MemsetZero(samplePosition);
				MemsetZero(systemTime);
				asioCallUnchecked(&asioresult, getSamplePosition(&samplePosition, &systemTime));
				if(asioresult == ASE_OK)
				{
					AsioTimeInfo asioTimeInfoQueried;
					MemsetZero(asioTimeInfoQueried);
					asioTimeInfoQueried.flags = kSamplePositionValid | kSystemTimeValid;
					asioTimeInfoQueried.samplePosition = samplePosition;
					asioTimeInfoQueried.systemTime = systemTime;
					asioTimeInfoQueried.speed = 1.0;
					ASIOSampleRate sampleRate;
					MemsetZero(sampleRate);
					asioCallUnchecked(&asioresult, getSampleRate(&sampleRate));
					if(asioresult == ASE_OK)
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
		}
		ApplyAsioTimeInfo(asioTimeInfo);
	}
	m_BufferIndex = doubleBufferIndex;
	bool rendersilence = (m_RenderSilence == 1);
	m_RenderingSilence = (rendersilence ? 1 : 0);
	if(rendersilence)
	{
		m_StreamPositionOffset += m_nAsioBufferLen;
		FillAsioBuffer(false);
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
	if(static_cast<double>(m_Settings.Samplerate) * (1.0 - AsioSampleRateTolerance) <= sRate && sRate <= static_cast<double>(m_Settings.Samplerate) * (1.0 + AsioSampleRateTolerance))
	{
		// ignore slight differences which might me due to a unstable external ASIO clock source
		return;
	}
	// play safe and close the device
	RequestClose();
}


static mpt::ustring AsioFeaturesToString(FlagSet<AsioFeatures> features)
//----------------------------------------------------------------------
{
	mpt::ustring result;
	bool first = true;
	if(features[AsioFeatureResetRequest]) { if(!first) { result += MPT_USTRING(","); } first = false; result += MPT_USTRING("reset"); }
	if(features[AsioFeatureResyncRequest]) { if(!first) { result += MPT_USTRING(","); } first = false; result += MPT_USTRING("resync"); }
	if(features[AsioFeatureLatenciesChanged]) { if(!first) { result += MPT_USTRING(","); } first = false; result += MPT_USTRING("latency"); }
	if(features[AsioFeatureBufferSizeChange]) { if(!first) { result += MPT_USTRING(","); } first = false; result += MPT_USTRING("buffer"); }
	if(features[AsioFeatureOverload]) { if(!first) { result += MPT_USTRING(","); } first = false; result += MPT_USTRING("load"); }
	if(features[AsioFeatureNoDirectProcess]) { if(!first) { result += MPT_USTRING(","); } first = false; result += MPT_USTRING("nodirect"); }
	if(features[AsioFeatureSampleRateChange]) { if(!first) { result += MPT_USTRING(","); } first = false; result += MPT_USTRING("srate"); }
	return result;
}


bool CASIODevice::DebugIsFragileDevice() const
//--------------------------------------------
{
	return true;
}


bool CASIODevice::DebugInRealtimeCallback() const
//-----------------------------------------------
{
	return GetCurrentThreadId() == m_DebugRealtimeThreadID.load();
}


SoundDevice::Statistics CASIODevice::GetStatistics() const
//--------------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::Statistics result;
	result.InstantaneousLatency = m_BufferLatency;
	result.LastUpdateInterval = static_cast<double>(m_nAsioBufferLen) / static_cast<double>(m_Settings.Samplerate);
	result.text = mpt::ustring();
	const FlagSet<AsioFeatures> unsupported(AsioFeatureNoDirectProcess | AsioFeatureOverload | AsioFeatureBufferSizeChange | AsioFeatureSampleRateChange);
	FlagSet<AsioFeatures> unsupportedFeatues = m_UsedFeatures;
	unsupportedFeatues &= unsupported;
	if(unsupportedFeatues.any())
	{
		result.text = mpt::String::Print(MPT_USTRING("WARNING: unsupported features: %1"), AsioFeaturesToString(unsupportedFeatues));
	} else if(m_UsedFeatures.any())
	{
		result.text = mpt::String::Print(MPT_USTRING("OK, features used: %1"), AsioFeaturesToString(m_UsedFeatures));
	} else if(m_QueriedFeatures.any())
	{
		result.text = mpt::String::Print(MPT_USTRING("OK, features queried: %1"), AsioFeaturesToString(m_QueriedFeatures));
	} else
	{
		result.text = MPT_USTRING("OK.");
	}
	return result;
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
		m_AsioRequestFlags.fetch_or(AsioRequestFlagLatenciesChanged);
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
	MPT_ASSERT_ALWAYS(g_CallbacksInstance);
	if(!g_CallbacksInstance) return 0;
	return g_CallbacksInstance->AsioMessage(selector, value, message, opt);
}


void CASIODevice::CallbackSampleRateDidChange(ASIOSampleRate sRate)
//-----------------------------------------------------------------
{
	MPT_TRACE();
	MPT_ASSERT_ALWAYS(g_CallbacksInstance);
	if(!g_CallbacksInstance) return;
	g_CallbacksInstance->SampleRateDidChange(sRate);
}


void CASIODevice::CallbackBufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
//------------------------------------------------------------------------------------
{
	MPT_TRACE();
	MPT_ASSERT_ALWAYS(g_CallbacksInstance);
	if(!g_CallbacksInstance) return;
	g_CallbacksInstance->BufferSwitch(doubleBufferIndex, directProcess);
}


ASIOTime* CASIODevice::CallbackBufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
//-------------------------------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	MPT_ASSERT_ALWAYS(g_CallbacksInstance);
	if(!g_CallbacksInstance) return params;
	return g_CallbacksInstance->BufferSwitchTimeInfo(params, doubleBufferIndex, directProcess);
}


void CASIODevice::ReportASIOException(const std::string &str)
//-----------------------------------------------------------
{
	MPT_TRACE();
	SendDeviceMessage(LogError, mpt::ToUnicode(mpt::CharsetLocale, str));
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
	caps.CanInput = true;
	caps.HasNamedInputSources = true;
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
			ASIOError asioresult = ASE_InvalidParameter;
			asioCallUnchecked(&asioresult, canSampleRate((ASIOSampleRate)baseSampleRates[i]));
			if(asioresult == ASE_OK)
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
			mpt::ustring name = mpt::ufmt::dec(i);
			try
			{
				asioCall(getChannelInfo(&channelInfo));
				mpt::String::SetNullTerminator(channelInfo.name);
				name = mpt::ToUnicode(mpt::CharsetLocale, channelInfo.name);
			} catch(...)
			{
				// continue
			}
			caps.channelNames.push_back(name);
		}
		for(long i = 0; i < inputChannels; ++i)
		{
			ASIOChannelInfo channelInfo;
			MemsetZero(channelInfo);
			channelInfo.channel = i;
			channelInfo.isInput = ASIOTrue;
			mpt::ustring name = mpt::ufmt::dec(i);
			try
			{
				asioCall(getChannelInfo(&channelInfo));
				mpt::String::SetNullTerminator(channelInfo.name);
				name = mpt::ToUnicode(mpt::CharsetLocale, channelInfo.name);
			} catch(...)
			{
				// continue
			}
			caps.inputSourceNames.push_back(std::make_pair(static_cast<uint32>(i), name));
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


#endif // MPT_WITH_ASIO


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
