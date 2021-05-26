
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

typedef struct OpenMPT_Wine_Wrapper_SoundDevice_ICallback {
	void * inst;
	// main thread
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackGetReferenceClockNowNanosecondsFunc)( void * inst, uint64_t * result );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackPreStartFunc)( void * inst );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackPostStopFunc)( void * inst );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackIsLockedByCurrentThreadFunc)( void * inst, uintptr_t * result );
	// audio thread
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackLockFunc)( void * inst );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackLockedGetReferenceClockNowNanosecondsFunc)( void * inst, uint64_t * result );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackLockedProcessPrepareFunc)( void * inst, const OpenMPT_SoundDevice_TimeInfo * timeInfo );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackLockedProcessUint8Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, uint8_t * buffer, const uint8_t * inputBuffer );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackLockedProcessInt8Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int8_t * buffer, const int8_t * inputBuffer );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackLockedProcessInt16Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int16_t * buffer, const int16_t * inputBuffer );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackLockedProcessInt24Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, OpenMPT_int24 * buffer, const OpenMPT_int24 * inputBuffer );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackLockedProcessInt32Func)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int32_t * buffer, const int32_t * inputBuffer );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackLockedProcessFloatFunc)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, float * buffer, const float * inputBuffer );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackLockedProcessDoubleFunc)( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, double * buffer, const double * inputBuffer );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackLockedProcessDoneFunc)( void * inst, const OpenMPT_SoundDevice_TimeInfo * timeInfo );
	void (OPENMPT_WINESUPPORT_WRAPPER_CALL * SoundCallbackUnlockFunc)( void * inst );
} OpenMPT_Wine_Wrapper_SoundDevice_ICallback;

#ifdef WINE_THREAD
typedef enum OpenMPT_Wine_Wrapper_AudioThreadCommand {
	AudioThreadCommandInvalid       = -1
	, AudioThreadCommandExit        = 0
	, AudioThreadCommandLock        = 1
	, AudioThreadCommandClock       = 2
	, AudioThreadCommandReadPrepare = 3
	, AudioThreadCommandReadUint8   = 4
	, AudioThreadCommandReadInt8    = 5
	, AudioThreadCommandReadInt16   = 6
	, AudioThreadCommandReadInt24   = 7
	, AudioThreadCommandReadInt32   = 8
	, AudioThreadCommandReadFloat   = 9
	, AudioThreadCommandReadDouble  = 10
	, AudioThreadCommandReadDone    = 11
	, AudioThreadCommandUnlock      = 12
} OpenMPT_Wine_Wrapper_AudioThreadCommand;
#endif

