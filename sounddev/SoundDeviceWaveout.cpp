/*
 * SoundDeviceWaveout.cpp
 * ----------------------
 * Purpose: Waveout sound device driver class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDevice.h"
#include "SoundDeviceUtilities.h"

#include "SoundDeviceWaveout.h"

#include "../common/misc_util.h"


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


#if MPT_OS_WINDOWS

MPT_REGISTERED_COMPONENT(ComponentWaveOut, "WaveOut")


static const std::size_t WAVEOUT_MINBUFFERS = 3;
static const std::size_t WAVEOUT_MAXBUFFERS = 4096;
static const std::size_t WAVEOUT_MINBUFFERFRAMECOUNT = 8;
static const std::size_t WAVEOUT_MAXBUFFERSIZE = 16384; // fits in int16


CWaveDevice::CWaveDevice(SoundDevice::Info info, SoundDevice::SysInfo sysInfo)
//----------------------------------------------------------------------------
	: CSoundDeviceWithThread(info, sysInfo)
{
	MPT_TRACE();
	m_ThreadWakeupEvent;
	m_Failed = false;
	m_hWaveOut = NULL;
	m_nWaveBufferSize = 0;
	m_JustStarted = false;
	m_nPreparedHeaders = 0;
}


CWaveDevice::~CWaveDevice()
//-------------------------
{
	MPT_TRACE();
	Close();
}


int CWaveDevice::GetDeviceIndex() const
//-------------------------------------
{
	return ConvertStrTo<int>(GetDeviceInternalID());
}


SoundDevice::Caps CWaveDevice::InternalGetDeviceCaps()
//--------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::Caps caps;
	caps.Available = true;
	caps.CanUpdateInterval = true;
	caps.CanSampleFormat = true;
	caps.CanExclusiveMode = (GetDeviceIndex() > 0); // no direct mode for WAVE_MAPPER, makes no sense there
	caps.CanBoostThreadPriority = true;
	caps.CanKeepDeviceRunning = false;
	caps.CanUseHardwareTiming = false;
	caps.CanChannelMapping = false;
	caps.CanInput = false;
	caps.HasNamedInputSources = false;
	caps.CanDriverPanel = false;
	caps.HasInternalDither = false;
	caps.ExclusiveModeDescription = MPT_USTRING("Use direct mode");
	if(GetSysInfo().IsWine)
	{
		caps.DefaultSettings.sampleFormat = SampleFormatInt16;
	} else if(GetSysInfo().WindowsVersion.IsAtLeast(mpt::Windows::Version::WinVista))
	{
		caps.DefaultSettings.sampleFormat = SampleFormatFloat32;
	} else
	{
		caps.DefaultSettings.sampleFormat = SampleFormatInt16;
	}
	return caps;
}


SoundDevice::DynamicCaps CWaveDevice::GetDeviceDynamicCaps(const std::vector<uint32> & /*baseSampleRates*/ )
//--------------------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::DynamicCaps caps;
	WAVEOUTCAPSW woc;
	MemsetZero(woc);
	if(GetDeviceIndex() > 0)
	{
		if(waveOutGetDevCapsW(GetDeviceIndex() - 1, &woc, sizeof(woc)) == MMSYSERR_NOERROR)
		{
			if(woc.dwFormats & (WAVE_FORMAT_96M08 | WAVE_FORMAT_96M16	| WAVE_FORMAT_96S08 | WAVE_FORMAT_96S16))
			{
				caps.supportedExclusiveSampleRates.push_back(96000);
			}
			if(woc.dwFormats & (WAVE_FORMAT_48M08 | WAVE_FORMAT_48M16	| WAVE_FORMAT_48S08 | WAVE_FORMAT_48S16))
			{
				caps.supportedExclusiveSampleRates.push_back(48000);
			}
			if(woc.dwFormats & (WAVE_FORMAT_4M08 | WAVE_FORMAT_4M16	| WAVE_FORMAT_4S08 | WAVE_FORMAT_4S16))
			{
				caps.supportedExclusiveSampleRates.push_back(44100);
			}
			if(woc.dwFormats & (WAVE_FORMAT_2M08 | WAVE_FORMAT_2M16	| WAVE_FORMAT_2S08 | WAVE_FORMAT_2S16))
			{
				caps.supportedExclusiveSampleRates.push_back(22050);
			}
			if(woc.dwFormats & (WAVE_FORMAT_1M08 | WAVE_FORMAT_1M16	| WAVE_FORMAT_1S08 | WAVE_FORMAT_1S16))
			{
				caps.supportedExclusiveSampleRates.push_back(11025);
			}
		}
	}
	return caps;
}


