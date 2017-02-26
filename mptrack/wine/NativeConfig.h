
#ifndef OPENMPT_WNESUPPORT_CONFIG_H
#define OPENMPT_WNESUPPORT_CONFIG_H

#include <stdint.h>

#if defined(__DOXYGEN__)

#define OPENMPT_API_HELPER_EXPORT
#define OPENMPT_API_HELPER_IMPORT
#define OPENMPT_API_HELPER_PUBLIC
#define OPENMPT_API_HELPER_LOCAL

#elif defined(MPT_WINEGCC)

#define OPENMPT_API_HELPER_EXPORT __attribute__((visibility("default")))
#define OPENMPT_API_HELPER_IMPORT __attribute__((visibility("default")))
#define OPENMPT_API_HELPER_PUBLIC __attribute__((visibility("default")))
#define OPENMPT_API_HELPER_LOCAL  __attribute__((visibility("hidden")))

#elif defined(_MSC_VER)

#define OPENMPT_API_HELPER_EXPORT __declspec(dllexport)
#define OPENMPT_API_HELPER_IMPORT __declspec(dllimport)
#define OPENMPT_API_HELPER_PUBLIC 
#define OPENMPT_API_HELPER_LOCAL  

#elif defined(__GNUC__) || defined(__clang__)

#define OPENMPT_API_HELPER_EXPORT __attribute__((visibility("default")))
#define OPENMPT_API_HELPER_IMPORT __attribute__((visibility("default")))
#define OPENMPT_API_HELPER_PUBLIC __attribute__((visibility("default")))
#define OPENMPT_API_HELPER_LOCAL  __attribute__((visibility("hidden")))

#else

#define OPENMPT_API_HELPER_EXPORT 
#define OPENMPT_API_HELPER_IMPORT 
#define OPENMPT_API_HELPER_PUBLIC 
#define OPENMPT_API_HELPER_LOCAL  

#endif

#if defined(__DOXYGEN__)

#define OPENMPT_API_WINE_MS_CDECL    
#define OPENMPT_API_WINE_MS_STDCALL  
#define OPENMPT_API_WINE_MS_FASTCALL 
#define OPENMPT_API_WINE_MS_THISCALL 
#undef  OPENMPT_API_WINE_SYSV        

#elif defined(MPT_WINEGCC)

#ifdef _WIN64
#define OPENMPT_API_WINE_MS_CDECL    __attribute__((ms_abi))
#define OPENMPT_API_WINE_MS_STDCALL  __attribute__((ms_abi))
#define OPENMPT_API_WINE_MS_FASTCALL __attribute__((ms_abi))
#define OPENMPT_API_WINE_MS_THISCALL __attribute__((ms_abi))
#else
// winegcc on Ubuntu 16.04, wine-development 1.9.6 completely explodes in
// incomprehensible ways while parsing __attribute__((cdecl)).
#if defined(__cdecl)
#define OPENMPT_API_WINE_MS_CDECL    __attribute__((ms_abi)) __cdecl
#else
#define OPENMPT_API_WINE_MS_CDECL    __attribute__((ms_abi)) __attribute__((cdecl))
#endif
#if defined(__stdcall)
#define OPENMPT_API_WINE_MS_STDCALL  __attribute__((ms_abi)) __stdcall
#else
#define OPENMPT_API_WINE_MS_STDCALL  __attribute__((ms_abi)) __attribute__((stdcall))
#endif
#if defined(__fastcall)
#define OPENMPT_API_WINE_MS_FASTCALL __attribute__((ms_abi)) __fastcall
#else
#define OPENMPT_API_WINE_MS_FASTCALL __attribute__((ms_abi)) __attribute__((fastcall))
#endif
#if defined(__thiscall)
#define OPENMPT_API_WINE_MS_THISCALL __attribute__((ms_abi)) __thiscall
#else
#define OPENMPT_API_WINE_MS_THISCALL __attribute__((ms_abi)) __attribute__((thiscall))
#endif
#endif
#define OPENMPT_API_WINE_SYSV        __attribute__((sysv_abi))

