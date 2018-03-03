#include "stdafx.h"

#include "packet_decoder_aac_common.h"

#include "../SDK/filesystem_helper.h"


bool packet_decoder_aac_common::parseDecoderSetup(const GUID &p_owner, t_size p_param1, const void *p_param2, t_size p_param2size, const void *&outCodecPrivate, size_t &outCodecPrivateSize) {
    outCodecPrivate = NULL;
    outCodecPrivateSize = 0;
    
    if ( p_owner == owner_ADTS ) { return true; }
    else if ( p_owner == owner_ADIF ) { return true; }
    else if ( p_owner == owner_MP4 )
    {
        if ( p_param1 == 0x40 || p_param1 == 0x66 || p_param1 == 0x67 || p_param1 == 0x68 ) {
            outCodecPrivate = p_param2; outCodecPrivateSize = p_param2size;
            return true;
        }
    }
    else if ( p_owner == owner_matroska )
    {
        const matroska_setup * setup = ( const matroska_setup * ) p_param2;
        if ( p_param2size == sizeof(*setup) )
        {
            if ( !strcmp(setup->codec_id, "A_AAC") || !strncmp(setup->codec_id, "A_AAC/", 6) ) {
                outCodecPrivate = (const uint8_t *) setup->codec_private;
                outCodecPrivateSize = setup->codec_private_size;
                return true;
            }
        }
    }
    return false;
    
}

bool packet_decoder_aac_common::testDecoderSetup( const GUID & p_owner, t_size p_param1, const void * p_param2, t_size p_param2size ) {
    const void * dummy1; size_t dummy2;
    return parseDecoderSetup(p_owner, p_param1, p_param2, p_param2size, dummy1, dummy2);
}





namespace {
    class esds_maker : public stream_writer_buffer_simple {
    public:
        void write_esds_obj( uint8_t code, const void * data, size_t size, abort_callback & aborter ) {
            if ( size >= ( 1 << 28 ) ) throw pfc::exception_overflow();
            write_byte(code, aborter);
            for ( int i = 3; i >= 0; -- i ) {
                uint8_t c = (uint8_t)( 0x7F & ( size >> 7 * i ) );
                if ( i > 0 ) c |= 0x80;
                write_byte(c, aborter);
            }
            write( data, size, aborter );
        }
        void write_esds_obj( uint8_t code, esds_maker const & other, abort_callback & aborter ) {
            write_esds_obj( code, other.m_buffer.get_ptr(), other.m_buffer.get_size(), aborter );
        }
        void write_byte( uint8_t byte , abort_callback & aborter ) {
            write( &byte, 1, aborter );
        }
    };
}

void packet_decoder_aac_common::make_ESDS( pfc::array_t<uint8_t> & outESDS, const void * inCodecPrivate, size_t inCodecPrivateSize ) {
    if (inCodecPrivateSize > 1024*1024) throw exception_io_data(); // sanity
    abort_callback_dummy p_abort;
    
    esds_maker esds4;
    
    const uint8_t crap[] = {0x40, 0x15, 0x00, 0x00, 0x00, 0x00, 0x05, 0x34, 0x08, 0x00, 0x02, 0x3D, 0x55};
    esds4.write( crap, sizeof(crap), p_abort );
    
    {
        esds_maker esds5;
        esds5.write( inCodecPrivate, inCodecPrivateSize, p_abort );
        esds4.write_esds_obj(5, esds5, p_abort);
    }
    
    
    esds_maker esds3;
    
    esds3.write_byte( 0, p_abort );
    esds3.write_byte( 1, p_abort );
    esds3.write_byte( 0, p_abort );
    esds3.write_esds_obj(4, esds4, p_abort);
    
    // esds6 after esds4, but doesn't seem that important
    
    esds_maker final;
    final.write_esds_obj(3, esds3, p_abort);
    outESDS.set_data_fromptr( final.m_buffer.get_ptr(), final.m_buffer.get_size() );
    
    /*
     static const uint8_t esdsTemplate[] = {
     0x03, 0x80, 0x80, 0x80, 0x25, 0x00, 0x01, 0x00, 0x04, 0x80, 0x80, 0x80, 0x17, 0x40, 0x15, 0x00,
     0x00, 0x00, 0x00, 0x05, 0x34, 0x08, 0x00, 0x02, 0x3D, 0x55, 0x05, 0x80, 0x80, 0x80, 0x05, 0x12,
     0x30, 0x56, 0xE5, 0x00, 0x06, 0x80, 0x80, 0x80, 0x01, 0x02
     };
     */
    
    // ESDS: 03 80 80 80 25 00 01 00 04 80 80 80 17 40 15 00 00 00 00 05 34 08 00 02 3D 55 05 80 80 80 05 12 30 56 E5 00 06 80 80 80 01 02
    // For: 12 30 56 E5 00
    
}
