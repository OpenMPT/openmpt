#ifndef MODCOMMAND_H
#define MODCOMMAND_H


// Note definitions
#define NOTE_NONE			0
#define NOTE_MIDDLEC		(5*12+1)
#define NOTE_KEYOFF			0xFF //255
#define NOTE_NOTECUT		0xFE //254
#define NOTE_FADE			0xFD //253, IT's action for illegal notes - DO NOT SAVE AS 253 as this is IT's internal representation of "no note"!
#define NOTE_PC				0xFC //252, Param Control 'note'. Changes param value on first tick.
#define NOTE_PCS			0xFB //251, Param Control(Smooth) 'note'. Changes param value during the whole row.
#define NOTE_MIN			1
#define NOTE_MAX			120  //Defines maximum notevalue(with index starting from 1) as well as maximum number of notes.
#define NOTE_MAX_SPECIAL	NOTE_KEYOFF
#define NOTE_MIN_SPECIAL	NOTE_PCS


//==============
class MODCOMMAND
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
	enum {maxColumnValue = 999};

	// Returns empty modcommand.
	static MODCOMMAND Empty() {MODCOMMAND m = {0,0,0,0,0,0}; return m;}

	bool operator==(const MODCOMMAND& mc) const { return (memcmp(this, &mc, sizeof(MODCOMMAND)) == 0); }
	bool operator!=(const MODCOMMAND& mc) const { return !(*this == mc); }

	void Set(NOTE n, INSTR ins, uint16 volcol, uint16 effectcol) {note = n; instr = ins; SetValueVolCol(volcol); SetValueEffectCol(effectcol);}

	uint16 GetValueVolCol() const {return GetValueVolCol(volcmd, vol);}
	static uint16 GetValueVolCol(BYTE volcmd, BYTE vol) {return (volcmd << 8) + vol;}
	void SetValueVolCol(const uint16 val) {volcmd = static_cast<BYTE>(val >> 8); vol = static_cast<BYTE>(val & 0xFF);}

	uint16 GetValueEffectCol() const {return GetValueEffectCol(command, param);}
	static uint16 GetValueEffectCol(BYTE command, BYTE param) {return (command << 8) + param;}
	void SetValueEffectCol(const uint16 val) {command = static_cast<BYTE>(val >> 8); param = static_cast<BYTE>(val & 0xFF);}

	// Clears modcommand.
	void Clear() {memset(this, 0, sizeof(MODCOMMAND));}

	// Returns true if modcommand is empty, false otherwise.
	// If ignoreEffectValues is true (default), effect values are ignored are ignored if there is no effect command present.
	bool IsEmpty(bool ignoreEffectValues = true) const
	{
		if(ignoreEffectValues)
			return (this->note == 0 && this->instr == 0 && this->volcmd == 0 && this->command == 0);
		else
			return (*this == Empty());
	}

	// Returns true if instrument column represents plugin index.
	bool IsInstrPlug() const {return IsPcNote();}

	// Returns true if and only if note is NOTE_PC or NOTE_PCS.
	bool IsPcNote() const { return note == NOTE_PC || note == NOTE_PCS; }
	static bool IsPcNote(NOTE note_id) { return note_id == NOTE_PC || note_id == NOTE_PCS; }

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

typedef MODCOMMAND* LPMODCOMMAND;
typedef MODCOMMAND MODCOMMAND_ORIGINAL;


// Volume Column commands
#define VOLCMD_NONE				0
#define VOLCMD_VOLUME			1
#define VOLCMD_PANNING			2
#define VOLCMD_VOLSLIDEUP		3
#define VOLCMD_VOLSLIDEDOWN		4
#define VOLCMD_FINEVOLUP		5
#define VOLCMD_FINEVOLDOWN		6
#define VOLCMD_VIBRATOSPEED		7
#define VOLCMD_VIBRATODEPTH		8
#define VOLCMD_PANSLIDELEFT		9
#define VOLCMD_PANSLIDERIGHT	10
#define VOLCMD_TONEPORTAMENTO	11
#define VOLCMD_PORTAUP			12
#define VOLCMD_PORTADOWN		13
#define VOLCMD_DELAYCUT			14 //currently unused
#define VOLCMD_OFFSET			15 //rewbs.volOff
#define MAX_VOLCMDS				16


// Effect column commands
#define CMD_NONE				0
#define CMD_ARPEGGIO			1
#define CMD_PORTAMENTOUP		2
#define CMD_PORTAMENTODOWN		3
#define CMD_TONEPORTAMENTO		4
#define CMD_VIBRATO				5
#define CMD_TONEPORTAVOL		6
#define CMD_VIBRATOVOL			7
#define CMD_TREMOLO				8
#define CMD_PANNING8			9
#define CMD_OFFSET				10
#define CMD_VOLUMESLIDE			11
#define CMD_POSITIONJUMP		12
#define CMD_VOLUME				13
#define CMD_PATTERNBREAK		14
#define CMD_RETRIG				15
#define CMD_SPEED				16
#define CMD_TEMPO				17
#define CMD_TREMOR				18
#define CMD_MODCMDEX			19
#define CMD_S3MCMDEX			20
#define CMD_CHANNELVOLUME		21
#define CMD_CHANNELVOLSLIDE		22
#define CMD_GLOBALVOLUME		23
#define CMD_GLOBALVOLSLIDE		24
#define CMD_KEYOFF				25
#define CMD_FINEVIBRATO			26
#define CMD_PANBRELLO			27
#define CMD_XFINEPORTAUPDOWN	28
#define CMD_PANNINGSLIDE		29
#define CMD_SETENVPOSITION		30
#define CMD_MIDI				31
#define CMD_SMOOTHMIDI			32 //rewbs.smoothVST
#define CMD_DELAYCUT			33
#define CMD_XPARAM				34 // -> CODE#0010 -> DESC="add extended parameter mechanism to pattern effects" -! NEW_FEATURE#0010
#define CMD_NOTESLIDEUP         35 // IMF Gxy
#define CMD_NOTESLIDEDOWN       36 // IMF Hxy
#define MAX_EFFECTS				37


#endif
