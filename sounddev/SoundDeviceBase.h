/*
 * SoundDeviceBase.h
 * -----------------
 * Purpose: Sound device drivers base class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "SoundDevice.h"

#include "../common/mutex.h"
#include "../common/misc_util.h"
#include "../common/mptAtomic.h"

#include <vector>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


//========
class Base
//========
	: public IBase
{

private:

	SoundDevice::ISource *m_Source;
	SoundDevice::IMessageReceiver *m_MessageReceiver;

	const SoundDevice::Info m_Info;

private:

	SoundDevice::Caps m_Caps;

protected:

	SoundDevice::SysInfo m_SysInfo;
	SoundDevice::AppInfo m_AppInfo;
	SoundDevice::Settings m_Settings;
	SoundDevice::Flags m_Flags;
	bool m_DeviceUnavailableOnOpen;

private:

	bool m_IsPlaying;

	SoundDevice::TimeInfo m_TimeInfo;

	int64 m_StreamPositionRenderFrames; // only updated or read in audio CALLBACK or when device is stopped. requires no further locking

	mutable mpt::mutex m_StreamPositionMutex;
	int64 m_StreamPositionOutputFrames;

	mpt::atomic_uint32_t m_RequestFlags;

public:

	SoundDevice::SysInfo GetSysInfo() const { return m_SysInfo; }
	SoundDevice::AppInfo GetAppInfo() const { return m_AppInfo; }

protected:

	SoundDevice::Type GetDeviceType() const { return m_Info.type; }
	mpt::ustring GetDeviceInternalID() const { return m_Info.internalID; }
	SoundDevice::Identifier GetDeviceIdentifier() const { return m_Info.GetIdentifier(); }

	virtual void InternalFillAudioBuffer() = 0;

	uint64 SourceGetReferenceClockNowNanoseconds() const;
	void SourceNotifyPreStart();
	void SourceNotifyPostStop();
	bool SourceIsLockedByCurrentThread() const;
	void SourceFillAudioBufferLocked();
	void SourceAudioPreRead(std::size_t numFrames, std::size_t framesLatency);
	void SourceAudioRead(void *buffer, const void *inputBuffer, std::size_t numFrames);
	void SourceAudioDone();

	void RequestClose() { m_RequestFlags.fetch_or(RequestFlagClose); }
	void RequestReset() { m_RequestFlags.fetch_or(RequestFlagReset); }
	void RequestRestart() { m_RequestFlags.fetch_or(RequestFlagRestart); }

	void SendDeviceMessage(LogLevel level, const mpt::ustring &str);

protected:

	void SetTimeInfo(SoundDevice::TimeInfo timeInfo) { m_TimeInfo = timeInfo; }

	SoundDevice::StreamPosition StreamPositionFromFrames(int64 frames) const { return SoundDevice::StreamPosition(frames, static_cast<double>(frames) / static_cast<double>(m_Settings.Samplerate)); }

	virtual bool InternalHasTimeInfo() const { return false; }

	virtual bool InternalHasGetStreamPosition() const { return false; }
	virtual int64 InternalGetStreamPositionFrames() const { return 0; }

	virtual bool InternalIsOpen() const = 0;

	virtual bool InternalOpen() = 0;
	virtual bool InternalStart() = 0;
	virtual void InternalStop() = 0;
	virtual void InternalStopForce() { InternalStop(); }
	virtual bool InternalClose() = 0;

	virtual SoundDevice::Caps InternalGetDeviceCaps() = 0;

	virtual SoundDevice::BufferAttributes InternalGetEffectiveBufferAttributes() const = 0;

protected:

	Base(SoundDevice::Info info, SoundDevice::SysInfo sysInfo);

public:

	virtual ~Base();

	void SetSource(SoundDevice::ISource *source) { m_Source = source; }
	void SetMessageReceiver(SoundDevice::IMessageReceiver *receiver) { m_MessageReceiver = receiver; }

	SoundDevice::Info GetDeviceInfo() const { return m_Info; }

	SoundDevice::Caps GetDeviceCaps() const { return m_Caps; }
	virtual SoundDevice::DynamicCaps GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates);

	bool Init(const SoundDevice::AppInfo &appInfo);
	bool Open(const SoundDevice::Settings &settings);
	bool Close();
	bool Start();
	void Stop(bool force = false);

	FlagSet<RequestFlags> GetRequestFlags() const { return FlagSet<RequestFlags>(m_RequestFlags.load()); }

	bool IsInited() const { return m_Caps.Available; }
	bool IsOpen() const { return IsInited() && InternalIsOpen(); }
	bool IsAvailable() const { return m_Caps.Available && !m_DeviceUnavailableOnOpen; }
	bool IsPlaying() const { return m_IsPlaying; }

	virtual bool OnIdle() { return false; }

	SoundDevice::Settings GetSettings() const { return m_Settings; }
	SampleFormat GetActualSampleFormat() const { return IsOpen() ? m_Settings.sampleFormat : SampleFormat(SampleFormatInvalid); }
	SoundDevice::BufferAttributes GetEffectiveBufferAttributes() const { return (IsOpen() && IsPlaying()) ? InternalGetEffectiveBufferAttributes() : SoundDevice::BufferAttributes(); }

	SoundDevice::TimeInfo GetTimeInfo() const { return m_TimeInfo; }
	SoundDevice::StreamPosition GetStreamPosition() const;

	virtual bool DebugIsFragileDevice() const { return false; }
	virtual bool DebugInRealtimeCallback() const { return false; }

	virtual SoundDevice::Statistics GetStatistics() const;

	virtual bool OpenDriverSettings() { return false; };

};


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
