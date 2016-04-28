/*
 * DMOPlugin.h
 * -----------
 * Purpose: DirectX Media Object plugin handling / processing.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#ifndef NO_DMO

#include "../PlugInterface.h"
#include <dmoreg.h>
#include <strmif.h>

typedef interface IMediaObject IMediaObject;
typedef interface IMediaObjectInPlace IMediaObjectInPlace;
typedef interface IMediaParamInfo IMediaParamInfo;
typedef interface IMediaParams IMediaParams;

OPENMPT_NAMESPACE_BEGIN

//=================================
class DMOPlugin : public IMixPlugin
//=================================
{
protected:
	IMediaObject *m_pMediaObject;
	IMediaObjectInPlace *m_pMediaProcess;
	IMediaParamInfo *m_pParamInfo;
	IMediaParams *m_pMediaParams;

	uint32 m_nSamplesPerSec;
	uint32 m_uid;
	union
	{
		int16 *i16;
		float *f32;
	} m_alignedBuffer;
	union
	{
		int16 i16[MIXBUFFERSIZE * 2 + 16];		// 16-bit PCM Stereo interleaved
		float f32[MIXBUFFERSIZE * 2 + 16];		// 32-bit Float Stereo interleaved
	} m_interleavedBuffer;
	bool m_useFloat;

public:
	static IMixPlugin* Create(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);

protected:
	DMOPlugin(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct, IMediaObject *pMO, IMediaObjectInPlace *pMOIP, uint32 uid);
	~DMOPlugin();

public:
	virtual void Release() { delete this; }
	virtual int32 GetUID() const { return m_uid; }
	virtual int32 GetVersion() const { return 2; }
	virtual void Idle() { }
	virtual uint32 GetLatency() const;

	virtual void Process(float *pOutL, float *pOutR, uint32 numFrames);

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
	virtual void SetCurrentProgram(int32 /*nIndex*/) { }

	virtual PlugParamIndex GetNumParameters() const;
	virtual PlugParamValue GetParameter(PlugParamIndex index);
	virtual void SetParameter(PlugParamIndex index, PlugParamValue value);

	virtual void Resume();
	virtual void Suspend();
	virtual void PositionChanged() { }

	virtual bool IsInstrument() const { return false; }
	virtual bool CanRecieveMidiEvents() { return false; }
	virtual bool ShouldProcessSilence() { return true; }

#ifdef MODPLUG_TRACKER
	virtual CString GetDefaultEffectName() { return CString(); }

	virtual void CacheProgramNames(int32, int32) { }
	virtual void CacheParameterNames(int32, int32) { }

	virtual CString GetParamName(PlugParamIndex param);
	virtual CString GetParamLabel(PlugParamIndex param);
	virtual CString GetParamDisplay(PlugParamIndex param);

	// TODO we could simply add our own preset mechanism. But is it really useful for these plugins?
	virtual CString GetCurrentProgramName() { return CString(); }
	virtual void SetCurrentProgramName(const CString &) { }
	virtual CString GetProgramName(int32) { return CString(); }

	virtual bool HasEditor() const { return false; }
#endif

	virtual void BeginSetProgram(int32) { }
	virtual void EndSetProgram() { }

	virtual int GetNumInputChannels() const {return 2; }
	virtual int GetNumOutputChannels() const {return 2; }

	virtual bool ProgramsAreChunks() const { return false; }
	virtual size_t GetChunk(char *(&), bool) { return 0; }
	virtual void SetChunk(size_t, char *, bool) { }
};

OPENMPT_NAMESPACE_END

#endif // NO_DMO

