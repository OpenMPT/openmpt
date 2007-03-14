#ifndef PLAYBACKEVENTER_H
#define PLAYBACKEVENTER_H

#include "pattern.h"

class CSoundFile;

//====================
class CPlaybackEventer
//====================
{
public:
	CPlaybackEventer(CSoundFile& sndf) : m_rSndFile(sndf) {}
	~CPlaybackEventer();

	//SetPatternEvent(const PATTERNINDEX pattern, const ROWINDEX row, const CHANNELINDEX column);

	void PatternTranstionChnSolo(const CHANNELINDEX chnIndex);
	void PatternTransitionChnUnmuteAll();

private:
	CSoundFile& m_rSndFile;
};

#endif
