#pragma once

//! Duration counter class - accumulates duration using sample values, without any kind of rounding error accumulation.
class duration_counter {
public:
	duration_counter() : m_offset() {
	}
	void set(double v) {
		m_sampleCounts.remove_all();
		m_offset = v;
	}
	void reset() {
		set(0);
	}

	void add(double v) { m_offset += v; }
	void subtract(double v) { m_offset -= v; }

	double query() const {
		double acc = m_offset;
		for (t_map::const_iterator walk = m_sampleCounts.first(); walk.is_valid(); ++walk) {
			acc += audio_math::samples_to_time(walk->m_value, walk->m_key);
		}
		return acc;
	}

	uint64_t queryAsSampleCount(uint32_t rate) const {
		uint64_t samples = 0;
		double acc = m_offset;
		for (t_map::const_iterator walk = m_sampleCounts.first(); walk.is_valid(); ++walk) {
			if (walk->m_key == rate) samples += walk->m_value;
			else acc += audio_math::samples_to_time(walk->m_value, walk->m_key);
		}
		return samples + audio_math::time_to_samples(acc, rate);
	}

	void add(const audio_chunk & c) {
		add(c.get_sample_count(), c.get_sample_rate());
	}
	void add(t_uint64 sampleCount, t_uint32 sampleRate) {
		PFC_ASSERT(sampleRate > 0);
		if (sampleRate > 0 && sampleCount > 0) {
			m_sampleCounts.find_or_add(sampleRate) += sampleCount;
		}
	}
	void add(const duration_counter & other) {
		add(other.m_offset);
		for (t_map::const_iterator walk = other.m_sampleCounts.first(); walk.is_valid(); ++walk) {
			add(walk->m_value, walk->m_key);
		}
	}
	void subtract(const duration_counter & other) {
		subtract(other.m_offset);
		for (t_map::const_iterator walk = other.m_sampleCounts.first(); walk.is_valid(); ++walk) {
			subtract(walk->m_value, walk->m_key);
		}
	}
	void subtract(t_uint64 sampleCount, t_uint32 sampleRate) {
		PFC_ASSERT(sampleRate > 0);
		if (sampleRate > 0 && sampleCount > 0) {
			t_uint64 * val = m_sampleCounts.query_ptr(sampleRate);
			if (val == NULL) throw pfc::exception_invalid_params();
			if (*val < sampleCount) throw pfc::exception_invalid_params();
			else if (*val == sampleCount) {
				m_sampleCounts.remove(sampleRate);
			} else {
				*val -= sampleCount;
			}

		}
	}
	void subtract(const audio_chunk & c) {
		subtract(c.get_sample_count(), c.get_sample_rate());
	}
	template<typename t_source> duration_counter & operator+=(const t_source & source) { add(source); return *this; }
	template<typename t_source> duration_counter & operator-=(const t_source & source) { subtract(source); return *this; }
	template<typename t_source> duration_counter & operator=(const t_source & source) { reset(); add(source); return *this; }
private:
	double m_offset;
	typedef pfc::map_t<t_uint32, t_uint64> t_map;
	t_map m_sampleCounts;
};

