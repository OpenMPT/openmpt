#include <stdafx.h>
#include "mod_specifications.h"
#include "..\mptrack\misc_util.h"


MODTYPE CModSpecifications::ExtensionToType(LPCTSTR pszExt)
//---------------------------------------------------------
{
	if (pszExt == nullptr)
		return MOD_TYPE_NONE;
	if (pszExt[0] == '.')
		pszExt++;
	char szExtA[CountOf(ModSpecs::mod.fileExtension)];
	MemsetZero(szExtA);
	size_t i = 0;
	const size_t nLength = _tcslen(pszExt);
	if (nLength >= CountOf(szExtA))
		return MOD_TYPE_NONE;
	for(i = 0; i<nLength; ++i)
		szExtA[i] = static_cast<char>(pszExt[i]);
	if (!lstrcmpiA(szExtA, ModSpecs::mod.fileExtension) || !lstrcmpiA(szExtA, ModSpecs::modEx.fileExtension))
		return MOD_TYPE_MOD;
	else if (!lstrcmpiA(szExtA, ModSpecs::s3m.fileExtension) || !lstrcmpiA(szExtA, ModSpecs::s3mEx.fileExtension))
		return MOD_TYPE_S3M;
	else if (!lstrcmpiA(szExtA, ModSpecs::xm.fileExtension) || !lstrcmpiA(szExtA, ModSpecs::xmEx.fileExtension))
		return MOD_TYPE_XM;
	else if (!lstrcmpiA(szExtA, ModSpecs::it.fileExtension) || !lstrcmpiA(szExtA, ModSpecs::itEx.fileExtension)
			 || !lstrcmpi(szExtA, _T("itp")))
		return MOD_TYPE_IT;
	else if (!lstrcmpiA(szExtA, ModSpecs::mptm.fileExtension))
		return MOD_TYPE_MPT;
	else
		return MOD_TYPE_NONE;
}


bool CModSpecifications::HasNote(MODCOMMAND::NOTE note) const
//------------------------------------------------------------
{
	if(note >= noteMin && note <= noteMax)
		return true;
	else if(note >= NOTE_MIN_SPECIAL && note <= NOTE_MAX_SPECIAL)
	{
		if(note == NOTE_NOTECUT)
			return hasNoteCut;
		else if(note == NOTE_KEYOFF)
			return hasNoteOff;
		else if(note == NOTE_FADE)
			return hasNoteFade;
		else
			return (memcmp(fileExtension, ModSpecs::mptm.fileExtension, 4) == 0);
	} else if(note == NOTE_NONE)
		return true;
	return false;
}


bool CModSpecifications::HasVolCommand(MODCOMMAND::VOLCMD volcmd) const
//---------------------------------------------------------------------
{
	if(volcmd >= MAX_VOLCMDS) return false;
	if(volcommands[volcmd] == '?') return false;
	return true;
}


bool CModSpecifications::HasCommand(MODCOMMAND::COMMAND cmd) const
//----------------------------------------------------------------
{
	if(cmd >= MAX_EFFECTS) return false;
	if(commands[cmd] == '?') return false;
	return true;
}


char CModSpecifications::GetVolEffectLetter(MODCOMMAND::VOLCMD volcmd) const
//--------------------------------------------------------------------------
{
	if(volcmd >= MAX_VOLCMDS) return '?';
	return volcommands[volcmd];
}


char CModSpecifications::GetEffectLetter(MODCOMMAND::COMMAND cmd) const
//---------------------------------------------------------------------
{
	if(cmd >= MAX_EFFECTS) return '?';
	return commands[cmd];
}
