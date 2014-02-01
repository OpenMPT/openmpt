/*
 * libopenmpt_impl.cpp
 * -------------------
 * Purpose: libopenmpt private interface implementation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "common/stdafx.h"

#include "libopenmpt_internal.h"
#include "libopenmpt.hpp"

#include "libopenmpt_impl.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "common/version.h"
#include "common/misc_util.h"
#include "soundlib/Sndfile.h"
#include "soundlib/AudioReadTarget.h"
#include "soundlib/FileReader.h"
#include "test/test.h"

namespace openmpt {

#if defined( LIBOPENMPT_BUILD_TEST )
void run_tests() {
	MptTest::DoTests();
}
#endif // LIBOPENMPT_BUILD_TEST

namespace version {

static const char * const license =
"The OpenMPT code is licensed under the BSD license." "\n"
" " "\n"
"Copyright (c) 2004-2014, OpenMPT contributors" "\n"
"Copyright (c) 1997-2003, Olivier Lapicque" "\n"
"All rights reserved." "\n"
"" "\n"
"Redistribution and use in source and binary forms, with or without" "\n"
"modification, are permitted provided that the following conditions are met:" "\n"
"    * Redistributions of source code must retain the above copyright" "\n"
"      notice, this list of conditions and the following disclaimer." "\n"
"    * Redistributions in binary form must reproduce the above copyright" "\n"
"      notice, this list of conditions and the following disclaimer in the" "\n"
"      documentation and/or other materials provided with the distribution." "\n"
"    * Neither the name of the OpenMPT project nor the" "\n"
"      names of its contributors may be used to endorse or promote products" "\n"
"      derived from this software without specific prior written permission." "\n"
"" "\n"
"THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY" "\n"
"EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED" "\n"
"WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE" "\n"
"DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY" "\n"
"DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES" "\n"
"(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;" "\n"
"LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND" "\n"
"ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT" "\n"
"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS" "\n"
"SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE." "\n"
;

std::uint32_t get_library_version() {
	return OPENMPT_API_VERSION | ( MptVersion::GetRevision() & 0xffff );
}

std::uint32_t get_core_version() {
	return MptVersion::num;
}

static std::string get_library_version_string() {
	std::string str;
	std::uint32_t version = get_library_version();
	if ( ( version & 0xffff ) == 0 ) {
		str += Stringify((version>>24) & 0xff);
		str += ".";
		str += Stringify((version>>16) & 0xff);
	} else {
		str += Stringify((version>>24) & 0xff);
		str += ".";
		str += Stringify((version>>16) & 0xff);
		str += ".";
		str += Stringify((version>>0) & 0xffff);
	}
	if ( MptVersion::IsDirty() ) {
		str += ".2-modified";
		if ( MptVersion::IsPackage() ) {
			str += "-pkg";
		}
	} else if ( MptVersion::HasMixedRevisions() ) {
		str += ".1-modified";
		if ( MptVersion::IsPackage() ) {
			str += "-pkg";
		}
	} else if ( MptVersion::IsPackage() ) {
		str += ".0-pkg";
	}
	return str;
}

static std::string get_library_features_string() {
	return MptVersion::GetBuildFeaturesString();
}

static std::string get_core_version_string() {
	return MptVersion::GetVersionStringExtended();
}

static std::string get_build_string() {
	return MptVersion::GetBuildDateString();
}

static std::string get_credits_string() {
	return MptVersion::GetFullCreditsString();
}

static std::string get_contact_string() {
	return MptVersion::GetContactString();
}

static std::string get_license_string() {
	return license;
}

std::string get_string( const std::string & key ) {
	if ( key == "" ) {
		return std::string();
	} else if ( key == string::library_version ) {
		return get_library_version_string();
	} else if ( key == string::library_features ) {
		return get_library_features_string();
	} else if ( key == string::core_version ) {
		return get_core_version_string();
	} else if ( key == string::build ) {
		return get_build_string();
	} else if ( key == string::credits ) {
		return get_credits_string();
	} else if ( key == string::contact ) {
		return get_contact_string();
	} else if ( key == string::license ) {
		return get_license_string();
	} else {
		return std::string();
	}
}

} // namespace version

 log_interface::log_interface() {
	return;
}
log_interface::~log_interface() {
	return;
}

std_ostream_log::std_ostream_log( std::ostream & dst ) : destination(dst) {
	return;
}
std_ostream_log::~std_ostream_log() {
	return;
}
void std_ostream_log::log( const std::string & message ) const {
	destination.flush();
	destination << message << std::endl;
	destination.flush();
}

class log_forwarder : public ILog {
private:
	std::shared_ptr<log_interface> destination;
public:
	log_forwarder( std::shared_ptr<log_interface> dest ) : destination(dest) {
		return;
	}
	virtual ~log_forwarder() {
		return;
	}
private:
	void AddToLog( LogLevel level, const std::string & text ) const {
		destination->log( LogLevelToString(level) + std::string(": ") + text );
	}
}; // class log_forwarder

class loader_log : public ILog {
private:
	mutable std::vector<std::pair<LogLevel,std::string> > m_Messages;
public:
	std::vector<std::pair<LogLevel,std::string> > GetMessages() const;
private:
	void AddToLog( LogLevel level, const std::string & text ) const;
}; // class loader_log

std::vector<std::pair<LogLevel,std::string> > loader_log::GetMessages() const {
	return m_Messages;
}
void loader_log::AddToLog( LogLevel level, const std::string & text ) const {
	m_Messages.push_back( std::make_pair( level, text ) );
}

void module_impl::PushToCSoundFileLog( const std::string & text ) const {
	m_sndFile->AddToLog( LogError, text );
}
void module_impl::PushToCSoundFileLog( int loglevel, const std::string & text ) const {
	m_sndFile->AddToLog( (LogLevel)loglevel, text );
}

static ResamplingMode filterlength_to_resamplingmode(std::int32_t length) {
	if ( length == 0 ) {
		return SRCMODE_POLYPHASE;
	} else if ( length >= 8 ) {
		return SRCMODE_POLYPHASE;
	} else if ( length >= 3 ) {
		return SRCMODE_SPLINE;
	} else if ( length >= 2 ) {
		return SRCMODE_LINEAR;
	} else if ( length >= 1 ) {
		return SRCMODE_NEAREST;
	} else {
		throw openmpt::exception("negative filter length");
		return SRCMODE_POLYPHASE;
	}
}
static std::int32_t resamplingmode_to_filterlength(ResamplingMode mode) {
	switch ( mode ) {
	case SRCMODE_NEAREST:
		return 1;
		break;
	case SRCMODE_LINEAR:
		return 2;
		break;
	case SRCMODE_SPLINE:
		return 4;
		break;
	case SRCMODE_POLYPHASE:
	case SRCMODE_FIRFILTER:
	case SRCMODE_DEFAULT:
		return 8;
	default:
		throw openmpt::exception("unknown interpolation filter length set internally");
		break;
	}
}

static void ramping_to_mixersettings( MixerSettings & settings, int ramping ) {
	if ( ramping == -1 ) {
		settings.SetVolumeRampUpMicroseconds( MixerSettings().GetVolumeRampUpMicroseconds() );
		settings.SetVolumeRampDownMicroseconds( MixerSettings().GetVolumeRampDownMicroseconds() );
	} else if ( ramping <= 0 ) {
		settings.SetVolumeRampUpMicroseconds( 0 );
		settings.SetVolumeRampDownMicroseconds( 0 );
	} else {
		settings.SetVolumeRampUpMicroseconds( ramping * 1000 );
		settings.SetVolumeRampDownMicroseconds( ramping * 1000 );
	}
}
static void mixersettings_to_ramping( int & ramping, const MixerSettings & settings ) {
	std::int32_t ramp_us = std::max<std::int32_t>( settings.GetVolumeRampUpMicroseconds(), settings.GetVolumeRampDownMicroseconds() );
	if ( true
	     && settings.GetVolumeRampUpMicroseconds() == MixerSettings().GetVolumeRampUpMicroseconds()
	     && settings.GetVolumeRampDownMicroseconds() == MixerSettings().GetVolumeRampDownMicroseconds()
	   ) {
		ramping = -1;
	} else if ( ramp_us <= 0 ) {
		ramping = 0;
	} else {
		ramping = ( ramp_us + 500 ) / 1000;
	}
}

std::string module_impl::mod_string_to_utf8( const std::string & encoded ) const {
	return mpt::To( mpt::CharsetUTF8, m_sndFile->GetCharset().second, encoded );
}
void module_impl::apply_mixer_settings( std::int32_t samplerate, int channels ) {
	if (
		static_cast<std::int32_t>( m_sndFile->m_MixerSettings.gdwMixingFreq ) != samplerate ||
		static_cast<int>( m_sndFile->m_MixerSettings.gnChannels ) != channels
		) {
		MixerSettings mixersettings = m_sndFile->m_MixerSettings;
		std::int32_t volrampin_us = mixersettings.GetVolumeRampUpMicroseconds();
		std::int32_t volrampout_us = mixersettings.GetVolumeRampDownMicroseconds();
		mixersettings.gdwMixingFreq = samplerate;
		mixersettings.gnChannels = channels;
		mixersettings.SetVolumeRampUpMicroseconds( volrampin_us );
		mixersettings.SetVolumeRampDownMicroseconds( volrampout_us );
		m_sndFile->SetMixerSettings( mixersettings );
	}
}
void module_impl::apply_libopenmpt_defaults() {
	set_render_param( module::RENDER_STEREOSEPARATION_PERCENT, 100 );
}
void module_impl::init( const std::map< std::string, std::string > & ctls ) {
	m_sndFile = std::unique_ptr<CSoundFile>(new CSoundFile());
	m_Dither = std::unique_ptr<Dither>(new Dither());
	m_LogForwarder = std::unique_ptr<log_forwarder>(new log_forwarder(m_Log));
	m_sndFile->SetCustomLog( m_LogForwarder.get() );
	m_currentPositionSeconds = 0.0;
	m_Gain = 1.0f;
	for ( std::map< std::string, std::string >::const_iterator i = ctls.begin(); i != ctls.end(); ++i ) {
		ctl_set( i->first, i->second );
	}
}
void module_impl::load( CSoundFile & sndFile, const FileReader & file ) {
	if ( !sndFile.Create( file, CSoundFile::loadCompleteModule ) ) {
		throw openmpt::exception("error loading file");
	}
}
void module_impl::load( const FileReader & file ) {
	loader_log loaderlog;
	m_sndFile->SetCustomLog( &loaderlog );
	load( *m_sndFile, file );
	m_sndFile->SetCustomLog( m_LogForwarder.get() );
	std::vector<std::pair<LogLevel,std::string> > loaderMessages = loaderlog.GetMessages();
	for ( std::vector<std::pair<LogLevel,std::string> >::iterator i = loaderMessages.begin(); i != loaderMessages.end(); ++i ) {
		PushToCSoundFileLog( i->first, i->second );
		m_loaderMessages.push_back( LogLevelToString( i->first ) + std::string(": ") + i->second );
	}
}
std::size_t module_impl::read_wrapper( std::size_t count, std::int16_t * left, std::int16_t * right, std::int16_t * rear_left, std::int16_t * rear_right ) {
	m_sndFile->ResetMixStat();
	std::size_t count_read = 0;
	while ( count > 0 ) {
		std::int16_t * const buffers[4] = { left + count_read, right + count_read, rear_left + count_read, rear_right + count_read };
		AudioReadTargetGainBuffer<std::int16_t> target(*m_Dither, 0, buffers, m_Gain);
		std::size_t count_chunk = m_sndFile->Read(
			static_cast<CSoundFile::samplecount_t>( std::min<std::uint64_t>( count, std::numeric_limits<CSoundFile::samplecount_t>::max() / 2 / 4 / 4 ) ), // safety margin / samplesize / channels
			target
			);
		if ( count_chunk == 0 ) {
			break;
		}
		count -= count_chunk;
		count_read += count_chunk;
	}
	return count_read;
}
std::size_t module_impl::read_wrapper( std::size_t count, float * left, float * right, float * rear_left, float * rear_right ) {
	m_sndFile->ResetMixStat();
	std::size_t count_read = 0;
	while ( count > 0 ) {
		float * const buffers[4] = { left + count_read, right + count_read, rear_left + count_read, rear_right + count_read };
		AudioReadTargetGainBuffer<float> target(*m_Dither, 0, buffers, m_Gain);
		std::size_t count_chunk = m_sndFile->Read(
			static_cast<CSoundFile::samplecount_t>( std::min<std::uint64_t>( count, std::numeric_limits<CSoundFile::samplecount_t>::max() / 2 / 4 / 4 ) ), // safety margin / samplesize / channels
			target
			);
		if ( count_chunk == 0 ) {
			break;
		}
		count -= count_chunk;
		count_read += count_chunk;
	}
	return count_read;
}
std::size_t module_impl::read_interleaved_wrapper( std::size_t count, std::size_t channels, std::int16_t * interleaved ) {
	m_sndFile->ResetMixStat();
	std::size_t count_read = 0;
	while ( count > 0 ) {
		AudioReadTargetGainBuffer<std::int16_t> target(*m_Dither, interleaved + count_read * channels, 0, m_Gain);
		std::size_t count_chunk = m_sndFile->Read(
			static_cast<CSoundFile::samplecount_t>( std::min<std::uint64_t>( count, std::numeric_limits<CSoundFile::samplecount_t>::max() / 2 / 4 / 4 ) ), // safety margin / samplesize / channels
			target
			);
		if ( count_chunk == 0 ) {
			break;
		}
		count -= count_chunk;
		count_read += count_chunk;
	}
	return count_read;
}
std::size_t module_impl::read_interleaved_wrapper( std::size_t count, std::size_t channels, float * interleaved ) {
	m_sndFile->ResetMixStat();
	std::size_t count_read = 0;
	while ( count > 0 ) {
		AudioReadTargetGainBuffer<float> target(*m_Dither, interleaved + count_read * channels, 0, m_Gain);
		std::size_t count_chunk = m_sndFile->Read(
			static_cast<CSoundFile::samplecount_t>( std::min<std::uint64_t>( count, std::numeric_limits<CSoundFile::samplecount_t>::max() / 2 / 4 / 4 ) ), // safety margin / samplesize / channels
			target
			);
		if ( count_chunk == 0 ) {
			break;
		}
		count -= count_chunk;
		count_read += count_chunk;
	}
	return count_read;
}

std::vector<std::string> module_impl::get_supported_extensions() {
	std::vector<std::string> retval;
	std::vector<const char *> extensions = CSoundFile::GetSupportedExtensions( false );
	std::copy( extensions.begin(), extensions.end(), std::back_insert_iterator<std::vector<std::string> >( retval ) );
	return retval;
}
bool module_impl::is_extension_supported( const std::string & extension ) {
	std::vector<std::string> extensions = get_supported_extensions();
	std::string lowercase_ext = extension;
	std::transform( lowercase_ext.begin(), lowercase_ext.end(), lowercase_ext.begin(), tolower );
	return std::find( extensions.begin(), extensions.end(), lowercase_ext ) != extensions.end();
}
double module_impl::could_open_propability( std::istream & stream, double effort, std::shared_ptr<log_interface> log ) {
	std::unique_ptr<CSoundFile> sndFile( new CSoundFile() );
	std::unique_ptr<log_forwarder> logForwarder( new log_forwarder( log ) );
	sndFile->SetCustomLog( logForwarder.get() );

	try {

		if ( effort >= 0.8 ) {
			if ( !sndFile->Create( FileReader( &stream ), CSoundFile::loadCompleteModule ) ) {
				return 0.0;
			}
			sndFile->Destroy();
			return 1.0;
		} else if ( effort >= 0.6 ) {
			if ( !sndFile->Create( FileReader( &stream ), CSoundFile::loadNoPatternOrPluginData ) ) {
				return 0.0;
			}
			sndFile->Destroy();
			return 0.8;
		} else if ( effort >= 0.2 ) {
			if ( !sndFile->Create( FileReader( &stream ), CSoundFile::onlyVerifyHeader ) ) {
				return 0.0;
			}
			sndFile->Destroy();
			return 0.6;
		} else {
			return 0.2;
		}

	} catch ( ... ) {
		return 0.0;
	}

}

module_impl::module_impl( std::istream & stream, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
	init( ctls );
	load( FileReader( &stream ) );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const std::vector<std::uint8_t> & data, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
	init( ctls );
	load( FileReader( data.data(), data.size() ) );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const std::vector<char> & data, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
	init( ctls );
	load( FileReader( data.data(), data.size() ) );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const std::uint8_t * data, std::size_t size, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
	init( ctls );
	load( FileReader( data, size ) );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const char * data, std::size_t size, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
	init( ctls );
	load( FileReader( data, size ) );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const void * data, std::size_t size, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
	init( ctls );
	load( FileReader( data, size ) );
	apply_libopenmpt_defaults();
}
module_impl::~module_impl() {
	m_sndFile->Destroy();
}

std::int32_t module_impl::get_render_param( int param ) const {
	switch ( param ) {
		case module::RENDER_MASTERGAIN_MILLIBEL: {
			return static_cast<std::int32_t>( 1000.0f * 2.0f * std::log10( m_Gain ) );
		} break;
		case module::RENDER_STEREOSEPARATION_PERCENT: {
			return m_sndFile->m_MixerSettings.m_nStereoSeparation * 100 / 128;
		} break;
		case module::RENDER_INTERPOLATIONFILTER_LENGTH: {
			return resamplingmode_to_filterlength( m_sndFile->m_Resampler.m_Settings.SrcMode );
		} break;
		case module::RENDER_VOLUMERAMPING_STRENGTH: {
			int ramping = 0;
			mixersettings_to_ramping( ramping, m_sndFile->m_MixerSettings );
			return ramping;
		} break;
		default: throw openmpt::exception("unknown render param"); break;
	}
	return 0;
}
void module_impl::set_render_param( int param, std::int32_t value ) {
	switch ( param ) {
		case module::RENDER_MASTERGAIN_MILLIBEL: {
			m_Gain = static_cast<float>( std::pow( 10.0f, value * 0.001f * 0.5f ) );
		} break;
		case module::RENDER_STEREOSEPARATION_PERCENT: {
			std::int32_t newvalue = value * 128 / 100;
			if ( newvalue != static_cast<std::int32_t>( m_sndFile->m_MixerSettings.m_nStereoSeparation ) ) {
				MixerSettings settings = m_sndFile->m_MixerSettings;
				settings.m_nStereoSeparation = newvalue;
				m_sndFile->SetMixerSettings( settings );
			}
		} break;
		case module::RENDER_INTERPOLATIONFILTER_LENGTH: {
			CResamplerSettings newsettings;
			newsettings.SrcMode = filterlength_to_resamplingmode( value );
			if ( newsettings != m_sndFile->m_Resampler.m_Settings ) {
				m_sndFile->SetResamplerSettings( newsettings );
			}
		} break;
		case module::RENDER_VOLUMERAMPING_STRENGTH: {
			MixerSettings newsettings = m_sndFile->m_MixerSettings;
			ramping_to_mixersettings( newsettings, value );
			if ( m_sndFile->m_MixerSettings.VolumeRampUpMicroseconds != newsettings.VolumeRampUpMicroseconds || m_sndFile->m_MixerSettings.VolumeRampDownMicroseconds != newsettings.VolumeRampDownMicroseconds ) {
				m_sndFile->SetMixerSettings( newsettings );
			}
		} break;
		default: throw openmpt::exception("unknown render param"); break;
	}
}

std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, std::int16_t * mono ) {
	if ( !mono ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 1 );
	count = read_wrapper( count, mono, 0, 0, 0 );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right ) {
	if ( !left || !right ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 2 );
	count = read_wrapper( count, left, right, 0, 0 );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right, std::int16_t * rear_left, std::int16_t * rear_right ) {
	if ( !left || !right || !rear_left || !rear_right ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 4 );
	count = read_wrapper( count, left, right, rear_left, rear_right );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, float * mono ) {
	if ( !mono ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 1 );
	count = read_wrapper( count, mono, 0, 0, 0 );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, float * left, float * right ) {
	if ( !left || !right ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 2 );
	count = read_wrapper( count, left, right, 0, 0 );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, float * left, float * right, float * rear_left, float * rear_right ) {
	if ( !left || !right || !rear_left || !rear_right ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 4 );
	count = read_wrapper( count, left, right, rear_left, rear_right );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read_interleaved_stereo( std::int32_t samplerate, std::size_t count, std::int16_t * interleaved_stereo ) {
	if ( !interleaved_stereo ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 2 );
	count = read_interleaved_wrapper( count, 2, interleaved_stereo );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read_interleaved_quad( std::int32_t samplerate, std::size_t count, std::int16_t * interleaved_quad ) {
	if ( !interleaved_quad ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 4 );
	count = read_interleaved_wrapper( count, 4, interleaved_quad );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read_interleaved_stereo( std::int32_t samplerate, std::size_t count, float * interleaved_stereo ) {
	if ( !interleaved_stereo ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 2 );
	count = read_interleaved_wrapper( count, 2, interleaved_stereo );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read_interleaved_quad( std::int32_t samplerate, std::size_t count, float * interleaved_quad ) {
	if ( !interleaved_quad ) {
		throw openmpt::exception("null pointer");
	}
	apply_mixer_settings( samplerate, 4 );
	count = read_interleaved_wrapper( count, 4, interleaved_quad );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}


double module_impl::get_duration_seconds() const {
	return m_sndFile->GetLength( eNoAdjust ).duration;
}
void module_impl::select_subsong( std::int32_t subsong ) {
	if ( subsong < -1 || subsong >= m_sndFile->Order.GetNumSequences() ) {
		return;
	}
	if ( subsong == -1 ) {
		// default subsong
		m_sndFile->Order.SetSequence( 0 );
		return;
	}
	m_sndFile->Order.SetSequence( subsong );
}
void module_impl::set_repeat_count( std::int32_t repeat_count ) {
	m_sndFile->SetRepeatCount( repeat_count );
}
std::int32_t module_impl::get_repeat_count() const {
	return m_sndFile->GetRepeatCount();
}
double module_impl::get_position_seconds() const {
	return m_currentPositionSeconds;
}
double module_impl::set_position_seconds( double seconds ) {
	GetLengthType t = m_sndFile->GetLength( eNoAdjust, GetLengthTarget( seconds ) );
	m_sndFile->InitializeVisitedRows();
	m_sndFile->m_nCurrentOrder = t.lastOrder;
	m_sndFile->SetCurrentOrder( t.lastOrder );
	m_sndFile->m_nNextRow = t.lastRow;
	m_currentPositionSeconds = m_sndFile->GetLength( eAdjust, GetLengthTarget( t.lastOrder, t.lastRow ) ).duration;
	return m_currentPositionSeconds;
}
double module_impl::set_position_order_row( std::int32_t order, std::int32_t row ) {
	if ( order < 0 || order >= m_sndFile->Order.GetLengthTailTrimmed() ) {
		return m_currentPositionSeconds;
	}
	std::int32_t pattern = m_sndFile->Order[order];
	if ( m_sndFile->Patterns.IsValidIndex( pattern ) ) {
		if ( row < 0 || row >= (std::int32_t)m_sndFile->Patterns[pattern].GetNumRows() ) {
			return m_currentPositionSeconds;
		}
	} else {
		row = 0;
	}
	m_sndFile->InitializeVisitedRows();
	m_sndFile->m_nCurrentOrder = order;
	m_sndFile->SetCurrentOrder( order );
	m_sndFile->m_nNextRow = row;
	m_currentPositionSeconds = m_sndFile->GetLength( eAdjust, GetLengthTarget( order, row ) ).duration;
	return m_currentPositionSeconds;
}
std::vector<std::string> module_impl::get_metadata_keys() const {
	std::vector<std::string> retval;
	retval.push_back("type");
	retval.push_back("type_long");
	retval.push_back("container");
	retval.push_back("container_long");
	retval.push_back("tracker");
	retval.push_back("artist");
	retval.push_back("title");
	retval.push_back("date");
	retval.push_back("message");
	retval.push_back("warnings");
	return retval;
}
std::string module_impl::get_metadata( const std::string & key ) const {
	if ( key == std::string("type") ) {
		return CSoundFile::ModTypeToString( m_sndFile->GetType() );
	} else if ( key == std::string("type_long") ) {
		return CSoundFile::ModTypeToTracker( m_sndFile->GetType() );
	} else if ( key == std::string("container") ) {
		return CSoundFile::ModContainerTypeToString( m_sndFile->GetContainerType() );
	} else if ( key == std::string("container_long") ) {
		return CSoundFile::ModContainerTypeToTracker( m_sndFile->GetContainerType() );
	} else if ( key == std::string("tracker") ) {
		return m_sndFile->madeWithTracker;
	} else if ( key == std::string("artist") ) {
		return mod_string_to_utf8( m_sndFile->songArtist );
	} else if ( key == std::string("title") ) {
		return mod_string_to_utf8( m_sndFile->GetTitle() );
	} else if ( key == std::string("date") ) {
		if ( m_sndFile->GetFileHistory().empty() ) {
			return std::string();
		}
		return mod_string_to_utf8( m_sndFile->GetFileHistory()[m_sndFile->GetFileHistory().size() - 1].AsISO8601() );
	} else if ( key == std::string("message") ) {
		std::string retval = m_sndFile->songMessage.GetFormatted( SongMessage::leLF );
		if ( retval.empty() ) {
			std::string tmp;
			bool valid = false;
			for ( INSTRUMENTINDEX i = 1; i <= m_sndFile->GetNumInstruments(); ++i ) {
				std::string instname = m_sndFile->GetInstrumentName( i );
				if ( !instname.empty() ) {
					valid = true;
				}
				tmp += instname;
				tmp += "\n";
			}
			if ( valid ) {
				retval = tmp;
			}
		}
		if ( retval.empty() ) {
			std::string tmp;
			bool valid = false;
			for ( SAMPLEINDEX i = 1; i <= m_sndFile->GetNumSamples(); ++i ) {
				std::string samplename = m_sndFile->GetSampleName( i );
				if ( !samplename.empty() ) {
					valid = true;
				}
				tmp += samplename;
				tmp += "\n";
			}
			if ( valid ) {
				retval = tmp;
			}
		}
		return mod_string_to_utf8( retval );
	} else if ( key == std::string("warnings") ) {
		std::string retval;
		bool first = true;
		for ( std::vector<std::string>::const_iterator i = m_loaderMessages.begin(); i != m_loaderMessages.end(); ++i ) {
			if ( !first ) {
				retval += "\n";
				first = false;
			}
			retval += *i;
		}
		return retval;
	}
	return "";
}

std::int32_t module_impl::get_current_speed() const {
	return m_sndFile->m_nMusicSpeed;
}
std::int32_t module_impl::get_current_tempo() const {
	return m_sndFile->m_nMusicTempo;
}
std::int32_t module_impl::get_current_order() const {
	return m_sndFile->GetCurrentOrder();
}
std::int32_t module_impl::get_current_pattern() const {
	std::int32_t order = m_sndFile->GetCurrentOrder();
	if ( order < 0 || order >= m_sndFile->Order.GetLengthTailTrimmed() ) {
		return m_sndFile->GetCurrentPattern();
	}
	std::int32_t pattern = m_sndFile->Order[order];
	if ( !m_sndFile->Patterns.IsValidIndex( pattern ) ) {
		return -1;
	}
	return pattern;
}
std::int32_t module_impl::get_current_row() const {
	return m_sndFile->m_nRow;
}
std::int32_t module_impl::get_current_playing_channels() const {
	return m_sndFile->GetMixStat();
}

float module_impl::get_current_channel_vu_mono( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	const float left = get_current_channel_vu_left( channel );
	const float right = get_current_channel_vu_right( channel );
	return std::sqrt(left*left + right*right);
}
float module_impl::get_current_channel_vu_left( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	return m_sndFile->Chn[channel].nLeftVU * (1.0f/128.0f);
}
float module_impl::get_current_channel_vu_right( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	return m_sndFile->Chn[channel].nRightVU * (1.0f/128.0f);
}
float module_impl::get_current_channel_vu_rear_left( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	return 0.0f; // FIXME m_sndFile->Chn[channel].nLeftVU * (1.0f/128.0f);
}
float module_impl::get_current_channel_vu_rear_right( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	return 0.0f; // FIXME m_sndFile->Chn[channel].nRightVU * (1.0f/128.0f);
}

std::int32_t module_impl::get_num_subsongs() const {
	return m_sndFile->Order.GetNumSequences();
}
std::int32_t module_impl::get_num_channels() const {
	return m_sndFile->GetNumChannels();
}
std::int32_t module_impl::get_num_orders() const {
	return m_sndFile->Order.GetLengthTailTrimmed();
}
std::int32_t module_impl::get_num_patterns() const {
	return m_sndFile->Patterns.GetNumPatterns();
}
std::int32_t module_impl::get_num_instruments() const {
	return m_sndFile->GetNumInstruments();
}
std::int32_t module_impl::get_num_samples() const {
	return m_sndFile->GetNumSamples();
}

std::vector<std::string> module_impl::get_subsong_names() const {
	std::vector<std::string> retval;
	for ( SEQUENCEINDEX i = 0; i < m_sndFile->Order.GetNumSequences(); ++i ) {
		retval.push_back( mod_string_to_utf8( m_sndFile->Order.GetSequence( i ).GetName() ) );
	}
	return retval;
}
std::vector<std::string> module_impl::get_channel_names() const {
	std::vector<std::string> retval;
	for ( CHANNELINDEX i = 0; i < m_sndFile->GetNumChannels(); ++i ) {
		retval.push_back( mod_string_to_utf8( m_sndFile->ChnSettings[i].szName ) );
	}
	return retval;
}
std::vector<std::string> module_impl::get_order_names() const {
	std::vector<std::string> retval;
	for ( ORDERINDEX i = 0; i < m_sndFile->Order.GetLengthTailTrimmed(); ++i ) {
		PATTERNINDEX pat = m_sndFile->Order[i];
		if ( m_sndFile->Patterns.IsValidIndex( pat ) ) {
			retval.push_back( mod_string_to_utf8( m_sndFile->Patterns[ m_sndFile->Order[i] ].GetName() ) );
		} else {
			if ( pat == m_sndFile->Order.GetIgnoreIndex() ) {
				retval.push_back( "+++ skip" );
			} else if ( pat == m_sndFile->Order.GetInvalidPatIndex() ) {
				retval.push_back( "--- stop" );
			} else {
				retval.push_back( "???" );
			}
		}
	}
	return retval;
}
std::vector<std::string> module_impl::get_pattern_names() const {
	std::vector<std::string> retval;
	for ( PATTERNINDEX i = 0; i < m_sndFile->Patterns.GetNumPatterns(); ++i ) {
		retval.push_back( mod_string_to_utf8( m_sndFile->Patterns[i].GetName() ) );
	}
	return retval;
}
std::vector<std::string> module_impl::get_instrument_names() const {
	std::vector<std::string> retval;
	for ( INSTRUMENTINDEX i = 1; i <= m_sndFile->GetNumInstruments(); ++i ) {
		retval.push_back( mod_string_to_utf8( m_sndFile->GetInstrumentName( i ) ) );
	}
	return retval;
}
std::vector<std::string> module_impl::get_sample_names() const {
	std::vector<std::string> retval;
	for ( SAMPLEINDEX i = 1; i <= m_sndFile->GetNumSamples(); ++i ) {
		retval.push_back( mod_string_to_utf8( m_sndFile->GetSampleName( i ) ) );
	}
	return retval;
}

std::int32_t module_impl::get_order_pattern( std::int32_t o ) const {
	if ( o < 0 || o >= m_sndFile->Order.GetLengthTailTrimmed() ) {
		return -1;
	}
	return m_sndFile->Order[o];
}
std::int32_t module_impl::get_pattern_num_rows( std::int32_t p ) const {
	if ( p < 0 || p >= m_sndFile->Patterns.Size() ) {
		return 0;
	}
	return m_sndFile->Patterns[p].GetNumRows();
}

std::uint8_t module_impl::get_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const {
	CHANNELINDEX numchannels = m_sndFile->GetNumChannels();
	if ( p < 0 || p >= m_sndFile->Patterns.Size() ) {
		return 0;
	}
	if ( r < 0 || r >= (std::int32_t)m_sndFile->Patterns[p].GetNumRows() ) {
		return 0;
	}
	if ( c < 0 || c >= numchannels ) {
		return 0;
	}
	if ( cmd < module::command_note || cmd > module::command_parameter ) {
		return 0;
	}
	switch ( cmd ) {
		case module::command_note: return m_sndFile->Patterns[p][r*numchannels+c].note; break;
		case module::command_instrument: return m_sndFile->Patterns[p][r*numchannels+c].instr; break;
		case module::command_volumeffect: return m_sndFile->Patterns[p][r*numchannels+c].volcmd; break;
		case module::command_effect: return m_sndFile->Patterns[p][r*numchannels+c].command; break;
		case module::command_volume: return m_sndFile->Patterns[p][r*numchannels+c].vol; break;
		case module::command_parameter: return m_sndFile->Patterns[p][r*numchannels+c].param; break;
	}
	return 0;
}

/*

highlight chars explained:

  : empty/space
. : empty/dot
n : generic note
m : special note
i : generic instrument
u : generic volume column effect
v : generic volume column parameter
e : generic effect column effect
f : generic effect column parameter

*/

