/*
 * CleanupSong.cpp
 * ---------------
 * Purpose: Dialog for cleaning up modules (rearranging, removing unused items).
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
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
		CheckDlgButton(m_nCleanupIDtoDlgID[i], (m_bCheckBoxes[i]) ? BST_CHECKED : BST_UNCHECKED);
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
		m_bCheckBoxes[i] = IsDlgButtonChecked(m_nCleanupIDtoDlgID[i]) != BST_UNCHECKED;
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
	m_pModDoc->UpdateAllViews(NULL, HINT_MODTYPE | HINT_MODSEQUENCE | HINT_MODGENERAL | HINT_SMPNAMES | HINT_INSNAMES);
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
			if(IsDlgButtonChecked(m_nCleanupIDtoDlgID[i]) == BST_UNCHECKED)
				return;

			// now we can disable all elements that are mutually exclusive.
			if(m_nMutuallyExclusive[i] != CU_NONE)
				CheckDlgButton(m_nCleanupIDtoDlgID[m_nMutuallyExclusive[i]], BST_UNCHECKED);
			// find other elements which are mutually exclusive with the selected element.
			for(int j = 0; j < CU_MAX_CLEANUP_OPTIONS; j++)	
			{
				if(m_nMutuallyExclusive[j] == i)
					CheckDlgButton(m_nCleanupIDtoDlgID[j], BST_UNCHECKED);
			}
			return;
		}
	}
}


void CModCleanupDlg::OnPresetCleanupSong()
//----------------------------------------
{
	// patterns
	CheckDlgButton(IDC_CHK_CLEANUP_PATTERNS, BST_CHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_PATTERNS, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_REARRANGE_PATTERNS, BST_CHECKED);
	// orders
	CheckDlgButton(IDC_CHK_MERGE_SEQUENCES, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_ORDERS, BST_UNCHECKED);
	// samples
	CheckDlgButton(IDC_CHK_CLEANUP_SAMPLES, BST_CHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_SAMPLES, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_REARRANGE_SAMPLES, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_OPTIMIZE_SAMPLES, BST_CHECKED);
	// instruments
	CheckDlgButton(IDC_CHK_CLEANUP_INSTRUMENTS, BST_CHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_INSTRUMENTS, BST_UNCHECKED);
	// plugins
	CheckDlgButton(IDC_CHK_CLEANUP_PLUGINS, BST_CHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_PLUGINS, BST_UNCHECKED);
	// misc
	CheckDlgButton(IDC_CHK_SAMPLEPACK, BST_UNCHECKED);
}


void CModCleanupDlg::OnPresetCompoCleanup()
//-----------------------------------------
{
	// patterns
	CheckDlgButton(IDC_CHK_CLEANUP_PATTERNS, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_PATTERNS, BST_CHECKED);
	CheckDlgButton(IDC_CHK_REARRANGE_PATTERNS, BST_UNCHECKED);
	// orders
	CheckDlgButton(IDC_CHK_MERGE_SEQUENCES, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_ORDERS, BST_CHECKED);
	// samples
	CheckDlgButton(IDC_CHK_CLEANUP_SAMPLES, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_SAMPLES, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_REARRANGE_SAMPLES, BST_CHECKED);
	CheckDlgButton(IDC_CHK_OPTIMIZE_SAMPLES, BST_UNCHECKED);
	// instruments
	CheckDlgButton(IDC_CHK_CLEANUP_INSTRUMENTS, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_INSTRUMENTS, BST_CHECKED);
	// plugins
	CheckDlgButton(IDC_CHK_CLEANUP_PLUGINS, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_PLUGINS, BST_CHECKED);
	// misc
	CheckDlgButton(IDC_CHK_SAMPLEPACK, BST_CHECKED);
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
		StringFixer::CopyN(pTTTA->szText, strTipText);
	}
	else
	{
		::MultiByteToWideChar(CP_ACP , 0, strTipText, strTipText.GetLength() + 1,
			pTTTW->szText, CountOf(pTTTW->szText));
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
	ModCommand *data;			// original pattern data
	ROWINDEX numRows;			// original pattern sizes
	ROWINDEX rowsPerBeat;		// original pattern highlight
	ROWINDEX rowsPerMeasure;	// original pattern highlight
	CString name;				// original pattern name
};

const OrigPatSettings defaultSettings = { false, 0, nullptr, 0, 0, 0, "" };

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
	CriticalSection cs;
	for (PATTERNINDEX nPat = maxpat; nPat < maxPatIndex; nPat++) if ((pSndFile->Patterns[nPat]) && (nPat >= nMinToRemove))
	{
		if(pSndFile->Patterns.IsPatternEmpty(nPat))
		{
			pSndFile->Patterns.Remove(nPat);
			nPatRemoved++;
		}
	}

	// Number of unused patterns
	size_t nWaste = 0;
	for (UINT ichk=0; ichk < maxPatIndex; ichk++)
	{
		if ((pSndFile->Patterns[ichk]) && (!patternSettings[ichk].isPatUsed)) nWaste++;
	}

	if ((bRemove) && (nWaste))
	{
		cs.Leave();
		EndWaitCursor();
		wsprintf(s, "%d pattern%s present in file, but not used in the song\nDo you want to reorder the sequence list and remove these patterns?", nWaste, (nWaste == 1) ? "" : "s");
		if (Reporting::Confirm(s, "Pattern Cleanup", false, false, this) != cnfYes) return false;
		BeginWaitCursor();
		cs.Enter();
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

	cs.Leave();
	EndWaitCursor();
	if ((nPatRemoved) || (bReordered))
	{
		m_pModDoc->GetPatternUndo().ClearUndo();
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

	// Check if any samples are not referenced in the patterns (sample mode) or by an instrument (instrument mode).
	// This doesn't check yet if a sample is referenced by an instrument, but actually unused in the patterns.
	for(SAMPLEINDEX smp = 1; smp <= pSndFile->GetNumSamples(); smp++) if (pSndFile->GetSample(smp).pSample)
	{
		if(!pSndFile->IsSampleUsed(smp))
		{
			samplesUsed[smp] = false;
			m_pModDoc->GetSampleUndo().PrepareUndo(smp, sundo_delete);
		}
	}

	SAMPLEINDEX nRemoved = pSndFile->RemoveSelectedSamples(samplesUsed);

	const SAMPLEINDEX unusedInsSamples = pSndFile->DetectUnusedSamples(samplesUsed);

	EndWaitCursor();

	if(unusedInsSamples && !((pSndFile->GetType() == MOD_TYPE_IT) && pSndFile->m_SongFlags[SONG_ITPROJECT]))
	{
		// We don't remove an instrument's unused samples in an ITP.
		wsprintf(s, "OpenMPT detected %d sample%s referenced by an instrument,\n"
			"but not used in the song. Do you want to remove them?", unusedInsSamples, (unusedInsSamples == 1) ? "" : "s");
		if(Reporting::Confirm(s, "Sample Cleanup", false, false, this) == cnfYes)
		{
			nRemoved += pSndFile->RemoveSelectedSamples(samplesUsed);
		}
	}

	if(nRemoved > 0)
	{
		wsprintf(s, "%d unused sample%s removed\n" , nRemoved, (nRemoved == 1) ? "" : "s");
		m_pModDoc->AddToLog(s);
	}

	return (nRemoved > 0);
}


// Remove unused sample data
bool CModCleanupDlg::OptimizeSamples()
//------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	SAMPLEINDEX numLoopOpt = 0;
	
	for(SAMPLEINDEX smp = 1; smp <= pSndFile->GetNumSamples(); smp++)
	{
		const ModSample &sample = pSndFile->GetSample(smp);

		// Determine how much of the sample will be played
		SmpLength loopLength = sample.nLength;
		if(sample.uFlags[CHN_LOOP])
		{
			loopLength = sample.nLoopEnd;
			if(sample.uFlags[CHN_SUSTAINLOOP])
			{
				loopLength = Util::Max(sample.nLoopEnd, sample.nSustainEnd);
			}
		}

		if(sample.pSample && sample.nLength > loopLength + 2) numLoopOpt++;
	}
	if (numLoopOpt == 0) return false;

	char s[512];
	wsprintf(s, "%d sample%s unused data after the loop end point,\n"
		"Do you want to optimize %s and remove this unused data?", numLoopOpt, (numLoopOpt == 1) ? " has" : "s have", (numLoopOpt == 1) ? "it" : "them");
	if(Reporting::Confirm(s, "Sample Optimization", false, false, this) == cnfYes)
	{
		for(SAMPLEINDEX nSmp = 1; nSmp <= pSndFile->m_nSamples; nSmp++)
		{
			ModSample &sample = pSndFile->GetSample(nSmp);

			// Determine how much of the sample will be played
			SmpLength loopLength = sample.nLength;
			if(sample.uFlags[CHN_LOOP])
			{
				loopLength = sample.nLoopEnd;

				// Sustain loop is played before normal loop, and it can actually be located after the normal loop.
				if(sample.uFlags[CHN_SUSTAINLOOP])
				{
					loopLength = Util::Max(sample.nLoopEnd, sample.nSustainEnd);
				}
			}

			if(sample.nLength > loopLength + 2)
			{
				SmpLength lmax = loopLength + 2;
				if(lmax < sample.nLength && lmax >= 2)
				{
					m_pModDoc->GetSampleUndo().PrepareUndo(nSmp, sundo_delete, lmax, sample.nLength);
					ctrlSmp::ResizeSample(sample, lmax, pSndFile);
				}
			}
		}
		wsprintf(s, "%d sample loop%s optimized\n" ,numLoopOpt, (numLoopOpt == 1) ? "" : "s");
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

	if(pSndFile == nullptr || pSndFile->GetNumSamples() < 2)
		return false;

	std::vector<SAMPLEINDEX> sampleMap;
	sampleMap.reserve(pSndFile->GetNumSamples());

	// First, find out which sample slots are unused and create the new sample map only with used samples
	for(SAMPLEINDEX i = 1; i <= pSndFile->GetNumSamples(); i++)
	{
		if(pSndFile->GetSample(i).pSample != nullptr)
		{
			sampleMap.push_back(i);
		}
	}

	// Nothing found to remove...
	if(pSndFile->GetNumSamples() == sampleMap.size())
	{
		return false;
	}

	return (m_pModDoc->ReArrangeSamples(sampleMap) != SAMPLEINDEX_INVALID);
}


// Remove unused instruments
bool CModCleanupDlg::RemoveUnusedInstruments()
//--------------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();

	if(pSndFile == nullptr || !pSndFile->GetNumInstruments())
		return false;

	deleteInstrumentSamples removeSamples = doNoDeleteAssociatedSamples;
	if(!pSndFile->m_SongFlags[SONG_ITPROJECT]) // Never remove an instrument's samples in ITP.
	{
		if(Reporting::Confirm("Remove samples associated with unused instruments?", "Removing unused instruments", false, false, this) == cnfYes)
		{
			removeSamples = deleteAssociatedSamples;
		}
	} else
	{
		Reporting::Information("Samples associated with an used instrument won't be removed in IT Project files.", "Removing unused instruments", this);
	}

	BeginWaitCursor();

	vector<bool> instrUsed(pSndFile->GetNumInstruments());
	bool prevUsed = true, reorder = false;
	INSTRUMENTINDEX numUsed = 0, lastUsed = 1;
	for(INSTRUMENTINDEX i = 0; i < pSndFile->GetNumInstruments(); i++)
	{
		instrUsed[i] = (pSndFile->IsInstrumentUsed(i + 1));
		if(instrUsed[i])
		{
			numUsed++;
			lastUsed = i;
			if(!prevUsed)
			{
				reorder = true;
			}
		}
		prevUsed = instrUsed[i];
	}

	EndWaitCursor();

	if(reorder && numUsed >= 1)
	{
		reorder = (Reporting::Confirm("Do you want to reorganize the remaining instruments?", "Removing unused instruments", false, false, this) == cnfYes);
	} else
	{
		reorder = false;
	}

	const INSTRUMENTINDEX numRemoved = pSndFile->GetNumInstruments() - numUsed;

	if(numRemoved != 0)
	{
		BeginWaitCursor();

		vector<INSTRUMENTINDEX> instrMap;
		instrMap.reserve(pSndFile->GetNumInstruments());
		for(INSTRUMENTINDEX i = 0; i < pSndFile->GetNumInstruments(); i++)
		{
			if(instrUsed[i])
			{
				instrMap.push_back(i + 1);
			} else if(!reorder && i < lastUsed)
			{
				instrMap.push_back(INSTRUMENTINDEX_INVALID);
			}
		}

		m_pModDoc->ReArrangeInstruments(instrMap, removeSamples);

		EndWaitCursor();

		char s[64];
		wsprintf(s, "%d unused instrument%s removed\n", numRemoved, (numRemoved == 1) ? "" : "s");
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

	vector<bool> usedmap(MAX_MIXPLUGINS, false);
	
	for(PLUGINDEX nPlug = 0; nPlug < MAX_MIXPLUGINS; nPlug++)
	{

		// Is the plugin assigned to a channel?
		for(CHANNELINDEX nChn = 0; nChn < pSndFile->GetNumChannels(); nChn++)
		{
			if (pSndFile->ChnSettings[nChn].nMixPlugin == nPlug + 1)
			{
				usedmap[nPlug] = true;
				break;
			}
		}

		// Is the plugin used by an instrument?
		for(INSTRUMENTINDEX nIns = 1; nIns <= pSndFile->GetNumInstruments(); nIns++)
		{
			if (pSndFile->Instruments[nIns] && (pSndFile->Instruments[nIns]->nMixPlug == nPlug + 1))
			{
				usedmap[nPlug] = true;
				break;
			}
		}

		// Is the plugin assigned to master?
		if(pSndFile->m_MixPlugins[nPlug].IsMasterEffect())
			usedmap[nPlug] = true;

		// All outputs of used plugins count as used
		if(usedmap[nPlug] != false)
		{
			if(!pSndFile->m_MixPlugins[nPlug].IsOutputToMaster())
			{
				PLUGINDEX output = pSndFile->m_MixPlugins[nPlug].GetOutputPlugin();
				if(output != PLUGINDEX_INVALID)
				{
					usedmap[output] = true;
				}
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
	if(Reporting::Confirm(TEXT("WARNING: OpenMPT will convert the module to IT format and reset all song, sample and instrument attributes to default values. Continue?"), TEXT("Resetting variables"), false, false, this) == cnfNo)
		return false;

	// Stop play.
	CMainFrame::GetMainFrame()->StopMod(m_pModDoc);

	BeginWaitCursor();
	CriticalSection cs;

	// convert to IT...
	m_pModDoc->ChangeModType(MOD_TYPE_IT);
	pSndFile->m_nMixLevels = mixLevels_compatible;
	pSndFile->m_nTempoMode = tempo_mode_classic;
	pSndFile->m_SongFlags = SONG_LINEARSLIDES;
	pSndFile->m_MidiCfg.Reset();
	
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
		pSndFile->Instruments[i]->dwFlags.reset(INS_SETPANNING);
		pSndFile->Instruments[i]->nMixPlug = 0;

		pSndFile->Instruments[i]->nVolSwing = 0;
		pSndFile->Instruments[i]->nPanSwing = 0;
		pSndFile->Instruments[i]->nCutSwing = 0;
		pSndFile->Instruments[i]->nResSwing = 0;
	}

	for(CHANNELINDEX i = 0; i <= pSndFile->GetNumChannels(); i++)
	{
		pSndFile->ChnSettings[i].Reset();
	}

	// reset samples
	ctrlSmp::ResetSamples(*pSndFile, ctrlSmp::SmpResetCompo);

	// Set modflags.
	pSndFile->SetModFlag(MSF_MIDICC_BUGEMULATION, false);
	pSndFile->SetModFlag(MSF_OLDVOLSWING, false);
	pSndFile->SetModFlag(MSF_COMPATIBLE_PLAY, true);

	cs.Leave();
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
	pSndFile->RemoveSelectedSamples(keepSamples);

	ctrlSmp::ResetSamples(*pSndFile, ctrlSmp::SmpResetInit, 1, MAX_SAMPLES - 1);

	return true;
}

// Remove all instruments
bool CModCleanupDlg::RemoveAllInstruments()
//-----------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	if(pSndFile->GetNumInstruments() == 0) return false;

	m_pModDoc->ConvertInstrumentsToSamples();

	for(INSTRUMENTINDEX i = 1; i <= pSndFile->GetNumInstruments(); i++)
	{
		pSndFile->DestroyInstrument(i, doNoDeleteAssociatedSamples);
	}

	pSndFile->m_nInstruments = 0;
	return true;
}

// Remove all plugins
bool CModCleanupDlg::RemoveAllPlugins()
//-------------------------------------
{
	vector<bool> keepMask(MAX_MIXPLUGINS, false);
	m_pModDoc->RemovePlugs(keepMask);
	return true;
}


bool CModCleanupDlg::MergeSequences()
//-----------------------------------
{
	return m_pModDoc->GetSoundFile()->Order.MergeSequences();
}
