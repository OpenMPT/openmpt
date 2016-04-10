/*
 * Distortion.h
 * ------------
 * Purpose: Implementation of the DMO Distortion DSP (for non-Windows platforms)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#if !defined(NO_PLUGINS) //&& defined(NO_DMO)

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
	uint32 m_edge;

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
	virtual bool MidiSend(uint32) { return true; }
	virtual bool MidiSysexSend(const char *, uint32) { return true; }
	virtual void MidiCC(uint8, MIDIEvents::MidiCC, uint8, CHANNELINDEX) { }
	virtual void MidiPitchBend(uint8, int32, int8) { }
	virtual void MidiVibrato(uint8, int32, int8) { }
	virtual void MidiCommand(uint8, uint8, uint16, uint16, uint16, CHANNELINDEX) { }
	virtual void HardAllNotesOff() { }
	virtual bool IsNotePlaying(uint32, uint32, uint32) { return false; }

	virtual int32 GetNumPrograms() const { return 0; }
	virtual int32 GetCurrentProgram() { return 0; }
	virtual void SetCurrentProgram(int32) { }

	virtual PlugParamIndex GetNumParameters() const { return kDistNumParameters; }
	virtual PlugParamValue GetParameter(PlugParamIndex index);
	virtual void SetParameter(PlugParamIndex index, PlugParamValue value);

	virtual void Resume();
	virtual void Suspend() { }
	virtual bool IsInstrument() const { return false; }
	virtual bool CanRecieveMidiEvents() { return false; }
	virtual bool ShouldProcessSilence() { return true; }

#ifdef MODPLUG_TRACKER
	virtual CString GetDefaultEffectName() { return _T("Distortion"); }

	virtual void CacheProgramNames(int32, int32) { }
	virtual void CacheParameterNames(int32, int32) { }

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

	virtual bool ProgramsAreChunks() const { return false; }

	virtual size_t GetChunk(char *(&), bool) { return 0; }
	virtual void SetChunk(size_t, char *, bool) { }

protected:
	static float FreqInHertz(float param) { return 100.0f + param * 7900.0f; }
	float GainInDecibel() const { return -60.0f + m_param[kDistGain] * 60.0f; }
	float BandwidthInSemitones() const { return 1.0f + m_param[kDistPostEQBandwidth] * 35.0f; }	// TODO
	void RecalculateDistortionParams();
};

} // namespace DMO

OPENMPT_NAMESPACE_END

#endif // !NO_PLUGINS && NO_DMO
