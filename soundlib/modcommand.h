#ifndef MODCOMMAND_H
#define MODCOMMAND_H

#include <vector>
using std::vector;

//#undef TRADITIONAL_MODCOMMAND
#define TRADITIONAL_MODCOMMAND

typedef BYTE NOTE;
typedef BYTE INSTR_NUM;
typedef BYTE VOLCMD;
typedef BYTE VOLPARAM;
typedef BYTE EFFECT_ID;
typedef BYTE EFFECT_PARAM;
typedef BYTE EFFECTINDEX;

#ifdef TRADITIONAL_MODCOMMAND
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

#else //Some hacking to explore the way for multiple effects per command.

	//===============
	class CModCommand
	//===============
	{

	public: //Constructors, operators
		CModCommand() : 
			note(0),
			instr(0),
			volcmd(0),
			vol(0),
			command(0),
			param(0),
			m_Effect1(0),
			m_Effect1Param(0)
		{}

		bool operator==(const CModCommand& mc) const
		{
			return (GetNote() == mc.GetNote() &&
				GetInstr() == mc.GetInstr() &&
				GetVolCmd() == mc.GetVolCmd() &&
				GetVolParam() == mc.GetVolParam() &&
				GetEffect() == mc.GetEffect() &&
				GetEffectParam() == mc.GetEffectParam());
		}

		bool operator!=(const CModCommand& mc) const
		{
			return !(*this == mc);
		}

	public:

		inline NOTE GetNote() const {return note;}
		inline NOTE SetNote(const NOTE n) {note = n; return note;}

		inline INSTR_NUM GetInstr() const {return instr;}
		inline INSTR_NUM SetInstr(const INSTR_NUM i) {instr = i; return instr;}

		inline VOLCMD GetVolCmd() const {return volcmd;}
		inline VOLCMD SetVolCmd(const VOLCMD vc) {volcmd = vc; return volcmd;}

		inline VOLPARAM GetVolParam() const {return vol;}
		inline VOLPARAM SetVolParam(const VOLPARAM vp) {vol = vp; return vol;}

		inline EFFECTINDEX GetNumEffects() const
		{
			if(GetEffect(1))
				return 2;
			else
				if(GetEffect(0))
					return 1;
				else
					return 0;
		}

		inline EFFECT_ID GetEffect() const {return command;}
		inline EFFECT_ID GetEffect(EFFECTINDEX i) const {if(i>=2) return 0; else return (i == 1) ? m_Effect1 : command;}
		inline EFFECT_ID SetEffectByID(const EFFECT_ID es) {command = es; return command;}

		inline EFFECT_PARAM GetEffectParam() const {return param;}
		inline EFFECT_PARAM GetEffectParam(EFFECTINDEX i)
		{
			if(i == 1) return m_Effect1Param;
			else {if (i == 0) return command; else return 0;}
		}
		inline EFFECT_PARAM SetEffectParam(const EFFECT_PARAM ep) {param = ep; return param;}

		
	public: //mimic old MODCOMMAND
		NOTE note;
		INSTR_NUM instr;
		VOLCMD volcmd;
		VOLPARAM vol;
		EFFECT_ID command;
		EFFECT_PARAM param;

		EFFECT_ID m_Effect1;
		EFFECT_PARAM m_Effect1Param;
	};

	typedef CModCommand MODCOMMAND;
	typedef MODCOMMAND* LPMODCOMMAND;

	//========================
	struct MODCOMMAND_ORIGINAL
	//========================
	{
		BYTE note;
		BYTE instr;
		BYTE volcmd;
		BYTE command;
		BYTE vol;
		BYTE param;
	};

#endif


#endif
