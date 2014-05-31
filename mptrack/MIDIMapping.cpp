/*
 * MIDIMapping.cpp
 * ---------------
 * Purpose: MIDI Mapping management classes
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "../soundlib/MIDIEvents.h"
#include "Mainfrm.h"
#include "../soundlib/FileReader.h"
#include "MIDIMapping.h"


OPENMPT_NAMESPACE_BEGIN


std::string CMIDIMappingDirective::ToString() const
//-------------------------------------------------
{
	char flags[4] = "000";
	if(m_Active) flags[0] = '1';
	if(m_CaptureMIDI) flags[1] = '1';
	if(m_AllowPatternEdit) flags[2] = '1';
	return mpt::String::Format("%s:%d:%x:%d:%d:%d", flags, (int)GetChannel(), (int)GetEvent(), (int)GetController(), (int)m_PluginIndex, m_Parameter);
}


size_t CMIDIMapper::GetSerializationSize() const
//----------------------------------------------
{
	size_t s = 0;
	for(const_iterator citer = Begin(); citer != End(); citer++)
	{
		if(citer->GetParamIndex() <= uint8_max) s += 5;
		else if(citer->GetParamIndex() <= uint16_max) s += 6;
		else s += 8;
	}
	return s;
}


void CMIDIMapper::Serialize(FILE* f) const
//----------------------------------------
{
	//Bytes: 1 Flags, 2 key, 1 plugindex, 1,2,4,8 plug/etc.
	for(const_iterator citer = Begin(); citer != End(); citer++)
	{
		uint16 temp16 = (citer->GetChnEvent() << 1) + (citer->GetController() << 9);
		if(citer->GetAnyChannel()) temp16 |= 1;
		uint32 temp32 = citer->GetParamIndex();

		uint8 temp8 = citer->IsActive(); //bit 0
		if(citer->GetCaptureMIDI()) temp8 |= (1 << 1); //bit 1
		//bits 2-4: Mapping type: 0 for plug param control.
		//bit 5: 
		if(citer->GetAllowPatternEdit()) temp8 |= (1 << 5);
		//bits 6-7: Size: 5, 6, 8, 12

		BYTE parambytes = 4;
		if(temp32 <= uint16_max)
		{
			if(temp32 <= uint8_max) parambytes = 1;
			else {parambytes = 2; temp8 |= (1 << 6);}
		}
		else temp8 |= (2 << 6);

		fwrite(&temp8, 1, sizeof(temp8), f);
		fwrite(&temp16, 1, sizeof(temp16), f);
		temp8 = citer->GetPlugIndex();
		fwrite(&temp8, 1, sizeof(temp8), f);
		fwrite(&temp32, 1, parambytes, f);
	}
}


bool CMIDIMapper::Deserialize(FileReader &file)
//---------------------------------------------
{
	m_Directives.clear();
	while(file.AreBytesLeft())
	{
		uint8 i8 = file.ReadUint8();
		uint8 psize = 0;
		// Determine size of this event (depends on size of plugin parameter index)
		switch(i8 >> 6)
		{
		case 0: psize = 4; break;
		case 1: psize = 5; break;
		case 2: psize = 7; break;
		case 3: default: psize = 11; break;
		}

		if(!file.CanRead(psize)) return true;
		if(((i8 >> 2) & 7) != 0) { file.Skip(psize); continue;} //Skipping unrecognised mapping types.

		CMIDIMappingDirective s;
		s.SetActive((i8 & 1) != 0);
		s.SetCaptureMIDI((i8 & (1 << 1)) != 0);
		s.SetAllowPatternEdit((i8 & (1 << 5)) != 0);
		uint16 i16 = file.ReadUint16LE();	//Channel, event, MIDIbyte1.
		i8 = file.ReadUint8();		//Plugindex
		uint32 i32;
		file.ReadStructPartial(i32, psize - 3);
		SwapBytesLE(i32);

		s.SetChannel(((i16 & 1) != 0) ? 0 : 1 + ((i16 >> 1) & 0xF));
		s.SetEvent(static_cast<BYTE>((i16 >> 5) & 0xF));
		s.SetController(i16 >> 9);
		s.SetPlugIndex(i8);
		s.SetParamIndex(i32);
		AddDirective(s);
	}

	return false;
}


bool CMIDIMapper::OnMIDImsg(const DWORD midimsg, PLUGINDEX &mappedIndex, PlugParamIndex &paramindex, uint8 &paramval)
//-------------------------------------------------------------------------------------------------------------------
{
	bool captured = false;

	const MIDIEvents::EventType eventType = MIDIEvents::GetTypeFromEvent(midimsg);
	const uint8 controller = MIDIEvents::GetDataByte1FromEvent(midimsg);
	const uint8 channel = MIDIEvents::GetChannelFromEvent(midimsg);

	for(const_iterator citer = Begin(); citer != End() && !captured; citer++)
	{
		if(!citer->IsActive()) continue;
		if(citer->GetEvent() != eventType) continue;
		if(eventType == MIDIEvents::evControllerChange && citer->GetController() != controller) continue;
		if(!citer->GetAnyChannel() && channel + 1 != citer->GetChannel()) continue;

		const PLUGINDEX plugindex = citer->GetPlugIndex();
		const uint32 param = citer->GetParamIndex();
		const uint8 val = (citer->GetEvent() == MIDIEvents::evChannelAftertouch ? controller : MIDIEvents::GetDataByte2FromEvent(midimsg)) & 0x7F;

		if(citer->GetAllowPatternEdit())
		{
			mappedIndex = plugindex;
			paramindex = param;
			paramval = val;
		}
		if(citer->GetCaptureMIDI()) captured = true;

		if(plugindex > 0 && plugindex <= MAX_MIXPLUGINS)
		{
			IMixPlugin *pPlug = m_rSndFile.m_MixPlugins[plugindex - 1].pMixPlugin;
			if(!pPlug) continue;
			pPlug->SetZxxParameter(param, val);
			CMainFrame::GetMainFrame()->ThreadSafeSetModified(m_rSndFile.GetpModDoc());
		}
	}

	return captured;
}


bool CMIDIMapper::Swap(const size_t a, const size_t b)
//----------------------------------------------------
{
	if(a < m_Directives.size() && b < m_Directives.size())
	{
		CMIDIMappingDirective temp = m_Directives[a];
		m_Directives[a] = m_Directives[b];
		m_Directives[b] = temp;
		Sort();
		return false;
	}
	else return true;
}


OPENMPT_NAMESPACE_END
