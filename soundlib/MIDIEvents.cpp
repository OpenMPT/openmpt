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

namespace MIDIEvents
{

// Build a generic MIDI event
uint32 BuildEvent(EventType eventType, uint8 midiChannel, uint8 dataByte1, uint8 dataByte2)
//-----------------------------------------------------------------------------------------
{
	return (eventType << 4) | (midiChannel & 0x0F) | (dataByte1 << 8) | (dataByte2 << 16);
}


// Build a MIDI CC event
uint32 BuildCCEvent(MidiCC midiCC, uint8 midiChannel, uint8 param)
//----------------------------------------------------------------
{
	return BuildEvent(evControllerChange, midiChannel, static_cast<uint8>(midiCC), param);
}


// Build a MIDI Pitchbend event
uint32 BuildPitchBendEvent(uint8 midiChannel, uint16 bendAmount)
//--------------------------------------------------------------
{
	return BuildEvent(evPitchBend, midiChannel, static_cast<uint8>(bendAmount & 0x7F), static_cast<uint8>(bendAmount >> 7));
}


// Build a MIDI Program Change event
uint32 BuildProgramChangeEvent(uint8 midiChannel, uint8 program)
//--------------------------------------------------------------
{
	return BuildEvent(evProgramChange, midiChannel, program, 0);
}


// Build a MIDI Note Off event
uint32 BuildNoteOffEvent(uint8 midiChannel, uint8 note, uint8 velocity)
//---------------------------------------------------------------------
{
	return BuildEvent(evNoteOff, midiChannel, note, velocity);
}


// Build a MIDI Note On event
uint32 BuildNoteOnEvent(uint8 midiChannel, uint8 note, uint8 velocity)
//--------------------------------------------------------------------
{
	return BuildEvent(evNoteOn, midiChannel, note, velocity);
}


// Build a MIDI System Event
uint8 BuildSystemEvent(SystemEvent eventType)
//-------------------------------------------
{
	return static_cast<uint8>((evSystem << 4) | eventType);
}


// Get MIDI channel from a MIDI event
uint8 GetChannelFromEvent(uint32 midiMsg)
//---------------------------------------
{
	return static_cast<uint8>((midiMsg & 0xF));
}


// Get MIDI Event type from a MIDI event
EventType GetTypeFromEvent(uint32 midiMsg)
//----------------------------------------
{
	return static_cast<EventType>(((midiMsg >> 4) & 0xF));
}


// Get first data byte from a MIDI event
uint8 GetDataByte1FromEvent(uint32 midiMsg)
//-----------------------------------------
{
	return static_cast<uint8>(((midiMsg >> 8) & 0xFF));
}


// Get second data byte from a MIDI event
uint8 GetDataByte2FromEvent(uint32 midiMsg)
//-----------------------------------------
{
	return static_cast<uint8>(((midiMsg >> 16) & 0xFF));
}

}	// End namespace
