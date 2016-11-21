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
#include "../soundlib/mod_specifications.h"
#include "../soundlib/Tables.h"


OPENMPT_NAMESPACE_BEGIN


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
 * 'Swing: 16777216,16777216,16777216,16777216\r\n' (optional)
 * Pattern data...
 */

PatternClipboard PatternClipboard::instance;

std::string PatternClipboard::GetFileExtension(const char *ext, bool addPadding)
//------------------------------------------------------------------------------
{
	std::string format(ext);
	if(format.size() > 3)
	{
		format.resize(3);
	}
	format = mpt::ToUpperCaseAscii(format);
	if(addPadding)
	{
		format.insert(0, 3 - format.size(), ' ');
	}
	return format;
}


// Copy a range of patterns to both the system clipboard and the internal clipboard.
bool PatternClipboard::Copy(CSoundFile &sndFile, ORDERINDEX first, ORDERINDEX last, bool onlyOrders)
//--------------------------------------------------------------------------------------------------
{
	LimitMax(first, sndFile.Order.GetLength());
	LimitMax(last, sndFile.Order.GetLength());

	// Set up clipboard header.
	std::string data = "ModPlug Tracker " + GetFileExtension(sndFile.GetModSpecifications().fileExtension, true) + "\r\nOrders: ";
	std::string patternData;

	// Pattern => Order list assignment
	std::vector<PATTERNINDEX> patList(sndFile.Patterns.Size(), PATTERNINDEX_INVALID);
	PATTERNINDEX insertedPats = 0;

	// Add order list and pattern information to header.
	for(ORDERINDEX ord = first; ord <= last; ord++)
	{
		PATTERNINDEX pattern = sndFile.Order[ord];

		if(ord != first)
			data += ',';
		
		if(pattern == sndFile.Order.GetInvalidPatIndex())
		{
			data += '-';
		} else if(pattern == sndFile.Order.GetIgnoreIndex())
		{
			data += '+';
		} else if(sndFile.Patterns.IsValidPat(pattern))
		{
			if(onlyOrders)
			{
				patList[pattern] = pattern;
			} else if(patList[pattern] == PATTERNINDEX_INVALID)
			{
				// New pattern
				patList[pattern] = insertedPats++;

				const CPattern &pat = sndFile.Patterns[pattern];
				patternData += mpt::format("Rows: %1\r\n")(pat.GetNumRows());
				std::string name = pat.GetName();
				if(!name.empty())
				{
					patternData += "Name: " + name + "\r\n";
				}
				if(pat.GetOverrideSignature())
				{
					patternData += mpt::format("Signature: %1/%2\r\n")(pat.GetRowsPerBeat(), pat.GetRowsPerMeasure());
				}
				if(pat.HasTempoSwing())
				{
					patternData += "Swing: ";
					const TempoSwing &swing = pat.GetTempoSwing();
					for(size_t i = 0; i < swing.size(); i++)
					{
						patternData += mpt::format(i == 0 ? "%1" : ",%1")(swing[i]);
					}
					patternData += "\r\n";
				}
				patternData += CreateClipboardString(sndFile, pattern, PatternRect(PatternCursor(), PatternCursor(sndFile.Patterns[pattern].GetNumRows() - 1, sndFile.GetNumChannels() - 1, PatternCursor::lastColumn)));
			}

			data += mpt::ToString(patList[pattern]);
		}
	}
	if(!onlyOrders)
	{
		data += "\r\n" + patternData;
	}
	
	if(instance.activeClipboard < instance.clipboards.size())
	{
		// Copy to internal clipboard
		CString desc;
		desc.Format(_T("%u %s (%u to %u)"), last - first + 1, onlyOrders ? _T("Orders") : _T("Patterns"), first, last);
		instance.clipboards[instance.activeClipboard] = PatternClipboardElement(data, desc);
	}

	return ToSystemClipboard(data);
}


