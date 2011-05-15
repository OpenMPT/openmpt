/*
 *
 * CleanupSong.cpp
 * ---------------
 * Purpose: Interface for cleaning up modules (rearranging, removing unused items)
 *
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 *
 */

#include "stdafx.h"
#include "moddoc.h"
#include "Mainfrm.h"
#include "modsmp_ctrl.h"
#include "CleanupSong.h"

// Default checkbox state
bool CModCleanupDlg::m_bCheckBoxes[CU_MAX_CLEANUP_OPTIONS] =
{
	true,	false,	true,			// patterns	
	false,	false,					// orders
	true,	false,	false,	true,	// samples
	true,	false,					// instruments
	true,	false,					// plugins
	false,							// misc
};

// Checkbox -> Control ID LUT
WORD const CModCleanupDlg::m_nCleanupIDtoDlgID[CU_MAX_CLEANUP_OPTIONS] =
{
	// patterns
	IDC_CHK_CLEANUP_PATTERNS,		IDC_CHK_REMOVE_PATTERNS,	IDC_CHK_REARRANGE_PATTERNS,
	// orders
	IDC_CHK_MERGE_SEQUENCES,		IDC_CHK_REMOVE_ORDERS,
	// samples
	IDC_CHK_CLEANUP_SAMPLES,		IDC_CHK_REMOVE_SAMPLES,		IDC_CHK_REARRANGE_SAMPLES,
	IDC_CHK_OPTIMIZE_SAMPLES,
	// instruments
	IDC_CHK_CLEANUP_INSTRUMENTS,	IDC_CHK_REMOVE_INSTRUMENTS,
	// plugins
	IDC_CHK_CLEANUP_PLUGINS,		IDC_CHK_REMOVE_PLUGINS,
	// misc
	IDC_CHK_RESET_VARIABLES,
};

// Options that are mutually exclusive to each other
ENUM_CLEANUP_OPTIONS const CModCleanupDlg::m_nMutuallyExclusive[CU_MAX_CLEANUP_OPTIONS] =
{
	// patterns
	CU_REMOVE_PATTERNS,		CU_CLEANUP_PATTERNS,	CU_REMOVE_PATTERNS,
	// orders
	CU_REMOVE_ORDERS,		CU_MERGE_SEQUENCES,
	// samples
	CU_REMOVE_SAMPLES,		CU_CLEANUP_SAMPLES,		CU_REMOVE_SAMPLES,
	CU_REMOVE_SAMPLES,
	// instruments
	CU_REMOVE_INSTRUMENTS,	CU_CLEANUP_INSTRUMENTS,
	// plugins
	CU_REMOVE_PLUGINS,		CU_CLEANUP_PLUGINS,
	// misc
	CU_NONE,

};

///////////////////////////////////////////////////////////////////////
// CModCleanupDlg

BEGIN_MESSAGE_MAP(CModCleanupDlg, CDialog)
	//{{AFX_MSG_MAP(CModTypeDlg)
	ON_COMMAND(IDC_BTN_CLEANUP_SONG,			OnPresetCleanupSong)
	ON_COMMAND(IDC_BTN_COMPO_CLEANUP,			OnPresetCompoCleanup)

	ON_COMMAND(IDC_CHK_CLEANUP_PATTERNS,		OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REMOVE_PATTERNS,			OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REARRANGE_PATTERNS,		OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_MERGE_SEQUENCES,			OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REMOVE_ORDERS,			OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_CLEANUP_SAMPLES,			OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REMOVE_SAMPLES,			OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REARRANGE_SAMPLES,		OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_OPTIMIZE_SAMPLES,		OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_CLEANUP_INSTRUMENTS,		OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REMOVE_INSTRUMENTS,		OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_CLEANUP_PLUGINS,			OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REMOVE_PLUGINS,			OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_RESET_VARIABLES,			OnVerifyMutualExclusive)

	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, &CModCleanupDlg::OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, &CModCleanupDlg::OnToolTipNotify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CModCleanupDlg::OnInitDialog()
//---------------------------------
{
	CDialog::OnInitDialog();
	for(int i = 0; i < CU_MAX_CLEANUP_OPTIONS; i++)
	{
		CheckDlgButton(m_nCleanupIDtoDlgID[i], (m_bCheckBoxes[i]) ? MF_CHECKED : MF_UNCHECKED);
	}

	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return FALSE;

	GetDlgItem(m_nCleanupIDtoDlgID[CU_MERGE_SEQUENCES])->EnableWindow((pSndFile->m_nType & MOD_TYPE_MPT) ? TRUE : FALSE);

	GetDlgItem(m_nCleanupIDtoDlgID[CU_REMOVE_SAMPLES])->EnableWindow((pSndFile->m_nSamples > 0) ? TRUE : FALSE);
	GetDlgItem(m_nCleanupIDtoDlgID[CU_REARRANGE_SAMPLES])->EnableWindow((pSndFile->m_nSamples > 1) ? TRUE : FALSE);

	GetDlgItem(m_nCleanupIDtoDlgID[CU_CLEANUP_INSTRUMENTS])->EnableWindow((pSndFile->m_nInstruments > 0) ? TRUE : FALSE);
	GetDlgItem(m_nCleanupIDtoDlgID[CU_REMOVE_INSTRUMENTS])->EnableWindow((pSndFile->m_nInstruments > 0) ? TRUE : FALSE);

	EnableToolTips(TRUE);
	return TRUE;
}


