/*
 * PlaybackEventer.cpp
 * -------------------
 * Purpose: Class for scheduling playback events such as muting channels on pattern transitions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "PlaybackEventer.h"
#include "sndfile.h"


void CPlaybackEventer::PatternTranstionChnSolo(const CHANNELINDEX chnIndex)
//-------------------------------------------------------------------------
{
	if(chnIndex >= m_rSndFile.m_nChannels)
		return;

	for(CHANNELINDEX i = 0; i<m_rSndFile.m_nChannels; i++)
	{
		m_rSndFile.m_bChannelMuteTogglePending[i] = !m_rSndFile.ChnSettings[i].dwFlags[CHN_MUTE];
	}
	m_rSndFile.m_bChannelMuteTogglePending[chnIndex] = m_rSndFile.ChnSettings[chnIndex].dwFlags[CHN_MUTE];
}


void CPlaybackEventer::PatternTransitionChnUnmuteAll()
//----------------------------------------------------
{
	for(CHANNELINDEX i = 0; i<m_rSndFile.m_nChannels; i++)
	{
		m_rSndFile.m_bChannelMuteTogglePending[i] = m_rSndFile.ChnSettings[i].dwFlags[CHN_MUTE];
	}
}
