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

#include "SoundDevices.h"

#ifndef NO_ASIO
#include <iasiodrv.h>
#endif

////////////////////////////////////////////////////////////////////////////////////
//
// ASIO device
//

#ifndef NO_ASIO

//====================================
class CASIODevice: public ISoundDevice
//====================================
{
	friend class TemporaryASIODriverOpener;

protected:

	IASIO *m_pAsioDrv;

	long m_nAsioBufferLen;
	std::vector<ASIOBufferInfo> m_BufferInfo;
	ASIOCallbacks m_Callbacks;
	static CASIODevice *g_CallbacksInstance; // only 1 opened instance allowed for ASIO
	bool m_BuffersCreated;
	std::vector<ASIOChannelInfo> m_ChannelInfo;
	std::vector<int32> m_SampleBuffer;
	bool m_CanOutputReady;

	bool m_DeviceRunning;
	uint64 m_TotalFramesWritten;
	long m_BufferIndex;
	LONG m_RenderSilence;
	LONG m_RenderingSilence;

	int64 m_StreamPositionOffset;

private:
	void UpdateTimeInfo(AsioTimeInfo asioTimeInfo);

	static bool IsSampleTypeFloat(ASIOSampleType sampleType);
	static std::size_t GetSampleSize(ASIOSampleType sampleType);
	static bool IsSampleTypeBigEndian(ASIOSampleType sampleType);
	
	void SetRenderSilence(bool silence, bool wait=false);

public:
	CASIODevice(SoundDeviceID id, const std::wstring &internalID);
	~CASIODevice();

private:
	void Init();

public:
	bool InternalOpen();
	bool InternalClose();
	void InternalFillAudioBuffer();
	bool InternalStart();
	void InternalStop();
	bool InternalIsOpen() const { return m_BuffersCreated; }

	SoundDeviceCaps GetDeviceCaps(const std::vector<uint32> &baseSampleRates);
	bool OpenDriverSettings();

public:
	static std::vector<SoundDeviceInfo> EnumerateDevices();

protected:
	void OpenDriver();
	void CloseDriver();
	bool IsDriverOpen() const { return (m_pAsioDrv != nullptr); }

	bool InternalHasTimeInfo() const;
	bool InternalHasGetStreamPosition() const;
	int64 InternalGetStreamPositionFrames() const;

protected:
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

#endif // NO_ASIO