#elif defined(_MSC_VER)

#define OPENMPT_API_WINE_MS_CDECL    __cdecl
#define OPENMPT_API_WINE_MS_STDCALL  __stdcall
#define OPENMPT_API_WINE_MS_FASTCALL __fastcall
#define OPENMPT_API_WINE_MS_THISCALL __thiscall
#undef  OPENMPT_API_WINE_SYSV        

#elif defined(__GNUC__) || defined(__clang__)

#ifdef _WIN64
#define OPENMPT_API_WINE_MS_CDECL    __attribute__((ms_abi))
#define OPENMPT_API_WINE_MS_STDCALL  __attribute__((ms_abi))
#define OPENMPT_API_WINE_MS_FASTCALL __attribute__((ms_abi))
#define OPENMPT_API_WINE_MS_THISCALL __attribute__((ms_abi))
#else
// winegcc on Ubuntu 16.04, wine-development 1.9.6 completely explodes in
// incomprehensible ways while parsing __attribute__((cdecl)).
#if defined(__cdecl)
#define OPENMPT_API_WINE_MS_CDECL    __attribute__((ms_abi)) __cdecl
#else
#define OPENMPT_API_WINE_MS_CDECL    __attribute__((ms_abi)) __attribute__((cdecl))
#endif
#if defined(__stdcall)
#define OPENMPT_API_WINE_MS_STDCALL  __attribute__((ms_abi)) __stdcall
#else
#define OPENMPT_API_WINE_MS_STDCALL  __attribute__((ms_abi)) __attribute__((stdcall))
#endif
#if defined(__fastcall)
#define OPENMPT_API_WINE_MS_FASTCALL __attribute__((ms_abi)) __fastcall
#else
#define OPENMPT_API_WINE_MS_FASTCALL __attribute__((ms_abi)) __attribute__((fastcall))
#endif
#if defined(__thiscall)
#define OPENMPT_API_WINE_MS_THISCALL __attribute__((ms_abi)) __thiscall
#else
#define OPENMPT_API_WINE_MS_THISCALL __attribute__((ms_abi)) __attribute__((thiscall))
#endif
#endif
#define OPENMPT_API_WINE_SYSV        __attribute__((sysv_abi))

#endif

#if defined(MODPLUG_TRACKER) && (!(defined(MPT_BUILD_WINESUPPORT) || defined(MPT_BUILD_WINESUPPORT_WRAPPER)))

#define OPENMPT_WINESUPPORT_API
#define OPENMPT_WINESUPPORT_CALL
#define OPENMPT_WINESUPPORT_WRAPPER_API
#define OPENMPT_WINESUPPORT_WRAPPER_CALL

#else

#if defined(__DOXYGEN__)
#define OPENMPT_WINESUPPORT_CALL         OPENMPT_API_WINE_SYSV
#elif defined(MPT_WINEGCC)
#define OPENMPT_WINESUPPORT_CALL         OPENMPT_API_WINE_SYSV
#elif defined(_MSC_VER)
#define OPENMPT_WINESUPPORT_CALL         OPENMPT_API_WINE_MS_CDECL
#elif defined(__GNUC__) || defined(__clang__)
#define OPENMPT_WINESUPPORT_CALL         OPENMPT_API_WINE_SYSV
#endif
#define OPENMPT_WINESUPPORT_WRAPPER_CALL OPENMPT_API_WINE_MS_CDECL

#if defined(MPT_BUILD_WINESUPPORT)
#define OPENMPT_WINESUPPORT_API OPENMPT_API_HELPER_EXPORT
#else
#define OPENMPT_WINESUPPORT_API OPENMPT_API_HELPER_IMPORT
#endif

#if defined(MPT_BUILD_WINESUPPORT_WRAPPER)
#define OPENMPT_WINESUPPORT_WRAPPER_API OPENMPT_API_HELPER_EXPORT
#else
#define OPENMPT_WINESUPPORT_WRAPPER_API OPENMPT_API_HELPER_IMPORT
#endif

#endif

#endif // OPENMPT_WNESUPPORT_CONFIG_H
