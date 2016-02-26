/*
 * SoundDeviceDirectSound.h
 * ------------------------
 * Purpose: DirectSound sound device driver class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "SoundDeviceBase.h"
#include "SoundDeviceUtilities.h"

#include "../common/ComponentManager.h"

#ifdef MPT_WITH_DSOUND
#include <dsound.h>
#endif

OPENMPT_NAMESPACE_BEGIN

namespace SoundDevice {

#ifdef MPT_WITH_DSOUND

class ComponentDirectSound : public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentDirectSound() { }
	virtual ~ComponentDirectSound() { }
	virtual bool DoInitialize() { return true; }
};

//================================================
class CDSoundDevice: public CSoundDeviceWithThread
//================================================
{
protected:
	IDirectSound *m_piDS;
	IDirectSoundBuffer *m_pPrimary;
	IDirectSoundBuffer *m_pMixBuffer;
	DWORD m_nDSoundBufferSize;
	BOOL m_bMixRunning;
	DWORD m_dwWritePos;

	mpt::atomic_uint32_t m_StatisticLatencyFrames;
	mpt::atomic_uint32_t m_StatisticPeriodFrames;

public:
	CDSoundDevice(SoundDevice::Info info, SoundDevice::SysInfo sysInfo);
	~CDSoundDevice();

public:
	bool InternalOpen();
	bool InternalClose();
	void InternalFillAudioBuffer();
	void StartFromSoundThread();
	void StopFromSoundThread();
	bool InternalIsOpen() const { return (m_pMixBuffer != NULL); }
	SoundDevice::BufferAttributes InternalGetEffectiveBufferAttributes() const;
	SoundDevice::Statistics GetStatistics() const;
	SoundDevice::Caps InternalGetDeviceCaps();
	SoundDevice::DynamicCaps GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates);

public:
	static std::vector<SoundDevice::Info> EnumerateDevices(SoundDevice::SysInfo sysInfo);
};

#endif // NO_DIRECTSOUND


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