void CModCleanupDlg::OnOK()
//-------------------------
{
	for(int i = 0; i < CU_MAX_CLEANUP_OPTIONS; i++)
	{
		m_bCheckBoxes[i] = IsDlgButtonChecked(m_nCleanupIDtoDlgID[i]) ? true : false;
	}

	bool bModified = false;
	m_pModDoc->ClearLog();

	// Orders
	if(m_bCheckBoxes[CU_MERGE_SEQUENCES]) bModified |= MergeSequences();
	if(m_bCheckBoxes[CU_REMOVE_ORDERS]) bModified |= RemoveAllOrders();

	// Patterns
	if(m_bCheckBoxes[CU_REMOVE_PATTERNS]) bModified |= RemoveAllPatterns();
	if(m_bCheckBoxes[CU_CLEANUP_PATTERNS]) bModified |= RemoveUnusedPatterns();
	if(m_bCheckBoxes[CU_REARRANGE_PATTERNS]) bModified |= RemoveUnusedPatterns(false);

	// Instruments
	if(m_pModDoc->GetSoundFile()->m_nInstruments > 0)
	{
		if(m_bCheckBoxes[CU_REMOVE_INSTRUMENTS]) bModified |= RemoveAllInstruments();
		if(m_bCheckBoxes[CU_CLEANUP_INSTRUMENTS]) bModified |= RemoveUnusedInstruments();
	}

	// Samples
	if(m_bCheckBoxes[CU_REMOVE_SAMPLES]) bModified |= RemoveAllSamples();
	if(m_bCheckBoxes[CU_CLEANUP_SAMPLES]) bModified |= RemoveUnusedSamples();
	if(m_bCheckBoxes[CU_OPTIMIZE_SAMPLES]) bModified |= OptimizeSamples();
	if(m_pModDoc->GetSoundFile()->m_nSamples > 1)
	{
		if(m_bCheckBoxes[CU_REARRANGE_SAMPLES]) bModified |= RearrangeSamples();
	}

	// Plugins
	if(m_bCheckBoxes[CU_REMOVE_PLUGINS]) bModified |= RemoveAllPlugins();
	if(m_bCheckBoxes[CU_CLEANUP_PLUGINS]) bModified |= RemoveUnusedPlugins();

	// Create samplepack
	if(m_bCheckBoxes[CU_RESET_VARIABLES]) bModified |= ResetVariables();

	if(bModified) m_pModDoc->SetModified();
	m_pModDoc->UpdateAllViews(NULL, HINT_MODTYPE|HINT_MODSEQUENCE|HINT_MODGENERAL);
	m_pModDoc->ShowLog("Cleanup", this);
	CDialog::OnOK();
}


void CModCleanupDlg::OnCancel()
//-----------------------------
{
	CDialog::OnCancel();
}


void CModCleanupDlg::OnVerifyMutualExclusive()
//--------------------------------------------
{
	HWND hFocus = GetFocus()->m_hWnd;
	for(int i = 0; i < CU_MAX_CLEANUP_OPTIONS; i++)	
	{
		// if this item is focussed, we have just (un)checked it.
		if(hFocus == GetDlgItem(m_nCleanupIDtoDlgID[i])->m_hWnd)
		{
			// if we just unchecked it, there's nothing to verify.
			if(IsDlgButtonChecked(m_nCleanupIDtoDlgID[i]) == FALSE)
				return;

			// now we can disable all elements that are mutually exclusive.
			if(m_nMutuallyExclusive[i] != CU_NONE)
				CheckDlgButton(m_nCleanupIDtoDlgID[m_nMutuallyExclusive[i]], MF_UNCHECKED);
			// find other elements which are mutually exclusive with the selected element.
			for(int j = 0; j < CU_MAX_CLEANUP_OPTIONS; j++)	
			{
				if(m_nMutuallyExclusive[j] == i)
					CheckDlgButton(m_nCleanupIDtoDlgID[j], MF_UNCHECKED);
			}
			return;
		}
	}
}


