/*
 * WavesReverb.h
 * -------------
 * Purpose: Implementation of the DMO WavesReverb DSP (for non-Windows platforms)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#ifndef NO_PLUGINS

#include "../PlugInterface.h"

OPENMPT_NAMESPACE_BEGIN

namespace DMO
{

//===================================
class WavesReverb : public IMixPlugin
//===================================
{
public:
	enum Parameters
	{
		kRvbInGain = 0,
		kRvbReverbMix,
		kRvbReverbTime,
		kRvbHighFreqRTRatio,
		kDistNumParameters
	};

protected:
	float m_param[kDistNumParameters];

	// Parameters and coefficients
	float m_dryFactor;
	float m_wetFactor;
	float m_coeffs[10];
	uint32 m_delay[6];

	// State
	struct ReverbState
	{
		uint32 combPos, allpassPos;
		float comb[16384];
		float allpass1[2048];
		float allpass2[2048];
	} m_state;

public:
	static IMixPlugin* Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);
	WavesReverb(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);

	virtual void Release() { delete this; }
	virtual int32 GetUID() const { return 0x87FC0268; }
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
	virtual CString GetDefaultEffectName() { return _T("WavesReverb"); }

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
	static float GainInDecibel(float param) { return -96.0f + param * 96.0f; }
	float ReverbTime() const { return 0.001f + m_param[kRvbReverbTime] * 2999.999f; }
	float HighFreqRTRatio() const { return 0.001f + m_param[kRvbHighFreqRTRatio] * 0.998f; }
	void RecalculateWavesReverbParams();
};

} // namespace DMO

OPENMPT_NAMESPACE_END

#endif // !NO_PLUGINS
