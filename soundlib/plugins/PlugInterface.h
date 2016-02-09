/*
 * PlugInterface.h
 * ---------------
 * Purpose: Interface class and helpers for plugin handling
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../../soundlib/Snd_defs.h"
#include "../../common/misc_util.h"
#include "../../soundlib/MIDIEvents.h"
#include "../../common/Endianness.h"
#include "../../soundlib/Mixer.h"

OPENMPT_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////
// Mix Plugins

typedef int32 PlugParamIndex;
typedef float PlugParamValue;

struct VSTPluginLib;
struct SNDMIXPLUGIN;
class CSoundFile;
class CModDoc;
class CAbstractVstEditor;


#ifndef NO_PLUGINS

struct SNDMIXPLUGINSTATE
{
	// dwFlags flags
	enum PluginStateFlags
	{
		psfMixReady = 0x01,				// Set when cleared
		psfHasInput = 0x02,				// Set when plugin has non-silent input
		psfSilenceBypass = 0x04,		// Bypass because of silence detection
	};

	mixsample_t *pMixBuffer;			// Stereo effect send buffer
	float *pOutBufferL;					// Temp storage for int -> float conversion
	float *pOutBufferR;
	uint32 dwFlags;						// PluginStateFlags
	uint32 inputSilenceCount;			// How much silence has been processed? (for plugin auto-turnoff)
	mixsample_t nVolDecayL, nVolDecayR;	// End of sample click removal

	SNDMIXPLUGINSTATE() { memset(this, 0, sizeof(*this)); }

	void ResetSilence()
	{
		dwFlags |= psfHasInput;
		dwFlags &= ~psfSilenceBypass;
		inputSilenceCount = 0;
	}
};


//==============
class IMixPlugin
//==============
{
	friend class CAbstractVstEditor;

protected:
	IMixPlugin *m_pNext, *m_pPrev;
	VSTPluginLib &m_Factory;
	CSoundFile &m_SndFile;
	SNDMIXPLUGIN *m_pMixStruct;
#ifdef MODPLUG_TRACKER
	CAbstractVstEditor *m_pEditor;
#endif // MODPLUG_TRACKER
	SNDMIXPLUGINSTATE m_MixState;
	PLUGINDEX m_nSlot;

public:
	bool m_bRecordAutomation : 1;
	bool m_bPassKeypressesToPlug : 1;
	bool m_bRecordMIDIOut : 1;

protected:
	virtual ~IMixPlugin()
	{
		if (m_pNext) m_pNext->m_pPrev = m_pPrev;
		if (m_pPrev) m_pPrev->m_pNext = m_pNext;
		m_pPrev = nullptr;
		m_pNext = nullptr;
	}

	void InsertIntoFactoryList();

public:
	// Non-virtual part of the interface
	IMixPlugin(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);
	inline CSoundFile &GetSoundFile() { return m_SndFile; }
	inline const CSoundFile &GetSoundFile() const { return m_SndFile; }

#ifdef MODPLUG_TRACKER
	CModDoc *GetModDoc();
	const CModDoc *GetModDoc() const;

	CAbstractVstEditor *GetEditor() { return m_pEditor; }
	const CAbstractVstEditor *GetEditor() const { return m_pEditor; }
#endif // MODPLUG_TRACKER

	inline VSTPluginLib &GetPluginFactory() const { return m_Factory; }
	inline IMixPlugin *GetNextInstance() const { return m_pNext; }

	PLUGINDEX FindSlot() const;
	inline void SetSlot(PLUGINDEX slot) { m_nSlot = slot; }
	inline PLUGINDEX GetSlot() const { return m_nSlot; }

	inline void UpdateMixStructPtr(SNDMIXPLUGIN *p) { m_pMixStruct = p; }

	virtual void Release() = 0;
	virtual int32 GetUID() const = 0;
	virtual int32 GetVersion() const = 0;
	virtual void Idle() = 0;
	virtual int32 GetNumPrograms() = 0;
	virtual int32 GetCurrentProgram() = 0;
	virtual void SetCurrentProgram(int32 nIndex) = 0;
	virtual PlugParamIndex GetNumParameters() = 0;
	virtual void SaveAllParameters() = 0;
	virtual void RestoreAllParameters(long nProg=-1) = 0;
	virtual void Process(float *pOutL, float *pOutR, size_t nSamples) = 0;
	virtual float RenderSilence(size_t numSamples) = 0;
	virtual bool MidiSend(uint32 dwMidiCode) = 0;
	virtual bool MidiSysexSend(const char *message, uint32 length) = 0;
	virtual void MidiCC(uint8 nMidiCh, MIDIEvents::MidiCC nController, uint8 nParam, CHANNELINDEX trackChannel) = 0;
	virtual void MidiPitchBend(uint8 nMidiCh, int32 increment, int8 pwd) = 0;
	virtual void MidiVibrato(uint8 nMidiCh, int32 depth, int8 pwd) = 0;
	virtual void MidiCommand(uint8 nMidiCh, uint8 nMidiProg, uint16 wMidiBank, uint16 note, uint16 vol, CHANNELINDEX trackChannel) = 0;
	virtual void HardAllNotesOff() = 0;
	virtual void RecalculateGain() = 0;
	virtual bool IsPlaying(uint32 note, uint32 midiChn, uint32 trackerChn) = 0;
	virtual void SetParameter(PlugParamIndex paramindex, PlugParamValue paramvalue) = 0;
	virtual PlugParamValue GetParameter(PlugParamIndex nIndex) = 0;
	// Modify parameter by given amount. Only needs to be re-implemented if plugin architecture allows this to be performed atomically.
	virtual void ModifyParameter(PlugParamIndex nIndex, PlugParamValue diff);
	virtual void NotifySongPlaying(bool) = 0;
	virtual bool IsSongPlaying() const = 0;
	virtual bool IsResumed() const = 0;
	virtual void Resume() = 0;
	virtual void Suspend() = 0;
	virtual void Bypass(bool = true) = 0;
	virtual bool IsBypassed() const = 0;
	bool ToggleBypass() { Bypass(!IsBypassed()); return IsBypassed(); };
	virtual bool IsInstrument() const = 0;
	virtual bool CanRecieveMidiEvents() = 0;
	virtual void SetDryRatio(uint32 param) = 0;
	virtual bool ShouldProcessSilence() = 0;
	virtual void ResetSilence() = 0;

	size_t GetOutputPlugList(std::vector<IMixPlugin *> &list);
	size_t GetInputPlugList(std::vector<IMixPlugin *> &list);
	size_t GetInputInstrumentList(std::vector<INSTRUMENTINDEX> &list);
	size_t GetInputChannelList(std::vector<CHANNELINDEX> &list);

#ifdef MODPLUG_TRACKER
	virtual bool SaveProgram() = 0;
	virtual bool LoadProgram(mpt::PathString fileName = mpt::PathString()) = 0;

	virtual CString GetDefaultEffectName() = 0;

	virtual void CacheProgramNames(int32 firstProg, int32 lastProg) = 0;
	virtual void CacheParameterNames(int32 firstParam, int32 lastParam) = 0;

	virtual CString GetParamName(PlugParamIndex param) = 0;
	virtual CString GetParamLabel(PlugParamIndex param) = 0;
	virtual CString GetParamDisplay(PlugParamIndex param) = 0;
	CString GetFormattedParamName(PlugParamIndex param);
	CString GetFormattedParamValue(PlugParamIndex param);
	virtual CString GetCurrentProgramName() = 0;
	virtual void SetCurrentProgramName(const CString &name) = 0;
	virtual CString GetProgramName(int32 program) = 0;
	CString GetFormattedProgramName(int32 index);

	virtual bool HasEditor() const = 0;
	virtual void ToggleEditor() = 0;
	void SetEditorPos(int32 x, int32 y);
	void GetEditorPos(int32 &x, int32 &y) const;

	void AutomateParameter(PlugParamIndex param);

	virtual void BeginSetProgram(int32 program = -1) = 0;
	virtual void EndSetProgram() = 0;
#endif

	virtual int GetNumInputChannels() const = 0;
	virtual int GetNumOutputChannels() const = 0;

	virtual bool ProgramsAreChunks() const = 0;
	virtual size_t GetChunk(char *(&chunk), bool isBank) = 0;
	virtual void SetChunk(size_t size, char *chunk, bool isBank) = 0;
};


inline void IMixPlugin::ModifyParameter(PlugParamIndex nIndex, PlugParamValue diff)
//---------------------------------------------------------------------------------
{
	float val = GetParameter(nIndex) + diff;
	Limit(val, PlugParamValue(0), PlugParamValue(1));
	SetParameter(nIndex, val);
}


// IMidiPlugin: Default implementation of plugins with MIDI input

//===================================
class IMidiPlugin : public IMixPlugin
//===================================
{
protected:
	enum
	{
		// Pitch wheel constants
		vstPitchBendShift	= 12,		// Use lowest 12 bits for fractional part and vibrato flag => 16.11 fixed point precision
		vstPitchBendMask	= (~1),
		vstVibratoFlag		= 1,
	};

	struct PlugInstrChannel
	{
		int32  midiPitchBendPos;		// Current Pitch Wheel position, in 16.11 fixed point format. Lowest bit is used for indicating that vibrato was applied. Vibrato offset itself is not stored in this value.
		uint16 currentProgram;
		uint16 currentBank;
		uint8  noteOnMap[128][MAX_CHANNELS];

		void ResetProgram() { currentProgram = 0; currentBank = 0; }
	};

	PlugInstrChannel m_MidiCh[16];	// MIDI channel state

public:
	IMidiPlugin(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct);

	virtual void MidiCC(uint8 nMidiCh, MIDIEvents::MidiCC nController, uint8 nParam, CHANNELINDEX trackChannel);
	virtual void MidiPitchBend(uint8 nMidiCh, int32 increment, int8 pwd);
	virtual void MidiVibrato(uint8 nMidiCh, int32 depth, int8 pwd);
	virtual void MidiCommand(uint8 nMidiCh, uint8 nMidiProg, uint16 wMidiBank, uint16 note, uint16 vol, CHANNELINDEX trackChannel);
	virtual bool IsPlaying(uint32 note, uint32 midiChn, uint32 trackerChn);

protected:
	// Converts a 14-bit MIDI pitch bend position to our internal pitch bend position representation
	static int32 EncodePitchBendParam(int32 position) { return (position << vstPitchBendShift); }
	// Converts the internal pitch bend position to a 14-bit MIDI pitch bend position
	static int16 DecodePitchBendParam(int32 position) { return static_cast<int16>(position >> vstPitchBendShift); }
	// Apply Pitch Wheel Depth (PWD) to some MIDI pitch bend value.
	static inline void ApplyPitchWheelDepth(int32 &value, int8 pwd);

	void MidiPitchBend(uint8 nMidiCh, int32 pitchBendPos);

};


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED SNDMIXPLUGININFO
{
	// dwInputRouting flags
	enum RoutingFlags
	{
		irApplyToMaster	= 0x01,		// Apply to master mix
		irBypass		= 0x02,		// Bypass effect
		irWetMix		= 0x04,		// Wet Mix (dry added)
		irExpandMix		= 0x08,		// [0%,100%] -> [-200%,200%]
		irAutoSuspend	= 0x10,		// Plugin will automatically suspend on silence
	};

	int32 dwPluginId1;				// Plugin type (kEffectMagic, kDmoMagic, kBuzzMagic)
	int32 dwPluginId2;				// Plugin unique ID
	uint8 routingFlags;				// See RoutingFlags
	uint8 mixMode;
	uint8 gain;						// Divide by 10 to get real gain
	uint8 reserved;
	uint32 dwOutputRouting;			// 0 = send to master 0x80 + x = send to plugin x
	uint32 dwReserved[4];			// Reserved for routing info
	char szName[32];				// User-chosen plugin name - this is locale ANSI!
	char szLibraryName[64];			// original DLL name - this is UTF-8!

	// Should only be called from SNDMIXPLUGIN::SetBypass() and IMixPlugin::Bypass()
	void SetBypass(bool bypass = true) { if(bypass) routingFlags |= irBypass; else routingFlags &= ~irBypass; }

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(dwPluginId1);
		SwapBytesLE(dwPluginId2);
		SwapBytesLE(dwOutputRouting);
		SwapBytesLE(dwReserved[0]);
		SwapBytesLE(dwReserved[1]);
		SwapBytesLE(dwReserved[2]);
		SwapBytesLE(dwReserved[3]);
	}

};

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif

STATIC_ASSERT(sizeof(SNDMIXPLUGININFO) == 128);	// this is directly written to files, so the size must be correct!


struct SNDMIXPLUGIN
{
	IMixPlugin *pMixPlugin;
	SNDMIXPLUGINSTATE *pMixState;
	char *pPluginData;
	uint32 nPluginDataSize;
	SNDMIXPLUGININFO Info;
	float fDryRatio;
	int32 defaultProgram;
	int32 editorX, editorY;

	const char *GetName() const
		{ return Info.szName; }
	const char *GetLibraryName() const
		{ return Info.szLibraryName; }
	std::string GetParamName(PlugParamIndex index) const;

	// Check if a plugin is loaded into this slot (also returns true if the plugin in this slot has not been found)
	bool IsValidPlugin() const { return (Info.dwPluginId1 | Info.dwPluginId2) != 0; };

	// Input routing getters
	uint8 GetGain() const
		{ return Info.gain; }
	uint8 GetMixMode() const
		{ return Info.mixMode; }
	bool IsMasterEffect() const
		{ return (Info.routingFlags & SNDMIXPLUGININFO::irApplyToMaster) != 0; }
	bool IsWetMix() const
		{ return (Info.routingFlags & SNDMIXPLUGININFO::irWetMix) != 0; }
	bool IsExpandedMix() const
		{ return (Info.routingFlags & SNDMIXPLUGININFO::irExpandMix) != 0; }
	bool IsBypassed() const
		{ return (Info.routingFlags & SNDMIXPLUGININFO::irBypass) != 0; }
	bool IsAutoSuspendable() const
		{ return (Info.routingFlags & SNDMIXPLUGININFO::irAutoSuspend) != 0; }

	// Input routing setters
	void SetGain(uint8 gain)
		{ Info.gain = gain; if(pMixPlugin != nullptr) pMixPlugin->RecalculateGain(); }
	void SetMixMode(uint8 mixMode)
		{ Info.mixMode = mixMode; }
	void SetMasterEffect(bool master = true)
		{ if(master) Info.routingFlags |= SNDMIXPLUGININFO::irApplyToMaster; else Info.routingFlags &= ~SNDMIXPLUGININFO::irApplyToMaster; }
	void SetWetMix(bool wetMix = true)
		{ if(wetMix) Info.routingFlags |= SNDMIXPLUGININFO::irWetMix; else Info.routingFlags &= ~SNDMIXPLUGININFO::irWetMix; }
	void SetExpandedMix(bool expanded = true)
		{ if(expanded) Info.routingFlags |= SNDMIXPLUGININFO::irExpandMix; else Info.routingFlags &= ~SNDMIXPLUGININFO::irExpandMix; }
	void SetBypass(bool bypass = true)
		{ if(pMixPlugin != nullptr) pMixPlugin->Bypass(bypass); else Info.SetBypass(bypass); }
	void SetAutoSuspend(bool suspend = true)
		{ if(suspend) Info.routingFlags |= SNDMIXPLUGININFO::irAutoSuspend; else Info.routingFlags &= ~SNDMIXPLUGININFO::irAutoSuspend; }

	// Output routing getters
	bool IsOutputToMaster() const
		{ return Info.dwOutputRouting == 0; }
	PLUGINDEX GetOutputPlugin() const
		{ return Info.dwOutputRouting >= 0x80 ? static_cast<PLUGINDEX>(Info.dwOutputRouting - 0x80) : PLUGINDEX_INVALID; }

	// Output routing setters
	void SetOutputToMaster()
		{ Info.dwOutputRouting = 0; }
	void SetOutputPlugin(PLUGINDEX plugin)
		{ if(plugin < MAX_MIXPLUGINS) Info.dwOutputRouting = plugin + 0x80; else Info.dwOutputRouting = 0; }

	void Destroy()
	{
		delete[] pPluginData;
		pPluginData = nullptr;
		nPluginDataSize = 0;

		pMixState = nullptr;
		if(pMixPlugin)
		{
			pMixPlugin->Release();
			pMixPlugin = nullptr;
		}
	}
};

typedef bool (*PMIXPLUGINCREATEPROC)(SNDMIXPLUGIN &, CSoundFile &);

#else

class IMixPlugin;
struct SNDMIXPLUGIN;
struct SNDMIXPLUGINSTATE;

#endif // NO_PLUGINS

OPENMPT_NAMESPACE_END
