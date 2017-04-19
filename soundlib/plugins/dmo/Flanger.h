/*
 * Flanger.h
 * ---------
 * Purpose: Implementation of the DMO Flanger DSP (for non-Windows platforms)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifndef NO_PLUGINS

#include "Chorus.h"

OPENMPT_NAMESPACE_BEGIN

namespace DMO
{

//===========================
class Flanger : public Chorus
//===========================
{
protected:
	enum Parameters
	{
		kFlangerWetDryMix = 0,
		kFlangerWaveShape,
		kFlangerFrequency,
		kFlangerDepth,
		kFlangerPhase,
		kFlangerFeedback,
		kFlangerDelay,
		kFlangerNumParameters
	};

public:
	static IMixPlugin* Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);
	Flanger(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);

	virtual void Release() { delete this; }
	virtual int32 GetUID() const { return 0xEFCA3D92; }

	virtual PlugParamIndex GetNumParameters() const { return kFlangerNumParameters; }
	virtual void SetParameter(PlugParamIndex index, PlugParamValue value);

#ifdef MODPLUG_TRACKER
	virtual CString GetDefaultEffectName() { return _T("Flanger"); }

	virtual CString GetParamName(PlugParamIndex param);
	virtual CString GetParamLabel(PlugParamIndex);
	virtual CString GetParamDisplay(PlugParamIndex param);
#endif

protected:
	virtual float WetDryMix() const { return m_param[kFlangerWetDryMix]; }
	virtual bool IsTriangle() const { return m_param[kFlangerWaveShape] < 1; }
	virtual float Depth() const { return m_param[kFlangerDepth]; }
	virtual float Feedback() const { return -99.0f + m_param[kFlangerFeedback] * 198.0f; }
	virtual float Delay() const { return m_param[kFlangerDelay] * 4.0f; }
	virtual float FrequencyInHertz() const { return m_param[kFlangerFrequency] * 10.0f; }
	virtual int Phase() const { return Util::Round<uint32>(m_param[kFlangerPhase] * 4.0f); }
};

} // namespace DMO

OPENMPT_NAMESPACE_END

#endif // !NO_PLUGINS
