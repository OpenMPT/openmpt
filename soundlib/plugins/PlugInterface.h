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
typedef int32 VstInt32;
typedef intptr_t VstIntPtr;
#endif

#include "Snd_defs.h"
#include "../common/misc_util.h"

////////////////////////////////////////////////////////////////////
// Mix Plugins

typedef VstInt32 PlugParamIndex;
typedef float PlugParamValue;

//==============
class IMixPlugin
//==============
{
public:
	virtual int AddRef() = 0;
	virtual int Release() = 0;
	virtual void SaveAllParameters() = 0;
	virtual void RestoreAllParameters(long nProg=-1) = 0; //rewbs.plugDefaultProgram: added param
	virtual void Process(float *pOutL, float *pOutR, size_t nSamples) = 0;
	virtual void Init(unsigned long nFreq, int bReset) = 0;
	virtual bool MidiSend(DWORD dwMidiCode) = 0;
	virtual void MidiCC(UINT nMidiCh, UINT nController, UINT nParam, UINT trackChannel) = 0;
	virtual void MidiPitchBend(UINT nMidiCh, int nParam, UINT trackChannel) = 0;
	virtual void MidiCommand(UINT nMidiCh, UINT nMidiProg, WORD wMidiBank, UINT note, UINT vol, UINT trackChan) = 0;
	virtual void HardAllNotesOff() = 0;		//rewbs.VSTCompliance
	virtual void RecalculateGain() = 0;		
	virtual bool isPlaying(UINT note, UINT midiChn, UINT trackerChn) = 0; //rewbs.VSTiNNA
	virtual bool MoveNote(UINT note, UINT midiChn, UINT sourceTrackerChn, UINT destTrackerChn) = 0; //rewbs.VSTiNNA
	virtual void SetParameter(PlugParamIndex paramindex, PlugParamValue paramvalue) = 0;
	virtual void SetZxxParameter(UINT nParam, UINT nValue) = 0;
	virtual PlugParamValue GetParameter(PlugParamIndex nIndex) = 0;
	virtual UINT GetZxxParameter(UINT nParam) = 0; //rewbs.smoothVST 
	virtual void ModifyParameter(PlugParamIndex nIndex, PlugParamValue diff);
	virtual void AutomateParameter(PlugParamIndex param) = 0;
	virtual VstIntPtr Dispatch(VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt) =0; //rewbs.VSTCompliance
	virtual void NotifySongPlaying(bool) = 0;	//rewbs.VSTCompliance
	virtual bool IsSongPlaying() = 0;
	virtual bool IsResumed() = 0;
	virtual void Resume() = 0;
	virtual void Suspend() = 0;
	virtual void Bypass(bool = true) = 0;
	virtual bool IsBypassed() const = 0;
	bool ToggleBypass() { Bypass(!IsBypassed()); return IsBypassed(); };
	virtual bool isInstrument() = 0;
	virtual bool CanRecieveMidiEvents() = 0;
	virtual void SetDryRatio(UINT param) = 0;

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
		psfMixReady = 0x01,			// Set when cleared
	};

	DWORD dwFlags;					// PluginStateFlags
	LONG nVolDecayL, nVolDecayR;	// Buffer click removal
	int *pMixBuffer;				// Stereo effect send buffer
	float *pOutBufferL;				// Temp storage for int -> float conversion
	float *pOutBufferR;
};


struct SNDMIXPLUGININFO
{
	// dwInputRouting flags
	enum RoutingFlags
	{
		irApplyToMaster	= 0x01,		// Apply to master mix
		irBypass		= 0x02,		// Bypass effect
		irWetMix		= 0x04,		// Wet Mix (dry added)
		irExpandMix		= 0x08,		// [0%,100%] -> [-200%,200%]
	};

	VstInt32 dwPluginId1;			// Plugin type (kEffectMagic, kDmoMagic, kBuzzMagic)
	VstInt32 dwPluginId2;			// Plugin unique ID
	uint32 dwInputRouting;			// Bits 0 to 7 = RoutingFlags, bits 8 - 15 = mixing mode, bits 16-23 = gain
	uint32 dwOutputRouting;			// 0 = send to master 0x80 + x = send to plugin x
	uint32 dwReserved[4];			// Reserved for routing info
	CHAR szName[32];				// User-chosen plugin name
	CHAR szLibraryName[64];			// original DLL name

