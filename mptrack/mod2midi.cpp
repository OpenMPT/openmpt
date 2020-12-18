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
#include "../common/mptStringBuffer.h"
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
	constexpr int32 ppq = 480;

	enum StringType : uint8
	{
		kText = 1,
		kCopyright = 2,
		kTrackName = 3,
		kInstrument = 4,
		kLyric = 5,
		kMarker = 6,
		kCue = 7,
	};

	class MidiTrack final : public IMidiPlugin
	{
		ModInstrument m_instr;
		const ModInstrument *const m_oldInstr;
		const CSoundFile &m_sndFile;
		MidiTrack *const m_tempoTrack;  // Pointer to tempo track, nullptr if this is the tempo track
		decltype(m_MidiCh) *m_lastMidiCh = nullptr;
		std::array<decltype(m_instr.midiPWD), 16> m_pitchWheelDepth = { 0 };

		std::vector<std::array<char, 4>> m_queuedEvents;

		std::ostringstream f;
		double m_tempo = 0.0;
		double m_ticks = 0.0;                           // MIDI ticks since previous event
		CSoundFile::samplecount_t m_samplePos = 0;      // Current sample position
		CSoundFile::samplecount_t m_prevEventTime = 0;  // Sample position of previous event
		uint32 m_sampleRate;
		uint32 m_oldSigNumerator = 0;
		int32 m_oldGlobalVol = -1;
		const bool m_overlappingInstruments;

		// Calculate how many MIDI ticks have passed since the last written event
		void UpdateTicksSinceLastEvent()
		{
			m_ticks += (m_samplePos - m_prevEventTime) * m_tempo * static_cast<double>(MidiExport::ppq) / (m_sampleRate * 60);
			m_prevEventTime = m_samplePos;
		}
		
		// Write delta tick count since last event
		void WriteTicks()
		{
			uint32 ticks = (m_ticks <= 0) ? 0 : mpt::saturate_round<uint32>(m_ticks);
			mpt::IO::WriteVarInt(f, ticks);
			m_ticks -= ticks;
		}

		// Update MIDI channel states in non-overlapping export mode so that all plugins have the same view
		void SynchronizeMidiChannelState()
		{
			if(m_tempoTrack != nullptr && !m_overlappingInstruments)
			{
				if(m_tempoTrack->m_lastMidiCh != nullptr && m_tempoTrack->m_lastMidiCh != &m_MidiCh)
					m_MidiCh = *m_tempoTrack->m_lastMidiCh;
				m_tempoTrack->m_lastMidiCh = &m_MidiCh;
			}
		}

		void SynchronizeMidiPitchWheelDepth(CHANNELINDEX trackerChn)
		{
			const auto midiCh = GetMidiChannel(trackerChn);
			if(!m_overlappingInstruments && m_tempoTrack && m_tempoTrack->m_pitchWheelDepth[midiCh] != m_instr.midiPWD)
				WritePitchWheelDepth(static_cast<MidiChannel>(midiCh + MidiFirstChannel));
		}

	public:

		operator ModInstrument& () { return m_instr; }

		MidiTrack(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct, MidiTrack *tempoTrack, const mpt::ustring &name, const ModInstrument *oldInstr, bool overlappingInstruments)
			: IMidiPlugin(factory, sndFile, mixStruct)
			, m_oldInstr(oldInstr)
			, m_sndFile(sndFile)
			, m_tempoTrack(tempoTrack)
			, m_sampleRate(sndFile.GetSampleRate())
			, m_overlappingInstruments(overlappingInstruments)
		{
			// Write instrument / song name
			WriteString(kTrackName, name);
			m_pMixStruct->pMixPlugin = this;
		}

		void WritePitchWheelDepth(MidiChannel midiChOverride = MidiNoChannel)
		{
			// Set up MIDI pitch wheel depth
			uint8 firstCh = 0, lastCh = 15;
			if(midiChOverride != MidiNoChannel)
				firstCh = lastCh = midiChOverride - MidiFirstChannel;
			else if(m_instr.nMidiChannel != MidiMappedChannel && m_instr.nMidiChannel != MidiNoChannel)
				firstCh = lastCh = m_instr.nMidiChannel - MidiFirstChannel;
			
			for(uint8 i = firstCh; i <= lastCh; i++)
			{
				const uint8 ch = 0xB0 | i;
				const uint8 msg[] = { ch, 0x64, 0x00, 0x00, ch, 0x65, 0x00, 0x00, ch, 0x06, static_cast<uint8>(std::abs(m_instr.midiPWD)) };
				WriteTicks();
				mpt::IO::WriteRaw(f, msg, sizeof(msg));
				if(m_tempoTrack)
					m_tempoTrack->m_pitchWheelDepth[i] = m_instr.midiPWD;
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

			if(curTempo > 0.0)
				m_tempo = curTempo;
			m_oldSigNumerator = timeSigNumerator;
			m_oldGlobalVol = m_sndFile.m_PlayState.m_nGlobalVolume;

			if(m_tempoTrack != nullptr)
				return;

			// This is the tempo track
			if(tempoChanged && curTempo > 0.0)
			{
				// Write MIDI tempo
				WriteTicks();

				uint32 mspq = mpt::saturate_round<uint32>(60000000.0 / curTempo);
				uint8 msg[6] = { 0xFF, 0x51, 0x03, static_cast<uint8>(mspq >> 16), static_cast<uint8>(mspq >> 8), static_cast<uint8>(mspq) };
				mpt::IO::WriteRaw(f, msg, 6);
			}

			if(sigChanged)
			{
				// Write MIDI time signature
				WriteTicks();

				uint8 msg[7] = { 0xFF, 0x58, 0x04, static_cast<uint8>(timeSigNumerator), 2, 24, 8 };
				mpt::IO::WriteRaw(f, msg, 7);
			}

			if(volChanged)
			{
				// Write MIDI master volume
				WriteTicks();

				int32 midiVol = Util::muldiv(m_oldGlobalVol, 0x3FFF, MAX_GLOBAL_VOLUME);
				uint8 msg[9] = { 0xF0, 0x07, 0x7F, 0x7F, 0x04, 0x01, static_cast<uint8>(midiVol & 0x7F), static_cast<uint8>((midiVol >> 7) & 0x7F), 0xF7 };
				mpt::IO::WriteRaw(f, msg, 9);
			}
		}

		void Process(float *, float *, uint32 numFrames) override
		{
			UpdateGlobals();
			if(m_tempoTrack != nullptr)
				m_tempoTrack->UpdateGlobals();

			for(const auto &midiData : m_queuedEvents)
			{
				WriteTicks();
				mpt::IO::WriteRaw(f, midiData.data(), MIDIEvents::GetEventLength(midiData[0]));
			}
			m_queuedEvents.clear();

			m_samplePos += numFrames;
			if (m_tempoTrack != nullptr)
			{
				m_tempoTrack->m_samplePos = std::max(m_tempoTrack->m_samplePos, m_samplePos);
				m_tempoTrack->UpdateTicksSinceLastEvent();
			}
			UpdateTicksSinceLastEvent();
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

		void WriteString(StringType strType, const mpt::ustring &ustr)
		{
			std::string str = mpt::ToCharset(mpt::Charset::Locale, ustr);
			if(!str.empty())
			{
				uint8 msg[3] = { 0x00, 0xFF, strType };
				mpt::IO::WriteRaw(f, msg, 3);
				mpt::IO::WriteVarInt(f, str.length());
				mpt::IO::WriteRaw(f, str.data(), str.length());
			}
		}

		void Release() override { }
		int32 GetUID() const override { return 0; }
		int32 GetVersion() const override { return 0; }
		void Idle() override { }
		uint32 GetLatency() const override { return 0; }

		int32 GetNumPrograms() const override { return 0; }
		int32 GetCurrentProgram() override { return 0; }
		void SetCurrentProgram(int32) override { }

		PlugParamIndex GetNumParameters() const override { return 0; }
		PlugParamValue GetParameter(PlugParamIndex) override { return 0; }
		void SetParameter(PlugParamIndex, PlugParamValue) override { }

		float RenderSilence(uint32) override { return 0.0f; }

		bool MidiSend(uint32 midiCode) override
		{
			std::array<char, 4> midiData;
			memcpy(midiData.data(), &midiCode, 4);

			// Note-On events go last to prevent early note-off in a situation like this:
			// ... ..|C-5 01
			// C-5 01|=== ..
			if(MIDIEvents::GetTypeFromEvent(midiCode) == MIDIEvents::evNoteOn)
			{
				m_queuedEvents.push_back(midiData);
				return true;
			}
			WriteTicks();
			mpt::IO::WriteRaw(f, midiData.data(), MIDIEvents::GetEventLength(midiData[0]));
			return true;
		}

		bool MidiSysexSend(mpt::const_byte_span sysex) override
		{
			if(sysex.size() > 1)
			{
				WriteTicks();
				mpt::IO::WriteIntBE<uint8>(f, 0xF0);
				mpt::IO::WriteVarInt(f, mpt::saturate_cast<uint32>(sysex.size() - 1));
				mpt::IO::WriteRaw(f, sysex.data() + 1, sysex.size() - 1);
			}
			return true;
		}

		uint8 GetMidiChannel(CHANNELINDEX trackChannel) const override
		{
			if(m_instr.nMidiChannel == MidiMappedChannel && trackChannel < std::size(m_sndFile.m_PlayState.Chn))
			{
				// For mapped channels, distribute tracker channels evenly over MIDI channels, but avoid channel 10 (drums)
				uint8 midiCh = trackChannel % 15u;
				if(midiCh >= 9)
					midiCh++;
				return midiCh;
			}
			return IMidiPlugin::GetMidiChannel(trackChannel);
		}

		void MidiCommand(const ModInstrument &instr, uint16 note, uint16 vol, CHANNELINDEX trackChannel) override
		{
			if(note == NOTE_NOTECUT && (m_oldInstr == nullptr || !(m_oldInstr->nMixPlug != 0 && m_oldInstr->HasValidMIDIChannel())))
			{
				// The default implementation does things with Note Cut that we don't want here: it cuts all notes.
				note = NOTE_KEYOFF;
			}
			SynchronizeMidiChannelState();
			IMidiPlugin::MidiCommand(instr, note, vol, trackChannel);
		}

		void MidiPitchBend(int32 increment, int8 pwd, CHANNELINDEX trackerChn) override
		{
			SynchronizeMidiChannelState();
			SynchronizeMidiPitchWheelDepth(trackerChn);
			IMidiPlugin::MidiPitchBend(increment, pwd, trackerChn);
		}

		void MidiVibrato(int32 depth, int8 pwd, CHANNELINDEX trackerChn) override
		{
			SynchronizeMidiChannelState();
			SynchronizeMidiPitchWheelDepth(trackerChn);
			IMidiPlugin::MidiVibrato(depth, pwd, trackerChn);
		}

		bool IsNotePlaying(uint8 note, CHANNELINDEX trackerChn) override
		{
			SynchronizeMidiChannelState();
			return IMidiPlugin::IsNotePlaying(note, trackerChn);
		}

		void HardAllNotesOff() override
		{
			for(uint8 mc = 0; mc < m_MidiCh.size(); mc++)
			{
				PlugInstrChannel &channel = m_MidiCh[mc];
				for(size_t i = 0; i < std::size(channel.noteOnMap); i++)
				{
					for(auto &c : channel.noteOnMap[i])
					{
						while(c != 0)
						{
							MidiSend(MIDIEvents::NoteOff(mc, static_cast<uint8>(i), 0));
							c--;
						}
					}
				}
			}
		}

		void Resume() override { }
		void Suspend() override { }
		void PositionChanged() override { }

		bool IsInstrument() const override { return true; }
		bool CanRecieveMidiEvents() override { return true; }
		bool ShouldProcessSilence() override { return true; }
#ifdef MODPLUG_TRACKER
		CString GetDefaultEffectName() override { return {}; }
		CString GetParamName(PlugParamIndex) override { return {}; }
		CString GetParamLabel(PlugParamIndex) override { return {}; }
		CString GetParamDisplay(PlugParamIndex) override { return {}; }
		CString GetCurrentProgramName() override { return {}; }
		void SetCurrentProgramName(const CString &) override { }
		CString GetProgramName(int32) override { return {}; }
		bool HasEditor() const override { return false; }
#endif // MODPLUG_TRACKER
		int GetNumInputChannels() const override { return 0; }
		int GetNumOutputChannels() const override { return 0; }
	};


	class Conversion
	{
		std::vector<ModInstrument *> m_oldInstruments;
		std::vector<MidiTrack *> m_tracks;
		std::vector<SNDMIXPLUGIN> m_oldPlugins;
		SNDMIXPLUGIN tempoTrackPlugin;
		VSTPluginLib m_plugFactory;
		CSoundFile &m_sndFile;
		mpt::ofstream &m_file;
		const bool m_wasInstrumentMode;

	public:
		Conversion(CSoundFile &sndFile, const InstrMap &instrMap, mpt::ofstream &file, bool overlappingInstruments)
			: m_oldInstruments(sndFile.GetNumInstruments())
			, m_plugFactory(nullptr, true, {}, {}, {})
			, m_sndFile(sndFile)
			, m_file(file)
			, m_wasInstrumentMode(sndFile.GetNumInstruments() > 0)
		{
			m_oldPlugins.assign(std::begin(m_sndFile.m_MixPlugins), std::end(m_sndFile.m_MixPlugins));
			std::fill(std::begin(m_sndFile.m_MixPlugins), std::end(m_sndFile.m_MixPlugins), SNDMIXPLUGIN());
			for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
			{
				m_oldInstruments[i - 1] = m_sndFile.Instruments[i];
			}
			if(!m_wasInstrumentMode)
			{
				m_sndFile.m_nInstruments = std::min<INSTRUMENTINDEX>(m_sndFile.m_nSamples, MAX_INSTRUMENTS - 1u);
			}

			m_tracks.reserve(m_sndFile.GetNumInstruments() + 1);
			MidiTrack &tempoTrack = *(new MidiTrack(m_plugFactory, m_sndFile, &tempoTrackPlugin, nullptr, mpt::ToUnicode(m_sndFile.GetCharsetInternal(), m_sndFile.m_songName), nullptr, overlappingInstruments));
			tempoTrack.WriteString(kText, mpt::ToUnicode(m_sndFile.GetCharsetInternal(), m_sndFile.m_songMessage));
			tempoTrack.WriteString(kCopyright, m_sndFile.m_songArtist);
			m_tracks.push_back(&tempoTrack);

			PLUGINDEX nextPlug = 0;
			for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
			{
				m_sndFile.Instruments[i] = nullptr;
				if(!m_sndFile.GetpModDoc()->IsInstrumentUsed(i) || (m_wasInstrumentMode && m_oldInstruments[i - 1] == nullptr) || nextPlug >= MAX_MIXPLUGINS)
					continue;

				// FIXME: Having > MAX_MIXPLUGINS used instruments won't work! So in MPTM, you can only use 250 out of 255 instruments...
				SNDMIXPLUGIN &mixPlugin = m_sndFile.m_MixPlugins[nextPlug++];

				ModInstrument *oldInstr = m_wasInstrumentMode ? m_oldInstruments[i - 1] : nullptr;
				MidiTrack &midiInstr = *(new MidiTrack(m_plugFactory, m_sndFile, &mixPlugin, &tempoTrack, m_wasInstrumentMode ? mpt::ToUnicode(m_sndFile.GetCharsetInternal(), oldInstr->name) : mpt::ToUnicode(m_sndFile.GetCharsetInternal(), m_sndFile.GetSampleName(i)), oldInstr, overlappingInstruments));
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

				instr.nMidiChannel = instrMap[i].channel;
				if(instrMap[i].channel != MidiFirstChannel + 9)
				{
					// Melodic instrument
					instr.nMidiProgram = instrMap[i].program;
				} else
				{
					// Drums
					if(oldInstr != nullptr && oldInstr->nMidiChannel != MidiFirstChannel + 9)
						instr.nMidiProgram = 0;
					if(instrMap[i].program != 128)
					{
						for(auto &key : instr.NoteMap)
						{
							key = instrMap[i].program + NOTE_MIN;
						}
					}
				}
				midiInstr.WritePitchWheelDepth();
			}

			mpt::IO::WriteRaw(m_file, "MThd", 4);
			mpt::IO::WriteIntBE<uint32>(m_file, 6);
			mpt::IO::WriteIntBE<uint16>(m_file, 1);	// Type 1 MIDI - multiple simultaneous tracks
			mpt::IO::WriteIntBE<uint16>(m_file, static_cast<uint16>(m_tracks.size()));	// Number of tracks
			mpt::IO::WriteIntBE<uint16>(m_file, MidiExport::ppq);
		}

		void Finalise()
		{
			for(auto track : m_tracks)
			{
				std::string data = track->Finalise().str();
				if(!data.empty())
				{
					const uint32 len = mpt::saturate_cast<uint32>(data.size());
					mpt::IO::WriteRaw(m_file, "MTrk", 4);
					mpt::IO::WriteIntBE<uint32>(m_file, len);
					mpt::IO::WriteRaw(m_file, data.data(), len);
				}
			}
		}

		~Conversion()
		{
			for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
			{
				m_sndFile.Instruments[i] = m_wasInstrumentMode ? m_oldInstruments[i - 1] : nullptr;
			}

			if(!m_wasInstrumentMode)
			{
				m_sndFile.m_nInstruments = 0;
			}

			for(auto &plug : m_sndFile.m_MixPlugins)
			{
				plug.Destroy();
			}
			for(auto &track : m_tracks)
			{
				delete track;	// Resets m_MixPlugins[i].pMixPlugin, so do it before copying back the old structs
			}
			std::move(m_oldPlugins.cbegin(), m_oldPlugins.cend(), std::begin(m_sndFile.m_MixPlugins));

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
		void DataCallback(MixSampleInt *, std::size_t, std::size_t) override { }
		void DataCallback(MixSampleFloat *, std::size_t, std::size_t) override { }
	};
}


////////////////////////////////////////////////////////////////////////////////////
//
// CModToMidi dialog implementation
//

bool CModToMidi::s_overlappingInstruments = false;

BEGIN_MESSAGE_MAP(CModToMidi, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO1,	&CModToMidi::UpdateDialog)
	ON_CBN_SELCHANGE(IDC_COMBO2,	&CModToMidi::OnChannelChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,	&CModToMidi::OnProgramChanged)
	ON_COMMAND(IDC_CHECK1,			&CModToMidi::OnOverlapChanged)
	ON_WM_VSCROLL()
END_MESSAGE_MAP()


void CModToMidi::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1,	m_CbnInstrument);
	DDX_Control(pDX, IDC_COMBO2,	m_CbnChannel);
	DDX_Control(pDX, IDC_COMBO3,	m_CbnProgram);
	DDX_Control(pDX, IDC_SPIN1,		m_SpinInstrument);
}


