#include "pfc.h"
#ifdef _MSC_VER
#include <intrin.h>
#include <assert.h>
#endif
#ifndef _MSC_VER
#include <signal.h>
#endif

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#include <math.h>

namespace pfc {
	bool permutation_is_valid(t_size const * order, t_size count) {
		bit_array_bittable found(count);
		for(t_size walk = 0; walk < count; ++walk) {
			if (order[walk] >= count) return false;
			if (found[walk]) return false;
			found.set(walk,true);
		}
		return true;
	}
	void permutation_validate(t_size const * order, t_size count) {
		if (!permutation_is_valid(order,count)) throw exception_invalid_permutation();
	}

	t_size permutation_find_reverse(t_size const * order, t_size count, t_size value) {
		if (value >= count) return ~0;
		for(t_size walk = 0; walk < count; ++walk) {
			if (order[walk] == value) return walk;
		}
		return ~0;
	}
    
    void create_move_item_permutation( size_t * order, size_t count, size_t from, size_t to ) {
        PFC_ASSERT( from < count );
        PFC_ASSERT( to < count );
        for ( size_t w = 0; w < count; ++w ) {
            size_t i = w;
            if ( w == to ) i = from;
            else if ( w < to && w >= from ) {
                ++i;
            } else if ( w > to && w <= from ) {
                --i;
            }
            order[w] = i;
        }
    }
    
    void create_move_items_permutation(t_size * p_output,t_size p_count,const bit_array & p_selection,int p_delta) {
		t_size * const order = p_output;
		const t_size count = p_count;

		pfc::array_t<bool> selection; selection.set_size(p_count);
		
		for(t_size walk = 0; walk < count; ++walk) {
			order[walk] = walk;
			selection[walk] = p_selection[walk];
		}

		if (p_delta<0)
		{
			for(;p_delta<0;p_delta++)
			{
				t_size idx;
				for(idx=1;idx<count;idx++)
				{
					if (selection[idx] && !selection[idx-1])
					{
						pfc::swap_t(order[idx],order[idx-1]);
						pfc::swap_t(selection[idx],selection[idx-1]);
					}
				}
			}
		}
		else
		{
			for(;p_delta>0;p_delta--)
			{
				t_size idx;
				for(idx=count-2;(int)idx>=0;idx--)
				{
					if (selection[idx] && !selection[idx+1])
					{
						pfc::swap_t(order[idx],order[idx+1]);
						pfc::swap_t(selection[idx],selection[idx+1]);
					}
				}
			}
		}
	}
}

void order_helper::g_swap(t_size * data,t_size ptr1,t_size ptr2)
{
	t_size temp = data[ptr1];
	data[ptr1] = data[ptr2];
	data[ptr2] = temp;
}


t_size order_helper::g_find_reverse(const t_size * order,t_size val)
{
	t_size prev = val, next = order[val];
	while(next != val)
	{
		prev = next;
		next = order[next];
	}
	return prev;
}


void order_helper::g_reverse(t_size * order,t_size base,t_size count)
{
	t_size max = count>>1;
	t_size n;
	t_size base2 = base+count-1;
	for(n=0;n<max;n++)
		g_swap(order,base+n,base2-n);
}


namespace pfc {
#ifdef PFC_FOOBAR2000_CLASSIC
	void crashHook();
#endif

	void crashImpl() {
#if defined(_MSC_VER)
		__debugbreak();
#else
#if defined(__ANDROID__) && PFC_DEBUG
		nixSleep(1);
#endif
		raise(SIGINT);
#endif
	}

	void crash() {
#ifdef PFC_FOOBAR2000_CLASSIC
		crashHook();
#else
		crashImpl();
#endif
	}
} // namespace pfc



void pfc::byteswap_raw(void * p_buffer,const t_size p_bytes) {
	t_uint8 * ptr = (t_uint8*)p_buffer;
	t_size n;
	for(n=0;n<p_bytes>>1;n++) swap_t(ptr[n],ptr[p_bytes-n-1]);
}

void pfc::outputDebugLine(const char * msg) {
#ifdef _WIN32
	OutputDebugString(pfc::stringcvt::string_os_from_utf8(PFC_string_formatter() << msg << "\n") );
#elif defined(__ANDROID__)
	__android_log_write(ANDROID_LOG_INFO, "Debug", msg);
#else
	printf("%s\n", msg);
#endif
}

