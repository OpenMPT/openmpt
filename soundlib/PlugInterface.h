/*
 * PlugInterface.h
 * ---------------
 * Purpose: Interface class and helpers for plugin handling
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */

#pragma once
#ifndef PLUGINTERFACE_H
#define PLUGINTERFACE_H

// For VstInt32 and stuff - a stupid workaround.
#ifndef NO_VST
#define VST_FORCE_DEPRECATED 0
#include <aeffect.h>			// VST
#else
typedef int32 VstInt32;
typedef intptr_t VstIntPtr;
#endif

#include "../common/misc_util.h"

////////////////////////////////////////////////////////////////////
// Mix Plugins

#define MIXPLUG_MIXREADY			0x01	// Set when cleared

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
	virtual bool Bypass(bool = true) = 0;
	virtual bool IsBypassed() const = 0;
	bool ToggleBypass() { return Bypass(!IsBypassed()); };
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


///////////////////////////////////////////////////
// !!! bits 8 -> 15 reserved for mixing mode !!! //
///////////////////////////////////////////////////
#define MIXPLUG_INPUTF_MASTEREFFECT				0x01	// Apply to master mix
#define MIXPLUG_INPUTF_BYPASS					0x02	// Bypass effect
#define MIXPLUG_INPUTF_WETMIX					0x04	// Wet Mix (dry added)
// -> CODE#0028
// -> DESC="effect plugin mixing mode combo"
#define MIXPLUG_INPUTF_MIXEXPAND				0x08	// [0%,100%] -> [-200%,200%]
// -! BEHAVIOUR_CHANGE#0028


struct SNDMIXPLUGINSTATE
{
	DWORD dwFlags;					// MIXPLUG_XXXX
	LONG nVolDecayL, nVolDecayR;	// Buffer click removal
	int *pMixBuffer;				// Stereo effect send buffer
	float *pOutBufferL;				// Temp storage for int -> float conversion
	float *pOutBufferR;
};
typedef SNDMIXPLUGINSTATE* PSNDMIXPLUGINSTATE;

struct SNDMIXPLUGININFO
{
	DWORD dwPluginId1;
	DWORD dwPluginId2;
	DWORD dwInputRouting;	// MIXPLUG_INPUTF_XXXX, bits 16-23 = gain
	DWORD dwOutputRouting;	// 0=mix 0x80+=fx
	DWORD dwReserved[4];	// Reserved for routing info
	CHAR szName[32];
	CHAR szLibraryName[64];	// original DLL name
}; // Size should be 128 							
typedef SNDMIXPLUGININFO* PSNDMIXPLUGININFO;
STATIC_ASSERT(sizeof(SNDMIXPLUGININFO) == 128);	// this is directly written to files, so the size must be correct!

struct SNDMIXPLUGIN
{
	const char* GetName() const { return Info.szName; }
	const char* GetLibraryName();
	CString GetParamName(const UINT index) const;
	bool Bypass(bool bypass);
	bool IsBypassed() const { return (Info.dwInputRouting & MIXPLUG_INPUTF_BYPASS) != 0; };

	IMixPlugin *pMixPlugin;
	PSNDMIXPLUGINSTATE pMixState;
	ULONG nPluginDataSize;
	PVOID pPluginData;
	SNDMIXPLUGININFO Info;
	float fDryRatio;		    // rewbs.dryRatio [20040123]
	long defaultProgram;		// rewbs.plugDefaultProgram
}; // rewbs.dryRatio: Hopefully this doesn't need to be a fixed size.
typedef SNDMIXPLUGIN* PSNDMIXPLUGIN;

class CSoundFile;
typedef	BOOL (__cdecl *PMIXPLUGINCREATEPROC)(PSNDMIXPLUGIN, CSoundFile*);

#endif // PLUGINTERFACE_H
