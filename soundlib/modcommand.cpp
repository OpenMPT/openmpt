/*
 * ModCommand.cpp
 * --------------
 * Purpose: Various functions for writing effects to patterns, converting ModCommands, etc.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"

extern BYTE ImpulseTrackerPortaVolCmd[16];


// Convert an Exx command (MOD) to Sxx command (S3M)
void CSoundFile::MODExx2S3MSxx(ModCommand *m)
//-------------------------------------------
{
	if(m->command != CMD_MODCMDEX) return;
	m->command = CMD_S3MCMDEX;
	switch(m->param & 0xF0)
	{
	case 0x10:	m->command = CMD_PORTAMENTOUP; m->param |= 0xF0; break;
	case 0x20:	m->command = CMD_PORTAMENTODOWN; m->param |= 0xF0; break;
	case 0x30:	m->param = (m->param & 0x0F) | 0x10; break;
	case 0x40:	m->param = (m->param & 0x03) | 0x30; break;
	case 0x50:	m->param = (m->param & 0x0F) | 0x20; break;
	case 0x60:	m->param = (m->param & 0x0F) | 0xB0; break;
	case 0x70:	m->param = (m->param & 0x03) | 0x40; break;
	case 0x90:	m->command = CMD_RETRIG; m->param = (m->param & 0x0F); break;
	case 0xA0:	if (m->param & 0x0F) { m->command = CMD_VOLUMESLIDE; m->param = (m->param << 4) | 0x0F; } else m->command = 0; break;
	case 0xB0:	if (m->param & 0x0F) { m->command = CMD_VOLUMESLIDE; m->param |= 0xF0; } else m->command = 0; break;
	case 0xC0:  if (m->param == 0xC0) { m->command = CMD_NONE; m->note = NOTE_NOTECUT; }	// this does different things in IT and ST3
	case 0xD0:  if (m->param == 0xD0) { m->command = CMD_NONE; }	// dito
				// rest are the same
	}
}


// Convert an Sxx command (S3M) to Exx command (MOD)
void CSoundFile::S3MSxx2MODExx(ModCommand *m)
//-------------------------------------------
{
	if(m->command != CMD_S3MCMDEX) return;
	m->command = CMD_MODCMDEX;
	switch(m->param & 0xF0)
	{
	case 0x10:	m->param = (m->param & 0x0F) | 0x30; break;
	case 0x20:	m->param = (m->param & 0x0F) | 0x50; break;
	case 0x30:	m->param = (m->param & 0x0F) | 0x40; break;
	case 0x40:	m->param = (m->param & 0x0F) | 0x70; break;
	case 0x50:	
	case 0x60:	
	case 0x90:
	case 0xA0:	m->command = CMD_XFINEPORTAUPDOWN; break;
	case 0xB0:	m->param = (m->param & 0x0F) | 0x60; break;
	case 0x70:  m->command = CMD_NONE;	// No NNA / envelope control in MOD/XM format
		// rest are the same
	}
}


// Convert a mod command from one format to another. 
void CSoundFile::ConvertCommand(ModCommand *m, MODTYPE nOldType, MODTYPE nNewType)
//--------------------------------------------------------------------------------
{
	// helper variables
	const bool oldTypeIsMOD = (nOldType == MOD_TYPE_MOD), oldTypeIsXM = (nOldType == MOD_TYPE_XM),
		oldTypeIsS3M = (nOldType == MOD_TYPE_S3M), oldTypeIsIT = (nOldType == MOD_TYPE_IT),
		oldTypeIsMPT = (nOldType == MOD_TYPE_MPT), oldTypeIsMOD_XM = (oldTypeIsMOD || oldTypeIsXM),
		oldTypeIsS3M_IT_MPT = (oldTypeIsS3M || oldTypeIsIT || oldTypeIsMPT),
		oldTypeIsIT_MPT = (oldTypeIsIT || oldTypeIsMPT);

	const bool newTypeIsMOD = (nNewType == MOD_TYPE_MOD), newTypeIsXM =  (nNewType == MOD_TYPE_XM), 
		newTypeIsS3M = (nNewType == MOD_TYPE_S3M), newTypeIsIT = (nNewType == MOD_TYPE_IT),
		newTypeIsMPT = (nNewType == MOD_TYPE_MPT), newTypeIsMOD_XM = (newTypeIsMOD || newTypeIsXM), 
		newTypeIsS3M_IT_MPT = (newTypeIsS3M || newTypeIsIT || newTypeIsMPT), 
		newTypeIsIT_MPT = (newTypeIsIT || newTypeIsMPT);

	//////////////////////////
	// Convert 8-bit Panning
	if(m->command == CMD_PANNING8)
	{
		if(newTypeIsS3M)
		{
			m->param = (m->param + 1) >> 1;
		}
		else if(oldTypeIsS3M)
		{
			if(m->param == 0xA4)
			{
				// surround remap
				m->command = (nNewType & (MOD_TYPE_IT|MOD_TYPE_MPT)) ? CMD_S3MCMDEX : CMD_XFINEPORTAUPDOWN;
				m->param = 0x91;
			}
			else
			{
				m->param = min(m->param << 1, 0xFF);
			}
		}
	} // End if(m->command == CMD_PANNING8)

	// Re-map \xx to Zxx if the new format only knows the latter command.
	if(m->command == CMD_SMOOTHMIDI && !GetModSpecifications(nNewType).HasCommand(CMD_SMOOTHMIDI) && GetModSpecifications(nNewType).HasCommand(CMD_MIDI))
	{
		m->command = CMD_MIDI;
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// MPTM to anything: Convert param control, extended envelope control, note delay+cut
	if(oldTypeIsMPT)
	{
		if(m->IsPcNote())
		{
			ModCommand::COMMAND newcommand = (m->note == NOTE_PC) ? CMD_MIDI : CMD_SMOOTHMIDI;
			if(!GetModSpecifications(nNewType).HasCommand(newcommand))
			{
				newcommand = CMD_MIDI;	// assuming that this was CMD_SMOOTHMIDI
			}
			if(!GetModSpecifications(nNewType).HasCommand(newcommand))
			{
				newcommand = CMD_NONE;
			}

			m->param = (BYTE)(min(ModCommand::maxColumnValue, m->GetValueEffectCol()) * 0x7F / ModCommand::maxColumnValue);
			m->command = newcommand; // might be removed later
			m->volcmd = VOLCMD_NONE;
			m->note = NOTE_NONE;
			m->instr = 0;
		}

		// adjust extended envelope control commands
		if((m->command == CMD_S3MCMDEX) && ((m->param & 0xF0) == 0x70) && ((m->param & 0x0F) > 0x0C))
		{
			m->param = 0x7C;
		}

		if(m->command == CMD_DELAYCUT)
		{
			m->command = CMD_S3MCMDEX; // when converting to MOD/XM, this will be converted to CMD_MODCMDEX later
			m->param = 0xD0 | (m->param >> 4); // preserve delay nibble.
		}
	} // End if(oldTypeIsMPT)

	/////////////////////////////////////////
	// Convert MOD / XM to S3M / IT / MPTM
	if(oldTypeIsMOD_XM && newTypeIsS3M_IT_MPT)
	{
		switch(m->command)
		{
		case CMD_MODCMDEX:
			MODExx2S3MSxx(m);
			break;

		case CMD_VOLUME:
			// Effect column volume command overrides the volume column in XM.
			if (m->volcmd == VOLCMD_NONE || m->volcmd == VOLCMD_VOLUME)
			{
				m->volcmd = VOLCMD_VOLUME;
				m->vol = m->param;
				if(m->vol > 64) m->vol = 64;
				m->command = m->param = 0;
			} else if(m->volcmd == VOLCMD_PANNING)
			{
				m->SwapEffects();
				m->volcmd = VOLCMD_VOLUME;
				if(m->vol > 64) m->vol = 64;
				m->command = CMD_S3MCMDEX;
				m->param = 0x80 | ((m->param * 15 + 32) / 64);	// XM volcol panning is 4-Bit, so we can use 4-Bit panning here.
			}
			break;

		case CMD_PORTAMENTOUP:
			if (m->param > 0xDF) m->param = 0xDF;
			break;

		case CMD_PORTAMENTODOWN:
			if (m->param > 0xDF) m->param = 0xDF;
			break;

		case CMD_XFINEPORTAUPDOWN:
			switch(m->param & 0xF0)
			{
			case 0x10:	m->command = CMD_PORTAMENTOUP; m->param = (m->param & 0x0F) | 0xE0; break;
			case 0x20:	m->command = CMD_PORTAMENTODOWN; m->param = (m->param & 0x0F) | 0xE0; break;
			case 0x50:
			case 0x60:
			case 0x70:
			case 0x90:
			case 0xA0:
				m->command = CMD_S3MCMDEX;
				// surround remap (this is the "official" command)
				if(nNewType & MOD_TYPE_S3M && m->param == 0x91)
				{
					m->command = CMD_PANNING8;
					m->param = 0xA4;
				}
				break;
			}
			break;

		case CMD_KEYOFF:
			if(m->note == NOTE_NONE)
			{
				m->note = (newTypeIsS3M) ? NOTE_NOTECUT : NOTE_KEYOFF;
				m->command = CMD_S3MCMDEX;
				if(m->param == 0)
					m->instr = 0;
				m->param = 0xD0 | (m->param & 0x0F);
			}
			break;

		case CMD_PANNINGSLIDE:
			// swap L/R, convert to fine slide
			if(m->param & 0xF0)
			{
				m->param = 0xF0 | min(0x0E, (m->param >> 4));
			} else
			{
				m->param = 0x0F | (min(0x0E, m->param & 0x0F) << 4);
			}

		default:
			break;
		}
	} // End if(oldTypeIsMOD_XM && newTypeIsS3M_IT_MPT)


	/////////////////////////////////////////
	// Convert S3M / IT / MPTM to MOD / XM
	else if(oldTypeIsS3M_IT_MPT && newTypeIsMOD_XM)
	{
		if(m->note == NOTE_NOTECUT)
		{
			// convert note cut to EC0
			m->note = NOTE_NONE;
			m->command = CMD_MODCMDEX;
			m->param = 0xC0;
		} else if(m->note == NOTE_FADE)
		{
			// convert note fade to note off
			m->note = NOTE_KEYOFF;
		}

		switch(m->command)
		{
		case CMD_S3MCMDEX:
			S3MSxx2MODExx(m);
			break;

		case CMD_VOLUMESLIDE:
			if ((m->param & 0xF0) && ((m->param & 0x0F) == 0x0F))
			{
				m->command = CMD_MODCMDEX;
				m->param = (m->param >> 4) | 0xA0;
			} else
				if ((m->param & 0x0F) && ((m->param & 0xF0) == 0xF0))
				{
					m->command = CMD_MODCMDEX;
					m->param = (m->param & 0x0F) | 0xB0;
				}
				break;

		case CMD_PORTAMENTOUP:
			if (m->param >= 0xF0)
			{
				m->command = CMD_MODCMDEX;
				m->param = (m->param & 0x0F) | 0x10;
			} else
				if (m->param >= 0xE0)
				{
					if(newTypeIsXM)
					{
						m->command = CMD_XFINEPORTAUPDOWN;
						m->param = 0x10 | (m->param & 0x0F);
					} else
					{
						m->command = CMD_MODCMDEX;
						m->param = (((m->param & 0x0F) + 3) >> 2) | 0x10;
					}
				} else m->command = CMD_PORTAMENTOUP;
			break;

		case CMD_PORTAMENTODOWN:
			if (m->param >= 0xF0)
			{
				m->command = CMD_MODCMDEX;
				m->param = (m->param & 0x0F) | 0x20;
			} else
				if (m->param >= 0xE0)
				{
					if(newTypeIsXM)
					{
						m->command = CMD_XFINEPORTAUPDOWN;
						m->param = 0x20 | (m->param & 0x0F);
					} else
					{
						m->command = CMD_MODCMDEX;
						m->param = (((m->param & 0x0F) + 3) >> 2) | 0x20;
					}
				} else m->command = CMD_PORTAMENTODOWN;
			break;

		case CMD_SPEED:
			{
				m->param = min(m->param, (nNewType == MOD_TYPE_XM) ? 0x1F : 0x20);
			}
			break;

		case CMD_TEMPO:
			if(m->param < 0x20) m->command = CMD_NONE; // no tempo slides
			break;

		case CMD_PANNINGSLIDE:
			// swap L/R, convert fine slides to normal slides
			if((m->param & 0x0F) == 0x0F && (m->param & 0xF0))
			{
				m->param = (m->param >> 4);
			} else if((m->param & 0xF0) == 0xF0 && (m->param & 0x0F))
			{
				m->param = (m->param & 0x0F) << 4;
			} else if(m->param & 0x0F)
			{
				m->param = 0xF0;
			} else if(m->param & 0xF0)
			{
				m->param = 0x0F;
			} else
			{
				m->param = 0;
			}
			break;

		case CMD_RETRIG:
			// Retrig: Q0y doesn't change volume in IT/S3M, but R0y in XM takes the last x parameter
			if(m->param != 0 && (m->param & 0xF0) == 0)
			{
				m->param |= 0x80;
			}
			break;

		default:
			break;
		}
	} // End if(oldTypeIsS3M_IT_MPT && newTypeIsMOD_XM)


	///////////////////////
	// Convert IT to S3M
	else if(oldTypeIsIT_MPT && newTypeIsS3M)
	{
		if(m->note == NOTE_KEYOFF || m->note == NOTE_FADE)
			m->note = NOTE_NOTECUT;

		switch(m->command)
		{
		case CMD_S3MCMDEX:
			switch(m->param & 0xF0)
			{
			case 0x70: m->command = CMD_NONE; break;	// No NNA / envelope control in S3M format
			case 0x90:
				if(m->param == 0x91)
				{
					// surround remap (this is the "official" command)
					m->command = CMD_PANNING8;
					m->param = 0xA4;
				} else if(m->param == 0x90)
				{
					m->command = CMD_PANNING8;
					m->param = 0x40;
				}
				break;
			}
			break;

		case CMD_GLOBALVOLUME:
			m->param = (min(0x80, m->param) + 1) / 2;
			break;

		default:
			break;
		}
	} // End if (oldTypeIsIT_MPT && newTypeIsS3M)

	//////////////////////
	// Convert IT to XM
	if(oldTypeIsIT_MPT && newTypeIsXM)
	{
		switch(m->command)
		{
		case CMD_GLOBALVOLUME:
			m->param = (min(0x80, m->param) + 1) / 2;
			break;
		}
	} // End if(oldTypeIsIT_MPT && newTypeIsXM)

	//////////////////////
	// Convert XM to IT
	if(oldTypeIsXM && newTypeIsIT_MPT)
	{
		switch(m->command)
		{
		case CMD_GLOBALVOLUME:
			m->param = min(0x80, m->param * 2);
			break;
		}
	} // End if(oldTypeIsIT_MPT && newTypeIsXM)

	///////////////////////////////////
	// MOD <-> XM: Speed/Tempo update
	if(oldTypeIsMOD && newTypeIsXM)
	{
		switch(m->command)
		{
		case CMD_SPEED:
			m->param = min(m->param, 0x1F);
			break;
		}
	} else if(oldTypeIsXM && newTypeIsMOD)
	{
		switch(m->command)
		{
		case CMD_TEMPO:
			m->param = max(m->param, 0x21);
			break;
		}
	}


	///////////////////////////////////////////////////////////////////////
	// Convert MOD to anything - adjust effect memory, remove Invert Loop
	if (oldTypeIsMOD)
	{
		switch(m->command)
		{
		case CMD_TONEPORTAVOL: // lacks memory -> 500 is the same as 300
			if(m->param == 0x00) m->command = CMD_TONEPORTAMENTO;
			break;

		case CMD_VIBRATOVOL: // lacks memory -> 600 is the same as 400
			if(m->param == 0x00) m->command = CMD_VIBRATO;
			break;

		case CMD_MODCMDEX: // This would turn into "Set Active Macro", so let's better remove it
		case CMD_S3MCMDEX:
			if((m->param & 0xF0) == 0xF0) m->command = CMD_NONE;
			break;
		}
	} // End if (oldTypeIsMOD && newTypeIsXM)

	/////////////////////////////////////////////////////////////////////
	// Convert anything to MOD - remove volume column, remove Set Macro
	if (newTypeIsMOD)
	{
		// convert note off events
		if(m->note >= NOTE_MIN_SPECIAL)
		{
			m->note = NOTE_NONE;
			// no effect present, so just convert note off to volume 0
			if(m->command == CMD_NONE)
			{
				m->command = CMD_VOLUME;
				m->param = 0;
				// EDx effect present, so convert it to ECx
			} else if((m->command == CMD_MODCMDEX) && ((m->param & 0xF0) == 0xD0))
			{
				m->param = 0xC0 | (m->param & 0x0F);
			}
		}

		if(m->command) switch(m->command)
		{
			case CMD_RETRIG: // MOD only has E9x
				m->command = CMD_MODCMDEX;
				m->param = 0x90 | (m->param & 0x0F);
				break;

			case CMD_MODCMDEX: // This would turn into "Invert Loop", so let's better remove it
				if((m->param & 0xF0) == 0xF0) m->command = CMD_NONE;
				break;
		}

		else switch(m->volcmd)
		{
			case VOLCMD_VOLUME:
				m->command = CMD_VOLUME;
				m->param = m->vol;
				break;

			case VOLCMD_PANNING:
				m->command = CMD_PANNING8;
				m->param = CLAMP(m->vol << 2, 0, 0xFF);
				break;

			case VOLCMD_VOLSLIDEDOWN:
				m->command = CMD_VOLUMESLIDE;
				m->param = m->vol;
				break;

			case VOLCMD_VOLSLIDEUP:
				m->command = CMD_VOLUMESLIDE;
				m->param = m->vol << 4;
				break;

			case VOLCMD_FINEVOLDOWN:
				m->command = CMD_MODCMDEX;
				m->param = 0xB0 | m->vol;
				break;

			case VOLCMD_FINEVOLUP:
				m->command = CMD_MODCMDEX;
				m->param = 0xA0 | m->vol;
				break;

			case VOLCMD_PORTADOWN:
				m->command = CMD_PORTAMENTODOWN;
				m->param = m->vol << 2;
				break;

			case VOLCMD_PORTAUP:
				m->command = CMD_PORTAMENTOUP;
				m->param = m->vol << 2;
				break;

			case VOLCMD_TONEPORTAMENTO:
				m->command = CMD_TONEPORTAMENTO;
				m->param = m->vol << 2;
				break;

			case VOLCMD_VIBRATODEPTH:
				m->command = CMD_VIBRATO;
				m->param = m->vol;
				break;

			case VOLCMD_VIBRATOSPEED:
				m->command = CMD_VIBRATO;
				m->param = m->vol << 4;
				break;
				// OpenMPT-specific commands

			case VOLCMD_OFFSET:
				m->command = CMD_OFFSET;
				m->param = m->vol << 3;
				break;

		}
		m->volcmd = CMD_NONE;
	} // End if (newTypeIsMOD)

	///////////////////////////////////////////////////
	// Convert anything to S3M - adjust volume column
	if (newTypeIsS3M)
	{
		if(!m->command) switch(m->volcmd)
		{
			case VOLCMD_VOLSLIDEDOWN:
				m->command = CMD_VOLUMESLIDE;
				m->param = m->vol;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_VOLSLIDEUP:
				m->command = CMD_VOLUMESLIDE;
				m->param = m->vol << 4;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_FINEVOLDOWN:
				m->command = CMD_VOLUMESLIDE;
				m->param = 0xF0 | m->vol;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_FINEVOLUP:
				m->command = CMD_VOLUMESLIDE;
				m->param = (m->vol << 4) | 0x0F;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_PORTADOWN:
				m->command = CMD_PORTAMENTODOWN;
				m->param = m->vol << 2;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_PORTAUP:
				m->command = CMD_PORTAMENTOUP;
				m->param = m->vol << 2;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_TONEPORTAMENTO:
				m->command = CMD_TONEPORTAMENTO;
				m->param = m->vol << 2;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_VIBRATODEPTH:
				m->command = CMD_VIBRATO;
				m->param = m->vol;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_VIBRATOSPEED:
				m->command = CMD_VIBRATO;
				m->param = m->vol << 4;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_PANSLIDELEFT:
				m->command = CMD_PANNINGSLIDE;
				m->param = m->vol << 4;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_PANSLIDERIGHT:
				m->command = CMD_PANNINGSLIDE;
				m->param = m->vol;
				m->volcmd = CMD_NONE;
				break;
				// OpenMPT-specific commands

			case VOLCMD_OFFSET:
				m->command = CMD_OFFSET;
				m->param = m->vol << 3;
				m->volcmd = CMD_NONE;
				break;

		}
	} // End if (newTypeIsS3M)

	////////////////////////////////////////////////////////////////////////
	// Convert anything to XM - adjust volume column, breaking EDx command
	if (newTypeIsXM)
	{
		// remove EDx if no note is next to it, or it will retrigger the note in FT2 mode
		if(m->command == CMD_MODCMDEX && (m->param & 0xF0) == 0xD0 && m->note == NOTE_NONE)
		{
			m->command = m->param = 0;
		}

		if(m->note >= NOTE_MIN_SPECIAL)
		{
			// Instrument numbers next to Note Off reset instrument settings
			m->instr = 0;

			if(m->command == CMD_MODCMDEX && (m->param & 0xF0) == 0xD0)
			{
				// Note Off + Note Delay does nothing when using envelopes.
				m->note = NOTE_NONE;
				m->command = CMD_KEYOFF;
				m->param &= 0x0F;
			}
		}

		if(!m->command) switch(m->volcmd)
		{
			case VOLCMD_PORTADOWN:
				m->command = CMD_PORTAMENTODOWN;
				m->param = m->vol << 2;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_PORTAUP:
				m->command = CMD_PORTAMENTOUP;
				m->param = m->vol << 2;
				m->volcmd = CMD_NONE;
				break;
				// OpenMPT-specific commands

			case VOLCMD_OFFSET:
				m->command = CMD_OFFSET;
				m->param = m->vol << 3;
				m->volcmd = CMD_NONE;
				break;

		}
	} // End if (newTypeIsXM)

	///////////////////////////////////////////////////
	// Convert anything to IT - adjust volume column
	if (newTypeIsIT_MPT)
	{
		if(!m->command) switch(m->volcmd)
		{
			case VOLCMD_VOLSLIDEDOWN:
			case VOLCMD_VOLSLIDEUP:
			case VOLCMD_FINEVOLDOWN:
			case VOLCMD_FINEVOLUP:
			case VOLCMD_PORTADOWN:
			case VOLCMD_PORTAUP:
			case VOLCMD_TONEPORTAMENTO:
			case VOLCMD_VIBRATODEPTH:
				// OpenMPT-specific commands
			case VOLCMD_OFFSET:
				m->vol = min(m->vol, 9);
				break;

			case VOLCMD_PANSLIDELEFT:
				m->command = CMD_PANNINGSLIDE;
				m->param = m->vol << 4;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_PANSLIDERIGHT:
				m->command = CMD_PANNINGSLIDE;
				m->param = m->vol;
				m->volcmd = CMD_NONE;
				break;

			case VOLCMD_VIBRATOSPEED:
				m->command = CMD_VIBRATO;
				m->param = m->vol << 4;
				m->volcmd = CMD_NONE;
				break;

		}
	} // End if (newTypeIsIT)

	if(!CSoundFile::GetModSpecifications(nNewType).HasNote(m->note))
		m->note = NOTE_NONE;

	// ensure the commands really exist in this format
	if(!CSoundFile::GetModSpecifications(nNewType).HasCommand(m->command))
		m->command = CMD_NONE;
	if(!CSoundFile::GetModSpecifications(nNewType).HasVolCommand(m->volcmd))
		m->volcmd = VOLCMD_NONE;

}


// "importance" of every FX command. Table is used for importing from formats with multiple effect colums
// and is approximately the same as in SchismTracker.
size_t CSoundFile::GetEffectWeight(ModCommand::COMMAND cmd)
//---------------------------------------------------------
{
	// Effect weights, sorted from lowest to highest weight.
	static const ModCommand::COMMAND weights[] =
	{
		CMD_NONE,
		CMD_XPARAM,
		CMD_SETENVPOSITION,
		CMD_KEYOFF,
		CMD_TREMOLO,
		CMD_FINEVIBRATO,
		CMD_VIBRATO,
		CMD_XFINEPORTAUPDOWN,
		CMD_PANBRELLO,
		CMD_S3MCMDEX,
		CMD_MODCMDEX,
		CMD_DELAYCUT,
		CMD_MIDI,
		CMD_SMOOTHMIDI,
		CMD_PANNINGSLIDE,
		CMD_PANNING8,
		CMD_NOTESLIDEUP,
		CMD_NOTESLIDEDOWN,
		CMD_PORTAMENTOUP,
		CMD_PORTAMENTODOWN,
		CMD_VOLUMESLIDE,
		CMD_VIBRATOVOL,
		CMD_VOLUME,
		CMD_OFFSET,
		CMD_TREMOR,
		CMD_RETRIG,
		CMD_ARPEGGIO,
		CMD_TONEPORTAMENTO,
		CMD_TONEPORTAVOL,
		CMD_GLOBALVOLSLIDE,
		CMD_CHANNELVOLUME,
		CMD_GLOBALVOLSLIDE,
		CMD_GLOBALVOLUME,
		CMD_TEMPO,
		CMD_SPEED,
		CMD_POSITIONJUMP,
		CMD_PATTERNBREAK,
	};
	STATIC_ASSERT(CountOf(weights) == MAX_EFFECTS);

	for(size_t i = 0; i < CountOf(weights); i++)
	{
		if(weights[i] == cmd)
		{
			return i;
		}
	}
	// Invalid / unknown command.
	return 0;
}


/* Try to write an (volume column) effect in a given channel or any channel of a pattern in a specific row.
   Usage: nPat - Pattern that should be modified
          nRow - Row that should be modified
		  nEffect - (Volume) Effect that should be written
		  nParam - Effect that should be written
          bIsVolumeEffect  - Indicates whether the given effect is a volume column effect or not
		  nChn - Channel that should be modified - use CHANNELINDEX_INVALID to allow all channels of the given row
		  bAllowMultipleEffects - If false, No effect will be written if an effect of the same type is already present in the channel(s). Useful for f.e. tempo effects.
		  allowRowChange - Indicates whether it is allowed to use the next or previous row if there's no space for the effect
		  bRetry - For internal use only. Indicates whether an effect "rewrite" has already taken place (for recursive calls)
   NOTE: Effect remapping is only implemented for a few basic effects.
*/ 
bool CSoundFile::TryWriteEffect(PATTERNINDEX nPat, ROWINDEX nRow, BYTE nEffect, BYTE nParam, bool bIsVolumeEffect, CHANNELINDEX nChn, bool bAllowMultipleEffects, writeEffectAllowRowChange allowRowChange, bool bRetry)
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	// First, reject invalid parameters.
	if(!Patterns.IsValidPat(nPat) || nRow >= Patterns[nPat].GetNumRows() || (nChn >= GetNumChannels() && nChn != CHANNELINDEX_INVALID))
	{
		return false;
	}

	CHANNELINDEX nScanChnMin = nChn, nScanChnMax = nChn;

	// Scan all channels
	if(nChn == CHANNELINDEX_INVALID)
	{
		nScanChnMin = 0;
		nScanChnMax = GetNumChannels() - 1;
	}

	ModCommand  * const p = Patterns[nPat].GetpModCommand(nRow, nScanChnMin);
	ModCommand *m;

	// Scan channel(s) for same effect type - if an effect of the same type is already present, exit.
	if(!bAllowMultipleEffects)
	{
		m = p;
		for(CHANNELINDEX i = nScanChnMin; i <= nScanChnMax; i++, m++)
		{
			if(!bIsVolumeEffect && m->command == nEffect)
				return true;
			if(bIsVolumeEffect && m->volcmd == nEffect)
				return true;
		}
	}

	// Easy case: check if there's some space left to put the effect somewhere
	m = p;
	for(CHANNELINDEX i = nScanChnMin; i <= nScanChnMax; i++, m++)
	{
		if(!bIsVolumeEffect && m->command == CMD_NONE)
		{
			m->command = nEffect;
			m->param = nParam;
			return true;
		}
		if(bIsVolumeEffect && m->volcmd == VOLCMD_NONE)
		{
			m->volcmd = nEffect;
			m->vol = nParam;
			return true;
		}
	}

	// Ok, apparently there's no space. If we haven't tried already, try to map it to the volume column or effect column instead.
	if(bRetry)
	{
		// Move some effects that also work in the volume column, so there's place for our new effect.
		if(!bIsVolumeEffect)
		{
			m = p;
			for(CHANNELINDEX i = nScanChnMin; i <= nScanChnMax; i++, m++)
			{
				switch(m->command)
				{
				case CMD_VOLUME:
					m->volcmd = VOLCMD_VOLUME;
					m->vol = m->param;
					m->command = nEffect;
					m->param = nParam;
					return true;

				case CMD_PANNING8:
					if(m_nType & MOD_TYPE_S3M && nParam > 0x80)
						break;

					m->volcmd = VOLCMD_PANNING;
					m->command = nEffect;

					if(m_nType & MOD_TYPE_S3M)
					{
						m->vol = m->param >> 1;
					}
					else
					{
						m->vol = (m->param >> 2) + 1;
					}

					m->param = nParam;
					return true;
				}
			}
		}

		// Let's try it again by writing into the "other" effect column.
		BYTE nNewEffect = CMD_NONE;
		if(bIsVolumeEffect)
		{
			switch(nEffect)
			{
			case VOLCMD_PANNING:
				nNewEffect = CMD_PANNING8;
				if(m_nType & MOD_TYPE_S3M)
					nParam <<= 1;
				else
					nParam = min(nParam << 2, 0xFF);
				break;
			case VOLCMD_VOLUME:
				nNewEffect = CMD_VOLUME;
				break;
			}
		} else
		{
			switch(nEffect)
			{
			case CMD_PANNING8:
				nNewEffect = VOLCMD_PANNING;
				if(m_nType & MOD_TYPE_S3M)
				{
					if(nParam <= 0x80)
						nParam >>= 1;
					else
						nNewEffect = CMD_NONE;
				}
				else
				{
					nParam = (nParam >> 2) + 1;
				}
				break;
			case CMD_VOLUME:
				nNewEffect = CMD_VOLUME;
				break;
			}
		}
		if(nNewEffect != CMD_NONE)
		{
			if(TryWriteEffect(nPat, nRow, nNewEffect, nParam, !bIsVolumeEffect, nChn, bAllowMultipleEffects, allowRowChange, false) == true) return true;
		}
	}

	// Try in the next row if possible (this may also happen if we already retried)
	if(allowRowChange == weTryNextRow && (nRow + 1 < Patterns[nPat].GetNumRows()))
	{
		return TryWriteEffect(nPat, nRow + 1, nEffect, nParam, bIsVolumeEffect, nChn, bAllowMultipleEffects, allowRowChange, bRetry);
	} else if(allowRowChange == weTryPreviousRow && (nRow > 0))
	{
		return TryWriteEffect(nPat, nRow - 1, nEffect, nParam, bIsVolumeEffect, nChn, bAllowMultipleEffects, allowRowChange, bRetry);
	}

	return false;
}


