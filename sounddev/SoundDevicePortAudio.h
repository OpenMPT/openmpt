/*
 * SoundDevicePortAudio.h
 * ----------------------
 * Purpose: PortAudio sound device driver class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "SoundDevices.h"

#ifndef NO_PORTAUDIO
#include "portaudio/include/portaudio.h"
#include "portaudio/include/pa_win_wasapi.h"
#endif

////////////////////////////////////////////////////////////////////////////////////
//
// Protaudio device
//

#ifndef NO_PORTAUDIO

//=========================================
class CPortaudioDevice: public ISoundDevice
//=========================================
{
protected:
	PaHostApiIndex m_HostApi;
	PaStreamParameters m_StreamParameters;
	PaWasapiStreamInfo m_WasapiStreamInfo;
	PaStream * m_Stream;
	void * m_CurrentFrameBuffer;
	unsigned long m_CurrentFrameCount;

	double m_CurrentRealLatency; // seconds

public:
	CPortaudioDevice(SoundDeviceID id, const std::wstring &internalID);
	~CPortaudioDevice();

public:
	bool InternalOpen();
	bool InternalClose();
	void FillAudioBuffer();
	void InternalStart();
	void InternalStop();
	bool InternalIsOpen() const { return m_Stream ? true : false; }
	double GetCurrentRealLatency() const;
	bool InternalHasGetStreamPosition() const { return false; }
	int64 InternalGetStreamPositionFrames() const;
	SoundDeviceCaps GetDeviceCaps(const std::vector<uint32> &baseSampleRates);

	int StreamCallback(
		const void *input, void *output,
		unsigned long frameCount,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags
		);

public:
	static int StreamCallbackWrapper(
		const void *input, void *output,
		unsigned long frameCount,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData
		);

	static std::string HostApiToString(PaHostApiIndex hostapi);

	static PaDeviceIndex HostApiOutputIndexToGlobalDeviceIndex(int hostapioutputdeviceindex, PaHostApiIndex hostapi);
	static SoundDeviceType HostApiToSndDevType(PaHostApiIndex hostapi);
	static PaHostApiIndex SndDevTypeToHostApi(SoundDeviceType snddevtype);

	static std::vector<SoundDeviceInfo> EnumerateDevices(SoundDeviceType type);

private:
	static bool EnumerateDevices(SoundDeviceInfo &result, SoundDeviceIndex index, PaHostApiIndex hostapi);

};

void SndDevPortaudioInitialize();
void SndDevPortaudioUnnitialize();

bool SndDevPortaudioIsInitialized();

#endif // NO_PORTAUDIO
