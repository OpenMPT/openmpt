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

//
private:
	float m_IntToFloat;
	float m_FloatToInt;
	float m_VSTiAttenuation;
	DWORD m_LastSavedWithVersion;
	DWORD m_CreatedWithVersion;
};
