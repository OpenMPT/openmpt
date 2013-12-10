/*
 * libopenmpt_modplug_cpp.cpp
 * --------------------------
 * Purpose: libopenmpt emulation of the libmodplug c++ interface
 * Notes  : WARNING! THIS IS INCOMPLETE!
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef NO_LIBMODPLUG

/*

***********************************************************************
WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
***********************************************************************

This is a dirty hack to emulate just so much of the libmodplug c++
interface so that the current known users (mainly xmms-modplug itself,
gstreamer modplug, and stuff based on those 2) work. This is neither
a complete nor a correct implementation.
Symbols unused by these implementations are not included.
Metadata and other state is not provided or updated.

*/

#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif /* _MSC_VER */

#include "libopenmpt.hpp"

#include <string>
#include <vector>

#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _MSC_VER
/* msvc errors when seeing dllexport declarations after prototypes have been declared in modplug.h */
#define LIBOPENMPT_MODPLUG_API
#else /* !_MSC_VER */
#define LIBOPENMPT_MODPLUG_API LIBOPENMPT_API_HELPER_EXPORT
#endif /* _MSC_VER */

template <class T>
void Clear( T & x )
{
	std::memset( &x, 0, sizeof(T) );
}

class LIBOPENMPT_MODPLUG_API CSoundFile;

#include "libmodplug/stdafx.h"
#include "libmodplug/sndfile.h"

//#define mpcpplog() fprintf(stderr, "%s %i\n", __func__, __LINE__)
#define mpcpplog() do{}while(0)

#define UNUSED(x) (void)((x))

union self_t {
	CHAR CompressionTable[16];
	openmpt::module * self_;
};

static void set_self( CSoundFile * that, openmpt::module * self_ ) {
	self_t self_union;
	Clear(self_union);
	self_union.self_ = self_;
	std::memcpy( that->CompressionTable, self_union.CompressionTable, sizeof( self_union.CompressionTable ) );
}

static openmpt::module * get_self( const CSoundFile * that ) {
	self_t self_union;
	Clear(self_union);
	std::memcpy( self_union.CompressionTable, that->CompressionTable, sizeof( self_union.CompressionTable ) );
	return self_union.self_;
}

#define mod ( get_self( this ) )

UINT CSoundFile::m_nXBassDepth = 0;
UINT CSoundFile::m_nXBassRange = 0;
UINT CSoundFile::m_nReverbDepth = 0;
UINT CSoundFile::m_nReverbDelay = 0;
UINT CSoundFile::gnReverbType = 0;
UINT CSoundFile::m_nProLogicDepth = 0;
UINT CSoundFile::m_nProLogicDelay = 0;
UINT CSoundFile::m_nStereoSeparation = 128;
UINT CSoundFile::m_nMaxMixChannels = 256;
LONG CSoundFile::m_nStreamVolume = 0x8000;
DWORD CSoundFile::gdwSysInfo = 0;
DWORD CSoundFile::gdwSoundSetup = 0;
DWORD CSoundFile::gdwMixingFreq = 44100;
DWORD CSoundFile::gnBitsPerSample = 16;
DWORD CSoundFile::gnChannels = 2;
UINT CSoundFile::gnAGC = 0;
UINT CSoundFile::gnVolumeRampSamples = 0;
UINT CSoundFile::gnVUMeter = 0;
UINT CSoundFile::gnCPUUsage = 0;;
LPSNDMIXHOOKPROC CSoundFile::gpSndMixHook = 0;
PMIXPLUGINCREATEPROC CSoundFile::gpMixPluginCreateProc = 0;

CSoundFile::CSoundFile() {
	mpcpplog();
	Clear(Chn);
	Clear(ChnMix);
	Clear(Ins);
	Clear(Headers);
	Clear(ChnSettings);
	Clear(Patterns);
	Clear(PatternSize);
	Clear(Order);
	Clear(m_MidiCfg);
	Clear(m_MixPlugins);
	Clear(m_nDefaultSpeed);
	Clear(m_nDefaultTempo);
	Clear(m_nDefaultGlobalVolume);
	Clear(m_dwSongFlags);
	Clear(m_nChannels);
	Clear(m_nMixChannels);
	Clear(m_nMixStat);
	Clear(m_nBufferCount);
	Clear(m_nType);
	Clear(m_nSamples);
	Clear(m_nInstruments);
	Clear(m_nTickCount);
	Clear(m_nTotalCount);
	Clear(m_nPatternDelay);
	Clear(m_nFrameDelay);
	Clear(m_nMusicSpeed);
	Clear(m_nMusicTempo);
	Clear(m_nNextRow);
	Clear(m_nRow);
	Clear(m_nPattern);
	Clear(m_nCurrentPattern);
	Clear(m_nNextPattern);
	Clear(m_nRestartPos);
	Clear(m_nMasterVolume);
	Clear(m_nGlobalVolume);
	Clear(m_nSongPreAmp);
	Clear(m_nFreqFactor);
	Clear(m_nTempoFactor);
	Clear(m_nOldGlbVolSlide);
	Clear(m_nMinPeriod);
	Clear(m_nMaxPeriod);
	Clear(m_nRepeatCount);
	Clear(m_nInitialRepeatCount);
	Clear(m_nGlobalFadeSamples);
	Clear(m_nGlobalFadeMaxSamples);
	Clear(m_nMaxOrderPosition);
	Clear(m_nPatternNames);
	Clear(m_lpszSongComments);
	Clear(m_lpszPatternNames);
	Clear(m_szNames);
	Clear(CompressionTable);
}

