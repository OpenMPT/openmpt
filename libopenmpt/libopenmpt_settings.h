/*
 * libopenmpt_settings.h
 * ---------------------
 * Purpose: libopenmpt plugin settings
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_SETTINGS_H
#define LIBOPENMPT_SETTINGS_H

#define LIBOPENMPT_ALPHA_WARNING_SEEN_AND_I_KNOW_WHAT_I_AM_DOING
#include "libopenmpt.h"

#if defined(LIBOPENMPT_BUILD_SETTINGS_DLL)
#define LIBOPENMPT_SETTINGS_API __declspec(dllexport)
#elif defined(LIBOPENMPT_USE_SETTINGS_DLL)
#define LIBOPENMPT_SETTINGS_API __declspec(dllimport)
#ifndef _DEBUG
#pragma comment ( lib, "bin\\libopenmpt_settings.lib" )
#else
#pragma comment ( lib, "Debug\\libopenmpt_settings.lib" )
#endif
#endif

#include <map>
#include <string>

#include <windows.h>

namespace openmpt {

#ifdef _MSC_VER
#pragma warning ( disable : 4275 ) // warning C4275: non dll-interface class 'foo' used as base for dll-interface class 'bar'
#endif

class settings {
public:
	bool with_outputformat;
	int samplerate;
	int channels;
	int mastergain_millibel;
	int stereoseparation;
	int repeatcount;
	int maxmixchannels;
	int interpolationfilterlength;
	int volrampinus;
	int volrampoutus;
	std::map<std::string,int> save_map() const;
	void load_map( const std::map<std::string,int> & map );
	void load();
	void save();
	void edit( HWND parent, const char * title );
	settings( bool with_outputformat_ = true );
	virtual ~settings();
	virtual void changed();
	virtual void read_setting( const char * key, int & val );
	virtual void write_setting( const char * key, int val );
	virtual void edit_settings( HWND parent, const char * title );
	void import_xml( const std::string & xml );
	std::string export_xml() const;
};

inline std::map<std::string,int> settings::save_map() const {
	std::map<std::string,int> result;
	result[ "Samplerate_Hz" ] = samplerate;
	result[ "Channels" ] = channels;
	result[ "MasterGain_milliBel" ] = mastergain_millibel;
	result[ "SeteroSeparation_Percent" ] = stereoseparation;
	result[ "RepeatCount" ] = repeatcount;
	result[ "MixerChannels" ] = maxmixchannels;
	result[ "InterpolationFilterLength" ] = interpolationfilterlength;
	result[ "VolumeRampingIn_microseconds" ] = volrampinus;
	result[ "VolumeRampingOut_microseconds" ] = volrampoutus;
	return result;
}

static inline void load_map_setting( const std::map<std::string,int> & map, const std::string & key, int & val ) {
	std::map<std::string,int>::const_iterator it = map.find( key );
	if ( it != map.end() ) {
		val = it->second;
	}
}

inline void settings::load_map( const std::map<std::string,int> & map ) {
	load_map_setting( map, "Samplerate_Hz", samplerate );
	load_map_setting( map, "Channels", channels );
	load_map_setting( map, "MasterGain_milliBel", mastergain_millibel );
	load_map_setting( map, "SeteroSeparation_Percent", stereoseparation );
	load_map_setting( map, "RepeatCount", repeatcount );
	load_map_setting( map, "MixerChannels", maxmixchannels );
	load_map_setting( map, "InterpolationFilterLength", interpolationfilterlength );
	load_map_setting( map, "VolumeRampingIn_microseconds", volrampinus );
	load_map_setting( map, "VolumeRampingOut_microseconds", volrampoutus );
}

inline void settings::load() {
	read_setting( "Samplerate_Hz", samplerate );
	read_setting( "Channels", channels );
	read_setting( "MasterGain_milliBel", mastergain_millibel );
	read_setting( "SeteroSeparation_Percent", stereoseparation );
	read_setting( "RepeatCount", repeatcount );
	read_setting( "MixerChannels", maxmixchannels );
	read_setting( "InterpolationFilterLength", interpolationfilterlength );
	read_setting( "VolumeRampingIn_microseconds", volrampinus );
	read_setting( "VolumeRampingOut_microseconds", volrampoutus );
}

inline void settings::save() {
	write_setting( "Samplerate_Hz", samplerate );
	write_setting( "Channels", channels );
	write_setting( "MasterGain_milliBel", mastergain_millibel );
	write_setting( "SeteroSeparation_Percent", stereoseparation );
	write_setting( "RepeatCount", repeatcount );
	write_setting( "MixerChannels", maxmixchannels );
	write_setting( "InterpolationFilterLength", interpolationfilterlength );
	write_setting( "VolumeRampingIn_microseconds", volrampinus );
	write_setting( "VolumeRampingOut_microseconds", volrampoutus );
}

inline void settings::edit( HWND parent, const char * title ) {
	edit_settings( parent, title );
}

inline settings::settings( bool with_outputformat_ ) : with_outputformat(with_outputformat_) {
	samplerate = 48000;
	channels = 2;
	mastergain_millibel = 0;
	stereoseparation = 100;
	repeatcount = 0;
	maxmixchannels = 256;
	interpolationfilterlength = 8;
	volrampinus = 363;
	volrampoutus = 952;
}

inline settings::~settings() {
	return;
}

inline void settings::changed() {
	return;
}

inline void settings::read_setting( const char * key, int & val ) {
	return;
}

inline void settings::write_setting( const char * key, int val ) {
	return;
}

inline void settings::edit_settings( HWND parent, const char * title ) {
	return;
}

class LIBOPENMPT_SETTINGS_API registry_settings : public settings {
private:
	const char * subkey;
public:
	registry_settings( const char * subkey_ );
	virtual ~registry_settings();
	virtual void read_setting( const char * key, int & val );
	virtual void write_setting( const char * key, int val );
	virtual void edit_settings( HWND parent, const char * title );
};

#ifdef _MSC_VER
#pragma warning ( default : 4275 ) // warning C4275: non dll-interface class 'foo' used as base for dll-interface class 'bar'
#endif

} // namespace openmpt

#endif // LIBOPENMPT_SETTINGS_H