void CModCleanupDlg::OnPresetCleanupSong()
//----------------------------------------
{
	// patterns
	CheckDlgButton(IDC_CHK_CLEANUP_PATTERNS, MF_CHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_PATTERNS, MF_UNCHECKED);
	CheckDlgButton(IDC_CHK_REARRANGE_PATTERNS, MF_CHECKED);
	// orders
	CheckDlgButton(IDC_CHK_MERGE_SEQUENCES, MF_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_ORDERS, MF_UNCHECKED);
	// samples
	CheckDlgButton(IDC_CHK_CLEANUP_SAMPLES, MF_CHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_SAMPLES, MF_UNCHECKED);
	CheckDlgButton(IDC_CHK_REARRANGE_SAMPLES, MF_UNCHECKED);
	CheckDlgButton(IDC_CHK_OPTIMIZE_SAMPLES, MF_CHECKED);
	// instruments
	CheckDlgButton(IDC_CHK_CLEANUP_INSTRUMENTS, MF_CHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_INSTRUMENTS, MF_UNCHECKED);
	// plugins
	CheckDlgButton(IDC_CHK_CLEANUP_PLUGINS, MF_CHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_PLUGINS, MF_UNCHECKED);
	// misc
	CheckDlgButton(IDC_CHK_SAMPLEPACK, MF_UNCHECKED);
}

void CModCleanupDlg::OnPresetCompoCleanup()
//-----------------------------------------
{
	// patterns
	CheckDlgButton(IDC_CHK_CLEANUP_PATTERNS, MF_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_PATTERNS, MF_CHECKED);
	CheckDlgButton(IDC_CHK_REARRANGE_PATTERNS, MF_UNCHECKED);
	// orders
	CheckDlgButton(IDC_CHK_MERGE_SEQUENCES, MF_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_ORDERS, MF_CHECKED);
	// samples
	CheckDlgButton(IDC_CHK_CLEANUP_SAMPLES, MF_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_SAMPLES, MF_UNCHECKED);
	CheckDlgButton(IDC_CHK_REARRANGE_SAMPLES, MF_CHECKED);
	CheckDlgButton(IDC_CHK_OPTIMIZE_SAMPLES, MF_UNCHECKED);
	// instruments
	CheckDlgButton(IDC_CHK_CLEANUP_INSTRUMENTS, MF_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_INSTRUMENTS, MF_CHECKED);
	// plugins
	CheckDlgButton(IDC_CHK_CLEANUP_PLUGINS, MF_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_PLUGINS, MF_CHECKED);
	// misc
	CheckDlgButton(IDC_CHK_SAMPLEPACK, MF_CHECKED);
}

BOOL CModCleanupDlg::OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
//----------------------------------------------------------------------------
{
	UNREFERENCED_PARAMETER(id);
	UNREFERENCED_PARAMETER(pResult);

	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CStringA strTipText = "";
	UINT_PTR nID = pNMHDR->idFrom;
	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	switch(nID)
	{
	// patterns
	case IDC_CHK_CLEANUP_PATTERNS:
		strTipText = "Remove all unused patterns and rearrange them.";
		break;
	case IDC_CHK_REMOVE_PATTERNS:
		strTipText = "Remove all patterns.";
		break;
	case IDC_CHK_REARRANGE_PATTERNS:
		strTipText = "Number the patterns given by their order in the sequence.";
		break;
	// orders
	case IDC_CHK_REMOVE_ORDERS:
		strTipText = "Reset the order list.";
		break;
	case IDC_CHK_MERGE_SEQUENCES:
		strTipText = "Merge multiple sequences into one.";
		break;
	// samples
	case IDC_CHK_CLEANUP_SAMPLES:
		strTipText = "Remove all unused samples.";
		break;
	case IDC_CHK_REMOVE_SAMPLES:
		strTipText = "Remove all samples.";
		break;
	case IDC_CHK_REARRANGE_SAMPLES:
		strTipText = "Reorder sample list by removing empty samples.";
		break;
	case IDC_CHK_OPTIMIZE_SAMPLES:
		strTipText = "Remove unused data after the sample loop end.";
		break;
	// instruments
	case IDC_CHK_CLEANUP_INSTRUMENTS:
		strTipText = "Remove all unused instruments.";
		break;
	case IDC_CHK_REMOVE_INSTRUMENTS:
		strTipText = "Remove all instruments and convert them to samples.";
		break;
	// plugins
	case IDC_CHK_CLEANUP_PLUGINS:
		strTipText = "Remove all unused plugins.";
		break;
	case IDC_CHK_REMOVE_PLUGINS:
		strTipText = "Remove all plugins.";
		break;
	// misc
	case IDC_CHK_SAMPLEPACK:
		strTipText = "Convert the module to .IT and reset song / sample / instrument variables";
		break;
	}

	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		//strncpy_s(pTTTA->szText, sizeof(pTTTA->szText), strTipText, 
		//	strTipText.GetLength() + 1);
		strncpy(pTTTA->szText, strTipText, min(strTipText.GetLength() + 1, ARRAYELEMCOUNT(pTTTA->szText) - 1));
	}
	else
	{
		::MultiByteToWideChar(CP_ACP , 0, strTipText, strTipText.GetLength() + 1,
			pTTTW->szText, ARRAYELEMCOUNT(pTTTW->szText));
	}

	return TRUE;
}


///////////////////////////////////////////////////////////////////////
// Actual cleanup implementations

struct OrigPatSettings
{
	bool isPatUsed;				// Is pattern used in sequence?
	PATTERNINDEX newIndex;		// map old pattern index <-> new pattern index
	// This stuff is needed for copying the old pattern properties to the new pattern number
	MODCOMMAND *data;			// original pattern data
	ROWINDEX numRows;			// original pattern sizes
	ROWINDEX rowsPerBeat;		// original pattern highlight
	ROWINDEX rowsPerMeasure;	// original pattern highlight
	CString name;				// original pattern name
};

