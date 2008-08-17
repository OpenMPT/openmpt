#ifndef MIDI_H
#define MIDI_H

#include <vector>
#include <algorithm>
using std::vector;

class CMainFrame;

int ApplyVolumeRelatedMidiSettings(const DWORD& dwParam1, const BYTE midivolume);
void ApplyTransposeKeyboardSetting(CMainFrame& rMainFrm, DWORD& dwParam1);
inline BYTE GetFromMIDIMsg_Channel(const DWORD MIDImsg) {return static_cast<BYTE>((MIDImsg & 0xF));}
inline BYTE GetFromMIDIMsg_Event(const DWORD MIDImsg) {return static_cast<BYTE>(((MIDImsg >> 4) & 0xF));}
inline BYTE GetFromMIDIMsg_DataByte1(const DWORD MIDImsg) {return static_cast<BYTE>(((MIDImsg >> 8) & 0xFF));}
inline BYTE GetFromMIDIMsg_DataByte2(const DWORD MIDImsg) {return static_cast<BYTE>(((MIDImsg >> 16) & 0xFF));}

enum
{
	MIDIEVENT_NOTEOFF			= 0x8,
	MIDIEVENT_NOTEON			= 0x9,
	MIDIEVENT_CONTROLLERCHANGE	= 0xB,
	MIDIEVENT_PITCHBEND			= 0xE,
};

struct MODMIDICFG
{
	CHAR szMidiGlb[9*32];
	CHAR szMidiSFXExt[16*32];
	CHAR szMidiZXXExt[128*32];
};
typedef MODMIDICFG* LPMODMIDICFG;

enum {
	MIDIOUT_START=0,
	MIDIOUT_STOP,
	MIDIOUT_TICK,
	MIDIOUT_NOTEON,
	MIDIOUT_NOTEOFF,
	MIDIOUT_VOLUME,
	MIDIOUT_PAN,
	MIDIOUT_BANKSEL,
	MIDIOUT_PROGRAM,
};


//=========================
class CMIDIMappingDirective
//=========================
{
public:
	CMIDIMappingDirective() :
	  m_Active(true), m_CaptureMIDI(false), m_AllowPatternEdit(true), m_AnyChannel(true),
		  m_ChnEvent(0xB << 4), m_MIDIByte1(0), m_PluginIndex(1), m_Parameter(0) 
	  {}

	void SetActive(const bool b) {m_Active = b;}
	bool IsActive() const {return m_Active;}

	void SetCaptureMIDI(const bool b) {m_CaptureMIDI = b;}
	bool GetCaptureMIDI() const {return m_CaptureMIDI;}

	void SetAllowPatternEdit(const bool b) {m_AllowPatternEdit = b;}
	bool GetAllowPatternEdit() const {return m_AllowPatternEdit;}

	bool GetAnyChannel() const {return m_AnyChannel;}

	//Note: In these functions, channel value is in range [1,16],
	//GetChannel() returns 0 on 'any channel'.
	void SetChannel(const int c){if(c < 1 || c > 16) m_AnyChannel = true; else {m_ChnEvent &= ~0xF; m_ChnEvent |= c-1; m_AnyChannel = false;}}
	BYTE GetChannel() const {return (m_AnyChannel) ? 0 : (m_ChnEvent & 0xF) + 1;} 

	void SetEvent(BYTE e) {if(e > 15) e = 15; m_ChnEvent &= ~0xF0; m_ChnEvent |= (e << 4);}
	BYTE GetEvent() const {return static_cast<BYTE>((m_ChnEvent >> 4) & 0xF);}

	void SetController(int controller) {if(controller > 127) controller = 127; m_MIDIByte1 = static_cast<BYTE>(controller);}
	BYTE GetController() const {return m_MIDIByte1;}
	
	//Note: Plug index starts from 1.
	void SetPlugIndex(const int i) {m_PluginIndex = static_cast<BYTE>(i);}
	BYTE GetPlugIndex() const {return m_PluginIndex;}

	void SetParamIndex(const int i) {m_Parameter = i;}
	uint32 GetParamIndex() const {return m_Parameter;}

	bool operator==(const CMIDIMappingDirective& d) {return memcmp(this, &d, sizeof(CMIDIMappingDirective)) == 0;}

	CString ToString() const
	{
		CString str; str.Preallocate(20);
		char flags[3] = "00";
		if(m_Active) flags[0] = '1';
		if(m_CaptureMIDI) flags[1] = '1';
		//if(m_AllowPatternEdit) flags[2] = '1';
		str.Format("%s:%d:%x:%d:%d:%d", flags, (int)GetChannel(), (int)GetEvent(), (int)GetController(), (int)m_PluginIndex, m_Parameter);
		str.Trim();
		return str;
	}

	BYTE GetChnEvent() const {return m_ChnEvent;}

private:
	bool m_Active;
	bool m_CaptureMIDI; //When true, MIDI data should not be processed beyond this directive
	bool m_AllowPatternEdit; //When true, the mapping can be used for modifying pattern.
	bool m_AnyChannel;
	uint8 m_ChnEvent; //0-3 channel, 4-7 event
	BYTE m_MIDIByte1;
	BYTE m_PluginIndex;
	uint32 m_Parameter;
};

