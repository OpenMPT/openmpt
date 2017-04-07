/*
 * Chorus.h
 * --------
 * Purpose: Implementation of the DMO Chorus DSP (for non-Windows platforms)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifndef NO_PLUGINS

#include "../PlugInterface.h"

OPENMPT_NAMESPACE_BEGIN

namespace DMO
{

//==============================
class Chorus : public IMixPlugin
//==============================
{
public:
	enum Parameters
	{
		kChorusWetDryMix = 0,
		kChorusDepth,
		kChorusFrequency,
		kChorusWaveShape,
		kChorusPhase,
		kChorusFeedback,
		kChorusDelay,
		kChorusNumParameters
	};

protected:
	float m_param[kChorusNumParameters];

	// Calculated parameters
	float m_waveShapeMin, m_waveShapeMax, m_waveShapeVal;
	float m_depthDelay;
	float m_frequency;
	int32 m_delayOffset;

	// State
	std::vector<float> m_buffer;
	int32 m_bufPos, m_bufSize;

	int32 m_delayL1, m_delayL2, m_delayR1, m_delayR2;

public:
	static IMixPlugin* Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);
	Chorus(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);

	virtual void Release() { delete this; }
	virtual int32 GetUID() const { return 0xEFE6629C; }
	virtual int32 GetVersion() const { return 0; }
	virtual void Idle() { }
	virtual uint32 GetLatency() const { return 0; }

	virtual void Process(float *pOutL, float *pOutR, uint32 numFrames);

	virtual float RenderSilence(uint32) { return 0.0f; }

	virtual int32 GetNumPrograms() const { return 0; }
	virtual int32 GetCurrentProgram() { return 0; }
	virtual void SetCurrentProgram(int32) { }

	virtual PlugParamIndex GetNumParameters() const { return kChorusNumParameters; }
	virtual PlugParamValue GetParameter(PlugParamIndex index);
	virtual void SetParameter(PlugParamIndex index, PlugParamValue value);

	virtual void Resume();
	virtual void Suspend() { m_isResumed = false; }
	virtual void PositionChanged() { }
	virtual bool IsInstrument() const { return false; }
	virtual bool CanRecieveMidiEvents() { return false; }
	virtual bool ShouldProcessSilence() { return true; }

#ifdef MODPLUG_TRACKER
	virtual CString GetDefaultEffectName() { return _T("Chorus"); }

	virtual CString GetParamName(PlugParamIndex param);
	virtual CString GetParamLabel(PlugParamIndex);
	virtual CString GetParamDisplay(PlugParamIndex param);

	virtual CString GetCurrentProgramName() { return CString(); }
	virtual void SetCurrentProgramName(const CString &) { }
	virtual CString GetProgramName(int32) { return CString(); }

	virtual bool HasEditor() const { return false; }
#endif

	virtual void BeginSetProgram(int32) { }
	virtual void EndSetProgram() { }

	virtual int GetNumInputChannels() const { return 2; }
	virtual int GetNumOutputChannels() const { return 2; }

protected:
	int32 GetBufferIntOffset(int32 fpOffset) const;

	virtual float WetDryMix() const { return m_param[kChorusWetDryMix]; }
	virtual bool IsTriangle() const { return m_param[kChorusWaveShape] < 0.5f; }
	virtual float Depth() const { return m_param[kChorusDepth]; }
	virtual float Feedback() const { return -99.0f + m_param[kChorusFeedback] * 198.0f; }
	virtual float Delay() const { return m_param[kChorusDelay] * 20.0f; }
	virtual float FrequencyInHertz() const { return m_param[kChorusFrequency] * 10.0f; }
	virtual int Phase() const { return Util::Round<uint32>(m_param[kChorusPhase] * 4.0f); }
	void RecalculateChorusParams();
};

} // namespace DMO

OPENMPT_NAMESPACE_END

#endif // !NO_PLUGINS
