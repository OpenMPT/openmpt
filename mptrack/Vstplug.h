/*
 * Vstplug.h
 * ---------
 * Purpose: Plugin handling (loading and processing plugins)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifndef NO_VST

#define VST_FORCE_DEPRECATED 0
#include <pluginterfaces/vst2.x/aeffectx.h>			// VST

#include "../soundlib/Snd_defs.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "../soundlib/plugins/PluginEventQueue.h"
#include "../soundlib/Mixer.h"

OPENMPT_NAMESPACE_BEGIN


class CSoundFile;
struct SNDMIXPLUGIN;
struct VSTPluginLib;


//==================================
class CVstPlugin: public IMidiPlugin
//==================================
{
protected:

	enum
	{
		// Number of MIDI events that can be sent to a plugin at once (the internal queue is not affected by this number, it can hold any number of events)
		vstNumProcessEvents = 256,
	};

	HINSTANCE m_hLibrary;
	AEffect &m_Effect;
	void (*m_pProcessFP)(AEffect*, float**, float**, VstInt32); //Function pointer to AEffect processReplacing if supported, else process.

	double lastBarStartPos;
	uint32 m_nSampleRate;
	bool m_bIsVst2 : 1;
	bool m_bIsInstrument : 1;
	bool m_isInitialized : 1;
	bool m_bNeedIdle : 1;
	bool m_positionChanged : 1;

	PluginEventQueue<vstNumProcessEvents> vstEvents;	// MIDI events that should be sent to the plugin

	static VstTimeInfo timeInfo;

public:
	const bool isBridged : 1;		// True if our built-in plugin bridge is being used.

public:
	CVstPlugin(HINSTANCE hLibrary, VSTPluginLib &factory, SNDMIXPLUGIN &mixPlugin, AEffect &effect, CSoundFile &sndFile);
	virtual ~CVstPlugin();

	static AEffect *LoadPlugin(VSTPluginLib &plugin, HINSTANCE &library, bool forceBridge);

protected:
	void Initialize();

public:
	virtual int32 GetUID() const;
	virtual int32 GetVersion() const;
	virtual void Idle();
	virtual uint32 GetLatency() const { return m_Effect.initialDelay; }

	// Check if programs should be stored as chunks or parameters
	virtual bool ProgramsAreChunks() const { return (m_Effect.flags & effFlagsProgramChunks) != 0; }
	virtual size_t GetChunk(char *(&chunk), bool isBank);
	virtual void SetChunk(size_t size, char *chunk, bool isBank);
	// If true, the plugin produces an output even if silence is being fed into it.
	virtual bool ShouldProcessSilence() { return IsInstrument() || ((m_Effect.flags & effFlagsNoSoundInStop) == 0 && Dispatch(effGetTailSize, 0, 0, nullptr, 0.0f) != 1); }

	virtual int32 GetNumPrograms() const;
	virtual int32 GetCurrentProgram();
	virtual void SetCurrentProgram(int32 nIndex);

	virtual PlugParamIndex GetNumParameters() const;
	virtual PlugParamValue GetParameter(PlugParamIndex nIndex);
	virtual void SetParameter(PlugParamIndex nIndex, PlugParamValue fValue);

	virtual CString GetCurrentProgramName();
	virtual void SetCurrentProgramName(const CString &name);
	virtual CString GetProgramName(int32 program);

	virtual CString GetParamName(PlugParamIndex param);
	virtual CString GetParamLabel(PlugParamIndex param) { return GetParamPropertyString(param, effGetParamLabel); };
	virtual CString GetParamDisplay(PlugParamIndex param) { return GetParamPropertyString(param, effGetParamDisplay); };

	VstIntPtr Dispatch(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt);

	virtual bool HasEditor() const { return (m_Effect.flags & effFlagsHasEditor) != 0; }
	virtual CAbstractVstEditor *OpenEditor();
	virtual CString GetDefaultEffectName();

	virtual void Bypass(bool bypass = true);

	virtual bool IsInstrument() const;
	virtual bool CanRecieveMidiEvents();

	virtual void CacheProgramNames(int32 firstProg, int32 lastProg);
	virtual void CacheParameterNames(int32 firstParam, int32 lastParam);

public:
	virtual void Release();
	virtual void SaveAllParameters();
	virtual void RestoreAllParameters(int32 program);
	virtual void Process(float *pOutL, float *pOutR, uint32 numFrames);
	virtual bool MidiSend(uint32 dwMidiCode);
	virtual bool MidiSysexSend(const char *message, uint32 length);
	virtual void HardAllNotesOff();
	virtual void NotifySongPlaying(bool playing);

	virtual void Resume();
	virtual void Suspend();
	virtual void PositionChanged() { m_positionChanged = true; }

	// Check whether a VST parameter can be automated
	bool CanAutomateParameter(PlugParamIndex index);

	virtual int GetNumInputChannels() const { return m_Effect.numInputs; }
	virtual int GetNumOutputChannels() const { return m_Effect.numOutputs; }

	virtual void BeginSetProgram(int32 program);
	virtual void EndSetProgram();

protected:
	// Helper function for retreiving parameter name / label / display
	CString GetParamPropertyString(VstInt32 param, VstInt32 opcode);

	// Set up input / output buffers.
	bool InitializeIOBuffers();

	// Process incoming and outgoing VST events.
	void ProcessVSTEvents();
	void ReceiveVSTEvents(const VstEvents *events);

	void ReportPlugException(std::wstring text) const;

public:
	static VstIntPtr VSTCALLBACK MasterCallBack(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
protected:
	VstIntPtr VstFileSelector(bool destructor, VstFileSelect *fileSel);
};


OPENMPT_NAMESPACE_END

#endif // NO_VST