// Try to convert a fx column command (*e) into a volume column command.
// Returns true if successful.
// Some commands can only be converted by losing some precision.
// If moving the command into the volume column is more important than accuracy, use bForce = true.
// (Code translated from SchismTracker and mainly supposed to be used with loaders ported from this tracker)
bool CSoundFile::ConvertVolEffect(uint8 *e, uint8 *p, bool bForce)
//----------------------------------------------------------------
{
	switch (*e)
	{
	case CMD_NONE:
		return true;
	case CMD_VOLUME:
		*e = VOLCMD_VOLUME;
		*p = min(*p, 64);
		break;
	case CMD_PORTAMENTOUP:
		// if not force, reject when dividing causes loss of data in LSB, or if the final value is too
		// large to fit. (volume column Ex/Fx are four times stronger than effect column)
		if (!bForce && ((*p & 3) || *p > 9 * 4 + 3))
			return false;
		*p = min(*p / 4, 9);
		*e = VOLCMD_PORTAUP;
		break;
	case CMD_PORTAMENTODOWN:
		if (!bForce && ((*p & 3) || *p > 9 * 4 + 3))
			return false;
		*p = min(*p / 4, 9);
		*e = VOLCMD_PORTADOWN;
		break;
	case CMD_TONEPORTAMENTO:
		if (*p >= 0xF0)
		{
			// hack for people who can't type F twice :)
			*e = VOLCMD_TONEPORTAMENTO;
			*p = 9;
			return true;
		}
		for (uint8 n = 0; n < 10; n++)
		{
			if (bForce
				? (*p <= ImpulseTrackerPortaVolCmd[n])
				: (*p == ImpulseTrackerPortaVolCmd[n]))
			{
				*e = VOLCMD_TONEPORTAMENTO;
				*p = n;
				return true;
			}
		}
		return false;
	case CMD_VIBRATO:
		if (bForce)
			*p = min(*p, 9);
		else if (*p > 9)
			return false;
		*e = VOLCMD_VIBRATODEPTH;
		break;
	case CMD_FINEVIBRATO:
		if (bForce)
			*p = 0;
		else if (*p)
			return false;
		*e = VOLCMD_VIBRATODEPTH;
		break;
	case CMD_PANNING8:
		*p = min(64, *p * 64 / 255);
		*e = VOLCMD_PANNING;
		break;
	case CMD_VOLUMESLIDE:
		if (*p == 0)
			return false;
		if ((*p & 0xF) == 0)	// Dx0 / Cx
		{
			if (bForce)
				*p = min(*p >> 4, 9);
			else if ((*p >> 4) > 9)
				return false;
			else
				*p >>= 4;
			*e = VOLCMD_VOLSLIDEUP;
		} else if ((*p & 0xF0) == 0)	// D0x / Dx
		{
			if (bForce)
				*p = min(*p, 9);
			else if (*p > 9)
				return false;
			*e = VOLCMD_VOLSLIDEDOWN;
		} else if ((*p & 0xF) == 0xF)	// DxF / Ax
		{
			if (bForce)
				*p = min(*p >> 4, 9);
			else if ((*p >> 4) > 9)
				return false;
			else
				*p >>= 4;
			*e = VOLCMD_FINEVOLUP;
		} else if ((*p & 0xf0) == 0xf0)	// DFx / Bx
		{
			if (bForce)
				*p = min(*p, 9);
			else if ((*p & 0xF) > 9)
				return false;
			else
				*p &= 0xF;
			*e = VOLCMD_FINEVOLDOWN;
		} else // ???
		{
			return false;
		}
		break;
	case CMD_S3MCMDEX:
		switch (*p >> 4)
		{
		case 8:
			*e = VOLCMD_PANNING;
			*p = ((*p & 0xf) << 2) + 2;
			return true;
		case 0: case 1: case 2: case 0xF:
			if (bForce)
			{
				*e = *p = 0;
				return true;
			}
			break;
		default:
			break;
		}
		return false;
	default:
		return false;
	}
	return true;
}
