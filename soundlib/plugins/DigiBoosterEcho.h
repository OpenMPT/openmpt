/*
 * DigiBoosterEcho.h
 * -----------------
 * Purpose: Implementation of the DigiBooster Pro Echo DSP
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#ifndef NO_PLUGINS

#include "PlugInterface.h"

OPENMPT_NAMESPACE_BEGIN

//=======================================
class DigiBoosterEcho : public IMixPlugin
//=======================================
{
public:
	enum Parameters
	{
		kEchoDelay = 0,
		kEchoFeedback,
		kEchoMix,
		kEchoCross,
		kEchoNumParameters
	};

	// Our settings chunk for file I/O, as it will be written to files
	struct PluginChunk
	{
		char  id[4];
		uint8 param[kEchoNumParameters];

		PluginChunk(uint8 delay = 80, uint8 feedback = 150, uint8 mix = 80, uint8 cross = 255)
		{
			memcpy(id, "Echo", 4);
			param[kEchoDelay] = delay;
			param[kEchoFeedback] = feedback;
			param[kEchoMix] = mix;
			param[kEchoCross] = cross;

			STATIC_ASSERT(sizeof(PluginChunk) == 8);
		}
	};

protected:
	std::vector<float> m_delayLine;	// Echo delay line
	uint32 m_bufferSize;			// Delay line length in frames
	uint32 m_writePos;				// Current write position in the delay line
	uint32 m_delayTime;				// In frames
	uint32 m_sampleRate;

	// Echo calculation coefficients
	float m_PMix, m_NMix;
	float m_PCrossPBack, m_PCrossNBack;
	float m_NCrossPBack, m_NCrossNBack;

	// Settings chunk for file I/O
	PluginChunk chunk;

public:
	DigiBoosterEcho(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);

	virtual void Release() { delete this; }
	virtual void SaveAllParameters();
	virtual void RestoreAllParameters(int32 program);
	virtual int32 GetUID() const { int32 id; memcpy(&id, "Echo", 4); return id; }
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

	virtual PlugParamIndex GetNumParameters() const { return kEchoNumParameters; }
	virtual PlugParamValue GetParameter(PlugParamIndex index);
	virtual void SetParameter(PlugParamIndex index, PlugParamValue value);

	virtual void Resume();
	virtual void Suspend() { }
	virtual bool IsInstrument() const { return false; }
	virtual bool CanRecieveMidiEvents() { return false; }
	virtual bool ShouldProcessSilence() { return true; }

#ifdef MODPLUG_TRACKER
	virtual CString GetDefaultEffectName() { return _T("Echo"); }

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

	virtual int GetNumInputChannels() const {return 2; }
	virtual int GetNumOutputChannels() const {return 2; }

	virtual bool ProgramsAreChunks() const { return true; }

	virtual size_t GetChunk(char *(&data), bool);
	virtual void SetChunk(size_t size, char *data, bool);

protected:
	void RecalculateEchoParams();
};

OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
