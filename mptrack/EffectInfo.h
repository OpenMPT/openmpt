#pragma once
#ifndef EFFECTINFO_H
#define EFFECTINFO_H

#include "Moddoc.h"

//==============
class EffectInfo
//==============
{
protected:
	const CSoundFile &sndFile;

public:

	EffectInfo(const CSoundFile &sf) : sndFile(sf) {};

	// Effects Description
	
	// Get basic effect name with some additional information for some effects
	bool GetEffectName(LPSTR pszDescription, MODCOMMAND::COMMAND command, UINT param, bool bXX = false, CHANNELINDEX nChn = CHANNELINDEX_INVALID) const; // bXX: Nxx: ...
	// Get size of list of known effect commands
	UINT GetNumEffects() const;
	// Get range information, effect name, etc... from a given effect.
	bool GetEffectInfo(UINT ndx, LPSTR s, bool bXX = false, DWORD *prangeMin = nullptr, DWORD *prangeMax = nullptr) const;
	// Get effect index in effect list from effect command + param
	LONG GetIndexFromEffect(MODCOMMAND::COMMAND command, MODCOMMAND::PARAM param) const;
	// Get effect command + param from effect index
	MODCOMMAND::COMMAND GetEffectFromIndex(UINT ndx, int &refParam) const;
	MODCOMMAND::COMMAND GetEffectFromIndex(UINT ndx) const;
	// Get parameter mask from effect (for extended effects)
	UINT GetEffectMaskFromIndex(UINT ndx) const;
	// Get precise effect name, also with explanation of effect parameter
	bool GetEffectNameEx(LPSTR pszName, UINT ndx, UINT param) const;
	// Check whether an effect is extended (with parameter nibbles)
	bool IsExtendedEffect(UINT ndx) const;
	// Map an effect value to slider position
	UINT MapValueToPos(UINT ndx, UINT param) const;
	// Map slider position to an effect value
	UINT MapPosToValue(UINT ndx, UINT pos) const;

	// Volume column effects description

	// Get size of list of known volume commands
	UINT GetNumVolCmds() const;
	// Get effect index in effect list from volume command
	LONG GetIndexFromVolCmd(MODCOMMAND::VOLCMD volcmd) const;
	// Get volume command from effect index
	MODCOMMAND::VOLCMD GetVolCmdFromIndex(UINT ndx) const;
	// Get range information, effect name, etc... from a given effect.
	bool GetVolCmdInfo(UINT ndx, LPSTR s, DWORD *prangeMin = nullptr, DWORD *prangeMax = nullptr) const;
};

#endif // EFFECTINFO_H