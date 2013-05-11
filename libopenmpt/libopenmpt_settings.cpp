/*
 * libopenmpt_settings.cpp
 * -----------------------
 * Purpose: libopenmpt plugin settings
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#define LIBOPENMPT_BUILD_SETTINGS_DLL
#include "libopenmpt_settings.h"
#include "libopenmpt_settings_dialog.h"

namespace openmpt {

registry_settings::registry_settings( const char * subkey_ ) : subkey(subkey_) {
	return;
}

registry_settings::~registry_settings() {
	return;
}

} // namespace openmpt
