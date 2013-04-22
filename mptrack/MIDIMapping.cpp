/*
 * MIDIMapping.cpp
 * ---------------
 * Purpose: MIDI Mapping management classes
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "MIDIMapping.h"
#include "../soundlib/MIDIEvents.h"
#include "Mainfrm.h"


std::string CMIDIMappingDirective::ToString() const
//-------------------------------------------------
{
	mpt::String str;
	char flags[4] = "000";
	if(m_Active) flags[0] = '1';
	if(m_CaptureMIDI) flags[1] = '1';
	if(m_AllowPatternEdit) flags[2] = '1';
	str.Format("%s:%d:%x:%d:%d:%d", flags, (int)GetChannel(), (int)GetEvent(), (int)GetController(), (int)m_PluginIndex, m_Parameter);
	return str;
}


size_t CMIDIMapper::GetSerializationSize() const
//----------------------------------------------
{
	size_t s = 0;
	for(const_iterator citer = Begin(); citer != End(); citer++)
	{
		if(citer->GetParamIndex() <= uint8_max) {s += 5; continue;}
		if(citer->GetParamIndex() <= uint16_max) {s += 6; continue;}
		s += 8;
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


bool CMIDIMapper::Deserialize(const char *ptr, const size_t size)
//---------------------------------------------------------------
{
	m_Directives.clear();
	const char* endptr = ptr + size;
	while(ptr + 5 <= endptr)
	{
		uint8 i8 = 0;
		uint16 i16 = 0;
		uint32 i32 = 0;
		memcpy(&i8, ptr, 1); ptr++;
		BYTE psize = 0;
		switch(i8 >> 6)
		{
		case 0: psize = 5; break;
		case 1: psize = 6; break;
		case 2: psize = 8; break;
		case 3: default: psize = 12; break;
		}

		if(ptr + psize - 1 > endptr) return true;
		if(((i8 >> 2) & 7) != 0) {ptr += psize - 1; continue;} //Skipping unrecognised mapping types.

		CMIDIMappingDirective s;
		s.SetActive((i8 & 1) != 0);
		s.SetCaptureMIDI((i8 & (1 << 1)) != 0);
		s.SetAllowPatternEdit((i8 & (1 << 5)) != 0);
		memcpy(&i16, ptr, 2); ptr += 2; //Channel, event, MIDIbyte1.
		memcpy(&i8, ptr, 1); ptr++;		//Plugindex
		const BYTE remainingbytes = psize - 4;
		memcpy(&i32, ptr, MIN(4, remainingbytes)); ptr += remainingbytes;

		s.SetChannel(((i16 & 1) != 0) ? 0 : 1 + ((i16 >> 1) & 0xF));
		s.SetEvent(static_cast<BYTE>((i16 >> 5) & 0xF));
		s.SetController(i16 >> 9);
		s.SetPlugIndex(i8);
		s.SetParamIndex(i32);
		AddDirective(s);
	}

	return false;
}


bool CMIDIMapper::OnMIDImsg(const DWORD midimsg, BYTE& mappedIndex, uint32& paramindex, BYTE& paramval)
//-----------------------------------------------------------------------------------------------------
{
	bool captured = false;

	if(MIDIEvents::GetTypeFromEvent(midimsg) != MIDIEvents::evControllerChange) return captured;
	//For now only controllers can be mapped so if event is not controller change,
	//no mapping will be found and thus no search is done.
	//NOTE: The event value is not checked in code below.

	const BYTE controller = MIDIEvents::GetDataByte1FromEvent(midimsg);

	const_iterator citer = std::lower_bound(Begin(), End(), controller); 

	const BYTE channel = MIDIEvents::GetChannelFromEvent(midimsg);

	for(; citer != End() && citer->GetController() == controller && !captured; citer++)
	{
		if(!citer->IsActive()) continue;
		BYTE plugindex = 0;
		uint32 param = 0;
		if( citer->GetAnyChannel() || channel+1 == citer->GetChannel())
		{
			plugindex = citer->GetPlugIndex();
			param = citer->GetParamIndex();
			if(citer->GetAllowPatternEdit())
			{
				mappedIndex = plugindex;
				paramindex = param;
				paramval = MIDIEvents::GetDataByte2FromEvent(midimsg);
			}
			if(citer->GetCaptureMIDI()) captured = true;

			if(plugindex > 0 && plugindex <= MAX_MIXPLUGINS)
			{
				IMixPlugin* pPlug = m_rSndFile.m_MixPlugins[plugindex-1].pMixPlugin;
				if(!pPlug) continue;
				pPlug->SetZxxParameter(param, (midimsg >> 16) & 0x7F);
				CMainFrame::GetMainFrame()->ThreadSafeSetModified(m_rSndFile.GetpModDoc());
			}
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
