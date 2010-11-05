#pragma once

enum {
	NO_ATTENUATION = 1,
	MIXING_CLIPMIN = -0x07FFFFFF,
	MIXING_CLIPMAX = 0x07FFFFFF,
};

enum {
	tempo_mode_classic      = 0,
	tempo_mode_alternative  = 1,
	tempo_mode_modern       = 2,
};

enum {
	mixLevels_original		= 0,
	mixLevels_117RC1		= 1,
	mixLevels_117RC2		= 2,
	mixLevels_117RC3		= 3,
	mixLevels_compatible	= 4,
	mixLevels_Test			= 5,
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
	float getIntToFloat();
	void setIntToFloat(float);
	float getFloatToInt();
	void setFloatToInt(float);

	// default VSTi gain factor, different depending on the MPT version we're "emulating"
	void setVSTiAttenuation(float);
	float getVSTiAttenuation();

	// user-controllable VSTi gain factor.
	void setVSTiVolume(float);
	float getVSTiVolume();

    void setGlobalVolumeAppliesToMaster(bool);
	bool getGlobalVolumeAppliesToMaster();
	
	void setUseGlobalPreAmp(bool);
	bool getUseGlobalPreAmp();

	void setForcePanningMode(forcePanningMode);
	forcePanningMode getForcePanningMode();

	void setDisplayDBValues(bool);
	bool getDisplayDBValues();

	// Extra sample attenuation in bits
	void setExtraSampleAttenuation(int);
	int getExtraSampleAttenuation();

	//Values at which volumes are unchanged
	double getNormalSamplePreAmp();
	double getNormalVSTiVol();
	double getNormalGlobalVol();
	void setNormalSamplePreAmp(double);
	void setNormalVSTiVol(double);
	void setNormalGlobalVol(double);

private:

//calculated internally (getters only):	
	float getVSTiGainFactor();

	float m_IntToFloat;
	float m_FloatToInt;
	float m_VSTiAttenuation;	
	float m_VSTiVolume;			

	float m_normalSamplePreAmp;
	float m_normalVSTiVol;
	float m_normalGlobalVol;

	bool m_globalVolumeAppliesToMaster;
	bool m_ignorePreAmp;
	forcePanningMode m_forceSoftPanning;
	bool m_displayDBValues;

	int m_extraAttenuation;

	DWORD m_LastSavedWithVersion;
	DWORD m_CreatedWithVersion;
};