CSoundFile::~CSoundFile() {
	mpcpplog();
	Destroy();
}

BOOL CSoundFile::Create( LPCBYTE lpStream, DWORD dwMemLength ) {
	mpcpplog();
	try {
		openmpt::module * m = new openmpt::module( lpStream, dwMemLength );
		set_self( this, m );
		std::strncpy( m_szNames[0], mod->get_metadata("title").c_str(), sizeof( m_szNames[0] ) );
		m_szNames[0][ sizeof( m_szNames[0] ) - 1 ] = '\0';
		return TRUE;
	} catch ( ... ) {
		Destroy();
		return FALSE;
	}
}

BOOL CSoundFile::Destroy() {
	mpcpplog();
	if ( mod ) {
		delete mod;
		set_self( this, 0 );
	}
	return TRUE;
}

BOOL CSoundFile::SetWaveConfig( UINT nRate, UINT nBits, UINT nChannels, BOOL bMMX ) {
	UNUSED(bMMX);
	mpcpplog();
	gdwMixingFreq = nRate;
	gnBitsPerSample = nBits;
	gnChannels = nChannels;
	return TRUE;
}

BOOL CSoundFile::SetWaveConfigEx( BOOL bSurround, BOOL bNoOverSampling, BOOL bReverb, BOOL hqido, BOOL bMegaBass, BOOL bNR, BOOL bEQ ) {
	UNUSED(bSurround);
	UNUSED(bReverb);
	UNUSED(hqido);
	UNUSED(bMegaBass);
	UNUSED(bEQ);
	mpcpplog();
	DWORD d = gdwSoundSetup & ~(SNDMIX_NORESAMPLING|SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE);
	if ( bNoOverSampling ) {
		d |= SNDMIX_NORESAMPLING;
	} else if ( !hqido ) {
		d |= 0;
	} else if ( !bNR ) {
		d |= SNDMIX_HQRESAMPLER;
	} else {
			d |= (SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE);
	}
	gdwSoundSetup = d;
	return TRUE;
}

BOOL CSoundFile::SetResamplingMode( UINT nMode ) {
	mpcpplog();
	DWORD d = gdwSoundSetup & ~(SNDMIX_NORESAMPLING|SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE);
	switch ( nMode ) {
		case SRCMODE_NEAREST:
			d |= SNDMIX_NORESAMPLING;
			break;
		case SRCMODE_LINEAR:
			break;
		case SRCMODE_SPLINE:
			d |= SNDMIX_HQRESAMPLER;
			break;
		case SRCMODE_POLYPHASE:
			d |= (SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE);
			break;
		default:
			return FALSE;
			break;
	}
	gdwSoundSetup = d;
	return TRUE;
}

BOOL CSoundFile::SetReverbParameters( UINT nDepth, UINT nDelay ) {
	UNUSED(nDepth);
	UNUSED(nDelay);
	mpcpplog();
	return TRUE;
}

BOOL CSoundFile::SetXBassParameters( UINT nDepth, UINT nRange ) {
	UNUSED(nDepth);
	UNUSED(nRange);
	mpcpplog();
	return TRUE;
}

BOOL CSoundFile::SetSurroundParameters( UINT nDepth, UINT nDelay ) {
	UNUSED(nDepth);
	UNUSED(nDelay);
	mpcpplog();
	return TRUE;
}

UINT CSoundFile::GetMaxPosition() const {
	mpcpplog();
	// rows in original, just use seconds here
	if ( mod ) return static_cast<UINT>( mod->get_duration_seconds() + 0.5 );
	return 0;
}

DWORD CSoundFile::GetLength( BOOL bAdjust, BOOL bTotal ) {
	UNUSED(bAdjust);
	UNUSED(bTotal);
	mpcpplog();
	if ( mod ) return static_cast<DWORD>( mod->get_duration_seconds() + 0.5 );
	return 0;
}

