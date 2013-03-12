/*
 * MixerSettings.h
 * ---------------
 * Purpose: A struct containing settings for the mixer of soundlib.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

struct MixerSettings
{

	//rewbs.resamplerConf
	double gdWFIRCutoff;
	uint8 gbWFIRType;
	long glVolumeRampUpSamples, glVolumeRampDownSamples;
	//end rewbs.resamplerConf

	MixerSettings();

};
