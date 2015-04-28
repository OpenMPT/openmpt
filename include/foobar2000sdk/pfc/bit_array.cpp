#include "pfc.h"

namespace pfc {
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

}