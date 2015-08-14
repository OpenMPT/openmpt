#if !defined(PP_GETTICKCOUNT_H_INCLUDED) && defined(_WIN32)
#define PP_GETTICKCOUNT_H_INCLUDED

namespace PP {
#if _WIN32_WINNT >= 0x600
	typedef uint64_t tickcount_t;
	inline tickcount_t getTickCount() { return ::GetTickCount64(); }
#else
#define PFC_TICKCOUNT_32BIT
	typedef uint32_t tickcount_t;
	inline tickcount_t getTickCount() { return ::GetTickCount(); }
#endif

}

#endif // #if !defined(PP_GETTICKCOUNT_H_INCLUDED) && defined(_WIN32)