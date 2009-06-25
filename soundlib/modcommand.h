#ifndef MODCOMMAND_H
#define MODCOMMAND_H


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

	inline bool operator==(const MODCOMMAND& mc) const
	{
		return (memcmp(this, &mc, sizeof(MODCOMMAND)) == 0);
	}

	inline bool operator !=(const MODCOMMAND& mc) const
	{
		return !(*this == mc);
	}

	uint16 GetValueVolCol() const {return GetValueVolCol(volcmd, vol);}
	static uint16 GetValueVolCol(BYTE volcmd, BYTE vol) {return (volcmd << 8) + vol;}
	void SetValueVolCol(const uint16 val) {volcmd = static_cast<BYTE>(val >> 8); vol = static_cast<BYTE>(val & 0xFF);}

	uint16 GetValueEffectCol() const {return GetValueEffectCol(command, param);}
	static uint16 GetValueEffectCol(BYTE command, BYTE param) {return (command << 8) + param;}
	void SetValueEffectCol(const uint16 val) {command = static_cast<BYTE>(val >> 8); param = static_cast<BYTE>(val & 0xFF);}

	// Clears modcommand.
	void Clear() {memset(this, 0, sizeof(MODCOMMAND));}

	// Returns true if modcommand is empty, false otherwise.
	inline bool MODCOMMAND::IsEmpty() const {return (*this == Empty());}

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


// Note definitions
#define NOTE_MIDDLEC		(5*12+1)
#define NOTE_KEYOFF			0xFF //255
#define NOTE_NOTECUT		0xFE //254
#define NOTE_PC				0xFD //253, Param Control 'note'. Changes param value on first tick.
#define NOTE_PCS			0xFC //252,  Param Control(Smooth) 'note'. Changes param value during the whole row.
#define NOTE_MAX			120  //Defines maximum notevalue(with index starting from 1) as well as maximum number of notes.
#define NOTE_MAX_SPECIAL	NOTE_KEYOFF
#define NOTE_MIN_SPECIAL	NOTE_PCS


#endif