std::pair< std::string, std::string > module_impl::format_and_highlight_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const {
	CHANNELINDEX numchannels = m_sndFile->GetNumChannels();
	if ( p < 0 || p >= m_sndFile->Patterns.Size() ) {
		return std::make_pair( std::string(), std::string() );
	}
	if ( r < 0 || r >= (std::int32_t)m_sndFile->Patterns[p].GetNumRows() ) {
		return std::make_pair( std::string(), std::string() );
	}
	if ( c < 0 || c >= numchannels ) {
		return std::make_pair( std::string(), std::string() );
	}
	if ( cmd < module::command_note || cmd > module::command_parameter ) {
		return std::make_pair( std::string(), std::string() );
	}
	const ModCommand & cell = m_sndFile->Patterns[p][r*numchannels+c];
	switch ( cmd ) {
		case module::command_note:
			return std::make_pair(
					( cell.IsNote() || cell.IsSpecialNote() ) ? m_sndFile->GetNoteName( cell.note, cell.instr ) : std::string("...")
				,
					( cell.IsNote() ) ? std::string("nnn") : cell.IsSpecialNote() ? std::string("mmm") : std::string("...")
				);
			break;
		case module::command_instrument:
			return std::make_pair(
					cell.instr ? mpt::fmt::HEX0<2>( cell.instr ) : std::string("..")
				,
					cell.instr ? std::string("ii") : std::string("..")
				);
			break;
		case module::command_volumeffect:
			return std::make_pair(
					cell.IsPcNote() ? std::string(" ") : cell.volcmd != VOLCMD_NONE ? std::string( 1, m_sndFile->GetModSpecifications().GetVolEffectLetter( cell.volcmd ) ) : std::string(" ")
				,
					cell.IsPcNote() ? std::string(" ") : cell.volcmd != VOLCMD_NONE ? std::string("u") : std::string(" ")
				);
			break;
		case module::command_volume:
			return std::make_pair(
					cell.IsPcNote() ? mpt::fmt::HEX0<2>( cell.GetValueVolCol() & 0xff ) : cell.volcmd != VOLCMD_NONE ? mpt::fmt::HEX0<2>( cell.vol ) : std::string("..")
				,
					cell.IsPcNote() ? std::string("vv") : cell.volcmd != VOLCMD_NONE ? std::string("vv") : std::string("..")
				);
			break;
		case module::command_effect:
			return std::make_pair(
					cell.IsPcNote() ? mpt::fmt::HEX0<1>( ( cell.GetValueEffectCol() & 0x0f00 ) > 16 ) : cell.command != CMD_NONE ? std::string( 1, m_sndFile->GetModSpecifications().GetEffectLetter( cell.command ) ) : std::string(".")
				,
					cell.IsPcNote() ? std::string("e") : cell.command != CMD_NONE ? std::string("e") : std::string(".")
				);
			break;
		case module::command_parameter:
			return std::make_pair(
					cell.IsPcNote() ? mpt::fmt::HEX0<2>( cell.GetValueEffectCol() & 0x00ff ) : cell.command != CMD_NONE ? mpt::fmt::HEX0<2>( cell.param ) : std::string("..")
				,
					cell.IsPcNote() ? std::string("ff") : cell.command != CMD_NONE ? std::string("ff") : std::string("..")
				);
			break;
	}
	return std::make_pair( std::string(), std::string() );
}
std::string module_impl::format_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const {
	return format_and_highlight_pattern_row_channel_command( p, r, c, cmd ).first;
}
std::string module_impl::highlight_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const {
	return format_and_highlight_pattern_row_channel_command( p, r, c, cmd ).second;
}

