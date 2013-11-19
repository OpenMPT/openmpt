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
#define ASIO_LOG
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
	friend class TemporaryASIODeviceOpener;

	enum { ASIO_MAX_CHANNELS=4 };
protected:
	IASIO *m_pAsioDrv;
	long m_nAsioBufferLen;
	bool m_CanOutputReady;
	BOOL m_bMixRunning;
	LONG m_RenderSilence;
	LONG m_RenderingSilence;
	ASIOCallbacks m_Callbacks;
	ASIOChannelInfo m_ChannelInfo[ASIO_MAX_CHANNELS];
	ASIOBufferInfo m_BufferInfo[ASIO_MAX_CHANNELS];
	std::vector<int32> m_SampleBuffer;

private:
	static bool IsSampleTypeFloat(ASIOSampleType sampleType);
	static std::size_t GetSampleSize(ASIOSampleType sampleType);
	void SetRenderSilence(bool silence, bool wait=false);

public:
	CASIODevice(SoundDeviceID id, const std::wstring &internalID);
	~CASIODevice();

public:
	bool InternalOpen();
	bool InternalClose();
	void FillAudioBuffer();
	bool InternalStart();
	void InternalStop();
	bool InternalIsOpen() const { return (m_pAsioDrv != nullptr); }

	SoundDeviceCaps GetDeviceCaps(const std::vector<uint32> &baseSampleRates);
	bool OpenDriverSettings();

public:
	static std::vector<SoundDeviceInfo> EnumerateDevices();

protected:
	void OpenDevice();
	void CloseDevice();
	bool IsDeviceOpen() const { return (m_pAsioDrv != nullptr); }

protected:
	void BufferSwitch(long doubleBufferIndex);

	static CASIODevice *gpCurrentAsio;
	static void BufferSwitch(long doubleBufferIndex, ASIOBool directProcess);
	static void SampleRateDidChange(ASIOSampleRate sRate);
	static long AsioMessage(long selector, long value, void* message, double* opt);
	static ASIOTime* BufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);
	
	void ReportASIOException(const std::string &str);
};

#endif // NO_ASIO