CModToMidi::CModToMidi(CSoundFile &sndFile, CWnd *pWndParent)
	: CDialog(IDD_MOD2MIDI, pWndParent)
	, m_sndFile(sndFile)
	, m_instrMap((sndFile.GetNumInstruments() ? sndFile.GetNumInstruments() : sndFile.GetNumSamples()) + 1)
{
	for (INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
	{
		ModInstrument *pIns = m_sndFile.Instruments[i];
		if(pIns != nullptr)
		{
			m_instrMap[i].channel = pIns->nMidiChannel;
			if(m_instrMap[i].channel == MidiFirstChannel + 9)
			{
				if ((pIns->nMidiProgram > 20) && (pIns->nMidiProgram < 120))
					m_instrMap[i].program = pIns->nMidiProgram;
				else
					m_instrMap[i].program = (pIns->NoteMap[60] - NOTE_MIN) & 0x7F;
			} else
			{
				if(!pIns->HasValidMIDIChannel())
					m_instrMap[i].channel = MidiMappedChannel;
				m_instrMap[i].program = pIns->nMidiProgram;
			}
		}
	}
}


BOOL CModToMidi::OnInitDialog()
{
	CString s;

	CDialog::OnInitDialog();

	// Fill instruments box
	m_SpinInstrument.SetRange(-1, 1);
	m_SpinInstrument.SetPos(0);
	m_currentInstr = 1;
	m_CbnInstrument.SetRedraw(FALSE);
	if(m_sndFile.GetNumInstruments())
	{
		for(INSTRUMENTINDEX nIns = 1; nIns <= m_sndFile.GetNumInstruments(); nIns++)
		{
			ModInstrument *pIns = m_sndFile.Instruments[nIns];
			if(pIns && m_sndFile.GetpModDoc()->IsInstrumentUsed(nIns, false))
			{
				const CString name = m_sndFile.GetpModDoc()->GetPatternViewInstrumentName(nIns);
				m_CbnInstrument.SetItemData(m_CbnInstrument.AddString(name), nIns);
			}
		}
	} else
	{
		for(SAMPLEINDEX nSmp = 1; nSmp <= m_sndFile.GetNumSamples(); nSmp++)
		{
			if(m_sndFile.GetpModDoc()->IsSampleUsed(nSmp, false))
			{
				s.Format(_T("%02d: "), nSmp);
				s += mpt::ToCString(m_sndFile.GetCharsetInternal(), m_sndFile.m_szNames[nSmp]);
				m_CbnInstrument.SetItemData(m_CbnInstrument.AddString(s), nSmp);
			}
		}
	}
	m_CbnInstrument.SetRedraw(TRUE);
	m_CbnInstrument.SetCurSel(0);

	// Fill channels box
	m_CbnChannel.SetRedraw(FALSE);
	m_CbnChannel.SetItemData(m_CbnChannel.AddString(_T("Don't Export")), MidiNoChannel);
	m_CbnChannel.SetItemData(m_CbnChannel.AddString(_T("Melodic (any)")), MidiMappedChannel);
	m_CbnChannel.SetItemData(m_CbnChannel.AddString(_T("Percussions")), MidiFirstChannel + 9);
	for(uint32 chn = 1; chn <= 16; chn++)
	{
		if(chn == 10)
			continue;
		s.Format(_T("Melodic %u"), chn);
		m_CbnChannel.SetItemData(m_CbnChannel.AddString(s), MidiFirstChannel - 1 + chn);
	}
	m_CbnChannel.SetRedraw(TRUE);
	m_CbnChannel.SetCurSel(1);

	m_currentInstr = 1;
	m_percussion = true;
	FillProgramBox(false);
	m_CbnProgram.SetCurSel(0);
	UpdateDialog();
	CheckDlgButton(IDC_CHECK1, s_overlappingInstruments ? BST_CHECKED : BST_UNCHECKED);
	return TRUE;
}


void CModToMidi::FillProgramBox(bool percussion)
{
	if(m_percussion == percussion)
		return;
	m_CbnProgram.SetRedraw(FALSE);
	m_CbnProgram.ResetContent();
	if(percussion)
	{
		for(ModCommand::NOTE i = 0; i < 61; i++)
		{
			ModCommand::NOTE note = i + 24;
			auto s = MPT_CFORMAT("{} ({}): {}")(
				note,
				mpt::ToCString(m_sndFile.GetNoteName(note + NOTE_MIN)),
				mpt::ToCString(mpt::Charset::ASCII, szMidiPercussionNames[i]));
			m_CbnProgram.SetItemData(m_CbnProgram.AddString(s), note);
		}
		m_CbnProgram.SetItemData(m_CbnProgram.AddString(_T("Mapped")), 128);
	} else
	{
		m_CbnProgram.SetItemData(m_CbnProgram.AddString(_T("No Program Change")), 0);
		for(int i = 1; i <= 128; i++)
		{
			auto s = MPT_CFORMAT("{}: {}")(
				mpt::cfmt::dec0<3>(i),
				mpt::ToCString(mpt::Charset::ASCII, szMidiProgramNames[i - 1]));
			m_CbnProgram.SetItemData(m_CbnProgram.AddString(s), i);
		}
	}
	m_CbnProgram.SetRedraw(TRUE);
	m_CbnProgram.Invalidate(FALSE);
	m_percussion = percussion;
}


void CModToMidi::UpdateDialog()
{
	m_currentInstr = static_cast<UINT>(m_CbnInstrument.GetItemData(m_CbnInstrument.GetCurSel()));
	const bool validInstr = (m_currentInstr > 0 && m_currentInstr < m_instrMap.size());

	m_CbnProgram.EnableWindow(validInstr && m_instrMap[m_currentInstr].channel != MidiNoChannel);

	if(!validInstr)
		return;

	uint8 nMidiCh = m_instrMap[m_currentInstr].channel;
	int sel;
	switch(nMidiCh)
	{
	case MidiNoChannel:
		sel = 0;
		break;
	case MidiMappedChannel:
		sel = 1;
		break;
	case MidiFirstChannel + 9:
		sel = 2;
		break;
	default:
		sel = nMidiCh - MidiFirstChannel + 2;
		if(nMidiCh < MidiFirstChannel + 9)
			sel++;
	}
	if(!m_percussion && (nMidiCh == MidiFirstChannel + 9))
	{
		FillProgramBox(true);
	} else if(m_percussion && (nMidiCh != MidiFirstChannel + 9))
	{
		FillProgramBox(false);
	}
	m_CbnChannel.SetCurSel(sel);
	UINT nMidiProgram = m_instrMap[m_currentInstr].program;
	if(m_percussion)
	{
		nMidiProgram -= 24;
		if(nMidiProgram > 60)
			nMidiProgram = 24;
	} else
	{
		if(nMidiProgram > 127)
			nMidiProgram = 0;
	}
	m_CbnProgram.SetCurSel(nMidiProgram);
}


void CModToMidi::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
	int pos = m_SpinInstrument.GetPos32();
	if(pos)
	{
		m_SpinInstrument.SetPos(0);
		int numIns = m_CbnInstrument.GetCount();
		int ins = m_CbnInstrument.GetCurSel() + pos;
		if(ins < 0)
			ins = numIns - 1;
		if(ins >= numIns)
			ins = 0;
		m_CbnInstrument.SetCurSel(ins);
		UpdateDialog();
	}
}


