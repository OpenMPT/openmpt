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
#include "View_pat.h"


/* Clipboard format:
 * Hdr: "ModPlug Tracker S3M\r\n"
 * Full:  '|C#401v64A06'
 * Reset: '|...........'
 * Empty: '|           '
 * End of row: '\r\n'
 *
 * When pasting multiple patterns, the header line is followed by the order list:
 * Orders: 0,1,2,+,-,1\r\n
 * After that, the individual pattern headers and pattern data follows:
 * 'Rows: 64\r\n' (must be first)
 * 'Name: Pattern Name\r\n' (optional)
 * 'Signature: 4/16\r\n' (optional)
 * Pattern data...
 */

PatternClipboard PatternClipboard::patternClipboard;

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


// Copy a range of patterns to both the system clipboard and the internal clipboard.
bool PatternClipboard::Copy(CSoundFile &sndFile, ORDERINDEX first, ORDERINDEX last)
//---------------------------------------------------------------------------------
{
	LimitMax(first, sndFile.Order.GetLength());
	LimitMax(last, sndFile.Order.GetLength());

	// Set up clipboard header.
	CString data = "ModPlug Tracker " + GetFileExtension(sndFile.GetModSpecifications().fileExtension) + "\r\nOrders: ";
	CString patternData;

	// Pattern => Order list assignment
	std::vector<PATTERNINDEX> patList(sndFile.Patterns.Size(), PATTERNINDEX_INVALID);
	PATTERNINDEX insertedPats = 0;

	// Add order list and pattern information to header.
	for(ORDERINDEX ord = first; ord <= last; ord++)
	{
		PATTERNINDEX pattern = sndFile.Order[ord];

		if(ord != first)
		{
			data.AppendChar(',');
		}
		if(pattern == sndFile.Order.GetInvalidPatIndex())
		{
			data.AppendChar('-');
		} else if(pattern == sndFile.Order.GetIgnoreIndex())
		{
			data.AppendChar('+');
		} else if(sndFile.Patterns.IsValidPat(pattern))
		{
			if(patList[pattern] == PATTERNINDEX_INVALID)
			{
				// New pattern
				patList[pattern] = insertedPats++;

				patternData.AppendFormat("Rows: %u\r\n", sndFile.Patterns[pattern].GetNumRows());
				CString name = sndFile.Patterns[pattern].GetName();
				if(!name.IsEmpty())
				{
					patternData.Append("Name: " + name + "\r\n");
				}
				if(sndFile.Patterns[pattern].GetOverrideSignature())
				{
					patternData.AppendFormat("Signature: %u/%u\r\n", sndFile.Patterns[pattern].GetRowsPerBeat(), sndFile.Patterns[pattern].GetRowsPerMeasure());
				}
				patternData.Append(CreateClipboardString(sndFile, pattern, PatternRect(PatternCursor(), PatternCursor(sndFile.Patterns[pattern].GetNumRows() - 1, sndFile.GetNumChannels() - 1, PatternCursor::lastColumn))));
			}

			data.AppendFormat("%u", patList[pattern]);
		}
	}
	data.Append("\r\n" + patternData);
	
	if(activeClipboard < clipboards.size())
	{
		// Copy to internal clipboard
		CString desc;
		desc.Format("%u Patterns (%u to %u)", last - first + 1, first, last);
		clipboards[activeClipboard] = PatternClipboardElement(data, desc);
	}

	return ToSystemClipboard(data);
}


// Copy a pattern selection to both the system clipboard and the internal clipboard.
bool PatternClipboard::Copy(CSoundFile &sndFile, PATTERNINDEX pattern, PatternRect selection)
//-------------------------------------------------------------------------------------------
{
	CString data = CreateClipboardString(sndFile, pattern, selection);
	if(data.IsEmpty())
	{
		return false;
	}

	// Set up clipboard header.
	data = "ModPlug Tracker " + GetFileExtension(sndFile.GetModSpecifications().fileExtension) + "\r\n" + data;

	if(activeClipboard < clipboards.size())
	{
		// Copy to internal clipboard
		CString desc;
		desc.Format("%u rows, %u channels (pattern %u)", selection.GetNumRows(), selection.GetNumChannels(), pattern);
		clipboards[activeClipboard] = PatternClipboardElement(data, desc);
	}

	return ToSystemClipboard(data);
}


