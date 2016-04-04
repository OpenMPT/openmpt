/*
 * ParamEq.h
 * ---------
 * Purpose: Implementation of the DMO Parametric Equalizer DSP (for non-Windows platforms)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#if !defined(NO_PLUGINS) && defined(NO_DMO)

#include "../PlugInterface.h"

OPENMPT_NAMESPACE_BEGIN

namespace DMO
{

//===============================
class ParamEq : public IMixPlugin
//===============================
{
public:
	enum Parameters
	{
		kEqCenter = 0,
		kEqBandwidth,
		kEqGain,
		kEqNumParameters
	};

protected:
	float m_param[kEqNumParameters];
	uint32 m_sampleRate;

	// Equalizer coefficients
	float b0DIVa0, b1DIVa0, b2DIVa0, a1DIVa0, a2DIVa0;
	// Equalizer memory
	float x1[2], x2[2];
	float y1[2], y2[2];

public:
	static IMixPlugin* Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);
	ParamEq(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);

	virtual void Release() { delete this; }
	virtual int32 GetUID() const { return 0x120CED89; }
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

	virtual PlugParamIndex GetNumParameters() const { return kEqNumParameters; }
	virtual PlugParamValue GetParameter(PlugParamIndex index);
	virtual void SetParameter(PlugParamIndex index, PlugParamValue value);

	virtual void Resume();
	virtual void Suspend() { }
	virtual bool IsInstrument() const { return false; }
	virtual bool CanRecieveMidiEvents() { return false; }
	virtual bool ShouldProcessSilence() { return true; }

#ifdef MODPLUG_TRACKER
	virtual CString GetDefaultEffectName() { return _T("ParamEq"); }

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
	float BandwidthInSemitones() const { return 1.0f + m_param[kEqBandwidth] * 35.0f; }
	float FreqInHertz() const { return 80.0f + m_param[kEqCenter] * 15920.0f; }
	float GainInDecibel() const { return (m_param[kEqGain] - 0.5f) * 30.0f; }
	void RecalculateEqParams();
};

} // namespace DMO

OPENMPT_NAMESPACE_END

#endif // !NO_PLUGINS && NO_DMO
