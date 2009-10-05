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
	CU_CLEANUP_PATTERNS = 0,
	CU_REARRANGE_PATTERNS,
	CU_CLEANUP_SAMPLES,
	CU_REARRANGE_SAMPLES,
	CU_CLEANUP_INSTRUMENTS,
	CU_REMOVE_INSTRUMENTS,
	CU_CLEANUP_PLUGINS,
	CU_CREATE_SAMPLEPACK,
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
	bool RearrangeSamples(); // Rearrange sample list
	bool RemoveUnusedInstruments(); // Remove unused instruments
	bool RemoveAllInstruments(bool bConfirm = true); // Remove all instruments
	bool RemoveUnusedPlugins(); // Remove ununsed plugins
	bool CreateSamplepack(); // Turn module into samplepack (convert to IT, remove patterns, etc.)

public:
	CModCleanupDlg(CModDoc *pModDoc, CWnd *parent):CDialog(IDD_CLEANUP_SONG, parent) { m_pModDoc = pModDoc; m_wParent = parent; }

protected:
	//{{AFX_VIRTUAL(CModCleanupDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CModCleanupDlg)
	/*afx_msg void OnCheckCleanupPatterns();
	afx_msg void OnCheckRearrangePatterns();
	afx_msg void OnCheckCleanupSamples();
	afx_msg void OnCheckRearrangeSamples();
	afx_msg void OnCheckCleanupInstruments();
	afx_msg void OnCheckRemoveInstruments();
	afx_msg void OnCheckCleanupPlugins();
	afx_msg void OnCheckCreateSamplepack();*/

	afx_msg void OnPresetCleanupSong();
	afx_msg void OnPresetCompoCleanup();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};