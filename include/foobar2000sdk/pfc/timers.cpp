#include "pfc.h"

namespace pfc {

#ifdef PFC_HAVE_PROFILER

profiler_static::profiler_static(const char * p_name)
{
	name = p_name;
	total_time = 0;
	num_called = 0;
}

profiler_static::~profiler_static()
{
	try {
		pfc::string_fixed_t<511> message;
		message << "profiler: " << pfc::format_pad_left<pfc::string_fixed_t<127> >(48,' ',name) << " - " << 
			pfc::format_pad_right<pfc::string_fixed_t<128> >(16,' ',pfc::format_uint(total_time) ) << " cycles";

		if (num_called > 0) {
			message << " (executed " << num_called << " times, " << (total_time / num_called) << " average)";
		}
		message << "\n";
		OutputDebugStringA(message);
	} catch(...) {
		//should never happen
		OutputDebugString(_T("unexpected profiler failure\n"));
	}
}
#endif

#ifndef _WIN32

    void hires_timer::start() {
        m_start = nixGetTime();
    }
    double hires_timer::query() const {
        return nixGetTime() - m_start;
    }
    double hires_timer::query_reset() {
        double t = nixGetTime();
        double r = t - m_start;
        m_start = t;
        return r;
    }
    pfc::string8 hires_timer::queryString(unsigned precision) const {
        return format_time_ex( query(), precision ).get_ptr();
    }
#endif


    uint64_t fileTimeWtoU(uint64_t ft) {
        return (ft - 116444736000000000 + /*rounding*/10000000/2) / 10000000;
    }
    uint64_t fileTimeUtoW(uint64_t ft) {
        return (ft * 10000000) + 116444736000000000;
    }
#ifndef _WIN32
    uint64_t fileTimeUtoW(const timespec & ts) {
        uint64_t ft = (uint64_t)ts.tv_sec * 10000000 + (uint64_t)ts.tv_nsec / 100;
        return ft + 116444736000000000;
    }
#endif
    
	uint64_t fileTimeNow() {
#ifdef _WIN32
		uint64_t ret;
		GetSystemTimeAsFileTime((FILETIME*)&ret);
		return ret;
#else
		return fileTimeUtoW(time(NULL));
#endif
	}
    
}