const OrigPatSettings defaultSettings = {false, 0, nullptr, 0, 0, 0, ""};

// Remove unused patterns / rearrange patterns
// If argument bRemove is true, unused patterns are removed. Else, patterns are only rearranged.
bool CModCleanupDlg::RemoveUnusedPatterns(bool bRemove)
//-----------------------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	const SEQUENCEINDEX maxSeqIndex = pSndFile->Order.GetNumSequences();
	const PATTERNINDEX maxPatIndex = pSndFile->Patterns.Size();
	vector<OrigPatSettings> patternSettings(maxPatIndex, defaultSettings);

	CHAR s[512];
	bool bReordered = false;
	size_t nPatRemoved = 0;
	PATTERNINDEX nMinToRemove = 0;

	BeginWaitCursor();
	// First, find all used patterns in all sequences.
	PATTERNINDEX maxpat = 0;
	for(SEQUENCEINDEX nSeq = 0; nSeq < maxSeqIndex; nSeq++)
	{
		for (ORDERINDEX nOrd = 0; nOrd < pSndFile->Order.GetSequence(nSeq).GetLength(); nOrd++)
		{
			PATTERNINDEX n = pSndFile->Order.GetSequence(nSeq)[nOrd];
			if (n < maxPatIndex)
			{
				if (n >= maxpat) maxpat = n + 1;
				patternSettings[n].isPatUsed = true;
			}
		}
	}

	// Find first index to be removed
	if (!bRemove)
	{
		PATTERNINDEX imax = maxPatIndex;
		while (imax > 0)
		{
			imax--;
			if ((pSndFile->Patterns[imax]) && (patternSettings[imax].isPatUsed)) break;
		}
		nMinToRemove = imax + 1;
	}

	// Remove all completely empty patterns above last used pattern (those are safe to remove)
	BEGIN_CRITICAL();
	for (PATTERNINDEX nPat = maxpat; nPat < maxPatIndex; nPat++) if ((pSndFile->Patterns[nPat]) && (nPat >= nMinToRemove))
	{
		if(pSndFile->Patterns.IsPatternEmpty(nPat))
		{
			pSndFile->Patterns.Remove(nPat);
			nPatRemoved++;
		}
	}
	END_CRITICAL();

	// Number of unused patterns
	size_t nWaste = 0;
	for (UINT ichk=0; ichk < maxPatIndex; ichk++)
	{
		if ((pSndFile->Patterns[ichk]) && (!patternSettings[ichk].isPatUsed)) nWaste++;
	}

	if ((bRemove) && (nWaste))
	{
		EndWaitCursor();
		wsprintf(s, "%d pattern%s present in file, but not used in the song\nDo you want to reorder the sequence list and remove these patterns?", nWaste, (nWaste == 1) ? "" : "s");
		if (m_wParent->MessageBox(s, "Pattern Cleanup", MB_YESNO) != IDYES) return false;
		BeginWaitCursor();
	}

	for(PATTERNINDEX i = 0; i < maxPatIndex; i++)
		patternSettings[i].newIndex = PATTERNINDEX_INVALID;

	SEQUENCEINDEX oldSequence = pSndFile->Order.GetCurrentSequenceIndex();	// workaround, as GetSequence doesn't allow writing to sequences ATM

	// Re-order pattern numbers based on sequence
	PATTERNINDEX nPats = 0;	// last used index
	for(SEQUENCEINDEX nSeq = 0; nSeq < maxSeqIndex; nSeq++)
	{
		pSndFile->Order.SetSequence(nSeq);
		ORDERINDEX imap = 0;
		for (imap = 0; imap < pSndFile->Order.GetSequence(nSeq).GetLength(); imap++)
		{
			PATTERNINDEX n = pSndFile->Order.GetSequence(nSeq)[imap];
			if (n < maxPatIndex)
			{
				if (patternSettings[n].newIndex == PATTERNINDEX_INVALID) patternSettings[n].newIndex = nPats++;
				pSndFile->Order[imap] = patternSettings[n].newIndex;
			}
		}
		// Add unused patterns at the end
		if ((!bRemove) || (!nWaste))
		{
			for(PATTERNINDEX iadd = 0; iadd < maxPatIndex; iadd++)
			{
				if((pSndFile->Patterns[iadd]) && (patternSettings[iadd].newIndex >= maxPatIndex))
				{
					patternSettings[iadd].newIndex = nPats++;
				}
			}
		}
		while (imap < pSndFile->Order.GetSequence(nSeq).GetLength())
		{
			pSndFile->Order[imap++] = pSndFile->Order.GetInvalidPatIndex();
		}
	}

	pSndFile->Order.SetSequence(oldSequence);

	// Reorder patterns & Delete unused patterns
	BEGIN_CRITICAL();
	{
		for (PATTERNINDEX i = 0; i < maxPatIndex; i++)
		{
			PATTERNINDEX k = patternSettings[i].newIndex;
			if (k < maxPatIndex)
			{
				if (i != k) bReordered = true;
				patternSettings[k].numRows = pSndFile->Patterns[i].GetNumRows();
				patternSettings[k].data = pSndFile->Patterns[i];
				if(pSndFile->Patterns[i].GetOverrideSignature())
				{
					patternSettings[k].rowsPerBeat = pSndFile->Patterns[i].GetRowsPerBeat();
					patternSettings[k].rowsPerMeasure = pSndFile->Patterns[i].GetRowsPerMeasure();
				}
				patternSettings[k].name = pSndFile->Patterns[i].GetName();
			} else
				if (pSndFile->Patterns[i])
				{
					pSndFile->Patterns.Remove(i);
					nPatRemoved++;
				}
		}
		for (PATTERNINDEX nPat = 0; nPat < maxPatIndex; nPat++)
		{
			pSndFile->Patterns[nPat].SetData(patternSettings[nPat].data, patternSettings[nPat].numRows);
			pSndFile->Patterns[nPat].SetSignature(patternSettings[nPat].rowsPerBeat, patternSettings[nPat].rowsPerMeasure);
			pSndFile->Patterns[nPat].SetName(patternSettings[nPat].name);
		}
	}
	END_CRITICAL();
	EndWaitCursor();
	if ((nPatRemoved) || (bReordered))
	{
		m_pModDoc->GetPatternUndo()->ClearUndo();
		if (nPatRemoved)
		{
			wsprintf(s, "%d pattern%s removed.\n", nPatRemoved, (nPatRemoved == 1) ? "" : "s");
			m_pModDoc->AddToLog(s);
		}
		return true;
	}
	return false;
}


