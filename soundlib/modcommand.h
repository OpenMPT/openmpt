#ifndef MODCOMMAND_H
#define MODCOMMAND_H

typedef BYTE NOTE;
typedef BYTE INSTR_NUM;
typedef BYTE VOLCMD;
typedef BYTE VOLPARAM;
typedef BYTE EFFECT_ID;
typedef BYTE EFFECT_PARAM;
typedef BYTE EFFECTINDEX;


//==============
class MODCOMMAND
//==============
{
public:
	//NOTE: Default constructor must construct empty modcommand.
	MODCOMMAND() : 
		note(0),
		instr(0),
		volcmd(0),
		command(0),
		vol(0),
		param(0)
		{}

	inline bool operator==(const MODCOMMAND& mc) const
	{
		return (memcmp(this, &mc, sizeof(MODCOMMAND)) == 0);
	}

	inline bool operator !=(const MODCOMMAND& mc) const
	{
		return !(*this == mc);
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


//Effect command IDs
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
#define CMD_VELOCITY			33 //rewbs.velocity
#define CMD_XPARAM				34 //Extended parameter mechanism to pattern effects

// Volume Column command IDs
#define VOLCMD_VOLUME			1
#define VOLCMD_PANNING			2
#define VOLCMD_VOLSLIDEUP		3
#define VOLCMD_VOLSLIDEDOWN		4
#define VOLCMD_FINEVOLUP		5
#define VOLCMD_FINEVOLDOWN		6
#define VOLCMD_VIBRATOSPEED		7
#define VOLCMD_VIBRATO			8
#define VOLCMD_PANSLIDELEFT		9
#define VOLCMD_PANSLIDERIGHT	10
#define VOLCMD_TONEPORTAMENTO	11
#define VOLCMD_PORTAUP			12
#define VOLCMD_PORTADOWN		13
#define VOLCMD_VELOCITY			14 //rewbs.velocity
#define VOLCMD_OFFSET			15 //rewbs.volOff


// Note definitions
#define NOTE_MIDDLEC		(5*12+1)
#define NOTE_KEYOFF			0xFF //255
#define NOTE_NOTECUT		0xFE //254
#define NOTE_PC				0xFD //253, Param Control 'note'. Changes param value on first tick.
#define NOTE_PCS			0xFC //252,  Param Control(Smooth) 'note'. Changes param value during the whole row.
#define NOTE_MAX			120 //Defines maximum notevalue as well as maximum number of notes.


#endif