bool CWaveDevice::InternalOpen()
//------------------------------
{
	MPT_TRACE();
	if(m_Settings.InputChannels > 0)
	{
		return false;
	}
	WAVEFORMATEXTENSIBLE wfext;
	if(!FillWaveFormatExtensible(wfext, m_Settings))
	{
		return false;
	}
	WAVEFORMATEX *pwfx = &wfext.Format;
	UINT nWaveDev = GetDeviceIndex();
	nWaveDev = (nWaveDev > 0) ? nWaveDev - 1 : WAVE_MAPPER;
	m_ThreadWakeupEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(m_ThreadWakeupEvent == INVALID_HANDLE_VALUE)
	{
		InternalClose();
		return false;
	}
	m_Failed = false;
	m_DriverBugs = 0;
	m_hWaveOut = NULL;
	if(waveOutOpen(&m_hWaveOut, nWaveDev, pwfx, (DWORD_PTR)WaveOutCallBack, (DWORD_PTR)this, CALLBACK_FUNCTION | (m_Settings.ExclusiveMode ? WAVE_FORMAT_DIRECT : 0)) != MMSYSERR_NOERROR)
	{
		InternalClose();
		return false;
	}
	if(waveOutPause(m_hWaveOut) != MMSYSERR_NOERROR)
	{
		InternalClose();
		return false;
	}
	m_nWaveBufferSize = Util::Round<int32>(m_Settings.UpdateInterval * pwfx->nAvgBytesPerSec);
	m_nWaveBufferSize = Util::AlignUp<uint32>(m_nWaveBufferSize, pwfx->nBlockAlign);
	m_nWaveBufferSize = Clamp(m_nWaveBufferSize, static_cast<uint32>(WAVEOUT_MINBUFFERFRAMECOUNT * pwfx->nBlockAlign), static_cast<uint32>(Util::AlignDown<uint32>(WAVEOUT_MAXBUFFERSIZE, pwfx->nBlockAlign)));
	std::size_t numBuffers = Util::Round<int32>(m_Settings.Latency * pwfx->nAvgBytesPerSec / m_nWaveBufferSize);
	numBuffers = Clamp(numBuffers, WAVEOUT_MINBUFFERS, WAVEOUT_MAXBUFFERS);
	m_nPreparedHeaders = 0;
	m_WaveBuffers.resize(numBuffers);
	m_WaveBuffersData.resize(numBuffers);
	for(std::size_t buf = 0; buf < numBuffers; ++buf)
	{
		MemsetZero(m_WaveBuffers[buf]);
		m_WaveBuffersData[buf].resize(m_nWaveBufferSize);
		m_WaveBuffers[buf].dwFlags = 0;
		m_WaveBuffers[buf].lpData = &m_WaveBuffersData[buf][0];
		m_WaveBuffers[buf].dwBufferLength = m_nWaveBufferSize;
		if(waveOutPrepareHeader(m_hWaveOut, &m_WaveBuffers[buf], sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
		{
			break;
		}
		m_WaveBuffers[buf].dwFlags |= WHDR_DONE;
		m_nPreparedHeaders++;
	}
	if(!m_nPreparedHeaders)
	{
		InternalClose();
		return false;
	}
	m_nBuffersPending = 0;
	m_nWriteBuffer = 0;
	m_nDoneBuffer = 0;
	{
		mpt::lock_guard<mpt::mutex> guard(m_PositionWraparoundMutex);
		MemsetZero(m_PositionLast);
		m_PositionWrappedCount = 0;
	}
	SetWakeupEvent(m_ThreadWakeupEvent);
	SetWakeupInterval(m_nWaveBufferSize * 1.0 / m_Settings.GetBytesPerSecond());
	m_Flags.NeedsClippedFloat = GetSysInfo().WindowsVersion.IsAtLeast(mpt::Windows::Version::WinVista);
	return true;
}


bool CWaveDevice::InternalClose()
//-------------------------------
{
	MPT_TRACE();
	if(m_hWaveOut)
	{
		waveOutReset(m_hWaveOut);
		m_JustStarted = false;
		InterlockedExchange(&m_nBuffersPending, 0);
		m_nWriteBuffer = 0;
		m_nDoneBuffer = 0;
		while(m_nPreparedHeaders > 0)
		{
			m_nPreparedHeaders--;
			waveOutUnprepareHeader(m_hWaveOut, &m_WaveBuffers[m_nPreparedHeaders], sizeof(WAVEHDR));
		}
		waveOutClose(m_hWaveOut);
		m_hWaveOut = NULL;
	}
	#ifdef _DEBUG
		if(m_DriverBugs.load())
		{
				SendDeviceMessage(LogError, MPT_USTRING("Errors were detected while playing sound:\n") + GetStatistics().text);
		}
	#endif
	m_DriverBugs = 0;
	m_Failed = false;
	if(m_ThreadWakeupEvent)
	{
		CloseHandle(m_ThreadWakeupEvent);
		m_ThreadWakeupEvent = NULL;
	}
	{
		mpt::lock_guard<mpt::mutex> guard(m_PositionWraparoundMutex);
		MemsetZero(m_PositionLast);
		m_PositionWrappedCount = 0;
	}
	return true;
}


void CWaveDevice::StartFromSoundThread()
//--------------------------------------
{
	MPT_TRACE();
	if(m_hWaveOut)
	{
		{
			mpt::lock_guard<mpt::mutex> guard(m_PositionWraparoundMutex);
			MemsetZero(m_PositionLast);
			m_PositionWrappedCount = 0;
		}
		m_JustStarted = true;
		// Actual starting is done in InternalFillAudioBuffer to avoid crackling with tiny buffers.
	}
}


void CWaveDevice::StopFromSoundThread()
//-------------------------------------
{
	MPT_TRACE();
	if(m_hWaveOut)
	{
		CheckResult(waveOutPause(m_hWaveOut));
		m_JustStarted = false;
		{
			mpt::lock_guard<mpt::mutex> guard(m_PositionWraparoundMutex);
			MemsetZero(m_PositionLast);
			m_PositionWrappedCount = 0;
		}
	}
}


bool CWaveDevice::CheckResult(MMRESULT result)
//--------------------------------------------
{
	if(result == MMSYSERR_NOERROR)
	{
		return true;
	}
	if(!m_Failed)
	{ // only show the first error
		m_Failed = true;
		WCHAR errortext[MAXERRORLENGTH + 1];
		MemsetZero(errortext);
		waveOutGetErrorTextW(result, errortext, MAXERRORLENGTH);
		SendDeviceMessage(LogError, mpt::format(MPT_USTRING("WaveOut error: 0x%1: %2"))(mpt::ufmt::hex0<8>(result), mpt::ToUnicode(errortext)));
	}
	RequestClose();
	return false;
}


bool CWaveDevice::CheckResult(MMRESULT result, DWORD param)
//---------------------------------------------------------
{
	if(result == MMSYSERR_NOERROR)
	{
		return true;
	}
	if(!m_Failed)
	{ // only show the first error
		m_Failed = true;
		WCHAR errortext[MAXERRORLENGTH + 1];
		MemsetZero(errortext);
		waveOutGetErrorTextW(result, errortext, MAXERRORLENGTH);
		SendDeviceMessage(LogError, mpt::format(MPT_USTRING("WaveOut error: 0x%1 (param 0x%2): %3"))
			( mpt::ufmt::hex0<8>(result)
			, mpt::ufmt::hex0<8>(param)
			, mpt::ToUnicode(errortext)
			));
	}
	RequestClose();
	return false;
}


void CWaveDevice::InternalFillAudioBuffer()
//-----------------------------------------
{
	MPT_TRACE();
	if(!m_hWaveOut)
	{
		return;
	}
	
	const std::size_t bytesPerFrame = m_Settings.GetBytesPerFrame();

	ULONG oldBuffersPending = InterlockedExchangeAdd(&m_nBuffersPending, 0); // read
	ULONG nLatency = oldBuffersPending * m_nWaveBufferSize;

	ULONG nBytesWritten = 0;
	while((oldBuffersPending < m_nPreparedHeaders) && !m_Failed)
	{
		DWORD oldFlags = static_cast<volatile WAVEHDR*>(&(m_WaveBuffers[m_nWriteBuffer]))->dwFlags;
		if((oldFlags & WHDR_INQUEUE))
		{
			m_DriverBugs.fetch_or(DriverBugBufferFillAndHeaderInQueue);
		}
		if(!(oldFlags & WHDR_DONE))
		{
			m_DriverBugs.fetch_or(DriverBugBufferFillAndHeaderNotDone);
		}
		if((oldFlags & WHDR_INQUEUE))
		{
			if(m_DriverBugs.load() & DriverBugDoneNotificationOutOfOrder)
			{
				//  Some drivers/setups can return WaveHeader notifications out of
				// order. WaveHeaders which have not yet been notified to be ready stay
				// in the INQUEUE and !DONE state internally and cannot be reused yet
				// even though they causally should be able to. waveOutWrite fails for
				// them.
				//  In this case we skip filling the buffers until we actually see the
				// next expected buffer to be ready for refilling.
				//  This problem has been spotted on Wine 1.7.46 (non-official packages)
				// running on Debian 8 Jessie 32bit. It may also be related to WaveOut
				// playback being too fast and crackling which had benn reported on
				// Wine 1.6 + WinePulse on UbuntuStudio 12.04 32bit (this has not been
				// verified yet because the problem is not always reproducable on the
				// system in question).
				return;
			}
		}
		nLatency += m_nWaveBufferSize;
		SourceLockedAudioPreRead(m_nWaveBufferSize / bytesPerFrame, nLatency / bytesPerFrame);
		SourceLockedAudioRead(m_WaveBuffers[m_nWriteBuffer].lpData, nullptr, m_nWaveBufferSize / bytesPerFrame);
		nBytesWritten += m_nWaveBufferSize;
		m_WaveBuffers[m_nWriteBuffer].dwFlags &= ~(WHDR_INQUEUE|WHDR_DONE);
		m_WaveBuffers[m_nWriteBuffer].dwBufferLength = m_nWaveBufferSize;
		InterlockedIncrement(&m_nBuffersPending);
		oldBuffersPending++; // increment separately to avoid looping without leaving at all when rendering takes more than 100% CPU
		CheckResult(waveOutWrite(m_hWaveOut, &m_WaveBuffers[m_nWriteBuffer], sizeof(WAVEHDR)), oldFlags);
		m_nWriteBuffer++;
		m_nWriteBuffer %= m_nPreparedHeaders;
		SourceLockedAudioDone();
	}

	if(m_JustStarted && !m_Failed)
	{
		// Fill the buffers completely before starting the stream.
		// This avoids buffer underruns which result in audible crackling with small buffers.
		m_JustStarted = false;
		CheckResult(waveOutRestart(m_hWaveOut));
	}

}


int64 CWaveDevice::InternalGetStreamPositionFrames() const
//---------------------------------------------------------
{
	// Apparently, at least with Windows XP, TIME_SAMPLES wraps aroud at 0x7FFFFFF (see
	// http://www.tech-archive.net/Archive/Development/microsoft.public.win32.programmer.mmedia/2005-02/0070.html
	// ).
	// We may also, additionally, default to TIME_BYTES which would wraparound the earliest.
	// We could thereby try to avoid any potential wraparound inside the driver on older
	// Windows versions, which would be, once converted into other units, really
	// difficult to detect or handle.
	static const UINT timeType = TIME_SAMPLES; // shpuld work for sane systems
	//static const std::size_t valid_bits = 32; // shpuld work for sane systems
	//static const UINT timeType = TIME_BYTES; // safest
	static const std::size_t valid_bits = 27; // safe for WinXP TIME_SAMPLES
	static const uint32 valid_mask = static_cast<uint32>((uint64(1) << valid_bits) - 1u);
	static const uint32 valid_watermark = static_cast<uint32>(uint64(1) << (valid_bits - 1u)); // half the valid range in order to be able to catch backwards fluctuations

	MMTIME mmtime;
	MemsetZero(mmtime);
	mmtime.wType = timeType;
	if(waveOutGetPosition(m_hWaveOut, &mmtime, sizeof(mmtime)) != MMSYSERR_NOERROR)
	{
		return 0;
	}
	if(mmtime.wType != TIME_MS && mmtime.wType != TIME_BYTES && mmtime.wType != TIME_SAMPLES)
	{ // unsupported time format
		return 0;
	}
	int64 offset = 0;
	{
		// handle wraparound
		mpt::lock_guard<mpt::mutex> guard(m_PositionWraparoundMutex);
		if(!m_PositionLast.wType)
		{
			// first call
			m_PositionWrappedCount = 0;
		} else if(mmtime.wType != m_PositionLast.wType)
		{
			// what? value type changed, do not try handling that for now.
			m_PositionWrappedCount = 0;
		} else
		{
			DWORD oldval = 0;
			DWORD curval = 0;
			switch(mmtime.wType)
			{
				case TIME_MS: oldval = m_PositionLast.u.ms; curval = mmtime.u.ms; break;
				case TIME_BYTES: oldval = m_PositionLast.u.cb; curval = mmtime.u.cb; break;
				case TIME_SAMPLES: oldval = m_PositionLast.u.sample; curval = mmtime.u.sample; break;
			}
			oldval &= valid_mask;
			curval &= valid_mask;
			if(((curval - oldval) & valid_mask) >= valid_watermark) // guard against driver problems resulting in time jumping backwards for short periods of time. BEWARE of integer wraparound when refactoring
			{
				curval = oldval;
			}
			switch(mmtime.wType)
			{
				case TIME_MS: mmtime.u.ms = curval; break;
				case TIME_BYTES: mmtime.u.cb = curval; break;
				case TIME_SAMPLES: mmtime.u.sample = curval; break;
			}
			if((curval ^ oldval) & valid_watermark) // MSB flipped
			{
				if(!(curval & valid_watermark)) // actually wrapped
				{
					m_PositionWrappedCount += 1;
				}
			}
		}
		m_PositionLast = mmtime;
		offset = (static_cast<uint64>(m_PositionWrappedCount) << valid_bits);
	}
	int64 result = 0;
	switch(mmtime.wType)
	{
		case TIME_MS: result += (static_cast<int64>(mmtime.u.ms & valid_mask) + offset) * m_Settings.GetBytesPerSecond() / (1000 * m_Settings.GetBytesPerFrame()); break;
		case TIME_BYTES: result += (static_cast<int64>(mmtime.u.cb & valid_mask) + offset) / m_Settings.GetBytesPerFrame(); break;
		case TIME_SAMPLES: result += (static_cast<int64>(mmtime.u.sample & valid_mask) + offset); break;
	}
	return result;
}


void CWaveDevice::HandleWaveoutDone(WAVEHDR *hdr)
//-----------------------------------------------
{
	MPT_TRACE();
	DWORD flags = static_cast<volatile WAVEHDR*>(hdr)->dwFlags;
	uint32 hdrIndex = hdr - &(m_WaveBuffers[0]);
	if(hdrIndex != m_nDoneBuffer)
	{
		m_DriverBugs.fetch_or(DriverBugDoneNotificationOutOfOrder);
	}
	if(!(flags & WHDR_DONE))
	{
		m_DriverBugs.fetch_or(DriverBugDoneNotificationAndHeaderNotDone);
	}
	if((flags & WHDR_INQUEUE))
	{
		m_DriverBugs.fetch_or(DriverBugDoneNotificationAndHeaderInQueue);
	}
	m_nDoneBuffer += 1;
	m_nDoneBuffer %= m_nPreparedHeaders;
	InterlockedDecrement(&m_nBuffersPending);
	SetEvent(m_ThreadWakeupEvent);
}


void CWaveDevice::WaveOutCallBack(HWAVEOUT, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR param1, DWORD_PTR /* param2 */)
//----------------------------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	if((uMsg == WOM_DONE) && (dwUser))
	{
		CWaveDevice *that = (CWaveDevice *)dwUser;
		that->HandleWaveoutDone((WAVEHDR*)param1);
	}
}


SoundDevice::BufferAttributes CWaveDevice::InternalGetEffectiveBufferAttributes() const
//-------------------------------------------------------------------------------------
{
	SoundDevice::BufferAttributes bufferAttributes;
	bufferAttributes.Latency = m_nWaveBufferSize * m_nPreparedHeaders * 1.0 / m_Settings.GetBytesPerSecond();
	bufferAttributes.UpdateInterval = m_nWaveBufferSize * 1.0 / m_Settings.GetBytesPerSecond();
	bufferAttributes.NumBuffers = m_nPreparedHeaders;
	return bufferAttributes;
}


SoundDevice::Statistics CWaveDevice::GetStatistics() const
//--------------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::Statistics result;
	result.InstantaneousLatency = InterlockedExchangeAdd(&m_nBuffersPending, 0) * m_nWaveBufferSize * 1.0 / m_Settings.GetBytesPerSecond();
	result.LastUpdateInterval = 1.0 * m_nWaveBufferSize / m_Settings.GetBytesPerSecond();
	uint32 bugs = m_DriverBugs.load();
	if(bugs != 0)
	{
		result.text = mpt::format(MPT_USTRING("Problematic driver detected! Error flags: %1"))(mpt::ufmt::hex0<8>(bugs));
	} else
	{
		result.text = mpt::format(MPT_USTRING("Driver working as expected."))();
	}
	return result;
}


