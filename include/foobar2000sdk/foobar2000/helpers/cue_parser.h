#pragma once

#include "cue_creator.h"

//HINT: for info on how to generate an embedded cuesheet enabled input, see the end of this header.



namespace file_info_record_helper {

	class file_info_record {
	public:
		typedef pfc::chain_list_v2_t<pfc::string8> t_meta_value;
		typedef pfc::map_t<pfc::string8,t_meta_value,file_info::field_name_comparator> t_meta_map;
		typedef pfc::map_t<pfc::string8,pfc::string8,file_info::field_name_comparator> t_info_map;

		file_info_record() : m_replaygain(replaygain_info_invalid), m_length(0) {}

		replaygain_info get_replaygain() const {return m_replaygain;}
		void set_replaygain(const replaygain_info & p_replaygain) {m_replaygain = p_replaygain;}
		double get_length() const {return m_length;}
		void set_length(double p_length) {m_length = p_length;}

		void reset();
		void from_info_overwrite_info(const file_info & p_info);
		void from_info_overwrite_meta(const file_info & p_info);
		void from_info_overwrite_rg(const file_info & p_info);

		template<typename t_source>
		void overwrite_meta(const t_source & p_meta) {
			m_meta.overwrite(p_meta);
		}
		template<typename t_source>
		void overwrite_info(const t_source & p_info) {
			m_info.overwrite(p_info);
		}

		void merge_overwrite(const file_info & p_info);
		void transfer_meta_entry(const char * p_name,const file_info & p_info,t_size p_index);

		void meta_set(const char * p_name,const char * p_value);
		const t_meta_value * meta_query_ptr(const char * p_name) const;

		void from_info_set_meta(const file_info & p_info);

		void from_info(const file_info & p_info);
		void to_info(file_info & p_info) const;

		template<typename t_callback> void enumerate_meta(t_callback & p_callback) const {m_meta.enumerate(p_callback);}
		template<typename t_callback> void enumerate_meta(t_callback & p_callback) {m_meta.enumerate(p_callback);}

	//private:
		t_meta_map m_meta;
		t_info_map m_info;
		replaygain_info m_replaygain;
		double m_length;
	};

}//namespace file_info_record_helper


namespace cue_parser
{
	struct cue_entry {
		pfc::string8 m_file;
		unsigned m_track_number;
		t_cuesheet_index_list m_indexes;
	};

	typedef pfc::chain_list_v2_t<cue_entry> t_cue_entry_list;


	PFC_DECLARE_EXCEPTION(exception_bad_cuesheet,exception_io_data,"Invalid cuesheet");

	//! Throws exception_bad_cuesheet on failure.
	void parse(const char *p_cuesheet,t_cue_entry_list & p_out);
	//! Throws exception_bad_cuesheet on failure.
	void parse_info(const char *p_cuesheet,file_info & p_info,unsigned p_index);
	//! Throws exception_bad_cuesheet on failure.
	void parse_full(const char * p_cuesheet,cue_creator::t_entry_list & p_out);

	

	struct track_record {
		file_info_record_helper::file_info_record m_info;
		pfc::string8 m_file,m_flags;
		t_cuesheet_index_list m_index_list;
	};

	typedef pfc::map_t<unsigned,track_record> track_record_list;

	class embeddedcue_metadata_manager {
	public:
		void get_tag(file_info & p_info) const;
		void set_tag(file_info const & p_info);

		void get_track_info(unsigned p_track,file_info & p_info) const;
		void set_track_info(unsigned p_track,file_info const & p_info);
		void query_track_offsets(unsigned p_track,double & p_begin,double & p_length) const;
		bool have_cuesheet() const;
		unsigned remap_trackno(unsigned p_index) const;
		t_size get_cue_track_count() const;
	private:
		track_record_list m_content;
	};





	template<typename t_base>
	class input_wrapper_cue_t : public input_forward_static_methods<t_base> {
	public:
		typedef input_info_writer_v2 interface_info_writer_t; // remove_tags supplied
		void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
			m_impl.open( p_filehint, p_path, p_reason, p_abort );
			file_info_impl info;
			m_impl.get_info(info,p_abort);
			m_meta.set_tag(info);
		}

