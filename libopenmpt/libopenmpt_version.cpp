/*
 * libopenmpt_version.cpp
 * ----------------------
 * Purpose: libopenmpt versioning helpers
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "common/stdafx.h"

#include "libopenmpt_internal.h"
#include "libopenmpt.hpp"
#include "libopenmpt.h"

#include <sstream>

#include "common/version.h"

#include "svn_version.h"

namespace openmpt { namespace version {

std::uint32_t get_library_version() {
	return OPENMPT_API_VERSION | ( MptVersion::GetRevision() & 0xffff );
}

std::uint32_t get_core_version() {
	return MptVersion::num;
}

static std::string get_library_version_string() {
	std::ostringstream str;
	std::uint32_t version = get_library_version();
	if ( ( version & 0xffff ) == 0 ) {
		str << ((version>>24) & 0xff) << "." << ((version>>16) & 0xff);
	} else {
		str << ((version>>24) & 0xff) << "." << ((version>>16) & 0xff) << "." << ((version>>0) & 0xffff);
	}
	if ( MptVersion::IsDirty() ) {
		str << ".2-modified";
	} else if ( MptVersion::HasMixedRevisions() ) {
		str << ".1-modified";
	}
	return str.str();
}

static std::string get_core_version_string() {
	return MptVersion::GetVersionStringExtended();
}

static std::string get_build_string() {
	return MptVersion::GetBuildDateString();
}

static std::string get_credits_string() {
	return mpt::String::Convert(MptVersion::GetFullCreditsString(), mpt::CharsetISO8859_1, mpt::CharsetUTF8);
}

static std::string get_contact_string() {
	return mpt::String::Convert(MptVersion::GetContactString(), mpt::CharsetISO8859_1, mpt::CharsetUTF8);
}

std::string get_string( const std::string & key ) {
	if ( key == "" ) {
		return std::string();
	} else if ( key == string::library_version ) {
		return get_library_version_string();
	} else if ( key == string::core_version ) {
		return get_core_version_string();
	} else if ( key == string::build ) {
		return get_build_string();
	} else if ( key == string::credits ) {
		return get_credits_string();
	} else if ( key == string::contact ) {
		return get_contact_string();
	} else {
		return std::string();
	}
}

int get_version_compatbility( std::uint32_t api_version ) {
	if ( ( api_version & 0xff000000 ) != ( openmpt::get_library_version() & 0xff000000 ) ) {
		return 0;
	}
	if ( api_version > openmpt::get_library_version() ) {
		return 1;
	}
	return 2;
}

#define OPENMPT_API_VERSION_STRING STRINGIZE(OPENMPT_API_VERSION_MAJOR)"."STRINGIZE(OPENMPT_API_VERSION_MINOR)

#ifndef NO_WINAMP
char * in_openmpt_string = "in_openmpt " OPENMPT_API_VERSION_STRING "." STRINGIZE(OPENMPT_VERSION_REVISION);
#endif // NO_WINAMP

#ifndef NO_XMPLAY
const char * xmp_openmpt_string = "OpenMPT (" OPENMPT_API_VERSION_STRING "." STRINGIZE(OPENMPT_VERSION_REVISION) ")";
#endif // NO_XMPLAY

} // namespace version

namespace detail {

// has to be exported for type_info lookup to work
class LIBOPENMPT_CXX_API version_mismatch : public openmpt::exception {
public:
	version_mismatch() : openmpt::exception("API and header version mismatch") { }
}; // class version_mismatch

void version_compatible_or_throw( std::int32_t api_version ) {
	if ( version::get_version_compatbility( api_version ) <  2 ) {
		throw version_mismatch();
	}
}

} // namespace detail

} // namespace openmpt
