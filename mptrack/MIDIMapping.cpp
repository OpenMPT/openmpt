/*
 * MIDIMapping.cpp
 * ---------------
 * Purpose: MIDI Mapping management classes
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Moddoc.h"
#include "MIDIMapping.h"
#include "../common/FileReader.h"
#include "../soundlib/MIDIEvents.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"


OPENMPT_NAMESPACE_BEGIN


size_t CMIDIMapper::Serialize(std::ostream *file) const
{
	//Bytes: 1 Flags, 2 key, 1 plugindex, 1,2,4,8 plug/etc.
	size_t size = 0;
	for(const auto &d : m_Directives)
	{
		uint16 temp16 = (d.GetChnEvent() << 1) + (d.GetController() << 9);
		if(d.GetAnyChannel()) temp16 |= 1;
		uint32 temp32 = d.GetParamIndex();

		uint8 temp8 = d.IsActive(); //bit 0
		if(d.GetCaptureMIDI()) temp8 |= (1 << 1); //bit 1
		//bits 2-4: Mapping type: 0 for plug param control.
		//bit 5: 
		if(d.GetAllowPatternEdit()) temp8 |= (1 << 5);
		//bits 6-7: Size: 5, 6, 8, 12

		uint8 parambytes = 4;
		if(temp32 <= uint16_max)
		{
			if(temp32 <= uint8_max) parambytes = 1;
			else {parambytes = 2; temp8 |= (1 << 6);}
		}
		else temp8 |= (2 << 6);

		if(file)
		{
			std::ostream & f = *file;
			mpt::IO::WriteIntLE<uint8>(f, temp8);
			mpt::IO::WriteIntLE<uint16>(f, temp16);
			mpt::IO::WriteIntLE<uint8>(f, d.GetPlugIndex());
			mpt::IO::WritePartial<uint32le>(f, mpt::as_le(temp32), parambytes);
		}
		size += sizeof(temp8) + sizeof(temp16) + sizeof(temp8) + parambytes;
	}
	return size;
}


bool CMIDIMapper::Deserialize(FileReader &file)
{
	m_Directives.clear();
	while(file.CanRead(1))
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

		if(!file.CanRead(psize)) return false;
		if(((i8 >> 2) & 7) != 0) { file.Skip(psize); continue;} //Skipping unrecognised mapping types.

		CMIDIMappingDirective s;
		s.SetActive((i8 & 1) != 0);
		s.SetCaptureMIDI((i8 & (1 << 1)) != 0);
		s.SetAllowPatternEdit((i8 & (1 << 5)) != 0);
		uint16 i16 = file.ReadUint16LE();	//Channel, event, MIDIbyte1.
		i8 = file.ReadUint8();		//Plugindex
		uint32le i32;
		file.ReadStructPartial(i32, psize - 3);

		s.SetChannel(((i16 & 1) != 0) ? 0 : 1 + ((i16 >> 1) & 0xF));
		s.SetEvent(static_cast<uint8>((i16 >> 5) & 0xF));
		s.SetController(i16 >> 9);
		s.SetPlugIndex(i8);
		s.SetParamIndex(i32);
		AddDirective(s);
	}

	return true;
}


bool CMIDIMapper::OnMIDImsg(const DWORD midimsg, PLUGINDEX &mappedIndex, PlugParamIndex &paramindex, uint16 &paramval)
{
	const MIDIEvents::EventType eventType = MIDIEvents::GetTypeFromEvent(midimsg);
	const uint8 controller = MIDIEvents::GetDataByte1FromEvent(midimsg);
	const uint8 channel = MIDIEvents::GetChannelFromEvent(midimsg) & 0x7F;
	const uint8 controllerVal = MIDIEvents::GetDataByte2FromEvent(midimsg) & 0x7F;

	for(const auto &d : m_Directives)
	{
		if(!d.IsActive()) continue;
		if(d.GetEvent() != eventType) continue;
		if(eventType == MIDIEvents::evControllerChange
			&& d.GetController() != controller
			&& (d.GetController() >= 32 || d.GetController() + 32 != controller))
			continue;
		if(!d.GetAnyChannel() && channel + 1 != d.GetChannel()) continue;

		const PLUGINDEX plugindex = d.GetPlugIndex();
		const uint32 param = d.GetParamIndex();
		uint16 val = (d.GetEvent() == MIDIEvents::evChannelAftertouch ? controller : controllerVal) << 7;

		if(eventType == MIDIEvents::evControllerChange)
		{
			// Fine (0...31) / Coarse (32...63) controller pairs - Fine should be sent first.
			if(controller == m_lastCC + 32 && m_lastCC < 32)
			{
				val = (val >> 7) | m_lastCCvalue;
			}
			m_lastCC = controller;
			m_lastCCvalue = val;
		}

		if(d.GetAllowPatternEdit())
		{
			mappedIndex = plugindex;
			paramindex = param;
			paramval = val;
		}

		if(plugindex > 0 && plugindex <= MAX_MIXPLUGINS)
		{
#ifndef NO_PLUGINS
			IMixPlugin *pPlug = m_rSndFile.m_MixPlugins[plugindex - 1].pMixPlugin;
			if(!pPlug) continue;
			pPlug->SetParameter(param, val / 16383.0f);
			if(m_rSndFile.GetpModDoc() != nullptr)
				m_rSndFile.GetpModDoc()->SetModified();
#endif // NO_PLUGINS
		}
		if(d.GetCaptureMIDI())
		{
			return true;
		}
	}

	return false;
}


void CMIDIMapper::Swap(const size_t a, const size_t b)
{
	if(a < m_Directives.size() && b < m_Directives.size())
	{
		std::swap(m_Directives[a], m_Directives[b]);
		Sort();
	}
}


OPENMPT_NAMESPACE_END
