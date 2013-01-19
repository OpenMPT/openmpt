/*
 * MixerSettings.cpp
 * ---------
 * Purpose: A struct containing settings for the mixer of soundlib.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "../soundlib/MixerSettings.h"

MixerSettings::MixerSettings() {
	
	//rewbs.resamplerConf
	glVolumeRampUpSamples = 16;
	glVolumeRampDownSamples = 42;
	gdWFIRCutoff = 0.97;
	gbWFIRType = 7; //WFIR_KAISER4T;
	//end rewbs.resamplerConf

}

