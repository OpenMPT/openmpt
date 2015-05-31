/*
 * UpdateModule.cpp
 * ----------------
 * Purpose: CSoundFile functions for correcting modules made with previous versions of OpenMPT.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#include "../common/StringFixer.h"
#include "../common/version.h"


OPENMPT_NAMESPACE_BEGIN

// For old files made with MPT that don't have m_ModFlags set yet, set the flags appropriately.
void CSoundFile::UpgradeModFlags()
//--------------------------------
{
	if(m_dwLastSavedWithVersion && m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 17, 02, 50))
	{
		SetModFlag(MSF_COMPATIBLE_PLAY, false);
		SetModFlag(MSF_MIDICC_BUGEMULATION, false);

		if(m_dwLastSavedWithVersion >= MAKE_VERSION_NUMERIC(1, 17, 00, 00))
		{
			// If there are any plugins that can receive volume commands, enable volume bug emulation.
			for(INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++) if(Instruments[i])
			{
				if(Instruments[i]->nMixPlug && Instruments[i]->HasValidMIDIChannel())
				{
					SetModFlag(MSF_MIDICC_BUGEMULATION, true);
					break;
				}
			}
		}

		// If there are any instruments with random variation, enable the old random variation behaviour.
		for(INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++)
		{
			if(Instruments[i] && (Instruments[i]->nVolSwing | Instruments[i]->nPanSwing | Instruments[i]->nCutSwing | Instruments[i]->nResSwing))
			{
				SetModFlag(MSF_OLDVOLSWING, true);
				break;
			}
		}
	}
}


struct UpgradePatternData
//=======================
{
	UpgradePatternData(CSoundFile &sf) : sndFile(sf), chn(0) { }

	void operator() (ModCommand &m)
	{
		const CHANNELINDEX curChn = chn;
		chn++;
		if(chn >= sndFile.GetNumChannels())
		{
			chn = 0;
		}

		if(m.IsPcNote())
		{
			return;
		}

		if(sndFile.GetType() == MOD_TYPE_S3M)
		{
			// Out-of-range global volume commands should be ignored in S3M. Fixed in OpenMPT 1.19 (r831).
			// So for tracks made with older versions of OpenMPT, we limit invalid global volume commands.
			if(sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 19, 00, 00) && m.command == CMD_GLOBALVOLUME)
			{
				LimitMax(m.param, ModCommand::PARAM(64));
			}
		}

		else if((sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)))
		{
			if(sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 17, 03, 02) ||
				(!sndFile.IsCompatibleMode(TRK_IMPULSETRACKER) && sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 20, 00, 00)))
			{
				if(m.command == CMD_GLOBALVOLUME)
				{
					// Out-of-range global volume commands should be ignored in IT.
					// OpenMPT 1.17.03.02 fixed this in compatible mode, OpenMPT 1.20 fixes it in normal mode as well.
					// So for tracks made with older versions than OpenMPT 1.17.03.02 or tracks made with 1.17.03.02 <= version < 1.20, we limit invalid global volume commands.
					LimitMax(m.param, ModCommand::PARAM(128));
				}

				// SC0 and SD0 should be interpreted as SC1 and SD1 in IT files.
				// OpenMPT 1.17.03.02 fixed this in compatible mode, OpenMPT 1.20 fixes it in normal mode as well.
				else if(m.command == CMD_S3MCMDEX)
				{
					if(m.param == 0xC0)
					{
						m.command = CMD_NONE;
						m.note = NOTE_NOTECUT;
					} else if(m.param == 0xD0)
					{
						m.command = CMD_NONE;
					}
				}
			}

			// In the IT format, slide commands with both nibbles set should be ignored.
			// For note volume slides, OpenMPT 1.18 fixes this in compatible mode, OpenMPT 1.20 fixes this in normal mode as well.
			const bool noteVolSlide =
				(sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 18, 00, 00) ||
				(!sndFile.IsCompatibleMode(TRK_IMPULSETRACKER) && sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 20, 00, 00)))
				&&
				(m.command == CMD_VOLUMESLIDE || m.command == CMD_VIBRATOVOL || m.command == CMD_TONEPORTAVOL || m.command == CMD_PANNINGSLIDE);

			// OpenMPT 1.20 also fixes this for global volume and channel volume slides.
			const bool chanVolSlide =
				(sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 20, 00, 00))
				&&
				(m.command == CMD_GLOBALVOLSLIDE || m.command == CMD_CHANNELVOLSLIDE);

			if(noteVolSlide || chanVolSlide)
			{
				if((m.param & 0x0F) != 0x00 && (m.param & 0x0F) != 0x0F && (m.param & 0xF0) != 0x00 && (m.param & 0xF0) != 0xF0)
				{
					m.param &= 0x0F;
				}
			}

			if(sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 22, 01, 04)
				&& sndFile.m_dwLastSavedWithVersion != MAKE_VERSION_NUMERIC(1, 22, 00, 00))	// Ignore compatibility export
			{
				// OpenMPT 1.22.01.04 fixes illegal (out of range) instrument numbers; they should do nothing. In previous versions, they stopped the playing sample.
				if(sndFile.GetNumInstruments() && m.instr > sndFile.GetNumInstruments() && !sndFile.IsCompatibleMode(TRK_IMPULSETRACKER))
				{
					m.volcmd = VOLCMD_VOLUME;
					m.vol = 0;
				}
			}
		}

		else if(sndFile.GetType() == MOD_TYPE_XM)
		{
			// Something made be believe that out-of-range global volume commands are ignored in XM
			// just like they are ignored in IT, but apparently they are not. Aaaaaargh!
			if(((sndFile.m_dwLastSavedWithVersion >= MAKE_VERSION_NUMERIC(1, 17, 03, 02) && sndFile.IsCompatibleMode(TRK_FASTTRACKER2)) || (sndFile.m_dwLastSavedWithVersion >= MAKE_VERSION_NUMERIC(1, 20, 00, 00)))
				&& sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 24, 02, 02)
				&& m.command == CMD_GLOBALVOLUME
				&& m.param > 64)
			{
				m.command = CMD_NONE;
			}

			if(sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 19, 00, 00)
				|| (!sndFile.IsCompatibleMode(TRK_FASTTRACKER2) && sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 20, 00, 00)))
			{
				if(m.command == CMD_OFFSET && m.volcmd == VOLCMD_TONEPORTAMENTO)
				{
					// If there are both a portamento and an offset effect, the portamento should be preferred in XM files.
					// OpenMPT 1.19 fixed this in compatible mode, OpenMPT 1.20 fixes it in normal mode as well.
					m.command = CMD_NONE;
				}
			}

			if(sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 20, 01, 10)
				&& m.volcmd == VOLCMD_TONEPORTAMENTO && m.command == CMD_TONEPORTAMENTO
				&& (m.vol != 0 || sndFile.IsCompatibleMode(TRK_FASTTRACKER2)) && m.param != 0)
			{
				// Mx and 3xx on the same row does weird things in FT2: 3xx is completely ignored and the Mx parameter is doubled. Fixed in revision 1312 / OpenMPT 1.20.01.10
				// Previously the values were just added up, so let's fix this!
				m.volcmd = VOLCMD_NONE;
				const uint16 param = static_cast<uint16>(m.param) + static_cast<uint16>(m.vol << 4);
				m.param = mpt::saturate_cast<ModCommand::PARAM>(param);
			}

			if(sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 22, 07, 09)
				&& m.command == CMD_SPEED && m.param == 0)
			{
				// OpenMPT can emulate FT2's F00 behaviour now.
				m.command = CMD_NONE;
			}
		}

		if(sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 20, 00, 00))
		{
			// Pattern Delay fixes

			const bool fixS6x = (m.command == CMD_S3MCMDEX && (m.param & 0xF0) == 0x60);
			// We also fix X6x commands in hacked XM files, since they are treated identically to the S6x command in IT/S3M files.
			// We don't treat them in files made with OpenMPT 1.18+ that have compatible play enabled, though, since they are ignored there anyway.
			const bool fixX6x = (m.command == CMD_XFINEPORTAUPDOWN && (m.param & 0xF0) == 0x60
				&& (!sndFile.IsCompatibleMode(TRK_FASTTRACKER2) || sndFile.m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 18, 00, 00)));

			if(fixS6x || fixX6x)
			{
				// OpenMPT 1.20 fixes multiple fine pattern delays on the same row. Previously, only the last command was considered,
				// but all commands should be added up. Since Scream Tracker 3 itself doesn't support S6x, we also use Impulse Tracker's behaviour here,
				// since we can assume that most S3Ms that make use of S6x were composed with Impulse Tracker.
				for(ModCommand *fixCmd = (&m) - curChn; fixCmd < &m; fixCmd++)
				{
					if((fixCmd->command == CMD_S3MCMDEX || fixCmd->command == CMD_XFINEPORTAUPDOWN) && (fixCmd->param & 0xF0) == 0x60)
					{
						fixCmd->command = CMD_NONE;
					}
				}
			}

			if(m.command == CMD_S3MCMDEX && (m.param & 0xF0) == 0xE0)
			{
				// OpenMPT 1.20 fixes multiple pattern delays on the same row. Previously, only the *last* command was considered,
				// but Scream Tracker 3 and Impulse Tracker only consider the *first* command.
				for(ModCommand *fixCmd = (&m) - curChn; fixCmd < &m; fixCmd++)
				{
					if(fixCmd->command == CMD_S3MCMDEX && (fixCmd->param & 0xF0) == 0xE0)
					{
						fixCmd->command = CMD_NONE;
					}
				}
			}
		}

		// Volume column offset in IT/XM is bad, mkay?
		if(sndFile.GetType() != MOD_TYPE_MPT && m.volcmd == VOLCMD_OFFSET && m.command == CMD_NONE)
		{
			m.command = CMD_OFFSET;
			m.param = m.vol << 3;
			m.volcmd = VOLCMD_NONE;
		}

	}

	CSoundFile &sndFile;
	CHANNELINDEX chn;
};


void CSoundFile::UpgradeModule()
//------------------------------
{
	if(m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 20, 00, 00))
	{
		for(INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++) if(Instruments[i] != nullptr)
		{
			// Previously, volume swing values ranged from 0 to 64. They should reach from 0 to 100 instead.
			Instruments[i]->nVolSwing = MIN(Instruments[i]->nVolSwing * 100 / 64, 100);

			if(!IsCompatibleMode(TRK_IMPULSETRACKER) || m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 18, 00, 00))
			{
				// Previously, Pitch/Pan Separation was only half depth.
				// This was corrected in compatible mode in OpenMPT 1.18, and in OpenMPT 1.20 it is corrected in normal mode as well.
				Instruments[i]->nPPS = (Instruments[i]->nPPS + (Instruments[i]->nPPS >= 0 ? 1 : -1)) / 2;
			}

			if(!IsCompatibleMode(TRK_IMPULSETRACKER) || m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 17, 03, 02))
			{
				// IT compatibility 24. Short envelope loops
				// Previously, the pitch / filter envelope loop handling was broken, the loop was shortened by a tick (like in XM).
				// This was corrected in compatible mode in OpenMPT 1.17.03.02, and in OpenMPT 1.20 it is corrected in normal mode as well.
				Instruments[i]->GetEnvelope(ENV_PITCH).Convert(MOD_TYPE_XM, GetType());
			}
		}

		if((GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) && (m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 17, 03, 02) || !IsCompatibleMode(TRK_IMPULSETRACKER)))
		{
			// In the IT format, a sweep value of 0 shouldn't apply vibrato at all. Previously, a value of 0 was treated as "no sweep".
			// In OpenMPT 1.17.03.02, this was corrected in compatible mode, in OpenMPT 1.20 it is corrected in normal mode as well,
			// so we have to fix the setting while loading.
			for(SAMPLEINDEX i = 1; i <= GetNumSamples(); i++)
			{
				if(Samples[i].nVibSweep == 0 && (Samples[i].nVibDepth | Samples[i].nVibRate))
				{
					Samples[i].nVibSweep = 255;
				}
			}
		}

		// Fix old nasty broken (non-standard) MIDI configs in files.
		m_MidiCfg.UpgradeMacros();
	}

	if(m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 20, 02, 10)
		&& m_dwLastSavedWithVersion != MAKE_VERSION_NUMERIC(1, 20, 00, 00)
		&& (GetType() & (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT)))
	{
		bool instrPlugs = false;
		// Old pitch wheel commands were closest to sample pitch bend commands if the PWD is 13.
		for(INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++)
		{
			if(Instruments[i] != nullptr && Instruments[i]->nMidiChannel != MidiNoChannel)
			{
				Instruments[i]->midiPWD = 13;
				instrPlugs = true;
			}
		}
		if(instrPlugs)
		{
			SetModFlag(MSF_OLD_MIDI_PITCHBENDS, true);
		}
	}

	if(m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 22, 03, 12)
		&& m_dwLastSavedWithVersion != MAKE_VERSION_NUMERIC(1, 22, 00, 00)
		&& (GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
		&& (IsCompatibleMode(TRK_IMPULSETRACKER) || GetModFlag(MSF_OLDVOLSWING)))
	{
		// The "correct" pan swing implementation did nothing if the instrument also had a pan envelope.
		// If there's a pan envelope, disable pan swing for such modules.
		for(INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++)
		{
			if(Instruments[i] != nullptr && Instruments[i]->nPanSwing != 0 && Instruments[i]->PanEnv.dwFlags[ENV_ENABLED])
			{
				Instruments[i]->nPanSwing = 0;
			}
		}
	}

	if(m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 22, 07, 01))
	{
		// Convert ANSI plugin path names to UTF-8 (irrelevant in probably 99% of all cases anyway, I think I've never seen a VST plugin with a non-ASCII file name)
		for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
		{
#if defined(MODPLUG_TRACKER)
			const std::string name = mpt::ToCharset(mpt::CharsetUTF8, mpt::CharsetLocale, m_MixPlugins[i].Info.szLibraryName);
#else
			const std::string name = mpt::ToCharset(mpt::CharsetUTF8, mpt::CharsetWindows1252, m_MixPlugins[i].Info.szLibraryName);
#endif
			mpt::String::Copy(m_MixPlugins[i].Info.szLibraryName, name);
		}
	}

	// Starting from OpenMPT 1.22.07.19, FT2-style panning was applied compatible mix mode.
	// Starting from OpenMPT 1.23.01.04, FT2-style panning has its own mix mode instead.
	if(GetType() == MOD_TYPE_XM)
	{
		if(m_dwLastSavedWithVersion >= MAKE_VERSION_NUMERIC(1, 22, 07, 19)
			&& m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 23, 01, 04)
			&& GetMixLevels() == mixLevelsCompatible)
		{
			SetMixLevels(mixLevelsCompatibleFT2);
		}
	}

	if(m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 25, 00, 07) && m_dwLastSavedWithVersion != MAKE_VERSION_NUMERIC(1, 25, 00, 00))
	{
		// Instrument plugins can now receive random volume variation.
		// For old instruments, disable volume swing in case there was no sample associated.
		for(INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++)
		{
			if(Instruments[i] != nullptr && Instruments[i]->nVolSwing != 0 && Instruments[i]->nMidiChannel != MidiNoChannel)
			{
				bool hasSample = false;
				for(size_t k = 0; k < CountOf(Instruments[k]->Keyboard); k++)
				{
					if(Instruments[i]->Keyboard[k] != 0)
					{
						hasSample = true;
						break;
					}
				}
				if(!hasSample)
				{
					Instruments[i]->nVolSwing = 0;
				}
			}
		}
	}

	Patterns.ForEachModCommand(UpgradePatternData(*this));
}


OPENMPT_NAMESPACE_END