// Remove unused samples
bool CModCleanupDlg::RemoveUnusedSamples()
//----------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	CHAR s[512];
	vector<bool> samplesUsed(pSndFile->GetNumSamples() + 1, true);

	BeginWaitCursor();
	for (SAMPLEINDEX nSmp = pSndFile->GetNumSamples(); nSmp >= 1; nSmp--) if (pSndFile->Samples[nSmp].pSample)
	{
		if (!pSndFile->IsSampleUsed(nSmp))
		{
			samplesUsed[nSmp] = false;
			m_pModDoc->GetSampleUndo()->PrepareUndo(nSmp, sundo_delete);
		}
	}
	BEGIN_CRITICAL();
	SAMPLEINDEX nRemoved = pSndFile->RemoveSelectedSamples(samplesUsed);
	END_CRITICAL();

	SAMPLEINDEX nExt = pSndFile->DetectUnusedSamples(samplesUsed);

	EndWaitCursor();

	if (nExt && !((pSndFile->GetType() == MOD_TYPE_IT) && (pSndFile->m_dwSongFlags & SONG_ITPROJECT)))
	{	//We don't remove an instrument's unused samples in an ITP.
		wsprintf(s, "OpenMPT detected %d sample%s referenced by an instrument,\n"
			"but not used in the song. Do you want to remove them?", nExt, (nExt == 1) ? "" : "s");
		if (::MessageBox(NULL, s, "Sample Cleanup", MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			for (SAMPLEINDEX nSmp = 1; nSmp <= pSndFile->GetNumSamples(); nSmp++)
			{
				if ((!samplesUsed[nSmp]) && (pSndFile->Samples[nSmp].pSample))
				{
					m_pModDoc->GetSampleUndo()->PrepareUndo(nSmp, sundo_delete);
					BEGIN_CRITICAL();
					pSndFile->DestroySample(nSmp);
					if ((nSmp == pSndFile->m_nSamples) && (nSmp > 1)) pSndFile->m_nSamples--;
					END_CRITICAL();
					nRemoved++;
					m_pModDoc->GetSampleUndo()->ClearUndo(nSmp);
				}
			}
			wsprintf(s, "%d unused sample%s removed\n" , nRemoved, (nRemoved == 1) ? "" : "s");
			m_pModDoc->AddToLog(s);
			return true;
		}
	}
	return (nRemoved > 0);
}


