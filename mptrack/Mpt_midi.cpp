/*
 * MPT_MIDI.cpp
 * ------------
 * Purpose: MIDI Input handling code.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include <mmsystem.h>
#include "Mainfrm.h"
#include "InputHandler.h"
#include "Dlsbank.h"
#include "../soundlib/MIDIEvents.h"


OPENMPT_NAMESPACE_BEGIN


//#define MPTMIDI_RECORDLOG

// Midi Input globals
HMIDIIN CMainFrame::shMidiIn = NULL;

//Get Midi message(dwParam1), apply MIDI settings having effect on volume, and return
//the volume value [0, 256]. In addition value -1 is used as 'use default value'-indicator.
int CMainFrame::ApplyVolumeRelatedSettings(const DWORD &dwParam1, const BYTE midivolume)
//--------------------------------------------------------------------------------------
{
	int nVol = MIDIEvents::GetDataByte2FromEvent(dwParam1);
	if(TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_RECORDVELOCITY)
	{
		nVol = (CDLSBank::DLSMidiVolumeToLinear(nVol)+255) >> 8;
		nVol *= TrackerSettings::Instance().midiVelocityAmp / 100;
		Limit(nVol, 1, 256);
		if(TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_MIDIVOL_TO_NOTEVOL)
			nVol = static_cast<int>((midivolume / 127.0) * nVol);
	} else
	{
		// Case: No velocity record.
		if(TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_MIDIVOL_TO_NOTEVOL)
			nVol = 4*((midivolume+1)/2);
		else //Use default volume
			nVol = -1;
	}

	return nVol;
}


void ApplyTransposeKeyboardSetting(CMainFrame &rMainFrm, uint32 &dwParam1)
//------------------------------------------------------------------------
{
	if ( (TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_TRANSPOSEKEYBOARD)
		&& (MIDIEvents::GetChannelFromEvent(dwParam1) != 9) )
	{
		int nTranspose = rMainFrm.GetBaseOctave() - 4;
		if (nTranspose)
		{
			int note = MIDIEvents::GetDataByte1FromEvent(dwParam1);
			if (note < 0x80)
			{
				note += nTranspose * 12;
				Limit(note, 0, NOTE_MAX - NOTE_MIN);

				dwParam1 &= 0xffff00ff;

				dwParam1 |= (note << 8);
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// MMSYSTEM Midi Record

void CALLBACK MidiInCallBack(HMIDIIN, UINT wMsg, DWORD_PTR, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
//-------------------------------------------------------------------------------------------------
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
	if(wMsg == MIM_DATA || wMsg == MIM_MOREDATA)
	{
		uint32 data = static_cast<uint32>(dwParam1);
		if(::IsWindow(hWndMidi))
		{
			switch(MIDIEvents::GetTypeFromEvent(data))
			{
			case MIDIEvents::evNoteOff:	// Note Off
			case MIDIEvents::evNoteOn:	// Note On
				ApplyTransposeKeyboardSetting(*pMainFrm, data);
				MPT_FALLTHROUGH;
			default:
				if(::SendMessage(hWndMidi, WM_MOD_MIDIMSG, data, dwParam2))
					return;	// Message has been handled
				break;
			}
		}
		// Pass MIDI to keyboard handler
		CMainFrame::GetInputHandler()->HandleMIDIMessage(kCtxAllContexts, data);
	} else if(wMsg == MIM_LONGDATA)
	{
		// Sysex...
	}
}


bool CMainFrame::midiOpenDevice(bool showSettings)
//------------------------------------------------
{
	if (shMidiIn) return true;
	
	if (midiInOpen(&shMidiIn, TrackerSettings::Instance().GetCurrentMIDIDevice(), (DWORD_PTR)MidiInCallBack, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
	{
		shMidiIn = NULL;

		// Show MIDI configuration on fail.
		if(showSettings)
		{
			CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_MIDI;
			CMainFrame::GetMainFrame()->OnViewOptions();
		}

		// Let's see if the user updated the settings.
		if(midiInOpen(&shMidiIn, TrackerSettings::Instance().GetCurrentMIDIDevice(), (DWORD_PTR)MidiInCallBack, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
		{
			shMidiIn = NULL;
			return false;
		}
	}
	midiInStart(shMidiIn);
	return true;
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


OPENMPT_NAMESPACE_END
