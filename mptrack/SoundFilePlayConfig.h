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
	plugmix_mode_original = 0,
	plugmix_mode_117RC1   = 1,
	plugmix_mode_117RC2   = 2,
	plugmix_mode_Test   = 3,
};

// Class used to store settings for a song file.
class CSoundFilePlayConfig
{
public:
	CSoundFilePlayConfig(void);
	~CSoundFilePlayConfig(void);

	void SetPluginMixLevels(int mixLevelType);

//getters/setters
	float getIntToFloat();
	void setIntToFloat(float);
	float getFloatToInt();
	void setFloatToInt(float);

	void setVSTiAttenuation(float);
	float getVSTiAttenuation();
	void setVSTiVolume(float);
	float getVSTiVolume();

	void setGlobalVolumeAppliesToMaster(bool);
	bool getGlobalVolumeAppliesToMaster();
	

//calculated internally (getters only):	
	float getVSTiGainFactor();



//
private:
	float m_IntToFloat;
	float m_FloatToInt;
	float m_VSTiAttenuation;	// default VSTi gain factor, different depending on the MPT version we're "emulating"
	float m_VSTiVolume;			// user-controllable VSTi gain factor.
	float m_VSTiGainFactor;		// always m_VSTiAttenuation * m_VSTiVolume. No public setter.

	bool m_bGlobalVolumeAppliesToMaster;

	DWORD m_LastSavedWithVersion;
	DWORD m_CreatedWithVersion;
};
