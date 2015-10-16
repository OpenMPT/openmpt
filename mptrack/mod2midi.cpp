/*
 * mod2midi.cpp
 * ------------
 * Purpose: Module to MIDI conversion (dialog + conversion code).
 * Notes  : This code makes use of the existing VST MIDI output functionality, so requires OpenMPT to be built with VST support.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "../common/StringFixer.h"
#include "../common/mptFileIO.h"
#include "mod2midi.h"
#include "Vstplug.h"
#include <sstream>


#ifndef NO_VST

OPENMPT_NAMESPACE_BEGIN

namespace MidiExport
{
	// MIDI file resolution
	const int32 ppq = 480;

	enum StringType
	{
		kText = 1,
		kCopyright = 2,
		kTrackName = 3,
		kInstrument = 4,
		kLyric = 5,
		kMarker = 6,
		kCue = 7,
	};

	class MidiInstrument
	{
		ModInstrument m_instr;
		AEffect m_effect;
		VstEvents *m_events;
		const CSoundFile *m_sndFile;
		MidiInstrument *m_tempoTrack;	// Pointer to tempo track, nullptr if this is the tempo track

		std::ostringstream f;
		double m_tempo;
		double m_samplePos;		// Current sample position
		double m_prevEventTime;	// Sample position of previous event
		double m_sampleRate;
		int32 m_oldSigNumerator, m_oldSigDenominator;
		int32 m_oldGlobalVol;
		uint32 m_ticks;			// MIDI ticks since previous event

		// Calculate how many MIDI ticks have passed since the last written event
		void UpdateTicksSinceLastEvent()
		{
			m_ticks += Util::Round<uint32>((m_samplePos - m_prevEventTime) * m_tempo * static_cast<double>(MidiExport::ppq) / (m_sampleRate * 60.0));
			m_prevEventTime = m_samplePos;
		}
		
		// Write delta tick count since last event
		void WriteTicks()
		{
			mpt::IO::WriteVarInt(f, m_ticks);
			m_ticks = 0;
		}

	public:

		operator ModInstrument& () { return m_instr; }
		operator AEffect& () { return m_effect; }

		MidiInstrument()
			: m_events(nullptr)
			, m_sndFile(nullptr)
			, m_tempoTrack(nullptr)
			, m_tempo(0.0)
			, m_samplePos(0.0), m_prevEventTime(0.0), m_sampleRate(1.0)
			, m_oldSigNumerator(0), m_oldSigDenominator(0)
			, m_oldGlobalVol(-1)
			, m_ticks(0)
		{
			// Initialize VST struct, needs to be done first
			MemsetZero(m_effect);
			m_effect.magic = kEffectMagic;
			m_effect.dispatcher = Dispatcher;
			m_effect.process = Process;
			m_effect.setParameter = SetParameter;
			m_effect.getParameter = GetParameter;

			m_effect.numPrograms = 0;
			m_effect.numParams = 0;
			m_effect.numInputs = 0;
			m_effect.numOutputs = 2;

			m_effect.flags = effFlagsCanReplacing | effFlagsIsSynth;

			m_effect.resvd1 = 0;
			m_effect.resvd2 = 0;

			m_effect.initialDelay = 0;

			m_effect.realQualities = 0;
			m_effect.offQualities = 0;
			m_effect.ioRatio = 0.0f;

			m_effect.object = this;
			m_effect.user = nullptr;

			m_effect.uniqueID = 0;
			m_effect.version = 0;

			m_effect.processReplacing = Process;
		}

		void Initialize(const CSoundFile &sndFile, MidiInstrument *tempoTrack, const char *name)
		{
			m_sndFile = &sndFile;
			m_tempoTrack = tempoTrack;
			
			// This is the tempo track, don't write any playback-related stuff
			if(tempoTrack == nullptr) return;

			// Write instrument name
			WriteString(kTrackName, name);
			// Set up MIDI pitch wheel depth
			uint8 firstCh = m_instr.nMidiChannel, lastCh = m_instr.nMidiChannel;
			if(firstCh == MidiMappedChannel || firstCh == MidiNoChannel)
			{
				firstCh = 0;
				lastCh = 16;
			} else
			{
				firstCh--;
			}
			for(uint8 i = firstCh; i < lastCh; i++)
			{
				uint8 ch = 0xB0 | i;
				uint8 msg[12] = { 0x00, ch, 0x64, 0x00, 0x00, ch, 0x65, 0x00, 0x00, ch, 0x06, m_instr.midiPWD };
				mpt::IO::WriteRaw(f, msg, 12);
			}
		}

		static VstIntPtr Dispatcher(AEffect *effect, VstInt32 opcode, VstInt32 /*index*/, VstIntPtr /*value*/, void *ptr, float /*opt*/)
		{
			switch(opcode)
			{
			case effProcessEvents:
				static_cast<MidiInstrument *>(effect->object)->m_events = static_cast<VstEvents *>(ptr);
				return 1;
			}
			return 0;
		}

		static void Process(AEffect *effect, float **, float **, VstInt32 numFrames)
		{
			VstIntPtr ptr = CVstPluginManager::MasterCallBack(effect, audioMasterGetTime, 0, kVstPpqPosValid | kVstTempoValid | kVstTimeSigValid, nullptr, 0.0f);
			VstTimeInfo timeInfo = *FromVstPtr<VstTimeInfo>(ptr);
			static_cast<MidiInstrument *>(effect->object)->Process(timeInfo, numFrames);
		}
		void Process(const VstTimeInfo &timeInfo, int32 numFrames)
		{
			const int32 numEvents = m_events != nullptr ? m_events->numEvents : 0;
			m_samplePos = timeInfo.samplePos;
			m_sampleRate = timeInfo.sampleRate;

			const bool tempoChanged = timeInfo.tempo != m_tempo;
			const bool sigChanged = timeInfo.timeSigNumerator != m_oldSigNumerator || timeInfo.timeSigDenominator != m_oldSigDenominator;
			const bool volChanged = m_sndFile->m_PlayState.m_nGlobalVolume != m_oldGlobalVol;

			// Anything interesting happening in this process call at all?
			if(!tempoChanged && !sigChanged && !volChanged && numEvents == 0)
			{
				m_samplePos += numFrames;
				if(m_tempoTrack != nullptr) m_tempoTrack->m_samplePos = std::max(m_tempoTrack->m_samplePos, m_samplePos);
				return;
			}

			UpdateTicksSinceLastEvent();

			if(timeInfo.tempo > 0.0) m_tempo = timeInfo.tempo;
			m_oldSigNumerator = timeInfo.timeSigNumerator;
			m_oldSigDenominator = timeInfo.timeSigDenominator;
			m_oldGlobalVol = m_sndFile->m_PlayState.m_nGlobalVolume;

			if(m_tempoTrack == nullptr)
			{
				// This is the tempo track
				if(tempoChanged && timeInfo.tempo > 0.0)
				{
					// Write MIDI tempo
					WriteTicks();

					int32 mspq = Util::Round<int32>(60000000.0 / timeInfo.tempo);
					uint8 msg[6] = { 0xFF, 0x51, 0x03, (mspq >> 16) & 0xFF, (mspq >> 8) & 0xFF, mspq & 0xFF };
					mpt::IO::WriteRaw(f, msg, 6);
				}

				if(sigChanged)
				{
					// Write MIDI time signature
					WriteTicks();

					uint8 msg[7] = { 0xFF, 0x58, 0x04, static_cast<char>(timeInfo.timeSigNumerator), 2, 24, 8 };
					mpt::IO::WriteRaw(f, msg, 7);
				}

				if(volChanged)
				{
					// Write MIDI master volume
					WriteTicks();

					int32 midiVol = Util::muldiv(m_oldGlobalVol, 0x3FFF, MAX_GLOBAL_VOLUME);
					uint8 msg[9] = { 0xF0, 0x07, 0x7F, 0x7F, 0x04, 0x01, midiVol & 0x7F, (midiVol >> 7) & 0x7F, 0xF7 };
					mpt::IO::WriteRaw(f, msg, 9);
				}
			} else
			{
				// This is not the tempo track, defer tempo and volume changes to that track instead.
				m_tempoTrack->Process(timeInfo, numFrames);
			}

			// TODO: support event.deltaFrames (currently not used by OpenMPT)
			for(int32 i = 0; i < numEvents; i++)
			{
				WriteTicks();

				const VstEvent &event = *m_events->events[i];
				if(event.type == kVstMidiType)
				{
					const VstMidiEvent &midiEvent = reinterpret_cast<const VstMidiEvent &>(event);
					char midiData[4];
					MemCopy<char [4]>(midiData, midiEvent.midiData);
					size_t msgSize = 3;
					switch(midiData[0] & 0xF0)
					{
					case 0xC0:
					case 0xD0:
						msgSize = 2;
						break;
					case 0xF0:
						switch(midiData[0])
						{
						case 0xF1:
						case 0xF3:
							msgSize = 2;
							break;
						case 0xF2:
							msgSize = 3;
							break;
						default:
							msgSize = 1;
							break;
						}
						break;
					}
					// TODO: If channels are shared between tracks, program change events need to be inserted!
					if((midiData[0] & 0xF0) != 0xF0 && m_instr.nMidiChannel == MidiMappedChannel)
					{
						// Avoid putting things on channel 10
						midiData[0] &= 0xF7;
					}
					mpt::IO::WriteRaw(f, midiData, msgSize);
				} else if(event.type == kVstSysExType)
				{
					const VstMidiSysexEvent &sysexEvent = reinterpret_cast<const VstMidiSysexEvent &>(event);
					if(sysexEvent.dumpBytes > 1)
					{
						mpt::IO::WriteIntBE<uint8>(f, 0xF0);
						mpt::IO::WriteVarInt(f, static_cast<uint32>(sysexEvent.dumpBytes - 1));
						mpt::IO::WriteRaw(f, sysexEvent.sysexDump + 1, sysexEvent.dumpBytes - 1);
					}
				}
			}
			m_samplePos += numFrames;
		}

		// Write end marker and return the stream
		const std::ostringstream& Finalise()
		{
			UpdateTicksSinceLastEvent();
			WriteTicks();

			uint8 msg[3] = { 0xFF, 0x2F, 0x00 };
			mpt::IO::WriteRaw(f, msg, 3);

			return f;
		}

		// No-op setParameter
		static void SetParameter(AEffect *, VstInt32, float) { }
		// No-op getParameter
		static float GetParameter(AEffect *, VstInt32) { return 0.0f; }

		void WriteString(StringType strType, const mpt::ustring &str)
		{
			WriteString(strType, mpt::ToCharset(mpt::CharsetLocaleOrUTF8, str));
		}

		void WriteString(StringType strType, const std::string &str)
		{
			if(str.length() > 0)
			{
				uint8 msg[3] = { 0x00, 0xFF, static_cast<uint8>(strType) };
				mpt::IO::WriteRaw(f, msg, 3);
				mpt::IO::WriteVarInt(f, str.length());
				mpt::IO::WriteRaw(f, str.c_str(), str.length());
			}
		}

		void WriteString(StringType strType, const char *str)
		{
			const size_t len = strlen(str);
			if(len > 0)
			{
				uint8 msg[3] = { 0x00, 0xFF, static_cast<uint8>(strType) };
				mpt::IO::WriteRaw(f, msg, 3);
				mpt::IO::WriteVarInt(f, len);
				mpt::IO::WriteRaw(f, str, len);
			}
		}
	};


	class Conversion
	{
		std::vector<ModInstrument *> m_oldInstruments;
		MidiInstrument *m_instruments;
		SNDMIXPLUGIN m_oldPlugins[MAX_MIXPLUGINS];
		VSTPluginLib m_plugFactory;
		CSoundFile &m_sndFile;
		mpt::ofstream &m_file;
		bool m_wasInstrumentMode;

	public:
		Conversion(CSoundFile &sndFile, const InstrMap &instrMap, mpt::ofstream &file)
			: m_oldInstruments(sndFile.GetNumInstruments())
			, m_plugFactory(mpt::PathString(), mpt::PathString())
			, m_sndFile(sndFile)
			, m_file(file)
			, m_wasInstrumentMode(sndFile.GetNumInstruments() > 0)
		{
			MemCopy(m_oldPlugins, m_sndFile.m_MixPlugins);
			MemsetZero(m_sndFile.m_MixPlugins);
			for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
			{
				m_oldInstruments[i - 1] = m_sndFile.Instruments[i];
			}
			if(!m_wasInstrumentMode)
			{
				m_sndFile.m_nInstruments = m_sndFile.m_nSamples;
			}
			m_instruments = new (std::nothrow) MidiInstrument[1 + m_sndFile.GetNumInstruments()];
			if(m_instruments == nullptr)
			{
				return;
			}

			PLUGINDEX nextPlug = 0;
			for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
			{
				MidiInstrument &midiInstr = m_instruments[i];
				ModInstrument &instr = midiInstr;
				
				m_sndFile.Instruments[i] = &instr;

				if(!m_sndFile.IsInstrumentUsed(i) || (m_wasInstrumentMode && m_oldInstruments[i - 1] == nullptr))
				{
					m_sndFile.Instruments[i] = nullptr;
					continue;
				}

				ModInstrument &oldInstr = m_wasInstrumentMode ? *m_oldInstruments[i - 1] : instr;
				if(m_wasInstrumentMode) instr = oldInstr;
				if(oldInstr.nMixPlug == 0 || oldInstr.nMidiChannel == MidiNoChannel)
				{
					instr.midiPWD = 12;
				}
				instr.nMidiChannel = instrMap[i].nChannel ? (instrMap[i].nChannel - 1 + MidiFirstChannel) : MidiMappedChannel;
				if(instrMap[i].nChannel != 10)
				{
					// Melodic instrument
					instr.nMidiProgram = instrMap[i].nProgram + 1;
				} else
				{
					// Drums
					if(oldInstr.nMidiChannel != MidiFirstChannel + 9) instr.nMidiProgram = 0;
					if(instrMap[i].nProgram != 128)
					{
						for(size_t n = 0; n < CountOf(instr.NoteMap); n++)
						{
							instr.NoteMap[n] = instrMap[i].nProgram + NOTE_MIN;
						}
					}
				}

				// FIXME: Having > MAX_MIXPLUGINS used instruments won't work! So in MPTM, you can only use 250 out of 255 instruments...
				midiInstr.Initialize(m_sndFile, &m_instruments[0], m_wasInstrumentMode ? oldInstr.name : m_sndFile.GetSampleName(i));
				instr.nMixPlug = 0;
				if(nextPlug < MAX_MIXPLUGINS)
				{
					SNDMIXPLUGIN &mixPlugin = m_sndFile.m_MixPlugins[nextPlug];
					instr.nMixPlug = nextPlug + 1;
					m_sndFile.m_MixPlugins[nextPlug].pMixPlugin = new (std::nothrow) CVstPlugin(nullptr, m_plugFactory, mixPlugin, midiInstr, m_sndFile);
				}

				nextPlug++;
			}

			mpt::IO::WriteRaw(m_file, "MThd", 4);
			mpt::IO::WriteIntBE<uint32>(m_file, 6);
			mpt::IO::WriteIntBE<uint16>(m_file, 1);	// Type 1 MIDI - multiple simultaneous tracks
			mpt::IO::WriteIntBE<uint16>(m_file, m_sndFile.GetNumInstruments() + 1);	// Number of tracks
			mpt::IO::WriteIntBE<uint16>(m_file, MidiExport::ppq);

			MidiInstrument &tempoTrack = m_instruments[0];
			tempoTrack.Initialize(m_sndFile, nullptr, nullptr);
			tempoTrack.WriteString(kTrackName, m_sndFile.songName);
			tempoTrack.WriteString(kText, m_sndFile.songMessage);
			tempoTrack.WriteString(kCopyright, m_sndFile.songArtist);
		}

		~Conversion()
		{
			delete[] m_instruments;
		}

		void Finalise()
		{
			for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
			{
				IMixPlugin *plug = m_sndFile.m_MixPlugins[i].pMixPlugin;
				if(plug != nullptr) plug->HardAllNotesOff();
			}

			for(INSTRUMENTINDEX i = 0; i <= m_sndFile.GetNumInstruments(); i++)
			{
				std::string data = m_instruments[i].Finalise().str();
				if(!data.empty())
				{
					mpt::IO::WriteRaw(m_file, "MTrk", 4);
					mpt::IO::WriteIntBE<uint32>(m_file, data.size());
					mpt::IO::WriteRaw(m_file, &data[0], data.size());
				}

				if(i > 0) m_sndFile.Instruments[i] = m_wasInstrumentMode ? m_oldInstruments[i - 1] : nullptr;
			}
			if(!m_wasInstrumentMode)
			{
				m_sndFile.m_nInstruments = 0;
			}

			for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
			{
				m_sndFile.m_MixPlugins[i].Destroy();
			}
			MemCopy(m_sndFile.m_MixPlugins, m_oldPlugins);
		}
	};


	class DummyAudioTarget : public IAudioReadTarget
	{
	public:
		virtual void DataCallback(int *, std::size_t, std::size_t) { }
	};
}


