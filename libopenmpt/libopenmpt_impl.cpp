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

#include "soundlib/Sndfile.h"
#include "soundlib/FileReader.h"

namespace openmpt {

exception_message::exception_message( const char * text_ ) throw() : text(text_) {
	return;
}
exception_message::~exception_message() throw() {
	return;
}
const char * exception_message::what() const throw() {
	return text;
}

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

static std::int32_t float_to_fx16( float x ) {
	return static_cast<std::int32_t>( x * (1<<16) );
}
static float fx16_to_float( std::int32_t x ) {
	return static_cast<float>( x * (1.0f/(1<<16)) );
}
template < typename T > static T scale_percent( T percent, T min, T max ) {
	return Clamp( min + ( percent * ( max - min ) ) / 100, min, max );
}
template < typename T > static T unscale_percent( T value, T min, T max ) {
	return Clamp( ( value - min ) * 100 / ( max - min ), 0, 100 );
}

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

std::int32_t module_impl::get_quality() const {
	return unscale_percent<int>( m_sndFile->m_Resampler.m_Settings.SrcMode, SRCMODE_NEAREST, SRCMODE_FIRFILTER );
}
void module_impl::set_quality( std::int32_t value ) {
	CResamplerSettings resamplersettings = m_sndFile->m_Resampler.m_Settings;
	resamplersettings.SrcMode = (ResamplingMode)scale_percent<int>( value, SRCMODE_NEAREST, SRCMODE_FIRFILTER );
	resamplersettings.gdWFIRCutoff = CResamplerSettings().gdWFIRCutoff; // use default
	resamplersettings.gbWFIRType = CResamplerSettings().gbWFIRType; // use default
	m_sndFile->SetResamplerSettings( resamplersettings );
}
void module_impl::apply_mixer_settings( std::int32_t samplerate, int channels, bool format_float ) {
	SampleFormat format = ( format_float ? SampleFormatFloat32 : SampleFormatInt16 );
	if (
		static_cast<std::int32_t>( m_sndFile->m_MixerSettings.gdwMixingFreq ) != samplerate ||
		static_cast<int>( m_sndFile->m_MixerSettings.gnChannels ) != channels ||
		m_sndFile->m_MixerSettings.m_SampleFormat != format
		) {
		MixerSettings mixersettings = m_sndFile->m_MixerSettings;
		std::int32_t volrampin_us = mixersettings.GetVolumeRampUpMicroseconds();
		std::int32_t volrampout_us = mixersettings.GetVolumeRampDownMicroseconds();
		mixersettings.gdwMixingFreq = samplerate;
		mixersettings.gnChannels = channels;
		mixersettings.m_SampleFormat = format;
		mixersettings.SetVolumeRampUpMicroseconds( volrampin_us );
		mixersettings.SetVolumeRampDownMicroseconds( volrampout_us );
		m_sndFile->SetMixerSettings( mixersettings );
	}
}
void module_impl::apply_libopenmpt_defaults() {
	set_render_param( module::RENDER_STEREOSEPARATION_PERCENT, 100 );
	set_render_param( module::RENDER_QUALITY_PERCENT, 100 );
}
void module_impl::init() {
	m_sndFile = std::unique_ptr<CSoundFile>(new CSoundFile());
	m_LogForwarder = std::unique_ptr<log_forwarder>(new log_forwarder(m_Log));
	m_sndFile->SetCustomLog( m_LogForwarder.get() );
	m_currentPositionSeconds = 0.0;
}
void module_impl::load( CSoundFile & sndFile, const FileReader & file ) {
	if ( !sndFile.Create( file, CSoundFile::loadCompleteModule ) ) {
		throw openmpt::exception_message("error loading file");
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
	std::size_t count_read = 0;
	while ( count > 0 ) {
		std::int16_t * const buffers[4] = { left + count_read, right + count_read, rear_left + count_read, rear_right + count_read };
		std::size_t count_chunk = m_sndFile->ReadNonInterleaved(
			reinterpret_cast<void*const*>( buffers ),
			static_cast<CSoundFile::samplecount_t>( std::min<std::uint64_t>( count, std::numeric_limits<CSoundFile::samplecount_t>::max() / 2 / 4 / 4 ) ) // safety margin / samplesize / channels
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
	std::size_t count_read = 0;
	while ( count > 0 ) {
		float * const buffers[4] = { left + count_read, right + count_read, rear_left + count_read, rear_right + count_read };
		std::size_t count_chunk = m_sndFile->ReadNonInterleaved(
			reinterpret_cast<void*const*>( buffers ),
			static_cast<CSoundFile::samplecount_t>( std::min<std::uint64_t>( count, std::numeric_limits<CSoundFile::samplecount_t>::max() / 2 / 4 / 4 ) ) // safety margin / samplesize / channels
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
			if ( !sndFile->Create( FileReader( &stream ), CSoundFile::loadNoPatternData ) ) {
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

module_impl::module_impl( std::istream & stream, std::shared_ptr<log_interface> log ) : m_Log(log) {
	init();
	load( FileReader( &stream ) );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const std::vector<std::uint8_t> & data, std::shared_ptr<log_interface> log ) : m_Log(log) {
	init();
	load( FileReader( data.data(), data.size() ) );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const std::vector<char> & data, std::shared_ptr<log_interface> log ) : m_Log(log) {
	init();
	load( FileReader( data.data(), data.size() ) );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const std::uint8_t * data, std::size_t size, std::shared_ptr<log_interface> log ) : m_Log(log) {
	init();
	load( FileReader( data, size ) );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const char * data, std::size_t size, std::shared_ptr<log_interface> log ) : m_Log(log) {
	init();
	load( FileReader( data, size ) );
	apply_libopenmpt_defaults();
}
module_impl::module_impl( const void * data, std::size_t size, std::shared_ptr<log_interface> log ) : m_Log(log) {
	init();
	load( FileReader( data, size ) );
	apply_libopenmpt_defaults();
}
module_impl::~module_impl() {
	m_sndFile->Destroy();
}

std::int32_t module_impl::get_render_param( int command ) const {
	switch ( command ) {
		case module::RENDER_MASTERGAIN_MILLIBEL: {
			return static_cast<std::int32_t>( 1000.0f * 2.0f * std::log10( fx16_to_float( m_sndFile->m_MixerSettings.m_FinalOutputGain ) ) );
		} break;
		case module::RENDER_STEREOSEPARATION_PERCENT: {
			return m_sndFile->m_MixerSettings.m_nStereoSeparation * 100 / 128;
		} break;
		case module::RENDER_REPEATCOUNT: {
			return m_sndFile->m_nRepeatCount;
		} break;
		case module::RENDER_QUALITY_PERCENT: {
			return get_quality();
		} break;
		case module::RENDER_MAXMIXCHANNELS: {
			return m_sndFile->m_MixerSettings.m_nMaxMixChannels;
		} break;
		case module::RENDER_INTERPOLATION_MODE: {
			switch ( m_sndFile->m_Resampler.m_Settings.SrcMode ) {
				case SRCMODE_NEAREST: return module::INTERPOLATION_NEAREST; break;
				case SRCMODE_LINEAR: return module::INTERPOLATION_LINEAR; break;
				case SRCMODE_SPLINE: return module::INTERPOLATION_SPLINE; break;
				case SRCMODE_POLYPHASE: return module::INTERPOLATION_POLYPHASE; break;
				case SRCMODE_DEFAULT:
				case SRCMODE_FIRFILTER: {
					switch ( m_sndFile->m_Resampler.m_Settings.gbWFIRType ) {
						case WFIR_HANN: return module::INTERPOLATION_FIR_HANN; break;
						case WFIR_HAMMING: return module::INTERPOLATION_FIR_HAMMING; break;
						case WFIR_BLACKMANEXACT: return module::INTERPOLATION_FIR_BLACKMANEXACT; break;
						case WFIR_BLACKMAN3T61: return module::INTERPOLATION_FIR_BLACKMAN3T61; break;
						case WFIR_BLACKMAN3T67: return module::INTERPOLATION_FIR_BLACKMAN3T67; break;
						case WFIR_BLACKMAN4T92: return module::INTERPOLATION_FIR_BLACKMAN4T92; break;
						case WFIR_BLACKMAN4T74: return module::INTERPOLATION_FIR_BLACKMAN4T74; break;
						case WFIR_KAISER4T: return module::INTERPOLATION_FIR_KAISER4T; break;
					}
				} break;
				default:
				break;
			}
			throw openmpt::exception_message("unknown interpolation mode set internally");
		} break;
		case module::RENDER_VOLUMERAMP_UP_MICROSECONDS: {
			return m_sndFile->m_MixerSettings.GetVolumeRampUpMicroseconds();
		} break;
		case module::RENDER_VOLUMERAMP_DOWN_MICROSECONDS: {
			return m_sndFile->m_MixerSettings.GetVolumeRampDownMicroseconds();
		} break;
		default: throw openmpt::exception_message("unknown command"); break;
	}
	return 0;
}
void module_impl::set_render_param( int command, std::int32_t value ) {
	switch ( command ) {
		case module::RENDER_MASTERGAIN_MILLIBEL: {
			float gainFactor = static_cast<float>( std::pow( 10.0f, value * 0.001f * 0.5f ) );
			if ( static_cast<std::int32_t>( m_sndFile->m_MixerSettings.m_FinalOutputGain ) != float_to_fx16( gainFactor ) ) {
				MixerSettings settings = m_sndFile->m_MixerSettings;
				settings.m_FinalOutputGain = float_to_fx16( gainFactor );
				m_sndFile->SetMixerSettings( settings );
			}
		} break;
		case module::RENDER_STEREOSEPARATION_PERCENT: {
			std::int32_t newvalue = value * 128 / 100;
			if ( newvalue != static_cast<std::int32_t>( m_sndFile->m_MixerSettings.m_nStereoSeparation ) ) {
				MixerSettings settings = m_sndFile->m_MixerSettings;
				settings.gdwMixingFreq = newvalue;
				m_sndFile->SetMixerSettings( settings );
			}
		} break;
		case module::RENDER_REPEATCOUNT: {
			m_sndFile->SetRepeatCount( value );
		} break;
		case module::RENDER_QUALITY_PERCENT: {
			set_quality( value );
		} break;
		case module::RENDER_MAXMIXCHANNELS: {
			if ( value != static_cast<std::int32_t>( m_sndFile->m_MixerSettings.m_nMaxMixChannels ) ) {
				MixerSettings settings = m_sndFile->m_MixerSettings;
				settings.m_nMaxMixChannels = value;
				m_sndFile->SetMixerSettings( settings );
			}
		} break;
		case module::RENDER_INTERPOLATION_MODE: {
			CResamplerSettings newsettings;
			switch ( value ) {
				case module::INTERPOLATION_NEAREST: newsettings.SrcMode = SRCMODE_NEAREST; break;
				case module::INTERPOLATION_LINEAR: newsettings.SrcMode = SRCMODE_LINEAR; break;
				case module::INTERPOLATION_SPLINE: newsettings.SrcMode = SRCMODE_SPLINE; break;
				case module::INTERPOLATION_POLYPHASE: newsettings.SrcMode = SRCMODE_POLYPHASE; break;
				case module::INTERPOLATION_FIR_HANN: newsettings.SrcMode = SRCMODE_FIRFILTER; newsettings.gbWFIRType = WFIR_HANN; break;
				case module::INTERPOLATION_FIR_HAMMING: newsettings.SrcMode = SRCMODE_FIRFILTER; newsettings.gbWFIRType = WFIR_HAMMING; break;
				case module::INTERPOLATION_FIR_BLACKMANEXACT: newsettings.SrcMode = SRCMODE_FIRFILTER; newsettings.gbWFIRType = WFIR_BLACKMANEXACT; break;
				case module::INTERPOLATION_FIR_BLACKMAN3T61: newsettings.SrcMode = SRCMODE_FIRFILTER; newsettings.gbWFIRType = WFIR_BLACKMAN3T61; break;
				case module::INTERPOLATION_FIR_BLACKMAN3T67: newsettings.SrcMode = SRCMODE_FIRFILTER; newsettings.gbWFIRType = WFIR_BLACKMAN3T67; break;
				case module::INTERPOLATION_FIR_BLACKMAN4T92: newsettings.SrcMode = SRCMODE_FIRFILTER; newsettings.gbWFIRType = WFIR_BLACKMAN4T92; break;
				case module::INTERPOLATION_FIR_BLACKMAN4T74: newsettings.SrcMode = SRCMODE_FIRFILTER; newsettings.gbWFIRType = WFIR_BLACKMAN4T74; break;
				case module::INTERPOLATION_FIR_KAISER4T: newsettings.SrcMode = SRCMODE_FIRFILTER; newsettings.gbWFIRType = WFIR_KAISER4T; break;
			}
			if ( newsettings != m_sndFile->m_Resampler.m_Settings ) {
				m_sndFile->SetResamplerSettings( newsettings );
			}
		} break;
		case module::RENDER_VOLUMERAMP_UP_MICROSECONDS: {
			MixerSettings newsettings = m_sndFile->m_MixerSettings;
			newsettings.SetVolumeRampUpMicroseconds( value );
			if ( m_sndFile->m_MixerSettings.glVolumeRampUpSamples != newsettings.glVolumeRampUpSamples ) {
				m_sndFile->SetMixerSettings( newsettings );
			}
		} break;
		case module::RENDER_VOLUMERAMP_DOWN_MICROSECONDS: {
			MixerSettings newsettings = m_sndFile->m_MixerSettings;
			newsettings.SetVolumeRampDownMicroseconds( value );
			if ( m_sndFile->m_MixerSettings.glVolumeRampDownSamples != newsettings.glVolumeRampDownSamples ) {
				m_sndFile->SetMixerSettings( newsettings );
			}
		} break;
		default: throw openmpt::exception_message("unknown command"); break;
	}
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, std::int16_t * mono ) {
	if ( !mono ) {
		throw openmpt::exception_message("null pointer");
	}
	apply_mixer_settings( samplerate, 1, false );
	count = read_wrapper( count, mono, 0, 0, 0 );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right ) {
	if ( !left || !right ) {
		throw openmpt::exception_message("null pointer");
	}
	apply_mixer_settings( samplerate, 2, false );
	count = read_wrapper( count, left, right, 0, 0 );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right, std::int16_t * rear_left, std::int16_t * rear_right ) {
	if ( !left || !right || !rear_left || !rear_right ) {
		throw openmpt::exception_message("null pointer");
	}
	apply_mixer_settings( samplerate, 4, false );
	count = read_wrapper( count, left, right, rear_left, rear_right );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, float * mono ) {
	if ( !mono ) {
		throw openmpt::exception_message("null pointer");
	}
	apply_mixer_settings( samplerate, 1, true );
	count = read_wrapper( count, mono, 0, 0, 0 );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, float * left, float * right ) {
	if ( !left || !right ) {
		throw openmpt::exception_message("null pointer");
	}
	apply_mixer_settings( samplerate, 2, true );
	count = read_wrapper( count, left, right, 0, 0 );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
std::size_t module_impl::read( std::int32_t samplerate, std::size_t count, float * left, float * right, float * rear_left, float * rear_right ) {
	if ( !left || !right || !rear_left || !rear_right ) {
		throw openmpt::exception_message("null pointer");
	}
	apply_mixer_settings( samplerate, 4, true );
	count = read_wrapper( count, left, right, rear_left, rear_right );
	m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
	return count;
}
double module_impl::get_duration_seconds() const {
	return m_sndFile->GetLength( eNoAdjust ).duration;
}
double module_impl::get_current_position_seconds() const {
	return m_currentPositionSeconds;
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
double module_impl::seek_seconds( double seconds ) {
	GetLengthType t = m_sndFile->GetLength( eNoAdjust, GetLengthTarget( seconds ) );
	m_sndFile->InitializeVisitedRows();
	m_sndFile->m_nCurrentOrder = t.lastOrder;
	m_sndFile->SetCurrentOrder( t.lastOrder );
	m_sndFile->m_nNextRow = t.lastRow;
	m_currentPositionSeconds = m_sndFile->GetLength( eAdjust, GetLengthTarget( t.lastOrder, t.lastRow ) ).duration;
	return m_currentPositionSeconds;
}
std::vector<std::string> module_impl::get_metadata_keys() const {
	std::vector<std::string> retval;
	retval.push_back("type");
	retval.push_back("type_long");
	retval.push_back("tracker");
	retval.push_back("author");
	retval.push_back("title");
	retval.push_back("message");
	retval.push_back("warnings");
	return retval;
}
std::string module_impl::get_metadata( const std::string & key ) const {
	if ( key == std::string("type") ) {
		return CSoundFile::ModTypeToString( m_sndFile->GetType() );
	} else if ( key == std::string("type_long") ) {
		return CSoundFile::ModTypeToTracker( m_sndFile->GetType() );
	} else if ( key == std::string("tracker") ) {
		return m_sndFile->madeWithTracker;
	} else if ( key == std::string("title") ) {
		return m_sndFile->GetTitle();
	} else if ( key == std::string("message") ) {
		std::string retval = m_sndFile->songMessage.GetFormatted( SongMessage::leLF );
		if ( retval.empty() ) {
			std::ostringstream tmp;
			bool valid = false;
			for ( INSTRUMENTINDEX i = 1; i <= m_sndFile->GetNumInstruments(); ++i ) {
				std::string instname = m_sndFile->GetInstrumentName( i );
				if ( !instname.empty() ) {
					valid = true;
				}
				tmp << instname << std::endl;
			}
			if ( valid ) {
				retval = tmp.str();
			}
		}
		if ( retval.empty() ) {
			std::ostringstream tmp;
			bool valid = false;
			for ( SAMPLEINDEX i = 1; i <= m_sndFile->GetNumSamples(); ++i ) {
				std::string samplename = m_sndFile->GetSampleName( i );
				if ( !samplename.empty() ) {
					valid = true;
				}
				tmp << samplename << std::endl;
			}
			if ( valid ) {
				retval = tmp.str();
			}
		}
		return retval;
	} else if ( key == std::string("warnings") ) {
		std::ostringstream retval;
		bool first = true;
		for ( std::vector<std::string>::const_iterator i = m_loaderMessages.begin(); i != m_loaderMessages.end(); ++i ) {
			if ( !first ) {
				retval << std::endl;
				first = false;
			}
			retval << *i;
		}
		return retval.str();
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
	return m_sndFile->GetCurrentPattern();
}
std::int32_t module_impl::get_current_row() const {
	return m_sndFile->m_nRow;
}
std::int32_t module_impl::get_current_playing_channels() const {
	return m_sndFile->m_nMixChannels;
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
		retval.push_back( m_sndFile->Order.GetSequence( i ).m_sName );
	}
	return retval;
}
std::vector<std::string> module_impl::get_channel_names() const {
	std::vector<std::string> retval;
	for ( CHANNELINDEX i = 0; i < m_sndFile->GetNumChannels(); ++i ) {
		retval.push_back( m_sndFile->ChnSettings[i].szName );
	}
	return retval;
}
std::vector<std::string> module_impl::get_order_names() const {
	std::vector<std::string> retval;
	for ( ORDERINDEX i = 0; i < m_sndFile->Order.GetLengthTailTrimmed(); ++i ) {
		PATTERNINDEX pat = m_sndFile->Order[i];
		if ( m_sndFile->Patterns.IsValidIndex( pat ) ) {
			retval.push_back( m_sndFile->Patterns[ m_sndFile->Order[i] ].GetName() );
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
		retval.push_back( m_sndFile->Patterns[i].GetName() );
	}
	return retval;
}
std::vector<std::string> module_impl::get_instrument_names() const {
	std::vector<std::string> retval;
	for ( INSTRUMENTINDEX i = 1; i <= m_sndFile->GetNumInstruments(); ++i ) {
		retval.push_back( m_sndFile->GetInstrumentName( i ) );
	}
	return retval;
}
std::vector<std::string> module_impl::get_sample_names() const {
	std::vector<std::string> retval;
	for ( SAMPLEINDEX i = 1; i <= m_sndFile->GetNumSamples(); ++i ) {
		retval.push_back( m_sndFile->GetSampleName( i ) );
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

} // namespace openmpt
