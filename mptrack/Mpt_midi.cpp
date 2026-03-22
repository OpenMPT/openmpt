/*
 * MPT_MIDI.cpp
 * ------------
 * Purpose: MIDI Input handling code.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Dlsbank.h"
#include "InputHandler.h"
#include "Mainfrm.h"
#include "resource.h"
#include "TrackerSettings.h"
#include "WindowMessages.h"
#include "../soundlib/MIDIEvents.h"

#include <mmsystem.h>


OPENMPT_NAMESPACE_BEGIN


#ifdef MPT_ALL_LOGGING
#define MPTMIDI_RECORDLOG
#endif

//Get Midi message(dwParam1), apply MIDI settings having effect on volume, and return
//the volume value [0, 256]. In addition value -1 is used as 'use default value'-indicator.
int CMainFrame::ApplyVolumeRelatedSettings(const DWORD &dwParam1, uint8 midiChannelVolume)
{
	int nVol = MIDIEvents::GetDataByte2FromEvent(dwParam1);
	const FlagSet<MidiSetup> midiSetup = TrackerSettings::Instance().midiSetup;
	if(midiSetup[MidiSetup::RecordVelocity])
	{
		if(!midiSetup[MidiSetup::ApplyChannelVolumeToVelocity])
			midiChannelVolume = 127;
		nVol = Util::muldivr_unsigned(CDLSBank::DLSMidiVolumeToLinear(nVol), TrackerSettings::Instance().midiVelocityAmp * midiChannelVolume, 100 * 127 * 256);
		Limit(nVol, 1, 256);
	} else
	{
		// Case: No velocity record.
		if(midiSetup[MidiSetup::ApplyChannelVolumeToVelocity])
			nVol = (midiChannelVolume + 1) * 2;
		else //Use default volume
			nVol = -1;
	}

	return nVol;
}


void ApplyTransposeKeyboardSetting(CMainFrame &rMainFrm, uint32 &midiMsg)
{
	const FlagSet<MidiSetup> midiSetup = TrackerSettings::Instance().midiSetup;
	if(midiSetup[MidiSetup::TransposeKeyboard]
		&& (MIDIEvents::GetChannelFromEvent(midiMsg) != 9))
	{
		int nTranspose = rMainFrm.GetBaseOctave() - 4;
		if (nTranspose)
		{
			int note = MIDIEvents::GetDataByte1FromEvent(midiMsg);
			if (note < 0x80)
			{
				note += nTranspose * 12;
				Limit(note, 0, NOTE_MAX - NOTE_MIN);

				midiMsg &= 0xffff00ff;

				midiMsg |= (note << 8);
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// MMSYSTEM Midi Record

void CALLBACK MidiInCallBack(HMIDIIN inHandle, UINT wMsg, DWORD_PTR instance, DWORD_PTR midiMsg, DWORD_PTR timestamp)
{
	CMainFrame *pMainFrm = reinterpret_cast<CMainFrame *>(instance);
	if(!pMainFrm)
		return;

#ifdef MPTMIDI_RECORDLOG
	DWORD dwMidiStatus = midiMsg & 0xFF;
	DWORD dwMidiByte1 = (midiMsg >> 8) & 0xFF;
	DWORD dwMidiByte2 = (midiMsg >> 16) & 0xFF;
	MPT_LOG_GLOBAL(LogDebug, "MIDI", MPT_UFORMAT("time={}ms status={} data={}.{}")(mpt::ufmt::dec(timestamp), mpt::ufmt::HEX0<2>(dwMidiStatus), mpt::ufmt::HEX0<2>(dwMidiByte1), mpt::ufmt::HEX0<2>(dwMidiByte2)));
#endif

	HWND hWndMidi = pMainFrm->GetMidiRecordWnd();
	if(wMsg == MIM_DATA || wMsg == MIM_MOREDATA)
	{
		uint32 data = static_cast<uint32>(midiMsg);
		if(::IsWindow(hWndMidi))
		{
			switch(MIDIEvents::GetTypeFromEvent(data))
			{
			case MIDIEvents::evNoteOff:	// Note Off
			case MIDIEvents::evNoteOn:	// Note On
				ApplyTransposeKeyboardSetting(*pMainFrm, data);
				[[fallthrough]];
			default:
				if(::PostMessage(hWndMidi, WM_MOD_MIDIMSG, data, timestamp))
					return;	// Message has been handled
				break;
			}
		}
		// Pass MIDI to keyboard handler
		CMainFrame::GetInputHandler()->HandleMIDIMessage(kCtxAllContexts, data);
	} else if(wMsg == MIM_LONGDATA)
	{
		// SysEx
		if(MIDIHDR &sysex = *reinterpret_cast<MIDIHDR *>(midiMsg); sysex.dwBytesRecorded)
		{
			std::lock_guard<mpt::mutex> guard{pMainFrm->midiInData.dataMutex};
			if(auto callback = pMainFrm->GetMidiSysexCallback())
				callback(mpt::const_byte_span{mpt::byte_cast<const std::byte *>(sysex.lpData), sysex.dwBytesRecorded});
			midiInAddBuffer(inHandle, &sysex, sizeof(sysex));
		}
	} else if(wMsg == MIM_CLOSE)
	{
		// midiInClose will trigger this, but also disconnecting a USB MIDI device (although delayed, seems to be coupled to calling something like midiInGetNumDevs).
		// In the latter case, we need to inform the UI.
		if(pMainFrm->midiInData.inHandle != nullptr)
		{
			pMainFrm->SendMessage(WM_COMMAND, ID_MIDI_RECORD);
		}
	}
}


bool CMainFrame::midiOpenDevice(bool showSettings)
{
	if (midiInData.inHandle) return true;
	
	if (midiInOpen(&midiInData.inHandle, TrackerSettings::Instance().GetCurrentMIDIDevice(), reinterpret_cast<DWORD_PTR>(MidiInCallBack), reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
	{
		midiInData.inHandle = nullptr;

		// Show MIDI configuration on fail.
		if(showSettings)
		{
			CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_MIDI;
			CMainFrame::GetMainFrame()->OnViewOptions();
		}

		// Let's see if the user updated the settings.
		if(midiInOpen(&midiInData.inHandle, TrackerSettings::Instance().GetCurrentMIDIDevice(), reinterpret_cast<DWORD_PTR>(MidiInCallBack), reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
		{
			midiInData.inHandle = nullptr;
			return false;
		}
	}

	midiInData.sysexBuffers.resize(16);
	for(auto &buffer : midiInData.sysexBuffers)
	{
		buffer.data.resize(4096);
		buffer.header.lpData = mpt::byte_cast<char *>(buffer.data.data());
		buffer.header.dwBufferLength = mpt::saturate_cast<DWORD>(buffer.data.size());
		buffer.header.dwFlags = 0;

		midiInPrepareHeader(midiInData.inHandle, &buffer.header, sizeof(buffer.header));
		midiInAddBuffer(midiInData.inHandle, &buffer.header, sizeof(buffer.header));
	}

	midiInStart(midiInData.inHandle);
	return true;
}


void CMainFrame::midiCloseDevice()
{
	if(midiInData.inHandle)
	{
		// Prevent infinite loop in MIM_CLOSE
		auto handle = midiInData.inHandle;
		midiInData.inHandle = nullptr;

		std::lock_guard<mpt::mutex> guard{midiInData.dataMutex};
		midiInReset(handle);
		midiInStop(handle);

		for(auto &buffer : midiInData.sysexBuffers)
		{
			midiInUnprepareHeader(handle, &buffer.header, sizeof(buffer.header));
		}
		midiInClose(handle);
	}
}


void CMainFrame::OnMidiRecord()
{
	if(midiInData.inHandle)
	{
		midiCloseDevice();
	} else
	{
		midiOpenDevice();
	}
}


void CMainFrame::OnUpdateMidiRecord(CCmdUI *pCmdUI)
{
	if (pCmdUI) pCmdUI->SetCheck((midiInData.inHandle) ? TRUE : FALSE);
}


OPENMPT_NAMESPACE_END