// Copy a pattern selection to both the system clipboard and the internal clipboard.
bool PatternClipboard::Copy(CSoundFile &sndFile, PATTERNINDEX pattern, PatternRect selection)
//-------------------------------------------------------------------------------------------
{
	std::string data = CreateClipboardString(sndFile, pattern, selection);
	if(data.empty())
		return false;

	// Set up clipboard header.
	data = "ModPlug Tracker " + GetFileExtension(sndFile.GetModSpecifications().fileExtension, true) + "\r\n" + data;

	if(instance.activeClipboard < instance.clipboards.size())
	{
		// Copy to internal clipboard
		CString desc;
		desc.Format(_T("%u rows, %u channels (pattern %u)"), selection.GetNumRows(), selection.GetNumChannels(), pattern);
		instance.clipboards[instance.activeClipboard] = PatternClipboardElement(data, desc);
	}

	return ToSystemClipboard(data);
}


// Create the clipboard text for a pattern selection
std::string PatternClipboard::CreateClipboardString(CSoundFile &sndFile, PATTERNINDEX pattern, PatternRect selection)
//--------------------------------------------------------------------------------------------------------------------
{
	if(!sndFile.Patterns.IsValidPat(pattern))
		return "";

	if(selection.GetStartColumn() == PatternCursor::paramColumn)
	{
		// Special case: If selection starts with a parameter column, extend it to include the effect letter as well.
		PatternCursor upper(selection.GetUpperLeft());
		upper.Move(0, 0, -1);
		selection = PatternRect(upper, selection.GetLowerRight());
	}

	const ROWINDEX startRow = selection.GetStartRow(), numRows = selection.GetNumRows();
	const CHANNELINDEX startChan = selection.GetStartChannel(), numChans = selection.GetNumChannels();

	std::string data;
	data.reserve(numRows * (numChans * 12 + 2));

	for(ROWINDEX row = 0; row < numRows; row++)
	{
		if(row + startRow >= sndFile.Patterns[pattern].GetNumRows())
			break;

		const ModCommand *m = sndFile.Patterns[pattern].GetpModCommand(row + startRow, startChan);

		for(CHANNELINDEX chn = 0; chn < numChans; chn++, m++)
		{
			PatternCursor cursor(0, startChan + chn);
			data += '|';

			// Note
			if(selection.ContainsHorizontal(cursor))
			{
				if(m->IsNote())
				{
					// Need to guarantee that sharps are used for the clipboard.
					data += NoteNamesSharp[(m->note - NOTE_MIN) % 12];
					data += ('0' + (m->note - NOTE_MIN) / 12);
				} else
				{
					data += sndFile.GetNoteName(m->note);
				}
			} else
			{
				// No note
				data += "   ";
			}

			// Instrument
			cursor.Move(0, 0, 1);
			if(selection.ContainsHorizontal(cursor))
			{
				if(m->instr)
				{
					data += ('0' + (m->instr / 10));
					data += ('0' + (m->instr % 10));
				} else
				{
					data += "..";
				}
			} else
			{
				data += "  ";
			}

			// Volume
			cursor.Move(0, 0, 1);
			if(selection.ContainsHorizontal(cursor))
			{
				if(m->IsPcNote())
				{
					data += mpt::fmt::dec0<3>(m->GetValueVolCol());
				}
				else
				{
					if(m->volcmd != VOLCMD_NONE && m->vol <= 99)
					{
						data += sndFile.GetModSpecifications().GetVolEffectLetter(m->volcmd);
						data += mpt::fmt::dec0<2>(m->vol);
					} else
					{
						data += "...";
					}
				}
			} else
			{
				data += "   ";
			}
			
			// Effect
			cursor.Move(0, 0, 1);
			if(selection.ContainsHorizontal(cursor))
			{
				if(m->IsPcNote())
				{
					data += mpt::fmt::dec0<3>(m->GetValueEffectCol());
				}
				else
				{
					if(m->command != CMD_NONE)
					{
						data += sndFile.GetModSpecifications().GetEffectLetter(m->command);
					} else
					{
						data += '.';
					}

					if(m->param != 0 && m->command != CMD_NONE)
					{
						data += mpt::fmt::HEX0<2>(m->param);
					} else
					{
						data += "..";
					}
				}
			} else
			{
				data += "   ";
			}
		}

		// Next Row
		data += "\r\n";
	}

	return data;
}


