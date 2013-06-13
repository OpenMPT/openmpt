/*
 * libopenmpt_settings_xml.cpp
 * ---------------------------
 * Purpose: libopenmpt plugin settings xml i/o
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "libopenmpt_internal.h"

#define LIBOPENMPT_BUILD_SETTINGS_DLL
#include "libopenmpt_settings.h"

#include <sstream>

#include "pugixml.hpp"

namespace openmpt {

void settings::import_xml( const std::string & xml ) {
	pugi::xml_document doc;
	doc.load( xml.c_str() );
	pugi::xml_node settings_node = doc.child( "settings" );
	std::map<std::string,int> map;
	for ( pugi::xml_attribute_iterator it = settings_node.attributes_begin(); it != settings_node.attributes_end(); ++it ) {
		map[ it->name() ] = it->as_int();
	}
	load_map( map );
}

std::string settings::export_xml() const {
	const std::map<std::string,int> map = save_map();
	pugi::xml_document doc;
	pugi::xml_node settings_node = doc.append_child( "settings" );
	for ( std::map<std::string,int>::const_iterator it = map.begin(); it != map.end(); ++it ) {
		settings_node.append_attribute( it->first.c_str() ).set_value( it->second );
	}
	std::ostringstream buf;
	doc.save( buf );
	return buf.str();
}

} // namespace openmpt