class CSoundFile;
inline bool operator<(const CMIDIMappingDirective& a, const CMIDIMappingDirective& b) {return a.GetController() < b.GetController();}
inline bool operator<(const CMIDIMappingDirective& d, const BYTE& ctrlVal) {return d.GetController() < ctrlVal;}
inline bool operator<(const BYTE& ctrlVal, const CMIDIMappingDirective& d) {return ctrlVal < d.GetController();}

//===============
class CMIDIMapper
//===============
{
public:
	typedef vector<CMIDIMappingDirective>::const_iterator const_iterator;
	CMIDIMapper(CSoundFile& sndfile) : m_rSndFile(sndfile) {}

	//If mapping found:
	//	-mappedIndex is set to mapped value(plug index)
	//	-paramindex to mapped parameter
	//	-paramvalue to parameter value.
	//In case of multiple mappings, these get the values from the last mapping found.
	//Returns true if MIDI was 'captured' by some directive, false otherwise.
	bool OnMIDImsg(const DWORD midimsg, BYTE& mappedIndex, uint32& paramindex, BYTE& paramvalue);

	//Swaps the positions of two elements. Returns true if swap was not done.
	bool Swap(const size_t a, const size_t b);

	//Return the index after sorting for the added element
	size_t SetDirective(const size_t i, const CMIDIMappingDirective& d) {m_Directives[i] = d; Sort(); return std::find(m_Directives.begin(), m_Directives.end(), d) - m_Directives.begin();}

	//Return the index after sorting for the added element
	size_t AddDirective(const CMIDIMappingDirective& d) {m_Directives.push_back(d); Sort(); return std::find(m_Directives.begin(), m_Directives.end(), d) - m_Directives.begin();}

	void RemoveDirective(const size_t i) {m_Directives.erase(m_Directives.begin()+i);}

	const CMIDIMappingDirective& GetDirective(const size_t i) const {return m_Directives[i];}

	const_iterator Begin() const {return m_Directives.begin();}
	const_iterator End() const {return m_Directives.end();}
	size_t GetCount() const {return m_Directives.size();}

	size_t GetSerializationSize() const;
	void Serialize(FILE* f) const;
	bool Unserialize(const BYTE* ptr, const size_t size); //Return false if succesful, true otherwise.

	bool AreOrderEqual(const size_t a, const size_t b) {return !(m_Directives[a] < m_Directives[b] || m_Directives[b] < m_Directives[a]);}

private:
	void Sort() {std::stable_sort(m_Directives.begin(), m_Directives.end());}

private:
	CSoundFile& m_rSndFile;
	vector<CMIDIMappingDirective> m_Directives;
};


