#include "stdafx.h"
#include ".\soundfileplayconfig.h"

CSoundFilePlayConfig::CSoundFilePlayConfig(void)
{
	SetPluginMixLevels(plugmix_mode_117RC3);
	setVSTiVolume(1.0f);
}

CSoundFilePlayConfig::~CSoundFilePlayConfig(void)
{
}

void CSoundFilePlayConfig::SetPluginMixLevels(int mixLevelType) {
	switch (mixLevelType)
	{
		// Olivier's version gives us floats in [-0.5; 0.5] and slightly saturates VSTis. 
		case plugmix_mode_original:		
			setVSTiAttenuation(NO_ATTENUATION);
			setIntToFloat(1.0f/static_cast<float>(1<<28));
			setFloatToInt(static_cast<float>(1<<28));
			setGlobalVolumeAppliesToMaster(false);
			setUseGlobalPreAmp(true);
			setTreatPanLikeBalance(false);
			setDisplayDBValues(false);
			setNormalSamplePreAmp(128.0);
			setNormalVSTiVol(100.0);
			setNormalGlobalVol(128.0);
			break;

		// Ericus' version gives us floats in [-0.06;0.06] and requires attenuation to
		// avoid massive VSTi saturation.
		case plugmix_mode_117RC1:		
			setVSTiAttenuation(32.0f);
			setIntToFloat(1.0f/static_cast<float>(0x07FFFFFFF));
			setFloatToInt(static_cast<float>(0x07FFFFFFF));
			setGlobalVolumeAppliesToMaster(false);
			setUseGlobalPreAmp(true);
			setTreatPanLikeBalance(false);
			setDisplayDBValues(false);
			setNormalSamplePreAmp(128.0);
			setNormalVSTiVol(100.0);
			setNormalGlobalVol(128.0);
			break;

		// 117RC2 gives us floats in [-1.0; 1.0] and hopefully plays VSTis at 
		// the right volume... but we attenuate by 2x to approx. match sample volume.
	
		case plugmix_mode_117RC2:
			setVSTiAttenuation(2.0f);
			setIntToFloat(1.0f/static_cast<float>(MIXING_CLIPMAX));
			setFloatToInt(static_cast<float>(MIXING_CLIPMAX));
			setGlobalVolumeAppliesToMaster(true);
			setUseGlobalPreAmp(true);
			setTreatPanLikeBalance(false);
			setDisplayDBValues(false);
			setNormalSamplePreAmp(128.0);
			setNormalVSTiVol(100.0);
			setNormalGlobalVol(128.0);
			break;

		// 117RC3 ignores the horrible global, system-specific pre-amp, 
		// treats panning as balance to avoid saturation on loud sample (and because I think it's better :),
		// and allows display of attenuation in decibels.
		default:
		case plugmix_mode_117RC3:
			setVSTiAttenuation(1.0f);
			setIntToFloat(1.0f/static_cast<float>(MIXING_CLIPMAX));
			setFloatToInt(static_cast<float>(MIXING_CLIPMAX));
			setGlobalVolumeAppliesToMaster(true);
			setUseGlobalPreAmp(false);
			setTreatPanLikeBalance(true);
			setDisplayDBValues(true);
			setNormalSamplePreAmp(128.0);
			setNormalVSTiVol(128.0);
			setNormalGlobalVol(256.0);
			break;

		// FOR TEST PURPOSES ONLY:
		/*
		case plugmix_mode_Test:
			setVSTiAttenuation(1.0f);
			setIntToFloat(1.0f/static_cast<float>(MIXING_CLIPMAX));
			setFloatToInt(static_cast<float>(MIXING_CLIPMAX));
			setGlobalVolumeAppliesToMaster(true);
			setUseGlobalPreAmp(false);
			setTreatPanLikeBalance(true);
			setDisplayDBValues(true);
			setNormalSamplePreAmp(128.0);
			setNormalVSTiVol(128.0);
			setNormalGlobalVol(256.0);
			break;
		*/

	}
	
	return;
}



//getters and setters.
bool CSoundFilePlayConfig::getGlobalVolumeAppliesToMaster() {
	return m_globalVolumeAppliesToMaster;
}


void CSoundFilePlayConfig::setGlobalVolumeAppliesToMaster(bool inGlobalVolumeAppliesToMaster){
	m_globalVolumeAppliesToMaster=inGlobalVolumeAppliesToMaster;
}

float CSoundFilePlayConfig::getVSTiGainFactor() {
	return m_VSTiVolume;
}

float CSoundFilePlayConfig::getVSTiVolume() {
	return m_VSTiVolume;
}

void  CSoundFilePlayConfig::setVSTiVolume(float inVSTiVolume) {
	m_VSTiVolume = inVSTiVolume;
	m_VSTiGainFactor = m_VSTiAttenuation*m_VSTiVolume;
}

float CSoundFilePlayConfig::getVSTiAttenuation() {
	return m_VSTiAttenuation;
}

void  CSoundFilePlayConfig::setVSTiAttenuation(float inVSTiAttenuation) {
	m_VSTiAttenuation=inVSTiAttenuation;
	m_VSTiGainFactor = m_VSTiAttenuation*m_VSTiVolume;
}

float CSoundFilePlayConfig::getIntToFloat() {
	return m_IntToFloat;
}

void  CSoundFilePlayConfig::setIntToFloat(float inIntToFloat) {
	m_IntToFloat=inIntToFloat;
}


float CSoundFilePlayConfig::getFloatToInt() {
	return m_FloatToInt;
}


void  CSoundFilePlayConfig::setFloatToInt(float inFloatToInt) {
	m_FloatToInt=inFloatToInt;
}

bool CSoundFilePlayConfig::getUseGlobalPreAmp() {
	return m_ignorePreAmp;
}

void CSoundFilePlayConfig::setUseGlobalPreAmp(bool inUseGlobalPreAmp) {
	m_ignorePreAmp=inUseGlobalPreAmp;
}


bool CSoundFilePlayConfig::getTreatPanLikeBalance() {
	return m_treatPanLikeBalance;
}

void CSoundFilePlayConfig::setTreatPanLikeBalance(bool inTreatPanLikeBalance) {
	m_treatPanLikeBalance=inTreatPanLikeBalance;
}

void CSoundFilePlayConfig::setDisplayDBValues(bool in) {
	m_displayDBValues=in;
}

void CSoundFilePlayConfig::setNormalSamplePreAmp(double in) {
	m_normalSamplePreAmp=in;
}

void CSoundFilePlayConfig::setNormalVSTiVol(double in) {
	m_normalVSTiVol=in;
}

void CSoundFilePlayConfig::setNormalGlobalVol(double in) {
	m_normalGlobalVol=in;
}

bool CSoundFilePlayConfig::getDisplayDBValues() {
	return m_displayDBValues;
}

double CSoundFilePlayConfig::getNormalSamplePreAmp() {
	return m_normalSamplePreAmp;
}

double CSoundFilePlayConfig::getNormalVSTiVol() {
	return m_normalVSTiVol;
}

double CSoundFilePlayConfig::getNormalGlobalVol() {
	return m_normalGlobalVol;
}



