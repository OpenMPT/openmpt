/*
 * libopenmpt_impl.hpp
 * -------------------
 * Purpose: libopenmpt private interface implementation
 * Notes  : This is not a public header. Do NOT ship in distributions dev packages.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_IMPL_HPP
#define LIBOPENMPT_IMPL_HPP

#include "common/stdafx.h"

#include "libopenmpt_internal.h"
#include "libopenmpt.hpp"
#include "libopenmpt.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <iostream>
#include <iterator>
#include <string>

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "soundlib/Sndfile.h"
#include "soundlib/FileReader.h"

namespace openmpt {

namespace version {

std::uint32_t get_library_version();
std::uint32_t get_core_version();
std::string get_string( const std::string & key );
int get_version_compatbility( std::uint32_t api_version );

} // namespace version

class log_interface : public ILog {
protected:
	log_interface() {
		return;
	}
public:
	virtual ~log_interface() {
		return;
	}
	virtual void log( const std::string & message ) const = 0;
private:
	void AddToLog( LogLevel level, const std::string & text ) const {
		log( LogLevelToString(level) + std::string(": ") + text );
	}
}; // class log_interface

class std_ostream_log : public log_interface {
private:
	mutable std::ostream & destination;
public:
	std_ostream_log( std::ostream & dst ) : destination(dst) {
		return;
	}
	virtual ~std_ostream_log() {
		return;
	}
	virtual void log( const std::string & message ) const {
		destination.flush();
		destination << message << std::endl;
		destination.flush();
	}
}; // class CSoundFileLog_std_ostream

class module_impl {
protected:
	std::shared_ptr<log_interface> m_LogForwarder;
	std::vector<std::int16_t> m_int16Buffer;
	std::vector<float> m_floatBuffer;
	double m_currentPositionSeconds;
protected:
	mutable CSoundFile m_sndFile;
	std::vector<std::pair<LogLevel,std::string> > m_loaderMessages;
	class loader_log : public ILog {
	private:
		module_impl & self;
	public:
		loader_log( module_impl & s ) : self(s) {
			return;
		}
	private:
		void AddToLog( LogLevel level, const std::string & text ) const {
			self.m_loaderMessages.push_back( std::make_pair( level, text ) );
		}
	}; // class loader_log
public:
	void PushToCSoundFileLog( LogLevel level, const std::string & text ) const {
		m_sndFile.AddToLog( level, text );
	}
private:
	static std::int32_t float_to_fx16( float x ) {
		return static_cast<std::int32_t>( x * (1<<16) );
	}
	static float fx16_to_float( std::int32_t x ) {
		return static_cast<float>( x * (1.0f/(1<<16)) );
	}
	template < typename T > T scale_percent( T percent, T min, T max ) {
		return Clamp( min + ( percent * ( max - min ) ) / 100, min, max );
	}
private:
	int32_t get_quality() const {
		return ( m_sndFile.m_MixerSettings.m_nMaxMixChannels - 4 ) * 100 / MAX_CHANNELS;
	}
	void set_quality( std::int32_t value ) {
		MixerSettings mixersettings = m_sndFile.m_MixerSettings;
		mixersettings.m_nMaxMixChannels = scale_percent<int>( value, 4, MAX_CHANNELS );
		CResamplerSettings resamplersettings = m_sndFile.m_Resampler.m_Settings;
		resamplersettings.SrcMode = (ResamplingMode)scale_percent<int>( value, SRCMODE_NEAREST, SRCMODE_FIRFILTER );
		resamplersettings.gdWFIRCutoff = CResamplerSettings().gdWFIRCutoff; // use default
		resamplersettings.gbWFIRType = CResamplerSettings().gbWFIRType; // use default
		m_sndFile.SetMixerSettings( mixersettings );
		m_sndFile.SetResamplerSettings( resamplersettings );
	}
	void apply_mixer_settings( std::int32_t samplerate, int channels, SampleFormat format ) {
		if (
			static_cast<std::int32_t>( m_sndFile.m_MixerSettings.gdwMixingFreq ) != samplerate ||
			static_cast<int>( m_sndFile.m_MixerSettings.gnChannels ) != channels ||
			m_sndFile.m_MixerSettings.m_SampleFormat != format
			) {
			MixerSettings mixersettings = m_sndFile.m_MixerSettings;
			mixersettings.gdwMixingFreq = samplerate;
			mixersettings.gnChannels = channels;
			mixersettings.m_SampleFormat = format;
			m_sndFile.SetMixerSettings( mixersettings );
		}
	}
	void apply_libopenmpt_defaults() {
		set_render_param( module::RENDER_STEREOSEPARATION_PERCENT, 100 );
		set_render_param( module::RENDER_QUALITY_PERCENT, 100 );
	}
	void init() {
		m_sndFile.SetCustomLog( m_LogForwarder.get() );
		m_currentPositionSeconds = 0.0;
	}
	static void load( CSoundFile & sndFile, FileReader file ) {
		if ( !sndFile.Create( file, CSoundFile::loadCompleteModule ) ) {
			throw openmpt::exception("error loading file");
		}
	}
	void load( FileReader file ) {
		loader_log loaderlog(*this);
		m_sndFile.SetCustomLog( &loaderlog );
		load( m_sndFile, file );
		m_sndFile.SetCustomLog( m_LogForwarder.get() );
		for ( std::vector<std::pair<LogLevel,std::string> >::iterator i = m_loaderMessages.begin(); i != m_loaderMessages.end(); ++i ) {
			PushToCSoundFileLog( i->first, i->second );
		}
	}
public:
	static double could_open_propability( std::istream & stream, double effort, std::shared_ptr<log_interface> log ) {
		CSoundFile sndFile;
		sndFile.SetCustomLog( log.get() );

		try {

			if ( effort >= 0.8 ) {
				if ( !sndFile.Create( FileReader( &stream ), CSoundFile::loadCompleteModule ) ) {
					return 0.0;
				}
				sndFile.Destroy();
				return 1.0;
			} else if ( effort >= 0.6 ) {
				if ( !sndFile.Create( FileReader( &stream ), CSoundFile::loadNoPatternData ) ) {
					return 0.0;
				}
				sndFile.Destroy();
				return 0.8;
			} else if ( effort >= 0.2 ) {
				if ( !sndFile.Create( FileReader( &stream ), CSoundFile::onlyVerifyHeader ) ) {
					return 0.0;
				}
				sndFile.Destroy();
				return 0.6;
			} else {
				return 0.2;
			}

		} catch ( ... ) {
			return 0.0;
		}

	}
	module_impl( std::istream & stream, std::shared_ptr<log_interface> log ) : m_LogForwarder(log) {
		init();
		load( FileReader( &stream ) );
		apply_libopenmpt_defaults();
	}
	module_impl( const std::vector<std::uint8_t> & data, std::shared_ptr<log_interface> log ) : m_LogForwarder(log) {
		init();
		load( FileReader( data.data(), data.size() ) );
		apply_libopenmpt_defaults();
	}
	module_impl( const std::vector<char> & data, std::shared_ptr<log_interface> log ) : m_LogForwarder(log) {
		init();
		load( FileReader( data.data(), data.size() ) );
		apply_libopenmpt_defaults();
	}
	module_impl( const std::uint8_t * data, std::size_t size, std::shared_ptr<log_interface> log ) : m_LogForwarder(log) {
		init();
		load( FileReader( data, size ) );
		apply_libopenmpt_defaults();
	}
	module_impl( const char * data, std::size_t size, std::shared_ptr<log_interface> log ) : m_LogForwarder(log) {
		init();
		load( FileReader( data, size ) );
		apply_libopenmpt_defaults();
	}
	module_impl( const void * data, std::size_t size, std::shared_ptr<log_interface> log ) : m_LogForwarder(log) {
		init();
		load( FileReader( data, size ) );
		apply_libopenmpt_defaults();
	}
	~module_impl() {
		m_sndFile.Destroy();
	}
public:
	std::int32_t get_render_param( int command ) const {
		switch ( command ) {
			case module::RENDER_MASTERGAIN_DB: {
				return static_cast<std::int32_t>( 10.0f * 2.0f * std::log10( fx16_to_float( m_sndFile.m_MixerSettings.m_FinalOutputGain ) ) );
			} break;
			case module::RENDER_STEREOSEPARATION_PERCENT: {
				return m_sndFile.m_MixerSettings.m_nStereoSeparation * 100 / 128;
			} break;
			case module::RENDER_REPEATCOUNT: {
				return m_sndFile.m_nRepeatCount;
			} break;
			case module::RENDER_QUALITY_PERCENT: {
				return get_quality();
			} break;
			case module::RENDER_MAXMIXCHANNELS: {
				return m_sndFile.m_MixerSettings.m_nMaxMixChannels;
			} break;
			case module::RENDER_INTERPOLATION_MODE: {
				switch ( m_sndFile.m_Resampler.m_Settings.SrcMode ) {
					case SRCMODE_NEAREST: return module::INTERPOLATION_NEAREST; break;
					case SRCMODE_LINEAR: return module::INTERPOLATION_LINEAR; break;
					case SRCMODE_SPLINE: return module::INTERPOLATION_SPLINE; break;
					case SRCMODE_POLYPHASE: return module::INTERPOLATION_POLYPHASE; break;
					case SRCMODE_DEFAULT:
					case SRCMODE_FIRFILTER: {
						switch ( m_sndFile.m_Resampler.m_Settings.gbWFIRType ) {
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
					case NUM_SRC_MODES:
					default:
					break;
				}
				throw exception("unknown interpolation mode set internally");
			} break;
			case module::RENDER_VOLUMERAMP_IN_SAMPLES: {
				return m_sndFile.m_MixerSettings.glVolumeRampUpSamples;
			} break;
			case module::RENDER_VOLUMERAMP_OUT_SAMPLES: {
				return m_sndFile.m_MixerSettings.glVolumeRampDownSamples;
			} break;
			default: throw exception("unknown command"); break;
		}
		return 0;
	}
	void set_render_param( int command, std::int32_t value ) {
		switch ( command ) {
			case module::RENDER_MASTERGAIN_DB: {
				float gainFactor = static_cast<float>( std::pow( 10.0f, value * 0.1f * 0.5f ) );
				if ( static_cast<std::int32_t>( m_sndFile.m_MixerSettings.m_FinalOutputGain ) != float_to_fx16( gainFactor ) ) {
					MixerSettings settings = m_sndFile.m_MixerSettings;
					settings.m_FinalOutputGain = float_to_fx16( gainFactor );
					m_sndFile.SetMixerSettings( settings );
				}
			} break;
			case module::RENDER_STEREOSEPARATION_PERCENT: {
				std::int32_t newvalue = value * 128 / 100;
				if ( newvalue != static_cast<std::int32_t>( m_sndFile.m_MixerSettings.m_nStereoSeparation ) ) {
					MixerSettings settings = m_sndFile.m_MixerSettings;
					settings.gdwMixingFreq = newvalue;
					m_sndFile.SetMixerSettings( settings );
				}
			} break;
			case module::RENDER_REPEATCOUNT: {
				m_sndFile.SetRepeatCount( value );
			} break;
			case module::RENDER_QUALITY_PERCENT: {
				set_quality( value );
			} break;
			case module::RENDER_MAXMIXCHANNELS: {
				if ( value != static_cast<std::int32_t>( m_sndFile.m_MixerSettings.m_nMaxMixChannels ) ) {
					MixerSettings settings = m_sndFile.m_MixerSettings;
					settings.m_nMaxMixChannels = value;
					m_sndFile.SetMixerSettings( settings );
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
				if ( newsettings != m_sndFile.m_Resampler.m_Settings ) {
					m_sndFile.SetResamplerSettings( newsettings );
				}
			} break;
			case module::RENDER_VOLUMERAMP_IN_SAMPLES: {
				if ( m_sndFile.m_MixerSettings.glVolumeRampUpSamples != value ) {
					MixerSettings settings = m_sndFile.m_MixerSettings;
					settings.glVolumeRampUpSamples = value;
					m_sndFile.SetMixerSettings( settings );
				}
			} break;
			case module::RENDER_VOLUMERAMP_OUT_SAMPLES: {
				if ( m_sndFile.m_MixerSettings.glVolumeRampDownSamples != value ) {
					MixerSettings settings = m_sndFile.m_MixerSettings;
					settings.glVolumeRampDownSamples = value;
					m_sndFile.SetMixerSettings( settings );
				}
			} break;
			default: throw exception("unknown command"); break;
		}
	}
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * mono ) {
		if ( !mono ) {
			throw exception("null pointer");
		}
		apply_mixer_settings( samplerate, 1, SampleFormatInt16 );
		m_int16Buffer.resize( count * 1 );
		count = m_sndFile.Read( &m_int16Buffer[0], count );
		for ( std::size_t i = 0; i < count; ++i ) {
			mono[i] = m_int16Buffer[i*1];
		}
		m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
		return count;
	}
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right ) {
		if ( !left || !right ) {
			throw exception("null pointer");
		}
		apply_mixer_settings( samplerate, 2, SampleFormatInt16 );
		m_int16Buffer.resize( count * 2 );
		count = m_sndFile.Read( &m_int16Buffer[0], count );
		for ( std::size_t i = 0; i < count; ++i ) {
			left[i] = m_int16Buffer[i*2+0];
			right[i] = m_int16Buffer[i*2+1];
		}
		m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
		return count;
	}
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right, std::int16_t * back_left, std::int16_t * back_right ) {
		if ( !left || !right || !back_left || !back_right ) {
			throw exception("null pointer");
		}
		apply_mixer_settings( samplerate, 4, SampleFormatInt16 );
		m_int16Buffer.resize( count * 4 );
		count = m_sndFile.Read( &m_int16Buffer[0], count );
		for ( std::size_t i = 0; i < count; ++i ) {
			left[i] = m_int16Buffer[i*4+0];
			right[i] = m_int16Buffer[i*4+1];
			back_left[i] = m_int16Buffer[i*4+2];
			back_right[i] = m_int16Buffer[i*4+3];
		}
		m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
		return count;
	}
	std::size_t read( std::int32_t samplerate, std::size_t count, float * mono ) {
		if ( !mono ) {
			throw exception("null pointer");
		}
		apply_mixer_settings( samplerate, 1, SampleFormatFloat32 );
		m_floatBuffer.resize( count * 1 );
		count = m_sndFile.Read( &m_floatBuffer[0], count );
		for ( std::size_t i = 0; i < count; ++i ) {
			mono[i] = m_floatBuffer[i*1];
		}
		m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
		return count;
	}
	std::size_t read( std::int32_t samplerate, std::size_t count, float * left, float * right ) {
		if ( !left || !right ) {
			throw exception("null pointer");
		}
		apply_mixer_settings( samplerate, 2, SampleFormatFloat32 );
		m_floatBuffer.resize( count * 2 );
		count = m_sndFile.Read( &m_floatBuffer[0], count );
		for ( std::size_t i = 0; i < count; ++i ) {
			left[i] = m_floatBuffer[i*2+0];
			right[i] = m_floatBuffer[i*2+1];
		}
		m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
		return count;
	}
	std::size_t read( std::int32_t samplerate, std::size_t count, float * left, float * right, float * back_left, float * back_right ) {
		if ( !left || !right || !back_left || !back_right ) {
			throw exception("null pointer");
		}
		apply_mixer_settings( samplerate, 4, SampleFormatFloat32 );
		m_floatBuffer.resize( count * 4 );
		count = m_sndFile.Read( &m_floatBuffer[0], count );
		for ( std::size_t i = 0; i < count; ++i ) {
			left[i] = m_floatBuffer[i*4+0];
			right[i] = m_floatBuffer[i*4+1];
			back_left[i] = m_floatBuffer[i*4+2];
			back_right[i] = m_floatBuffer[i*4+3];
		}
		m_currentPositionSeconds += static_cast<double>( count ) / static_cast<double>( samplerate );
		return count;
	}
	double get_duration_seconds() const {
		return m_sndFile.GetLength( eNoAdjust ).duration;
	}
	double get_current_position_seconds() const {
		return m_currentPositionSeconds;
	}
	void select_subsong( std::int32_t subsong ) {
		if ( subsong < -1 || subsong >= m_sndFile.Order.GetNumSequences() ) {
			return;
		}
		if ( subsong == -1 ) {
			// default subsong
			m_sndFile.Order.SetSequence( 0 );
			return;
		}
		m_sndFile.Order.SetSequence( subsong );
	}
	double seek_seconds( double seconds ) {
		GetLengthType t = m_sndFile.GetLength( eNoAdjust, GetLengthTarget( seconds ) );
		m_sndFile.InitializeVisitedRows();
		m_sndFile.m_nCurrentOrder = t.lastOrder;
		m_sndFile.SetCurrentOrder( t.lastOrder );
		m_sndFile.m_nNextRow = t.lastRow;
		m_currentPositionSeconds = m_sndFile.GetLength( eAdjust, GetLengthTarget( t.lastOrder, t.lastRow ) ).duration;
		return m_currentPositionSeconds;
	}
	std::vector<std::string> get_metadata_keys() const {
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
	std::string get_metadata( const std::string & key ) const {
		if ( key == std::string("type") ) {
			return CSoundFile::ModTypeToString( m_sndFile.GetType() );
		} else if ( key == std::string("type_long") ) {
			return CSoundFile::ModTypeToTracker( m_sndFile.GetType() );
		} else if ( key == std::string("tracker") ) {
			return m_sndFile.madeWithTracker;
		} else if ( key == std::string("title") ) {
			return m_sndFile.GetTitle();
		} else if ( key == std::string("message") ) {
			std::string retval = m_sndFile.songMessage;
			if ( retval.empty() ) {
				std::ostringstream tmp;
				bool valid = false;
				for ( INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); ++i ) {
					std::string instname = m_sndFile.GetInstrumentName( i );
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
				for ( SAMPLEINDEX i = 1; i <= m_sndFile.GetNumSamples(); ++i ) {
					std::string samplename = m_sndFile.GetSampleName( i );
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
			for ( std::vector<std::pair<LogLevel,std::string> >::const_iterator i = m_loaderMessages.begin(); i != m_loaderMessages.end(); ++i ) {
				if ( !first ) {
					retval << std::endl;
					first = false;
				}
				retval << LogLevelToString( i->first ) << std::string(": ") << i->second;
			}
			return retval.str();
		}
		return "";
	}
	std::int32_t get_current_speed() const {
		return m_sndFile.m_nMusicSpeed;
	}
	std::int32_t get_current_tempo() const {
		return m_sndFile.m_nMusicTempo;
	}
	std::int32_t get_current_order() const {
		return m_sndFile.GetCurrentOrder();
	}
	std::int32_t get_current_pattern() const {
		return m_sndFile.GetCurrentPattern();
	}
	std::int32_t get_current_row() const {
		return m_sndFile.m_nRow;
	}
	std::int32_t get_current_playing_channels() const {
		return m_sndFile.m_nMixChannels;
	}
	std::int32_t get_num_subsongs() const {
		return m_sndFile.Order.GetNumSequences();
	}
	std::int32_t get_num_channels() const {
		return m_sndFile.GetNumChannels();
	}
	std::int32_t get_num_orders() const {
		return m_sndFile.Order.GetLengthTailTrimmed();
	}
	std::int32_t get_num_patterns() const {
		return m_sndFile.Patterns.GetNumPatterns();
	}
	std::int32_t get_num_instruments() const {
		return m_sndFile.GetNumInstruments();
	}
	std::int32_t get_num_samples() const {
		return m_sndFile.GetNumSamples();
	}

	std::vector<std::string> get_subsong_names() const {
		std::vector<std::string> retval;
		for ( SEQUENCEINDEX i = 0; i < m_sndFile.Order.GetNumSequences(); ++i ) {
			retval.push_back( m_sndFile.Order.GetSequence( i ).m_sName );
		}
		return retval;
	}
	std::vector<std::string> get_channel_names() const {
		std::vector<std::string> retval;
		for ( CHANNELINDEX i = 0; i < m_sndFile.GetNumChannels(); ++i ) {
			retval.push_back( m_sndFile.ChnSettings[i].szName );
		}
		return retval;
	}
	std::vector<std::string> get_order_names() const {
		std::vector<std::string> retval;
		for ( ORDERINDEX i = 0; i < m_sndFile.Order.GetLengthTailTrimmed(); ++i ) {
			PATTERNINDEX pat = m_sndFile.Order[i];
			if ( m_sndFile.Patterns.IsValidIndex( pat ) ) {
				retval.push_back( m_sndFile.Patterns[ m_sndFile.Order[i] ].GetName() );
			} else {
				if ( pat == m_sndFile.Order.GetIgnoreIndex() ) {
					retval.push_back( "+++ skip" );
				} else if ( pat == m_sndFile.Order.GetInvalidPatIndex() ) {
					retval.push_back( "--- stop" );
				} else {
					retval.push_back( "???" );
				}
			}
		}
		return retval;
	}
	std::vector<std::string> get_pattern_names() const {
		std::vector<std::string> retval;
		for ( PATTERNINDEX i = 0; i < m_sndFile.Patterns.GetNumPatterns(); ++i ) {
			retval.push_back( m_sndFile.Patterns[i].GetName() );
		}
		return retval;
	}
	std::vector<std::string> get_instrument_names() const {
		std::vector<std::string> retval;
		for ( INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); ++i ) {
			retval.push_back( m_sndFile.GetInstrumentName( i ) );
		}
		return retval;
	}
	std::vector<std::string> get_sample_names() const {
		std::vector<std::string> retval;
		for ( SAMPLEINDEX i = 1; i <= m_sndFile.GetNumSamples(); ++i ) {
			retval.push_back( m_sndFile.GetSampleName( i ) );
		}
		return retval;
	}

	std::int32_t get_order_pattern( std::int32_t o ) const {
		if ( o < 0 || o >= m_sndFile.Order.GetLengthTailTrimmed() ) {
			return -1;
		}
		return m_sndFile.Order[o];
	}
	std::int32_t get_pattern_num_rows( std::int32_t p ) const {
		if ( p < 0 || p >= m_sndFile.Patterns.Size() ) {
			return 0;
		}
		return m_sndFile.Patterns[p].GetNumRows();
	}

	std::uint8_t get_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const {
		CHANNELINDEX numchannels = m_sndFile.GetNumChannels();
		if ( p < 0 || p >= m_sndFile.Patterns.Size() ) {
			return 0;
		}
		if ( r < 0 || r >= (std::int32_t)m_sndFile.Patterns[p].GetNumRows() ) {
			return 0;
		}
		if ( c < 0 || c >= numchannels ) {
			return 0;
		}
		if ( cmd < module::command_note || cmd > module::command_parameter ) {
			return 0;
		}
		switch ( cmd ) {
			case module::command_note: return m_sndFile.Patterns[p][r*numchannels+c].note; break;
			case module::command_instrument: return m_sndFile.Patterns[p][r*numchannels+c].instr; break;
			case module::command_volumeffect: return m_sndFile.Patterns[p][r*numchannels+c].volcmd; break;
			case module::command_effect: return m_sndFile.Patterns[p][r*numchannels+c].command; break;
			case module::command_volume: return m_sndFile.Patterns[p][r*numchannels+c].vol; break;
			case module::command_parameter: return m_sndFile.Patterns[p][r*numchannels+c].param; break;
		}
		return 0;
	}

}; // class module_impl

} // namespace openmpt

#endif // LIBOPENMPT_IMPL_HPP