std::pair< std::string, std::string > module_impl::format_and_highlight_pattern_row_channel( std::int32_t p, std::int32_t r, std::int32_t c, std::size_t width, bool pad ) const {
	std::string text = pad ? std::string( width, ' ' ) : std::string();
	std::string high = pad ? std::string( width, ' ' ) : std::string();
	const CHANNELINDEX numchannels = m_sndFile->GetNumChannels();
	if ( p < 0 || p >= m_sndFile->Patterns.Size() ) {
		return std::make_pair( text, high );
	}
	if ( r < 0 || r >= (std::int32_t)m_sndFile->Patterns[p].GetNumRows() ) {
		return std::make_pair( text, high );
	}
	if ( c < 0 || c >= numchannels ) {
		return std::make_pair( text, high );
	}
	if ( width == 0 ) {
		return std::make_pair( text, high );
	}
	//  0000000001111
	//  1234567890123
	// "NNN IIvVV EFF"
	const ModCommand cell = m_sndFile->Patterns[p][r*numchannels+c];
	text.clear();
	high.clear();
	text += ( cell.IsNote() || cell.IsSpecialNote() ) ? m_sndFile->GetNoteName( cell.note, cell.instr ) : std::string("...");
	high += ( cell.IsNote() ) ? std::string("nnn") : cell.IsSpecialNote() ? std::string("mmm") : std::string("...");
	if ( width >= 6 ) {
		text += std::string(" ");
		high += std::string(" ");
		text += cell.instr ? mpt::fmt::HEX0<2>( cell.instr ) : std::string("..");
		high += cell.instr ? std::string("ii") : std::string("..");
	}
	if ( width >= 9 ) {
		text += cell.IsPcNote() ? std::string(" ") + mpt::fmt::HEX0<2>( cell.GetValueVolCol() & 0xff ) : cell.volcmd != VOLCMD_NONE ? std::string( 1, m_sndFile->GetModSpecifications().GetVolEffectLetter( cell.volcmd ) ) + mpt::fmt::HEX0<2>( cell.vol ) : std::string(" ..");
		high += cell.IsPcNote() ? std::string(" vv") : cell.volcmd != VOLCMD_NONE ? std::string("uvv") : std::string(" ..");
	}
	if ( width >= 13 ) {
		text += std::string(" ");
		high += std::string(" ");
		text += cell.IsPcNote() ? mpt::fmt::HEX0<3>( cell.GetValueEffectCol() & 0x0fff ) : cell.command != CMD_NONE ? std::string( 1, m_sndFile->GetModSpecifications().GetEffectLetter( cell.command ) ) + mpt::fmt::HEX0<2>( cell.param ) : std::string("...");
		high += cell.IsPcNote() ? std::string("eff") : cell.command != CMD_NONE ? std::string("eff") : std::string("...");
	}
	if ( text.length() > width ) {
		text = text.substr( 0, width );
	} else if ( pad ) {
		text += std::string( width - text.length(), ' ' );
	}
	if ( high.length() > width ) {
		high = high.substr( 0, width );
	} else if ( pad ) {
		high += std::string( width - high.length(), ' ' );
	}
	return std::make_pair( text, high );
}
std::string module_impl::format_pattern_row_channel( std::int32_t p, std::int32_t r, std::int32_t c, std::size_t width, bool pad ) const {
	return format_and_highlight_pattern_row_channel( p, r, c, width, pad ).first;
}
std::string module_impl::highlight_pattern_row_channel( std::int32_t p, std::int32_t r, std::int32_t c, std::size_t width, bool pad ) const {
	return format_and_highlight_pattern_row_channel( p, r, c, width, pad ).second;
}

std::vector<std::string> module_impl::get_ctls() const {
	std::vector<std::string> retval;
	retval.push_back( "dither" );
	return retval;
}
std::string module_impl::ctl_get( const std::string & ctl ) const {
	if ( ctl == "" ) {
		throw openmpt::exception("unknown ctl");
	} else if ( ctl == "dither" ) {
		return mpt::ToString( static_cast<int>( m_Dither->GetMode() ) );
	} else {
		throw openmpt::exception("unknown ctl");
	}
}
void module_impl::ctl_set( const std::string & ctl, const std::string & value ) {
	if ( ctl == "" ) {
		throw openmpt::exception("unknown ctl: " + ctl + " := " + value);
	} else if ( ctl == "dither" ) {
		m_Dither->SetMode( static_cast<DitherMode>( ConvertStrTo<int>( value ) ) );
	} else {
		throw openmpt::exception("unknown ctl: " + ctl + " := " + value);
	}
}

} // namespace openmpt
