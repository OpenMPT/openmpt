/*
 * EffectInfo.h
 * ------------
 * Purpose: Provide information about effect names, parameter interpretation to the tracker interface.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "modcommand.h"

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;

class EffectInfo
{
protected:
	const CSoundFile &sndFile;

public:

	EffectInfo(const CSoundFile &sf) : sndFile(sf) {};

	// Effects Description
	
	bool GetEffectName(CString &pszDescription, ModCommand::COMMAND command, UINT param, bool bXX = false) const; // bXX: Nxx: ...
	// Get size of list of known effect commands
	UINT GetNumEffects() const;
	// Get range information, effect name, etc... from a given effect.
	bool GetEffectInfo(UINT ndx, CString *s, bool bXX = false, ModCommand::PARAM *prangeMin = nullptr, ModCommand::PARAM *prangeMax = nullptr) const;
	// Get effect index in effect list from effect command + param
	LONG GetIndexFromEffect(ModCommand::COMMAND command, ModCommand::PARAM param) const;
	// Get effect command + param from effect index
	EffectCommand GetEffectFromIndex(UINT ndx, ModCommand::PARAM &refParam) const;
	EffectCommand GetEffectFromIndex(UINT ndx) const;
	// Get parameter mask from effect (for extended effects)
	UINT GetEffectMaskFromIndex(UINT ndx) const;
	// Get precise effect name, also with explanation of effect parameter
	bool GetEffectNameEx(CString &pszName, UINT ndx, UINT param, CHANNELINDEX chn = CHANNELINDEX_INVALID) const;
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
	LONG GetIndexFromVolCmd(ModCommand::VOLCMD volcmd) const;
	// Get volume command from effect index
	VolumeCommand GetVolCmdFromIndex(UINT ndx) const;
	// Get range information, effect name, etc... from a given effect.
	bool GetVolCmdInfo(UINT ndx, CString *s, ModCommand::VOL *prangeMin = nullptr, ModCommand::VOL *prangeMax = nullptr) const;
	// Get effect name and parameter description
	bool GetVolCmdParamInfo(const ModCommand &m, CString *s) const;
};

OPENMPT_NAMESPACE_END
