#include "stdafx.h"
#include <mmsystem.h>
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "dlsbank.h"
#include "midi.h"

//#define MPTMIDI_RECORDLOG

// Midi Input globals
HMIDIIN CMainFrame::shMidiIn = NULL;

//Get Midi message(dwParam1), apply MIDI settings having effect on volume, and return
//the volume value [0, 256]. In addition value -1 is used as 'use default value'-indicator.
int ApplyVolumeRelatedMidiSettings(const DWORD& dwParam1, const BYTE midivolume)
//----------------------------------------------------------------
{
	int nVol = GetFromMIDIMsg_DataByte2(dwParam1);
	if (CMainFrame::m_dwMidiSetup & MIDISETUP_RECORDVELOCITY)
	{
		nVol = (CDLSBank::DLSMidiVolumeToLinear(nVol)+255) >> 8;
		if (CMainFrame::m_dwMidiSetup & MIDISETUP_AMPLIFYVELOCITY) nVol *= 2;
		if (nVol < 1) nVol = 1;
		if (nVol > 256) nVol = 256;
		if(CMainFrame::m_dwMidiSetup & MIDISETUP_MIDIVOL_TO_NOTEVOL)
			nVol = static_cast<int>((midivolume / 127.0) * nVol);
	}
	else //Case: No velocity record.
	{	
		if(CMainFrame::m_dwMidiSetup & MIDISETUP_MIDIVOL_TO_NOTEVOL)
			nVol = 4*((midivolume+1)/2);
		else //Use default volume
			nVol = -1;
	}

	return nVol;
}

void ApplyTransposeKeyboardSetting(CMainFrame& rMainFrm, DWORD& dwParam1)
//------------------------------------------------------------------------
{
	if ( (CMainFrame::m_dwMidiSetup & MIDISETUP_TRANSPOSEKEYBOARD)
		&& (GetFromMIDIMsg_Channel(dwParam1) != 9) )
	{
		int nTranspose = rMainFrm.GetBaseOctave() - 4;
		if (nTranspose)
		{
			int note = GetFromMIDIMsg_DataByte1(dwParam1);
			if (note < 0x80)
			{
				note += nTranspose*12;
				if (note < 0) note = 0;
				if (note > NOTE_MAX - 1) note = NOTE_MAX - 1;

				// -> CODE#0011
				// -> DESC="bug fix about transpose midi keyboard option"
				//dwParam1 &= 0xffffff00;
				dwParam1 &= 0xffff00ff;
				// -! BUG_FIX#0011

				dwParam1 |= (note<<8);
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// MMSYSTEM Midi Record

DWORD gdwLastMidiEvtTime = 0;

void CALLBACK MidiInCallBack(HMIDIIN, UINT wMsg, DWORD, DWORD dwParam1, DWORD dwParam2)
//-------------------------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	HWND hWndMidi;

	if (!pMainFrm) return;
#ifdef MPTMIDI_RECORDLOG
	DWORD dwMidiStatus = dwParam1 & 0xFF;
	DWORD dwMidiByte1 = (dwParam1 >> 8) & 0xFF;
	DWORD dwMidiByte2 = (dwParam1 >> 16) & 0xFF;
	DWORD dwTimeStamp = dwParam2;
	Log("time=%8dms status=%02X data=%02X.%02X\n", dwTimeStamp, dwMidiStatus, dwMidiByte1, dwMidiByte2);
#endif

	hWndMidi = pMainFrm->GetMidiRecordWnd();
	if ((hWndMidi) && ((wMsg == MIM_DATA) || (wMsg == MIM_MOREDATA)))
	{
		switch (dwParam1 & 0xF0)
		{
			case 0xF0:	// Midi Clock
				if (wMsg == MIM_DATA)
				{
					DWORD dwTime = timeGetTime();
					const DWORD timediff = dwTime - gdwLastMidiEvtTime;
					if (timediff < 20*3) break;
					
					gdwLastMidiEvtTime = dwTime; // continue
				}
				else break;

			case 0x80:	// Note Off
			case 0x90:	// Note On
				ApplyTransposeKeyboardSetting(*pMainFrm, dwParam1);

			default:
				::PostMessage(hWndMidi, WM_MOD_MIDIMSG, dwParam1, dwParam2);
			break;
		}
	}
}


BOOL CMainFrame::midiOpenDevice()
//-------------------------------
{
	if (shMidiIn) return TRUE;
	try {
		if (midiInOpen(&shMidiIn, m_nMidiDevice, (DWORD)MidiInCallBack, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
		{
			shMidiIn = NULL;
			return FALSE;
		}
		midiInStart(shMidiIn);
	} catch (...) {}
	return TRUE;
}


void CMainFrame::midiCloseDevice()
//--------------------------------
{
	if (shMidiIn)
	{
		midiInClose(shMidiIn);
		shMidiIn = NULL;
	}
}


void CMainFrame::OnMidiRecord()
//-----------------------------
{
	if (shMidiIn)
	{
		midiCloseDevice();
	} else
	{
		midiOpenDevice();
	}
}


void CMainFrame::OnUpdateMidiRecord(CCmdUI *pCmdUI)
//-------------------------------------------------
{
	if (pCmdUI) pCmdUI->SetCheck((shMidiIn) ? TRUE : FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


size_t CMIDIMapper::GetSerializationSize() const
//---------------------------------------------
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
        //bit 5: if(citer->GetAllowPatternEdit()) temp8 |= (1 << 4);
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


bool CMIDIMapper::Unserialize(const BYTE* ptr, const size_t size)
//----------------------------------------------------------------------------
{
	m_Directives.clear();
	const BYTE* endptr = ptr + size;
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
		s.SetAllowPatternEdit((i8 & (1 << 4)) != 0);
		memcpy(&i16, ptr, 2); ptr += 2; //Channel, event, MIDIbyte1.
		memcpy(&i8, ptr, 1); ptr++;		//Plugindex
		const BYTE remainingbytes = psize - 4;
		memcpy(&i32, ptr, min(4, remainingbytes)); ptr += remainingbytes;
		   
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
//----------------------------------------------------------------------------------------------------
{
	bool captured = false;

	if(GetFromMIDIMsg_Event(midimsg) != MIDIEVENT_CONTROLLERCHANGE) return captured;
	//For now only controllers can be mapped so if event is not controller change,
	//no mapping will be found and thus no search is done.
	//NOTE: The event value is not checked in code below.

	const BYTE controller = GetFromMIDIMsg_DataByte1(midimsg);

	const_iterator citer = std::lower_bound(Begin(), End(), controller); 

	const BYTE channel = GetFromMIDIMsg_Channel(midimsg);

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
				paramval = GetFromMIDIMsg_DataByte2(midimsg);
			}
			if(citer->GetCaptureMIDI()) captured = true;

			if(plugindex > 0 && plugindex <= MAX_MIXPLUGINS)
			{
				IMixPlugin* pPlug = m_rSndFile.m_MixPlugins[plugindex-1].pMixPlugin;
				if(!pPlug) continue;
				pPlug->SetZxxParameter(param, (midimsg >> 16) & 0x7F);
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


