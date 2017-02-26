
#ifndef OPENMPT_WINE_H
#define OPENMPT_WINE_H

#include "NativeConfig.h"
#include "NativeUtils.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_Init(void);
OPENMPT_WINESUPPORT_API uintptr_t OPENMPT_WINESUPPORT_CALL OpenMPT_Fini(void);

#ifdef __cplusplus
}
#endif

#endif // OPENMPT_WINE_H
