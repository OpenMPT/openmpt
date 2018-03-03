#pragma once

#ifdef FOOBAR2000_HAVE_DSP
//! \since 1.1
//! This service is essentially a special workaround to easily decode DTS/HDCD content stored in files pretending to contain plain PCM data. \n
//! Callers: Instead of calling this directly, you probably want to use input_postprocessed template. \n
//! Implementers: This service is called only by specific decoders, not by all of them! Implementing your own to provide additional functionality is not recommended!
class decode_postprocessor_instance : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(decode_postprocessor_instance, service_base);
public:
	enum {
		//! End of stream. Flush any buffered data during this call.
		flag_eof = 1 << 0,
		//! Stream has already been altered by another instance.
		flag_altered = 1 << 1,
	};
	//! @returns True if the chunk list has been altered by the call, false if not - to tell possible other running instances whether the stream has already been altered or not.
	virtual bool run(dsp_chunk_list & p_chunk_list,t_uint32 p_flags,abort_callback & p_abort) = 0;
	virtual bool get_dynamic_info(file_info & p_out) = 0;
	virtual void flush() = 0;
	virtual double get_buffer_ahead() = 0;
};

//! \since 1.1
//! Entrypoint class for instantiating decode_postprocessor_instance. See decode_postprocessor_instance documentation for more information. \n
//! Instead of calling this directly, you probably want to use input_postprocessed template.
class decode_postprocessor_entry : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(decode_postprocessor_entry)
public:
	virtual bool instantiate(const file_info & info, decode_postprocessor_instance::ptr & out) = 0;
};


//! Helper class for managing decode_postprocessor_instance objects. See also: input_postprocessed.
class decode_postprocessor {
public:
	typedef decode_postprocessor_instance::ptr item;
	void initialize(const file_info & info) {
		m_items.remove_all();
		service_enum_t<decode_postprocessor_entry> e;
		decode_postprocessor_entry::ptr ptr;
		while(e.next(ptr)) {
			item i;
			if (ptr->instantiate(info, i)) m_items += i;
		}
	}
	void run(dsp_chunk_list & p_chunk_list,bool p_eof,abort_callback & p_abort) {
		t_uint32 flags = p_eof ? decode_postprocessor_instance::flag_eof : 0;
		for(t_size walk = 0; walk < m_items.get_size(); ++walk) {
			if (m_items[walk]->run(p_chunk_list, flags, p_abort)) flags |= decode_postprocessor_instance::flag_altered;
		}
	}
	void flush() {
		for(t_size walk = 0; walk < m_items.get_size(); ++walk) {
			m_items[walk]->flush();
		}
	}
	static bool should_bother() { 
		return service_factory_base::is_service_present(decode_postprocessor_entry::class_guid);
	}
	bool is_active() const {
		return m_items.get_size() > 0;
	}
	bool get_dynamic_info(file_info & p_out) {
		bool rv = false;
		for(t_size walk = 0; walk < m_items.get_size(); ++walk) {
			if (m_items[walk]->get_dynamic_info(p_out)) rv = true;
		}
		return rv;
	}
	void close() {
		m_items.remove_all();
	}
	double get_buffer_ahead() {
		double acc = 0;
		for(t_size walk = 0; walk < m_items.get_size(); ++walk) {
			pfc::max_acc(acc, m_items[walk]->get_buffer_ahead());
		}
		return acc;
	}
private:
	pfc::list_t<item> m_items;
};

//! Generic template to add decode_postprocessor support to your input class. Works with both single-track and multi-track inputs.
template<typename baseclass> class input_postprocessed : public baseclass {
public:
	void decode_initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort) {
		m_chunks.remove_all();
		m_haveEOF = false;
		m_toSkip = 0;
		m_postproc.close();
		if ((p_flags & input_flag_no_postproc) == 0 && m_postproc.should_bother()) {
			file_info_impl info;
			this->get_info(p_subsong, info, p_abort);
			m_postproc.initialize(info);
		}
		baseclass::decode_initialize(p_subsong, p_flags, p_abort);
	}
	void decode_initialize(unsigned p_flags,abort_callback & p_abort) {
		m_chunks.remove_all();
		m_haveEOF = false;
		m_toSkip = 0;
		m_postproc.close();
		if ((p_flags & input_flag_no_postproc) == 0 && m_postproc.should_bother()) {
			file_info_impl info;
			this->get_info(info, p_abort);
			m_postproc.initialize(info);
		}
		baseclass::decode_initialize(p_flags, p_abort);
	}
	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {
		if (m_postproc.is_active()) {
			for(;;) {
				p_abort.check();
				if (m_chunks.get_count() > 0) {
					audio_chunk * c = m_chunks.get_item(0);
					if (m_toSkip > 0) {
						if (!c->process_skip(m_toSkip)) {
							m_chunks.remove_by_idx(0);
							continue;
						}
					}
					p_chunk = *c;
					m_chunks.remove_by_idx(0);
					return true;
				}
				if (m_haveEOF) return false;
				if (!baseclass::decode_run(*m_chunks.insert_item(0), p_abort)) {
					m_haveEOF = true;
					m_chunks.remove_by_idx(0);
				}
				m_postproc.run(m_chunks, m_haveEOF, p_abort);
			}
		} else {
			return baseclass::decode_run(p_chunk, p_abort);
		}
	}
	bool decode_run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) {
		if (m_postproc.is_active()) {
			throw pfc::exception_not_implemented();
		} else {
			return baseclass::decode_run_raw(p_chunk, p_raw, p_abort);
		}
	}
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {
		bool rv = baseclass::decode_get_dynamic_info(p_out, p_timestamp_delta);
		if (m_postproc.get_dynamic_info(p_out)) rv = true;
		return rv;
	}
	void decode_seek(double p_seconds,abort_callback & p_abort) {
		m_chunks.remove_all();
		m_haveEOF = false;
		m_postproc.flush();
		double target = pfc::max_t<double>(0, p_seconds - m_postproc.get_buffer_ahead());
		m_toSkip = p_seconds - target;
		baseclass::decode_seek(target, p_abort);
	}
private:
	dsp_chunk_list_impl m_chunks;
	bool m_haveEOF;
	double m_toSkip;
	decode_postprocessor m_postproc;
};

#else // FOOBAR2000_HAVE_DSP

template<typename baseclass> class input_postprocessed : public baseclass {};

#endif // FOOBAR2000_HAVE_DSP