typedef struct OpenMPT_Wine_Wrapper_SoundDevice {
	OpenMPT_SoundDevice * impl;
	OpenMPT_SoundDevice_IMessageReceiver native_receiver;
	OpenMPT_SoundDevice_ICallback native_callback;
	OpenMPT_Wine_Wrapper_SoundDevice_IMessageReceiver wine_receiver;
	OpenMPT_Wine_Wrapper_SoundDevice_ICallback wine_callback;
#ifdef WINE_THREAD
	HANDLE audiothread_startup_done;
	OpenMPT_Semaphore * audiothread_sem_request;
	OpenMPT_Semaphore * audiothread_sem_done;
	OpenMPT_Wine_Wrapper_AudioThreadCommand audiothread_command;
	const OpenMPT_SoundDevice_TimeInfo * audiothread_command_timeInfo;
	const OpenMPT_SoundDevice_BufferFormat * audiothread_command_bufferFormat;
	const OpenMPT_SoundDevice_BufferAttributes * audiothread_command_bufferAttributes;
	uintptr_t audiothread_command_numFrames;
	union {
		uint8_t * buf_uint8;
		int8_t * buf_int8;
		int16_t * buf_int16;
		OpenMPT_int24 * buf_int24;
		int32_t * buf_int32;
		float * buf_float;
		double * buf_double;
	} audiothread_command_buffer;
	union {
		const uint8_t * buf_uint8;
		const int8_t * buf_int8;
		const int16_t * buf_int16;
		const OpenMPT_int24 * buf_int24;
		const int32_t * buf_int32;
		const float * buf_float;
		const double * buf_double;
	} audiothread_command_inputBuffer;
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

static void OPENMPT_WINESUPPORT_CALL SoundCallbackGetReferenceClockNowNanosecondsFunc( void * inst, uint64_t * result ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
	return sd->wine_callback.SoundCallbackGetReferenceClockNowNanosecondsFunc( sd->wine_callback.inst, result );
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackPreStartFunc( void * inst ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
	return sd->wine_callback.SoundCallbackPreStartFunc( sd->wine_callback.inst );
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackPostStopFunc( void * inst ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
	return sd->wine_callback.SoundCallbackPostStopFunc( sd->wine_callback.inst );
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackIsLockedByCurrentThreadFunc( void * inst, uintptr_t * result ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
	return sd->wine_callback.SoundCallbackIsLockedByCurrentThreadFunc( sd->wine_callback.inst, result );
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackLockFunc( void * inst ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandLock;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_callback.SoundCallbackLockFunc( sd->wine_callback.inst );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackLockedGetReferenceClockNowNanosecondsFunc( void * inst, uint64_t * result ) {
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
	return sd->wine_callback.SoundCallbackLockedGetReferenceClockNowNanosecondsFunc( sd->wine_callback.inst, result );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackLockedProcessPrepareFunc( void * inst, const OpenMPT_SoundDevice_TimeInfo * timeInfo ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandReadPrepare;
	sd->audiothread_command_timeInfo = timeInfo;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_callback.SoundCallbackLockedProcessPrepareFunc( sd->wine_callback.inst, timeInfo );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackLockedProcessUint8Func( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, uint8_t * buffer, const uint8_t * inputBuffer ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandReadUint8;
	sd->audiothread_command_bufferFormat = bufferFormat;
	sd->audiothread_command_numFrames = numFrames;
	sd->audiothread_command_buffer.buf_uint8 = buffer;
	sd->audiothread_command_inputBuffer.buf_uint8 = inputBuffer;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_callback.SoundCallbackLockedProcessUint8Func( sd->wine_callback.inst, bufferFormat, bufferAttributes, numFrames, buffer, inputBuffer );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackLockedProcessInt8Func( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int8_t * buffer, const int8_t * inputBuffer ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandReadInt8;
	sd->audiothread_command_bufferFormat = bufferFormat;
	sd->audiothread_command_numFrames = numFrames;
	sd->audiothread_command_buffer.buf_int8 = buffer;
	sd->audiothread_command_inputBuffer.buf_int8 = inputBuffer;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_callback.SoundCallbackLockedProcessInt8Func( sd->wine_callback.inst, bufferFormat, bufferAttributes, numFrames, buffer, inputBuffer );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackLockedProcessInt16Func( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int16_t * buffer, const int16_t * inputBuffer ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandReadInt16;
	sd->audiothread_command_bufferFormat = bufferFormat;
	sd->audiothread_command_numFrames = numFrames;
	sd->audiothread_command_buffer.buf_int16 = buffer;
	sd->audiothread_command_inputBuffer.buf_int16 = inputBuffer;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_callback.SoundCallbackLockedProcessInt16Func( sd->wine_callback.inst, bufferFormat, bufferAttributes, numFrames, buffer, inputBuffer );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackLockedProcessInt24Func( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, OpenMPT_int24 * buffer, const OpenMPT_int24 * inputBuffer ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandReadInt24;
	sd->audiothread_command_bufferFormat = bufferFormat;
	sd->audiothread_command_numFrames = numFrames;
	sd->audiothread_command_buffer.buf_int24 = buffer;
	sd->audiothread_command_inputBuffer.buf_int24 = inputBuffer;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_callback.SoundCallbackLockedProcessInt24Func( sd->wine_callback.inst, bufferFormat, bufferAttributes, numFrames, buffer, inputBuffer );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackLockedProcessInt32Func( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, int32_t * buffer, const int32_t * inputBuffer ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandReadInt32;
	sd->audiothread_command_bufferFormat = bufferFormat;
	sd->audiothread_command_numFrames = numFrames;
	sd->audiothread_command_buffer.buf_int32 = buffer;
	sd->audiothread_command_inputBuffer.buf_int32 = inputBuffer;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_callback.SoundCallbackLockedProcessInt32Func( sd->wine_callback.inst, bufferFormat, bufferAttributes, numFrames, buffer, inputBuffer );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackLockedProcessFloatFunc( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, float * buffer, const float * inputBuffer ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandReadFloat;
	sd->audiothread_command_bufferFormat = bufferFormat;
	sd->audiothread_command_numFrames = numFrames;
	sd->audiothread_command_buffer.buf_float = buffer;
	sd->audiothread_command_inputBuffer.buf_float = inputBuffer;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_callback.SoundCallbackLockedProcessFloatFunc( sd->wine_callback.inst, bufferFormat, bufferAttributes, numFrames, buffer, inputBuffer );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackLockedProcessDoubleFunc( void * inst, const OpenMPT_SoundDevice_BufferFormat * bufferFormat, uintptr_t numFrames, double * buffer, const double * inputBuffer ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandReadDouble;
	sd->audiothread_command_bufferFormat = bufferFormat;
	sd->audiothread_command_numFrames = numFrames;
	sd->audiothread_command_buffer.buf_double = buffer;
	sd->audiothread_command_inputBuffer.buf_double = inputBuffer;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_callback.SoundCallbackLockedProcessDoubleFunc( sd->wine_callback.inst, bufferFormat, bufferAttributes, numFrames, buffer, inputBuffer );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackLockedProcessDoneFunc( void * inst, const OpenMPT_SoundDevice_TimeInfo * timeInfo ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandReadDone;
	sd->audiothread_command_timeInfo = timeInfo;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_callback.SoundCallbackLockedProcessDoneFunc( sd->wine_callback.inst, timeInfo );
#endif
}
static void OPENMPT_WINESUPPORT_CALL SoundCallbackUnlockFunc( void * inst ) {
	OpenMPT_Wine_Wrapper_SoundDevice * sd = (OpenMPT_Wine_Wrapper_SoundDevice*)inst;
	if ( !sd ) {
		return;
	}
#ifdef WINE_THREAD
	sd->audiothread_command = AudioThreadCommandUnlock;
	OpenMPT_Semaphore_Post( sd->audiothread_sem_request );
	OpenMPT_Semaphore_Wait( sd->audiothread_sem_done );
#else
	return sd->wine_callback.SoundCallbackUnlockFunc( sd->wine_callback.inst );
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
				if(sd->wine_callback.SoundCallbackLockFunc)
					sd->wine_callback.SoundCallbackLockFunc( sd->wine_callback.inst );
				break;
			case AudioThreadCommandClock:
				if(sd->wine_callback.SoundCallbackLockedGetReferenceClockNowNanosecondsFunc)
					sd->wine_callback.SoundCallbackLockedGetReferenceClockNowNanosecondsFunc( sd->wine_callback.inst, sd->audiothread_command_result );
				break;
			case AudioThreadCommandReadPrepare:
				if(sd->wine_callback.SoundCallbackLockedProcessPrepareFunc)
					sd->wine_callback.SoundCallbackLockedProcessPrepareFunc
						( sd->wine_callback.inst
						, sd->audiothread_command_timeInfo
						);
				break;
			case AudioThreadCommandReadUint8:
				if(sd->wine_callback.SoundCallbackLockedProcessUint8Func)
					sd->wine_callback.SoundCallbackLockedProcessUint8Func
						( sd->wine_callback.inst
						, sd->audiothread_command_bufferFormat
						, sd->audiothread_command_numFrames
						, sd->audiothread_command_buffer.buf_uint8
						, sd->audiothread_command_inputBuffer.buf_uint8
						);
				break;
			case AudioThreadCommandReadInt8:
				if(sd->wine_callback.SoundCallbackLockedProcessInt8Func)
					sd->wine_callback.SoundCallbackLockedProcessInt8Func
						( sd->wine_callback.inst
						, sd->audiothread_command_bufferFormat
						, sd->audiothread_command_numFrames
						, sd->audiothread_command_buffer.buf_int8
						, sd->audiothread_command_inputBuffer.buf_int8
						);
				break;
			case AudioThreadCommandReadInt16:
				if(sd->wine_callback.SoundCallbackLockedProcessInt16Func)
					sd->wine_callback.SoundCallbackLockedProcessInt16Func
						( sd->wine_callback.inst
						, sd->audiothread_command_bufferFormat
						, sd->audiothread_command_numFrames
						, sd->audiothread_command_buffer.buf_int16
						, sd->audiothread_command_inputBuffer.buf_int16
						);
				break;
			case AudioThreadCommandReadInt24:
				if(sd->wine_callback.SoundCallbackLockedProcessInt24Func)
					sd->wine_callback.SoundCallbackLockedProcessInt24Func
						( sd->wine_callback.inst
						, sd->audiothread_command_bufferFormat
						, sd->audiothread_command_numFrames
						, sd->audiothread_command_buffer.buf_int24
						, sd->audiothread_command_inputBuffer.buf_int24
						);
				break;
			case AudioThreadCommandReadInt32:
				if(sd->wine_callback.SoundCallbackLockedProcessInt32Func)
					sd->wine_callback.SoundCallbackLockedProcessInt32Func
						( sd->wine_callback.inst
						, sd->audiothread_command_bufferFormat
						, sd->audiothread_command_numFrames
						, sd->audiothread_command_buffer.buf_int32
						, sd->audiothread_command_inputBuffer.buf_int32
						);
				break;
			case AudioThreadCommandReadFloat:
				if(sd->wine_callback.SoundCallbackLockedProcessFloatFunc)
					sd->wine_callback.SoundCallbackLockedProcessFloatFunc
						( sd->wine_callback.inst
						, sd->audiothread_command_bufferFormat
						, sd->audiothread_command_numFrames
						, sd->audiothread_command_buffer.buf_float
						, sd->audiothread_command_inputBuffer.buf_float
						);
				break;
			case AudioThreadCommandReadDouble:
				if(sd->wine_callback.SoundCallbackLockedProcessDoubleFunc)
					sd->wine_callback.SoundCallbackLockedProcessDoubleFunc
						( sd->wine_callback.inst
						, sd->audiothread_command_bufferFormat
						, sd->audiothread_command_numFrames
						, sd->audiothread_command_buffer.buf_double
						, sd->audiothread_command_inputBuffer.buf_double
						);
				break;
			case AudioThreadCommandReadDone:
				if(sd->wine_callback.SoundCallbackLockedProcessDoneFunc)
					sd->wine_callback.SoundCallbackLockedProcessDoneFunc
						( sd->wine_callback.inst
						, sd->audiothread_command_timeInfo
						);
				break;
			case AudioThreadCommandUnlock:
				if(sd->wine_callback.SoundCallbackUnlockFunc)
					sd->wine_callback.SoundCallbackUnlockFunc( sd->wine_callback.inst );
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

OPENMPT_WINESUPPORT_WRAPPER_API void OPENMPT_WINESUPPORT_WRAPPER_CALL OpenMPT_Wine_Wrapper_SoundDevice_SetCallback( OpenMPT_Wine_Wrapper_SoundDevice * sd, const OpenMPT_Wine_Wrapper_SoundDevice_ICallback * callback ) {
	OpenMPT_SoundDevice_SetCallback( sd->impl, NULL );
	ZeroMemory( &( sd->wine_callback ), sizeof( sd->wine_callback) );
	if(callback)
	{
		sd->wine_callback = *callback;
	}
	sd->native_callback.inst = sd;
	sd->native_callback.SoundCallbackGetReferenceClockNowNanosecondsFunc = &SoundCallbackGetReferenceClockNowNanosecondsFunc;
	sd->native_callback.SoundCallbackPreStartFunc = &SoundCallbackPreStartFunc;
	sd->native_callback.SoundCallbackPostStopFunc = &SoundCallbackPostStopFunc;
	sd->native_callback.SoundCallbackIsLockedByCurrentThreadFunc = &SoundCallbackIsLockedByCurrentThreadFunc;
	sd->native_callback.SoundCallbackLockFunc = &SoundCallbackLockFunc;
	sd->native_callback.SoundCallbackLockedGetReferenceClockNowNanosecondsFunc = &SoundCallbackLockedGetReferenceClockNowNanosecondsFunc;
	sd->native_callback.SoundCallbackLockedProcessPrepareFunc = &SoundCallbackLockedProcessPrepareFunc;
	sd->native_callback.SoundCallbackLockedProcessUint8Func = &SoundCallbackLockedProcessUint8Func;
	sd->native_callback.SoundCallbackLockedProcessInt8Func = &SoundCallbackLockedProcessInt8Func;
	sd->native_callback.SoundCallbackLockedProcessInt16Func = &SoundCallbackLockedProcessInt16Func;
	sd->native_callback.SoundCallbackLockedProcessInt24Func = &SoundCallbackLockedProcessInt24Func;
	sd->native_callback.SoundCallbackLockedProcessInt32Func = &SoundCallbackLockedProcessInt32Func;
	sd->native_callback.SoundCallbackLockedProcessFloatFunc = &SoundCallbackLockedProcessFloatFunc;
	sd->native_callback.SoundCallbackLockedProcessDoubleFunc = &SoundCallbackLockedProcessDoubleFunc;
	sd->native_callback.SoundCallbackLockedProcessDoneFunc = &SoundCallbackLockedProcessDoneFunc;
	sd->native_callback.SoundCallbackUnlockFunc = &SoundCallbackUnlockFunc;
	OpenMPT_SoundDevice_SetCallback( sd->impl, &sd->native_callback );
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