// Create the clipboard text for a pattern selection
CString PatternClipboard::CreateClipboardString(CSoundFile &sndFile, PATTERNINDEX pattern, PatternRect selection)
//---------------------------------------------------------------------------------------------------------------
{
	if(!sndFile.Patterns.IsValidPat(pattern))
	{
		return "";
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

	CString data;
	data.Preallocate(numRows * (numChans * 12 + 2));

	for(ROWINDEX row = 0; row < numRows; row++)
	{
		if(row + startRow >= sndFile.Patterns[pattern].GetNumRows())
		{
			break;
		}

		const ModCommand *m = sndFile.Patterns[pattern].GetpModCommand(row + startRow, startChan);

		for(CHANNELINDEX chn = 0; chn < numChans; chn++, m++)
		{
			PatternCursor cursor(0, startChan + chn);
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

	return data;
}


// Try pasting a pattern selection from the system clipboard.
bool PatternClipboard::Paste(CSoundFile &sndFile, ModCommandPos &pastePos, PasteModes mode, ORDERINDEX curOrder)
//--------------------------------------------------------------------------------------------------------------
{
	CString data;
	if(!FromSystemClipboard(data) || !HandlePaste(sndFile, pastePos, mode, data, curOrder))
	{
		// Fall back to internal clipboard if there's no valid pattern data in the system clipboard.
		return Paste(sndFile, pastePos, mode, curOrder, activeClipboard);
	}
	return true;
}


// Try pasting a pattern selection from an internal clipboard.
bool PatternClipboard::Paste(CSoundFile &sndFile, ModCommandPos &pastePos, PasteModes mode, ORDERINDEX curOrder, clipindex_t internalClipboard)
//---------------------------------------------------------------------------------------------------------------------------------------------
{
	if(internalClipboard >= clipboards.size())
	{
		return false;
	}
	return HandlePaste(sndFile, pastePos, mode, clipboards[internalClipboard].content, curOrder);
}


// Parse clipboard string and perform the pasting operation.
bool PatternClipboard::HandlePaste(CSoundFile &sndFile, ModCommandPos &pastePos, PasteModes mode, const CString &data, ORDERINDEX curOrder)
//-----------------------------------------------------------------------------------------------------------------------------------------
{
	PATTERNINDEX pattern = pastePos.pattern;
	if(sndFile.GetpModDoc() == nullptr)
	{
		return false;
	}

	CModDoc &modDoc = *(sndFile.GetpModDoc());

	const TEMPO tempoMin = sndFile.GetModSpecifications().tempoMin;

	bool success = false;
	bool prepareUndo = true;	// prepare pattern for undo next time
	bool firstUndo = true;		// for chaining undos (see overflow / multi-pattern paste)

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

	// Skip whitespaces
	while(data[startPos] == '\r' || data[startPos] == '\n' || data[startPos] == ' ')
	{
		startPos++;
	}
	
	std::vector<PATTERNINDEX> patList;
	bool multiPaste = false;
	if(data.Mid(startPos, 8) == "Orders: ")
	{
		// Pasting several patterns at once.
		multiPaste = true;
		mode = pmOverwrite;

		if(sndFile.Patterns.IsValidPat(sndFile.Order[curOrder]))
		{
			// Put new patterns after current pattern, if it exists
			curOrder++;
		}

		pos = startPos + 8;
		startPos = data.Find("\n", pos) + 1;
		ORDERINDEX writeOrder = curOrder;

		while(pos < startPos && pos != 0)
		{
			PATTERNINDEX insertPat;
			if(data[pos] == '+')
			{
				insertPat = sndFile.Order.GetIgnoreIndex();
			} else if(data[pos] == '-')
			{
				insertPat = sndFile.Order.GetInvalidPatIndex();
			} else
			{
				insertPat = ConvertStrTo<PATTERNINDEX>(data.Mid(pos, 16));

				if(insertPat < patList.size())
				{
					// Duplicate pattern
					insertPat = patList[insertPat];
				} else
				{
					// New pattern
					if(insertPat >= patList.size())
					{
						patList.resize(insertPat + 1, PATTERNINDEX_INVALID);
					}

					patList[insertPat] = sndFile.Patterns.Insert(64);
					insertPat = patList[insertPat];
				}
			}

			// Next order item, please
			pos = data.Find(",", pos + 1) + 1;

			if((insertPat == sndFile.Order.GetIgnoreIndex() && !sndFile.GetModSpecifications().hasIgnoreIndex) || insertPat == PATTERNINDEX_INVALID)
			{
				continue;
			}

			if(sndFile.Order.Insert(writeOrder, 1) == 0)
			{
				break;
			}
			sndFile.Order[writeOrder++] = insertPat;
		}

		if(!patList.empty())
		{
			// First pattern we're going to paste in.
			pattern = patList[0];
		}

		// We already modified the order list...
		success = true;

		pastePos.pattern = pattern;
		pastePos.row = 0;
		pastePos.channel = 0;
	}

	size_t curPattern = 0;	// Currently pasted pattern for multi-paste
	ROWINDEX startRow = pastePos.row;
	ROWINDEX curRow = startRow;
	CHANNELINDEX startChan = pastePos.channel, col;

	// Can we actually paste at this position?
	if(!sndFile.Patterns.IsValidPat(pattern) || startRow >= sndFile.Patterns[pattern].GetNumRows() || startChan >= sndFile.GetNumChannels())
	{
		return success;
	}

	const CModSpecifications &sourceSpecs = CSoundFile::GetModSpecifications(pasteFormat);
	const bool overflowPaste = ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OVERFLOWPASTE) && mode != pmPasteFlood && mode != pmPushForward) && !multiPaste;
	const bool doITStyleMix = (mode == pmMixPasteIT);
	const bool doMixPaste = (mode == pmMixPaste) || doITStyleMix;
	const bool clipboardHasS3MCommands = (pasteFormat & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_S3M)) != 0;

	ModCommand *patData = sndFile.Patterns[pattern].GetpModCommand(startRow, 0);

	pos = startPos;
	while(curRow < sndFile.Patterns[pattern].GetNumRows() || multiPaste)
	{
		const int nextPatStart = data.Find('|', pos);
		// Search for column separator or end of paste data.
		if(nextPatStart == -1)
		{
			// End of paste
			if(mode == pmPasteFlood && curRow != startRow)
			{
				// Restarting pasting from beginning.
				pos = startPos;
				continue;
			} else
			{
				// Prevent infinite loop with malformed clipboard data.
				break;
			}
		}

		// Handle multi-paste: Read pattern information
		if(multiPaste)
		{
			// Skip whitespaces
			while(data[pos] == '\r' || data[pos] == '\n' || data[pos] == ' ')
			{
				pos++;
			}

			// Parse pattern header lines
			while(pos < nextPatStart)
			{
				int eol = data.Find("\n", pos + 1) + 1;
				if(eol == -1)
				{
					break;
				}
				if(data.Mid(pos, 6) == "Rows: ")
				{
					pos += 6;
					// Advance to next pattern
					do 
					{
						if(curPattern >= patList.size())
						{
							return success;
						}
						pattern = patList[curPattern++];
					} while (pattern == PATTERNINDEX_INVALID);
					ROWINDEX numRows = ConvertStrTo<ROWINDEX>(data.Mid(pos, eol - pos));
					sndFile.Patterns[pattern].Resize(numRows);
					patData = sndFile.Patterns[pattern].GetpModCommand(0, 0);
					curRow = 0;
					prepareUndo = true;
				} else if(data.Mid(pos, 6) == "Name: ")
				{
					pos += 6;
					sndFile.Patterns[pattern].SetName(data.Mid(pos, eol - pos - 2).TrimRight());
				} else if(data.Mid(pos, 11) == "Signature: ")
				{
					pos += 11;
					int pos2 = data.Find("/", pos + 1) + 1;
					ROWINDEX rpb = ConvertStrTo<ROWINDEX>(data.Mid(pos, pos2 - pos));
					ROWINDEX rpm = ConvertStrTo<ROWINDEX>(data.Mid(pos2, eol - pos2));
					sndFile.Patterns[pattern].SetSignature(rpb, rpm);
				} else
				{
					break;
				}
				pos = eol;
			}
		}

		// Search for column separator or end of paste data.
		pos = nextPatStart;

		success = true;
		col = startChan;
		// Paste columns
		while((pos + 11 < data.GetLength()) && (data[pos] == '|'))
		{
			pos++;
			// Handle pasting large pattern into smaller pattern (e.g. 128-row pattern into MOD, which only allows 64 rows)
			static ModCommand dummy;
			ModCommand &m = curRow < sndFile.Patterns[pattern].GetNumRows() ? patData[col] : dummy;

			// Check valid paste condition. Paste will be skipped if
			// - col is not a valid channelindex or
			// - doing mix paste and paste destination modcommand is a PCnote or
			// - doing mix paste and trying to paste PCnote on non-empty modcommand.
			const bool skipPaste =
				col >= sndFile.GetNumChannels() ||
				(doMixPaste && m.IsPcNote()) ||
				(doMixPaste && data[pos] == 'P' && !m.IsEmpty());

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
				const ModCommand origModCmd = m;

				// push channel data below paste point first.
				if(mode == pmPushForward)
				{
					for(ROWINDEX pushRow = sndFile.Patterns[pattern].GetNumRows() - 1 - curRow; pushRow > 0; pushRow--)
					{
						patData[col + pushRow * sndFile.GetNumChannels()] = patData[col + (pushRow - 1) * sndFile.GetNumChannels()];
					}
					m.Clear();
				}

				// Note
				if(data[pos] != ' ' && (!doMixPaste || ((!doITStyleMix && origModCmd.note == NOTE_NONE) || 
					(doITStyleMix && origModCmd.note == NOTE_NONE && origModCmd.instr == 0 && origModCmd.volcmd == VOLCMD_NONE))))
				{
					m.note = NOTE_NONE;
					if(data[pos] == '=')
						m.note = NOTE_KEYOFF;
					else if(data[pos] == '^')
						m.note = NOTE_NOTECUT;
					else if(data[pos] == '~')
						m.note = NOTE_FADE;
					else if(data[pos] == 'P')
					{
						if(data[pos + 2] == 'S')
							m.note = NOTE_PCS;
						else
							m.note = NOTE_PC;
					} else if (data[pos] != '.')
					{
						// Check note names
						for(size_t i = 0; i < 12; i++)
						{
							if(data[pos] == szNoteNames[i][0] && data[pos + 1] == szNoteNames[i][1])
							{
								m.note = ModCommand::NOTE(i + NOTE_MIN);
								break;
							}
						}
						if(m.note != NOTE_NONE)
						{
							// Check octave
							m.note += (data[pos + 2] - '0') * 12;
							if(!m.IsNote())
							{
								// Invalid octave
								m.note = NOTE_NONE;
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
						m.instr = (data[pos + 3] - '0') * 10 + (data[pos + 4] - '0');
					} else m.instr = 0;
				}

				// Volume
				if(data[pos + 5] > ' ' && (!doMixPaste || ((!doITStyleMix && origModCmd.volcmd == VOLCMD_NONE) || 
					(doITStyleMix && origModCmd.note == NOTE_NONE && origModCmd.instr == 0 && origModCmd.volcmd == VOLCMD_NONE))))
				{
					if(data[pos + 5] != '.')
					{
						if(m.IsPcNote())
						{
							m.SetValueVolCol(ConvertStrTo<uint16>(data.Mid(pos + 5, 3)));
						}
						else
						{
							m.volcmd = VOLCMD_NONE;
							for(ModCommand::VOLCMD i = 1; i < MAX_VOLCMDS; i++)
							{
								const char cmd = sourceSpecs.GetVolEffectLetter(i);
								if(data[pos + 5] == cmd && cmd != '?')
								{
									m.volcmd = i;
									break;
								}
							}
							m.vol = (data[pos + 6] - '0') * 10 + (data[pos + 7] - '0');
						}
					} else
					{
						m.volcmd = VOLCMD_NONE;
						m.vol = 0;
					}
				}

				// Effect
				if(m.IsPcNote())
				{
					if(data[pos + 8] != '.' && data[pos + 8] > ' ')
					{
						m.SetValueEffectCol(ConvertStrTo<uint16>(data.Mid(pos + 8, 3)));
					}
				} else
				{
					if(data[pos + 8] > ' ' && (!doMixPaste || ((!doITStyleMix && origModCmd.command == CMD_NONE) || 
						(doITStyleMix && origModCmd.command == CMD_NONE && origModCmd.param == 0))))
					{
						m.command = CMD_NONE;
						if(data[pos + 8] != '.')
						{
							for(ModCommand::COMMAND i = 1; i < MAX_EFFECTS; i++)
							{
								const char cmd = sourceSpecs.GetEffectLetter(i);
								if(data[pos + 8] == cmd && cmd != '?')
								{
									m.command = i;
									break;
								}
							}
						}
					}

					// Effect value
					if(data[pos + 9] > ' ' && (!doMixPaste || ((!doITStyleMix && (origModCmd.command == CMD_NONE || origModCmd.param == 0)) || 
						(doITStyleMix && origModCmd.command == CMD_NONE && origModCmd.param == 0))))
					{
						m.param = 0;
						if(data[pos + 9] != '.')
						{
							for(size_t i = 0; i < 16; i++)
							{
								if(data[pos + 9] == szHexChar[i]) m.param |= (i << 4);
								if(data[pos + 10] == szHexChar[i]) m.param |= i;
							}
						}
					}
					
					// Speed / tempo command conversion
					if (sndFile.GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM))
					{
						switch(m.command)
						{
						case CMD_SPEED:
						case CMD_TEMPO:
							if(!clipboardHasS3MCommands)
							{
								if(m.param < tempoMin)
								{
									m.command = CMD_SPEED;
								} else
								{
									m.command = CMD_TEMPO;
								}
							} else
							{
								if(m.command == CMD_SPEED && m.param >= tempoMin)
								{
									m.param = CMD_TEMPO;
								} else if(m.command == CMD_TEMPO && m.param < tempoMin)
								{
									m.param = CMD_SPEED;
								}
							}
							break;
						}
					} else
					{
						switch(m.command)
						{
						case CMD_SPEED:
						case CMD_TEMPO:
							if(!clipboardHasS3MCommands)
							{
								if(m.param  < tempoMin)
								{
									m.command = CMD_SPEED;
								} else
								{
									m.command = CMD_TEMPO;
								}
							}
							break;
						}
					}
				}

				// convert some commands, if necessary. With mix paste convert only
				// if the original modcommand was empty as otherwise the unchanged parts
				// of the old modcommand would falsely be interpreted being of type
				// origFormat and ConvertCommand could change them.
				if (pasteFormat != sndFile.GetType() && (!doMixPaste || origModCmd.IsEmpty(false)))
					m.Convert(pasteFormat, sndFile.GetType());
			}

			pos += 11;
			col++;
		}
		// Next row
		patData += sndFile.GetNumChannels();
		curRow++;

		if(overflowPaste)
		{
			// Handle overflow paste. Continue pasting in next pattern if enabled.
			// If Paste Flood is enabled, this won't be called due to obvious reasons.
			while(curRow >= sndFile.Patterns[pattern].GetNumRows())
			{
				curRow = 0;
				ORDERINDEX nextOrder = sndFile.Order.GetNextOrderIgnoringSkips(curOrder);
				if(nextOrder == 0 || nextOrder >= sndFile.Order.size()) return success;
				pattern = sndFile.Order[nextOrder];
				if(sndFile.Patterns.IsValidPat(pattern) == false) return success;
				patData = sndFile.Patterns[pattern];
				curOrder = nextOrder;
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
	clipboards.resize(maxEntries, PatternClipboardElement("", "unused"));
	LimitMax(activeClipboard, maxEntries - 1);
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


BEGIN_MESSAGE_MAP(PatternClipboardDialog, CDialog)
	ON_EN_UPDATE(IDC_EDIT1,		OnNumClipboardsChanged)
	ON_LBN_SELCHANGE(IDC_LIST1,	OnSelectClipboard)
END_MESSAGE_MAP()

PatternClipboardDialog PatternClipboardDialog::patternClipboardDialog(PatternClipboard::patternClipboard);

void PatternClipboardDialog::DoDataExchange(CDataExchange* pDX)
//-------------------------------------------------------------
{
	DDX_Control(pDX, IDC_SPIN1,	numClipboardsSpin);
	DDX_Control(pDX, IDC_LIST1,	clipList);
}


PatternClipboardDialog::PatternClipboardDialog(PatternClipboard &c) : clipboards(c), isLocked(true), isCreated(false), posX(-1)
//-----------------------------------------------------------------------------------------------------------------------------
{
}


void PatternClipboardDialog::Show()
//---------------------------------
{
	isLocked = true;
	if(!isCreated)
	{
		Create(IDD_CLIPBOARD, CMainFrame::GetMainFrame());
		numClipboardsSpin.SetRange(0, int16_max);
	}
	SetDlgItemInt(IDC_EDIT1, clipboards.GetClipboardSize(), FALSE);
	isLocked = false;
	isCreated = true;
	UpdateList();
	
	SetWindowPos(nullptr, posX, posY, 0, 0, SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER | (posX == -1 ? SWP_NOMOVE : 0));
}


void PatternClipboardDialog::OnNumClipboardsChanged()
//---------------------------------------------------
{
	if(isLocked)
	{
		return;
	}
	clipboards.SetClipboardSize(GetDlgItemInt(IDC_EDIT1, nullptr, FALSE));
	UpdateList();
}


void PatternClipboardDialog::UpdateList()
//---------------------------------------
{
	if(isLocked)
	{
		return;
	}
	clipList.ResetContent();
	PatternClipboard::clipindex_t i = 0;
	for(std::deque<PatternClipboardElement>::const_iterator clip = clipboards.clipboards.cbegin(); clip != clipboards.clipboards.cend(); clip++, i++)
	{
		const int item = clipList.AddString(clip->description);
		clipList.SetItemDataPtr(item, reinterpret_cast<void *>(i));
		if(clipboards.activeClipboard == i)
		{
			clipList.SetCurSel(item);
		}
	}
}


void PatternClipboardDialog::OnSelectClipboard()
//----------------------------------------------
{
	if(isLocked)
	{
		return;
	}
	PatternClipboard::clipindex_t item = reinterpret_cast<PatternClipboard::clipindex_t>(clipList.GetItemDataPtr(clipList.GetCurSel()));
	clipboards.SelectClipboard(item);
}


void PatternClipboardDialog::OnCancel()
//-------------------------------------
{
	isCreated = false;
	isLocked = true;

	RECT rect;
	GetWindowRect(&rect);
	posX = rect.left;
	posY = rect.top;

	DestroyWindow();
}
