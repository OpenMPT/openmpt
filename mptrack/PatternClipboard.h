/*
 * PatternClipboard.h
 * ------------------
 * Purpose: Implementation of the pattern clipboard mechanism
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <deque>
#include "Sndfile.h"
#include "PatternCursor.h"

//===========================
class PatternClipboardElement
//===========================
{
public:
	
	CString content;

public:

	PatternClipboardElement() { };
	PatternClipboardElement(CString &data) : content(data) { };
};


//====================
class PatternClipboard
//====================
{
public:

	enum PasteModes
	{
		pmOverwrite = 0,
		pmMixPaste,
		pmMixPasteIT,
		pmPasteFlood,
		pmPushForward,
	};

	// Clipboard index type
	typedef size_t clipindex_t;

protected:

	// Maximum number of internal clipboard entries
	static clipindex_t clipboardSize;
	// Active internal clipboard index
	clipindex_t activeClipboard;
	// Internal clipboard collection
	std::deque<PatternClipboardElement> clipboards;

public:

	PatternClipboard() : activeClipboard(0) { };

	// Copy a pattern selection to both the system clipboard and the internal clipboard.
	bool Copy(CSoundFile &sndFile, PATTERNINDEX pattern, PatternRect selection);
	// Try pasting a pattern selection from the system clipboard.
	bool Paste(CSoundFile &sndFile, PATTERNINDEX pattern, const PatternCursor &pastePos, PasteModes mode);
	// Try pasting a pattern selection from an internal clipboard.
	bool Paste(CSoundFile &sndFile, PATTERNINDEX pattern, const PatternCursor &pastePos, PasteModes mode, clipindex_t internalClipboard);
	// Copy one of the internal clipboards to the system clipboard.
	bool SelectClipboard(clipindex_t which);
	// Switch to the next internal clipboard.
	bool CycleForward();
	// Switch to the previous internal clipboard.
	bool CycleBackward();
	// Set the maximum number of internal clipboards.
	void SetClipboardSize(clipindex_t maxEntries);
	// Return the current number of clipboards.
	clipindex_t GetClipboardSize() const { return clipboards.size(); };
	// Remove all clipboard contents.
	void Clear();

protected:

	CString GetFileExtension(const char *ext) const;
	// Perform the pasting operation.
	bool HandlePaste(CSoundFile &sndFile, PATTERNINDEX pattern, const PatternCursor &pastePos, PasteModes mode, const CString &data);

	// Keep the number of clipboards consistent with the maximum number of allowed clipboards.
	void RestrictClipboardSize();

	// System-specific clipboard functions
	bool ToSystemClipboard(const PatternClipboardElement &clipboard) { return ToSystemClipboard(clipboard.content); };
	bool ToSystemClipboard(const CString &data);
	bool FromSystemClipboard(CString &data);
};


//==========================
class PatternClipboardDialog
//==========================
{
protected:

	PatternClipboard &clipboards;

public:

	PatternClipboardDialog(PatternClipboard &c) : clipboards(c) { };
};