UINT CSoundFile::GetSongComments( LPSTR s, UINT cbsize, UINT linesize ) {
	UNUSED(linesize);
	mpcpplog();
	if ( !s ) {
		return 0;
	}
	if ( cbsize <= 0 ) {
		return 0;
	}
	if ( !mod ) {
		s[0] = '\0';
		return 1;
	}
	std::strncpy( s, mod->get_metadata("message").c_str(), cbsize );
	s[ cbsize - 1 ] = '\0';
	return std::strlen( s ) + 1;
}

void CSoundFile::SetCurrentPos( UINT nPos ) {
	mpcpplog();
	if ( mod ) mod->set_position_seconds( nPos );
}

UINT CSoundFile::GetCurrentPos() const {
	mpcpplog();
	if ( mod ) return static_cast<UINT>( mod->get_position_seconds() + 0.5 );
	return 0;
}

static std::int32_t get_filter_length() {
	mpcpplog();
	if ( ( CSoundFile::gdwSoundSetup & (SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE) ) == (SNDMIX_HQRESAMPLER|SNDMIX_ULTRAHQSRCMODE) ) {
		return 8;
	} else if ( ( CSoundFile::gdwSoundSetup & SNDMIX_HQRESAMPLER ) == SNDMIX_HQRESAMPLER ) {
		return 4;
	} else if ( ( CSoundFile::gdwSoundSetup & SNDMIX_HQRESAMPLER ) == SNDMIX_NORESAMPLING ) {
		return 1;
	} else {
		return 2;
	}
}

static std::size_t get_sample_size() {
	return (CSoundFile::gnBitsPerSample/8);
}

static std::size_t get_num_channels() {
	return CSoundFile::gnChannels;
}

static std::size_t get_frame_size() {
	return get_sample_size() * get_num_channels();
}

static std::int32_t get_samplerate() {
	return CSoundFile::gdwMixingFreq;
}

UINT CSoundFile::Read( LPVOID lpBuffer, UINT cbBuffer ) {
	mpcpplog();
	if ( !mod ) {
		return 0;
	}
	mpcpplog();
	if ( !lpBuffer ) {
		return 0;
	}
	mpcpplog();
	if ( cbBuffer <= 0 ) {
		return 0;
	}
	mpcpplog();
	if ( get_samplerate() <= 0 ) {
		return 0;
	}
	mpcpplog();
	if ( get_sample_size() != 1 && get_sample_size() != 2 && get_sample_size() != 4 ) {
		return 0;
	}
	mpcpplog();
	if ( get_num_channels() != 1 && get_num_channels() != 2 && get_num_channels() != 4 ) {
		return 0;
	}
	mpcpplog();
	std::memset( lpBuffer, 0, cbBuffer );
	const std::size_t frames_torender = cbBuffer / get_frame_size();
	std::int16_t * out = (std::int16_t*)lpBuffer;
	std::vector<std::int16_t> tmpbuf;
	if ( get_sample_size() == 1 || get_sample_size() == 4 ) {
		tmpbuf.resize( frames_torender * get_num_channels() );
		out = &tmpbuf[0];
	}

	mod->set_render_param( openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH, get_filter_length() );
	std::size_t frames_rendered = 0;
	if ( get_num_channels() == 1 ) {
		frames_rendered = mod->read( get_samplerate(), frames_torender, out );
	} else if ( get_num_channels() == 4 ) {
		frames_rendered = mod->read_interleaved_quad( get_samplerate(), frames_torender, out );
	} else {
		frames_rendered = mod->read_interleaved_stereo( get_samplerate(), frames_torender, out );
	}

	if ( get_sample_size() == 1 ) {
		std::uint8_t * dst = (std::uint8_t*)lpBuffer;
		for ( std::size_t sample = 0; sample < frames_rendered * get_num_channels(); ++sample ) {
			dst[sample] = ( tmpbuf[sample] / 0x100 ) + 0x80;
		}
	} else if ( get_sample_size() == 4 ) {
		std::int32_t * dst = (std::int32_t*)lpBuffer;
		for ( std::size_t sample = 0; sample < frames_rendered * get_num_channels(); ++sample ) {
			dst[sample] = tmpbuf[sample] << (32-16-1-MIXING_ATTENUATION);
		}
	}
	return frames_rendered * get_frame_size();
}

/*

gstreamer modplug calls:

mSoundFile->Create
mSoundFile->Destroy

mSoundFile->SetWaveConfig
mSoundFile->SetWaveConfigEx
mSoundFile->SetResamplingMode
mSoundFile->SetSurroundParameters
mSoundFile->SetXBassParameters
mSoundFile->SetReverbParameters

mSoundFile->GetMaxPosition (inline, -> GetLength)
mSoundFile->GetSongTime

mSoundFile->GetTitle (inline)
mSoundFile->GetSongComments

mSoundFile->SetCurrentPos
mSoundFile->Read

mSoundFile->GetCurrentPos
mSoundFile->GetMusicTempo (inline)

*/

#endif // NO_LIBMODPLUG
