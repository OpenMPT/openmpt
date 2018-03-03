#include "stdafx.h"
#include "packet_decoder_mp3_common.h"
#include "mp3_utils.h"

unsigned packet_decoder_mp3_common::parseDecoderSetup( const GUID & p_owner,t_size p_param1,const void * p_param2,t_size p_param2size )
{
    if (p_owner == owner_MP3) return 3;
    else if (p_owner == owner_MP2) return 2;
    else if (p_owner == owner_MP1) return 1;
    else if (p_owner == owner_MP4)
    {
        switch(p_param1)
        {
			case 0x6B:
				return 3;
				break;
			case 0x69:
				return 3;
				break;
			default:
                return 0;
        }
    }
    else if (p_owner == owner_matroska)
    {
        if (p_param2size==sizeof(matroska_setup))
        {
            const matroska_setup * setup = (const matroska_setup*) p_param2;
            if (!strcmp(setup->codec_id,"A_MPEG/L3")) return 3;
            else if (!strcmp(setup->codec_id,"A_MPEG/L2")) return 2;
            else if (!strcmp(setup->codec_id,"A_MPEG/L1")) return 1;
            else return 0;
        }
        else return 0;
    }
    else return 0;
}

unsigned packet_decoder_mp3_common::layer_from_frame(const void * frame, size_t size) {
	using namespace mp3_utils;
	TMPEGFrameInfo info;
	if (!ParseMPEGFrameHeader(info, frame, size)) throw exception_io_data();
	return info.m_layer;
}