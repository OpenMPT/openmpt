
#if defined(MPT_BUILD_WINESUPPORT_WRAPPER)

#include "NativeConfig.h"

#include <windows.h>

#include <stdint.h>

#include "Native.h"
#include "NativeUtils.h"
#include "NativeSoundDevice.h"

#ifdef _MSC_VER
#pragma warning(disable:4098) /* 'void' function returning a value */
#endif

#define WINE_THREAD

#ifdef __cplusplus
extern "C" {
#endif

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_Init(void) {
	return OpenMPT_Init();
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_Fini(void) {
	return OpenMPT_Fini();
}

OPENMPT_WINESUPPORT_WRAPPER_API void * OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_Alloc( size_t size ) {
	return OpenMPT_Alloc( size );
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_Free( void * mem ) {
	return OpenMPT_Free( mem );
}

OPENMPT_WINESUPPORT_WRAPPER_API size_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_String_Length( const char * str ) {
	return OpenMPT_String_Length( str );
}

OPENMPT_WINESUPPORT_WRAPPER_API char * OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_String_Duplicate( const char * src ) {
	return OpenMPT_String_Duplicate( src );
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_String_Free( char * str ) {
	return OpenMPT_String_Free( str );
}

OPENMPT_WINESUPPORT_WRAPPER_API char * OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_EnumerateDevices() {
	return OpenMPT_SoundDevice_EnumerateDevices();
}

typedef struct OpenMPT_Wine_Wrapper_SoundDevice_IMessageReceiver {
	void * inst;
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundDeviceMessageFunc)( void * inst, uintptr_t level, const char * message );
} OpenMPT_Wine_Wrapper_SoundDevice_IMessageReceiver;

typedef struct OpenMPT_Wine_Wrapper_SoundDevice_ISource {
	void * inst;
	// main thread
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundSourceGetReferenceClockNowNanosecondsFunc)( void * inst, uint64_t * result );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundSourcePreStartCallbackFunc)( void * inst );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundSourcePostStopCallbackFunc)( void * inst );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundSourceIsLockedByCurrentThreadFunc)( void * inst, uintptr_t * result );
	// audio thread
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundSourceLockFunc)( void * inst );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundSourceLockedGetReferenceClockNowNanosecondsFunc)( void * inst, uint64_t * result );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundSourceLockedReadFunc)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, const OpenMPT_SoundDevice_BufferAttributes * bufferAttributes, const OpenMPT_SoundDevice_TimeInfo * timeInfo, uintptr_t numFrames, void * buffer, const void * inputBuffer );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundSourceLockedDoneFunc)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, const OpenMPT_SoundDevice_BufferAttributes * bufferAttributes, const OpenMPT_SoundDevice_TimeInfo * timeInfo );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundSourceUnlockFunc)( void * inst );
} OpenMPT_Wine_Wrapper_SoundDevice_ISource;

#ifdef WINE_THREAD
typedef enum OpenMPT_Wine_Wrapper_AudioThreadCommand {
	AudioThreadCommandInvalid  = -1
	, AudioThreadCommandExit   = 0
	, AudioThreadCommandLock   = 1
	, AudioThreadCommandClock  = 2
	, AudioThreadCommandRead   = 3
	, AudioThreadCommandDone   = 4
	, AudioThreadCommandUnlock = 5
} OpenMPT_Wine_Wrapper_AudioThreadCommand;
#endif

typedef struct OpenMPT_Wine_Wrapper_SoundDevice {
	OpenMPT_SoundDevice * impl;
	OpenMPT_SoundDevice_IMessageReceiver native_receiver;
	OpenMPT_SoundDevice_ISource native_source;
	OpenMPT_Wine_Wrapper_SoundDevice_IMessageReceiver wine_receiver;
	OpenMPT_Wine_Wrapper_SoundDevice_ISource wine_source;
#ifdef WINE_THREAD
	HANDLE audiothread_startup_done;
	OpenMPT_Semaphore * audiothread_sem_request;
	OpenMPT_Semaphore * audiothread_sem_done;
	OpenMPT_Wine_Wrapper_AudioThreadCommand audiothread_command;
	const OpenMPT_SoundDevice_BufferFormat * audiothread_command_bufferFormat;
	const OpenMPT_SoundDevice_BufferAttributes * audiothread_command_bufferAttributes;
	const OpenMPT_SoundDevice_TimeInfo * audiothread_command_timeInfo;
	uintptr_t audiothread_command_numFrames;
	void * audiothread_command_buffer;
	const void * audiothread_command_inputBuffer;
	uint64_t * audiothread_command_result;
	HANDLE audiothread;
	OpenMPT_PriorityBooster * priority_booster;
#endif
} OpenMPT_Wine_Wrapper_SoundDevice;