// Try pasting a pattern selection from the system clipboard.
bool PatternClipboard::Paste(CSoundFile &sndFile, ModCommandPos &pastePos, PasteModes mode, ORDERINDEX curOrder, PatternRect &pasteRect)
//--------------------------------------------------------------------------------------------------------------------------------------
{
	std::string data;
	if(!FromSystemClipboard(data) || !HandlePaste(sndFile, pastePos, mode, data, curOrder, pasteRect))
	{
		// Fall back to internal clipboard if there's no valid pattern data in the system clipboard.
		return Paste(sndFile, pastePos, mode, curOrder, pasteRect, instance.activeClipboard);
	}
	return true;
}


// Try pasting a pattern selection from an internal clipboard.
bool PatternClipboard::Paste(CSoundFile &sndFile, ModCommandPos &pastePos, PasteModes mode, ORDERINDEX curOrder, PatternRect &pasteRect, clipindex_t internalClipboard)
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(internalClipboard >= instance.clipboards.size())
		return false;
	
	return HandlePaste(sndFile, pastePos, mode, instance.clipboards[internalClipboard].content, curOrder, pasteRect);
}


// Parse clipboard string and perform the pasting operation.
bool PatternClipboard::HandlePaste(CSoundFile &sndFile, ModCommandPos &pastePos, PasteModes mode, const std::string &data, ORDERINDEX curOrder, PatternRect &pasteRect)
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	const std::string whitespace(" \n\r\t");
	PATTERNINDEX pattern = pastePos.pattern;
	if(sndFile.GetpModDoc() == nullptr)
		return false;

	CModDoc &modDoc = *(sndFile.GetpModDoc());

	bool success = false;
	bool prepareUndo = true;	// Prepare pattern for undo next time
	bool firstUndo = true;		// For chaining undos (see overflow / multi-pattern paste)

	// Search for signature
	std::string::size_type pos, startPos = 0;
	MODTYPE pasteFormat = MOD_TYPE_NONE;
	while(pasteFormat == MOD_TYPE_NONE && (startPos = data.find("ModPlug Tracker ", startPos)) != std::string::npos)
	{
		startPos += 16;
		// Check paste format
		const std::string format = mpt::ToUpperCaseAscii(mpt::String::Trim(data.substr(startPos, 3)));

		for(size_t i = 0; i < CountOf(ModSpecs::Collection); i++)
		{
			if(format == GetFileExtension(ModSpecs::Collection[i]->fileExtension, false))
			{
				pasteFormat = ModSpecs::Collection[i]->internalType;
				startPos += 3;
				break;
			}
		}
	}

	// What is this I don't even
	if(startPos == std::string::npos)
		return false;
	// Skip whitespaces
	startPos = data.find_first_not_of(whitespace, startPos);
	if(startPos == std::string::npos)
		return false;
	
	// Multi-order stuff
	std::vector<PATTERNINDEX> patList;
	// Multi-order mix-paste stuff
	std::vector<ORDERINDEX> ordList;
	std::vector<std::string::size_type> patOffset;

	enum { kSinglePaste, kMultiInsert, kMultiOverwrite } patternMode = kSinglePaste;
	if(data.substr(startPos, 8) == "Orders: ")
	{
		// Pasting several patterns at once.
		patternMode = (mode == pmOverwrite) ?  kMultiInsert : kMultiOverwrite;

		// Put new patterns after current pattern, if it exists
		if(sndFile.Order.IsValidPat(curOrder) && patternMode == kMultiInsert)
			curOrder++;

		pos = startPos + 8;
		startPos = data.find('\n', pos);
		ORDERINDEX writeOrder = curOrder;
		const bool onlyOrders = (startPos == std::string::npos);
		if(onlyOrders)
		{
			// Only create order list, no patterns
			startPos = data.size();
		} else
		{
			startPos++;
		}

		while(pos < startPos && pos != std::string::npos)
		{
			PATTERNINDEX insertPat;
			auto curPos = pos;
			// Next order item, please
			pos = data.find(',', pos + 1);
			if(pos != std::string::npos)
				pos++;

			if(data[curPos] == '+')
			{
				insertPat = sndFile.Order.GetIgnoreIndex();
			} else if(data[curPos] == '-')
			{
				insertPat = sndFile.Order.GetInvalidPatIndex();
			} else
			{
				insertPat = ConvertStrTo<PATTERNINDEX>(data.substr(curPos, 10));
				if(patternMode == kMultiOverwrite)
				{
					// We only want the order of pasted patterns now, do not create any new patterns
					ordList.push_back(insertPat);
					continue;
				}

				if(insertPat < patList.size())
				{
					// Duplicate pattern
					insertPat = patList[insertPat];
				} else if(!onlyOrders)
				{
					// New pattern
					if(insertPat >= patList.size())
					{
						patList.resize(insertPat + 1, PATTERNINDEX_INVALID);
					}

					patList[insertPat] = sndFile.Patterns.InsertAny(64, true);
					insertPat = patList[insertPat];
				}
			}

			if((insertPat == sndFile.Order.GetIgnoreIndex() && !sndFile.GetModSpecifications().hasIgnoreIndex)
				|| (insertPat == sndFile.Order.GetInvalidPatIndex() && !sndFile.GetModSpecifications().hasStopIndex)
				|| insertPat == PATTERNINDEX_INVALID)
			{
				continue;
			}

			if(sndFile.Order.Insert(writeOrder, 1) == 0)
			{
				break;
			}
			sndFile.Order[writeOrder++] = insertPat;
		}

		if(patternMode == kMultiInsert)
		{
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
		} else
		{
			if(ordList.empty())
				return success;
			// Find pattern offsets
			pos = startPos;
			patOffset.reserve(ordList.size());
			bool patStart = false;
			while((pos = data.find_first_not_of(whitespace, pos)) != std::string::npos)
			{
				auto eol = data.find('\n', pos + 1);
				if(eol == std::string::npos)
					eol = data.size();
				if(data.substr(pos, 6) == "Rows: ")
				{
					patStart = true;
				} else if(data.substr(pos, 1) == "|" && patStart)
				{
					patOffset.push_back(pos);
					patStart = false;
				}
				pos = eol;
			}
			if(patOffset.empty())
				return success;
			startPos = patOffset[0];
		}
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
	const bool overflowPaste = (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OVERFLOWPASTE) && mode != pmPasteFlood && mode != pmPushForward && patternMode != kMultiInsert;
	const bool doITStyleMix = (mode == pmMixPasteIT);
	const bool doMixPaste = (mode == pmMixPaste) || doITStyleMix;
	const bool clipboardHasS3MCommands = (pasteFormat & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_S3M));
	const bool insertNewPatterns = overflowPaste && (patternMode == kMultiOverwrite);

	PatternCursor startPoint(startRow, startChan, PatternCursor::lastColumn), endPoint(startRow, startChan, PatternCursor::firstColumn);
	ModCommand *patData = sndFile.Patterns[pattern].GetpModCommand(startRow, 0);

	auto multiPastePos = ordList.cbegin();
	pos = startPos;

	while(curRow < sndFile.Patterns[pattern].GetNumRows() || patternMode != kSinglePaste)
	{
		// Parse next line
		pos = data.find_first_not_of(whitespace, pos);
		if(pos == std::string::npos)
		{
			// End of paste
			if(mode == pmPasteFlood && curRow != startRow && curRow < sndFile.Patterns[pattern].GetNumRows())
			{
				// Restarting pasting from beginning.
				pos = startPos;
				multiPastePos = ordList.cbegin();
				continue;
			} else
			{
				// Prevent infinite loop with malformed clipboard data.
				break;
			}
		}
		auto eol = data.find('\n', pos + 1);
		if(eol == std::string::npos)
			eol = data.size();

		// Handle multi-paste: Read pattern information
		if(patternMode != kSinglePaste)
		{
			// Parse pattern header lines
			bool parsedLine = true;
			if(data.substr(pos, 6) == "Rows: ")
			{
				pos += 6;
				// Advance to next pattern
				if(patternMode == kMultiOverwrite)
				{
					// In case of multi-pattern mix-paste, we know that we reached the end of the previous pattern and need to parse the next order now.
					multiPastePos++;
					if(multiPastePos == ordList.cend() || *multiPastePos >= patOffset.size())
						pos = data.size();
					else
						pos = patOffset[*multiPastePos];
					continue;
				}

				// Otherwise, parse this pattern header normally.
				do
				{
					if(curPattern >= patList.size())
					{
						return success;
					}
					pattern = patList[curPattern++];
				} while (pattern == PATTERNINDEX_INVALID);
				ROWINDEX numRows = ConvertStrTo<ROWINDEX>(data.substr(pos, 10));
				sndFile.Patterns[pattern].Resize(numRows);
				patData = sndFile.Patterns[pattern].GetpModCommand(0, 0);
				curRow = 0;
				prepareUndo = true;
			} else if(data.substr(pos, 6) == "Name: ")
			{
				pos += 6;
				auto name = mpt::String::RTrim(data.substr(pos, eol - pos - 1));
				sndFile.Patterns[pattern].SetName(name);
			} else if(data.substr(pos, 11) == "Signature: ")
			{
				pos += 11;
				auto pos2 = data.find("/", pos + 1);
				if(pos2 != std::string::npos)
				{
					pos2++;
					ROWINDEX rpb = ConvertStrTo<ROWINDEX>(data.substr(pos, pos2 - pos));
					ROWINDEX rpm = ConvertStrTo<ROWINDEX>(data.substr(pos2, eol - pos2));
					sndFile.Patterns[pattern].SetSignature(rpb, rpm);
				}
			} else if(data.substr(pos, 7) == "Swing: ")
			{
				pos += 7;
				TempoSwing swing;
				swing.resize(sndFile.Patterns[pattern].GetRowsPerBeat(), TempoSwing::Unity);
				size_t i = 0;
				while(pos != std::string::npos && pos < eol && i < swing.size())
				{
					swing[i++] = ConvertStrTo<TempoSwing::value_type>(data.substr(pos, eol - pos));
					pos = data.find(',', pos + 1);
					if(pos != std::string::npos)
						pos++;
				}
				sndFile.Patterns[pattern].SetTempoSwing(swing);
			} else
			{
				parsedLine = false;
			}
			if(parsedLine)
			{
				pos = eol;
				continue;
			}
		}
		if(data[pos] != '|')
		{
			// Not a valid line?
			pos = eol;
			continue;
		}

		success = true;
		col = startChan;
		// Paste columns
		while((pos + 11 < data.size()) && (data[pos] == '|'))
		{
			pos++;
			// Handle pasting large pattern into smaller pattern (e.g. 128-row pattern into MOD, which only allows 64 rows)
			ModCommand dummy;
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
					modDoc.GetPatternUndo().PrepareUndo(pattern, startChan, startRow, sndFile.GetNumChannels(), sndFile.Patterns[pattern].GetNumRows(), "Paste", !firstUndo);
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

				PatternCursor::Columns firstCol = PatternCursor::lastColumn, lastCol = PatternCursor::firstColumn;

				// Note
				if(data[pos] != ' ' && (!doMixPaste || ((!doITStyleMix && origModCmd.note == NOTE_NONE) || 
					(doITStyleMix && origModCmd.note == NOTE_NONE && origModCmd.instr == 0 && origModCmd.volcmd == VOLCMD_NONE))))
				{
					firstCol = PatternCursor::noteColumn;
					m.note = NOTE_NONE;
					if(data[pos] == '=')
						m.note = NOTE_KEYOFF;
					else if(data[pos] == '^')
						m.note = NOTE_NOTECUT;
					else if(data[pos] == '~')
						m.note = NOTE_FADE;
					else if(data[pos] == 'P')
					{
						if(data[pos + 2] == 'S' || data[pos + 2] == 's')
							m.note = NOTE_PCS;
						else
							m.note = NOTE_PC;
					} else if (data[pos] != '.')
					{
						// Check note names
						for(size_t i = 0; i < 12; i++)
						{
							if(data[pos] == NoteNamesSharp[i][0] && data[pos + 1] == NoteNamesSharp[i][1])
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
					firstCol = std::min(firstCol, PatternCursor::instrColumn);
					lastCol = std::max(lastCol, PatternCursor::instrColumn);
					if(data[pos + 3] >= '0' && data[pos + 3] <= ('0' + (MAX_INSTRUMENTS / 10)))
					{
						m.instr = (data[pos + 3] - '0') * 10 + (data[pos + 4] - '0');
					} else m.instr = 0;
				}

				// Volume
				if(data[pos + 5] > ' ' && (!doMixPaste || ((!doITStyleMix && origModCmd.volcmd == VOLCMD_NONE) || 
					(doITStyleMix && origModCmd.note == NOTE_NONE && origModCmd.instr == 0 && origModCmd.volcmd == VOLCMD_NONE))))
				{
					firstCol = std::min(firstCol, PatternCursor::volumeColumn);
					lastCol = std::max(lastCol, PatternCursor::volumeColumn);
					if(data[pos + 5] != '.')
					{
						if(m.IsPcNote())
						{
							m.SetValueVolCol(ConvertStrTo<uint16>(data.substr(pos + 5, 3)));
						} else
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
						firstCol = std::min(firstCol, PatternCursor::paramColumn);
						lastCol = std::max(lastCol, PatternCursor::paramColumn);
						m.SetValueEffectCol(ConvertStrTo<uint16>(data.substr(pos + 8, 3)));
					} else if(!origModCmd.IsPcNote())
					{
						// No value provided in clipboard
						if((m.command == CMD_MIDI || m.command == CMD_SMOOTHMIDI) && m.param < 128)
							m.SetValueEffectCol(static_cast<uint16>(Util::muldivr(m.param, ModCommand::maxColumnValue, 127)));
						else
							m.SetValueEffectCol(0);
					}
				} else
				{
					if(data[pos + 8] > ' ' && (!doMixPaste || ((!doITStyleMix && origModCmd.command == CMD_NONE) || 
						(doITStyleMix && origModCmd.command == CMD_NONE && origModCmd.param == 0))))
					{
						firstCol = std::min(firstCol, PatternCursor::effectColumn);
						lastCol = std::max(lastCol, PatternCursor::effectColumn);
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
						firstCol = std::min(firstCol, PatternCursor::paramColumn);
						lastCol = std::max(lastCol, PatternCursor::paramColumn);
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
								if(m.param < 32)
									m.command = CMD_SPEED;
								else
									m.command = CMD_TEMPO;
							} else
							{
								if(m.command == CMD_SPEED && m.param >= 32)
									m.param = CMD_TEMPO;
								else if(m.command == CMD_TEMPO && m.param < 32)
									m.param = CMD_SPEED;
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
								if(m.param  < 32)
									m.command = CMD_SPEED;
								else
									m.command = CMD_TEMPO;
							}
							break;
						}
					}
				}

				// Convert some commands, if necessary. With mix paste convert only
				// if the original modcommand was empty as otherwise the unchanged parts
				// of the old modcommand would falsely be interpreted being of type
				// origFormat and ConvertCommand could change them.
				if(pasteFormat != sndFile.GetType() && (!doMixPaste || origModCmd.IsEmpty()))
					m.Convert(pasteFormat, sndFile.GetType(), sndFile);

				// Sanitize PC events
				if(m.IsPcNote())
				{
					m.SetValueEffectCol(std::min<decltype(m.GetValueEffectCol())>(m.GetValueEffectCol(), ModCommand::maxColumnValue));
					m.SetValueVolCol(std::min<decltype(m.GetValueEffectCol())>(m.GetValueVolCol(), ModCommand::maxColumnValue));
				}

				// Adjust pattern selection
				if(col == startChan) startPoint.SetColumn(startChan, firstCol);
				if(endPoint.CompareColumn(PatternCursor(0, col, lastCol)) < 0) endPoint.SetColumn(col, lastCol);
				if(curRow > endPoint.GetRow()) endPoint.SetRow(curRow);
				pasteRect = PatternRect(startPoint, endPoint);
			}

			pos += 11;
			col++;
		}
		// Next row
		patData += sndFile.GetNumChannels();
		curRow++;
		pos = eol;

		if(overflowPaste)
		{
			// Handle overflow paste. Continue pasting in next pattern if enabled.
			// If Paste Flood is enabled, this won't be called due to obvious reasons.
			while(curRow >= sndFile.Patterns[pattern].GetNumRows())
			{
				curRow = 0;
				ORDERINDEX nextOrder = sndFile.Order.GetNextOrderIgnoringSkips(curOrder);
				if(nextOrder <= curOrder || nextOrder >= sndFile.Order.size() || !sndFile.Patterns.IsValidPat(sndFile.Order[nextOrder]))
				{
					PATTERNINDEX newPat;
					if(!insertNewPatterns
						|| curOrder >= sndFile.GetModSpecifications().ordersMax
						|| (newPat = sndFile.Patterns.InsertAny(sndFile.Patterns[pattern].GetNumRows(), true)) == PATTERNINDEX_INVALID
						|| sndFile.Order.Insert(curOrder + 1, 1, newPat) == 0)
					{
						return success;
					}
					nextOrder = curOrder + 1;
				}
				pattern = sndFile.Order[nextOrder];
				if(!sndFile.Patterns.IsValidPat(pattern)) return success;
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
	instance.activeClipboard = which;
	return ToSystemClipboard(instance.clipboards[instance.activeClipboard]);
}


// Switch to the next internal clipboard.
bool PatternClipboard::CycleForward()
//-----------------------------------
{
	instance.activeClipboard++;
	if(instance.activeClipboard >= instance.clipboards.size())
	{
		instance.activeClipboard = 0;
	}

	return SelectClipboard(instance.activeClipboard);
}


// Switch to the previous internal clipboard.
bool PatternClipboard::CycleBackward()
//------------------------------------
{
	if(instance.activeClipboard == 0)
	{
		instance.activeClipboard = instance.clipboards.size() - 1;
	} else
	{
		instance.activeClipboard--;
	}

	return SelectClipboard(instance.activeClipboard);
}


// Set the maximum number of internal clipboards.
void PatternClipboard::SetClipboardSize(clipindex_t maxEntries)
//-------------------------------------------------------------
{
	instance.clipboards.resize(maxEntries, PatternClipboardElement("", _T("unused")));
	LimitMax(instance.activeClipboard, maxEntries - 1);
}


// Check whether patterns can be pasted from clipboard
bool PatternClipboard::CanPaste()
//-------------------------------
{
	return !!IsClipboardFormatAvailable(CF_TEXT);
}



// System-specific clipboard functions
bool PatternClipboard::ToSystemClipboard(const std::string &data)
//---------------------------------------------------------------
{
	CMainFrame *mainFrame = CMainFrame::GetMainFrame();
	if(mainFrame == nullptr || !mainFrame->OpenClipboard())
	{
		return false;
	}

	HGLOBAL hCpy;
	if((hCpy = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, data.size() + 1)) == NULL)
	{
		return false;
	}

	::EmptyClipboard();

	void *clipboard;
	if((clipboard = ::GlobalLock(hCpy)) != nullptr)
	{
		memcpy(clipboard, data.c_str(), data.size() + 1);
		::GlobalUnlock(hCpy);
	}

	::SetClipboardData (CF_TEXT, (HANDLE)hCpy);
	::CloseClipboard();

	return (clipboard != nullptr);

}