// Midi Continuous Controller Codes
// http://www.borg.com/~jglatt/tech/midispec/ctllist.htm
enum {
	MIDICC_start = 0,
	MIDICC_BankSelect_Coarse = MIDICC_start,
	MIDICC_ModulationWheel_Coarse = 1,
	MIDICC_Breathcontroller_Coarse = 2,
	MIDICC_FootPedal_Coarse = 4,
	MIDICC_PortamentoTime_Coarse = 5,
	MIDICC_DataEntry_Coarse = 6,
	MIDICC_Volume_Coarse = 7,
	MIDICC_Balance_Coarse = 8,
	MIDICC_Panposition_Coarse = 10,
	MIDICC_Expression_Coarse = 11,
	MIDICC_EffectControl1_Coarse = 12,
	MIDICC_EffectControl2_Coarse = 13,
	MIDICC_GeneralPurposeSlider1 = 16,
	MIDICC_GeneralPurposeSlider2 = 17,
	MIDICC_GeneralPurposeSlider3 = 18,
	MIDICC_GeneralPurposeSlider4 = 19,
	MIDICC_BankSelect_Fine = 32,
	MIDICC_ModulationWheel_Fine = 33,
	MIDICC_Breathcontroller_Fine = 34,
	MIDICC_FootPedal_Fine = 36,
	MIDICC_PortamentoTime_Fine = 37,
	MIDICC_DataEntry_Fine = 38,
	MIDICC_Volume_Fine = 39,
	MIDICC_Balance_Fine = 40,
	MIDICC_Panposition_Fine = 42,
	MIDICC_Expression_Fine = 43,
	MIDICC_EffectControl1_Fine = 44,
	MIDICC_EffectControl2_Fine = 45,
	MIDICC_HoldPedal_OnOff = 64,
	MIDICC_Portamento_OnOff = 65,
	MIDICC_SustenutoPedal_OnOff = 66,
	MIDICC_SoftPedal_OnOff = 67,
	MIDICC_LegatoPedal_OnOff = 68,
	MIDICC_Hold2Pedal_OnOff = 69,
	MIDICC_SoundVariation = 70,
	MIDICC_SoundTimbre = 71,
	MIDICC_SoundReleaseTime = 72,
	MIDICC_SoundAttackTime = 73,
	MIDICC_SoundBrightness = 74,
	MIDICC_SoundControl6 = 75,
	MIDICC_SoundControl7 = 76,
	MIDICC_SoundControl8 = 77,
	MIDICC_SoundControl9 = 78,
	MIDICC_SoundControl10 = 79,
	MIDICC_GeneralPurposeButton1_OnOff = 80,
	MIDICC_GeneralPurposeButton2_OnOff = 81,
	MIDICC_GeneralPurposeButton3_OnOff = 82,
	MIDICC_GeneralPurposeButton4_OnOff = 83,
	MIDICC_EffectsLevel = 91,
	MIDICC_TremuloLevel = 92,
	MIDICC_ChorusLevel = 93,
	MIDICC_CelesteLevel = 94,
	MIDICC_PhaserLevel = 95,
	MIDICC_DataButtonincrement = 96,
	MIDICC_DataButtondecrement = 97,
	MIDICC_NonRegisteredParameter_Fine = 98,
	MIDICC_NonRegisteredParameter_Coarse = 99,
	MIDICC_RegisteredParameter_Fine = 100,
	MIDICC_RegisteredParameter_Coarse = 101,
	MIDICC_AllSoundOff = 120,
	MIDICC_AllControllersOff = 121,
	MIDICC_LocalKeyboard_OnOff = 122,
	MIDICC_AllNotesOff = 123,
	MIDICC_OmniModeOff = 124,
	MIDICC_OmniModeOn = 125,
	MIDICC_MonoOperation = 126,
	MIDICC_PolyOperation = 127,
	MIDICC_end = MIDICC_PolyOperation,
};

