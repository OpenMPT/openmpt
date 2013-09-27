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
	return mpt::String::Convert( MptVersion::GetFullCreditsString(), mpt::CharsetISO8859_1, mpt::CharsetUTF8 );
}

static std::string get_contact_string() {
	return mpt::String::Convert( MptVersion::GetContactString(), mpt::CharsetISO8859_1, mpt::CharsetUTF8 );
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

} } // namespace openmpt::version
