/*
 * CleanupSong.h
 * ---------------
 * Purpose: Dialog for cleaning up modules (rearranging, removing unused items).
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "DialogBase.h"

OPENMPT_NAMESPACE_BEGIN

class CModDoc;

class CModCleanupDlg : public DialogBase
{
private:
	enum CleanupOptions
	{
		// patterns
		kCleanupPatterns = 0,
		kRemovePatterns,
		kRearrangePatterns,
		kRemoveDuplicatePatterns,
		// orders
		kMergeSequences,
		kRemoveOrders,
		// samples
		kCleanupSamples,
		kRemoveSamples,
		kRearrangeSamples,
		kOptimizeSamples,
		// instruments
		kCleanupInstruments,
		kRemoveAllInstruments,
		// plugins
		kCleanupPlugins,
		kRemoveAllPlugins,
		// misc
		kResetVariables,
		kCleanupChannels,

		kNone,
		kMaxCleanupOptions = kNone
	};

	CModDoc &modDoc;
	static bool m_CheckBoxes[kMaxCleanupOptions]; // Checkbox state
	static const WORD m_CleanupIDtoDlgID[kMaxCleanupOptions]; // Checkbox -> Control ID LUT
	static const CleanupOptions m_MutuallyExclusive[kMaxCleanupOptions]; // Options that are mutually exclusive to each other.

	// Actual cleanup implementations:
	// Patterns
	bool RemoveDuplicatePatterns();
	bool RemoveUnusedPatterns(); // Remove unused patterns
	bool RearrangePatterns(); // Rearrange patterns
	bool RemoveAllPatterns();
	// Orders
	bool MergeSequences();
	bool RemoveAllOrders();
	// Samples
	bool RemoveUnusedSamples(); // Remove unused samples
	bool RemoveAllSamples();
	bool RearrangeSamples(); // Rearrange sample list
	bool OptimizeSamples(); // Remove unused sample data
	// Instruments
	bool RemoveUnusedInstruments(); // Remove unused instruments
	bool RemoveAllInstruments();
	// Plugins
	bool RemoveUnusedPlugins(); // Remove ununsed plugins
	bool RemoveAllPlugins();
	// Misc
	bool ResetVariables(); // Turn module into samplepack (convert to IT, remove patterns, etc.)
	bool RemoveUnusedChannels();

public:
	CModCleanupDlg(CModDoc &modParent, CWnd *parent);

protected:
	//{{AFX_VIRTUAL(CModCleanupDlg)
	BOOL OnInitDialog() override;
	void OnOK() override;
	CString GetToolTipText(UINT id, HWND hwnd) const override;
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CModCleanupDlg)
	afx_msg void OnPresetCleanupSong();
	afx_msg void OnPresetCompoCleanup();
	afx_msg void OnVerifyMutualExclusive();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
