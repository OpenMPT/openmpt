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


#ifdef MPT_ALL_LOGGING
#define MPTMIDI_RECORDLOG
#endif


// Midi Input globals
HMIDIIN CMainFrame::shMidiIn = nullptr;

//Get Midi message(dwParam1), apply MIDI settings having effect on volume, and return
//the volume value [0, 256]. In addition value -1 is used as 'use default value'-indicator.
int CMainFrame::ApplyVolumeRelatedSettings(const DWORD &dwParam1, const BYTE midivolume)
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
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	HWND hWndMidi;

	if (!pMainFrm) return;
#ifdef MPTMIDI_RECORDLOG
	DWORD dwMidiStatus = dwParam1 & 0xFF;
	DWORD dwMidiByte1 = (dwParam1 >> 8) & 0xFF;
	DWORD dwMidiByte2 = (dwParam1 >> 16) & 0xFF;
	DWORD dwTimeStamp = (DWORD)dwParam2;
	MPT_LOG_GLOBAL(LogDebug, "MIDI", MPT_UFORMAT("time={}ms status={} data={}.{}")(mpt::ufmt::dec(dwTimeStamp), mpt::ufmt::HEX0<2>(dwMidiStatus), mpt::ufmt::HEX0<2>(dwMidiByte1), mpt::ufmt::HEX0<2>(dwMidiByte2)));
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
				[[fallthrough]];
			default:
				if(::PostMessage(hWndMidi, WM_MOD_MIDIMSG, data, dwParam2))
					return;	// Message has been handled
				break;
			}
		}
		// Pass MIDI to keyboard handler
		CMainFrame::GetInputHandler()->HandleMIDIMessage(kCtxAllContexts, data);
	} else if(wMsg == MIM_LONGDATA)
	{
		// SysEx...
	} else if (wMsg == MIM_CLOSE)
	{
		// midiInClose will trigger this, but also disconnecting a USB MIDI device (although delayed, seems to be coupled to calling something like midiInGetNumDevs).
		// In the latter case, we need to inform the UI.
		if(CMainFrame::shMidiIn != nullptr)
		{
			pMainFrm->SendMessage(WM_COMMAND, ID_MIDI_RECORD);
		}
	}
}


bool CMainFrame::midiOpenDevice(bool showSettings)
{
	if (shMidiIn) return true;
	
	if (midiInOpen(&shMidiIn, TrackerSettings::Instance().GetCurrentMIDIDevice(), (DWORD_PTR)MidiInCallBack, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
	{
		shMidiIn = nullptr;

		// Show MIDI configuration on fail.
		if(showSettings)
		{
			CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_MIDI;
			CMainFrame::GetMainFrame()->OnViewOptions();
		}

		// Let's see if the user updated the settings.
		if(midiInOpen(&shMidiIn, TrackerSettings::Instance().GetCurrentMIDIDevice(), (DWORD_PTR)MidiInCallBack, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
		{
			shMidiIn = nullptr;
			return false;
		}
	}
	midiInStart(shMidiIn);
	return true;
}


void CMainFrame::midiCloseDevice()
{
	if (shMidiIn)
	{
		// Prevent infinite loop in MIM_CLOSE
		auto handle = shMidiIn;
		shMidiIn = nullptr;
		midiInClose(handle);
	}
}


void CMainFrame::OnMidiRecord()
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
{
	if (pCmdUI) pCmdUI->SetCheck((shMidiIn) ? TRUE : FALSE);
}


OPENMPT_NAMESPACE_END