		t_uint32 get_subsong_count() {
			return m_meta.have_cuesheet() ? (uint32_t) m_meta.get_cue_track_count() : 1;
		}

		t_uint32 get_subsong(t_uint32 p_index) {
			return m_meta.have_cuesheet() ? m_meta.remap_trackno(p_index) : 0;
		}

		void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) {
			if (p_subsong == 0) {
				m_meta.get_tag(p_info);
			} else {
				m_meta.get_track_info(p_subsong,p_info);
			}
		}

		t_filestats get_file_stats(abort_callback & p_abort) {return m_impl.get_file_stats(p_abort);}

		void decode_initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort) {
			if (p_subsong == 0) {
				m_impl.decode_initialize(p_flags, p_abort);
				m_decodeFrom = 0; m_decodeLength = -1; m_decodePos = 0;
			} else {
				double start, length;
				m_meta.query_track_offsets(p_subsong,start,length);
				unsigned flags2 = p_flags;
				if (start > 0) flags2 &= ~input_flag_no_seeking;
				flags2 &= ~input_flag_allow_inaccurate_seeking;
				m_impl.decode_initialize(flags2, p_abort);
				m_impl.decode_seek(start, p_abort);
				m_decodeFrom = start; m_decodeLength = length; m_decodePos = 0;
			}
		}

		bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {
			return _run(p_chunk, NULL, p_abort);
		}
		
		void decode_seek(double p_seconds,abort_callback & p_abort) {
			if (this->m_decodeLength >= 0 && p_seconds > m_decodeLength) p_seconds = m_decodeLength;
			m_impl.decode_seek(m_decodeFrom + p_seconds,p_abort);
			m_decodePos = p_seconds;
		}
		
		bool decode_can_seek() {return m_impl.decode_can_seek();}
		
		bool decode_run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort) {
			return _run(p_chunk, &p_raw, p_abort);
		}
		void set_logger(event_logger::ptr ptr) {
			m_impl.set_logger(ptr);
		}
		bool flush_on_pause() {
			return m_impl.flush_on_pause();
		}
		void set_pause(bool) {} // obsolete
		size_t extended_param(const GUID & type, size_t arg1, void * arg2, size_t arg2size) {
			return m_impl.extended_param(type, arg1, arg2, arg2size);
		}

		bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) {
			return m_impl.decode_get_dynamic_info(p_out, p_timestamp_delta);
		}

		bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) {
			return m_impl.decode_get_dynamic_info_track(p_out, p_timestamp_delta);
		}

		void decode_on_idle(abort_callback & p_abort) {
			m_impl.decode_on_idle(p_abort);
		}

		void remove_tags(abort_callback & abort) {
			m_impl.remove_tags( abort );
			file_info_impl info;
			m_impl.get_info(info, abort);
			m_meta.set_tag( info );
		}

		void retag_set_info(t_uint32 p_subsong,const file_info & p_info,abort_callback & p_abort) {
			if (p_subsong == 0) {
				m_meta.set_tag(p_info);
			} else {
				m_meta.set_track_info(p_subsong,p_info);
			}
		}

		void retag_commit(abort_callback & p_abort) {
			file_info_impl info;
			m_meta.get_tag(info);
			m_impl.retag(pfc::implicit_cast<const file_info&>(info), p_abort);
			info.reset();
			m_impl.get_info(info, p_abort);
			m_meta.set_tag( info );
		}
	private:
		bool _run(audio_chunk & chunk, mem_block_container * raw, abort_callback & aborter) {
			if (m_decodeLength >= 0 && m_decodePos >= m_decodeLength) return false;
			if (raw == NULL) {
				if (!m_impl.decode_run(chunk, aborter)) return false;
			} else {
				if (!m_impl.decode_run_raw(chunk, *raw, aborter)) return false;
			}

			if (m_decodeLength >= 0) {
				const uint64_t remaining = audio_math::time_to_samples( m_decodeLength - m_decodePos, chunk.get_sample_rate() );
				const size_t samplesGot = chunk.get_sample_count();
				if (remaining < samplesGot) {
					m_decodePos = m_decodeLength;
					if (remaining == 0) { // rare but possible as a result of rounding SNAFU - we're EOF but we didn't notice earlier
						return false;
					}
					
					chunk.set_sample_count( (size_t) remaining );
					if (raw != NULL) {
						const t_size rawSize = raw->get_size();
						PFC_ASSERT( rawSize % samplesGot == 0 );
						raw->set_size( (t_size) ( (t_uint64) rawSize * remaining / samplesGot ) );
					}
				} else {
					m_decodePos += chunk.get_duration();
				}
			} else {
				m_decodePos += chunk.get_duration();
			}
			return true;
		}
		t_base m_impl;
		double m_decodeFrom, m_decodeLength, m_decodePos;

		embeddedcue_metadata_manager m_meta;
	};
