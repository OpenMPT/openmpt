#include "stdafx.h"
#include <mmsystem.h>
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"


//#define MPTMIDI_RECORDLOG

// Midi Input globals
HMIDIIN CMainFrame::shMidiIn = NULL;


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
				if (dwTime - gdwLastMidiEvtTime < 20*3) break;
				gdwLastMidiEvtTime = dwTime; // continue
			} else break;
		case 0x80:	// Note Off
		case 0x90:	// Note On
			{
				if ((CMainFrame::m_dwMidiSetup & MIDISETUP_TRANSPOSEKEYBOARD)
				 && ((dwParam1 & 0x0f) != 9))
				{
					int nTranspose = pMainFrm->GetBaseOctave() - 4;
					if (nTranspose)
					{
						int note = (dwParam1 >> 8) & 0xFF;
						if (note < 0x80)
						{
							note += nTranspose*12;
							if (note < 0) note = 0;
							if (note > 119) note = 119;
// -> CODE#0011
// -> DESC="bug fix about transpose midi keyboard option"
//							dwParam1 &= 0xffffff00;
							dwParam1 &= 0xffff00ff;
// -! BUG_FIX#0011
							dwParam1 |= (note<<8);
						}
					}
				}
				::PostMessage(hWndMidi, WM_MOD_MIDIMSG, dwParam1, dwParam2);
			}
			break;
		}
	}
}


BOOL CMainFrame::midiOpenDevice()
//-------------------------------
{
	if (shMidiIn) return TRUE;
	if (midiInOpen(&shMidiIn, m_nMidiDevice, (DWORD)MidiInCallBack, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
	{
		shMidiIn = NULL;
		return FALSE;
	}
	midiInStart(shMidiIn);
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


