#include "stdafx.h"
#include "PlaybackEventer.h"
#include "sndfile.h"

CPlaybackEventer::~CPlaybackEventer()
//---------------------------------
{
}

void CPlaybackEventer::PatternTranstionChnSolo(const CHANNELINDEX chnIndex)
//-------------------------------------------------------------------------
{
	if(chnIndex >= m_rSndFile.m_nChannels)
		return;

	for(CHANNELINDEX i = 0; i<m_rSndFile.m_nChannels; i++)
	{
		m_rSndFile.m_bChannelMuteTogglePending[i] = (m_rSndFile.ChnSettings[i].dwFlags & CHN_MUTE) ? false : true;
	}
	m_rSndFile.m_bChannelMuteTogglePending[chnIndex] = (m_rSndFile.ChnSettings[chnIndex].dwFlags & CHN_MUTE) ? true : false;
}


void CPlaybackEventer::PatternTransitionChnUnmuteAll()
//----------------------------------------------------
{
	for(CHANNELINDEX i = 0; i<m_rSndFile.m_nChannels; i++)
	{
		m_rSndFile.m_bChannelMuteTogglePending[i] = (m_rSndFile.ChnSettings[i].dwFlags & CHN_MUTE) ? true : false;
	}
}