// Remove unused sample data
bool CModCleanupDlg::OptimizeSamples()
//------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	UINT nLoopOpt = 0;
	
	for (SAMPLEINDEX nSmp=1; nSmp <= pSndFile->m_nSamples; nSmp++)
	{
		if(pSndFile->Samples[nSmp].pSample && (pSndFile->Samples[nSmp].uFlags & CHN_LOOP)
			&& (pSndFile->Samples[nSmp].nLength > pSndFile->Samples[nSmp].nLoopEnd + 2)) nLoopOpt++;
	}
	if (nLoopOpt == 0) return false;

	CHAR s[512];
	wsprintf(s, "%d sample%s unused data after the loop end point,\n"
		"Do you want to optimize %s and remove this unused data?", nLoopOpt, (nLoopOpt == 1) ? " has" : "s have", (nLoopOpt == 1) ? "it" : "them");
	if (::MessageBox(NULL, s, "Sample Optimization", MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		for (SAMPLEINDEX nSmp = 1; nSmp <= pSndFile->m_nSamples; nSmp++)
		{
			if ((pSndFile->Samples[nSmp].uFlags & CHN_LOOP)
				&& (pSndFile->Samples[nSmp].nLength > pSndFile->Samples[nSmp].nLoopEnd + 2))
			{
				UINT lmax = pSndFile->Samples[nSmp].nLoopEnd + 2;
				if ((lmax < pSndFile->Samples[nSmp].nLength) && (lmax >= 2))
				{
					m_pModDoc->GetSampleUndo()->PrepareUndo(nSmp, sundo_delete, lmax, pSndFile->Samples[nSmp].nLength);
					ctrlSmp::ResizeSample(pSndFile->Samples[nSmp], lmax, pSndFile);
				}
			}
		}
		wsprintf(s, "%d sample loop%s optimized\n" ,nLoopOpt, (nLoopOpt == 1) ? "" : "s");
		m_pModDoc->AddToLog(s);
		return true;
	}

	return false;	
}

// Rearrange sample list
bool CModCleanupDlg::RearrangeSamples()
//-------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	if(pSndFile->m_nSamples < 2)
		return false;

	SAMPLEINDEX nRemap = 0; // remap count
	std::vector<SAMPLEINDEX> nSampleMap(pSndFile->GetNumSamples() + 1);
	for(SAMPLEINDEX i = 0; i <= pSndFile->GetNumSamples(); i++)
	{
		nSampleMap[i] = i;
	}

	// First, find out which sample slots are unused and create the new sample map
	for(SAMPLEINDEX i = 1; i <= pSndFile->GetNumSamples(); i++)
	{
		if(!pSndFile->Samples[i].pSample)
		{
			// Move all following samples
			nRemap++;
			nSampleMap[i] = 0;
			for(UINT j = i + 1; j <= pSndFile->GetNumSamples(); j++)
				nSampleMap[j]--;
		}
	}

	if(!nRemap)
		return false;

	// Now, move everything around
	for(SAMPLEINDEX i = 1; i <= pSndFile->GetNumSamples(); i++)
	{
		if(nSampleMap[i] != i)
		{
			// This gotta be moved
			BEGIN_CRITICAL();
			pSndFile->MoveSample(i, nSampleMap[i]);
			pSndFile->Samples[i].pSample = nullptr;
			END_CRITICAL();
			if(nSampleMap[i] > 0) strcpy(pSndFile->m_szNames[nSampleMap[i]], pSndFile->m_szNames[i]);
			memset(pSndFile->m_szNames[i], 0, sizeof(pSndFile->m_szNames[i]));

			// Also update instrument mapping (if module is in instrument mode)
			for(INSTRUMENTINDEX nIns = 1; nIns <= pSndFile->GetNumInstruments(); nIns++)
			{
				MODINSTRUMENT *pIns = pSndFile->Instruments[nIns];
				if(pIns)
				{
					for(size_t iNote = 0; iNote < 128; iNote++)
						if(pIns->Keyboard[iNote] == i) pIns->Keyboard[iNote] = nSampleMap[i];
				}
			}
		}
	}

	// Go through the patterns and remap samples (if module is in sample mode)
	if(!pSndFile->GetNumInstruments())
	{
		for (PATTERNINDEX nPat = 0; nPat < pSndFile->Patterns.Size(); nPat++) if (pSndFile->Patterns[nPat])
		{
			MODCOMMAND *m = pSndFile->Patterns[nPat];
			for(UINT len = pSndFile->Patterns[nPat].GetNumRows() * pSndFile->GetNumChannels(); len; m++, len--)
			{
				if(!m->IsPcNote() &&  m->instr <= pSndFile->GetNumSamples()) m->instr = (BYTE)nSampleMap[m->instr];
			}
		}
	}

	// Too lazy to fix sample undo...
	m_pModDoc->GetSampleUndo()->ClearUndo();

	pSndFile->m_nSamples -= nRemap;

	return true;

}


