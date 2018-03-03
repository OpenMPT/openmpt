#pragma once
#include "../SDK/packet_decoder.h"

/*
Helper code with common AAC packet_decoder functionality. Primarily meant for foo_input_std-internal use.
*/

class packet_decoder_aac_common : public packet_decoder {
public:
    static bool parseDecoderSetup( const GUID & p_owner,t_size p_param1,const void * p_param2,t_size p_param2size, const void * &outCodecPrivate, size_t & outCodecPrivateSize );
    static bool testDecoderSetup( const GUID & p_owner, t_size p_param1, const void * p_param2, t_size p_param2size );

	static unsigned get_max_frame_dependency_()
	{
		return 2;
	}
	static double get_max_frame_dependency_time_()
	{
		return 1024.0 / 8000.0;
	}
    
    static void make_ESDS( pfc::array_t<uint8_t> & outESDS, const void * inCodecPrivate, size_t inCodecPrivateSize );
};
