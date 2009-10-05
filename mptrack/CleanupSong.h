/*
 *
 * CleanupSong.h
 * ---------------
 * Purpose: Header file for CleanupSong.cpp
 *
 */

#pragma once

enum ENUM_CLEANUP_OPTIONS
{
	// remove unused
	CU_CLEANUP_PATTERNS = 0,
	CU_CLEANUP_SAMPLES,
	CU_CLEANUP_INSTRUMENTS,
	CU_CLEANUP_PLUGINS,
	// remove all
	CU_REMOVE_PATTERNS,
	CU_REMOVE_ORDERS,
	CU_REMOVE_SAMPLES,
	CU_REMOVE_INSTRUMENTS,
	CU_REMOVE_PLUGINS,
	// misc
	CU_REARRANGE_PATTERNS,
	CU_REARRANGE_SAMPLES,
	CU_OPTIMIZE_SAMPLES,
	CU_RESET_VARIABLES,

	CU_MAX_CLEANUP_OPTIONS
};

//==================================
class CModCleanupDlg: public CDialog
//==================================
{
private:
	CModDoc *m_pModDoc;
	CWnd *m_wParent;
	static bool m_bCheckBoxes[CU_MAX_CLEANUP_OPTIONS]; // Checkbox state
	static const WORD m_nCleanupIDtoDlgID[CU_MAX_CLEANUP_OPTIONS]; // Checkbox -> Control ID LUT

	// Actual cleanup implementations
	bool RemoveUnusedPatterns(bool bRemove = true); // Remove unused patterns / rearrange patterns
	bool RemoveUnusedSamples(); // Remove unused samples
	bool RemoveUnusedInstruments(); // Remove unused instruments
	bool RemoveUnusedPlugins(); // Remove ununsed plugins
	// Zap
	bool RemoveAllPatterns();
	bool RemoveAllOrders();
	bool RemoveAllSamples();
	bool RemoveAllInstruments(bool bConfirm = true);
	bool RemoveAllPlugins();
	// Misc
	bool RearrangeSamples(); // Rearrange sample list
	bool OptimizeSamples(); // Remove unused sample data
	bool ResetVariables(); // Turn module into samplepack (convert to IT, remove patterns, etc.)

public:
	CModCleanupDlg(CModDoc *pModDoc, CWnd *parent):CDialog(IDD_CLEANUP_SONG, parent) { m_pModDoc = pModDoc; m_wParent = parent; }

protected:
	//{{AFX_VIRTUAL(CModCleanupDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CModCleanupDlg)
	afx_msg void OnPresetCleanupSong();
	afx_msg void OnPresetCompoCleanup();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};