// Remove unused instruments
bool CModCleanupDlg::RemoveUnusedInstruments()
//--------------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	vector<bool> usedmap;
	INSTRUMENTINDEX swapmap[MAX_INSTRUMENTS];
	INSTRUMENTINDEX swapdest[MAX_INSTRUMENTS];
	CHAR s[512];
	UINT nRemoved = 0;
	INSTRUMENTINDEX nSwap, nIndex;
	bool bReorg = false;

	if (!pSndFile->m_nInstruments) return false;

	char removeSamples = -1;
	if ( !((pSndFile->GetType() == MOD_TYPE_IT) && (pSndFile->m_dwSongFlags & SONG_ITPROJECT))) //never remove an instrument's samples in ITP.
	{
		if(::MessageBox(NULL, "Remove samples associated with an instrument if they are unused?", "Removing unused instruments", MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			removeSamples = 1;
		}
	} else
	{
		::MessageBox(NULL, "This is an IT project file, so no samples associated with a used instrument will be removed.", "Removing unused instruments", MB_OK | MB_ICONINFORMATION);
	}

	BeginWaitCursor();
	usedmap.resize(pSndFile->GetNumInstruments() + 1, false);

	for(INSTRUMENTINDEX i = pSndFile->GetNumInstruments(); i >= 1; i--)
	{
		if (!pSndFile->IsInstrumentUsed(i))
		{
			BEGIN_CRITICAL();
			// -> CODE#0003
			// -> DESC="remove instrument's samples"
			//			pSndFile->DestroyInstrument(i);
			pSndFile->DestroyInstrument(i, removeSamples);
			// -! BEHAVIOUR_CHANGE#0003
			if ((i == pSndFile->GetNumInstruments()) && (i>1))
				pSndFile->m_nInstruments--;
			else
				bReorg = true;
			END_CRITICAL();
			nRemoved++;
		} else
		{
			usedmap[i] = true;
		}
	}
	EndWaitCursor();
	if ((bReorg) && (pSndFile->m_nInstruments > 1)
		&& (::MessageBox(NULL, "Do you want to reorganize the remaining instruments?", "Removing unused instruments", MB_YESNO | MB_ICONQUESTION) == IDYES))
	{
		BeginWaitCursor();
		BEGIN_CRITICAL();
		nSwap = 0;
		nIndex = 1;
		for (INSTRUMENTINDEX nIns = 1; nIns <= pSndFile->m_nInstruments; nIns++)
		{
			if (usedmap[nIns])
			{
				while (nIndex<nIns)
				{
					if ((!usedmap[nIndex]) && (!pSndFile->Instruments[nIndex]))
					{
						swapmap[nSwap] = nIns;
						swapdest[nSwap] = nIndex;
						pSndFile->Instruments[nIndex] = pSndFile->Instruments[nIns];
						pSndFile->Instruments[nIns] = nullptr;
						usedmap[nIndex] = true;
						usedmap[nIns] = false;
						nSwap++;
						nIndex++;
						break;
					}
					nIndex++;
				}
			}
		}
		while ((pSndFile->m_nInstruments > 1) && (!pSndFile->Instruments[pSndFile->m_nInstruments])) pSndFile->m_nInstruments--;
		END_CRITICAL();
		if (nSwap > 0)
		{
			for (PATTERNINDEX iPat = 0; iPat < pSndFile->Patterns.Size(); iPat++) if (pSndFile->Patterns[iPat])
			{
				MODCOMMAND *p = pSndFile->Patterns[iPat];
				UINT nLen = pSndFile->m_nChannels * pSndFile->Patterns[iPat].GetNumRows();
				while (nLen--)
				{
					if (p->instr && !p->IsPcNote())
					{
						for (UINT k=0; k<nSwap; k++)
						{
							if (p->instr == swapmap[k]) p->instr = (MODCOMMAND::INSTR)swapdest[k];
						}
					}
					p++;
				}
			}
		}
		EndWaitCursor();
	}
	if (nRemoved)
	{
		wsprintf(s, "%d unused instrument%s removed\n", nRemoved, (nRemoved == 1) ? "" : "s");
		m_pModDoc->AddToLog(s);
		return true;
	}
	return false;
}


// Remove ununsed plugins
bool CModCleanupDlg::RemoveUnusedPlugins()
//----------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	bool usedmap[MAX_MIXPLUGINS];
	memset(usedmap, false, sizeof(usedmap));
	
	for (PLUGINDEX nPlug = 0; nPlug < MAX_MIXPLUGINS; nPlug++) {

		//Is the plugin assigned to a channel?
		for (CHANNELINDEX nChn = 0; nChn < pSndFile->GetNumChannels(); nChn++)
		{
			if (pSndFile->ChnSettings[nChn].nMixPlugin == nPlug + 1)
			{
				usedmap[nPlug] = true;
				break;
			}
		}

		//Is the plugin used by an instrument?
		for (INSTRUMENTINDEX nIns=1; nIns<=pSndFile->GetNumInstruments(); nIns++)
		{
			if (pSndFile->Instruments[nIns] && (pSndFile->Instruments[nIns]->nMixPlug == nPlug + 1))
			{
				usedmap[nPlug] = true;
				break;
			}
		}

		//Is the plugin assigned to master?
		if (pSndFile->m_MixPlugins[nPlug].Info.dwInputRouting & MIXPLUG_INPUTF_MASTEREFFECT)
			usedmap[nPlug] = true;

		//all outputs of used plugins count as used
		if (usedmap[nPlug] != false)
		{
			if (pSndFile->m_MixPlugins[nPlug].Info.dwOutputRouting & 0x80)
			{
				int output = pSndFile->m_MixPlugins[nPlug].Info.dwOutputRouting & 0x7f;
				usedmap[output] = true;
			}
		}

	}

	UINT nRemoved = m_pModDoc->RemovePlugs(usedmap);

	return (nRemoved > 0);
}


