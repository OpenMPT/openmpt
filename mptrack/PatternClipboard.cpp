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
#include "PatternCursor.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Clipboard.h"
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


std::string PatternClipboard::FormatClipboardHeader(const CSoundFile &sndFile)
{
	return "ModPlug Tracker " + GetFileExtension(sndFile.GetModSpecifications().fileExtension, true) + "\r\n";
}


// Copy a range of patterns to both the system clipboard and the internal clipboard.
bool PatternClipboard::Copy(const CSoundFile &sndFile, ORDERINDEX first, ORDERINDEX last, bool onlyOrders)
{
	const ModSequence &order = sndFile.Order();
	LimitMax(first, order.GetLength());
	LimitMax(last, order.GetLength());

	// Set up clipboard header.
	std::string data = FormatClipboardHeader(sndFile) + "Orders: ";
	std::string patternData;

	// Pattern => Order list assignment
	std::vector<PATTERNINDEX> patList(sndFile.Patterns.Size(), PATTERNINDEX_INVALID);
	PATTERNINDEX insertedPats = 0;

	// Add order list and pattern information to header.
	for(ORDERINDEX ord = first; ord <= last; ord++)
	{
		PATTERNINDEX pattern = order[ord];

		if(ord != first)
			data += ',';
		
		if(pattern == order.GetInvalidPatIndex())
		{
			data += '-';
		} else if(pattern == order.GetIgnoreIndex())
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
				patternData += MPT_AFORMAT("Rows: {}\r\n")(pat.GetNumRows());
				std::string name = pat.GetName();
				if(!name.empty())
				{
					patternData += "Name: " + name + "\r\n";
				}
				if(pat.GetOverrideSignature())
				{
					patternData += MPT_AFORMAT("Signature: {}/{}\r\n")(pat.GetRowsPerBeat(), pat.GetRowsPerMeasure());
				}
				if(pat.HasTempoSwing())
				{
					patternData += "Swing: ";
					const TempoSwing &swing = pat.GetTempoSwing();
					for(size_t i = 0; i < swing.size(); i++)
					{
						if(i == 0)
						{
							patternData += MPT_AFORMAT("{}")(swing[i]);
						} else
						{
							patternData += MPT_AFORMAT(",{}")(swing[i]);
						}
					}
					patternData += "\r\n";
				}
				patternData += CreateClipboardString(sndFile, pattern, PatternRect(PatternCursor(), PatternCursor(sndFile.Patterns[pattern].GetNumRows() - 1, sndFile.GetNumChannels() - 1, PatternCursor::lastColumn)));
			}

			data += mpt::afmt::val(patList[pattern]);
		}
	}
	if(!onlyOrders)
	{
		data += "\r\n" + patternData;
	}
	
	if(instance.m_activeClipboard < instance.m_clipboards.size())
	{
		// Copy to internal clipboard
		CString desc = MPT_CFORMAT("{} {} ({} to {})")(last - first + 1, onlyOrders ? CString(_T("Orders")) : CString(_T("Patterns")), first, last);
		instance.m_clipboards[instance.m_activeClipboard] = {data, desc};
	}

	return ToSystemClipboard(data);
}


// Copy a pattern selection to both the system clipboard and the internal clipboard.
bool PatternClipboard::Copy(const CSoundFile &sndFile, PATTERNINDEX pattern, PatternRect selection)
{
	std::string data = CreateClipboardString(sndFile, pattern, selection);
	if(data.empty())
		return false;

	// Set up clipboard header
	data.insert(0, FormatClipboardHeader(sndFile));

	if(instance.m_activeClipboard < instance.m_clipboards.size())
	{
		// Copy to internal clipboard
		CString desc;
		desc.Format(_T("%u rows, %u channels (pattern %u)"), selection.GetNumRows(), selection.GetNumChannels(), pattern);
		instance.m_clipboards[instance.m_activeClipboard] = {data, desc};
	}

	return ToSystemClipboard(data);
}


