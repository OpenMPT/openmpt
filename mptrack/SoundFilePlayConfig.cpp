/*
 * SoundFilePlayConfig.cpp
 * -----------------------
 * Purpose: Configuration of sound levels, pan laws, etc... for various mix configurations.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#include ".\soundfileplayconfig.h"

CSoundFilePlayConfig::CSoundFilePlayConfig()
//------------------------------------------
{
	setVSTiVolume(1.0f);
}

CSoundFilePlayConfig::~CSoundFilePlayConfig()
//-------------------------------------------
{
}

void CSoundFilePlayConfig::SetMixLevels(int mixLevelType)
//-------------------------------------------------------
{
	switch (mixLevelType)
	{

		// Olivier's version gives us floats in [-0.5; 0.5] and slightly saturates VSTis. 
		case mixLevels_original:		
			setVSTiAttenuation(NO_ATTENUATION);
			setIntToFloat(1.0f/static_cast<float>(1<<28));
			setFloatToInt(static_cast<float>(1<<28));
			setGlobalVolumeAppliesToMaster(false);
			setUseGlobalPreAmp(true);
			setForcePanningMode(dontForcePanningMode);
			setDisplayDBValues(false);
			setNormalSamplePreAmp(128.0);
			setNormalVSTiVol(100.0);
			setNormalGlobalVol(128.0);
			setExtraSampleAttenuation(MIXING_ATTENUATION);
			break;

		// Ericus' version gives us floats in [-0.06;0.06] and requires attenuation to
		// avoid massive VSTi saturation.
		case mixLevels_117RC1:		
			setVSTiAttenuation(32.0f);
			setIntToFloat(1.0f/static_cast<float>(0x07FFFFFFF));
			setFloatToInt(static_cast<float>(0x07FFFFFFF));
			setGlobalVolumeAppliesToMaster(false);
			setUseGlobalPreAmp(true);
			setForcePanningMode(dontForcePanningMode);
			setDisplayDBValues(false);
			setNormalSamplePreAmp(128.0);
			setNormalVSTiVol(100.0);
			setNormalGlobalVol(128.0);
			setExtraSampleAttenuation(MIXING_ATTENUATION);
			break;

		// 117RC2 gives us floats in [-1.0; 1.0] and hopefully plays VSTis at 
		// the right volume... but we attenuate by 2x to approx. match sample volume.
	
		case mixLevels_117RC2:
			setVSTiAttenuation(2.0f);
			setIntToFloat(1.0f/static_cast<float>(MIXING_CLIPMAX));
			setFloatToInt(static_cast<float>(MIXING_CLIPMAX));
			setGlobalVolumeAppliesToMaster(true);
			setUseGlobalPreAmp(true);
			setForcePanningMode(dontForcePanningMode);
			setDisplayDBValues(false);
			setNormalSamplePreAmp(128.0);
			setNormalVSTiVol(100.0);
			setNormalGlobalVol(128.0);
			setExtraSampleAttenuation(MIXING_ATTENUATION);
			break;

		// 117RC3 ignores the horrible global, system-specific pre-amp, 
		// treats panning as balance to avoid saturation on loud sample (and because I think it's better :),
		// and allows display of attenuation in decibels.
		default:
		case mixLevels_117RC3:
			setVSTiAttenuation(1.0f);
			setIntToFloat(1.0f/static_cast<float>(MIXING_CLIPMAX));
			setFloatToInt(static_cast<float>(MIXING_CLIPMAX));
			setGlobalVolumeAppliesToMaster(true);
			setUseGlobalPreAmp(false);
			setForcePanningMode(forceSoftPanning);
			setDisplayDBValues(true);
			setNormalSamplePreAmp(128.0);
			setNormalVSTiVol(128.0);
			setNormalGlobalVol(256.0);
			setExtraSampleAttenuation(0);
			break;

		// A mixmode that is intended to be compatible to legacy trackers (IT/FT2/etc).
		// This is basically derived from mixmode 1.17 RC3, with panning mode and volume levels changed.
		// Sample attenuation is the same as in Schism Tracker (more attenuation than with RC3, thus VSTi attenuation is also higher).
		case mixLevels_compatible:
			setVSTiAttenuation(0.75f);
			setIntToFloat(1.0f/static_cast<float>(MIXING_CLIPMAX));
			setFloatToInt(static_cast<float>(MIXING_CLIPMAX));
			setGlobalVolumeAppliesToMaster(true);
			setUseGlobalPreAmp(false);
			setForcePanningMode(forceNoSoftPanning);
			setDisplayDBValues(true);
			setNormalSamplePreAmp(256.0);
			setNormalVSTiVol(256.0);
			setNormalGlobalVol(256.0);
			setExtraSampleAttenuation(1);
			break;

	}
	
	return;
}



//getters and setters.
bool CSoundFilePlayConfig::getGlobalVolumeAppliesToMaster()
//---------------------------------------------------------
{
	return m_globalVolumeAppliesToMaster;
}


void CSoundFilePlayConfig::setGlobalVolumeAppliesToMaster(bool inGlobalVolumeAppliesToMaster)
//-------------------------------------------------------------------------------------------
{
	m_globalVolumeAppliesToMaster=inGlobalVolumeAppliesToMaster;
}

float CSoundFilePlayConfig::getVSTiGainFactor()
//---------------------------------------------
{
	return m_VSTiVolume;
}

float CSoundFilePlayConfig::getVSTiVolume()
//-----------------------------------------
{
	return m_VSTiVolume;
}

void  CSoundFilePlayConfig::setVSTiVolume(float inVSTiVolume)
//-----------------------------------------------------------
{
	m_VSTiVolume = inVSTiVolume;
}

float CSoundFilePlayConfig::getVSTiAttenuation()
//----------------------------------------------
{
	return m_VSTiAttenuation;
}

void  CSoundFilePlayConfig::setVSTiAttenuation(float inVSTiAttenuation)
//---------------------------------------------------------------------
{
	m_VSTiAttenuation = inVSTiAttenuation;
}

float CSoundFilePlayConfig::getIntToFloat()
//-----------------------------------------
{
	return m_IntToFloat;
}

void  CSoundFilePlayConfig::setIntToFloat(float inIntToFloat)
//-----------------------------------------------------------
{
	m_IntToFloat = inIntToFloat;
}


float CSoundFilePlayConfig::getFloatToInt()
//-----------------------------------------
{
	return m_FloatToInt;
}


void  CSoundFilePlayConfig::setFloatToInt(float inFloatToInt)
//-----------------------------------------------------------
{
	m_FloatToInt = inFloatToInt;
}

bool CSoundFilePlayConfig::getUseGlobalPreAmp()
//---------------------------------------------
{
	return m_ignorePreAmp;
}

void CSoundFilePlayConfig::setUseGlobalPreAmp(bool inUseGlobalPreAmp)
//-------------------------------------------------------------------
{
	m_ignorePreAmp = inUseGlobalPreAmp;
}


forcePanningMode CSoundFilePlayConfig::getForcePanningMode()
//----------------------------------------------------------
{
	return m_forceSoftPanning;
}

void CSoundFilePlayConfig::setForcePanningMode(forcePanningMode inForceSoftPanning)
//---------------------------------------------------------------------------------
{
	m_forceSoftPanning = inForceSoftPanning;
}

void CSoundFilePlayConfig::setDisplayDBValues(bool in)
//----------------------------------------------------
{
	m_displayDBValues = in;
}

void CSoundFilePlayConfig::setNormalSamplePreAmp(double in)
//---------------------------------------------------------
{
	m_normalSamplePreAmp = in;
}

void CSoundFilePlayConfig::setNormalVSTiVol(double in)
//----------------------------------------------------
{
	m_normalVSTiVol = in;
}

void CSoundFilePlayConfig::setNormalGlobalVol(double in)
//------------------------------------------------------
{
	m_normalGlobalVol = in;
}

bool CSoundFilePlayConfig::getDisplayDBValues()
//---------------------------------------------
{
	return m_displayDBValues;
}

double CSoundFilePlayConfig::getNormalSamplePreAmp()
//--------------------------------------------------
{
	return m_normalSamplePreAmp;
}

double CSoundFilePlayConfig::getNormalVSTiVol()
//---------------------------------------------
{
	return m_normalVSTiVol;
}

double CSoundFilePlayConfig::getNormalGlobalVol()
//-----------------------------------------------
{
	return m_normalGlobalVol;
}

void CSoundFilePlayConfig::setExtraSampleAttenuation(int attn)
//------------------------------------------------------------
{
	m_extraAttenuation = attn;
}

int CSoundFilePlayConfig::getExtraSampleAttenuation()
//---------------------------------------------------
{
	return m_extraAttenuation;
}
