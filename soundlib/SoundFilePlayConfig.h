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

OPENMPT_NAMESPACE_BEGIN

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
	mixLevels_original			= 0,
	mixLevels_117RC1			= 1,
	mixLevels_117RC2			= 2,
	mixLevels_117RC3			= 3,
	mixLevels_compatible		= 4,
	mixLevels_compatible_FT2	= 5,
};

enum forcePanningMode
{
	dontForcePanningMode,
	forceSoftPanning,
	forceNoSoftPanning,
	forceFT2Panning,
};

// Class used to store settings for a song file.
class CSoundFilePlayConfig
{
public:
	CSoundFilePlayConfig(void);
	~CSoundFilePlayConfig(void);

	void SetMixLevels(int mixLevelType);

//getters/setters
	bool getGlobalVolumeAppliesToMaster() const { return m_globalVolumeAppliesToMaster; }
	void setGlobalVolumeAppliesToMaster(bool inGlobalVolumeAppliesToMaster) { m_globalVolumeAppliesToMaster=inGlobalVolumeAppliesToMaster; }

	// user-controllable VSTi gain factor.
	float getVSTiVolume() const { return m_VSTiVolume; }
	void  setVSTiVolume(float inVSTiVolume) { m_VSTiVolume = inVSTiVolume; }

	// default VSTi gain factor, different depending on the MPT version we're "emulating"
	float getVSTiAttenuation() const { return m_VSTiAttenuation; }
	void  setVSTiAttenuation(float inVSTiAttenuation) { m_VSTiAttenuation = inVSTiAttenuation; }

	float getIntToFloat() const { return m_IntToFloat; }
	void  setIntToFloat(float inIntToFloat) { m_IntToFloat = inIntToFloat; }

	float getFloatToInt() const { return m_FloatToInt; }
	void  setFloatToInt(float inFloatToInt) { m_FloatToInt = inFloatToInt; }

	bool getUseGlobalPreAmp() const { return m_ignorePreAmp; }
	void setUseGlobalPreAmp(bool inUseGlobalPreAmp) { m_ignorePreAmp = inUseGlobalPreAmp; }

	forcePanningMode getForcePanningMode() const { return m_forceSoftPanning; }
	void setForcePanningMode(forcePanningMode inForceSoftPanning) { m_forceSoftPanning = inForceSoftPanning; }

	bool getDisplayDBValues() const { return m_displayDBValues; }
	void setDisplayDBValues(bool in) { m_displayDBValues = in; }

	// Values at which volumes are unchanged
	double getNormalSamplePreAmp() const { return m_normalSamplePreAmp; }
	void setNormalSamplePreAmp(double in) { m_normalSamplePreAmp = in; }
	double getNormalVSTiVol() const { return m_normalVSTiVol; }
	void setNormalVSTiVol(double in) { m_normalVSTiVol = in; }
	double getNormalGlobalVol() const { return m_normalGlobalVol; }
	void setNormalGlobalVol(double in) { m_normalGlobalVol = in; }

	// Extra sample attenuation in bits
	int getExtraSampleAttenuation() const { return m_extraAttenuation; }
	void setExtraSampleAttenuation(int attn) { m_extraAttenuation = attn; }

protected:

	float m_IntToFloat;
	float m_FloatToInt;
	float m_VSTiAttenuation;
	float m_VSTiVolume;

	double m_normalSamplePreAmp;
	double m_normalVSTiVol;
	double m_normalGlobalVol;

	int m_extraAttenuation;
	forcePanningMode m_forceSoftPanning;
	bool m_globalVolumeAppliesToMaster;
	bool m_ignorePreAmp;
	bool m_displayDBValues;
};

OPENMPT_NAMESPACE_END
