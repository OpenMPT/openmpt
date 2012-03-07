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

protected:

	CSoundFile &sndFile;

	static size_t clipboardSize;
	size_t activeClipboard;
	std::deque<PatternClipboardElement> clipboards;

public:

	PatternClipboard(CSoundFile &owner) : sndFile(owner), activeClipboard(0) { };

	// Copy a pattern selection to both the system clipboard and the internal clipboard.
	bool Copy(PATTERNINDEX pattern, PatternRect selection);
	// Try pasting a pattern selection from the system clipboard.
	bool Paste(PATTERNINDEX pattern, const PatternCursor &pastePos, PasteModes mode);
	// Try pasting a pattern selection from an internal clipboard.
	bool Paste(PATTERNINDEX pattern, const PatternCursor &pastePos, PasteModes mode, size_t internalClipboard);
	// Copy one of the internal clipboards to the system clipboard.
	bool SelectClipboard(size_t which);
	// Switch to the next internal clipboard.
	bool CycleForward();
	// Switch to the previous internal clipboard.
	bool CycleBackward();
	// Set the maximum number of internal clipboards.
	void SetClipboardSize(size_t maxEntries);
	// Return the current number of clipboards.
	size_t GetClipboardSize() const { return clipboards.size(); };
	// Remove all clipboard contents.
	void Clear();

protected:

	CString GetFileExtension(const char *ext) const;
	// Perform the pasting operation.
	bool HandlePaste(PATTERNINDEX pattern, const PatternCursor &pastePos, PasteModes mode, const CString &data);

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
