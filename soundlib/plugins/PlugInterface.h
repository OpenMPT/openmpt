/*
 * PlugInterface.h
 * ---------------
 * Purpose: Interface class and helpers for plugin handling
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

// For VstInt32 and stuff - a stupid workaround.
#ifndef NO_VST
#define VST_FORCE_DEPRECATED 0
#include <pluginterfaces/vst2.x/aeffect.h>			// VST
#else
typedef OPENMPT_NAMESPACE::int32 VstInt32;
typedef intptr_t VstIntPtr;
#endif

#include "../../soundlib/Snd_defs.h"
#include "../../common/misc_util.h"
#include "../../soundlib/MIDIEvents.h"
#include "../../common/Endianness.h"
#include "../../soundlib/Mixer.h"

OPENMPT_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////
// Mix Plugins

typedef VstInt32 PlugParamIndex;
typedef float PlugParamValue;

//==============
class IMixPlugin
//==============
{
protected:
	virtual ~IMixPlugin() {}
public:
	virtual void Release() = 0;
	virtual void SaveAllParameters() = 0;
	virtual void RestoreAllParameters(long nProg=-1) = 0; //rewbs.plugDefaultProgram: added param
	virtual void Process(float *pOutL, float *pOutR, size_t nSamples) = 0;
	virtual float RenderSilence(size_t numSamples) = 0;
	virtual bool MidiSend(uint32 dwMidiCode) = 0;
	virtual bool MidiSysexSend(const char *message, uint32 length) = 0;
	virtual void MidiCC(uint8 nMidiCh, MIDIEvents::MidiCC nController, uint8 nParam, CHANNELINDEX trackChannel) = 0;
	virtual void MidiPitchBend(uint8 nMidiCh, int32 increment, int8 pwd) = 0;
	virtual void MidiVibrato(uint8 nMidiCh, int32 depth, int8 pwd) = 0;
	virtual void MidiCommand(uint8 nMidiCh, uint8 nMidiProg, uint16 wMidiBank, uint16 note, uint16 vol, CHANNELINDEX trackChannel) = 0;
	virtual void HardAllNotesOff() = 0;		//rewbs.VSTCompliance
	virtual void RecalculateGain() = 0;
	virtual bool isPlaying(UINT note, UINT midiChn, UINT trackerChn) = 0; //rewbs.VSTiNNA
	virtual PlugParamIndex GetNumParameters() = 0;
	virtual void SetParameter(PlugParamIndex paramindex, PlugParamValue paramvalue) = 0;
	virtual void SetZxxParameter(UINT nParam, UINT nValue) = 0;
	virtual PlugParamValue GetParameter(PlugParamIndex nIndex) = 0;
	virtual UINT GetZxxParameter(UINT nParam) = 0; //rewbs.smoothVST
	virtual void ModifyParameter(PlugParamIndex nIndex, PlugParamValue diff);
	virtual void AutomateParameter(PlugParamIndex param) = 0;
	virtual VstIntPtr Dispatch(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt) =0; //rewbs.VSTCompliance
	virtual void NotifySongPlaying(bool) = 0;	//rewbs.VSTCompliance
	virtual bool IsSongPlaying() const = 0;
	virtual bool IsResumed() = 0;
	virtual void Resume() = 0;
	virtual void Suspend() = 0;
	virtual void Bypass(bool = true) = 0;
	virtual bool IsBypassed() const = 0;
	bool ToggleBypass() { Bypass(!IsBypassed()); return IsBypassed(); };
	virtual bool isInstrument() = 0;
	virtual bool CanRecieveMidiEvents() = 0;
	virtual void SetDryRatio(UINT param) = 0;
	virtual bool ShouldProcessSilence() = 0;
	virtual void ResetSilence() = 0;
	virtual void SetEditorPos(int32 x, int32 y) = 0;
	virtual void GetEditorPos(int32 &x, int32 &y) const = 0;
	virtual int GetNumInputChannels() const = 0;
	virtual int GetNumOutputChannels() const = 0;
};


inline void IMixPlugin::ModifyParameter(PlugParamIndex nIndex, PlugParamValue diff)
//---------------------------------------------------------------------------------
{
	float val = GetParameter(nIndex) + diff;
	Limit(val, PlugParamValue(0), PlugParamValue(1));
	SetParameter(nIndex, val);
}


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

	VstInt32 dwPluginId1;			// Plugin type (kEffectMagic, kDmoMagic, kBuzzMagic)
	VstInt32 dwPluginId2;			// Plugin unique ID
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
	uint32 nPluginDataSize;
	char *pPluginData;
	SNDMIXPLUGININFO Info;
	float fDryRatio;
	VstInt32 defaultProgram;
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

class CSoundFile;
typedef bool (*PMIXPLUGINCREATEPROC)(SNDMIXPLUGIN &, CSoundFile &);


OPENMPT_NAMESPACE_END