// Copy a pattern or pattern channel to the internal pattern or channel clipboard.
bool PatternClipboard::Copy(const CSoundFile &sndFile, PATTERNINDEX pattern, CHANNELINDEX channel)
{
	if(!sndFile.Patterns.IsValidPat(pattern))
		return false;

	const bool patternCopy = (channel == CHANNELINDEX_INVALID);
	const CPattern &pat = sndFile.Patterns[pattern];
	PatternRect selection;
	if(patternCopy)
		selection = {PatternCursor(0, 0, PatternCursor::firstColumn), PatternCursor(pat.GetNumRows() - 1, pat.GetNumChannels() - 1, PatternCursor::lastColumn)};
	else
		selection = {PatternCursor(0, channel, PatternCursor::firstColumn), PatternCursor(pat.GetNumRows() - 1, channel, PatternCursor::lastColumn)};

	std::string data = CreateClipboardString(sndFile, pattern, selection);
	if(data.empty())
		return false;

	// Set up clipboard header
	data.insert(0, FormatClipboardHeader(sndFile));

	// Copy to internal clipboard
	(patternCopy ? instance.m_patternClipboard : instance.m_channelClipboard) = {data, {}};
	return true;
}


// Create the clipboard text for a pattern selection
std::string PatternClipboard::CreateClipboardString(const CSoundFile &sndFile, PATTERNINDEX pattern, PatternRect selection)
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
					data += mpt::ToCharset(mpt::Charset::Locale, mpt::ustring(NoteNamesSharp[(m->note - NOTE_MIN) % 12]));
					data += ('0' + (m->note - NOTE_MIN) / 12);
				} else
				{
					data += mpt::ToCharset(mpt::Charset::Locale, sndFile.GetNoteName(m->note));
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
					data += mpt::afmt::dec0<3>(m->GetValueVolCol());
				}
				else
				{
					if(m->volcmd != VOLCMD_NONE && m->vol <= 99)
					{
						data += sndFile.GetModSpecifications().GetVolEffectLetter(m->volcmd);
						data += mpt::afmt::dec0<2>(m->vol);
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
					data += mpt::afmt::dec0<3>(m->GetValueEffectCol());
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
						data += mpt::afmt::HEX0<2>(m->param);
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
bool PatternClipboard::Paste(CSoundFile &sndFile, PatternEditPos &pastePos, PasteModes mode, PatternRect &pasteRect, bool &orderChanged)
{
	std::string data;
	if(!FromSystemClipboard(data) || !HandlePaste(sndFile, pastePos, mode, data, pasteRect, orderChanged))
	{
		// Fall back to internal clipboard if there's no valid pattern data in the system clipboard.
		return Paste(sndFile, pastePos, mode, pasteRect, instance.m_activeClipboard, orderChanged);
	}
	return true;
}


// Try pasting a pattern selection from an internal clipboard.
bool PatternClipboard::Paste(CSoundFile &sndFile, PatternEditPos &pastePos, PasteModes mode, PatternRect &pasteRect, clipindex_t internalClipboard, bool &orderChanged)
{
	if(internalClipboard >= instance.m_clipboards.size())
		return false;
	
	return HandlePaste(sndFile, pastePos, mode, instance.m_clipboards[internalClipboard].content, pasteRect, orderChanged);
}


// Paste from pattern or channel clipboard.
bool PatternClipboard::Paste(CSoundFile &sndFile, PATTERNINDEX pattern, CHANNELINDEX channel)
{
	PatternEditPos pastePos{0, ORDERINDEX_INVALID, pattern, channel != CHANNELINDEX_INVALID ? channel : CHANNELINDEX(0)};
	PatternRect pasteRect;
	bool orderChanged = false;
	return HandlePaste(sndFile, pastePos, pmOverwrite, (channel == CHANNELINDEX_INVALID ? instance.m_patternClipboard : instance.m_channelClipboard).content, pasteRect, orderChanged);
}


// Parse clipboard string and perform the pasting operation.
bool PatternClipboard::HandlePaste(CSoundFile &sndFile, PatternEditPos &pastePos, PasteModes mode, const std::string &data, PatternRect &pasteRect, bool &orderChanged)
{
	const std::string whitespace(" \n\r\t");
	PATTERNINDEX pattern = pastePos.pattern;
	ORDERINDEX &curOrder = pastePos.order;
	orderChanged = false;
	if(sndFile.GetpModDoc() == nullptr)
		return false;

	CModDoc &modDoc = *(sndFile.GetpModDoc());
	ModSequence &order = sndFile.Order();

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
		const std::string format = mpt::ToUpperCaseAscii(mpt::trim(data.substr(startPos, 3)));

		for(const auto &spec : ModSpecs::Collection)
		{
			if(format == GetFileExtension(spec->fileExtension, false))
			{
				pasteFormat = spec->internalType;
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
		if(order.IsValidPat(curOrder) && patternMode == kMultiInsert)
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
				insertPat = order.GetIgnoreIndex();
			} else if(data[curPos] == '-')
			{
				insertPat = order.GetInvalidPatIndex();
			} else
			{
				insertPat = ConvertStrTo<PATTERNINDEX>(data.substr(curPos, 10));
				if(patternMode == kMultiOverwrite)
				{
					// We only want the order of pasted patterns now, do not create any new patterns
					ordList.push_back(insertPat);
					continue;
				}

				if(insertPat < patList.size() && patList[insertPat] != PATTERNINDEX_INVALID)
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

					patList[insertPat] = modDoc.InsertPattern(64);
					insertPat = patList[insertPat];
				}
			}

			if((insertPat == order.GetIgnoreIndex() && !sndFile.GetModSpecifications().hasIgnoreIndex)
				|| (insertPat == order.GetInvalidPatIndex() && !sndFile.GetModSpecifications().hasStopIndex)
				|| insertPat == PATTERNINDEX_INVALID
				|| patternMode == kMultiOverwrite)
			{
				continue;
			}

			if(order.insert(writeOrder, 1) == 0)
			{
				break;
			}
			order[writeOrder++] = insertPat;
			orderChanged = true;
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
	const bool overflowPaste = (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OVERFLOWPASTE) && mode != pmPasteFlood && mode != pmPushForward && patternMode != kMultiInsert && curOrder != ORDERINDEX_INVALID;
	const bool doITStyleMix = (mode == pmMixPasteIT);
	const bool doMixPaste = (mode == pmMixPaste) || doITStyleMix;
	const bool clipboardHasS3MCommands = (pasteFormat & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_S3M));
	const bool insertNewPatterns = overflowPaste && (patternMode == kMultiOverwrite);

	PatternCursor startPoint(startRow, startChan, PatternCursor::lastColumn), endPoint(startRow, startChan, PatternCursor::firstColumn);
	ModCommand *patData = sndFile.Patterns[pattern].GetpModCommand(startRow, 0);

	auto multiPastePos = ordList.cbegin();
	pos = startPos;

	while(curRow < sndFile.Patterns[pattern].GetNumRows() || overflowPaste || patternMode == kMultiInsert)
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
				auto name = mpt::trim_right(data.substr(pos, eol - pos - 1));
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

		if(overflowPaste)
		{
			// Handle overflow paste. Continue pasting in next pattern if enabled.
			// If Paste Flood is enabled, this won't be called due to obvious reasons.
			while(curRow >= sndFile.Patterns[pattern].GetNumRows())
			{
				curRow = 0;
				ORDERINDEX nextOrder = order.GetNextOrderIgnoringSkips(curOrder);
				if(nextOrder <= curOrder || !order.IsValidPat(nextOrder))
				{
					PATTERNINDEX newPat;
					if(!insertNewPatterns
						|| curOrder >= sndFile.GetModSpecifications().ordersMax
						|| (newPat = modDoc.InsertPattern(sndFile.Patterns[pattern].GetNumRows())) == PATTERNINDEX_INVALID
						|| order.insert(curOrder + 1, 1, newPat) == 0)
					{
						return success;
					}
					orderChanged = true;
					nextOrder = curOrder + 1;
				}
				pattern = order[nextOrder];
				if(!sndFile.Patterns.IsValidPat(pattern)) return success;
				patData = sndFile.Patterns[pattern].GetpModCommand(0, 0);
				curOrder = nextOrder;
				prepareUndo = true;
				startRow = 0;
			}
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
						for(uint8 i = 0; i < 12; i++)
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
							for(int i = VOLCMD_NONE + 1; i < MAX_VOLCMDS; i++)
							{
								const char cmd = sourceSpecs.GetVolEffectLetter(static_cast<VolumeCommand>(i));
								if(data[pos + 5] == cmd && cmd != '?')
								{
									m.volcmd = static_cast<VolumeCommand>(i);
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
							for(int i = CMD_NONE + 1; i < MAX_EFFECTS; i++)
							{
								const char cmd = sourceSpecs.GetEffectLetter(static_cast<EffectCommand>(i));
								if(data[pos + 8] == cmd && cmd != '?')
								{
									m.command = static_cast<EffectCommand>(i);
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
							for(uint8 i = 0; i < 16; i++)
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
					m.SetValueEffectCol(std::min(m.GetValueEffectCol(), static_cast<decltype(m.GetValueEffectCol())>(ModCommand::maxColumnValue)));
					m.SetValueVolCol(std::min(m.GetValueVolCol(), static_cast<decltype(m.GetValueEffectCol())>(ModCommand::maxColumnValue)));
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
	}

	return success;
}


// Copy one of the internal clipboards to the system clipboard.
bool PatternClipboard::SelectClipboard(clipindex_t which)
{
	instance.m_activeClipboard = which;
	return ToSystemClipboard(instance.m_clipboards[instance.m_activeClipboard]);
}


// Switch to the next internal clipboard.
bool PatternClipboard::CycleForward()
{
	instance.m_activeClipboard++;
	if(instance.m_activeClipboard >= instance.m_clipboards.size())
		instance.m_activeClipboard = 0;

	return SelectClipboard(instance.m_activeClipboard);
}


// Switch to the previous internal clipboard.
bool PatternClipboard::CycleBackward()
{
	if(instance.m_activeClipboard == 0)
		instance.m_activeClipboard = instance.m_clipboards.size() - 1;
	else
		instance.m_activeClipboard--;

	return SelectClipboard(instance.m_activeClipboard);
}


// Set the maximum number of internal clipboards.
void PatternClipboard::SetClipboardSize(clipindex_t maxEntries)
{
	instance.m_clipboards.resize(maxEntries, {"", _T("unused")});
	LimitMax(instance.m_activeClipboard, maxEntries - 1);
}


// Check whether patterns can be pasted from clipboard
bool PatternClipboard::CanPaste()
{
	return !!IsClipboardFormatAvailable(CF_TEXT);
}



// System-specific clipboard functions
bool PatternClipboard::ToSystemClipboard(const std::string_view &data)
{
	Clipboard clipboard(CF_TEXT, data.size() + 1);
	if(auto dst = clipboard.As<char>())
	{
		std::copy(data.begin(), data.end(), dst);
		dst[data.size()] = '\0';
		return true;
	}
	return false;
}


// System-specific clipboard functions
bool PatternClipboard::FromSystemClipboard(std::string &data)
{
	Clipboard clipboard(CF_TEXT);
	if(auto cbdata = clipboard.Get(); cbdata.data())
	{
		if(cbdata.size() > 0)
			data.assign(mpt::byte_cast<char *>(cbdata.data()), cbdata.size() - 1);
		return !data.empty();
	}
	return false;
}


BEGIN_MESSAGE_MAP(PatternClipboardDialog, ResizableDialog)
	ON_EN_UPDATE(IDC_EDIT1,     &PatternClipboardDialog::OnNumClipboardsChanged)
	ON_LBN_SELCHANGE(IDC_LIST1, &PatternClipboardDialog::OnSelectClipboard)
	ON_LBN_DBLCLK(IDC_LIST1,    &PatternClipboardDialog::OnEditName)
END_MESSAGE_MAP()

PatternClipboardDialog PatternClipboardDialog::instance;

void PatternClipboardDialog::DoDataExchange(CDataExchange *pDX)
{
	DDX_Control(pDX, IDC_SPIN1, m_numClipboardsSpin);
	DDX_Control(pDX, IDC_LIST1, m_clipList);
}


PatternClipboardDialog::PatternClipboardDialog() : m_editNameBox(*this)
{
}


void PatternClipboardDialog::Show()
{
	instance.m_isLocked = true;
	if(!instance.m_isCreated)
	{
		instance.Create(IDD_CLIPBOARD, CMainFrame::GetMainFrame());
		instance.m_numClipboardsSpin.SetRange(0, int16_max);
	}
	instance.SetDlgItemInt(IDC_EDIT1, mpt::saturate_cast<UINT>(PatternClipboard::GetClipboardSize()), FALSE);
	instance.m_isLocked = false;
	instance.m_isCreated = true;
	instance.UpdateList();
	
	instance.SetWindowPos(nullptr, instance.m_posX, instance.m_posY, 0, 0, SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER | (instance.m_posX == -1 ? SWP_NOMOVE : 0));
}


void PatternClipboardDialog::OnNumClipboardsChanged()
{
	if(m_isLocked)
	{
		return;
	}
	OnEndEdit();
	PatternClipboard::SetClipboardSize(GetDlgItemInt(IDC_EDIT1, nullptr, FALSE));
	UpdateList();
}


void PatternClipboardDialog::UpdateList()
{
	if(instance.m_isLocked)
	{
		return;
	}
	instance.m_clipList.ResetContent();
	PatternClipboard::clipindex_t i = 0;
	for(const auto &clip : PatternClipboard::instance.m_clipboards)
	{
		const int item = instance.m_clipList.AddString(clip.description);
		instance.m_clipList.SetItemDataPtr(item, reinterpret_cast<void *>(i));
		if(PatternClipboard::instance.m_activeClipboard == i)
		{
			instance.m_clipList.SetCurSel(item);
		}
		i++;
	}
}


void PatternClipboardDialog::OnSelectClipboard()
{
	if(m_isLocked)
	{
		return;
	}
	PatternClipboard::clipindex_t item = reinterpret_cast<PatternClipboard::clipindex_t>(m_clipList.GetItemDataPtr(m_clipList.GetCurSel()));
	PatternClipboard::SelectClipboard(item);
	OnEndEdit();
}


void PatternClipboardDialog::OnOK()
{
	const CWnd *focus = GetFocus();
	if(focus == &m_editNameBox)
	{
		// User pressed enter in clipboard name edit box => cancel editing
		OnEndEdit();
	} else if(focus == &m_clipList)
	{
		// User pressed enter in the clipboard name list => start editing
		OnEditName();
	} else
	{
		ResizableDialog::OnOK();
	}
}


void PatternClipboardDialog::OnCancel()
{
	if(GetFocus() == &m_editNameBox)
	{
		// User pressed enter in clipboard name edit box => just cancel editing
		m_editNameBox.DestroyWindow();
		return;
	}

	OnEndEdit(false);

	m_isCreated = false;
	m_isLocked = true;

	RECT rect;
	GetWindowRect(&rect);
	m_posX = rect.left;
	m_posY = rect.top;

	DestroyWindow();
}


void PatternClipboardDialog::OnEditName()
{
	OnEndEdit();

	const int sel = m_clipList.GetCurSel();
	if(sel == LB_ERR)
	{
		return;
	}

	CRect rect;
	m_clipList.GetItemRect(sel, rect);
	rect.InflateRect(0, 2, 0, 2);

	// Create the edit control
	m_editNameBox.Create(WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, rect, &m_clipList, 1);
	m_editNameBox.SetFont(m_clipList.GetFont());
	m_editNameBox.SetWindowText(PatternClipboard::instance.m_clipboards[sel].description);
	m_editNameBox.SetSel(0, -1, TRUE);
	m_editNameBox.SetFocus();
	SetWindowLongPtr(m_editNameBox.m_hWnd, GWLP_USERDATA, (LONG_PTR)m_clipList.GetItemDataPtr(sel));
}


void PatternClipboardDialog::OnEndEdit(bool apply)
{
	if(m_editNameBox.GetSafeHwnd() == NULL)
	{
		return;
	}

	if(apply)
	{
		size_t sel = GetWindowLongPtr(m_editNameBox.m_hWnd, GWLP_USERDATA);
		if(sel >= PatternClipboard::instance.m_clipboards.size())
		{
			// What happened?
			return;
		}

		CString newName;
		m_editNameBox.GetWindowText(newName);

		PatternClipboard::instance.m_clipboards[sel].description = newName;
	}

	SetWindowLongPtr(m_editNameBox.m_hWnd, GWLP_USERDATA, LONG_PTR(-1));
	m_editNameBox.DestroyWindow();

	UpdateList();
}


BEGIN_MESSAGE_MAP(PatternClipboardDialog::CInlineEdit, CEdit)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


PatternClipboardDialog::CInlineEdit::CInlineEdit(PatternClipboardDialog &dlg) : parent(dlg)
{
}


void PatternClipboardDialog::CInlineEdit::OnKillFocus(CWnd *newWnd)
{
	parent.OnEndEdit(true);
	CEdit::OnKillFocus(newWnd);
}


OPENMPT_NAMESPACE_END
