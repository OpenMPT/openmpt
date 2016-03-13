/*
 * SoundDeviceWaveout.h
 * --------------------
 * Purpose: Waveout sound device driver class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "SoundDeviceBase.h"
#include "SoundDeviceUtilities.h"

#include "../common/ComponentManager.h"

#if MPT_OS_WINDOWS
#include <MMSystem.h>
#endif // MPT_OS_WINDOWS


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


#if MPT_OS_WINDOWS

class ComponentWaveOut : public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentWaveOut() { }
	virtual ~ComponentWaveOut() { }
	virtual bool DoInitialize() { return true; }
};


//==============================================
class CWaveDevice: public CSoundDeviceWithThread
//==============================================
{
protected:
	HANDLE m_ThreadWakeupEvent;
	bool m_Failed;
	HWAVEOUT m_hWaveOut;
	ULONG m_nWaveBufferSize;
	bool m_JustStarted;
	ULONG m_nPreparedHeaders;
	ULONG m_nWriteBuffer;
	mutable LONG m_nBuffersPending;
	std::vector<WAVEHDR> m_WaveBuffers;
	std::vector<std::vector<char> > m_WaveBuffersData;

	mutable mpt::mutex m_PositionWraparoundMutex;
	mutable MMTIME m_PositionLast;
	mutable std::size_t m_PositionWrappedCount;

public:
	CWaveDevice(SoundDevice::Info info, SoundDevice::SysInfo sysInfo);
	~CWaveDevice();

public:
	bool InternalOpen();
	bool InternalClose();
	void InternalFillAudioBuffer();
	void StartFromSoundThread();
	void StopFromSoundThread();
	bool InternalIsOpen() const { return (m_hWaveOut != NULL); }
	bool InternalHasGetStreamPosition() const { return true; }
	int64 InternalGetStreamPositionFrames() const;
	SoundDevice::BufferAttributes InternalGetEffectiveBufferAttributes() const;

	SoundDevice::Statistics GetStatistics() const;

	SoundDevice::Caps InternalGetDeviceCaps();
	SoundDevice::DynamicCaps GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates);

private:

	bool CheckResult(MMRESULT result);

	void HandleWaveoutDone();

	int GetDeviceIndex() const;
	
public:
	static void CALLBACK WaveOutCallBack(HWAVEOUT, UINT uMsg, DWORD_PTR, DWORD_PTR dw1, DWORD_PTR dw2);
	static std::vector<SoundDevice::Info> EnumerateDevices(SoundDevice::SysInfo sysInfo);
};

#endif // MPT_OS_WINDOWS


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
