/*
 * libopenmpt_settings.cpp
 * -----------------------
 * Purpose: libopenmpt plugin settings
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "libopenmpt_internal.h"

#define LIBOPENMPT_BUILD_SETTINGS_DLL
#include "libopenmpt_settings.h"
#include "SettingsForm.h"

namespace openmpt { namespace settings {

	
static void read_setting( const char * subkey, const char * key, int & val ) {
	System::String ^ net_root = "HKEY_CURRENT_USER\\Software\\libopenmpt\\";
	System::String ^ net_path = gcnew System::String( subkey );
	System::String ^ net_key = gcnew System::String( key );
	System::Object ^ retval = Microsoft::Win32::Registry::GetValue( System::String::Concat( net_root, net_path ), net_key, val );
	if ( retval == nullptr ) {
		return;
	} else {
		val = (int)retval;
	}
}

static void write_setting( const char * subkey, const char * key, int val ) {
	System::String ^ net_root = "HKEY_CURRENT_USER\\Software\\libopenmpt\\";
	System::String ^ net_path = gcnew System::String( subkey );
	System::String ^ net_key = gcnew System::String( key );
	Microsoft::Win32::Registry::SetValue( System::String::Concat( net_root, net_path ), net_key, val );
}

void load( settings & s, const char * subkey ) {
	read_setting( subkey, "Samplerate_Hz", s.samplerate );
	read_setting( subkey, "Channels", s.channels );
	read_setting( subkey, "MasterGain_milliBel", s.mastergain_millibel );
	read_setting( subkey, "SeteroSeparation_Percent", s.stereoseparation );
	read_setting( subkey, "RepeatCount", s.repeatcount );
	read_setting( subkey, "InterpolationFilterLength", s.interpolationfilterlength );
	read_setting( subkey, "VolumeRampingIn_microseconds", s.volrampinus );
	read_setting( subkey, "VolumeRampingOut_microseconds", s.volrampoutus );
}

void save( const settings & s, const char * subkey ) {
	write_setting( subkey, "Samplerate_Hz", s.samplerate );
	write_setting( subkey, "Channels", s.channels );
	write_setting( subkey, "MasterGain_milliBel", s.mastergain_millibel );
	write_setting( subkey, "SeteroSeparation_Percent", s.stereoseparation );
	write_setting( subkey, "RepeatCount", s.repeatcount );
	write_setting( subkey, "InterpolationFilterLength", s.interpolationfilterlength );
	write_setting( subkey, "VolumeRampingIn_microseconds", s.volrampinus );
	write_setting( subkey, "VolumeRampingOut_microseconds", s.volrampoutus );
}

void edit( settings & s, HWND parent, const char * title ) {
	libopenmpt::SettingsForm ^ form = gcnew libopenmpt::SettingsForm( title, &s );
	System::Windows::Forms::IWin32Window ^w = System::Windows::Forms::Control::FromHandle((System::IntPtr)parent);
	form->Show(w);
}

} } // namespace openmpt::settings