////////////////////////////////////////////////////////////////////////////////////
//
// CModToMidi dialog implementation
//

BEGIN_MESSAGE_MAP(CModToMidi, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO1,	UpdateDialog)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnChannelChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,	OnProgramChanged)
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

void CModToMidi::DoDataExchange(CDataExchange *pDX)
//-------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1,	m_CbnInstrument);
	DDX_Control(pDX, IDC_COMBO2,	m_CbnChannel);
	DDX_Control(pDX, IDC_COMBO3,	m_CbnProgram);
	DDX_Control(pDX, IDC_SPIN1,		m_SpinInstrument);
}


CModToMidi::CModToMidi(CSoundFile &sndFile, CWnd *pWndParent)
	: CDialog(IDD_MOD2MIDI, pWndParent)
	, m_instrMap((sndFile.GetNumInstruments() ? sndFile.GetNumInstruments() : sndFile.GetNumSamples()) + 1)
	, m_sndFile(sndFile)
//--------------------------------------------------------------------------------------------
{
	for (INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
	{
		ModInstrument *pIns = m_sndFile.Instruments[i];
		if ((pIns) && (pIns->nMidiChannel <= 16))
		{
			m_instrMap[i].nChannel = pIns->nMidiChannel;
			if (m_instrMap[i].nChannel == 10)
			{
				if ((pIns->nMidiProgram > 20) && (pIns->nMidiProgram < 120))
					m_instrMap[i].nProgram = pIns->nMidiProgram;
				else
					m_instrMap[i].nProgram = (pIns->NoteMap[60] - NOTE_MIN) & 0x7F;
			} else
			{
				m_instrMap[i].nProgram = (pIns->nMidiProgram > 0) ? ((pIns->nMidiProgram - 1) & 0x7F) : 0;
			}
		}
	}
}


BOOL CModToMidi::OnInitDialog()
//-----------------------------
{
	CHAR s[256];

	CDialog::OnInitDialog();
	// Fill instruments box
	m_SpinInstrument.SetRange(-1, 1);
	m_SpinInstrument.SetPos(0);
	m_nCurrInstr = 1;
	if (m_sndFile.GetNumInstruments())
	{
		for(INSTRUMENTINDEX nIns = 1; nIns <= m_sndFile.GetNumInstruments(); nIns++)
		{
			ModInstrument *pIns = m_sndFile.Instruments[nIns];
			if ((pIns) && (m_sndFile.IsInstrumentUsed(nIns)))
			{
				const CString name = m_sndFile.GetpModDoc()->GetPatternViewInstrumentName(nIns);
				m_CbnInstrument.SetItemData(m_CbnInstrument.AddString(name), nIns);
			}
		}
	} else
	{
		for(SAMPLEINDEX nSmp = 1; nSmp <= m_sndFile.GetNumSamples(); nSmp++)
		{
			if ((m_sndFile.GetSample(nSmp).pSample)
			 && (m_sndFile.IsSampleUsed(nSmp)))
			{
				wsprintf(s, "%02d: %s", nSmp, m_sndFile.m_szNames[nSmp]);
				m_CbnInstrument.SetItemData(m_CbnInstrument.AddString(s), nSmp);
			}
		}
	}
	// Fill channels box
	m_CbnChannel.SetItemData(m_CbnChannel.AddString("Melodic (any)"), 0);
	m_CbnChannel.SetItemData(m_CbnChannel.AddString("Percussions"), 10);
	for (UINT iCh=1; iCh<=16; iCh++) if (iCh != 10)
	{
		wsprintf(s, "Melodic %d", iCh);
		m_CbnChannel.SetItemData(m_CbnChannel.AddString(s), iCh);
	}
	m_nCurrInstr = 1;
	m_bPerc = TRUE;
	m_CbnChannel.SetCurSel(0);
	m_CbnInstrument.SetCurSel(0);
	FillProgramBox(false);
	m_CbnProgram.SetCurSel(0);
	UpdateDialog();
	return TRUE;
}


void CModToMidi::FillProgramBox(bool percussion)
//----------------------------------------------
{
	CHAR s[256];
	
	if (m_bPerc == percussion) return;
	m_CbnProgram.ResetContent();
	if (percussion)
	{
		for (int i = 0; i < 61; i++)
		{
			int note = i + 24;
			wsprintf(s, "%d (%s%d): %s", note, szNoteNames[note % 12], note / 12, szMidiPercussionNames[i]);
			m_CbnProgram.SetItemData(m_CbnProgram.AddString(s), note);
		}
		m_CbnProgram.SetItemData(m_CbnProgram.AddString(_T("Mapped")), 128);
	} else
	{
		for (int i = 0; i < 128; i++)
		{
			wsprintf(s, "%03d: %s", i + 1, szMidiProgramNames[i]);
			m_CbnProgram.SetItemData(m_CbnProgram.AddString(s), i);
		}
	}
	m_bPerc = percussion;
}


void CModToMidi::UpdateDialog()
//-----------------------------
{
	m_nCurrInstr = m_CbnInstrument.GetItemData(m_CbnInstrument.GetCurSel());
	if ((m_nCurrInstr > 0) && (m_nCurrInstr < MAX_SAMPLES))
	{
		UINT nMidiCh = m_instrMap[m_nCurrInstr].nChannel;
		if (nMidiCh > 16) nMidiCh = 0;
		if ((!m_bPerc) && (nMidiCh == 10))
		{
			FillProgramBox(true);
		} else
		if ((m_bPerc) && (nMidiCh != 10))
		{
			FillProgramBox(false);
		}
		if (nMidiCh == 10) nMidiCh = 1; else
		if ((nMidiCh) && (nMidiCh < 10)) nMidiCh++;
		m_CbnChannel.SetCurSel(nMidiCh);
		UINT nMidiProgram = m_instrMap[m_nCurrInstr].nProgram;
		if (m_bPerc)
		{
			nMidiProgram -= 24;
			if (nMidiProgram > 60) nMidiProgram = 24;
		} else
		{
			if (nMidiProgram > 127) nMidiProgram = 0;
		}
		m_CbnProgram.SetCurSel(nMidiProgram);
	}
}


void CModToMidi::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//-------------------------------------------------------------------------
{
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
	short int pos = (short int)m_SpinInstrument.GetPos();
	if (pos)
	{
		m_SpinInstrument.SetPos(0);
		int nmax = m_CbnInstrument.GetCount();
		int nins = m_CbnInstrument.GetCurSel() + pos;
		if (nins < 0) nins = nmax-1;
		if (nins >= nmax) nins = 0;
		m_CbnInstrument.SetCurSel(nins);
		UpdateDialog();
	}
}


void CModToMidi::OnChannelChanged()
//---------------------------------
{
	uint8 nMidiCh = static_cast<uint8>(m_CbnChannel.GetItemData(m_CbnChannel.GetCurSel()));
	if (nMidiCh > 16) return;
	if ((m_nCurrInstr > 0) && (m_nCurrInstr < MAX_SAMPLES))
	{
		m_instrMap[m_nCurrInstr].nChannel = nMidiCh;
		if (((!m_bPerc) && (nMidiCh == 10))
		 || ((m_bPerc) && (nMidiCh != 10)))
		{
			UpdateDialog();
		}
	}
}


void CModToMidi::OnProgramChanged()
//---------------------------------
{
	DWORD_PTR nProgram = m_CbnProgram.GetItemData(m_CbnProgram.GetCurSel());
	if (nProgram == CB_ERR) return;
	if ((m_nCurrInstr > 0) && (m_nCurrInstr < MAX_SAMPLES))
	{
		m_instrMap[m_nCurrInstr].nProgram = static_cast<uint8>(nProgram);
	}
}


void CDoMidiConvert::Run()
//------------------------
{
	mpt::ofstream f(m_fileName, std::ios::binary | std::ios::trunc);
	if(!f.good()) 
	{
		Reporting::Error("Could not open file for writing. Is it open in another application?");
		EndDialog(IDCANCEL);
		return;
	}

	CMainFrame::GetMainFrame()->PauseMod(m_sndFile.GetpModDoc());
	MidiExport::Conversion conv(m_sndFile, m_instrMap, f);

	double duration = m_sndFile.GetLength(eNoAdjust).front().duration;
	uint64 totalSamples = Util::Round<uint64>(duration * m_sndFile.m_MixerSettings.gdwMixingFreq);
	SetRange(0, Util::Round<uint32>(duration));
	DWORD startTime = timeGetTime();

	m_sndFile.SetCurrentOrder(0);
	m_sndFile.GetLength(eAdjust, GetLengthTarget(0, 0));
	m_sndFile.m_SongFlags.reset(SONG_PATTERNLOOP);
	int oldRepCount = m_sndFile.GetRepeatCount();
	m_sndFile.SetRepeatCount(0);
	m_sndFile.m_bIsRendering = true;

	MidiExport::DummyAudioTarget target;
	UINT ok = IDOK;
	int n = 0;
	while(m_sndFile.Read(MIXBUFFERSIZE, target) > 0)
	{
		n++;
		if(n == 100)
		{
			n = 0;

			uint64 curSamples = m_sndFile.GetTotalSampleCount();
			uint32 curTime = static_cast<uint32>(curSamples / m_sndFile.m_MixerSettings.gdwMixingFreq);
			const DWORD currentTime = timeGetTime();
			uint32 timeRemaining = 0;
			if(curSamples > 0 && curSamples < totalSamples)
			{
				timeRemaining = static_cast<uint32>(((currentTime - startTime) * (totalSamples - curSamples) / curSamples) / 1000);
			}
			TCHAR s[128];
			_stprintf(s, _T("Rendering file... (%umn%02us, %umn%02us remaining)"), curTime / 60, curTime % 60, timeRemaining / 60, timeRemaining % 60u);
			SetText(s);
			SetProgress(curTime);
			ProcessMessages();

			if(m_abort)
			{
				ok = IDCANCEL;
				break;
			}
		}
	}

	conv.Finalise();

	m_sndFile.m_bIsRendering = false;
	m_sndFile.m_PatternCuePoints.clear();
	m_sndFile.SetRepeatCount(oldRepCount);

	EndDialog(ok);
}

OPENMPT_NAMESPACE_END

#endif // NO_VST