// Reset variables (convert to IT, reset global/smp/ins vars, etc.)
bool CModCleanupDlg::ResetVariables()
//-----------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	//jojo.compocleanup
	if(::MessageBox(NULL, TEXT("WARNING: OpenMPT will convert the module to IT format and reset all song, sample and instrument attributes to default values. Continue?"), TEXT("Resetting variables"), MB_YESNO | MB_ICONWARNING) == IDNO)
		return false;

	// Stop play.
	CMainFrame::GetMainFrame()->StopMod(m_pModDoc);

	BeginWaitCursor();
	BEGIN_CRITICAL();

	// convert to IT...
	m_pModDoc->ChangeModType(MOD_TYPE_IT);
	pSndFile->m_nMixLevels = mixLevels_compatible;
	pSndFile->m_nTempoMode = tempo_mode_classic;
	pSndFile->m_dwSongFlags = SONG_LINEARSLIDES;
	
	// Global vars
	pSndFile->m_nDefaultTempo = 125;
	pSndFile->m_nDefaultSpeed = 6;
	pSndFile->m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	pSndFile->m_nSamplePreAmp = 48;
	pSndFile->m_nVSTiVolume = 48;
	pSndFile->m_nRestartPos = 0;

	// reset instruments (if there are any)
	for(INSTRUMENTINDEX i = 1; i <= pSndFile->GetNumInstruments(); i++) if(pSndFile->Instruments[i])
	{
		pSndFile->Instruments[i]->nFadeOut = 256;
		pSndFile->Instruments[i]->nGlobalVol = 64;
		pSndFile->Instruments[i]->nPan = 128;
		pSndFile->Instruments[i]->dwFlags &= ~INS_SETPANNING;
		pSndFile->Instruments[i]->nMixPlug = 0;

		pSndFile->Instruments[i]->nVolSwing = 0;
		pSndFile->Instruments[i]->nPanSwing = 0;
		pSndFile->Instruments[i]->nCutSwing = 0;
		pSndFile->Instruments[i]->nResSwing = 0;
	}

	// reset samples
	ctrlSmp::ResetSamples(*pSndFile, ctrlSmp::SmpResetCompo);

	// Set modflags.
	pSndFile->SetModFlag(MSF_MIDICC_BUGEMULATION, false);
	pSndFile->SetModFlag(MSF_OLDVOLSWING, false);
	pSndFile->SetModFlag(MSF_COMPATIBLE_PLAY, true);

	END_CRITICAL();
	EndWaitCursor();

	return true;
}

// Remove all patterns
bool CModCleanupDlg::RemoveAllPatterns()
//--------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	if (pSndFile->Patterns.Size() == 0) return false;
	pSndFile->Patterns.Init();
	pSndFile->Patterns.Insert(0, 64);
	pSndFile->SetCurrentOrder(0);
	return true;
}

// Remove all orders
bool CModCleanupDlg::RemoveAllOrders()
//------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	pSndFile->Order.SetSequence(0);
	while(pSndFile->Order.GetNumSequences() > 1)
	{
		pSndFile->Order.RemoveSequence(1);
	}
	pSndFile->Order.Init();
	pSndFile->Order[0] = 0;
	pSndFile->SetCurrentOrder(0);
	return true;
}

// Remove all samples
bool CModCleanupDlg::RemoveAllSamples()
//-------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	if (pSndFile->GetNumSamples() == 0) return false;
	vector<bool> keepSamples(pSndFile->GetNumSamples() + 1, false);

	for (SAMPLEINDEX nSmp = 1; nSmp <= pSndFile->GetNumSamples(); nSmp++)
	{
		m_pModDoc->GetSampleUndo()->PrepareUndo(nSmp, sundo_delete, 0, pSndFile->Samples[nSmp].nLength);
	}
	ctrlSmp::ResetSamples(*pSndFile, ctrlSmp::SmpResetInit);
	BEGIN_CRITICAL();
	pSndFile->RemoveSelectedSamples(keepSamples);
	END_CRITICAL();
	return true;
}

// Remove all instruments
bool CModCleanupDlg::RemoveAllInstruments(bool bConfirm)
//------------------------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	if (pSndFile->GetNumInstruments() == 0) return false;

	if(bConfirm)
	{
		if (CMainFrame::GetMainFrame()->MessageBox("Do you want to convert all instruments to samples?",
			"Removing all instruments", MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			m_pModDoc->ConvertInstrumentsToSamples();
		}
	}

	for (INSTRUMENTINDEX i = 1; i <= pSndFile->GetNumInstruments(); i++)
	{
		pSndFile->DestroyInstrument(i, -1);
	}

	pSndFile->m_nInstruments = 0;
	return true;
}

// Remove all plugins
bool CModCleanupDlg::RemoveAllPlugins()
//-------------------------------------
{
	bool keepMask[MAX_MIXPLUGINS];
	memset(keepMask, false, sizeof(keepMask));
	m_pModDoc->RemovePlugs(keepMask);
	return true;
}


bool CModCleanupDlg::MergeSequences()
//-----------------------------------
{
	return m_pModDoc->GetSoundFile()->Order.MergeSequences();
}
