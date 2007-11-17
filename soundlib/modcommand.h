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


#endif
