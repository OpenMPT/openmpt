/*
 * libopenmpt_settings.hpp
 * -----------------------
 * Purpose: libopenmpt plugin settings
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_SETTINGS_HPP
#define LIBOPENMPT_SETTINGS_HPP

#include "libopenmpt_settings.h"

#include <string>

namespace openmpt { namespace settings {

class settings : public libopenmpt_settings {
private:
	std::wstring subkey;
private:
	static void read_setting( const std::wstring & subkey, const std::wstring & key, int & val ) {
		HKEY regkey = HKEY();
		if ( RegOpenKeyEx( HKEY_CURRENT_USER, ( L"Software\\libopenmpt\\" + subkey ).c_str(), 0, KEY_READ, &regkey ) == ERROR_SUCCESS ) {
			DWORD v = val;
			DWORD type = REG_DWORD;
			DWORD typesize = sizeof(v);
			if ( RegQueryValueEx( regkey, key.c_str(), NULL, &type, (BYTE *)&v, &typesize ) == ERROR_SUCCESS )
			{
				val = v;
			}
			RegCloseKey( regkey );
			regkey = HKEY();
		}
	}
	static void write_setting( const std::wstring & subkey, const std::wstring & key, int val ) {
		HKEY regkey = HKEY();
		if ( RegOpenKeyEx( HKEY_CURRENT_USER, ( L"Software\\libopenmpt\\" + subkey ).c_str(), 0, KEY_WRITE, &regkey ) == ERROR_SUCCESS ) {
			DWORD v = val;
			DWORD type = REG_DWORD;
			DWORD typesize = sizeof(v);
			if ( RegSetValueEx( regkey, key.c_str(), NULL, type, (const BYTE *)&v, typesize ) == ERROR_SUCCESS )
			{
				// ok
			}
			RegCloseKey( regkey );
			regkey = HKEY();
		}
	}
public:
	settings( const std::wstring & subkey, bool withoutputformat )
		: subkey(subkey)
	{
		with_outputformat = withoutputformat;
		samplerate = 48000;
		channels = 2;
		mastergain_millibel = 0;
		stereoseparation = 100;
		repeatcount = 0;
		interpolationfilterlength = 8;
		ramping = -1;
		changed = 0;
	}
	void load()
	{
		read_setting( subkey, L"Samplerate_Hz", samplerate );
		read_setting( subkey, L"Channels", channels );
		read_setting( subkey, L"MasterGain_milliBel", mastergain_millibel );
		read_setting( subkey, L"SeteroSeparation_Percent", stereoseparation );
		read_setting( subkey, L"RepeatCount", repeatcount );
		read_setting( subkey, L"InterpolationFilterLength", interpolationfilterlength );
		read_setting( subkey, L"VolumeRampingStrength", ramping );
	}
	void save()
	{
		write_setting( subkey, L"Samplerate_Hz", samplerate );
		write_setting( subkey, L"Channels", channels );
		write_setting( subkey, L"MasterGain_milliBel", mastergain_millibel );
		write_setting( subkey, L"SeteroSeparation_Percent", stereoseparation );
		write_setting( subkey, L"RepeatCount", repeatcount );
		write_setting( subkey, L"InterpolationFilterLength", interpolationfilterlength );
		write_setting( subkey, L"VolumeRampingStrength", ramping );
	}
};

} } // namespace openmpt::settings

#endif // LIBOPENMPT_SETTINGS_HPP
