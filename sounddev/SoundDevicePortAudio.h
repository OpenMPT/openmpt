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

	float m_CurrentRealLatencyMS;

public:
	CPortaudioDevice(PaHostApiIndex hostapi);
	~CPortaudioDevice();

public:
	UINT GetDeviceType() const { return HostApiToSndDevType(m_HostApi); }
	bool InternalOpen();
	bool InternalClose();
	void FillAudioBuffer();
	void InternalReset();
	void InternalStart();
	void InternalStop();
	bool IsOpen() const { return m_Stream ? true : false; }
	UINT GetNumBuffers() { return 1; }
	float GetCurrentRealLatencyMS();
	bool InternalHasGetStreamPosition() const { return false; }
	int64 InternalGetStreamPositionSamples() const;
	std::vector<uint32> GetSampleRates(const std::vector<uint32> &samplerates);

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

	static PaDeviceIndex HostApiOutputIndexToGlobalDeviceIndex(int hostapioutputdeviceindex, PaHostApiIndex hostapi);
	static int HostApiToSndDevType(PaHostApiIndex hostapi);
	static PaHostApiIndex SndDevTypeToHostApi(int snddevtype);

	static std::vector<SoundDeviceInfo> EnumerateDevices(UINT type);

private:
	static bool EnumerateDevices(SoundDeviceInfo &result, UINT nIndex, PaHostApiIndex hostapi);

};

void SndDevPortaudioInitialize();
void SndDevPortaudioUnnitialize();

bool SndDevPortaudioIsInitialized();

#endif // NO_PORTAUDIO
