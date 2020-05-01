/*
 * PatternClipboard.h
 * ------------------
 * Purpose: Implementation of the pattern clipboard mechanism
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "ResizableDialog.h"
#include "../soundlib/Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

struct PatternEditPos;
class PatternRect;
class CSoundFile;

struct PatternClipboardElement
{
	std::string content;
	CString description;
};


class PatternClipboard
{
	friend class PatternClipboardDialog;

public:
	enum PasteModes
	{
		pmOverwrite,
		pmMixPaste,
		pmMixPasteIT,
		pmPasteFlood,
		pmPushForward,
	};

	// Clipboard index type
	using clipindex_t = size_t;

protected:
	// The one and only pattern clipboard object
	static PatternClipboard instance;

	// Active internal clipboard index
	clipindex_t m_activeClipboard = 0;
	// Internal clipboard collection
	std::vector<PatternClipboardElement> m_clipboards;
	PatternClipboardElement m_channelClipboard;
	PatternClipboardElement m_patternClipboard;

public:
	// Copy a range of patterns to both the system clipboard and the internal clipboard.
	static bool Copy(const CSoundFile &sndFile, ORDERINDEX first, ORDERINDEX last, bool onlyOrders);
	// Copy a pattern selection to both the system clipboard and the internal clipboard.
	static bool Copy(const CSoundFile &sndFile, PATTERNINDEX pattern, PatternRect selection);
	// Copy a pattern or pattern channel to the internal pattern or channel clipboard.
	static bool Copy(const CSoundFile &sndFile, PATTERNINDEX pattern, CHANNELINDEX channel = CHANNELINDEX_INVALID);
	// Try pasting a pattern selection from the system clipboard.
	static bool Paste(CSoundFile &sndFile, PatternEditPos &pastePos, PasteModes mode, PatternRect &pasteRect, bool &orderChanged);
	// Try pasting a pattern selection from an internal clipboard.
	static bool Paste(CSoundFile &sndFile, PatternEditPos &pastePos, PasteModes mode, PatternRect &pasteRect, clipindex_t internalClipboard, bool &orderChanged);
	// Paste from pattern or channel clipboard.
	static bool Paste(CSoundFile &sndFile, PATTERNINDEX pattern, CHANNELINDEX channel = CHANNELINDEX_INVALID);
	// Copy one of the internal clipboards to the system clipboard.
	static bool SelectClipboard(clipindex_t which);
	// Switch to the next internal clipboard.
	static bool CycleForward();
	// Switch to the previous internal clipboard.
	static bool CycleBackward();
	// Set the maximum number of internal clipboards.
	static void SetClipboardSize(clipindex_t maxEntries);
	// Return the current number of clipboards.
	static clipindex_t GetClipboardSize() { return instance.m_clipboards.size(); };
	// Check whether patterns can be pasted from clipboard
	static bool CanPaste();

protected:
	PatternClipboard() { SetClipboardSize(1); };

	static std::string GetFileExtension(const char *ext, bool addPadding);

	static std::string FormatClipboardHeader(const CSoundFile &sndFile);

	// Create the clipboard text for a pattern selection
	static std::string CreateClipboardString(const CSoundFile &sndFile, PATTERNINDEX pattern, PatternRect selection);

	// Parse clipboard string and perform the pasting operation.
	static bool HandlePaste(CSoundFile &sndFile, PatternEditPos &pastePos, PasteModes mode, const std::string &data, PatternRect &pasteRect, bool &orderChanged);

	// System-specific clipboard functions
	static bool ToSystemClipboard(const PatternClipboardElement &clipboard) { return ToSystemClipboard(clipboard.content); };
	static bool ToSystemClipboard(const std::string_view &data);
	static bool FromSystemClipboard(std::string &data);
};


class PatternClipboardDialog : public ResizableDialog
{
protected:
	// The one and only pattern clipboard dialog object
	static PatternClipboardDialog instance;

	// Edit class for clipboard name editing
	class CInlineEdit : public CEdit
	{
	protected:
		PatternClipboardDialog &parent;

	public:
		CInlineEdit(PatternClipboardDialog &dlg);

		DECLARE_MESSAGE_MAP();

		afx_msg void OnKillFocus(CWnd *newWnd);
	};

	CSpinButtonCtrl m_numClipboardsSpin;
	CListBox m_clipList;
	CInlineEdit m_editNameBox;
	int m_posX = -1, m_posY = -1;
	bool m_isLocked = true, m_isCreated = false;

public:
	static void UpdateList();
	static void Show();
	static void Toggle()
	{
		if(instance.m_isCreated)
			instance.OnCancel();
		else
			instance.Show();
	}

protected:
	PatternClipboardDialog();

	void DoDataExchange(CDataExchange *pDX) override;

	void OnOK() override;
	void OnCancel() override;

	afx_msg void OnNumClipboardsChanged();
	afx_msg void OnSelectClipboard();

	// Edit clipboard name
	afx_msg void OnEditName();
	void OnEndEdit(bool apply = true);

	DECLARE_MESSAGE_MAP();
};

OPENMPT_NAMESPACE_END
