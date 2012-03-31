/*
 * PatternClipboard.cpp
 * --------------------
 * Purpose: Implementation of the pattern clipboard mechanism
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "PatternClipboard.h"
#include "Mainfrm.h"
#include "Moddoc.h"

// Currently just one slot for fall-back pasting.
PatternClipboard::clipindex_t PatternClipboard::clipboardSize = 1;


// Clipboard format:
// Hdr: "ModPlug Tracker S3M\r\n"
// Full:  '|C#401v64A06'
// Reset: '|...........'
// Empty: '|           '
// End of row: '\n'

CString PatternClipboard::GetFileExtension(const char *ext) const
//---------------------------------------------------------------
{
	CString format(ext);
	if(format.GetLength() > 3)
	{
		format.Truncate(3);
	}
	format.MakeUpper();
	while(format.GetLength() < 3)
	{
		format = " " + format;
	}
	return format;
}


// Copy a pattern selection to both the system clipboard and the internal clipboard.
bool PatternClipboard::Copy(CSoundFile &sndFile, PATTERNINDEX pattern, PatternRect selection)
//-------------------------------------------------------------------------------------------
{
	if(!sndFile.Patterns.IsValidPat(pattern))
	{
		return false;
	}

	if(selection.GetStartColumn() == PatternCursor::paramColumn)
	{
		// Special case: If selection starts with a parameter column, extend it to include the effect letter as well.
		PatternCursor upper(selection.GetUpperLeft());
		upper.Move(0, 0, -1);
		selection = PatternRect(upper, selection.GetLowerRight());
	}

	const ROWINDEX startRow = selection.GetStartRow(), numRows = selection.GetNumRows();
	const CHANNELINDEX startChan = selection.GetStartChannel(), numChans = selection.GetNumChannels();

	// XXX
	size_t memSize = 21 + numRows * (numChans * 12 + 2) + 1;

	CString data;
	data.Preallocate(memSize);

	// Set up clipboard header.
	// XXX
	data = "ModPlug Tracker " + GetFileExtension(sndFile.GetModSpecifications().fileExtension) + "\r\n";

	for(ROWINDEX row = 0; row < numRows; row++)
	{
		if(row + startRow >= sndFile.Patterns[pattern].GetNumRows())
		{
			break;
		}

		const ModCommand *m = sndFile.Patterns[pattern].GetpModCommand(row + startRow, startChan);

		for(CHANNELINDEX chan = 0; chan < numChans; chan++, m++)
		{
			PatternCursor cursor(0, startChan + chan);
			data.AppendChar('|');

			// Note
			if(selection.ContainsHorizontal(cursor))
			{
				switch(m->note)
				{
				case NOTE_NONE:		data.Append("..."); break;
				case NOTE_KEYOFF:	data.Append("==="); break;
				case NOTE_NOTECUT:	data.Append("^^^"); break;
				case NOTE_FADE:		data.Append("~~~"); break;
				case NOTE_PC:		data.Append("PC "); break;
				case NOTE_PCS:		data.Append("PCS"); break;
				default:			if(m->IsNote()) data.Append(szDefaultNoteNames[m->note - NOTE_MIN]);
									else data.Append("...");
				}
			} else
			{
				// No note
				data.Append("   ");
			}

			// Instrument
			cursor.Move(0, 0, 1);
			if(selection.ContainsHorizontal(cursor))
			{
				if(m->instr)
				{
					data.AppendChar('0' + (m->instr / 10));
					data.AppendChar('0' + (m->instr % 10));
				} else
				{
					data.Append("..");
				}
			} else
			{
				data.Append("  ");
			}

			// Volume
			cursor.Move(0, 0, 1);
			if(selection.ContainsHorizontal(cursor))
			{
				if(m->IsPcNote())
				{
					data.AppendFormat("%03u", m->GetValueVolCol());
				}
				else
				{
					if(m->volcmd != VOLCMD_NONE && m->vol <= 99)
					{
						data.AppendChar(sndFile.GetModSpecifications().GetVolEffectLetter(m->volcmd));
						data.AppendFormat("%02u", m->vol);
					} else
					{
						data.Append("...");
					}
				}
			} else
			{
				data.Append("   ");
			}
			
			// Effect
			cursor.Move(0, 0, 1);
			if(selection.ContainsHorizontal(cursor))
			{
				if(m->IsPcNote())
				{
					data.AppendFormat("%03u", m->GetValueEffectCol());
				}
				else
				{
					if(m->command != CMD_NONE)
					{
						data.AppendChar(sndFile.GetModSpecifications().GetEffectLetter(m->command));
					} else
					{
						data.AppendChar('.');
					}

					if(m->param != 0 && m->param <= 0xFF)
					{
						data.AppendFormat("%02X", m->param);
					} else
					{
						data.Append("..");
					}
				}
			} else
			{
				data.Append("   ");
			}
		}

		// Next Row
		data.Append("\r\n");
	}

	activeClipboard = 0;

	if(clipboardSize > 0)
	{
		// Copy to internal clipboard
		if(clipboards.size() == clipboardSize)
		{
			clipboards.pop_back();
		}

		clipboards.push_front(PatternClipboardElement(data));
	}

	return ToSystemClipboard(data);

}


// Try pasting a pattern selection from the system clipboard.
bool PatternClipboard::Paste(CSoundFile &sndFile, PATTERNINDEX pattern, const PatternCursor &pastePos, PasteModes mode)
//---------------------------------------------------------------------------------------------------------------------
{
	CString data;
	if(!FromSystemClipboard(data) || !HandlePaste(sndFile, pattern, pastePos, mode, data))
	{
		// Fall back to internal clipboard if there's no valid pattern data in the system clipboard.
		return Paste(sndFile, pattern, pastePos, mode, 0);
	}
	return true;
}


// Try pasting a pattern selection from an internal clipboard.
bool PatternClipboard::Paste(CSoundFile &sndFile, PATTERNINDEX pattern, const PatternCursor &pastePos, PasteModes mode, clipindex_t internalClipboard)
//----------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(internalClipboard >= clipboards.size())
	{
		return false;
	}
	return HandlePaste(sndFile, pattern, pastePos, mode, clipboards[internalClipboard].content);
}


// Perform the pasting operation.
bool PatternClipboard::HandlePaste(CSoundFile &sndFile, PATTERNINDEX pattern, const PatternCursor &pastePos, PasteModes mode, const CString &data)
//------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(!sndFile.Patterns.IsValidPat(pattern) || sndFile.GetpModDoc() == nullptr)
	{
		return false;
	}

	CModDoc &modDoc = *(sndFile.GetpModDoc());

	const TEMPO tempoMin = sndFile.GetModSpecifications().tempoMin;

	CHANNELINDEX startChan = pastePos.GetChannel(), col;
	ROWINDEX startRow = pastePos.GetRow();
	ROWINDEX curRow = startRow;
	bool success = false;
	bool prepareUndo = true;	// prepare pattern for undo next time
	bool firstUndo = true;		// for chaining undos (see overflow paste)

	const bool overflowPaste = (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_OVERFLOWPASTE) && (mode != pmPasteFlood) && (mode != pmPushForward);
	const bool doITStyleMix = (mode == pmMixPasteIT);
	const bool doMixPaste = (mode == pmMixPaste) || doITStyleMix;

	// We need to know the current order for overflow paste
	ORDERINDEX currentOrder = 0;
	if(overflowPaste)
	{
		ROWINDEX rTemp;
		PATTERNINDEX pTemp;
		modDoc.GetEditPosition(rTemp, pTemp, currentOrder);
	}

	// Can we actually paste at this position?
	if(startRow >= sndFile.Patterns[pattern].GetNumRows() || startChan >= sndFile.GetNumChannels())
	{
		return false;
	}

	// Search for signature
	int pos, startPos = 0;
	MODTYPE pasteFormat = MOD_TYPE_NONE;
	while(pasteFormat == MOD_TYPE_NONE && (startPos = data.Find("ModPlug Tracker ", startPos)) != -1)
	{
		startPos += 16;
		// Check paste format
		const CString format = data.Mid(startPos, 3);

		for(size_t i = 0; i < CountOf(ModSpecs::Collection); i++)
		{
			const CString ext = GetFileExtension(ModSpecs::Collection[i]->fileExtension);
			if(format == ext)
			{
				pasteFormat = ModSpecs::Collection[i]->internalType;
				startPos += 3;
				break;
			}
		}
	}

	if(startPos == -1)
	{
		// What is this I don't even
		return false;
	}

	const CModSpecifications &sourceSpecs = CSoundFile::GetModSpecifications(pasteFormat);
	const bool clipboardHasS3MCommands = (pasteFormat & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_S3M)) != 0;

	ModCommand *m = sndFile.Patterns[pattern].GetpModCommand(startRow, 0);

	pos = startPos;
	while(curRow < sndFile.Patterns[pattern].GetNumRows())
	{
		// Search for column separator or end of paste data.
		if((pos = data.Find('|', pos)) == -1)
		{
			// End of paste
			if(mode == pmPasteFlood && curRow != startRow)
			{
				// Restarting pasting from beginning.
				pos = startPos;
				continue;
			}
			else
			{
				// Prevent infinite loop with malformed clipboard data.
				break;
			}
		}

		success = true;
		col = startChan;
		// Paste columns
		while((data[pos] == '|') && (pos + 11 < data.GetLength()))
		{
			pos++;

			// Check valid paste condition. Paste will be skipped if
			// - col is not a valid channelindex or
			// - doing mix paste and paste destination modcommand is a PCnote or
			// - doing mix paste and trying to paste PCnote on non-empty modcommand.
			const bool skipPaste =
				col >= sndFile.GetNumChannels() ||
				(doMixPaste && m[col].IsPcNote()) ||
				(doMixPaste && data[pos] == 'P' && !m[col].IsEmpty());

			if(skipPaste == false)
			{
				// Before changing anything in this pattern, we have to create an undo point.
				if(prepareUndo)
				{
					modDoc.GetPatternUndo().PrepareUndo(pattern, startChan, startRow, sndFile.GetNumChannels(), sndFile.Patterns[pattern].GetNumRows(), !firstUndo);
					prepareUndo = false;
					firstUndo = false;
				}

				// ITSyle mixpaste requires that we keep a copy of the thing we are about to paste on
				// so that we can refer back to check if there was anything in e.g. the note column before we pasted.
				const ModCommand origModCmd = m[col];

				// push channel data below paste point first.
				if(mode == pmPushForward)
				{
					for(ROWINDEX nPushRow = sndFile.Patterns[pattern].GetNumRows() - 1 - curRow; nPushRow > 0; nPushRow--)
					{
						m[col + nPushRow * sndFile.GetNumChannels()] = m[col + (nPushRow - 1) * sndFile.GetNumChannels()];
					}
					m[col].Clear();
				}

				// Note
				if(data[pos] != ' ' && (!doMixPaste || ((!doITStyleMix && origModCmd.note == NOTE_NONE) || 
					(doITStyleMix && origModCmd.note == NOTE_NONE && origModCmd.instr == 0 && origModCmd.volcmd == VOLCMD_NONE))))
				{
					m[col].note = NOTE_NONE;
					if(data[pos] == '=')
						m[col].note = NOTE_KEYOFF;
					else if(data[pos] == '^')
						m[col].note = NOTE_NOTECUT;
					else if(data[pos] == '~')
						m[col].note = NOTE_FADE;
					else if(data[pos] == 'P')
					{
						if(data[pos + 2] == 'S')
							m[col].note = NOTE_PCS;
						else
							m[col].note = NOTE_PC;
					} else if (data[pos] != '.')
					{
						// Check note names
						for(size_t i = 0; i < 12; i++)
						{
							if(data[pos] == szNoteNames[i][0] && data[pos + 1] == szNoteNames[i][1])
							{
								m[col].note = ModCommand::NOTE(i + NOTE_MIN);
								break;
							}
						}
						if(m[col].note != NOTE_NONE)
						{
							// Check octave
							m[col].note += (data[pos + 2] - '0') * 12;
							if(!m[col].IsNote())
							{
								// Invalid octave
								m[col].note = NOTE_NONE;
							}
						}
					}
				}

				// Instrument
				if(data[pos + 3] > ' ' && (!doMixPaste || ( (!doITStyleMix && origModCmd.instr == 0) || 
					(doITStyleMix && origModCmd.note == NOTE_NONE && origModCmd.instr == 0 && origModCmd.volcmd == VOLCMD_NONE) ) ))
				{
					if(data[pos + 3] >= '0' && data[pos + 3] <= ('0' + (MAX_SAMPLES / 10)))
					{
						m[col].instr = (data[pos + 3] - '0') * 10 + (data[pos + 4] - '0');
					} else m[col].instr = 0;
				}

				// Volume
				if(data[pos + 5] > ' ' && (!doMixPaste || ((!doITStyleMix && origModCmd.volcmd == VOLCMD_NONE) || 
					(doITStyleMix && origModCmd.note == NOTE_NONE && origModCmd.instr == 0 && origModCmd.volcmd == VOLCMD_NONE))))
				{
					if(data[pos + 5] != '.')
					{
						if(m[col].IsPcNote())
						{
							m[col].SetValueVolCol(ConvertStrTo<uint16>(data.Mid(pos + 5, 3)));
						}
						else
						{
							m[col].volcmd = VOLCMD_NONE;
							for(ModCommand::VOLCMD i = 1; i < MAX_VOLCMDS; i++)
							{
								const char cmd = sourceSpecs.GetVolEffectLetter(i);
								if(data[pos + 5] == cmd && cmd != '?')
								{
									m[col].volcmd = i;
									break;
								}
							}
							m[col].vol = (data[pos + 6] - '0') * 10 + (data[pos + 7] - '0');
						}
					} else
					{
						m[col].volcmd = VOLCMD_NONE;
						m[col].vol = 0;
					}
				}

				// Effect
				if(m[col].IsPcNote())
				{
					if(data[pos + 8] != '.' && data[pos + 8] > ' ')
					{
						m[col].SetValueEffectCol(ConvertStrTo<uint16>(data.Mid(pos + 8, 3)));
					}
				}
				else
				{
					if(data[pos + 8] > ' ' && (!doMixPaste || ((!doITStyleMix && origModCmd.command == CMD_NONE) || 
						(doITStyleMix && origModCmd.command == CMD_NONE && origModCmd.param == 0))))
					{
						m[col].command = CMD_NONE;
						if(data[pos + 8] != '.')
						{
							for(ModCommand::COMMAND i = 1; i < MAX_EFFECTS; i++)
							{
								const char cmd = sourceSpecs.GetEffectLetter(i);
								if(data[pos + 8] == cmd && cmd != '?')
								{
									m[col].command = i;
									break;
								}
							}
						}
					}

					// Effect value
					if(data[pos + 9] > ' ' && (!doMixPaste || ((!doITStyleMix && (origModCmd.command == CMD_NONE || origModCmd.param == 0)) || 
						(doITStyleMix && origModCmd.command == CMD_NONE && origModCmd.param == 0))))
					{
						m[col].param = 0;
						if(data[pos + 9] != '.')
						{
							for(size_t i = 0; i < 16; i++)
							{
								if(data[pos + 9] == szHexChar[i]) m[col].param |= (i << 4);
								if(data[pos + 10] == szHexChar[i]) m[col].param |= i;
							}
						}
					}
					
					// Speed / tempo command conversion
					if (sndFile.GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM))
					{
						switch(m[col].command)
						{
						case CMD_SPEED:
						case CMD_TEMPO:
							if(!clipboardHasS3MCommands) m[col].command = (m[col].param < tempoMin) ? CMD_SPEED : CMD_TEMPO;
							else
							{
								if ((m[col].command == CMD_SPEED) && (m[col].param >= tempoMin)) m[col].param = CMD_TEMPO; else
									if ((m[col].command == CMD_TEMPO) && (m[col].param < tempoMin)) m[col].param = CMD_SPEED;
							}
							break;
						}
					} else
					{
						switch(m[col].command)
						{
						case CMD_SPEED:
						case CMD_TEMPO:
							if(!clipboardHasS3MCommands) m[col].command = (m[col].param  < tempoMin) ? CMD_SPEED : CMD_TEMPO;
							break;
						}
					}
				}

				// convert some commands, if necessary. With mix paste convert only
				// if the original modcommand was empty as otherwise the unchanged parts
				// of the old modcommand would falsely be interpreted being of type
				// origFormat and ConvertCommand could change them.
				if (pasteFormat != sndFile.GetType() && (!doMixPaste || origModCmd.IsEmpty(false)))
					m[col].Convert(pasteFormat, sndFile.GetType());
			}

			pos += 11;
			col++;
		}
		// Next row
		m += sndFile.GetNumChannels();
		curRow++;

		if(overflowPaste)
		{
			// Handle overflow paste. Continue pasting in next pattern if enabled.
			// If Paste Flood is enabled, this won't be called due to obvious reasons.
			while(curRow >= sndFile.Patterns[pattern].GetNumRows())
			{
				curRow = 0;
				ORDERINDEX nextOrder = sndFile.Order.GetNextOrderIgnoringSkips(currentOrder);
				if(nextOrder == 0 || nextOrder >= sndFile.Order.size()) return success;
				pattern = sndFile.Order[nextOrder];
				if(sndFile.Patterns.IsValidPat(pattern) == false) return success;
				m = sndFile.Patterns[pattern];
				currentOrder = nextOrder;
				prepareUndo = true;
				startRow = 0;
			}
		}

	}

	return success;

}


// Copy one of the internal clipboards to the system clipboard.
bool PatternClipboard::SelectClipboard(clipindex_t which)
//-------------------------------------------------------
{
	activeClipboard = which;
	return ToSystemClipboard(clipboards[activeClipboard]);
}

// Switch to the next internal clipboard.
bool PatternClipboard::CycleForward()
//-----------------------------------
{
	activeClipboard++;
	if(activeClipboard >= clipboards.size())
	{
		activeClipboard = 0;
	}

	return SelectClipboard(activeClipboard);
}


// Switch to the previous internal clipboard.
bool PatternClipboard::CycleBackward()
//------------------------------------
{
	if(activeClipboard == 0)
	{
		activeClipboard = clipboards.size() - 1;
	} else
	{
		activeClipboard--;
	}

	return SelectClipboard(activeClipboard);
}


// Set the maximum number of internal clipboards.
void PatternClipboard::SetClipboardSize(clipindex_t maxEntries)
//-------------------------------------------------------------
{
	clipboardSize = maxEntries;
	RestrictClipboardSize();
}


// Remove all clipboard contents.
void PatternClipboard::Clear()
//----------------------------
{
	clipboards.clear();
	activeClipboard = 0;
}


// Keep the number of clipboards consistent with the maximum number of allowed clipboards.
void PatternClipboard::RestrictClipboardSize()
//--------------------------------------------
{
	if(clipboards.size() > clipboardSize)
	{
		clipboards.resize(clipboardSize);
		activeClipboard = 0;
	}
}


// System-specific clipboard functions
bool PatternClipboard::ToSystemClipboard(const CString &data)
//-----------------------------------------------------------
{
	CMainFrame *mainFrame = CMainFrame::GetMainFrame();
	if(mainFrame == nullptr || !mainFrame->OpenClipboard())
	{
		return false;
	}

	HGLOBAL hCpy;
	if((hCpy = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, data.GetLength() + 1)) == NULL)
	{
		return false;
	}

	::EmptyClipboard();

	void *clipboard;
	if((clipboard = ::GlobalLock(hCpy)) != nullptr)
	{
		strcpy(clipboard, data);
		::GlobalUnlock(hCpy);
	}

	::SetClipboardData (CF_TEXT, (HANDLE)hCpy);
	::CloseClipboard();

	return (clipboard != nullptr);

}


// System-specific clipboard functions
bool PatternClipboard::FromSystemClipboard(CString &data)
//-------------------------------------------------------
{
	CMainFrame *mainFrame = CMainFrame::GetMainFrame();
	if(mainFrame == nullptr || !mainFrame->OpenClipboard())
	{
		return false;
	}

	HGLOBAL hCpy = ::GetClipboardData(CF_TEXT);
	LPSTR p = nullptr;
	if(hCpy && (p = (LPSTR)::GlobalLock(hCpy)) != nullptr)
	{
		data.SetString(p, ::GlobalSize(hCpy));
		::GlobalUnlock(hCpy);
	}

	::CloseClipboard();

	return(p != nullptr);
}