enum {
	MIDI_PitchBend_Command = 0xE0,
	MIDI_PitchBend_Min     = 0x00,
	MIDI_PitchBend_Centre  = 0x2000,
	MIDI_PitchBend_Max     = 0x3FFF
};


const char* const MidiCCNames[MIDICC_end+1] ={
"BankSelect [Coarse]",			//0
"ModulationWheel [Coarse]",		//1
"Breathcontroller [Coarse]",	//2
"",								//3
"FootPedal [Coarse]",			//4
"PortamentoTime [Coarse]",		//5
"DataEntry [Coarse]",			//6
"Volume [Coarse]",				//7
"Balance [Coarse]",				//8
"",								//9
"Panposition [Coarse]",			//10
"Expression [Coarse]",			//11
"EffectControl1 [Coarse]",		//12
"EffectControl2 [Coarse]",		//13
"",								//14
"",								//15
"GeneralPurposeSlider1",		//16
"GeneralPurposeSlider2",		//17
"GeneralPurposeSlider3",		//18
"GeneralPurposeSlider4",		//19
"",								//20
"",								//21
"",								//22
"",								//23
"",								//24
"",								//25
"",								//26
"",								//27
"",								//28
"",								//29
"",								//30
"",								//31
"BankSelect [Fine]",			//32
"ModulationWheel [Fine]",		//33
"Breathcontroller [Fine]",		//34
"",								//35
"FootPedal [Fine]",				//36
"PortamentoTime [Fine]",		//37
"DataEntry [Fine]",				//38
"Volume [Fine]",				//39
"Balance [Fine]",				//40
"",								//41
"Panposition [Fine]",			//42
"Expression [Fine]",			//43
"EffectControl1 [Fine]",		//44
"EffectControl2 [Fine]",		//45
"",								//46
"",								//47
"",								//48
"",								//49
"",								//50
"",								//51
"",								//52
"",								//53
"",								//54
"",								//55
"",								//56
"",								//57
"",								//58
"",								//59
"",								//60
"",								//61
"",								//62
"",								//63
"HoldPedal [OnOff]",			//64
"Portamento [OnOff]",			//65
"SustenutoPedal [OnOff]",		//66
"SoftPedal [OnOff]",			//67
"LegatoPedal [OnOff]",			//68
"Hold2Pedal [OnOff]",			//69
"SoundVariation",				//70
"SoundTimbre",					//71
"SoundReleaseTime",				//72
"SoundAttackTime",				//73
"SoundBrightness",				//74
"SoundControl6",				//75
"SoundControl7",				//76
"SoundControl8",				//77
"SoundControl9",				//78
"SoundControl10",				//79
"GeneralPurposeButton1 [OnOff]",//80
"GeneralPurposeButton2 [OnOff]",//81
"GeneralPurposeButton3 [OnOff]",//82
"GeneralPurposeButton4 [OnOff]",//83
"",								//84
"",								//85
"",								//86
"",								//87
"",								//88
"",								//89
"",								//90
"EffectsLevel",					//91
"TremuloLevel",					//92
"ChorusLevel",					//93
"CelesteLevel",					//94
"PhaserLevel",					//95
"DataButtonIncrement",			//96
"DataButtonDecrement",			//97
"NonRegisteredParameter [Fine]",//98
"NonRegisteredParameter [Coarse]",//99
"RegisteredParameter [Fine]",	//100
"RegisteredParameter [Coarse]",	//101
"",								//102
"",								//103
"",								//104
"",								//105
"",								//106
"",								//107
"",								//108
"",								//109
"",								//110
"",								//111
"",								//112
"",								//113
"",								//114
"",								//115
"",								//116
"",								//117
"",								//118
"",								//119
"AllSoundOff",					//120
"AllControllersOff",			//121
"LocalKeyboard [OnOff]",		//122
"AllNotesOff",					//123
"OmniModeOff",					//124
"OmniModeOn",					//125
"MonoOperation",				//126
"PolyOperation",				//127
};



#endif
