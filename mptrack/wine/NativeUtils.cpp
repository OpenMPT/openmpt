
#include "stdafx.h"

#include "NativeUtils.h"

#include <string>
#ifndef _MSC_VER
#include <condition_variable>
#include <mutex>
#include <thread>
#endif

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

OPENMPT_NAMESPACE_BEGIN

#ifndef _MSC_VER

namespace mpt {

class semaphore {
private:
	unsigned int count;
	unsigned int waiters_count;
	std::mutex mutex;
	std::condition_variable count_nonzero;
public:
	semaphore( unsigned int initial_count = 0 )
		: count(initial_count)
		, waiters_count(0)
	{
		return;
	}
	~semaphore() {
		return;
	}
	void wait() {
		std::unique_lock<std::mutex> l(mutex);
		waiters_count++;
		while ( count == 0 ) {
			count_nonzero.wait( l );
		}
		waiters_count--;
		count--;
	}
	void post() {
		std::unique_lock<std::mutex> l(mutex);
		if ( waiters_count > 0 ) {
			count_nonzero.notify_one();
		}
		count++;
	}
	void lock() {
		wait();
	}
	void unlock() {
		post();
	}
};

} // namespace mpt

#endif

OPENMPT_NAMESPACE_END

extern "C" {

OPENMPT_WINESUPPORT_API void * OPENMPT_WINESUPPORT_CALL OpenMPT_Alloc( size_t size ) {
	return calloc( 1, size );
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_Free( void * mem ) {
	if ( mem == nullptr ) {
		return;
	}
	free( mem );
	return;
}

OPENMPT_WINESUPPORT_API size_t OPENMPT_WINESUPPORT_CALL OpenMPT_String_Length( const char * str ) {
	size_t len = 0;
	if ( !str ) {
		return len;
	}
	len = strlen( str );
	return len;
}

OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_String_Duplicate( const char * src ) {
	if ( !src ) {
		return strdup( "" );
	}
	return strdup( src );
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_String_Free( char * str ) {
	OpenMPT_Free( str );
}

typedef struct OpenMPT_Semaphore {
#ifndef _MSC_VER
	OPENMPT_NAMESPACE::mpt::semaphore * impl;
#endif
} OpenMPT_Semaphore;

OPENMPT_WINESUPPORT_API OpenMPT_Semaphore * OPENMPT_WINESUPPORT_CALL OpenMPT_Semaphore_Construct(void) {
#ifndef _MSC_VER
	OpenMPT_Semaphore * sem = (OpenMPT_Semaphore*)OpenMPT_Alloc( sizeof( OpenMPT_Semaphore ) );
	sem->impl = new OPENMPT_NAMESPACE::mpt::semaphore(0);
	return sem;
#else
	return nullptr;
#endif
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_Semaphore_Destruct( OpenMPT_Semaphore * sem ) {
#ifndef _MSC_VER
	delete sem->impl;
	sem->impl = nullptr;
	OpenMPT_Free( sem );
#else
	MPT_UNREFERENCED_PARAMETER(sem);
#endif
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_Semaphore_Wait( OpenMPT_Semaphore * sem ) {
#ifndef _MSC_VER
	sem->impl->wait();
#else
	MPT_UNREFERENCED_PARAMETER(sem);
#endif
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_Semaphore_Post( OpenMPT_Semaphore * sem ) {
#ifndef _MSC_VER
	sem->impl->post();
#else
	MPT_UNREFERENCED_PARAMETER(sem);
#endif
}

} // extern "C"