// System-specific clipboard functions
bool PatternClipboard::FromSystemClipboard(std::string &data)
//-----------------------------------------------------------
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
		auto size = ::GlobalSize(hCpy);
		if(size > 0)
			data.assign(p, size - 1);
		::GlobalUnlock(hCpy);
	}

	::CloseClipboard();

	return(p != nullptr);
}


BEGIN_MESSAGE_MAP(PatternClipboardDialog, CDialog)
	ON_EN_UPDATE(IDC_EDIT1,		OnNumClipboardsChanged)
	ON_LBN_SELCHANGE(IDC_LIST1,	OnSelectClipboard)
	ON_LBN_DBLCLK(IDC_LIST1,	OnEditName)
END_MESSAGE_MAP()

PatternClipboardDialog PatternClipboardDialog::instance;

void PatternClipboardDialog::DoDataExchange(CDataExchange *pDX)
//-------------------------------------------------------------
{
	DDX_Control(pDX, IDC_SPIN1,	numClipboardsSpin);
	DDX_Control(pDX, IDC_LIST1,	clipList);
}


PatternClipboardDialog::PatternClipboardDialog() : isLocked(true), isCreated(false), posX(-1), editNameBox(*this)
//---------------------------------------------------------------------------------------------------------------
{
}


void PatternClipboardDialog::Show()
//---------------------------------
{
	instance.isLocked = true;
	if(!instance.isCreated)
	{
		instance.Create(IDD_CLIPBOARD, CMainFrame::GetMainFrame());
		instance.numClipboardsSpin.SetRange(0, int16_max);
	}
	instance.SetDlgItemInt(IDC_EDIT1, PatternClipboard::GetClipboardSize(), FALSE);
	instance.isLocked = false;
	instance.isCreated = true;
	instance.UpdateList();
	
	instance.SetWindowPos(nullptr, instance.posX, instance.posY, 0, 0, SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER | (instance.posX == -1 ? SWP_NOMOVE : 0));
}


