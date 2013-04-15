/*
 * SoundFilePlayConfig.h
 * ---------------------
 * Purpose: Configuration of sound levels, pan laws, etc... for various mix configurations.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

enum
{
	NO_ATTENUATION = 1,
};

enum tempoMode
{
	tempo_mode_classic		= 0,
	tempo_mode_alternative	= 1,
	tempo_mode_modern		= 2,
};

enum mixLevels
{
	mixLevels_original		= 0,
	mixLevels_117RC1		= 1,
	mixLevels_117RC2		= 2,
	mixLevels_117RC3		= 3,
	mixLevels_compatible	= 4,
};

enum forcePanningMode
{
	dontForcePanningMode,
	forceSoftPanning,
	forceNoSoftPanning,
};

// Class used to store settings for a song file.
class CSoundFilePlayConfig
{
public:
	CSoundFilePlayConfig(void);
	~CSoundFilePlayConfig(void);

	void SetMixLevels(int mixLevelType);

//getters/setters
	float getIntToFloat() const;
	void setIntToFloat(float);
	float getFloatToInt() const;
	void setFloatToInt(float);

	// default VSTi gain factor, different depending on the MPT version we're "emulating"
	void setVSTiAttenuation(float);
	float getVSTiAttenuation() const;

	// user-controllable VSTi gain factor.
	void setVSTiVolume(float);
	float getVSTiVolume() const;

	void setGlobalVolumeAppliesToMaster(bool);
	bool getGlobalVolumeAppliesToMaster() const;
	
	void setUseGlobalPreAmp(bool);
	bool getUseGlobalPreAmp() const;

	void setForcePanningMode(forcePanningMode);
	forcePanningMode getForcePanningMode() const;

	void setDisplayDBValues(bool);
	bool getDisplayDBValues() const;

	// Extra sample attenuation in bits
	void setExtraSampleAttenuation(int);
	int getExtraSampleAttenuation() const;

	//Values at which volumes are unchanged
	double getNormalSamplePreAmp() const;
	double getNormalVSTiVol() const;
	double getNormalGlobalVol() const;
	void setNormalSamplePreAmp(double);
	void setNormalVSTiVol(double);
	void setNormalGlobalVol(double);

private:

//calculated internally (getters only):
	float getVSTiGainFactor() const;

	float m_IntToFloat;
	float m_FloatToInt;
	float m_VSTiAttenuation;
	float m_VSTiVolume;

	double m_normalSamplePreAmp;
	double m_normalVSTiVol;
	double m_normalGlobalVol;

	bool m_globalVolumeAppliesToMaster;
	bool m_ignorePreAmp;
	forcePanningMode m_forceSoftPanning;
	bool m_displayDBValues;

	int m_extraAttenuation;
};

