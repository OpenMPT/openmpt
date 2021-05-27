
#ifndef OPENMPT_WINE_SOUNDDEVICE_H
#define OPENMPT_WINE_SOUNDDEVICE_H

#include "NativeConfig.h"
#include "NativeUtils.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_EnumerateDevices();

typedef void OpenMPT_int24;

typedef struct OpenMPT_SoundDevice_StreamPosition {
	int64_t Frames;
	double Seconds;
} OpenMPT_SoundDevice_StreamPosition;

typedef struct OpenMPT_SoundDevice_TimeInfo {
	int64_t SyncPointStreamFrames;
	uint64_t SyncPointSystemTimestamp;
	double Speed;
	OpenMPT_SoundDevice_StreamPosition RenderStreamPositionBefore;
	OpenMPT_SoundDevice_StreamPosition RenderStreamPositionAfter;
	double Latency;
} OpenMPT_SoundDevice_TimeInfo;

typedef struct OpenMPT_SoundDevice_Flags {
	uint8_t WantsClippedOutput;
	uint8_t pad1;
	uint16_t pad2;
	uint32_t pad3;
} OpenMPT_SoundDevice_Flags;

typedef struct OpenMPT_SoundDevice_BufferFormat {
	uint32_t Samplerate;
	uint32_t Channels;
	uint8_t InputChannels;
	uint8_t pad1;
	uint16_t pad2; 
	int32_t sampleFormat;
	uint8_t WantsClippedOutput;
	uint8_t pad3;
	uint16_t pad4; 
	int32_t DitherType;
} OpenMPT_SoundDevice_BufferFormat;

typedef struct OpenMPT_SoundDevice_BufferAttributes {
	double Latency;
	double UpdateInterval;
	int32_t NumBuffers;
	uint32_t pad1;
} OpenMPT_SoundDevice_BufferAttributes;

typedef struct OpenMPT_SoundDevice_RequestFlags {
	uint32_t RequestFlags;
	uint32_t pad1;
} OpenMPT_SoundDevice_RequestFlags;

typedef struct OpenMPT_SoundDevice OpenMPT_SoundDevice;

typedef struct OpenMPT_SoundDevice_IMessageReceiver {
	void * inst;
	void (OPENMPT_WINESUPPORT_CALL * SoundDeviceMessageFunc)( void * inst, uintptr_t level, const char * message );
} OpenMPT_SoundDevice_IMessageReceiver;

typedef struct OpenMPT_SoundDevice_ICallback {
	void * inst;
	// main thread
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackGetReferenceClockNowNanosecondsFunc)( void * inst, uint64_t * result );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackPreStartFunc)( void * inst );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackPostStopFunc)( void * inst );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackIsLockedByCurrentThreadFunc)( void * inst, uintptr_t * result );
	// audio thread
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackLockFunc)( void * inst );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackLockedGetReferenceClockNowNanosecondsFunc)( void * inst, uint64_t * result );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackLockedProcessPrepareFunc)( void * inst, const OpenMPT_SoundDevice_TimeInfo * timeInfo );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackLockedProcessUint8Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, uint8_t * buffer, const uint8_t * inputBuffer );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackLockedProcessInt8Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int8_t * buffer, const int8_t * inputBuffer );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackLockedProcessInt16Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int16_t * buffer, const int16_t * inputBuffer );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackLockedProcessInt24Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, OpenMPT_int24 * buffer, const OpenMPT_int24 * inputBuffer );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackLockedProcessInt32Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int32_t * buffer, const int32_t * inputBuffer );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackLockedProcessFloatFunc)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, float * buffer, const float * inputBuffer );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackLockedProcessDoubleFunc)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, double * buffer, const double * inputBuffer );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackLockedProcessDoneFunc)( void * inst, const OpenMPT_SoundDevice_TimeInfo * timeInfo );
	void (OPENMPT_WINESUPPORT_CALL * SoundCallbackUnlockFunc)( void * inst );
} OpenMPT_SoundDevice_ICallback;

OPENMPT_WINESUPPORT_API OpenMPT_SoundDevice * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Construct( const char * info );

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Destruct( OpenMPT_SoundDevice * sd );

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_SetMessageReceiver( OpenMPT_SoundDevice * sd, const OpenMPT_SoundDevice_IMessageReceiver * receiver );
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_SetCallback( OpenMPT_SoundDevice * sd, const OpenMPT_SoundDevice_ICallback * callback );

OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetDeviceInfo( const OpenMPT_SoundDevice * sd );

OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetDeviceCaps( const OpenMPT_SoundDevice * sd );
OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetDeviceDynamicCaps( OpenMPT_SoundDevice * sd, const char * baseSampleRates );

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Init( OpenMPT_SoundDevice * sd, const char * appInfo );
OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Open( OpenMPT_SoundDevice * sd, const char * settings );
OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Close( OpenMPT_SoundDevice * sd );
OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Start( OpenMPT_SoundDevice * sd );
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_Stop( OpenMPT_SoundDevice * sd );

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetRequestFlags( const OpenMPT_SoundDevice * sd, uint32_t * result );

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_IsInited( const OpenMPT_SoundDevice * sd );
OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_IsOpen( const OpenMPT_SoundDevice * sd );
OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_IsAvailable( const OpenMPT_SoundDevice * sd );
OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_IsPlaying( const OpenMPT_SoundDevice * sd );

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_IsPlayingSilence( const OpenMPT_SoundDevice * sd );
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_StopAndAvoidPlayingSilence( OpenMPT_SoundDevice * sd );
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_EndPlayingSilence( OpenMPT_SoundDevice * sd );

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_OnIdle( OpenMPT_SoundDevice * sd );

OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetSettings( const OpenMPT_SoundDevice * sd );
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetActualSampleFormat( const OpenMPT_SoundDevice * sd, int32_t * result );
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetEffectiveBufferAttributes( const OpenMPT_SoundDevice * sd, OpenMPT_SoundDevice_BufferAttributes * result );

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetTimeInfo( const OpenMPT_SoundDevice * sd, OpenMPT_SoundDevice_TimeInfo * result );
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetStreamPosition( const OpenMPT_SoundDevice * sd, OpenMPT_SoundDevice_StreamPosition * result );

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_DebugIsFragileDevice( const OpenMPT_SoundDevice * sd );
OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_DebugInRealtimeCallback( const OpenMPT_SoundDevice * sd );

OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_GetStatistics( const OpenMPT_SoundDevice * sd );

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_SoundDevice_OpenDriverSettings( OpenMPT_SoundDevice * sd );

typedef struct OpenMPT_PriorityBooster OpenMPT_PriorityBooster;
OPENMPT_WINESUPPORT_API OpenMPT_PriorityBooster * OPENMPT_WINESUPPORT_CALL OpenMPT_PriorityBooster_Construct_From_SoundDevice( const OpenMPT_SoundDevice * sd );
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_PriorityBooster_Destruct( OpenMPT_PriorityBooster * pb );

#ifdef __cplusplus
}
#endif

#endif // OPENMPT_WINE_SOUNDDEVICE_H