#ifdef FOOBAR2000_HAVE_CHAPTERIZER
	template<typename I>
	class chapterizer_impl_t : public chapterizer
	{
	public:
		bool is_our_path(const char * p_path) {
			return I::g_is_our_path(p_path, pfc::string_extension(p_path));
		}

		void set_chapters(const char * p_path,chapter_list const & p_list,abort_callback & p_abort) {
			input_wrapper_cue_t<I> instance;
			instance.open(0,p_path,input_open_info_write,p_abort);

			//stamp the cuesheet first
			{
				file_info_impl info;
				instance.get_info(0,info,p_abort);

				pfc::string_formatter cuesheet;
							
				{
					cue_creator::t_entry_list entries;
					t_size n, m = p_list.get_chapter_count();
					const double pregap = p_list.get_pregap();
					double offset_acc = pregap;
					for(n=0;n<m;n++)
					{
						cue_creator::t_entry_list::iterator entry;
						entry = entries.insert_last();
						entry->m_infos = p_list.get_info(n);
						entry->m_file = "CDImage.wav";
						entry->m_track_number = (unsigned)(n+1);
						entry->m_index_list.from_infos(entry->m_infos,offset_acc);
						if (n == 0) entry->m_index_list.m_positions[0] = 0;
						offset_acc += entry->m_infos.get_length();
					}
					cue_creator::create(cuesheet,entries);
				}

				info.meta_set("cuesheet",cuesheet);

				instance.retag_set_info(0,info,p_abort);
			}
			//stamp per-chapter infos
			for(t_size walk = 0, total = p_list.get_chapter_count(); walk < total; ++walk) {
				instance.retag_set_info( (uint32_t)( walk + 1 ), p_list.get_info(walk),p_abort);
			}

			instance.retag_commit(p_abort);
		}

		void get_chapters(const char * p_path,chapter_list & p_list,abort_callback & p_abort) {

			input_wrapper_cue_t<I> instance;
			instance.open(0,p_path,input_open_info_read,p_abort);
			const t_uint32 total = instance.get_subsong_count();

			p_list.set_chapter_count(total);
			for(t_uint32 walk = 0; walk < total; ++walk) {
				file_info_impl info;
				instance.get_info(instance.get_subsong(walk),info,p_abort);
				p_list.set_info(walk,info);
			}
		}

		bool supports_pregaps() {
			return true;
		}
	};
#endif
};

//! Wrapper template for generating embedded cuesheet enabled inputs.
//! t_input_impl is a singletrack input implementation (see input_singletrack_impl for method declarations).
//! To declare an embedded cuesheet enabled input, change your input declaration from input_singletrack_factory_t<myinput> to input_cuesheet_factory_t<myinput>.
template<typename t_input_impl, unsigned t_flags = 0>
class input_cuesheet_factory_t {
public:
	input_factory_t<cue_parser::input_wrapper_cue_t<t_input_impl>,t_flags > m_input_factory;
#ifdef FOOBAR2000_HAVE_CHAPTERIZER
	service_factory_single_t<cue_parser::chapterizer_impl_t<t_input_impl> > m_chapterizer_factory;	
#endif
};
