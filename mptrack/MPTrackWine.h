/*
 * MPTrackWine.h
 * -------------
 * Purpose: OpenMPT Wine support functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"


#include "../common/ComponentManager.h"


extern "C" {

typedef void OpenMPT_int24;

typedef struct OpenMPT_SoundDevice_StreamPosition OpenMPT_SoundDevice_StreamPosition;
typedef struct OpenMPT_SoundDevice_TimeInfo OpenMPT_SoundDevice_TimeInfo;
typedef struct OpenMPT_SoundDevice_Flags OpenMPT_SoundDevice_Flags;
typedef struct OpenMPT_SoundDevice_BufferFormat OpenMPT_SoundDevice_BufferFormat;
typedef struct OpenMPT_SoundDevice_BufferAttributes OpenMPT_SoundDevice_BufferAttributes;
typedef struct OpenMPT_SoundDevice_RequestFlags OpenMPT_SoundDevice_RequestFlags;
typedef struct OpenMPT_SoundDevice OpenMPT_SoundDevice;

typedef struct OpenMPT_Wine_Wrapper_SoundDevice_IMessageReceiver {
	void * inst;
	void (__cdecl * SoundDeviceMessageFunc)( void * inst, uintptr_t level, const char * message );
} OpenMPT_Wine_Wrapper_SoundDevice_IMessageReceiver;

typedef struct OpenMPT_Wine_Wrapper_SoundDevice_ICallback {
	void * inst;
	// main thread
	void (__cdecl * SoundCallbackGetReferenceClockNowNanosecondsFunc)( void * inst, uint64_t * result );
	void (__cdecl * SoundCallbackPreStartFunc)( void * inst );
	void (__cdecl * SoundCallbackPostStopFunc)( void * inst );
	void (__cdecl * SoundCallbackIsLockedByCurrentThreadFunc)( void * inst, uintptr_t * result );
	// audio thread
	void (__cdecl * SoundCallbackLockFunc)( void * inst );
	void (__cdecl * SoundCallbackLockedGetReferenceClockNowNanosecondsFunc)( void * inst, uint64_t * result );
	void (__cdecl * SoundCallbackLockedProcessPrepareFunc)( void * inst, const OpenMPT_SoundDevice_TimeInfo * timeInfo );
	void (__cdecl * SoundCallbackLockedProcessUint8Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, uint8_t * buffer, const uint8_t * inputBuffer );
	void (__cdecl * SoundCallbackLockedProcessInt8Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int8_t * buffer, const int8_t * inputBuffer );
	void (__cdecl * SoundCallbackLockedProcessInt16Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int16_t * buffer, const int16_t * inputBuffer );
	void (__cdecl * SoundCallbackLockedProcessInt24Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, OpenMPT_int24 * buffer, const OpenMPT_int24 * inputBuffer );
	void (__cdecl * SoundCallbackLockedProcessInt32Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int32_t * buffer, const int32_t * inputBuffer );
	void (__cdecl * SoundCallbackLockedProcessFloatFunc)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, float * buffer, const float * inputBuffer );
	void (__cdecl * SoundCallbackLockedProcessDoubleFunc)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, double * buffer, const double * inputBuffer );
	void (__cdecl * SoundCallbackLockedProcessDoneFunc)( void * inst, const OpenMPT_SoundDevice_TimeInfo * timeInfo );
	void (__cdecl * SoundCallbackUnlockFunc)( void * inst );
} OpenMPT_Wine_Wrapper_SoundDevice_ICallback;

typedef struct OpenMPT_Wine_Wrapper_SoundDevice OpenMPT_Wine_Wrapper_SoundDevice;

};

OPENMPT_NAMESPACE_BEGIN


namespace WineIntegration {

void Initialize();

bool IsSupported();

bool IsCompiled();

void Load();

} // namespace WineIntegration


class ComponentWineWrapper
	: public ComponentLibrary
{
	MPT_DECLARE_COMPONENT_MEMBERS(ComponentWineWrapper, "WineWrapper")

public:

private: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_Init)(void) = nullptr;
private: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_Fini)(void) = nullptr;

private: void (__cdecl * OpenMPT_Wine_Wrapper_String_Free)(char * str) = nullptr;

private: char * (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_EnumerateDevices)(void) = nullptr;
public: std::string SoundDevice_EnumerateDevices() const { return result_as_string(OpenMPT_Wine_Wrapper_SoundDevice_EnumerateDevices()); }

public: OpenMPT_Wine_Wrapper_SoundDevice * (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_Construct)( const char * info ) = nullptr;
public: void (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_Destruct)( OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;

public: void (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_SetMessageReceiver)( OpenMPT_Wine_Wrapper_SoundDevice * sd, const OpenMPT_Wine_Wrapper_SoundDevice_IMessageReceiver * receiver ) = nullptr;
public: void (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_SetCallback)( OpenMPT_Wine_Wrapper_SoundDevice * sd, const OpenMPT_Wine_Wrapper_SoundDevice_ICallback * callback ) = nullptr;

public: char * (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_GetDeviceInfo)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: char * (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_GetDeviceCaps)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: char * (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_GetDeviceDynamicCaps)( OpenMPT_Wine_Wrapper_SoundDevice * sd, const char * baseSampleRates ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_Init)( OpenMPT_Wine_Wrapper_SoundDevice * sd, const char * appInfo ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_Open)( OpenMPT_Wine_Wrapper_SoundDevice * sd, const char * settings ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_Close)( OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_Start)( OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: void (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_Stop)( OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: void (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_GetRequestFlags)( const OpenMPT_Wine_Wrapper_SoundDevice * sd, uint32_t * result ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_IsInited)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_IsOpen)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_IsAvailable)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_IsPlaying)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_IsPlayingSilence)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: void (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_StopAndAvoidPlayingSilence)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: void (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_EndPlayingSilence)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_OnIdle)( OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: char * (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_GetSettings)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: void (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_GetActualSampleFormat)( const OpenMPT_Wine_Wrapper_SoundDevice * sd, int32_t * result ) = nullptr;
public: void (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_GetEffectiveBufferAttributes)( const OpenMPT_Wine_Wrapper_SoundDevice * sd, OpenMPT_SoundDevice_BufferAttributes * result ) = nullptr;
public: void (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_GetTimeInfo)( const OpenMPT_Wine_Wrapper_SoundDevice * sd, OpenMPT_SoundDevice_TimeInfo * result ) = nullptr;
public: void (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_GetStreamPosition)( const OpenMPT_Wine_Wrapper_SoundDevice * sd, OpenMPT_SoundDevice_StreamPosition * result ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_DebugIsFragileDevice)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_DebugInRealtimeCallback)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: char * (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_GetStatistics)( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;
public: uintptr_t (__cdecl * OpenMPT_Wine_Wrapper_SoundDevice_OpenDriverSettings)( OpenMPT_Wine_Wrapper_SoundDevice * sd ) = nullptr;

public: std::string result_as_string(char * str) const;

public:
	ComponentWineWrapper();
	bool DoInitialize();
	virtual ~ComponentWineWrapper();
};


OPENMPT_NAMESPACE_END
