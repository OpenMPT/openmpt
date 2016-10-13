/*
 * Distortion.h
 * ------------
 * Purpose: Implementation of the DMO Distortion DSP (for non-Windows platforms)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#ifndef NO_PLUGINS

#include "../PlugInterface.h"

OPENMPT_NAMESPACE_BEGIN

namespace DMO
{

//==================================
class Distortion : public IMixPlugin
//==================================
{
public:
	enum Parameters
	{
		kDistGain = 0,
		kDistEdge,
		kDistPreLowpassCutoff,
		kDistPostEQCenterFrequency,
		kDistPostEQBandwidth,
		kDistNumParameters
	};

protected:
	float m_param[kDistNumParameters];

	// Pre-EQ coefficients
	float m_preEQz1[2], m_preEQb1, m_preEQa0;
	// Post-EQ coefficients
	float m_postEQz1[2], m_postEQz2[2], m_postEQa0, m_postEQb0, m_postEQb1;
	uint8 m_edge, m_shift;

public:
	static IMixPlugin* Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);
	Distortion(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);

	virtual void Release() { delete this; }
	virtual int32 GetUID() const { return 0xEF114C90; }
	virtual int32 GetVersion() const { return 0; }
	virtual void Idle() { }
	virtual uint32 GetLatency() const { return 0; }

	virtual void Process(float *pOutL, float *pOutR, uint32 numFrames);

	virtual float RenderSilence(uint32) { return 0.0f; }

	virtual int32 GetNumPrograms() const { return 0; }
	virtual int32 GetCurrentProgram() { return 0; }
	virtual void SetCurrentProgram(int32) { }

	virtual PlugParamIndex GetNumParameters() const { return kDistNumParameters; }
	virtual PlugParamValue GetParameter(PlugParamIndex index);
	virtual void SetParameter(PlugParamIndex index, PlugParamValue value);

	virtual void Resume();
	virtual void Suspend() { m_isResumed = false; }
	virtual void PositionChanged() { }
	virtual bool IsInstrument() const { return false; }
	virtual bool CanRecieveMidiEvents() { return false; }
	virtual bool ShouldProcessSilence() { return true; }

#ifdef MODPLUG_TRACKER
	virtual CString GetDefaultEffectName() { return _T("Distortion"); }

	virtual CString GetParamName(PlugParamIndex param);
	virtual CString GetParamLabel(PlugParamIndex);
	virtual CString GetParamDisplay(PlugParamIndex param);

	virtual CString GetCurrentProgramName() { return CString(); }
	virtual void SetCurrentProgramName(const CString &) { }
	virtual CString GetProgramName(int32) { return CString(); }

	virtual bool HasEditor() const { return false; }
#endif

	virtual int GetNumInputChannels() const { return 2; }
	virtual int GetNumOutputChannels() const { return 2; }

protected:
	static float FreqInHertz(float param) { return 100.0f + param * 7900.0f; }
	float GainInDecibel() const { return -60.0f + m_param[kDistGain] * 60.0f; }
	void RecalculateDistortionParams();
};

} // namespace DMO

OPENMPT_NAMESPACE_END

#endif // !NO_PLUGINS
