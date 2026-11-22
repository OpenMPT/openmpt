/*
 * MIDIEvents.cpp
 * --------------
 * Purpose: MIDI event handling, event lists, ...
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "MIDIEvents.h"

OPENMPT_NAMESPACE_BEGIN

namespace MIDIEvents
{

// Build a generic MIDI event
std::array<std::byte, 3> Event(EventType eventType, uint8 midiChannel, uint8 dataByte1, uint8 dataByte2)
{
	MPT_ASSERT(!(dataByte1 & 0x80));
	MPT_ASSERT(!(dataByte2 & 0x80));
	return {
		mpt::byte_cast<std::byte>(static_cast<uint8>((eventType & 0xF0) | (midiChannel & 0x0F))),
		mpt::byte_cast<std::byte>(dataByte1),
		mpt::byte_cast<std::byte>(dataByte2)
	};
}


std::array<std::byte, 2> Event(EventType eventType, uint8 midiChannel, uint8 dataByte)
{
	MPT_ASSERT(!(dataByte & 0x80));
	return {
		mpt::byte_cast<std::byte>(static_cast<uint8>((eventType & 0xF0) | (midiChannel & 0x0F))),
		mpt::byte_cast<std::byte>(dataByte)
	};
}


// Build a MIDI CC event
std::array<std::byte, 3> CC(MidiCC midiCC, uint8 midiChannel, uint8 param)
{
	return Event(evControllerChange, midiChannel, static_cast<uint8>(midiCC), param);
}


// Build a MIDI Pitchbend event
std::array<std::byte, 3> PitchBend(uint8 midiChannel, uint16 bendAmount)
{
	MPT_ASSERT(!(bendAmount & 0xC000));
	return Event(evPitchBend, midiChannel, static_cast<uint8>(bendAmount & 0x7F), static_cast<uint8>((bendAmount >> 7) & 0x7F));
}


// Build a MIDI Program Change event
std::array<std::byte, 2> ProgramChange(uint8 midiChannel, uint8 program)
{
	return Event(evProgramChange, midiChannel, program);
}


// Build a MIDI Note Off event
std::array<std::byte, 3> NoteOff(uint8 midiChannel, uint8 note, uint8 velocity)
{
	return Event(evNoteOff, midiChannel, note, velocity);
}


// Build a MIDI Note On event
std::array<std::byte, 3> NoteOn(uint8 midiChannel, uint8 note, uint8 velocity)
{
	return Event(evNoteOn, midiChannel, note, velocity);
}


// Build a MIDI System Event
std::array<std::byte, 1> System(SystemEvent eventType)
{
	return {mpt::byte_cast<std::byte>(static_cast<uint8>(eventType))};
}


// Build a MIDI Song Position Event
std::array<std::byte, 3> SongPosition(uint16 quarterNotes)
{
	MPT_ASSERT(!(quarterNotes & 0xC000));
	return Event(evSystem, sysPositionPointer, static_cast<uint8>(quarterNotes & 0x7F), static_cast<uint8>((quarterNotes >> 7) & 0x7F));
}


// Get MIDI channel from a MIDI event
uint8 GetChannelFromEvent(mpt::const_byte_span midiMsg)
{
	MPT_ASSERT(midiMsg.size() >= 1);
	return static_cast<uint8>((mpt::byte_cast<uint8>(midiMsg[0]) & 0x0F));
}


// Get MIDI Event type from a MIDI event
EventType GetTypeFromEvent(mpt::const_byte_span midiMsg)
{
	MPT_ASSERT(midiMsg.size() >= 1);
	return static_cast<EventType>((mpt::byte_cast<uint8>(midiMsg[0]) & 0xF0));
}


// Get first data byte from a MIDI event
uint8 GetDataByte1FromEvent(mpt::const_byte_span midiMsg)
{
	return midiMsg.size() >= 2 ? mpt::byte_cast<uint8>(midiMsg[1]) : uint8{0};
}

// Get second data byte from a MIDI event
uint8 GetDataByte2FromEvent(mpt::const_byte_span midiMsg)
{
	return midiMsg.size() >= 3 ? mpt::byte_cast<uint8>(midiMsg[2]) : uint8{0};
}


// True if message is a Song Position/Clock/Start/Continue/Stop system message
bool IsTransportEvent(mpt::const_byte_span midiMsg)
{
	if(GetTypeFromEvent(midiMsg) != evSystem)
		return false;
	switch(static_cast<MIDIEvents::SystemEvent>(midiMsg[0]))
	{
	case MIDIEvents::sysPositionPointer:
	case MIDIEvents::sysMIDIClock:
	case MIDIEvents::sysStart:
	case MIDIEvents::sysContinue:
	case MIDIEvents::sysStop:
		return true;
	default:
		return false;
	}
}


// True only for note-on events with velocity > 0
bool IsNoteOnEvent(mpt::const_byte_span midiMsg)
{
	return midiMsg.size() >= 3 && GetTypeFromEvent(midiMsg) == evNoteOn && GetDataByte2FromEvent(midiMsg) > 0;
}


// True for note-off events and note-on events with velocity == 0
bool IsNoteOffEvent(mpt::const_byte_span midiMsg)
{
	return midiMsg.size() >= 3 && (GetTypeFromEvent(midiMsg) == evNoteOff || (GetTypeFromEvent(midiMsg) == evNoteOn && GetDataByte2FromEvent(midiMsg) == 0));
}


// Get the length of a MIDI event in bytes
uint8 GetEventLength(std::byte firstByte)
{
	uint8 msgSize = 3;
	switch(static_cast<EventType>((mpt::byte_cast<uint8>(firstByte) & 0xF0)))
	{
	case evProgramChange:
	case evChannelAftertouch:
		msgSize = 2;
		break;
	case evSystem:
		switch(static_cast<SystemEvent>(mpt::byte_cast<uint8>(firstByte)))
		{
		case sysQuarterFrame:
		case sysSongSelect:
			msgSize = 2;
			break;
		case sysPositionPointer:
			msgSize = 3;
			break;
		default:
			msgSize = 1;
			break;
		}
		break;
	default:
		break;
	}
	return msgSize;
}


// MIDI CC Names
const char* const MidiCCNames[MIDICC_end + 1] =
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


}	// End namespace


OPENMPT_NAMESPACE_END
