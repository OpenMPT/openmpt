#pragma once

/*
Helper code with common MP3 packet_decoder functionality. Primarily meant for foo_input_std-internal use.
*/

class packet_decoder_mp3_common : public packet_decoder {
public:
    static unsigned parseDecoderSetup( const GUID & p_owner,t_size p_param1,const void * p_param2,t_size p_param2size );
	
	static unsigned layer_from_frame(const void *, size_t);

	static unsigned get_max_frame_dependency_() {return 10;}
	static double get_max_frame_dependency_time_() {return 10.0 * 1152.0 / 32000.0;}
};


template<typename impl_t>
class packet_decoder_mp3 : public packet_decoder
{
	impl_t m_decoder;
	unsigned m_layer;

	void init(unsigned p_layer) {
		m_layer = p_layer;
		m_decoder.reset_after_seek();
	}

	bool m_MP4fixes;

public:
	packet_decoder_mp3() : m_layer(0), m_MP4fixes()
	{
	}

	~packet_decoder_mp3()
	{
	}

	t_size set_stream_property(const GUID & p_type, t_size p_param1, const void * p_param2, t_size p_param2size) {
		return m_decoder.set_stream_property(p_type, p_param1, p_param2, p_param2size);
	}

	void get_info(file_info & p_info)
	{
		switch (m_layer)
		{ 
		case 1:	p_info.info_set("codec", "MP1"); break;
		case 2:	p_info.info_set("codec", "MP2"); break;
		case 3:	p_info.info_set("codec", "MP3"); break;
		default:
			throw exception_io_data();
		}
		p_info.info_set("encoding", "lossy");
	}

	unsigned get_max_frame_dependency() { return 10; }
	double get_max_frame_dependency_time() { return 10.0 * 1152.0 / 32000.0; }

	static bool g_is_our_setup(const GUID & p_owner, t_size p_param1, const void * p_param2, t_size p_param2size) {
		return packet_decoder_mp3_common::parseDecoderSetup(p_owner, p_param1, p_param2, p_param2size) != 0;
	}

	void open(const GUID & p_owner, bool p_decode, t_size p_param1, const void * p_param2, t_size p_param2size, abort_callback & p_abort)
	{
		m_MP4fixes = (p_owner == owner_MP4);
		unsigned layer = packet_decoder_mp3_common::parseDecoderSetup(p_owner, p_param1, p_param2, p_param2size);
		if (layer == 0) throw exception_io_data();
		init(layer);
	}

	void reset_after_seek() {
		m_decoder.reset_after_seek();
	}

	void decode(const void * p_buffer, t_size p_bytes, audio_chunk & p_chunk, abort_callback & p_abort) {
		m_decoder.decode(p_buffer, p_bytes, p_chunk, p_abort);
	}

	bool analyze_first_frame_supported() { return m_MP4fixes; }
	void analyze_first_frame(const void * p_buffer, t_size p_bytes, abort_callback & p_abort) {
		m_layer = packet_decoder_mp3_common::layer_from_frame(p_buffer, p_bytes);
	}
};
