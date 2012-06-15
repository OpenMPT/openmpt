/*
 * ModCommand.h
 * ------------
 * Purpose: Moduel Command (pattern content) header class and helpers. One Module Command corresponds to one pattern cell.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "Snd_defs.h"

// Note definitions
#define NOTE_NONE			0
#define NOTE_MIN			1
#define NOTE_MAX			120  // Defines maximum notevalue(with index starting from 1) as well as maximum number of notes.
#define NOTE_MIDDLEC		(5 * 12 + NOTE_MIN)
#define NOTE_KEYOFF			0xFF // 255
#define NOTE_NOTECUT		0xFE // 254
#define NOTE_FADE			0xFD // 253, IT's action for illegal notes - DO NOT SAVE AS 253 as this is IT's internal representation of "no note"!
#define NOTE_PC				0xFC // 252, Param Control 'note'. Changes param value on first tick.
#define NOTE_PCS			0xFB // 251, Param Control (Smooth) 'note'. Changes param value during the whole row.
#define NOTE_MAX_SPECIAL	NOTE_KEYOFF
#define NOTE_MIN_SPECIAL	NOTE_PCS


//==============
class ModCommand
//==============
{
public:
	typedef BYTE NOTE;
	typedef BYTE INSTR;
	typedef BYTE VOL;
	typedef BYTE VOLCMD;
	typedef BYTE COMMAND;
	typedef BYTE PARAM;

	// Defines the maximum value for column data when interpreted as 2-byte value
	// (for example volcmd and vol). The valid value range is [0, maxColumnValue].
	enum { maxColumnValue = 999 };

	// Returns empty modcommand.
	static ModCommand Empty() { ModCommand m = { 0, 0, 0, 0, 0, 0 }; return m; }

	bool operator==(const ModCommand& mc) const { return (memcmp(this, &mc, sizeof(ModCommand)) == 0); }
	bool operator!=(const ModCommand& mc) const { return !(*this == mc); }

	void Set(NOTE n, INSTR ins, uint16 volcol, uint16 effectcol) { note = n; instr = ins; SetValueVolCol(volcol); SetValueEffectCol(effectcol); }

	uint16 GetValueVolCol() const { return GetValueVolCol(volcmd, vol); }
	static uint16 GetValueVolCol(BYTE volcmd, BYTE vol) { return (volcmd << 8) + vol; }
	void SetValueVolCol(const uint16 val) { volcmd = static_cast<BYTE>(val >> 8); vol = static_cast<BYTE>(val & 0xFF); }

	uint16 GetValueEffectCol() const { return GetValueEffectCol(command, param); }
	static uint16 GetValueEffectCol(BYTE command, BYTE param) { return (command << 8) + param; }
	void SetValueEffectCol(const uint16 val) { command = static_cast<BYTE>(val >> 8); param = static_cast<BYTE>(val & 0xFF); }

	// Clears modcommand.
	void Clear() { memset(this, 0, sizeof(ModCommand)); }

	// Returns true if modcommand is empty, false otherwise.
	// If ignoreEffectValues is true (default), effect values are ignored are ignored if there is no effect command present.
	bool IsEmpty(const bool ignoreEffectValues = true) const
	{
		if(ignoreEffectValues)
			return (this->note == 0 && this->instr == 0 && this->volcmd == 0 && this->command == 0);
		else
			return (*this == Empty());
	}

	// Returns true if instrument column represents plugin index.
	bool IsInstrPlug() const { return IsPcNote(); }

	// Returns true if and only if note is NOTE_PC or NOTE_PCS.
	bool IsPcNote() const { return note == NOTE_PC || note == NOTE_PCS; }
	static bool IsPcNote(const NOTE note_id) { return note_id == NOTE_PC || note_id == NOTE_PCS; }

	// Returns true if and only if note is a valid musical note.
	bool IsNote() const { return note >= NOTE_MIN && note <= NOTE_MAX; }
	static bool IsNote(NOTE note) { return note >= NOTE_MIN && note <= NOTE_MAX; }
	// Returns true if and only if note is a valid musical note or the note entry is empty.
	bool IsNoteOrEmpty() const { return note == NOTE_NONE || IsNote(); }
	static bool IsNoteOrEmpty(NOTE note) { return note == NOTE_NONE || IsNote(note); }

	// Convert a complete ModCommand item from one format to another
	void Convert(MODTYPE fromType, MODTYPE toType);
	// Convert MOD/XM Exx to S3M/IT Sxx
	void ExtendedMODtoS3MEffect();
	// Convert S3M/IT Sxx to MOD/XM Exx
	void ExtendedS3MtoMODEffect();

	// "Importance" of every FX command. Table is used for importing from formats with multiple effect columns
	// and is approximately the same as in SchismTracker.
	static size_t GetEffectWeight(COMMAND cmd);
	// try to convert a an effect into a volume column effect.
	static bool ConvertVolEffect(uint8 &effect, uint8 &param, bool force);

	// Swap volume and effect column (doesn't do any conversion as it's mainly for importing formats with multiple effect columns, so beware!)
	void SwapEffects()
	{
		std::swap(volcmd, command);
		std::swap(vol, param);
	}

public:
	BYTE note;
	BYTE instr;
	BYTE volcmd;
	BYTE command;
	BYTE vol;
	BYTE param;
};

typedef ModCommand MODCOMMAND_ORIGINAL;


// Volume Column commands
enum VolumeCommands
{
	VOLCMD_NONE				= 0,
	VOLCMD_VOLUME			= 1,
	VOLCMD_PANNING			= 2,
	VOLCMD_VOLSLIDEUP		= 3,
	VOLCMD_VOLSLIDEDOWN		= 4,
	VOLCMD_FINEVOLUP		= 5,
	VOLCMD_FINEVOLDOWN		= 6,
	VOLCMD_VIBRATOSPEED		= 7,
	VOLCMD_VIBRATODEPTH		= 8,
	VOLCMD_PANSLIDELEFT		= 9,
	VOLCMD_PANSLIDERIGHT	= 10,
	VOLCMD_TONEPORTAMENTO	= 11,
	VOLCMD_PORTAUP			= 12,
	VOLCMD_PORTADOWN		= 13,
	VOLCMD_DELAYCUT			= 14, //currently unused
	VOLCMD_OFFSET			= 15, //rewbs.volOff
	MAX_VOLCMDS				= 16
};


// Effect column commands
enum EffectCommands
{
	CMD_NONE				= 0,
	CMD_ARPEGGIO			= 1,
	CMD_PORTAMENTOUP		= 2,
	CMD_PORTAMENTODOWN		= 3,
	CMD_TONEPORTAMENTO		= 4,
	CMD_VIBRATO				= 5,
	CMD_TONEPORTAVOL		= 6,
	CMD_VIBRATOVOL			= 7,
	CMD_TREMOLO				= 8,
	CMD_PANNING8			= 9,
	CMD_OFFSET				= 10,
	CMD_VOLUMESLIDE			= 11,
	CMD_POSITIONJUMP		= 12,
	CMD_VOLUME				= 13,
	CMD_PATTERNBREAK		= 14,
	CMD_RETRIG				= 15,
	CMD_SPEED				= 16,
	CMD_TEMPO				= 17,
	CMD_TREMOR				= 18,
	CMD_MODCMDEX			= 19,
	CMD_S3MCMDEX			= 20,
	CMD_CHANNELVOLUME		= 21,
	CMD_CHANNELVOLSLIDE		= 22,
	CMD_GLOBALVOLUME		= 23,
	CMD_GLOBALVOLSLIDE		= 24,
	CMD_KEYOFF				= 25,
	CMD_FINEVIBRATO			= 26,
	CMD_PANBRELLO			= 27,
	CMD_XFINEPORTAUPDOWN	= 28,
	CMD_PANNINGSLIDE		= 29,
	CMD_SETENVPOSITION		= 30,
	CMD_MIDI				= 31,
	CMD_SMOOTHMIDI			= 32, //rewbs.smoothVST
	CMD_DELAYCUT			= 33,
	CMD_XPARAM				= 34, // -> CODE#0010 -> DESC="add extended parameter mechanism to pattern effects" -! NEW_FEATURE#0010
	CMD_NOTESLIDEUP			= 35, // IMF Gxy
	CMD_NOTESLIDEDOWN		= 36, // IMF Hxy
	MAX_EFFECTS				= 37
};
