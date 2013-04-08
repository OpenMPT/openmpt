/*
 * PlaybackEventer.h
 * -----------------
 * Purpose: Class for scheduling playback events such as muting channels on pattern transitions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "Snd_defs.h"
class CSoundFile;

//====================
class CPlaybackEventer
//====================
{
public:
	CPlaybackEventer(CSoundFile &sndf) : m_rSndFile(sndf) {}

	void PatternTranstionChnSolo(const CHANNELINDEX chnIndex);
	void PatternTransitionChnUnmuteAll();

private:
	CSoundFile &m_rSndFile;
};
