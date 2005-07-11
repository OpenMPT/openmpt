#include "stdafx.h"
#include ".\soundfileplayconfig.h"

CSoundFilePlayConfig::CSoundFilePlayConfig(void)
{
	SetPluginMixLevels(plugmix_mode_117RC2);
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
			break;

		// Ericus' version gives us floats in [-0.06;0.06] and requires attenuation to
		// avoid massive VSTi saturation.
		case plugmix_mode_117RC1:		
			setVSTiAttenuation(32.0f);
			setIntToFloat(1.0f/static_cast<float>(0x07FFFFFFF));
			setFloatToInt(static_cast<float>(0x07FFFFFFF));
			break;

		// Rewbs' version gives us floats in [-1.0; 1.0] and hopefully plays VSTis at 
		// the right volume.
		default:
		case plugmix_mode_117RC2:
			setVSTiAttenuation(NO_ATTENUATION);
			setIntToFloat(1.0f/static_cast<float>(MIXING_CLIPMAX));
			setFloatToInt(static_cast<float>(MIXING_CLIPMAX));
	}
	
	return;
}



//getters and setters.
float CSoundFilePlayConfig::getVSTiAttenuation() {
	return m_VSTiAttenuation;
}

void  CSoundFilePlayConfig::setVSTiAttenuation(float inVSTiAttenuation) {
	m_VSTiAttenuation=inVSTiAttenuation;
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

