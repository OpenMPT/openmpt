/*
 * WavesReverb.h
 * -------------
 * Purpose: Implementation of the DMO WavesReverb DSP (for non-Windows platforms)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#if !defined(NO_PLUGINS) && defined(NO_DMO)

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
	virtual bool MidiSend(uint32) { return true; }
	virtual bool MidiSysexSend(const void *, uint32) { return true; }
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
	virtual void Suspend() { m_isResumed = false; }
	virtual void PositionChanged() { }

	virtual bool IsInstrument() const { return false; }
	virtual bool CanRecieveMidiEvents() { return false; }
	virtual bool ShouldProcessSilence() { return true; }

#ifdef MODPLUG_TRACKER
	virtual CString GetDefaultEffectName() { return _T("WavesReverb"); }

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
	static float GainInDecibel(float param) { return -96.0f + param * 96.0f; }
	float ReverbTime() const { return 0.001f + m_param[kRvbReverbTime] * 2999.999f; }
	float HighFreqRTRatio() const { return 0.001f + m_param[kRvbHighFreqRTRatio] * 0.998f; }
	void RecalculateWavesReverbParams();
};

} // namespace DMO

OPENMPT_NAMESPACE_END

#endif // !NO_PLUGINS && NO_DMO
