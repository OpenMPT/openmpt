#include "pfc.h"

namespace pfc {
	void bit_array::for_each(bool value, size_t base, size_t max, std::function<void(size_t)> f) const {
		for( size_t idx = find_first(value, base, max); idx < max; idx = find_next(value, idx, max) ) {
			f(idx);
		}
	}
	t_size bit_array::find(bool val, t_size start, t_ssize count) const
	{
		t_ssize d, todo, ptr = start;
		if (count == 0) return start;
		else if (count<0) { d = -1; todo = -count; }
		else { d = 1; todo = count; }
		while (todo>0 && get(ptr) != val) { ptr += d;todo--; }
		return ptr;
	}

	t_size bit_array::calc_count(bool val, t_size start, t_size count, t_size count_max) const
	{
		t_size found = 0;
		t_size max = start + count;
		t_size ptr;
		for (ptr = find(val, start, count);found<count_max && ptr<max;ptr = find(val, ptr + 1, max - ptr - 1)) found++;
		return found;
	}
	t_size bit_array::find_first(bool val, t_size start, t_size max) const { 
		return find(val, start, max - start); 
	}
	t_size bit_array::find_next(bool val, t_size previous, t_size max) const { 
		return find(val, previous + 1, max - (previous + 1)); 
	}
	void bit_array::walk(size_t to, std::function< void ( size_t ) > f, bool val ) const {
		for ( size_t w = find_first(val, 0, to ); w < to; w = find_next(val, w, to) ) {
			f(w);
		}
	}
	void bit_array::walkBack(size_t from, std::function<void (size_t) > f, bool val) const {
		t_ssize walk = from;
		if ( walk == 0 ) return;
		for( ;; ) {
			walk = find(val, walk-1, - walk );
			if ( walk < 0 ) return;
			f ( walk );
		}
	}

	bit_array_var_impl::bit_array_var_impl( const bit_array & source, size_t sourceCount) {
		for(size_t w = source.find_first( true, 0, sourceCount); w < sourceCount; w = source.find_next( true, w, sourceCount ) ) {
			set(w, true);
		}
	}

	bool bit_array_var_impl::get(t_size n) const {
		return m_data.have_item(n);
	}
	t_size bit_array_var_impl::find(bool val,t_size start,t_ssize count) const {
		if (!val) {
			return bit_array::find(false, start, count); //optimizeme.
		} else if (count > 0) {
			const t_size * v = m_data.find_nearest_item<true, true>(start);
			if (v == NULL || *v > start+count) return start + count;
			return *v;
		} else if (count < 0) {
			const t_size * v = m_data.find_nearest_item<true, false>(start);
			if (v == NULL || *v < start+count) return start + count;
			return *v;
		} else return start;
	}

	void bit_array_var_impl::set(t_size n,bool val) {
		if (val) m_data += n;
		else m_data -= n;
	}

		
	bit_array_flatIndexList::bit_array_flatIndexList() {
		m_content.prealloc( 1024 ); 
	}

	void bit_array_flatIndexList::add( size_t n ) {
		m_content.append_single_val( n );
	}

	bool bit_array_flatIndexList::get(t_size n) const {
		size_t dummy;
		return _find( n, dummy );
	}
	t_size bit_array_flatIndexList::find(bool val,t_size start,t_ssize count) const {
		if (val == false) {
			// unoptimized but not really used
			return bit_array::find(val, start, count);
		}
			
		if (count==0) return start;
		else if (count<0) {
			size_t idx;
			if (!_findNearestDown( start, idx ) || m_content[idx] < start+count) return start + count;
			return m_content[idx];
		} else { // count > 0
			size_t idx;
			if (!_findNearestUp( start, idx ) || m_content[idx] > start+count) return start + count;
			return m_content[idx];
		}
	}

	bool bit_array_flatIndexList::_findNearestUp( size_t val, size_t & outIdx ) const {
		size_t idx;
		if (_find( val, idx )) { outIdx = idx; return true; }
		// we have a valid outIdx at where the bsearch gave up
		PFC_ASSERT ( idx == 0 || m_content [ idx - 1 ] < val );
		PFC_ASSERT ( idx == m_content.get_size() || m_content[ idx ] > val );
		if (idx == m_content.get_size()) return false;
		outIdx = idx;
		return true;
	}
	bool bit_array_flatIndexList::_findNearestDown( size_t val, size_t & outIdx ) const {
		size_t idx;
		if (_find( val, idx )) { outIdx = idx; return true; }
		// we have a valid outIdx at where the bsearch gave up
		PFC_ASSERT ( idx == 0 || m_content [ idx - 1 ] < val );
		PFC_ASSERT ( idx == m_content.get_size() || m_content[ idx ] > val );
		if (idx == 0) return false;
		outIdx = idx - 1;
		return true;
	}

	void bit_array_flatIndexList::presort() {
		pfc::sort_t( m_content, pfc::compare_t< size_t, size_t >, m_content.get_size( ) );
	}


	bit_array_bittable::bit_array_bittable(const pfc::bit_array & in, size_t inSize) : m_count() {
		resize(inSize);
		for (size_t w = in.find_first(true, 0, inSize); w < inSize; w = in.find_next(true, w, inSize)) {
			set(w, true);
		}
	}

	void bit_array_bittable::resize(t_size p_count)
	{
		t_size old_bytes = g_estimate_size(m_count);
		m_count = p_count;
		t_size bytes = g_estimate_size(m_count);
		m_data.set_size(bytes);
		if (bytes > old_bytes) pfc::memset_null_t(m_data.get_ptr() + old_bytes, bytes - old_bytes);
	}

	void bit_array_bittable::set(t_size n, bool val)
	{
		if (n<m_count)
		{
			g_set(m_data, n, val);
		}
	}

	bool bit_array_bittable::get(t_size n) const
	{
		bool rv = false;
		if (n<m_count) {
			rv = g_get(m_data, n);
		}
		return rv;
	}

	t_size bit_array_one::find(bool p_val, t_size start, t_ssize count) const
	{
		if (count == 0) return start;
		else if (p_val)
		{
			if (count>0)
				return (val >= start && (t_ssize)val < (t_ssize)start + count) ? val : start + count;
			else
				return (val <= start && (t_ssize)val > (t_ssize)start + count) ? val : start + count;
		}
		else
		{
			if (start == val) return count>0 ? start + 1 : start - 1;
			else return start;
		}
	}
} // namespace pfc
