#include "pfc.h"


#if PFC_HAVE_CPUID

namespace pfc {
	bool query_cpu_feature_set(unsigned p_value) {
#ifdef _MSC_VER
		__try {
#endif
			if (p_value & (CPU_HAVE_SSE | CPU_HAVE_SSE2 | CPU_HAVE_SSE3 | CPU_HAVE_SSSE3 | CPU_HAVE_SSE41 | CPU_HAVE_SSE42)) {
				int buffer[4];
				__cpuid(buffer,1);
				if (p_value & CPU_HAVE_SSE) {
					if ((buffer[3]&(1<<25)) == 0) return false;
				}
				if (p_value & CPU_HAVE_SSE2) {
					if ((buffer[3]&(1<<26)) == 0) return false;
				}
				if (p_value & CPU_HAVE_SSE3) {
					if ((buffer[2]&(1<<0)) == 0) return false;
				}
				if (p_value & CPU_HAVE_SSSE3) {
					if ((buffer[2]&(1<<9)) == 0) return false;
				}
				if (p_value & CPU_HAVE_SSE41) {
					if ((buffer[2]&(1<<19)) == 0) return false;
				}
				if (p_value & CPU_HAVE_SSE42) {
					if ((buffer[2]&(1<<20)) == 0) return false;
				}
			}
	#ifdef _M_IX86
			if (p_value & (CPU_HAVE_3DNOW_EX | CPU_HAVE_3DNOW)) {
				int buffer_amd[4];
				__cpuid(buffer_amd,0x80000000);
				if ((unsigned)buffer_amd[0] < 0x80000001) return false;
				__cpuid(buffer_amd,0x80000001);
			
				if (p_value & CPU_HAVE_3DNOW) {
					if ((buffer_amd[3]&(1<<31)) == 0) return false;
				}
				if (p_value & CPU_HAVE_3DNOW_EX) {
					if ((buffer_amd[3]&(1<<30)) == 0) return false;
				}
			}
	#endif
			return true;
#ifdef _MSC_VER
		} __except(1) {
			return false;
		}
#endif
	}
}

#endif