static void OPENMPT_WINESUPPORT_CALL SoundDeviceMessageFunc( void * inst, uintptr_t level, const char * message ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
	return sd->wine_receiver.SoundDeviceMessageFunc( sd->wine_receiver.inst, level, message );
}

static void OPENMPT_WINESUPPORT_CALL SoundSourceGetReferenceClockNowNanosecondsFunc( void * inst, uint64_t * result ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
	return sd->wine_source.SoundSourceGetReferenceClockNowNanosecondsFunc( sd->wine_source.inst, result );
}
static void OPENMPT_WINESUPPORT_CALL SoundSourcePreStartCallbackFunc( void * inst ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
	return sd->wine_source.SoundSourcePreStartCallbackFunc( sd->wine_source.inst );
}
static void OPENMPT_WINESUPPORT_CALL SoundSourcePostStopCallbackFunc( void * inst ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
	return sd->wine_source.SoundSourcePostStopCallbackFunc( sd->wine_source.inst );
}
static void OPENMPT_WINESUPPORT_CALL SoundSourceIsLockedByCurrentThreadFunc( void * inst, uintptr_t * result ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
	return sd->wine_source.SoundSourceIsLockedByCurrentThreadFunc( sd->wine_source.inst, result );
}
static void OPENMPT_WINESUPPORT_CALL SoundSourceLockFunc( void * inst ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandLock;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_source.SoundSourceLockFunc( sd->wine_source.inst );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundSourceLockedGetReferenceClockNowNanosecondsFunc( void * inst, uint64_t * result ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandClock;
	sd->audiothread_command_result = result;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_source.SoundSourceLockedGetReferenceClockNowNanosecondsFunc( sd->wine_source.inst, result );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundSourceLockedReadFunc( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, const OpenMPT_SoundDevice_BufferAttributes * bufferAttributes, const OpenMPT_SoundDevice_TimeInfo * timeInfo, uintptr_t numFrames, void * buffer, const void * inputBuffer ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandRead;
	sd->audiothread_command_bufferFormat = bufferFormat;
	sd->audiothread_command_bufferAttributes = bufferAttributes;
	sd->audiothread_command_timeInfo = timeInfo;
	sd->audiothread_command_numFrames = numFrames;
	sd->audiothread_command_buffer = buffer;
	sd->audiothread_command_inputBuffer = inputBuffer;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_source.SoundSourceLockedReadFunc( sd->wine_source.inst, bufferFormat, bufferAttributes, timeInfo, numFrames, buffer, inputBuffer );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundSourceLockedDoneFunc( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, const OpenMPT_SoundDevice_BufferAttributes * bufferAttributes, const OpenMPT_SoundDevice_TimeInfo * timeInfo ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandDone;
	sd->audiothread_command_bufferFormat = bufferFormat;
	sd->audiothread_command_bufferAttributes = bufferAttributes;
	sd->audiothread_command_timeInfo = timeInfo;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_source.SoundSourceLockedDoneFunc( sd->wine_source.inst, bufferFormat, bufferAttributes, timeInfo );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundSourceUnlockFunc( void * inst ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandUnlock;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_source.SoundSourceUnlockFunc( sd->wine_source.inst );
#endif
}

#ifdef WINE_THREAD
static DWORD WINAPI AudioThread( LPVOID userdata ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)userdata;
	sd->priority_booster = OpenMPT_PriorityBooster_Construct_From_SoundDevice( sd->impl );
	SetEvent( sd->audiothread_startup_done );
	{
		int exit = 0;
		while(!exit)
		{
			OpenMPT_Semaphore_Wait( sd->audiothread_sem_request );
			switch ( sd->audiothread_command ) {
			case AudioThreadCommandExit:
				exit = 1;
				break;
			case AudioThreadCommandLock:
				if(sd->wine_source.SoundSourceLockFunc)
					sd->wine_source.SoundSourceLockFunc( sd->wine_source.inst );
				break;
			case AudioThreadCommandClock:
				if(sd->wine_source.SoundSourceLockedGetReferenceClockNowNanosecondsFunc)
					sd->wine_source.SoundSourceLockedGetReferenceClockNowNanosecondsFunc( sd->wine_source.inst, sd->audiothread_command_result );
				break;
			case AudioThreadCommandRead:
				if(sd->wine_source.SoundSourceLockedReadFunc)
					sd->wine_source.SoundSourceLockedReadFunc
						( sd->wine_source.inst
						, sd->audiothread_command_bufferFormat
						, sd->audiothread_command_bufferAttributes
						, sd->audiothread_command_timeInfo
						, sd->audiothread_command_numFrames
						, sd->audiothread_command_buffer
						, sd->audiothread_command_inputBuffer
						);
				break;
			case AudioThreadCommandDone:
				if(sd->wine_source.SoundSourceLockedDoneFunc)
					sd->wine_source.SoundSourceLockedDoneFunc
						( sd->wine_source.inst
						, sd->audiothread_command_bufferFormat
						, sd->audiothread_command_bufferAttributes
						, sd->audiothread_command_timeInfo
						);
				break;
			case AudioThreadCommandUnlock:
				if(sd->wine_source.SoundSourceUnlockFunc)
					sd->wine_source.SoundSourceUnlockFunc( sd->wine_source.inst );
				break;
			default:
				break;
			}
			OpenMPT_Semaphore_Post( sd->audiothread_sem_done );
		}
	}
	OpenMPT_PriorityBooster_Destruct(sd->priority_booster);
	sd->priority_booster = NULL;
	return 0;
}
#endif

OPENMPT_WINESUPPORT_WRAPPER_API OpenMPT_Wine_Wrapper_SoundDevice * OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_Construct( const char * info ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)OpenMPT_Wine_Wrapper_Alloc( sizeof( OpenMPT_Wine_Wrapper_SoundDevice ) );
	sd->impl = OpenMPT_SoundDevice_Construct( info );
	return sd;
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_Destruct( OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	OpenMPT_SoundDevice_Destruct( sd->impl );
	sd->impl = NULL;
	OpenMPT_Wine_Wrapper_Free( sd );
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_SetMessageReceiver( OpenMPT_Wine_Wrapper_SoundDevice * sd, const OpenMPT_Wine_Wrapper_SoundDevice_IMessageReceiver * receiver ) {
	OpenMPT_SoundDevice_SetMessageReceiver( sd->impl, NULL );
	ZeroMemory( &( sd->wine_receiver ), sizeof( sd->wine_receiver ) );
	if(receiver)
	{
		sd->wine_receiver = *receiver;
	}
	sd->native_receiver.inst = sd;
	sd->native_receiver.SoundDeviceMessageFunc = &SoundDeviceMessageFunc;
	OpenMPT_SoundDevice_SetMessageReceiver( sd->impl, &sd->native_receiver );
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_SetSource( OpenMPT_Wine_Wrapper_SoundDevice * sd, const OpenMPT_Wine_Wrapper_SoundDevice_ISource * source ) {
	OpenMPT_SoundDevice_SetSource( sd->impl, NULL );
	ZeroMemory( &( sd->wine_source ), sizeof( sd->wine_source ) );
	if(source)
	{
		sd->wine_source = *source;
	}
	sd->native_source.inst = sd;
	sd->native_source.SoundSourceGetReferenceClockNowNanosecondsFunc = &SoundSourceGetReferenceClockNowNanosecondsFunc;
	sd->native_source.SoundSourcePreStartCallbackFunc = &SoundSourcePreStartCallbackFunc;
	sd->native_source.SoundSourcePostStopCallbackFunc = &SoundSourcePostStopCallbackFunc;
	sd->native_source.SoundSourceIsLockedByCurrentThreadFunc = &SoundSourceIsLockedByCurrentThreadFunc;
	sd->native_source.SoundSourceLockFunc = &SoundSourceLockFunc;
	sd->native_source.SoundSourceLockedGetReferenceClockNowNanosecondsFunc = &SoundSourceLockedGetReferenceClockNowNanosecondsFunc;
	sd->native_source.SoundSourceLockedReadFunc = &SoundSourceLockedReadFunc;
	sd->native_source.SoundSourceLockedDoneFunc = &SoundSourceLockedDoneFunc;
	sd->native_source.SoundSourceUnlockFunc = &SoundSourceUnlockFunc;
	OpenMPT_SoundDevice_SetSource( sd->impl, &sd->native_source );
}

OPENMPT_WINESUPPORT_WRAPPER_API char * OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_GetDeviceInfo( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_GetDeviceInfo( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API char * OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_GetDeviceCaps( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_GetDeviceCaps( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API char * OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_GetDeviceDynamicCaps( OpenMPT_Wine_Wrapper_SoundDevice * sd, const char * baseSampleRates ) {
	return OpenMPT_SoundDevice_GetDeviceDynamicCaps( sd->impl, baseSampleRates );
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_Init( OpenMPT_Wine_Wrapper_SoundDevice * sd, const char * appInfo ) {
	return OpenMPT_SoundDevice_Init( sd->impl, appInfo );
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_Open( OpenMPT_Wine_Wrapper_SoundDevice * sd, const char * settings ) {
	uintptr_t result = 0;
	result = OpenMPT_SoundDevice_Open( sd->impl, settings );
#ifdef WINE_THREAD
	if ( result ) {
		DWORD threadId = 0;
		sd->audiothread_startup_done = CreateEvent( NULL, FALSE, FALSE, NULL );
		sd->audiothread_sem_request = OpenMPT_Semaphore_Construct();
		sd->audiothread_sem_done = OpenMPT_Semaphore_Construct();
		sd->audiothread_command = AudioThreadCommandInvalid;
		sd->audiothread = CreateThread( NULL, 0, &AudioThread, sd, 0, &threadId );
		WaitForSingleObject( sd->audiothread_startup_done, INFINITE );
	}
#endif
	return result;
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_Close( OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	uintptr_t result = 0;
#ifdef WINE_THREAD
	if ( OpenMPT_SoundDevice_IsOpen( sd->impl ) ) {
		sd->audiothread_command = AudioThreadCommandExit;
		OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
		OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
		sd->audiothread_command = AudioThreadCommandInvalid;
		WaitForSingleObject( sd->audiothread, INFINITE );
		CloseHandle( sd->audiothread );
		sd->audiothread = NULL;
		OpenMPT_Semaphore_Destruct( sd->audiothread_sem_done );
		sd->audiothread_sem_done = NULL;
		OpenMPT_Semaphore_Destruct( sd->audiothread_sem_request );
		sd->audiothread_sem_request = NULL;
		CloseHandle( sd->audiothread_startup_done );
		sd->audiothread_startup_done = NULL;
	}
#endif
	result = OpenMPT_SoundDevice_Close( sd->impl );
	return result;
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_Start( OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_Start( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_Stop( OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_Stop( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_GetRequestFlags( const OpenMPT_Wine_Wrapper_SoundDevice * sd, uint32_t * result) {
	return OpenMPT_SoundDevice_GetRequestFlags( sd->impl, result );
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_IsInited( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_IsInited( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_IsOpen( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_IsOpen( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_IsAvailable( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_IsAvailable( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_IsPlaying( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_IsPlaying( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_IsPlayingSilence( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_IsPlayingSilence( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_StopAndAvoidPlayingSilence( OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_StopAndAvoidPlayingSilence( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_EndPlayingSilence( OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_EndPlayingSilence( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_OnIdle( OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_OnIdle( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API char * OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_GetSettings( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_GetSettings( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_GetActualSampleFormat( const OpenMPT_Wine_Wrapper_SoundDevice * sd, int32_t * result ) {
	return OpenMPT_SoundDevice_GetActualSampleFormat( sd->impl, result );
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_GetEffectiveBufferAttributes( const OpenMPT_Wine_Wrapper_SoundDevice * sd, OpenMPT_SoundDevice_BufferAttributes * result ) {
	return OpenMPT_SoundDevice_GetEffectiveBufferAttributes( sd->impl, result );
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_GetTimeInfo( const OpenMPT_Wine_Wrapper_SoundDevice * sd, OpenMPT_SoundDevice_TimeInfo * result ) {
	return OpenMPT_SoundDevice_GetTimeInfo( sd->impl, result );
}

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_GetStreamPosition( const OpenMPT_Wine_Wrapper_SoundDevice * sd, OpenMPT_SoundDevice_StreamPosition * result ) {
	return OpenMPT_SoundDevice_GetStreamPosition( sd->impl, result );
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_DebugIsFragileDevice( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_DebugIsFragileDevice( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_DebugInRealtimeCallback( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_DebugInRealtimeCallback( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API char * OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_GetStatistics( const OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_GetStatistics( sd->impl );
}

OPENMPT_WINESUPPORT_WRAPPER_API uintptr_t OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_OpenDriverSettings( OpenMPT_Wine_Wrapper_SoundDevice * sd ) {
	return OpenMPT_SoundDevice_OpenDriverSettings( sd->impl );
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // MPT_BUILD_WINESUPPORT_WRAPPER
