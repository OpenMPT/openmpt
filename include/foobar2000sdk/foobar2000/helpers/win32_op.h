#pragma once

PFC_NORETURN PFC_NOINLINE void WIN32_OP_FAIL();
PFC_NORETURN PFC_NOINLINE void WIN32_OP_FAIL_CRITICAL(const char * what);

#ifdef _DEBUG
void WIN32_OP_D_FAIL(const wchar_t * _Message, const wchar_t *_File, unsigned _Line);
#endif

//Throws an exception when (OP) evaluates to false/zero.
#define WIN32_OP(OP)	\
	{	\
		SetLastError(NO_ERROR);	\
		if (!(OP)) WIN32_OP_FAIL();	\
	}

// Kills the application with appropriate debug info when (OP) evaluates to false/zero.
#define WIN32_OP_CRITICAL(WHAT, OP)	\
	{	\
		SetLastError(NO_ERROR);	\
		if (!(OP)) WIN32_OP_FAIL_CRITICAL(WHAT);	\
	}

//WIN32_OP_D() acts like an assert specialized for win32 operations in debug build, ignores the return value / error codes in release build.
//Use WIN32_OP_D() instead of WIN32_OP() on operations that are extremely unlikely to fail, so failure condition checks are performed in the debug build only, to avoid bloating release code with pointless error checks.
#ifdef _DEBUG
#define WIN32_OP_D(OP)	\
	{	\
		SetLastError(NO_ERROR); \
		if (!(OP)) WIN32_OP_D_FAIL(PFC_WIDESTRING(#OP), PFC_WIDESTRING(__FILE__), __LINE__); \
	}

#else
#define WIN32_OP_D(OP) (void)( (OP), 0);
#endif

