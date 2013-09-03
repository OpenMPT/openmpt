/*
 * MIDIEvents.h
 * ------------
 * Purpose: MIDI event handling, event lists, ...
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


// MIDI related enums and helper functions
namespace MIDIEvents
{

	// MIDI Event Types
	enum EventType
	{
		evNoteOff			= 0x8,	// Note Off event
		evNoteOn			= 0x9,	// Note On event
		evPolyAftertouch	= 0xA,	// Poly Aftertouch / Poly Pressure event
		evControllerChange	= 0xB,	// Controller Change (see MidiCC enum)
		evProgramChange		= 0xC,	// Program Change
		evChannelAftertouch	= 0xD,	// Channel Aftertouch
		evPitchBend			= 0xE,	// Pitchbend event (see PitchBend enum)
		evSystem			= 0xF,	// System event (see SystemEvent enum)
	};

	// System Events (Fx ...)
	enum SystemEvent
	{
		sysExStart			= 0x0,	// Begin of System Exclusive message
		sysQuarterFrame		= 0x1,	// Quarter Frame Message
		sysPositionPointer	= 0x2,	// Song Position Pointer
		sysSongSelect		= 0x3,	// Song Select
		sysTuneRequest		= 0x6,	// Tune Request
		sysExEnd			= 0x7,	// End of System Exclusive message
		sysMIDIClock		= 0x8,	// MIDI Clock event
		sysMIDITick			= 0x9,	// MIDI Tick event
		sysStart			= 0xA,	// Start Song
		sysContinue			= 0xB,	// Continue Song
		sysStop				= 0xC,	// Stop Song
		sysActiveSense		= 0xE,	// Active Sense Message
		sysReset			= 0xF,	// Reset Device
	};

	// MIDI Pitchbend Constants
	enum PitchBend
	{
		pitchBendMin     = 0x00,
		pitchBendCentre  = 0x2000,
		pitchBendMax     = 0x3FFF
	};

	// MIDI Continuous Controller Codes
	// http://home.roadrunner.com/~jgglatt/tech/midispec/ctllist.htm
	enum MidiCC
	{
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
		MIDICC_TremoloLevel = 92,
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

	// MIDI CC Names
	static const char* const MidiCCNames[MIDICC_end + 1] =
	{
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
		"TremoloLevel",					//92
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

	// Build a generic MIDI event
	uint32 Event(EventType eventType, uint8 midiChannel, uint8 dataByte1, uint8 dataByte2);
	// Build a MIDI CC event
	uint32 CC(MidiCC midiCC, uint8 midiChannel, uint8 param);
	// Build a MIDI Pitchbend event
	uint32 PitchBend(uint8 midiChannel, uint16 bendAmount);
	// Build a MIDI Program Change event
	uint32 ProgramChange(uint8 midiChannel, uint8 program);
	// Build a MIDI Note Off event
	uint32 NoteOff(uint8 midiChannel, uint8 note, uint8 velocity);
	// Build a MIDI Note On event
	uint32 NoteOn(uint8 midiChannel, uint8 note, uint8 velocity);
	// Build a MIDI System Event
	uint8 System(SystemEvent eventType);

	// Get MIDI channel from a MIDI event
	uint8 GetChannelFromEvent(uint32 midiMsg);
	// Get MIDI Event type from a MIDI event
	EventType GetTypeFromEvent(uint32 midiMsg);
	// Get first data byte from a MIDI event
	uint8 GetDataByte1FromEvent(uint32 midiMsg);
	// Get second data byte from a MIDI event
	uint8 GetDataByte2FromEvent(uint32 midiMsg);

}
