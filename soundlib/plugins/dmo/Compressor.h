/*
 * Compressor.h
 * -------------
 * Purpose: Implementation of the DMO Compressor DSP (for non-Windows platforms)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#if !defined(NO_PLUGINS) && defined(NO_DMO)

#include "../PlugInterface.h"

OPENMPT_NAMESPACE_BEGIN

namespace DMO
{

//==================================
class Compressor : public IMixPlugin
//==================================
{
public:
	enum Parameters
	{
		kCompGain = 0,
		kCompAttack,
		kCompRelease,
		kCompThreshold,
		kCompRatio,
		kCompPredelay,
		kCompNumParameters
	};

protected:
	float m_param[kCompNumParameters];

	// Calculated parameters and coefficients
	float m_gain;
	float m_attack;
	float m_release;
	float m_threshold;
	float m_ratio;
	int32 m_predelay;

	// State
	std::vector<float> m_buffer;
	int32 m_bufPos, m_bufSize;
	float m_peak;

public:
	static IMixPlugin* Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);
	Compressor(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);

	virtual void Release() { delete this; }
	virtual int32 GetUID() const { return 0xEF011F79; }
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

	virtual PlugParamIndex GetNumParameters() const { return kCompNumParameters; }
	virtual PlugParamValue GetParameter(PlugParamIndex index);
	virtual void SetParameter(PlugParamIndex index, PlugParamValue value);

	virtual void Resume();
	virtual void Suspend() { m_isResumed = false; }
	virtual void PositionChanged() { }
	virtual bool IsInstrument() const { return false; }
	virtual bool CanRecieveMidiEvents() { return false; }
	virtual bool ShouldProcessSilence() { return true; }

#ifdef MODPLUG_TRACKER
	virtual CString GetDefaultEffectName() { return _T("Compressor"); }

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
	float GainInDecibel() const { return -60.0f + m_param[kCompGain] * 120.0f; }
	float AttackTime() const { return 0.01f + m_param[kCompAttack] * 499.99f; }
	float ReleaseTime() const { return 50.0f + m_param[kCompRelease] * 2950.0f; }
	float ThresholdInDecibel() const { return -60.0f + m_param[kCompThreshold] * 60.0f; }
	float CompressorRatio() const { return 1.0f + m_param[kCompRatio] * 99.0f; }
	float PreDelay() const { return m_param[kCompPredelay] * 4.0f; }
	void RecalculateCompressorParams();
};

} // namespace DMO

OPENMPT_NAMESPACE_END

#endif // !NO_PLUGINS && NO_DMO
