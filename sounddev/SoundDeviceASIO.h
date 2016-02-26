/*
 * SoundDeviceASIO.h
 * -----------------
 * Purpose: ASIO sound device driver class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "SoundDeviceBase.h"

#include "../common/ComponentManager.h"

#include "../common/FlagSet.h"

#ifdef MPT_WITH_ASIO
#include <iasiodrv.h>
#endif // MPT_WITH_ASIO

OPENMPT_NAMESPACE_BEGIN

namespace SoundDevice {

#ifdef MPT_WITH_ASIO

class ComponentASIO : public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentASIO() { }
	virtual ~ComponentASIO() { }
	virtual bool DoInitialize() { return true; }
};

enum AsioFeatures
{
	AsioFeatureResetRequest     = 1<<0,
	AsioFeatureResyncRequest    = 1<<1,
	AsioFeatureLatenciesChanged = 1<<2,
	AsioFeatureBufferSizeChange = 1<<3,
	AsioFeatureOverload         = 1<<4,
	AsioFeatureNoDirectProcess  = 1<<5,
	AsioFeatureSampleRateChange = 1<<6,
	AsioFeatureNone = 0
};
DECLARE_FLAGSET(AsioFeatures)

//====================================
class CASIODevice: public SoundDevice::Base
//====================================
{
	friend class TemporaryASIODriverOpener;

protected:

	IASIO *m_pAsioDrv;

	double m_BufferLatency;
	long m_nAsioBufferLen;
	std::vector<ASIOBufferInfo> m_BufferInfo;
	ASIOCallbacks m_Callbacks;
	static CASIODevice *g_CallbacksInstance; // only 1 opened instance allowed for ASIO
	bool m_BuffersCreated;
	std::vector<ASIOChannelInfo> m_ChannelInfo;
	std::vector<float> m_SampleBufferFloat;
	std::vector<int16> m_SampleBufferInt16;
	std::vector<int24> m_SampleBufferInt24;
	std::vector<int32> m_SampleBufferInt32;
	std::vector<float> m_SampleInputBufferFloat;
	std::vector<int16> m_SampleInputBufferInt16;
	std::vector<int24> m_SampleInputBufferInt24;
	std::vector<int32> m_SampleInputBufferInt32;
	bool m_CanOutputReady;

	bool m_DeviceRunning;
	uint64 m_TotalFramesWritten;
	long m_BufferIndex;
	LONG m_RenderSilence;
	LONG m_RenderingSilence;

	int64 m_StreamPositionOffset;

	static const LONG AsioRequestFlagLatenciesChanged = 1<<0;
	LONG m_AsioRequestFlags;

	FlagSet<AsioFeatures> m_QueriedFeatures;
	FlagSet<AsioFeatures> m_UsedFeatures;

	mutable mpt::atomic_uint32_t m_DebugRealtimeThreadID;

private:
	void ApplyAsioTimeInfo(AsioTimeInfo asioTimeInfo);

	static bool IsSampleTypeFloat(ASIOSampleType sampleType);
	static bool IsSampleTypeInt16(ASIOSampleType sampleType);
	static bool IsSampleTypeInt24(ASIOSampleType sampleType);
	static std::size_t GetSampleSize(ASIOSampleType sampleType);
	static bool IsSampleTypeBigEndian(ASIOSampleType sampleType);
	
	void SetRenderSilence(bool silence, bool wait=false);

public:
	CASIODevice(SoundDevice::Info info, SoundDevice::SysInfo sysInfo);
	~CASIODevice();

private:
	void InitMembers();
	bool HandleRequests(); // return true if any work has been done
	void UpdateLatency();

	void InternalStopImpl(bool force);

public:
	bool InternalOpen();
	bool InternalClose();
	void InternalFillAudioBuffer();
	bool InternalStart();
	void InternalStop();
	void InternalStopForce();
	bool InternalIsOpen() const { return m_BuffersCreated; }

	bool OnIdle() { return HandleRequests(); }

	SoundDevice::Caps InternalGetDeviceCaps();
	SoundDevice::DynamicCaps GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates);

	bool OpenDriverSettings();

	bool DebugIsFragileDevice() const;
	bool DebugInRealtimeCallback() const;

	SoundDevice::Statistics GetStatistics() const;

public:
	static std::vector<SoundDevice::Info> EnumerateDevices(SoundDevice::SysInfo sysInfo);

protected:
	void OpenDriver();
	void CloseDriver();
	bool IsDriverOpen() const { return (m_pAsioDrv != nullptr); }

	bool InternalHasTimeInfo() const;

	SoundDevice::BufferAttributes InternalGetEffectiveBufferAttributes() const;

protected:
	void FillAsioBuffer(bool useSource = true);

	long AsioMessage(long selector, long value, void* message, double* opt);
	void SampleRateDidChange(ASIOSampleRate sRate);
	void BufferSwitch(long doubleBufferIndex, ASIOBool directProcess);
	ASIOTime* BufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);

	static long CallbackAsioMessage(long selector, long value, void* message, double* opt);
	static void CallbackSampleRateDidChange(ASIOSampleRate sRate);
	static void CallbackBufferSwitch(long doubleBufferIndex, ASIOBool directProcess);
	static ASIOTime* CallbackBufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);
	
	void ReportASIOException(const std::string &str);
};

#endif // MPT_WITH_ASIO


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