#if PFC_DEBUG

#ifdef _WIN32
void pfc::myassert_win32(const wchar_t * _Message, const wchar_t *_File, unsigned _Line) {
	if (IsDebuggerPresent()) pfc::crash();
	PFC_DEBUGLOG << "PFC_ASSERT failure: " << _Message;
	PFC_DEBUGLOG << "PFC_ASSERT location: " << _File << " : " << _Line;
	_wassert(_Message,_File,_Line);
}
#else

void pfc::myassert(const char * _Message, const char *_File, unsigned _Line)
{
	PFC_DEBUGLOG << "Assert failure: \"" << _Message << "\" in: " << _File << " line " << _Line;
	crash();
}
#endif

#endif


t_uint64 pfc::pow_int(t_uint64 base, t_uint64 exp) {
	t_uint64 mul = base;
	t_uint64 val = 1;
	t_uint64 mask = 1;
	while(exp != 0) {
		if (exp & mask) {
			val *= mul;
			exp ^= mask;
		}
		mul = mul * mul;
		mask <<= 1;
	}
	return val;
}

double pfc::exp_int( const double base, const int expS ) {
    //    return pow(base, (double)v);
    
    bool neg;
    unsigned exp;
    if (expS < 0) {
        neg = true;
        exp = (unsigned) -expS;
    } else {
        neg = false;
        exp = (unsigned) expS;
    }
    double v = 1.0;
    if (true) {
        if (exp) {
            double mul = base;
            for(;;) {
                if (exp & 1) v *= mul;
                exp >>= 1;
                if (exp == 0) break;
                mul *= mul;
            }
        }
    } else {
        for(unsigned i = 0; i < exp; ++i) {
            v *= base;
        }
    }
    if (neg) v = 1.0 / v;
    return v;
}


t_int32 pfc::rint32(double p_val) { return (t_int32)floor(p_val + 0.5); }
t_int64 pfc::rint64(double p_val) { return (t_int64)floor(p_val + 0.5); }


namespace pfc {
	// bigmem impl

	void bigmem::resize(size_t newSize) {
		clear();
		m_data.set_size((newSize + slice - 1) / slice);
		m_data.fill_null();
		for (size_t walk = 0; walk < m_data.get_size(); ++walk) {
			size_t thisSlice = slice;
			if (walk + 1 == m_data.get_size()) {
				size_t cut = newSize % slice;
				if (cut) thisSlice = cut;
			}
			void* ptr = malloc(thisSlice);
			if (ptr == NULL) { clear(); throw std::bad_alloc(); }
			m_data[walk] = (uint8_t*)ptr;
		}
		m_size = newSize;
	}
	void bigmem::clear() {
		for (size_t walk = 0; walk < m_data.get_size(); ++walk) free(m_data[walk]);
		m_data.set_size(0);
		m_size = 0;
	}
	void bigmem::read(void * ptrOut, size_t bytes, size_t offset) {
		PFC_ASSERT(offset + bytes <= size());
		uint8_t * outWalk = (uint8_t*)ptrOut;
		while (bytes > 0) {
			size_t o1 = offset / slice, o2 = offset % slice;
			size_t delta = slice - o2; if (delta > bytes) delta = bytes;
			memcpy(outWalk, m_data[o1] + o2, delta);
			offset += delta;
			bytes -= delta;
			outWalk += delta;
		}
	}
	void bigmem::write(const void * ptrIn, size_t bytes, size_t offset) {
		PFC_ASSERT(offset + bytes <= size());
		const uint8_t * inWalk = (const uint8_t*)ptrIn;
		while (bytes > 0) {
			size_t o1 = offset / slice, o2 = offset % slice;
			size_t delta = slice - o2; if (delta > bytes) delta = bytes;
			memcpy(m_data[o1] + o2, inWalk, delta);
			offset += delta;
			bytes -= delta;
			inWalk += delta;
		}
	}
	uint8_t * bigmem::_slicePtr(size_t which) { return m_data[which]; }
	size_t bigmem::_sliceCount() { return m_data.get_size(); }
	size_t bigmem::_sliceSize(size_t which) {
		if (which + 1 == _sliceCount()) {
			size_t s = m_size % slice;
			if (s) return s;
		}
		return slice;
	}

}