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
void ModCommand::ExtendedMODtoS3MEffect()
//---------------------------------------
{
	if(command != CMD_MODCMDEX)
		return;

	command = CMD_S3MCMDEX;
	switch(param & 0xF0)
	{
	case 0x10:	command = CMD_PORTAMENTOUP; param |= 0xF0; break;
	case 0x20:	command = CMD_PORTAMENTODOWN; param |= 0xF0; break;
	case 0x30:	param = (param & 0x0F) | 0x10; break;
	case 0x40:	param = (param & 0x03) | 0x30; break;
	case 0x50:	param = (param & 0x0F) | 0x20; break;
	case 0x60:	param = (param & 0x0F) | 0xB0; break;
	case 0x70:	param = (param & 0x03) | 0x40; break;
	case 0x90:	command = CMD_RETRIG; param = (param & 0x0F); break;
	case 0xA0:	if (param & 0x0F) { command = CMD_VOLUMESLIDE; param = (param << 4) | 0x0F; } else command = 0; break;
	case 0xB0:	if (param & 0x0F) { command = CMD_VOLUMESLIDE; param |= 0xF0; } else command = 0; break;
	case 0xC0:  if (param == 0xC0) { command = CMD_NONE; note = NOTE_NOTECUT; }	// this does different things in IT and ST3
	case 0xD0:  if (param == 0xD0) { command = CMD_NONE; }	// dito
				// rest are the same
	}
}


// Convert an Sxx command (S3M) to Exx command (MOD)
void ModCommand::ExtendedS3MtoMODEffect()
//---------------------------------------
{
	if(command != CMD_S3MCMDEX)
		return;

	command = CMD_MODCMDEX;
	switch(param & 0xF0)
	{
	case 0x10:	param = (param & 0x0F) | 0x30; break;
	case 0x20:	param = (param & 0x0F) | 0x50; break;
	case 0x30:	param = (param & 0x0F) | 0x40; break;
	case 0x40:	param = (param & 0x0F) | 0x70; break;
	case 0x50:
	case 0x60:
	case 0x90:
	case 0xA0:	command = CMD_XFINEPORTAUPDOWN; break;
	case 0xB0:	param = (param & 0x0F) | 0x60; break;
	case 0x70:  command = CMD_NONE;	// No NNA / envelope control in MOD/XM format
		// rest are the same
	}
}


