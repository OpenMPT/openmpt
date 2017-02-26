
#ifndef OPENMPT_WINE_UTILS_H
#define OPENMPT_WINE_UTILS_H

#include "NativeConfig.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

OPENMPT_WINESUPPORT_API void * OPENMPT_WINESUPPORT_CALL OpenMPT_Alloc( size_t size );
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_Free( void * mem );

OPENMPT_WINESUPPORT_API size_t OPENMPT_WINESUPPORT_CALL OpenMPT_String_Length( const char * str );
OPENMPT_WINESUPPORT_API char * OPENMPT_WINESUPPORT_CALL OpenMPT_String_Duplicate( const char * src );
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_String_Free( char * str );

typedef struct OpenMPT_Semaphore OpenMPT_Semaphore;
OPENMPT_WINESUPPORT_API OpenMPT_Semaphore * OPENMPT_WINESUPPORT_CALL OpenMPT_Semaphore_Construct(void);
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_Semaphore_Destruct( OpenMPT_Semaphore * sem );
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_Semaphore_Wait( OpenMPT_Semaphore * sem );
OPENMPT_WINESUPPORT_API void OPENMPT_WINESUPPORT_CALL OpenMPT_Semaphore_Post( OpenMPT_Semaphore * sem );

#ifdef __cplusplus
}
#endif

#endif // OPENMPT_WINE_UTILS_H
