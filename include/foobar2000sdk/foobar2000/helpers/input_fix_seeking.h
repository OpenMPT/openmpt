#pragma once

#include "duration_counter.h"

// Wrapper to implement input_flag_allow_inaccurate_seeking semantics with decoders that do not implement seeking properly.
// If input_flag_allow_inaccurate_seeking is not specified, brute force seeking is used.
template<typename base_t>
class input_fix_seeking : public base_t {
public:
	input_fix_seeking() : m_active() {}

	void decode_initialize(unsigned p_flags,abort_callback & p_abort) {
		base_t::decode_initialize( p_flags, p_abort );
		m_active = ( p_flags & input_flag_allow_inaccurate_seeking ) == 0;
		m_position = 0;
		m_decodeFrom = 0;
	}

	void decode_initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort) {
		base_t::decode_initialize( p_subsong, p_flags, p_abort );
		m_active = ( p_flags & input_flag_allow_inaccurate_seeking ) == 0;
		m_position = 0;
	}

	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {
		if ( ! m_active ) {
			return base_t::decode_run ( p_chunk, p_abort );
		}
		for ( ;; ) {
			p_abort.check();
			if (! base_t::decode_run( p_chunk, p_abort ) ) return false;

			double p = m_position.query();
			m_position += p_chunk;
			if ( m_decodeFrom > p ) {
				auto skip = audio_math::time_to_samples( m_decodeFrom - p, p_chunk.get_sample_rate() );
				if ( skip >= p_chunk.get_sample_count() ) continue;
				if ( skip > 0 ) {
					p_chunk.skip_first_samples( (size_t) skip );
				}
			}
			return true;
		}
	}

	void decode_seek(double p_seconds,abort_callback & p_abort) {
		if ( m_active ) {
			if ( ! this->decode_can_seek() ) throw exception_io_object_not_seekable();
			m_decodeFrom = p_seconds;
			if ( m_decodeFrom < m_position.query() ) {
				base_t::decode_seek(0, p_abort); m_position.reset();
			}
		} else {
			base_t::decode_seek(p_seconds, p_abort);
		}
	}

	bool decode_run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) {
		throw pfc::exception_not_implemented(); // shitformats that need this class are not likely to care
	}
private:
	bool m_active;
	duration_counter m_position;
	double m_decodeFrom;
};
