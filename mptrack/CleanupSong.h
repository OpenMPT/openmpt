/*
 * CleanupSong.h
 * ---------------
 * Purpose: Dialog for cleaning up modules (rearranging, removing unused items).
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

//==================================
class CModCleanupDlg: public CDialog
//==================================
{
private:
	enum ENUM_CLEANUP_OPTIONS
	{
		// patterns
		CU_CLEANUP_PATTERNS = 0,
		CU_REMOVE_PATTERNS,
		CU_REARRANGE_PATTERNS,
		CU_REMOVE_DUPLICATE_PATTERNS,
		// orders
		CU_MERGE_SEQUENCES,
		CU_REMOVE_ORDERS,
		// samples
		CU_CLEANUP_SAMPLES,
		CU_REMOVE_SAMPLES,
		CU_REARRANGE_SAMPLES,
		CU_OPTIMIZE_SAMPLES,
		// instruments
		CU_CLEANUP_INSTRUMENTS,
		CU_REMOVE_INSTRUMENTS,
		// plugins
		CU_CLEANUP_PLUGINS,
		CU_REMOVE_PLUGINS,
		// misc
		CU_RESET_VARIABLES,

		CU_NONE,
		CU_MAX_CLEANUP_OPTIONS = CU_NONE
	};

	CModDoc &modDoc;
	static bool m_bCheckBoxes[CU_MAX_CLEANUP_OPTIONS]; // Checkbox state
	static const WORD m_nCleanupIDtoDlgID[CU_MAX_CLEANUP_OPTIONS]; // Checkbox -> Control ID LUT
	static const ENUM_CLEANUP_OPTIONS m_nMutuallyExclusive[CU_MAX_CLEANUP_OPTIONS]; // Options that are mutually exclusive to each other.

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

public:
	CModCleanupDlg(CModDoc &modParent, CWnd *parent) : CDialog(IDD_CLEANUP_SONG, parent), modDoc(modParent) { }

protected:
	//{{AFX_VIRTUAL(CModCleanupDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_VIRTUAL

	BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);

	//{{AFX_MSG(CModCleanupDlg)
	afx_msg void OnPresetCleanupSong();
	afx_msg void OnPresetCompoCleanup();
	afx_msg void OnVerifyMutualExclusive();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
