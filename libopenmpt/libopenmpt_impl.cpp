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
#include <istream>
#include <iterator>
#include <limits>
#include <ostream>

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "common/version.h"
#include "common/misc_util.h"
#include "common/FileReader.h"
#include "common/Logging.h"
#include "soundlib/Sndfile.h"
#include "soundlib/mod_specifications.h"
#include "soundlib/AudioReadTarget.h"

OPENMPT_NAMESPACE_BEGIN

#if defined(MPT_ASSERT_HANDLER_NEEDED) && !defined(ENABLE_TESTS)

MPT_NOINLINE void AssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg)
//------------------------------------------------------------------------------------------------------------------
{
	if(msg)
	{
		mpt::log::Logger().SendLogMessage(mpt::log::Context(file, line, function), LogError, "ASSERT",
			MPT_USTRING("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::CharsetASCII, msg) + MPT_USTRING(" (") + mpt::ToUnicode(mpt::CharsetASCII, expr) + MPT_USTRING(")")
			);
	} else
	{
		mpt::log::Logger().SendLogMessage(mpt::log::Context(file, line, function), LogError, "ASSERT",
			MPT_USTRING("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::CharsetASCII, expr)
			);
	}
}

#endif // MPT_ASSERT_HANDLER_NEEDED && !ENABLE_TESTS

OPENMPT_NAMESPACE_END

using namespace OpenMPT;

namespace openmpt {

namespace version {

std::uint32_t get_library_version() {
	const MptVersion::SourceInfo sourceInfo = MptVersion::GetSourceInfo();
	return OPENMPT_API_VERSION | ( sourceInfo.Revision & 0xffff );
}

std::uint32_t get_core_version() {
	return MptVersion::num;
}

static std::string get_library_version_string() {
	std::string str;
	const MptVersion::SourceInfo sourceInfo = MptVersion::GetSourceInfo();
	std::uint32_t version = get_library_version();
	if ( ( version & 0xffff ) == 0 ) {
		str += mpt::ToString((version>>24) & 0xff);
		str += ".";
		str += mpt::ToString((version>>16) & 0xff);
	} else {
		str += mpt::ToString((version>>24) & 0xff);
		str += ".";
		str += mpt::ToString((version>>16) & 0xff);
		str += ".";
		str += mpt::ToString((version>>0) & 0xffff);
	}
	if ( sourceInfo.IsDirty ) {
		str += ".2-modified";
		if ( sourceInfo.IsPackage ) {
			str += "-pkg";
		}
	} else if ( sourceInfo.HasMixedRevisions ) {
		str += ".1-modified";
		if ( sourceInfo.IsPackage ) {
			str += "-pkg";
		}
	} else if ( sourceInfo.IsPackage ) {
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

static std::string get_source_url_string() {
	return MptVersion::GetSourceInfo().GetUrlWithRevision();
}

static std::string get_source_date_string() {
	return MptVersion::GetSourceInfo().Date;
}

static std::string get_build_string() {
	return MptVersion::GetBuildDateString();
}

static std::string get_build_compiler_string() {
	return MptVersion::GetBuildCompilerString();
}

static std::string get_credits_string() {
	return mpt::ToCharset(mpt::CharsetUTF8, MptVersion::GetFullCreditsString());
}

static std::string get_contact_string() {
	return mpt::ToCharset(mpt::CharsetUTF8, MptVersion::GetContactString());
}

static std::string get_license_string() {
	return mpt::ToCharset(mpt::CharsetUTF8, MptVersion::GetLicenseString());
}

std::string get_string( const std::string & key ) {
	if ( key == "" ) {
		return std::string();
	} else if ( key == "library_version" ) {
		return get_library_version_string();
	} else if ( key == "library_features" ) {
		return get_library_features_string();
	} else if ( key == "core_version" ) {
		return get_core_version_string();
	} else if ( key == "source_url" ) {
		return get_source_url_string();
	} else if ( key == "source_date" ) {
		return get_source_date_string();
	} else if ( key == "build" ) {
		return get_build_string();
	} else if ( key == "build_compiler" ) {
		return get_build_compiler_string();
	} else if ( key == "credits" ) {
		return get_credits_string();
	} else if ( key == "contact" ) {
		return get_contact_string();
	} else if ( key == "license" ) {
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
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
	LIBOPENMPT_SHARED_PTR<log_interface> destination;
#else
	std::shared_ptr<log_interface> destination;
#endif
public:
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
	log_forwarder( LIBOPENMPT_SHARED_PTR<log_interface> dest ) : destination(dest) {
#else
	log_forwarder( std::shared_ptr<log_interface> dest ) : destination(dest) {
#endif
		return;
	}
	virtual ~log_forwarder() {
		return;
	}
private:
	void AddToLog( LogLevel level, const mpt::ustring & text ) const {
		destination->log( mpt::ToCharset( mpt::CharsetUTF8, LogLevelToString( level ) + MPT_USTRING(": ") + text ) );
	}
}; // class log_forwarder

class loader_log : public ILog {
private:
	mutable std::vector<std::pair<LogLevel,std::string> > m_Messages;
public:
	std::vector<std::pair<LogLevel,std::string> > GetMessages() const;
private:
	void AddToLog( LogLevel level, const mpt::ustring & text ) const;
}; // class loader_log

std::vector<std::pair<LogLevel,std::string> > loader_log::GetMessages() const {
	return m_Messages;
}
void loader_log::AddToLog( LogLevel level, const mpt::ustring & text ) const {
	m_Messages.push_back( std::make_pair( level, mpt::ToCharset( mpt::CharsetUTF8, text ) ) );
}

void module_impl::PushToCSoundFileLog( const std::string & text ) const {
	m_sndFile->AddToLog( LogError, mpt::ToUnicode( mpt::CharsetUTF8, text ) );
}
void module_impl::PushToCSoundFileLog( int loglevel, const std::string & text ) const {
	m_sndFile->AddToLog( static_cast<LogLevel>( loglevel ), mpt::ToUnicode( mpt::CharsetUTF8, text ) );
}

module_impl::subsong_data::subsong_data( double duration, std::int32_t start_row, std::int32_t start_order, std::int32_t sequence )
	: duration(duration)
	, start_row(start_row)
	, start_order(start_order)
	, sequence(sequence)
{
	return;
}

static ResamplingMode filterlength_to_resamplingmode(std::int32_t length) {
	ResamplingMode result = SRCMODE_POLYPHASE;
	if ( length == 0 ) {
		result = SRCMODE_POLYPHASE;
	} else if ( length >= 8 ) {
		result = SRCMODE_POLYPHASE;
	} else if ( length >= 3 ) {
		result = SRCMODE_SPLINE;
	} else if ( length >= 2 ) {
		result = SRCMODE_LINEAR;
	} else if ( length >= 1 ) {
		result = SRCMODE_NEAREST;
	} else {
		throw openmpt::exception("negative filter length");
	}
	return result;
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
	if ( ( settings.GetVolumeRampUpMicroseconds() == MixerSettings().GetVolumeRampUpMicroseconds() ) && ( settings.GetVolumeRampDownMicroseconds() == MixerSettings().GetVolumeRampDownMicroseconds() ) ) {
		ramping = -1;
	} else if ( ramp_us <= 0 ) {
		ramping = 0;
	} else {
		ramping = ( ramp_us + 500 ) / 1000;
	}
}

std::string module_impl::mod_string_to_utf8( const std::string & encoded ) const {
	return mpt::ToCharset( mpt::CharsetUTF8, m_sndFile->GetCharset(), encoded );
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
	m_sndFile->Order.SetSequence( 0 );
}
module_impl::subsongs_type module_impl::get_subsongs() const {
	std::vector<subsong_data> subsongs;
	if ( m_sndFile->Order.GetNumSequences() == 0 ) {
		throw openmpt::exception("module contains no songs");
	}
	for ( SEQUENCEINDEX seq = 0; seq < m_sndFile->Order.GetNumSequences(); ++seq ) {
		const std::vector<GetLengthType> lengths = m_sndFile->GetLength( eNoAdjust, GetLengthTarget( true ).StartPos( seq, 0, 0 ) );
		for ( std::vector<GetLengthType>::const_iterator l = lengths.begin(); l != lengths.end(); ++l ) {
			subsongs.push_back( subsong_data( l->duration, l->startRow, l->startOrder, seq ) );
		}
	}
	return subsongs;
}
void module_impl::init_subsongs( subsongs_type & subsongs ) const {
	subsongs = get_subsongs();
}
bool module_impl::has_subsongs_inited() const {
	return !m_subsongs.empty();
}
void module_impl::ctor( const std::map< std::string, std::string > & ctls ) {
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
	m_sndFile = LIBOPENMPT_SHARED_PTR<CSoundFile>(new CSoundFile());
#else
	m_sndFile = std::unique_ptr<CSoundFile>(new CSoundFile());
#endif
	m_loaded = false;
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
	m_Dither = LIBOPENMPT_SHARED_PTR<Dither>(new Dither());
#else
	m_Dither = std::unique_ptr<Dither>(new Dither());
#endif
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
	m_LogForwarder = LIBOPENMPT_SHARED_PTR<log_forwarder>(new log_forwarder(m_Log));
#else
	m_LogForwarder = std::unique_ptr<log_forwarder>(new log_forwarder(m_Log));
#endif
	m_sndFile->SetCustomLog( m_LogForwarder.get() );
	m_current_subsong = 0;
	m_currentPositionSeconds = 0.0;
	m_Gain = 1.0f;
	m_ctl_load_skip_samples = false;
	m_ctl_load_skip_patterns = false;
	m_ctl_load_skip_subsongs_init = false;
	m_ctl_seek_sync_samples = false;
	// init member variables that correspond to ctls
	for ( std::map< std::string, std::string >::const_iterator i = ctls.begin(); i != ctls.end(); ++i ) {
		ctl_set( i->first, i->second, false );
	}
}
void module_impl::load( const FileReader & file, const std::map< std::string, std::string > & ctls ) {
	loader_log loaderlog;
	m_sndFile->SetCustomLog( &loaderlog );
	{
		int load_flags = CSoundFile::loadCompleteModule;
		if ( m_ctl_load_skip_samples ) {
			load_flags &= ~CSoundFile::loadSampleData;
		}
		if ( m_ctl_load_skip_patterns ) {
			load_flags &= ~CSoundFile::loadPatternData;
		}
		if ( !m_sndFile->Create( file, static_cast<CSoundFile::ModLoadingFlags>( load_flags ) ) ) {
			throw openmpt::exception("error loading file");
		}
		if ( !m_ctl_load_skip_subsongs_init ) {
			init_subsongs( m_subsongs );
		}
		m_loaded = true;
	}
	m_sndFile->SetCustomLog( m_LogForwarder.get() );
	std::vector<std::pair<LogLevel,std::string> > loaderMessages = loaderlog.GetMessages();
	for ( std::vector<std::pair<LogLevel,std::string> >::iterator i = loaderMessages.begin(); i != loaderMessages.end(); ++i ) {
		PushToCSoundFileLog( i->first, i->second );
		m_loaderMessages.push_back( mpt::ToCharset( mpt::CharsetUTF8, LogLevelToString( i->first ) ) + std::string(": ") + i->second );
	}
	// init CSoundFile state that corresponds to ctls
	for ( std::map< std::string, std::string >::const_iterator i = ctls.begin(); i != ctls.end(); ++i ) {
		ctl_set( i->first, i->second, false );
	}
}
bool module_impl::is_loaded() const {
	return m_loaded;
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
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
double module_impl::could_open_propability( const OpenMPT::FileReader & file, double effort, LIBOPENMPT_SHARED_PTR<log_interface> log ) {
#else
double module_impl::could_open_propability( const OpenMPT::FileReader & file, double effort, std::shared_ptr<log_interface> log ) {
#endif
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
	LIBOPENMPT_SHARED_PTR<CSoundFile> sndFile( new CSoundFile() );
#else
	std::unique_ptr<CSoundFile> sndFile( new CSoundFile() );
#endif
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
	LIBOPENMPT_SHARED_PTR<log_forwarder> logForwarder( new log_forwarder( log ) );
#else
	std::unique_ptr<log_forwarder> logForwarder( new log_forwarder( log ) );
#endif
	sndFile->SetCustomLog( logForwarder.get() );

	try {

		if ( effort >= 0.8 ) {
			if ( !sndFile->Create( file, CSoundFile::loadCompleteModule ) ) {
				return 0.0;
			}
			sndFile->Destroy();
			return 1.0;
		} else if ( effort >= 0.6 ) {
			if ( !sndFile->Create( file, CSoundFile::loadNoPatternOrPluginData ) ) {
				return 0.0;
			}
			sndFile->Destroy();
			return 0.8;
		} else if ( effort >= 0.2 ) {
			if ( !sndFile->Create( file, CSoundFile::onlyVerifyHeader ) ) {
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
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
double module_impl::could_open_propability( callback_stream_wrapper stream, double effort, LIBOPENMPT_SHARED_PTR<log_interface> log ) {
#else
double module_impl::could_open_propability( callback_stream_wrapper stream, double effort, std::shared_ptr<log_interface> log ) {
#endif
	CallbackStream fstream;
	fstream.stream = stream.stream;
	fstream.read = stream.read;
	fstream.seek = stream.seek;
	fstream.tell = stream.tell;
	return could_open_propability( FileReader( fstream ), effort, log );
}
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
double module_impl::could_open_propability( std::istream & stream, double effort, LIBOPENMPT_SHARED_PTR<log_interface> log ) {
#else
double module_impl::could_open_propability( std::istream & stream, double effort, std::shared_ptr<log_interface> log ) {
#endif
	return could_open_propability( FileReader( &stream ), effort, log );
}

#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
module_impl::module_impl( callback_stream_wrapper stream, LIBOPENMPT_SHARED_PTR<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#else
module_impl::module_impl( callback_stream_wrapper stream, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#endif
	ctor( ctls );
	CallbackStream fstream;
	fstream.stream = stream.stream;
	fstream.read = stream.read;
	fstream.seek = stream.seek;
	fstream.tell = stream.tell;
	load( FileReader( fstream ), ctls );
	apply_libopenmpt_defaults();
}
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
module_impl::module_impl( std::istream & stream, LIBOPENMPT_SHARED_PTR<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#else
module_impl::module_impl( std::istream & stream, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#endif
	ctor( ctls );
	load( FileReader( &stream ), ctls );
	apply_libopenmpt_defaults();
}
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
module_impl::module_impl( const std::vector<std::uint8_t> & data, LIBOPENMPT_SHARED_PTR<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#else
module_impl::module_impl( const std::vector<std::uint8_t> & data, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#endif
	ctor( ctls );
#ifdef LIBOPENMPT_ANCIENT_COMPILER
	load( FileReader( &(data[0]), data.size() ), ctls );
#else
	load( FileReader( data.data(), data.size() ), ctls );
#endif
	apply_libopenmpt_defaults();
}
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
module_impl::module_impl( const std::vector<char> & data, LIBOPENMPT_SHARED_PTR<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#else
module_impl::module_impl( const std::vector<char> & data, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#endif
	ctor( ctls );
#ifdef LIBOPENMPT_ANCIENT_COMPILER
	load( FileReader( &(data[0]), data.size() ), ctls );
#else
	load( FileReader( data.data(), data.size() ), ctls );
#endif
	apply_libopenmpt_defaults();
}
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
module_impl::module_impl( const std::uint8_t * data, std::size_t size, LIBOPENMPT_SHARED_PTR<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#else
module_impl::module_impl( const std::uint8_t * data, std::size_t size, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#endif
	ctor( ctls );
	load( FileReader( data, size ), ctls );
	apply_libopenmpt_defaults();
}
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
module_impl::module_impl( const char * data, std::size_t size, LIBOPENMPT_SHARED_PTR<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#else
module_impl::module_impl( const char * data, std::size_t size, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#endif
	ctor( ctls );
	load( FileReader( data, size ), ctls );
	apply_libopenmpt_defaults();
}
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
module_impl::module_impl( const void * data, std::size_t size, LIBOPENMPT_SHARED_PTR<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#else
module_impl::module_impl( const void * data, std::size_t size, std::shared_ptr<log_interface> log, const std::map< std::string, std::string > & ctls ) : m_Log(log) {
#endif
	ctor( ctls );
	load( FileReader( data, size ), ctls );
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
			return m_sndFile->m_MixerSettings.m_nStereoSeparation * 100 / MixerSettings::StereoSeparationScale;
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
			std::int32_t newvalue = value * MixerSettings::StereoSeparationScale / 100;
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
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
	LIBOPENMPT_SHARED_PTR<subsongs_type> subsongs_temp = has_subsongs_inited() ? LIBOPENMPT_SHARED_PTR<subsongs_type>() : LIBOPENMPT_SHARED_PTR<subsongs_type>( new subsongs_type( get_subsongs() ) );
#else
	std::unique_ptr<subsongs_type> subsongs_temp = has_subsongs_inited() ?  std::unique_ptr<subsongs_type>() : std::unique_ptr<subsongs_type>( new subsongs_type( get_subsongs() ) );
#endif
	const subsongs_type & subsongs = has_subsongs_inited() ? m_subsongs : *subsongs_temp;
	if ( m_current_subsong == all_subsongs ) {
		// Play all subsongs consecutively.
		double total_duration = 0.0;
		for ( std::size_t i = 0; i < subsongs.size(); ++i ) {
			total_duration += subsongs[i].duration;
		}
		return total_duration;
	}
	return subsongs[m_current_subsong].duration;
}
void module_impl::select_subsong( std::int32_t subsong ) {
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
	LIBOPENMPT_SHARED_PTR<subsongs_type> subsongs_temp = has_subsongs_inited() ? LIBOPENMPT_SHARED_PTR<subsongs_type>() : LIBOPENMPT_SHARED_PTR<subsongs_type>( new subsongs_type( get_subsongs() ) );
#else
	std::unique_ptr<subsongs_type> subsongs_temp = has_subsongs_inited() ?  std::unique_ptr<subsongs_type>() : std::unique_ptr<subsongs_type>( new subsongs_type( get_subsongs() ) );
#endif
	const subsongs_type & subsongs = has_subsongs_inited() ? m_subsongs : *subsongs_temp;
	if ( subsong != all_subsongs && ( subsong < 0 || subsong >= static_cast<std::int32_t>( subsongs.size() ) ) ) {
		throw openmpt::exception("invalid subsong");
	}
	m_current_subsong = subsong;
	m_sndFile->m_SongFlags.set( SONG_PLAYALLSONGS, subsong == all_subsongs );
	if ( subsong == all_subsongs ) {
		subsong = 0;
	}
	m_sndFile->Order.SetSequence( static_cast<SEQUENCEINDEX>( subsongs[subsong].sequence ) );
	set_position_order_row( subsongs[subsong].start_order, subsongs[subsong].start_row );
	m_currentPositionSeconds = 0.0;
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
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
	LIBOPENMPT_SHARED_PTR<subsongs_type> subsongs_temp = has_subsongs_inited() ? LIBOPENMPT_SHARED_PTR<subsongs_type>() : LIBOPENMPT_SHARED_PTR<subsongs_type>( new subsongs_type( get_subsongs() ) );
#else
	std::unique_ptr<subsongs_type> subsongs_temp = has_subsongs_inited() ?  std::unique_ptr<subsongs_type>() : std::unique_ptr<subsongs_type>( new subsongs_type( get_subsongs() ) );
#endif
	const subsongs_type & subsongs = has_subsongs_inited() ? m_subsongs : *subsongs_temp;
	const subsong_data * subsong = 0;
	double base_seconds = 0.0;
	if ( m_current_subsong == all_subsongs ) {
		// When playing all subsongs, find out which subsong this time would belong to.
		subsong = &subsongs.back();
		for ( std::size_t i = 0; i < m_subsongs.size(); ++i ) {
			if ( base_seconds + subsong->duration > seconds ) {
				subsong = &subsongs[i];
				break;
			}
			base_seconds += subsong->duration;
		}
		seconds -= base_seconds;
	} else {
		subsong = &subsongs[m_current_subsong];
	}
	GetLengthType t = m_sndFile->GetLength( eNoAdjust, GetLengthTarget( seconds ).StartPos( static_cast<SEQUENCEINDEX>( subsong->sequence ), static_cast<ORDERINDEX>( subsong->start_order ), static_cast<ROWINDEX>( subsong->start_row ) ) ).back();
	m_sndFile->m_PlayState.m_nCurrentOrder = t.lastOrder;
	m_sndFile->SetCurrentOrder( t.lastOrder );
	m_sndFile->m_PlayState.m_nNextRow = t.lastRow;
	m_currentPositionSeconds = base_seconds + m_sndFile->GetLength( m_ctl_seek_sync_samples ? eAdjustSamplePositions : eAdjust, GetLengthTarget( t.lastOrder, t.lastRow ).StartPos( static_cast<SEQUENCEINDEX>( subsong->sequence ), static_cast<ORDERINDEX>( subsong->start_order ), static_cast<ROWINDEX>( subsong->start_row ) ) ).back().duration;
	return m_currentPositionSeconds;
}
double module_impl::set_position_order_row( std::int32_t order, std::int32_t row ) {
	if ( order < 0 || order >= m_sndFile->Order.GetLengthTailTrimmed() ) {
		return m_currentPositionSeconds;
	}
	PATTERNINDEX pattern = m_sndFile->Order[order];
	if ( m_sndFile->Patterns.IsValidIndex( pattern ) ) {
		if ( row < 0 || row >= static_cast<std::int32_t>( m_sndFile->Patterns[pattern].GetNumRows() ) ) {
			return m_currentPositionSeconds;
		}
	} else {
		row = 0;
	}
	m_sndFile->m_PlayState.m_nCurrentOrder = static_cast<ORDERINDEX>( order );
	m_sndFile->SetCurrentOrder( static_cast<ORDERINDEX>( order ) );
	m_sndFile->m_PlayState.m_nNextRow = static_cast<ROWINDEX>( row );
	m_currentPositionSeconds = m_sndFile->GetLength( m_ctl_seek_sync_samples ? eAdjustSamplePositions : eAdjust, GetLengthTarget( static_cast<ORDERINDEX>( order ), static_cast<ROWINDEX>( row ) ) ).back().duration;
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
	retval.push_back("message_raw");
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
		return m_sndFile->m_madeWithTracker;
	} else if ( key == std::string("artist") ) {
		return mpt::ToCharset( mpt::CharsetUTF8, m_sndFile->m_songArtist );
	} else if ( key == std::string("title") ) {
		return mod_string_to_utf8( m_sndFile->GetTitle() );
	} else if ( key == std::string("date") ) {
		if ( m_sndFile->GetFileHistory().empty() ) {
			return std::string();
		}
		return mpt::ToCharset(mpt::CharsetUTF8, m_sndFile->GetFileHistory()[m_sndFile->GetFileHistory().size() - 1].AsISO8601() );
	} else if ( key == std::string("message") ) {
		std::string retval = m_sndFile->m_songMessage.GetFormatted( SongMessage::leLF );
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
	} else if ( key == std::string("message_raw") ) {
		std::string retval = m_sndFile->m_songMessage.GetFormatted( SongMessage::leLF );
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
	return m_sndFile->m_PlayState.m_nMusicSpeed;
}
std::int32_t module_impl::get_current_tempo() const {
	return static_cast<std::int32_t>( m_sndFile->m_PlayState.m_nMusicTempo.GetInt() );
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
	if ( !m_sndFile->Patterns.IsValidIndex( static_cast<PATTERNINDEX>( pattern ) ) ) {
		return -1;
	}
	return pattern;
}
std::int32_t module_impl::get_current_row() const {
	return m_sndFile->m_PlayState.m_nRow;
}
std::int32_t module_impl::get_current_playing_channels() const {
	return m_sndFile->GetMixStat();
}

float module_impl::get_current_channel_vu_mono( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	const float left = m_sndFile->m_PlayState.Chn[channel].nLeftVU * (1.0f/128.0f);
	const float right = m_sndFile->m_PlayState.Chn[channel].nRightVU * (1.0f/128.0f);
	return std::sqrt(left*left + right*right);
}
float module_impl::get_current_channel_vu_left( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	return m_sndFile->m_PlayState.Chn[channel].dwFlags[CHN_SURROUND] ? 0.0f : m_sndFile->m_PlayState.Chn[channel].nLeftVU * (1.0f/128.0f);
}
float module_impl::get_current_channel_vu_right( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	return m_sndFile->m_PlayState.Chn[channel].dwFlags[CHN_SURROUND] ? 0.0f : m_sndFile->m_PlayState.Chn[channel].nRightVU * (1.0f/128.0f);
}
float module_impl::get_current_channel_vu_rear_left( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	return m_sndFile->m_PlayState.Chn[channel].dwFlags[CHN_SURROUND] ? m_sndFile->m_PlayState.Chn[channel].nLeftVU * (1.0f/128.0f) : 0.0f;
}
float module_impl::get_current_channel_vu_rear_right( std::int32_t channel ) const {
	if ( channel < 0 || channel >= m_sndFile->GetNumChannels() ) {
		return 0.0f;
	}
	return m_sndFile->m_PlayState.Chn[channel].dwFlags[CHN_SURROUND] ? m_sndFile->m_PlayState.Chn[channel].nRightVU * (1.0f/128.0f) : 0.0f;
}

std::int32_t module_impl::get_num_subsongs() const {
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
	LIBOPENMPT_SHARED_PTR<subsongs_type> subsongs_temp = has_subsongs_inited() ? LIBOPENMPT_SHARED_PTR<subsongs_type>() : LIBOPENMPT_SHARED_PTR<subsongs_type>( new subsongs_type( get_subsongs() ) );
#else
	std::unique_ptr<subsongs_type> subsongs_temp = has_subsongs_inited() ?  std::unique_ptr<subsongs_type>() : std::unique_ptr<subsongs_type>( new subsongs_type( get_subsongs() ) );
#endif
	const subsongs_type & subsongs = has_subsongs_inited() ? m_subsongs : *subsongs_temp;
	return static_cast<std::int32_t>( subsongs.size() );
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
#ifdef LIBOPENMPT_ANCIENT_COMPILER_SHARED_PTR
	LIBOPENMPT_SHARED_PTR<subsongs_type> subsongs_temp = has_subsongs_inited() ? LIBOPENMPT_SHARED_PTR<subsongs_type>() : LIBOPENMPT_SHARED_PTR<subsongs_type>( new subsongs_type( get_subsongs() ) );
#else
	std::unique_ptr<subsongs_type> subsongs_temp = has_subsongs_inited() ?  std::unique_ptr<subsongs_type>() : std::unique_ptr<subsongs_type>( new subsongs_type( get_subsongs() ) );
#endif
	const subsongs_type & subsongs = has_subsongs_inited() ? m_subsongs : *subsongs_temp;
	for ( std::size_t i = 0; i < subsongs.size(); ++i ) {
		retval.push_back( mod_string_to_utf8( m_sndFile->Order.GetSequence( static_cast<SEQUENCEINDEX>( subsongs[i].sequence ) ).GetName() ) );
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
	if ( !IsInRange( p, std::numeric_limits<PATTERNINDEX>::min(), std::numeric_limits<PATTERNINDEX>::max() ) || !m_sndFile->Patterns.IsValidPat( static_cast<PATTERNINDEX>( p ) ) ) {
		return 0;
	}
	return m_sndFile->Patterns[p].GetNumRows();
}

std::uint8_t module_impl::get_pattern_row_channel_command( std::int32_t p, std::int32_t r, std::int32_t c, int cmd ) const {
	CHANNELINDEX numchannels = m_sndFile->GetNumChannels();
	if ( !IsInRange( p, std::numeric_limits<PATTERNINDEX>::min(), std::numeric_limits<PATTERNINDEX>::max() ) || !m_sndFile->Patterns.IsValidPat( static_cast<PATTERNINDEX>( p ) ) ) {
		return 0;
	}
	if ( r < 0 || r >= static_cast<std::int32_t>( m_sndFile->Patterns[p].GetNumRows() ) ) {
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
	if ( !IsInRange( p, std::numeric_limits<PATTERNINDEX>::min(), std::numeric_limits<PATTERNINDEX>::max() ) || !m_sndFile->Patterns.IsValidPat( static_cast<PATTERNINDEX>( p ) ) ) {
		return std::make_pair( std::string(), std::string() );
	}
	if ( r < 0 || r >= static_cast<std::int32_t>( m_sndFile->Patterns[p].GetNumRows() ) ) {
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
	if ( !IsInRange( p, std::numeric_limits<PATTERNINDEX>::min(), std::numeric_limits<PATTERNINDEX>::max() ) || !m_sndFile->Patterns.IsValidPat( static_cast<PATTERNINDEX>( p ) ) ) {
		return std::make_pair( text, high );
	}
	if ( r < 0 || r >= static_cast<std::int32_t>( m_sndFile->Patterns[p].GetNumRows() ) ) {
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
	retval.push_back( "load.skip_samples" );
	retval.push_back( "load.skip_patterns" );
	retval.push_back( "load.skip_subsongs_init" );
	retval.push_back( "seek.sync_samples" );
	retval.push_back( "play.tempo_factor" );
	retval.push_back( "play.pitch_factor" );
	retval.push_back( "dither" );
	return retval;
}
std::string module_impl::ctl_get( std::string ctl, bool throw_if_unknown ) const {
	if ( !ctl.empty() ) {
		std::string rightmost = ctl.substr( ctl.length() - 1, 1 );
		if ( rightmost == "!" || rightmost == "?" ) {
			if ( rightmost == "!" ) {
				throw_if_unknown = true;
			} else if ( rightmost == "?" ) {
				throw_if_unknown = false;
			}
			ctl = ctl.substr( 0, ctl.length() - 1 );
		}
	}
	if ( ctl == "" ) {
		throw openmpt::exception("empty ctl");
	} else if ( ctl == "load.skip_samples" || ctl == "load_skip_samples" ) {
		return mpt::ToString( m_ctl_load_skip_samples );
	} else if ( ctl == "load.skip_patterns" || ctl == "load_skip_patterns" ) {
		return mpt::ToString( m_ctl_load_skip_patterns );
	} else if ( ctl == "load.skip_subsongs_init" ) {
		return mpt::ToString( m_ctl_load_skip_subsongs_init );
	} else if ( ctl == "seek.sync_samples" ) {
		return mpt::ToString( m_ctl_seek_sync_samples );
	} else if ( ctl == "play.tempo_factor" ) {
		if ( !is_loaded() ) {
			return "1.0";
		}
		return mpt::ToString( 65536.0 / m_sndFile->m_nTempoFactor );
	} else if ( ctl == "play.pitch_factor" ) {
		if ( !is_loaded() ) {
			return "1.0";
		}
		return mpt::ToString( m_sndFile->m_nFreqFactor / 65536.0 );
	} else if ( ctl == "dither" ) {
		return mpt::ToString( static_cast<int>( m_Dither->GetMode() ) );
	} else {
		if ( throw_if_unknown ) {
			throw openmpt::exception("unknown ctl: " + ctl);
		} else {
			return std::string();
		}
	}
}
void module_impl::ctl_set( std::string ctl, const std::string & value, bool throw_if_unknown ) {
	if ( !ctl.empty() ) {
		std::string rightmost = ctl.substr( ctl.length() - 1, 1 );
		if ( rightmost == "!" || rightmost == "?" ) {
			if ( rightmost == "!" ) {
				throw_if_unknown = true;
			} else if ( rightmost == "?" ) {
				throw_if_unknown = false;
			}
			ctl = ctl.substr( 0, ctl.length() - 1 );
		}
	}
	if ( ctl == "" ) {
		throw openmpt::exception("empty ctl: := " + value);
	} else if ( ctl == "load.skip_samples" || ctl == "load_skip_samples" ) {
		m_ctl_load_skip_samples = ConvertStrTo<bool>( value );
	} else if ( ctl == "load.skip_patterns" || ctl == "load_skip_patterns" ) {
		m_ctl_load_skip_patterns = ConvertStrTo<bool>( value );
	} else if ( ctl == "load.skip_subsongs_init" ) {
		m_ctl_load_skip_subsongs_init = ConvertStrTo<bool>( value );
	} else if ( ctl == "seek.sync_samples" ) {
		m_ctl_seek_sync_samples = ConvertStrTo<bool>( value );
	} else if ( ctl == "play.tempo_factor" ) {
		if ( !is_loaded() ) {
			return;
		}
		double factor = ConvertStrTo<double>( value );
		if ( factor <= 0.0 || factor > 4.0 ) {
			throw openmpt::exception("invalid tempo factor");
		}
		m_sndFile->m_nTempoFactor = Util::Round<uint32_t>( 65536.0 / factor );
		m_sndFile->RecalculateSamplesPerTick();
	} else if ( ctl == "play.pitch_factor" ) {
		if ( !is_loaded() ) {
			return;
		}
		double factor = ConvertStrTo<double>( value );
		if ( factor <= 0.0 || factor > 4.0 ) {
			throw openmpt::exception("invalid pitch factor");
		}
		m_sndFile->m_nFreqFactor = Util::Round<uint32_t>( 65536.0 * factor );
		m_sndFile->RecalculateSamplesPerTick();
	} else if ( ctl == "dither" ) {
		m_Dither->SetMode( static_cast<DitherMode>( ConvertStrTo<int>( value ) ) );
	} else {
		if ( throw_if_unknown ) {
			throw openmpt::exception("unknown ctl: " + ctl + " := " + value);
		} else {
			// ignore
		}
	}
}

} // namespace openmpt