std::vector<SoundDevice::Info> CWaveDevice::EnumerateDevices(SoundDevice::SysInfo /* sysInfo */ )
//-----------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	std::vector<SoundDevice::Info> devices;
	UINT numDevs = waveOutGetNumDevs();
	for(UINT index = 0; index <= numDevs; ++index)
	{
		SoundDevice::Info info;
		info.type = TypeWAVEOUT;
		info.internalID = mpt::ufmt::dec(index);
		info.apiName = MPT_USTRING("WaveOut");
		info.useNameAsIdentifier = true;
		WAVEOUTCAPSW woc;
		MemsetZero(woc);
		if(waveOutGetDevCapsW((index == 0) ? WAVE_MAPPER : (index - 1), &woc, sizeof(woc)) == MMSYSERR_NOERROR)
		{
			info.name = mpt::ToUnicode(woc.szPname);
			info.extraData[MPT_USTRING("DriverID")] = mpt::format(MPT_USTRING("%1:%2"))(mpt::ufmt::hex0<4>(woc.wMid), mpt::ufmt::hex0<4>(woc.wPid));
			info.extraData[MPT_USTRING("DriverVersion")] = mpt::format(MPT_USTRING("%3.%4"))(mpt::ufmt::dec((static_cast<uint32>(woc.vDriverVersion) >> 24) & 0xff), mpt::ufmt::dec((static_cast<uint32>(woc.vDriverVersion) >>  0) & 0xff));
		} else if(index == 0)
		{
			info.name = mpt::format(MPT_USTRING("Auto (Wave Mapper)"))();
		} else
		{
			info.name = mpt::format(MPT_USTRING("Device %1"))(index - 1);
		}
		info.isDefault = (index == 0);
		devices.push_back(info);
	}
	return devices;
}

#endif // MPT_OS_WINDOWS


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
