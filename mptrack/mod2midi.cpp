/*
 * mod2midi.cpp
 * ------------
 * Purpose: Module to MIDI conversion (dialog + conversion code).
 * Notes  : This code makes use of the existing MIDI plugin output functionality.
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
#include "../soundlib/plugins/PlugInterface.h"
#include "../soundlib/plugins/PluginManager.h"
#include <sstream>

#ifndef NO_PLUGINS

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

	class MidiTrack : public IMidiPlugin
	{
		ModInstrument m_instr;
		const ModInstrument *m_oldInstr;
		const CSoundFile &m_sndFile;
		MidiTrack *m_tempoTrack;	// Pointer to tempo track, nullptr if this is the tempo track

		std::ostringstream f;
		double m_tempo;
		CSoundFile::samplecount_t m_samplePos;		// Current sample position
		CSoundFile::samplecount_t m_prevEventTime;	// Sample position of previous event
		uint32 m_ticks;			// MIDI ticks since previous event
		uint32 m_sampleRate;
		uint32 m_oldSigNumerator;
		int32 m_oldGlobalVol;

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

		MidiTrack(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct, MidiTrack *tempoTrack, const char *name, const ModInstrument * oldInstr)
			: IMidiPlugin(factory, sndFile, mixStruct)
			, m_oldInstr(oldInstr)
			, m_sndFile(sndFile)
			, m_tempoTrack(tempoTrack)
			, m_tempo(0.0)
			, m_samplePos(0), m_prevEventTime(0)
			, m_ticks(0)
			, m_sampleRate(sndFile.GetSampleRate())
			, m_oldSigNumerator(0)
			, m_oldGlobalVol(-1)
		{
			// Write instrument / song name
			WriteString(kTrackName, name);

			// This is the tempo track, don't write any playback-related stuff
			if(tempoTrack == nullptr) return;

			m_pMixStruct->pMixPlugin = this;

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


		void UpdateGlobals()
		{
			m_samplePos = m_sndFile.GetTotalSampleCount();
			m_sampleRate = m_sndFile.GetSampleRate();

			const double curTempo = m_sndFile.GetCurrentBPM();
			const ROWINDEX rpb = std::max(m_sndFile.m_PlayState.m_nCurrentRowsPerBeat, ROWINDEX(1));
			const uint32 timeSigNumerator = std::max(m_sndFile.m_PlayState.m_nCurrentRowsPerMeasure, rpb) / rpb;

			const bool tempoChanged = curTempo != m_tempo;
			const bool sigChanged = timeSigNumerator != m_oldSigNumerator;
			const bool volChanged = m_sndFile.m_PlayState.m_nGlobalVolume != m_oldGlobalVol;

			UpdateTicksSinceLastEvent();

			if(curTempo > 0.0) m_tempo = curTempo;
			m_oldSigNumerator = timeSigNumerator;
			m_oldGlobalVol = m_sndFile.m_PlayState.m_nGlobalVolume;

			if(m_tempoTrack != nullptr)
			{
				return;
			}
			// This is the tempo track
			if(tempoChanged && curTempo > 0.0)
			{
				// Write MIDI tempo
				WriteTicks();

				int32 mspq = Util::Round<int32>(60000000.0 / curTempo);
				uint8 msg[6] = { 0xFF, 0x51, 0x03, (mspq >> 16) & 0xFF, (mspq >> 8) & 0xFF, mspq & 0xFF };
				mpt::IO::WriteRaw(f, msg, 6);
			}

			if(sigChanged)
			{
				// Write MIDI time signature
				WriteTicks();

				uint8 msg[7] = { 0xFF, 0x58, 0x04, static_cast<char>(timeSigNumerator), 2, 24, 8 };
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
		}


		virtual void Process(float *, float *, uint32 numFrames)
		{
			UpdateGlobals();
			if(m_tempoTrack != nullptr) m_tempoTrack->UpdateGlobals();

			m_samplePos += numFrames;
			if(m_tempoTrack != nullptr) m_tempoTrack->m_samplePos = std::max(m_tempoTrack->m_samplePos, m_samplePos);
		}

		// Write end marker and return the stream
		const std::ostringstream& Finalise()
		{
			HardAllNotesOff();
			UpdateTicksSinceLastEvent();
			WriteTicks();

			uint8 msg[3] = { 0xFF, 0x2F, 0x00 };
			mpt::IO::WriteRaw(f, msg, 3);

			return f;
		}

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

		virtual void Release() { }
		virtual int32 GetUID() const { return 0; }
		virtual int32 GetVersion() const { return 0; }
		virtual void Idle() { }
		virtual uint32 GetLatency() const { return 0; }

		virtual int32 GetNumPrograms() const { return 0; }
		virtual int32 GetCurrentProgram() { return 0; }
		virtual void SetCurrentProgram(int32) { }

		virtual PlugParamIndex GetNumParameters() const { return 0; }
		virtual PlugParamValue GetParameter(PlugParamIndex) { return 0; }
		virtual void SetParameter(PlugParamIndex, PlugParamValue) { }

		virtual float RenderSilence(uint32) { return 0.0f; }

		virtual bool MidiSend(uint32 dwMidiCode)
		{
			UpdateGlobals();
			UpdateTicksSinceLastEvent();

			WriteTicks();
			char midiData[4];
			memcpy(midiData, &dwMidiCode, 4);
			// TODO: If channels are shared between tracks, program change events need to be inserted!
			if((midiData[0] & 0xF0) != 0xF0 && m_instr.nMidiChannel == MidiMappedChannel)
			{
				// Avoid putting things on channel 10, so we just put everything on the first 8 channels.
				midiData[0] &= 0xF7;
			}
			mpt::IO::WriteRaw(f, midiData, MIDIEvents::GetEventLength(midiData[0]));
			return true;
		}

		virtual bool MidiSysexSend(const void *message, uint32 length)
		{
			UpdateGlobals();
			UpdateTicksSinceLastEvent();

			if(length > 1)
			{
				WriteTicks();
				mpt::IO::WriteIntBE<uint8>(f, 0xF0);
				mpt::IO::WriteVarInt(f, static_cast<uint32>(length - 1));
				mpt::IO::WriteRaw(f, static_cast<const mpt::byte *>(message) + 1, length - 1);
			}
			return true;
		}

#if 0
		virtual void MidiCC(uint8 nMidiCh, MIDIEvents::MidiCC nController, uint8 nParam, CHANNELINDEX trackChannel)
		{
			if(m_instr.nMidiChannel == MidiMappedChannel)
			{
				// For "mapped" mode, distribute tracker channels evenly over MIDI channels, but avoid channel 10 (drums)
				nMidiCh = trackChannel % 15u;
				if(nMidiCh >= 9) nMidiCh++;
			}
			IMidiPlugin::MidiCC(nMidiCh, nController, nParam, trackChannel);
		}
#endif

		virtual void MidiCommand(uint8 nMidiCh, uint8 nMidiProg, uint16 wMidiBank, uint16 note, uint16 vol, CHANNELINDEX trackChannel)
		{
#if 0
			if(m_instr.nMidiChannel == MidiMappedChannel)
			{
				// For "mapped" mode, distribute tracker channels evenly over MIDI channels, but avoid channel 10 (drums)
				nMidiCh = trackChannel % 15u;
				if(nMidiCh >= 9) nMidiCh++;
			}
#endif
			if(note == NOTE_NOTECUT && (m_oldInstr == nullptr || !(m_oldInstr->nMixPlug != 0 && m_oldInstr->HasValidMIDIChannel())))
			{
				// The default implementation does things with Note Cut that we don't want here: it cuts all notes.
				note = NOTE_KEYOFF;
			}
			IMidiPlugin::MidiCommand(nMidiCh, nMidiProg, wMidiBank, note, vol, trackChannel);
		}

		virtual void HardAllNotesOff()
		{
			for(uint8 mc = 0; mc < CountOf(m_MidiCh); mc++)
			{
				PlugInstrChannel &channel = m_MidiCh[mc];
				for(size_t i = 0; i < CountOf(channel.noteOnMap); i++)
				{
					for(CHANNELINDEX c = 0; c < CountOf(channel.noteOnMap[i]); c++)
					{
						while(channel.noteOnMap[i][c])
						{
							MidiSend(MIDIEvents::NoteOff(mc, static_cast<uint8>(i), 0));
							channel.noteOnMap[i][c]--;
						}
					}
				}
			}
		}

		virtual void Resume() { }
		virtual void Suspend() { }
		virtual void PositionChanged() { }

		virtual bool IsInstrument() const { return true; }
		virtual bool CanRecieveMidiEvents() { return true; }
		virtual bool ShouldProcessSilence() { return true; }
#ifdef MODPLUG_TRACKER
		virtual CString GetDefaultEffectName() { return CString(); }
		virtual void CacheProgramNames(int32, int32) { }
		virtual void CacheParameterNames(int32, int32) { }
		virtual CString GetParamName(PlugParamIndex) { return CString(); }
		virtual CString GetParamLabel(PlugParamIndex) { return CString(); }
		virtual CString GetParamDisplay(PlugParamIndex) { return CString(); }
		virtual CString GetCurrentProgramName() { return CString(); }
		virtual void SetCurrentProgramName(const CString &) { }
		virtual CString GetProgramName(int32) { return CString(); }
		virtual bool HasEditor() const { return false; }
		virtual void BeginSetProgram(int32) { }
		virtual void EndSetProgram() { }
#endif // MODPLUG_TRACKER
		virtual int GetNumInputChannels() const { return 0; }
		virtual int GetNumOutputChannels() const { return 0; }
		virtual bool ProgramsAreChunks() const { return false; }
		virtual size_t GetChunk(char *(&), bool) { return 0; }
		virtual void SetChunk(size_t, char *, bool) { }
	};


	class Conversion
	{
		std::vector<ModInstrument *> m_oldInstruments;
		std::vector<MidiTrack *> m_tracks;
		SNDMIXPLUGIN m_oldPlugins[MAX_MIXPLUGINS];
		VSTPluginLib m_plugFactory;
		CSoundFile &m_sndFile;
		mpt::ofstream &m_file;
		bool m_wasInstrumentMode;

	public:
		Conversion(CSoundFile &sndFile, const InstrMap &instrMap, mpt::ofstream &file)
			: m_oldInstruments(sndFile.GetNumInstruments())
			, m_plugFactory(nullptr, true, mpt::PathString(), mpt::PathString(), mpt::ustring())
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
				LimitMax(m_sndFile.m_nInstruments, INSTRUMENTINDEX(MAX_INSTRUMENTS - 1));
			}

			m_tracks.reserve(m_sndFile.GetNumInstruments() + 1);
			MidiTrack &tempoTrack = *(new MidiTrack(m_plugFactory, m_sndFile, &m_oldPlugins[0], nullptr, m_sndFile.m_songName.c_str(), nullptr));
			tempoTrack.WriteString(kText, m_sndFile.m_songMessage);
			tempoTrack.WriteString(kCopyright, m_sndFile.m_songArtist);
			m_tracks.push_back(&tempoTrack);

			PLUGINDEX nextPlug = 0;
			for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
			{
				m_sndFile.Instruments[i] = nullptr;
				if(!m_sndFile.IsInstrumentUsed(i) || (m_wasInstrumentMode && m_oldInstruments[i - 1] == nullptr) || nextPlug >= MAX_MIXPLUGINS)
				{
					continue;
				}

				// FIXME: Having > MAX_MIXPLUGINS used instruments won't work! So in MPTM, you can only use 250 out of 255 instruments...
				SNDMIXPLUGIN &mixPlugin = m_sndFile.m_MixPlugins[nextPlug++];

				ModInstrument *oldInstr = m_wasInstrumentMode ? m_oldInstruments[i - 1] : nullptr;
				MidiTrack &midiInstr = *(new MidiTrack(m_plugFactory, m_sndFile, &mixPlugin, &tempoTrack, m_wasInstrumentMode ? oldInstr->name : m_sndFile.GetSampleName(i), oldInstr));
				ModInstrument &instr = midiInstr;
				mixPlugin.pMixPlugin = &midiInstr;
				
				m_sndFile.Instruments[i] = &instr;
				m_tracks.push_back(&midiInstr);

				if(m_wasInstrumentMode) instr = *oldInstr;
				instr.nMixPlug = nextPlug;
				if((oldInstr != nullptr && oldInstr->nMixPlug == 0) || instr.nMidiChannel == MidiNoChannel)
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
					if(oldInstr != nullptr && oldInstr->nMidiChannel != MidiFirstChannel + 9) instr.nMidiProgram = 0;
					if(instrMap[i].nProgram != 128)
					{
						for(size_t n = 0; n < CountOf(instr.NoteMap); n++)
						{
							instr.NoteMap[n] = instrMap[i].nProgram + NOTE_MIN;
						}
					}
				}
			}

			mpt::IO::WriteRaw(m_file, "MThd", 4);
			mpt::IO::WriteIntBE<uint32>(m_file, 6);
			mpt::IO::WriteIntBE<uint16>(m_file, 1);	// Type 1 MIDI - multiple simultaneous tracks
			mpt::IO::WriteIntBE<uint16>(m_file, static_cast<uint16>(m_tracks.size()));	// Number of tracks
			mpt::IO::WriteIntBE<uint16>(m_file, MidiExport::ppq);
		}

		~Conversion()
		{
			for(size_t i = 0; i < m_tracks.size(); i++)
			{
				delete m_tracks[i];
			}
		}

		void Finalise()
		{
			for(size_t i = 0; i < m_tracks.size(); i++)
			{
				std::string data = m_tracks[i]->Finalise().str();
				if(!data.empty())
				{
					mpt::IO::WriteRaw(m_file, "MTrk", 4);
					mpt::IO::WriteIntBE<uint32>(m_file, data.size());
					mpt::IO::WriteRaw(m_file, &data[0], data.size());
				}
			}

			for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
			{
				m_sndFile.Instruments[i] = m_wasInstrumentMode ? m_oldInstruments[i - 1] : nullptr;
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
			
			// Be sure that instrument pointers to our faked instruments are gone.
			for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
			{
				m_sndFile.m_PlayState.Chn[i].Reset(ModChannel::resetTotal, m_sndFile, i);
			}
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
	TCHAR s[256];

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
	m_CbnChannel.SetItemData(m_CbnChannel.AddString(_T("Melodic (any)")), 0);
	m_CbnChannel.SetItemData(m_CbnChannel.AddString(_T("Percussions")), 10);
	for (UINT iCh=1; iCh<=16; iCh++) if (iCh != 10)
	{
		wsprintf(s, _T("Melodic %d"), iCh);
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
		for (ModCommand::NOTE i = 0; i < 61; i++)
		{
			ModCommand::NOTE note = i + 24;
			sprintf(s, "%u (%s): %s", note, m_sndFile.GetNoteName(note + NOTE_MIN).c_str(), szMidiPercussionNames[i]);
			m_CbnProgram.SetItemData(m_CbnProgram.AddString(s), note);
		}
		m_CbnProgram.SetItemData(m_CbnProgram.AddString(_T("Mapped")), 128);
	} else
	{
		for (int i = 0; i < 128; i++)
		{
			sprintf(s, "%03d: %s", i + 1, szMidiProgramNames[i]);
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
				timeRemaining = static_cast<uint32>(((currentTime - startTime) * (totalSamples - curSamples) / curSamples) / 1000u);
			}
			TCHAR s[128];
			_stprintf(s, _T("Rendering file... (%umn%02us, %umn%02us remaining)"), curTime / 60u, curTime % 60u, timeRemaining / 60u, timeRemaining % 60u);
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

#endif // NO_PLUGINS