void PatternClipboardDialog::OnNumClipboardsChanged()
//---------------------------------------------------
{
	if(isLocked)
	{
		return;
	}
	OnEndEdit();
	PatternClipboard::SetClipboardSize(GetDlgItemInt(IDC_EDIT1, nullptr, FALSE));
	UpdateList();
}


void PatternClipboardDialog::UpdateList()
//---------------------------------------
{
	if(instance.isLocked)
	{
		return;
	}
	instance.clipList.ResetContent();
	PatternClipboard::clipindex_t i = 0;
	for(const auto &clip : PatternClipboard::instance.clipboards)
	{
		const int item = instance.clipList.AddString(clip.description);
		instance.clipList.SetItemDataPtr(item, reinterpret_cast<void *>(i));
		if(PatternClipboard::instance.activeClipboard == i)
		{
			instance.clipList.SetCurSel(item);
		}
		i++;
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
	PatternClipboard::SelectClipboard(item);
	OnEndEdit();
}


void PatternClipboardDialog::OnOK()
//---------------------------------
{
	const CWnd *focus = GetFocus();
	if(focus == &editNameBox)
	{
		// User pressed enter in clipboard name edit box => cancel editing
		OnEndEdit();
	} else if(focus == &clipList)
	{
		// User pressed enter in the clipboard name list => start editing
		OnEditName();
	} else
	{
		CDialog::OnOK();
	}
}


void PatternClipboardDialog::OnCancel()
//-------------------------------------
{
	if(GetFocus() == &editNameBox)
	{
		// User pressed enter in clipboard name edit box => just cancel editing
		editNameBox.DestroyWindow();
		return;
	}

	OnEndEdit(false);

	isCreated = false;
	isLocked = true;

	RECT rect;
	GetWindowRect(&rect);
	posX = rect.left;
	posY = rect.top;

	DestroyWindow();
}


void PatternClipboardDialog::OnEditName()
//---------------------------------------
{
	OnEndEdit();

	const int sel = clipList.GetCurSel();
	if(sel == LB_ERR)
	{
		return;
	}

	CRect rect;
	clipList.GetItemRect(sel, rect);
	rect.InflateRect(0, 2, 0, 2);

	// Create the edit control
	editNameBox.Create(WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, rect, &clipList, 1);
	editNameBox.SetFont(clipList.GetFont());
	editNameBox.SetWindowText(PatternClipboard::instance.clipboards[sel].description);
	editNameBox.SetSel(0, -1, TRUE);
	editNameBox.SetFocus();
	SetWindowLongPtr(editNameBox.m_hWnd, GWLP_USERDATA, (LONG_PTR)clipList.GetItemDataPtr(sel));
}


void PatternClipboardDialog::OnEndEdit(bool apply)
//------------------------------------------------
{
	if(editNameBox.GetSafeHwnd() == NULL)
	{
		return;
	}

	if(apply)
	{
		size_t sel = GetWindowLongPtr(editNameBox.m_hWnd, GWLP_USERDATA);
		if(sel >= PatternClipboard::instance.clipboards.size())
		{
			// What happened?
			return;
		}

		CString newName;
		editNameBox.GetWindowText(newName);

		PatternClipboard::instance.clipboards[sel].description = newName;
	}

	SetWindowLongPtr(editNameBox.m_hWnd, GWLP_USERDATA, LONG_PTR(-1));
	editNameBox.DestroyWindow();

	UpdateList();
}


BEGIN_MESSAGE_MAP(PatternClipboardDialog::CInlineEdit, CEdit)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


PatternClipboardDialog::CInlineEdit::CInlineEdit(PatternClipboardDialog &dlg) : parent(dlg)
//-----------------------------------------------------------------------------------------
{
	CEdit::CEdit();
}


void PatternClipboardDialog::CInlineEdit::OnKillFocus(CWnd *newWnd)
//-----------------------------------------------------------------
{
	parent.OnEndEdit(true);
	CEdit::OnKillFocus(newWnd);
}


OPENMPT_NAMESPACE_END