// Convert a mod command from one format to another.
void ModCommand::Convert(MODTYPE fromType, MODTYPE toType)
//--------------------------------------------------------
{
	if(fromType == toType)
	{
		return;
	}

	// helper variables
	const bool oldTypeIsMOD = (fromType == MOD_TYPE_MOD), oldTypeIsXM = (fromType == MOD_TYPE_XM),
		oldTypeIsS3M = (fromType == MOD_TYPE_S3M), oldTypeIsIT = (fromType == MOD_TYPE_IT),
		oldTypeIsMPT = (fromType == MOD_TYPE_MPT), oldTypeIsMOD_XM = (oldTypeIsMOD || oldTypeIsXM),
		oldTypeIsS3M_IT_MPT = (oldTypeIsS3M || oldTypeIsIT || oldTypeIsMPT),
		oldTypeIsIT_MPT = (oldTypeIsIT || oldTypeIsMPT);

	const bool newTypeIsMOD = (toType == MOD_TYPE_MOD), newTypeIsXM =  (toType == MOD_TYPE_XM),
		newTypeIsS3M = (toType == MOD_TYPE_S3M), newTypeIsIT = (toType == MOD_TYPE_IT),
		newTypeIsMPT = (toType == MOD_TYPE_MPT), newTypeIsMOD_XM = (newTypeIsMOD || newTypeIsXM),
		newTypeIsS3M_IT_MPT = (newTypeIsS3M || newTypeIsIT || newTypeIsMPT),
		newTypeIsIT_MPT = (newTypeIsIT || newTypeIsMPT);

	//////////////////////////
	// Convert 8-bit Panning
	if(command == CMD_PANNING8)
	{
		if(newTypeIsS3M)
		{
			param = (param + 1) >> 1;
		}
		else if(oldTypeIsS3M)
		{
			if(param == 0xA4)
			{
				// surround remap
				command = static_cast<ModCommand::COMMAND>((toType & (MOD_TYPE_IT | MOD_TYPE_MPT)) ? CMD_S3MCMDEX : CMD_XFINEPORTAUPDOWN);
				param = 0x91;
			}
			else
			{
				param = min(param << 1, 0xFF);
			}
		}
	} // End if(command == CMD_PANNING8)

	// Re-map \xx to Zxx if the new format only knows the latter command.
	if(command == CMD_SMOOTHMIDI && !CSoundFile::GetModSpecifications(toType).HasCommand(CMD_SMOOTHMIDI) && CSoundFile::GetModSpecifications(toType).HasCommand(CMD_MIDI))
	{
		command = CMD_MIDI;
	}

	///////////////////////////////////////////////////////////////////////////////////////
	// MPTM to anything: Convert param control, extended envelope control, note delay+cut
	if(oldTypeIsMPT)
	{
		if(IsPcNote())
		{
			ModCommand::COMMAND newcommand = static_cast<ModCommand::COMMAND>(note == NOTE_PC ? CMD_MIDI : CMD_SMOOTHMIDI);
			if(!CSoundFile::GetModSpecifications(toType).HasCommand(newcommand))
			{
				newcommand = CMD_MIDI;	// assuming that this was CMD_SMOOTHMIDI
			}
			if(!CSoundFile::GetModSpecifications(toType).HasCommand(newcommand))
			{
				newcommand = CMD_NONE;
			}

			param = (BYTE)(min(ModCommand::maxColumnValue, GetValueEffectCol()) * 0x7F / ModCommand::maxColumnValue);
			command = newcommand; // might be removed later
			volcmd = VOLCMD_NONE;
			note = NOTE_NONE;
			instr = 0;
		}

		// adjust extended envelope control commands
		if((command == CMD_S3MCMDEX) && ((param & 0xF0) == 0x70) && ((param & 0x0F) > 0x0C))
		{
			param = 0x7C;
		}

		if(command == CMD_DELAYCUT)
		{
			command = CMD_S3MCMDEX; // when converting to MOD/XM, this will be converted to CMD_MODCMDEX later
			param = 0xD0 | (param >> 4); // preserve delay nibble.
		}
	} // End if(oldTypeIsMPT)

	/////////////////////////////////////////
	// Convert MOD / XM to S3M / IT / MPTM
	if(oldTypeIsMOD_XM && newTypeIsS3M_IT_MPT)
	{
		switch(command)
		{
		case CMD_MODCMDEX:
			ExtendedMODtoS3MEffect();
			break;

		case CMD_VOLUME:
			// Effect column volume command overrides the volume column in XM.
			if (volcmd == VOLCMD_NONE || volcmd == VOLCMD_VOLUME)
			{
				volcmd = VOLCMD_VOLUME;
				vol = param;
				if(vol > 64) vol = 64;
				command = param = 0;
			} else if(volcmd == VOLCMD_PANNING)
			{
				SwapEffects();
				volcmd = VOLCMD_VOLUME;
				if(vol > 64) vol = 64;
				command = CMD_S3MCMDEX;
				param = 0x80 | ((param * 15 + 32) / 64);	// XM volcol panning is 4-Bit, so we can use 4-Bit panning here.
			}
			break;

		case CMD_PORTAMENTOUP:
			if (param > 0xDF) param = 0xDF;
			break;

		case CMD_PORTAMENTODOWN:
			if (param > 0xDF) param = 0xDF;
			break;

		case CMD_XFINEPORTAUPDOWN:
			switch(param & 0xF0)
			{
			case 0x10:	command = CMD_PORTAMENTOUP; param = (param & 0x0F) | 0xE0; break;
			case 0x20:	command = CMD_PORTAMENTODOWN; param = (param & 0x0F) | 0xE0; break;
			case 0x50:
			case 0x60:
			case 0x70:
			case 0x90:
			case 0xA0:
				command = CMD_S3MCMDEX;
				// surround remap (this is the "official" command)
				if(toType & MOD_TYPE_S3M && param == 0x91)
				{
					command = CMD_PANNING8;
					param = 0xA4;
				}
				break;
			}
			break;

		case CMD_KEYOFF:
			if(note == NOTE_NONE)
			{
				note = (newTypeIsS3M) ? NOTE_NOTECUT : NOTE_KEYOFF;
				command = CMD_S3MCMDEX;
				if(param == 0)
					instr = 0;
				param = 0xD0 | (param & 0x0F);
			}
			break;

		case CMD_PANNINGSLIDE:
			// swap L/R, convert to fine slide
			if(param & 0xF0)
			{
				param = 0xF0 | min(0x0E, (param >> 4));
			} else
			{
				param = 0x0F | (min(0x0E, param & 0x0F) << 4);
			}

		default:
			break;
		}
	} // End if(oldTypeIsMOD_XM && newTypeIsS3M_IT_MPT)


	/////////////////////////////////////////
	// Convert S3M / IT / MPTM to MOD / XM
	else if(oldTypeIsS3M_IT_MPT && newTypeIsMOD_XM)
	{
		if(note == NOTE_NOTECUT)
		{
			// convert note cut to EC0
			note = NOTE_NONE;
			command = CMD_MODCMDEX;
			param = 0xC0;
		} else if(note == NOTE_FADE)
		{
			// convert note fade to note off
			note = NOTE_KEYOFF;
		}

		switch(command)
		{
		case CMD_S3MCMDEX:
			ExtendedS3MtoMODEffect();
			break;

		case CMD_VOLUMESLIDE:
			if ((param & 0xF0) && ((param & 0x0F) == 0x0F))
			{
				command = CMD_MODCMDEX;
				param = (param >> 4) | 0xA0;
			} else
				if ((param & 0x0F) && ((param & 0xF0) == 0xF0))
				{
					command = CMD_MODCMDEX;
					param = (param & 0x0F) | 0xB0;
				}
				break;

		case CMD_PORTAMENTOUP:
			if (param >= 0xF0)
			{
				command = CMD_MODCMDEX;
				param = (param & 0x0F) | 0x10;
			} else
				if (param >= 0xE0)
				{
					if(newTypeIsXM)
					{
						command = CMD_XFINEPORTAUPDOWN;
						param = 0x10 | (param & 0x0F);
					} else
					{
						command = CMD_MODCMDEX;
						param = (((param & 0x0F) + 3) >> 2) | 0x10;
					}
				} else command = CMD_PORTAMENTOUP;
			break;

		case CMD_PORTAMENTODOWN:
			if (param >= 0xF0)
			{
				command = CMD_MODCMDEX;
				param = (param & 0x0F) | 0x20;
			} else
				if (param >= 0xE0)
				{
					if(newTypeIsXM)
					{
						command = CMD_XFINEPORTAUPDOWN;
						param = 0x20 | (param & 0x0F);
					} else
					{
						command = CMD_MODCMDEX;
						param = (((param & 0x0F) + 3) >> 2) | 0x20;
					}
				} else command = CMD_PORTAMENTODOWN;
			break;

		case CMD_SPEED:
			{
				param = min(param, (toType == MOD_TYPE_XM) ? 0x1F : 0x20);
			}
			break;

		case CMD_TEMPO:
			if(param < 0x20) command = CMD_NONE; // no tempo slides
			break;

		case CMD_PANNINGSLIDE:
			// swap L/R, convert fine slides to normal slides
			if((param & 0x0F) == 0x0F && (param & 0xF0))
			{
				param = (param >> 4);
			} else if((param & 0xF0) == 0xF0 && (param & 0x0F))
			{
				param = (param & 0x0F) << 4;
			} else if(param & 0x0F)
			{
				param = 0xF0;
			} else if(param & 0xF0)
			{
				param = 0x0F;
			} else
			{
				param = 0;
			}
			break;

		case CMD_RETRIG:
			// Retrig: Q0y doesn't change volume in IT/S3M, but R0y in XM takes the last x parameter
			if(param != 0 && (param & 0xF0) == 0)
			{
				param |= 0x80;
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
		if(note == NOTE_KEYOFF || note == NOTE_FADE)
			note = NOTE_NOTECUT;

		switch(command)
		{
		case CMD_S3MCMDEX:
			switch(param & 0xF0)
			{
			case 0x70: command = CMD_NONE; break;	// No NNA / envelope control in S3M format
			case 0x90:
				if(param == 0x91)
				{
					// surround remap (this is the "official" command)
					command = CMD_PANNING8;
					param = 0xA4;
				} else if(param == 0x90)
				{
					command = CMD_PANNING8;
					param = 0x40;
				}
				break;
			}
			break;

		case CMD_GLOBALVOLUME:
			param = (min(0x80, param) + 1) / 2;
			break;

		default:
			break;
		}
	} // End if (oldTypeIsIT_MPT && newTypeIsS3M)

	//////////////////////
	// Convert IT to XM
	if(oldTypeIsIT_MPT && newTypeIsXM)
	{
		switch(command)
		{
		case CMD_GLOBALVOLUME:
			param = (min(0x80, param) + 1) / 2;
			break;
		}
	} // End if(oldTypeIsIT_MPT && newTypeIsXM)

	//////////////////////
	// Convert XM to IT
	if(oldTypeIsXM && newTypeIsIT_MPT)
	{
		switch(command)
		{
		case CMD_GLOBALVOLUME:
			param = min(0x80, param * 2);
			break;
		}
	} // End if(oldTypeIsIT_MPT && newTypeIsXM)

	///////////////////////////////////
	// MOD <-> XM: Speed/Tempo update
	if(oldTypeIsMOD && newTypeIsXM)
	{
		switch(command)
		{
		case CMD_SPEED:
			param = min(param, 0x1F);
			break;
		}
	} else if(oldTypeIsXM && newTypeIsMOD)
	{
		switch(command)
		{
		case CMD_TEMPO:
			param = max(param, 0x21);
			break;
		}
	}


	///////////////////////////////////////////////////////////////////////
	// Convert MOD to anything - adjust effect memory, remove Invert Loop
	if (oldTypeIsMOD)
	{
		switch(command)
		{
		case CMD_TONEPORTAVOL: // lacks memory -> 500 is the same as 300
			if(param == 0x00) command = CMD_TONEPORTAMENTO;
			break;

		case CMD_VIBRATOVOL: // lacks memory -> 600 is the same as 400
			if(param == 0x00) command = CMD_VIBRATO;
			break;

		case CMD_MODCMDEX: // This would turn into "Set Active Macro", so let's better remove it
		case CMD_S3MCMDEX:
			if((param & 0xF0) == 0xF0) command = CMD_NONE;
			break;
		}
	} // End if (oldTypeIsMOD && newTypeIsXM)

	/////////////////////////////////////////////////////////////////////
	// Convert anything to MOD - remove volume column, remove Set Macro
	if (newTypeIsMOD)
	{
		// convert note off events
		if(note >= NOTE_MIN_SPECIAL)
		{
			note = NOTE_NONE;
			// no effect present, so just convert note off to volume 0
			if(command == CMD_NONE)
			{
				command = CMD_VOLUME;
				param = 0;
				// EDx effect present, so convert it to ECx
			} else if((command == CMD_MODCMDEX) && ((param & 0xF0) == 0xD0))
			{
				param = 0xC0 | (param & 0x0F);
			}
		}

		if(command) switch(command)
		{
			case CMD_RETRIG: // MOD only has E9x
				command = CMD_MODCMDEX;
				param = 0x90 | (param & 0x0F);
				break;

			case CMD_MODCMDEX: // This would turn into "Invert Loop", so let's better remove it
				if((param & 0xF0) == 0xF0) command = CMD_NONE;
				break;
		}

		else switch(volcmd)
		{
			case VOLCMD_VOLUME:
				command = CMD_VOLUME;
				param = vol;
				break;

			case VOLCMD_PANNING:
				command = CMD_PANNING8;
				param = CLAMP(vol << 2, 0, 0xFF);
				break;

			case VOLCMD_VOLSLIDEDOWN:
				command = CMD_VOLUMESLIDE;
				param = vol;
				break;

			case VOLCMD_VOLSLIDEUP:
				command = CMD_VOLUMESLIDE;
				param = vol << 4;
				break;

			case VOLCMD_FINEVOLDOWN:
				command = CMD_MODCMDEX;
				param = 0xB0 | vol;
				break;

			case VOLCMD_FINEVOLUP:
				command = CMD_MODCMDEX;
				param = 0xA0 | vol;
				break;

			case VOLCMD_PORTADOWN:
				command = CMD_PORTAMENTODOWN;
				param = vol << 2;
				break;

			case VOLCMD_PORTAUP:
				command = CMD_PORTAMENTOUP;
				param = vol << 2;
				break;

			case VOLCMD_TONEPORTAMENTO:
				command = CMD_TONEPORTAMENTO;
				param = vol << 2;
				break;

			case VOLCMD_VIBRATODEPTH:
				command = CMD_VIBRATO;
				param = vol;
				break;

			case VOLCMD_VIBRATOSPEED:
				command = CMD_VIBRATO;
				param = vol << 4;
				break;
				// OpenMPT-specific commands

			case VOLCMD_OFFSET:
				command = CMD_OFFSET;
				param = vol << 3;
				break;

		}
		volcmd = CMD_NONE;
	} // End if (newTypeIsMOD)

	///////////////////////////////////////////////////
	// Convert anything to S3M - adjust volume column
	if (newTypeIsS3M)
	{
		if(!command) switch(volcmd)
		{
			case VOLCMD_VOLSLIDEDOWN:
				command = CMD_VOLUMESLIDE;
				param = vol;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_VOLSLIDEUP:
				command = CMD_VOLUMESLIDE;
				param = vol << 4;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_FINEVOLDOWN:
				command = CMD_VOLUMESLIDE;
				param = 0xF0 | vol;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_FINEVOLUP:
				command = CMD_VOLUMESLIDE;
				param = (vol << 4) | 0x0F;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_PORTADOWN:
				command = CMD_PORTAMENTODOWN;
				param = vol << 2;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_PORTAUP:
				command = CMD_PORTAMENTOUP;
				param = vol << 2;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_TONEPORTAMENTO:
				command = CMD_TONEPORTAMENTO;
				param = vol << 2;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_VIBRATODEPTH:
				command = CMD_VIBRATO;
				param = vol;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_VIBRATOSPEED:
				command = CMD_VIBRATO;
				param = vol << 4;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_PANSLIDELEFT:
				command = CMD_PANNINGSLIDE;
				param = vol << 4;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_PANSLIDERIGHT:
				command = CMD_PANNINGSLIDE;
				param = vol;
				volcmd = CMD_NONE;
				break;
				// OpenMPT-specific commands

			case VOLCMD_OFFSET:
				command = CMD_OFFSET;
				param = vol << 3;
				volcmd = CMD_NONE;
				break;

		}
	} // End if (newTypeIsS3M)

	////////////////////////////////////////////////////////////////////////
	// Convert anything to XM - adjust volume column, breaking EDx command
	if (newTypeIsXM)
	{
		// remove EDx if no note is next to it, or it will retrigger the note in FT2 mode
		if(command == CMD_MODCMDEX && (param & 0xF0) == 0xD0 && note == NOTE_NONE)
		{
			command = param = 0;
		}

		if(note >= NOTE_MIN_SPECIAL)
		{
			// Instrument numbers next to Note Off reset instrument settings
			instr = 0;

			if(command == CMD_MODCMDEX && (param & 0xF0) == 0xD0)
			{
				// Note Off + Note Delay does nothing when using envelopes.
				note = NOTE_NONE;
				command = CMD_KEYOFF;
				param &= 0x0F;
			}
		}

		if(!command) switch(volcmd)
		{
			case VOLCMD_PORTADOWN:
				command = CMD_PORTAMENTODOWN;
				param = vol << 2;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_PORTAUP:
				command = CMD_PORTAMENTOUP;
				param = vol << 2;
				volcmd = CMD_NONE;
				break;
				// OpenMPT-specific commands

			case VOLCMD_OFFSET:
				command = CMD_OFFSET;
				param = vol << 3;
				volcmd = CMD_NONE;
				break;

		}
	} // End if (newTypeIsXM)

	///////////////////////////////////////////////////
	// Convert anything to IT - adjust volume column
	if (newTypeIsIT_MPT)
	{
		if(!command) switch(volcmd)
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
				vol = min(vol, 9);
				break;

			case VOLCMD_PANSLIDELEFT:
				command = CMD_PANNINGSLIDE;
				param = vol << 4;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_PANSLIDERIGHT:
				command = CMD_PANNINGSLIDE;
				param = vol;
				volcmd = CMD_NONE;
				break;

			case VOLCMD_VIBRATOSPEED:
				command = CMD_VIBRATO;
				param = vol << 4;
				volcmd = CMD_NONE;
				break;

		}
	} // End if (newTypeIsIT)

	if(!CSoundFile::GetModSpecifications(toType).HasNote(note))
		note = NOTE_NONE;

	// ensure the commands really exist in this format
	if(!CSoundFile::GetModSpecifications(toType).HasCommand(command))
		command = CMD_NONE;
	if(!CSoundFile::GetModSpecifications(toType).HasVolCommand(volcmd))
		volcmd = VOLCMD_NONE;

}


// "Importance" of every FX command. Table is used for importing from formats with multiple effect colums
// and is approximately the same as in SchismTracker.
size_t ModCommand::GetEffectWeight(COMMAND cmd)
//---------------------------------------------
{
	// Effect weights, sorted from lowest to highest weight.
	static const COMMAND weights[] =
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


// Try to convert a fx column command (&effect) into a volume column command.
// Returns true if successful.
// Some commands can only be converted by losing some precision.
// If moving the command into the volume column is more important than accuracy, use force = true.
// (Code translated from SchismTracker and mainly supposed to be used with loaders ported from this tracker)
bool ModCommand::ConvertVolEffect(uint8 &effect, uint8 &param, bool force)
//------------------------------------------------------------------------
{
	switch(effect)
	{
	case CMD_NONE:
		return true;
	case CMD_VOLUME:
		effect = VOLCMD_VOLUME;
		param = min(param, 64);
		break;
	case CMD_PORTAMENTOUP:
		// if not force, reject when dividing causes loss of data in LSB, or if the final value is too
		// large to fit. (volume column Ex/Fx are four times stronger than effect column)
		if(!force && ((param & 3) || param > 9 * 4 + 3))
			return false;
		param = min(param / 4, 9);
		effect = VOLCMD_PORTAUP;
		break;
	case CMD_PORTAMENTODOWN:
		if(!force && ((param & 3) || param > 9 * 4 + 3))
			return false;
		param = min(param / 4, 9);
		effect = VOLCMD_PORTADOWN;
		break;
	case CMD_TONEPORTAMENTO:
		if(param >= 0xF0)
		{
			// hack for people who can't type F twice :)
			effect = VOLCMD_TONEPORTAMENTO;
			param = 9;
			return true;
		}
		for(uint8 n = 0; n < 10; n++)
		{
			if(force
				? (param <= ImpulseTrackerPortaVolCmd[n])
				: (param == ImpulseTrackerPortaVolCmd[n]))
			{
				effect = VOLCMD_TONEPORTAMENTO;
				param = n;
				return true;
			}
		}
		return false;
	case CMD_VIBRATO:
		if(force)
			param = min(param, 9);
		else if(param > 9)
			return false;
		effect = VOLCMD_VIBRATODEPTH;
		break;
	case CMD_FINEVIBRATO:
		if(force)
			param = 0;
		else if(param)
			return false;
		effect = VOLCMD_VIBRATODEPTH;
		break;
	case CMD_PANNING8:
		param = min(64, param * 64 / 255);
		effect = VOLCMD_PANNING;
		break;
	case CMD_VOLUMESLIDE:
		if(param == 0)
			return false;
		if((param & 0xF) == 0)	// Dx0 / Cx
		{
			if(force)
				param = min(param >> 4, 9);
			else if((param >> 4) > 9)
				return false;
			else
				param >>= 4;
			effect = VOLCMD_VOLSLIDEUP;
		} else if((param & 0xF0) == 0)	// D0x / Dx
		{
			if(force)
				param = min(param, 9);
			else if(param > 9)
				return false;
			effect = VOLCMD_VOLSLIDEDOWN;
		} else if((param & 0xF) == 0xF)	// DxF / Ax
		{
			if(force)
				param = min(param >> 4, 9);
			else if((param >> 4) > 9)
				return false;
			else
				param >>= 4;
			effect = VOLCMD_FINEVOLUP;
		} else if((param & 0xf0) == 0xf0)	// DFx / Bx
		{
			if(force)
				param = min(param, 9);
			else if((param & 0xF) > 9)
				return false;
			else
				param &= 0xF;
			effect = VOLCMD_FINEVOLDOWN;
		} else // ???
		{
			return false;
		}
		break;
	case CMD_S3MCMDEX:
		switch (param >> 4)
		{
		case 8:
			effect = VOLCMD_PANNING;
			param = ((param & 0xf) << 2) + 2;
			return true;
		case 0: case 1: case 2: case 0xF:
			if(force)
			{
				effect = param = 0;
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