	// Should only be called from SNDMIXPLUGIN::SetBypass() and IMixPlugin::Bypass()
	void SetBypass(bool bypass = true) { if(bypass) dwInputRouting |= irBypass; else dwInputRouting &= ~irBypass; };

};

STATIC_ASSERT(sizeof(SNDMIXPLUGININFO) == 128);	// this is directly written to files, so the size must be correct!

struct SNDMIXPLUGIN
{
	IMixPlugin *pMixPlugin;
	SNDMIXPLUGINSTATE *pMixState;
	ULONG nPluginDataSize;
	void *pPluginData;
	SNDMIXPLUGININFO Info;
	float fDryRatio;		    // rewbs.dryRatio [20040123]
	long defaultProgram;		// rewbs.plugDefaultProgram

	const char *GetName() const
		{ return Info.szName; }
	const char *GetLibraryName() const
		{ return Info.szLibraryName; };
	CString GetParamName(PlugParamIndex index) const;

	// Check if a plugin is loaded into this slot (also returns true if the plugin in this slot has not been found)
	bool IsValidPlugin() const { return (Info.dwPluginId1 | Info.dwPluginId2) != 0; };

	// Input routing getters
	int GetGain() const
		{ return (Info.dwInputRouting >> 16) & 0xFF; };
	int GetMixMode() const
		{ return (Info.dwInputRouting >> 8) & 0xFF; };
	bool IsMasterEffect() const
		{ return (Info.dwInputRouting & SNDMIXPLUGININFO::irApplyToMaster) != 0; };
	bool IsWetMix() const
		{ return (Info.dwInputRouting & SNDMIXPLUGININFO::irWetMix) != 0; };
	bool IsExpandedMix() const
		{ return (Info.dwInputRouting & SNDMIXPLUGININFO::irExpandMix) != 0; };
	bool IsBypassed() const
		{ return (Info.dwInputRouting & SNDMIXPLUGININFO::irBypass) != 0; };

	// Input routing setters
	void SetGain(int gain)
		{ Info.dwInputRouting = (Info.dwInputRouting & 0xFF00FFFF) | ((gain & 0xFF) << 16); if(pMixPlugin != nullptr) pMixPlugin->RecalculateGain(); };
	void SetMixMode(int mixMode)
		{ Info.dwInputRouting = (Info.dwInputRouting & 0xFFFF00FF) | ((mixMode & 0xFF) << 8); };
	void SetMasterEffect(bool master = true)
		{ if(master) Info.dwInputRouting |= SNDMIXPLUGININFO::irApplyToMaster; else Info.dwInputRouting &= ~SNDMIXPLUGININFO::irApplyToMaster; };
	void SetWetMix(bool wetMix = true)
		{ if(wetMix) Info.dwInputRouting |= SNDMIXPLUGININFO::irWetMix; else Info.dwInputRouting &= ~SNDMIXPLUGININFO::irWetMix; };
	void SetExpandedMix(bool expanded = true)
		{ if(expanded) Info.dwInputRouting |= SNDMIXPLUGININFO::irExpandMix; else Info.dwInputRouting &= ~SNDMIXPLUGININFO::irExpandMix; };
	void SetBypass(bool bypass = true)
		{ if(pMixPlugin != nullptr) pMixPlugin->Bypass(bypass); else Info.SetBypass(bypass); };

	// Output routing getters
	bool IsOutputToMaster() const
		{ return Info.dwOutputRouting == 0; };
	PLUGINDEX GetOutputPlugin() const
		{ return Info.dwOutputRouting >= 0x80 ? static_cast<PLUGINDEX>(Info.dwOutputRouting - 0x80) : PLUGINDEX_INVALID; };

	// Output routing setters
	void SetOutputToMaster()
		{ Info.dwOutputRouting = 0; };
	void SetOutputPlugin(PLUGINDEX plugin)
		{ if(plugin < MAX_MIXPLUGINS) Info.dwOutputRouting = plugin + 0x80; else Info.dwOutputRouting = 0; };

}; // rewbs.dryRatio: Hopefully this doesn't need to be a fixed size.

class CSoundFile;
typedef	BOOL (__cdecl *PMIXPLUGINCREATEPROC)(SNDMIXPLUGIN *, CSoundFile *);