void CModToMidi::OnChannelChanged()
{
	uint8 midiCh = static_cast<uint8>(m_CbnChannel.GetItemData(m_CbnChannel.GetCurSel()));
	if(m_currentInstr >= m_instrMap.size())
		return;
	const auto oldCh = m_instrMap[m_currentInstr].channel;
	m_instrMap[m_currentInstr].channel = midiCh;
	if(midiCh == MidiNoChannel
	   || oldCh == MidiNoChannel
	   || (!m_percussion && midiCh == MidiFirstChannel + 9)
	   || (m_percussion && midiCh != MidiFirstChannel + 9))
	{
		UpdateDialog();
	}
}


void CModToMidi::OnProgramChanged()
{
	DWORD_PTR nProgram = m_CbnProgram.GetItemData(m_CbnProgram.GetCurSel());
	if (nProgram == CB_ERR) return;
	if ((m_currentInstr > 0) && (m_currentInstr < MAX_SAMPLES))
	{
		m_instrMap[m_currentInstr].program = static_cast<uint8>(nProgram);
	}
}


void CModToMidi::OnOverlapChanged()
{
	s_overlappingInstruments = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;
}


void CModToMidi::OnOK()
{
	for(size_t i = 1; i < m_instrMap.size(); i++)
	{
		if(m_instrMap[i].channel != MidiNoChannel)
		{
			CDialog::OnOK();
			return;
		}
	}

	auto choice = Reporting::Confirm(_T("No instruments have been selected for export. Would you still like to export the file?"), true, true);
	if(choice == cnfYes)
		CDialog::OnOK();
	else if(choice == cnfNo)
		CDialog::OnCancel();
}


