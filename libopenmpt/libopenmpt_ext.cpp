
#include "common/stdafx.h"

#define LIBOPENMPT_EXT_IS_EXPERIMENTAL

#include "libopenmpt_internal.h"
#include "libopenmpt_ext.hpp"
#include "libopenmpt_impl.hpp"

#include <stdexcept>

#include "soundlib/Sndfile.h"

#ifndef NO_LIBOPENMPT_CXX

namespace openmpt {

class module_ext_impl
	: public module_impl
	, public ext::pattern_vis



	/* add stuff here */



{
public:
	module_ext_impl( std::istream & stream, std::ostream & log, const std::map< std::string, std::string > & ctls ) : module_impl( stream, std::make_shared<std_ostream_log>( log ), ctls ) {
		init();
	}
	module_ext_impl( const std::vector<char> & data, std::ostream & log, const std::map< std::string, std::string > & ctls ) : module_impl( data, std::make_shared<std_ostream_log>( log ), ctls ) {
		init();
	}
	module_ext_impl( const char * data, std::size_t size, std::ostream & log, const std::map< std::string, std::string > & ctls ) : module_impl( data, size, std::make_shared<std_ostream_log>( log ), ctls ) {
		init();
	}
	module_ext_impl( const void * data, std::size_t size, std::ostream & log, const std::map< std::string, std::string > & ctls ) : module_impl( data, size, std::make_shared<std_ostream_log>( log ), ctls ) {
		init();
	}

private:



	/* add stuff here */



private:

	void init() {



		/* add stuff here */



	}

public:

	~module_ext_impl() {



		/* add stuff here */



	}

public:

	void * get_interface( const std::string & interface_id ) {
		if ( interface_id.empty() ) {
			return 0;
		} else if ( interface_id == ext::pattern_vis_id ) {
			return dynamic_cast< ext::pattern_vis * >( this );



			/* add stuff here */



		} else {
			return 0;
		}
	}

	// pattern_vis

	virtual effect_type get_pattern_row_channel_volume_effect_type( std::int32_t pattern, std::int32_t row, std::int32_t channel ) const {
		std::uint8_t byte = get_pattern_row_channel_command( pattern, row, channel, module::command_volumeffect );
		switch ( ModCommand::GetVolumeEffectType( byte ) ) {
			case EFFECT_TYPE_NORMAL : return effect_general; break;
			case EFFECT_TYPE_GLOBAL : return effect_global ; break;
			case EFFECT_TYPE_VOLUME : return effect_volume ; break;
			case EFFECT_TYPE_PANNING: return effect_panning; break;
			case EFFECT_TYPE_PITCH  : return effect_pitch  ; break;
			default: return effect_unknown; break;
		}
	}

	virtual effect_type get_pattern_row_channel_effect_type( std::int32_t pattern, std::int32_t row, std::int32_t channel ) const {
		std::uint8_t byte = get_pattern_row_channel_command( pattern, row, channel, module::command_effect );
		switch ( ModCommand::GetEffectType( byte ) ) {
			case EFFECT_TYPE_NORMAL : return effect_general; break;
			case EFFECT_TYPE_GLOBAL : return effect_global ; break;
			case EFFECT_TYPE_VOLUME : return effect_volume ; break;
			case EFFECT_TYPE_PANNING: return effect_panning; break;
			case EFFECT_TYPE_PITCH  : return effect_pitch  ; break;
			default: return effect_unknown; break;
		}
	}



	/* add stuff here */



}; // class module_ext_impl

module_ext::module_ext( std::istream & stream, std::ostream & log, const std::map< std::string, std::string > & ctls ) : ext_impl(0) {
	ext_impl = new module_ext_impl( stream, log, ctls );
	set_impl( ext_impl );
}
module_ext::module_ext( const std::vector<char> & data, std::ostream & log, const std::map< std::string, std::string > & ctls ) : ext_impl(0) {
	ext_impl = new module_ext_impl( data, log, ctls );
	set_impl( ext_impl );
}
module_ext::module_ext( const char * data, std::size_t size, std::ostream & log, const std::map< std::string, std::string > & ctls ) : ext_impl(0) {
	ext_impl = new module_ext_impl( data, size, log, ctls );
	set_impl( ext_impl );
}
module_ext::module_ext( const void * data, std::size_t size, std::ostream & log, const std::map< std::string, std::string > & ctls ) : ext_impl(0) {
	ext_impl = new module_ext_impl( data, size, log, ctls );
	set_impl( ext_impl );
}
module_ext::~module_ext() {
	set_impl( 0 );
	delete ext_impl;
	ext_impl = 0;
}
module_ext::module_ext( const module_ext & other ) : module(other) {
	throw std::runtime_error("openmpt::module_ext is non-copyable");
}
void module_ext::operator = ( const module_ext & ) {
	throw std::runtime_error("openmpt::module_ext is non-copyable");
}

void * module_ext::get_interface( const std::string & interface_id ) {
	return ext_impl->get_interface( interface_id );
}

} // namespace openmpt

#endif // NO_LIBOPENMPT_CXX
