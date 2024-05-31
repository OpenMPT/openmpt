/*
 * PlayState.cpp
 * -------------
 * Purpose: This class represents all of the playback state of a module.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "PlayState.h"
#include "MIDIMacros.h"
#include "Mixer.h"
#include "Sndfile.h"


OPENMPT_NAMESPACE_BEGIN


PlayState::PlayState()
{
	Chn.fill({});
	m_midiMacroScratchSpace.reserve(kMacroLength);  // Note: If macros ever become variable-length, the scratch space needs to be at least one byte longer than the longest macro in the file for end-of-SysEx insertion to stay allocation-free in the mixer!
}


void PlayState::ResetGlobalVolumeRamping() noexcept
{
	m_lHighResRampingGlobalVolume = m_nGlobalVolume << VOLUMERAMPPRECISION;
	m_nGlobalVolumeDestination = m_nGlobalVolume;
	m_nSamplesToGlobalVolRampDest = 0;
	m_nGlobalVolumeRampAmount = 0;
}


mpt::span<ModChannel> PlayState::PatternChannels(const CSoundFile &sndFile) noexcept
{
	return mpt::as_span(Chn).subspan(0, std::min(Chn.size(), static_cast<size_t>(sndFile.GetNumChannels())));
}


mpt::span<ModChannel> PlayState::BackgroundChannels(const CSoundFile &sndFile) noexcept
{
	return mpt::as_span(Chn).subspan(std::min(Chn.size(), static_cast<size_t>(sndFile.GetNumChannels())));
}


OPENMPT_NAMESPACE_END
