
#include "stdafx.h"

#include "NativeUtils.h"

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

OPENMPT_NAMESPACE_BEGIN

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
	OPENMPT_NAMESPACE::mpt::semaphore * impl;
} OpenMPT_Semaphore;

OPENMPT_WINESUPPORT_API OpenMPT_Semaphore * OPENMPT_WINESUPPORT_CALL OpenMPT_Semaphore_Construct(void) {
	OpenMPT_Semaphore * sem = (OpenMPT_Semaphore*)OpenMPT_Alloc( sizeof( OpenMPT_Semaphore ) );
	if ( !sem ) {
		return nullptr;
	}
	sem->impl = new OPENMPT_NAMESPACE::mpt::semaphore(0);
	if ( !sem->impl ) {
		OpenMPT_Free( sem );
		return nullptr;
	}
	return sem;
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_Semaphore_Destruct( OpenMPT_Semaphore * sem ) {
	delete sem->impl;
	sem->impl = nullptr;
	OpenMPT_Free( sem );
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_Semaphore_Wait( OpenMPT_Semaphore * sem ) {
	sem->impl->wait();
}

OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_Semaphore_Post( OpenMPT_Semaphore * sem ) {
	sem->impl->post();
}

} // extern "C"