void CDoMidiConvert::Run()
{
	CMainFrame::GetMainFrame()->PauseMod(m_sndFile.GetpModDoc());
	auto conv = std::make_unique<MidiExport::Conversion>(m_sndFile, m_instrMap, m_file, CModToMidi::s_overlappingInstruments);

	double duration = m_sndFile.GetLength(eNoAdjust).front().duration;
	uint64 totalSamples = mpt::saturate_round<uint64>(duration * m_sndFile.m_MixerSettings.gdwMixingFreq);
	SetRange(0, totalSamples);
	auto startTime = timeGetTime(), prevTime = startTime;

	m_sndFile.SetCurrentOrder(0);
	m_sndFile.GetLength(eAdjust, GetLengthTarget(0, 0));
	m_sndFile.m_SongFlags.reset(SONG_PATTERNLOOP);
	int oldRepCount = m_sndFile.GetRepeatCount();
	m_sndFile.SetRepeatCount(0);
	m_sndFile.m_bIsRendering = true;

	EnableTaskbarProgress();

	MidiExport::DummyAudioTarget target;
	UINT ok = IDOK;
	const auto fmt = MPT_TFORMAT("Rendering file... ({}mn{}s, {}mn{}s remaining)");
	while(m_sndFile.Read(MIXBUFFERSIZE, target) > 0)
	{
		auto currentTime = timeGetTime();
		if(currentTime - prevTime >= 16)
		{
			prevTime = currentTime;
			uint64 curSamples = m_sndFile.GetTotalSampleCount();
			uint32 curTime = static_cast<uint32>(curSamples / m_sndFile.m_MixerSettings.gdwMixingFreq);
			uint32 timeRemaining = 0;
			if(curSamples > 0 && curSamples < totalSamples)
			{
				timeRemaining = static_cast<uint32>(((currentTime - startTime) * (totalSamples - curSamples) / curSamples) / 1000u);
			}
			SetText(fmt(curTime / 60u, mpt::tfmt::dec0<2>(curTime % 60u), timeRemaining / 60u, mpt::tfmt::dec0<2>(timeRemaining % 60u)).c_str());
			SetProgress(curSamples);
			ProcessMessages();

			if(m_abort)
			{
				ok = IDCANCEL;
				break;
			}
		}
	}

	conv->Finalise();

	m_sndFile.m_bIsRendering = false;
	m_sndFile.SetRepeatCount(oldRepCount);

	EndDialog(ok);
}

OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
