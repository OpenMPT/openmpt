#include <stdafx.h>
#include "mod_specifications.h"


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
	}
	return false;
}


