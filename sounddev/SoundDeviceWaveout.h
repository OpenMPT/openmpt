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

#include "SoundDevice.h"
#include "SoundDeviceUtilities.h"

#include "../common/ComponentManager.h"

#include <MMSystem.h>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


////////////////////////////////////////////////////////////////////////////////////
//
// MMSYSTEM WaveOut device
//


class ComponentWaveOut : public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentWaveOut() { }
	virtual ~ComponentWaveOut() { }
	std::string GetSettingsKey() const { return "WaveOut"; }
	virtual bool DoInitialize() { return true; }
};


//==============================================
class CWaveDevice: public CSoundDeviceWithThread
//==============================================
{
protected:
	HANDLE m_ThreadWakeupEvent;
	HWAVEOUT m_hWaveOut;
	ULONG m_nWaveBufferSize;
	bool m_JustStarted;
	ULONG m_nPreparedHeaders;
	ULONG m_nWriteBuffer;
	mutable LONG m_nBuffersPending;
	std::vector<WAVEHDR> m_WaveBuffers;
	std::vector<std::vector<char> > m_WaveBuffersData;

public:
	CWaveDevice(SoundDevice::Info info);
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
	void HandleWaveoutDone();

public:
	static void CALLBACK WaveOutCallBack(HWAVEOUT, UINT uMsg, DWORD_PTR, DWORD_PTR dw1, DWORD_PTR dw2);
	static std::vector<SoundDevice::Info> EnumerateDevices();
};


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
