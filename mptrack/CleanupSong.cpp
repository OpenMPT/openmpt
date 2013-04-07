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

	CSoundFile &sndFile = modDoc.GetrSoundFile();

	GetDlgItem(m_nCleanupIDtoDlgID[CU_MERGE_SEQUENCES])->EnableWindow((sndFile.GetType() & MOD_TYPE_MPT) ? TRUE : FALSE);

	GetDlgItem(m_nCleanupIDtoDlgID[CU_REMOVE_SAMPLES])->EnableWindow((sndFile.GetNumSamples() > 0) ? TRUE : FALSE);
	GetDlgItem(m_nCleanupIDtoDlgID[CU_REARRANGE_SAMPLES])->EnableWindow((sndFile.GetNumSamples() > 1) ? TRUE : FALSE);

	GetDlgItem(m_nCleanupIDtoDlgID[CU_CLEANUP_INSTRUMENTS])->EnableWindow((sndFile.GetNumInstruments() > 0) ? TRUE : FALSE);
	GetDlgItem(m_nCleanupIDtoDlgID[CU_REMOVE_INSTRUMENTS])->EnableWindow((sndFile.GetNumInstruments() > 0) ? TRUE : FALSE);

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
	modDoc.ClearLog();

	// Orders
	if(m_bCheckBoxes[CU_MERGE_SEQUENCES]) bModified |= MergeSequences();
	if(m_bCheckBoxes[CU_REMOVE_ORDERS]) bModified |= RemoveAllOrders();

	// Patterns
	if(m_bCheckBoxes[CU_REMOVE_PATTERNS]) bModified |= RemoveAllPatterns();
	if(m_bCheckBoxes[CU_CLEANUP_PATTERNS]) bModified |= RemoveUnusedPatterns();
	if(m_bCheckBoxes[CU_REARRANGE_PATTERNS]) bModified |= RemoveUnusedPatterns(false);

	// Instruments
	if(modDoc.GetSoundFile()->m_nInstruments > 0)
	{
		if(m_bCheckBoxes[CU_REMOVE_INSTRUMENTS]) bModified |= RemoveAllInstruments();
		if(m_bCheckBoxes[CU_CLEANUP_INSTRUMENTS]) bModified |= RemoveUnusedInstruments();
	}

	// Samples
	if(m_bCheckBoxes[CU_REMOVE_SAMPLES]) bModified |= RemoveAllSamples();
	if(m_bCheckBoxes[CU_CLEANUP_SAMPLES]) bModified |= RemoveUnusedSamples();
	if(m_bCheckBoxes[CU_OPTIMIZE_SAMPLES]) bModified |= OptimizeSamples();
	if(modDoc.GetSoundFile()->m_nSamples > 1)
	{
		if(m_bCheckBoxes[CU_REARRANGE_SAMPLES]) bModified |= RearrangeSamples();
	}

	// Plugins
	if(m_bCheckBoxes[CU_REMOVE_PLUGINS]) bModified |= RemoveAllPlugins();
	if(m_bCheckBoxes[CU_CLEANUP_PLUGINS]) bModified |= RemoveUnusedPlugins();

	// Create samplepack
	if(m_bCheckBoxes[CU_RESET_VARIABLES]) bModified |= ResetVariables();

	if(bModified) modDoc.SetModified();
	modDoc.UpdateAllViews(NULL, HINT_MODTYPE | HINT_MODSEQUENCE | HINT_MODGENERAL | HINT_SMPNAMES | HINT_INSNAMES);
	modDoc.ShowLog("Cleanup", this);
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
	CSoundFile &sndFile = modDoc.GetrSoundFile();

	const SEQUENCEINDEX maxSeqIndex = sndFile.Order.GetNumSequences();
	const PATTERNINDEX maxPatIndex = sndFile.Patterns.Size();
	std::vector<OrigPatSettings> patternSettings(maxPatIndex, defaultSettings);

	CHAR s[512];
	bool bReordered = false;
	size_t nPatRemoved = 0;
	PATTERNINDEX nMinToRemove = 0;

	BeginWaitCursor();
	// First, find all used patterns in all sequences.
	PATTERNINDEX maxpat = 0;
	for(SEQUENCEINDEX nSeq = 0; nSeq < maxSeqIndex; nSeq++)
	{
		for (ORDERINDEX nOrd = 0; nOrd < sndFile.Order.GetSequence(nSeq).GetLength(); nOrd++)
		{
			PATTERNINDEX n = sndFile.Order.GetSequence(nSeq)[nOrd];
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
			if ((sndFile.Patterns[imax]) && (patternSettings[imax].isPatUsed)) break;
		}
		nMinToRemove = imax + 1;
	}

	// Remove all completely empty patterns above last used pattern (those are safe to remove)
	CriticalSection cs;
	for (PATTERNINDEX nPat = maxpat; nPat < maxPatIndex; nPat++) if ((sndFile.Patterns[nPat]) && (nPat >= nMinToRemove))
	{
		if(sndFile.Patterns.IsPatternEmpty(nPat))
		{
			sndFile.Patterns.Remove(nPat);
			nPatRemoved++;
		}
	}

	// Number of unused patterns
	size_t nWaste = 0;
	for (UINT ichk=0; ichk < maxPatIndex; ichk++)
	{
		if ((sndFile.Patterns[ichk]) && (!patternSettings[ichk].isPatUsed)) nWaste++;
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

	SEQUENCEINDEX oldSequence = sndFile.Order.GetCurrentSequenceIndex();	// workaround, as GetSequence doesn't allow writing to sequences ATM

	// Re-order pattern numbers based on sequence
	PATTERNINDEX nPats = 0;	// last used index
	for(SEQUENCEINDEX nSeq = 0; nSeq < maxSeqIndex; nSeq++)
	{
		sndFile.Order.SetSequence(nSeq);
		ORDERINDEX imap = 0;
		for (imap = 0; imap < sndFile.Order.GetSequence(nSeq).GetLength(); imap++)
		{
			PATTERNINDEX n = sndFile.Order.GetSequence(nSeq)[imap];
			if (n < maxPatIndex)
			{
				if (patternSettings[n].newIndex == PATTERNINDEX_INVALID) patternSettings[n].newIndex = nPats++;
				sndFile.Order[imap] = patternSettings[n].newIndex;
			}
		}
		// Add unused patterns at the end
		if ((!bRemove) || (!nWaste))
		{
			for(PATTERNINDEX iadd = 0; iadd < maxPatIndex; iadd++)
			{
				if((sndFile.Patterns[iadd]) && (patternSettings[iadd].newIndex >= maxPatIndex))
				{
					patternSettings[iadd].newIndex = nPats++;
				}
			}
		}
		while (imap < sndFile.Order.GetSequence(nSeq).GetLength())
		{
			sndFile.Order[imap++] = sndFile.Order.GetInvalidPatIndex();
		}
	}

	sndFile.Order.SetSequence(oldSequence);

	// Reorder patterns & Delete unused patterns
	{
		for (PATTERNINDEX i = 0; i < maxPatIndex; i++)
		{
			PATTERNINDEX k = patternSettings[i].newIndex;
			if (k < maxPatIndex)
			{
				if (i != k) bReordered = true;
				patternSettings[k].numRows = sndFile.Patterns[i].GetNumRows();
				patternSettings[k].data = sndFile.Patterns[i];
				if(sndFile.Patterns[i].GetOverrideSignature())
				{
					patternSettings[k].rowsPerBeat = sndFile.Patterns[i].GetRowsPerBeat();
					patternSettings[k].rowsPerMeasure = sndFile.Patterns[i].GetRowsPerMeasure();
				}
				patternSettings[k].name = sndFile.Patterns[i].GetName();
			} else
				if (sndFile.Patterns[i])
				{
					sndFile.Patterns.Remove(i);
					nPatRemoved++;
				}
		}
		for (PATTERNINDEX nPat = 0; nPat < maxPatIndex; nPat++)
		{
			sndFile.Patterns[nPat].SetData(patternSettings[nPat].data, patternSettings[nPat].numRows);
			sndFile.Patterns[nPat].SetSignature(patternSettings[nPat].rowsPerBeat, patternSettings[nPat].rowsPerMeasure);
			sndFile.Patterns[nPat].SetName(patternSettings[nPat].name);
		}
	}

	cs.Leave();
	EndWaitCursor();
	if ((nPatRemoved) || (bReordered))
	{
		modDoc.GetPatternUndo().ClearUndo();
		if (nPatRemoved)
		{
			wsprintf(s, "%d pattern%s removed.\n", nPatRemoved, (nPatRemoved == 1) ? "" : "s");
			modDoc.AddToLog(s);
		}
		return true;
	}
	return false;
}


// Remove unused samples
bool CModCleanupDlg::RemoveUnusedSamples()
//----------------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();

	CHAR s[512];
	std::vector<bool> samplesUsed(sndFile.GetNumSamples() + 1, true);

	BeginWaitCursor();

	// Check if any samples are not referenced in the patterns (sample mode) or by an instrument (instrument mode).
	// This doesn't check yet if a sample is referenced by an instrument, but actually unused in the patterns.
	for(SAMPLEINDEX smp = 1; smp <= sndFile.GetNumSamples(); smp++) if (sndFile.GetSample(smp).pSample)
	{
		if(!sndFile.IsSampleUsed(smp))
		{
			samplesUsed[smp] = false;
			modDoc.GetSampleUndo().PrepareUndo(smp, sundo_delete);
		}
	}

	SAMPLEINDEX nRemoved = sndFile.RemoveSelectedSamples(samplesUsed);

	const SAMPLEINDEX unusedInsSamples = sndFile.DetectUnusedSamples(samplesUsed);

	EndWaitCursor();

	if(unusedInsSamples && !((sndFile.GetType() == MOD_TYPE_IT) && sndFile.m_SongFlags[SONG_ITPROJECT]))
	{
		// We don't remove an instrument's unused samples in an ITP.
		wsprintf(s, "OpenMPT detected %d sample%s referenced by an instrument,\n"
			"but not used in the song. Do you want to remove them?", unusedInsSamples, (unusedInsSamples == 1) ? "" : "s");
		if(Reporting::Confirm(s, "Sample Cleanup", false, false, this) == cnfYes)
		{
			nRemoved += sndFile.RemoveSelectedSamples(samplesUsed);
		}
	}

	if(nRemoved > 0)
	{
		wsprintf(s, "%d unused sample%s removed\n" , nRemoved, (nRemoved == 1) ? "" : "s");
		modDoc.AddToLog(s);
	}

	return (nRemoved > 0);
}


// Remove unused sample data
bool CModCleanupDlg::OptimizeSamples()
//------------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();

	SAMPLEINDEX numLoopOpt = 0;
	
	for(SAMPLEINDEX smp = 1; smp <= sndFile.GetNumSamples(); smp++)
	{
		const ModSample &sample = sndFile.GetSample(smp);

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
		for(SAMPLEINDEX nSmp = 1; nSmp <= sndFile.m_nSamples; nSmp++)
		{
			ModSample &sample = sndFile.GetSample(nSmp);

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
					modDoc.GetSampleUndo().PrepareUndo(nSmp, sundo_delete, lmax, sample.nLength);
					ctrlSmp::ResizeSample(sample, lmax, sndFile);
				}
			}
		}
		wsprintf(s, "%d sample loop%s optimized\n" ,numLoopOpt, (numLoopOpt == 1) ? "" : "s");
		modDoc.AddToLog(s);
		return true;
	}

	return false;
}

// Rearrange sample list
bool CModCleanupDlg::RearrangeSamples()
//-------------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();
	if(sndFile.GetNumSamples() < 2)
		return false;

	std::vector<SAMPLEINDEX> sampleMap;
	sampleMap.reserve(sndFile.GetNumSamples());

	// First, find out which sample slots are unused and create the new sample map only with used samples
	for(SAMPLEINDEX i = 1; i <= sndFile.GetNumSamples(); i++)
	{
		if(sndFile.GetSample(i).pSample != nullptr)
		{
			sampleMap.push_back(i);
		}
	}

	// Nothing found to remove...
	if(sndFile.GetNumSamples() == sampleMap.size())
	{
		return false;
	}

	return (modDoc.ReArrangeSamples(sampleMap) != SAMPLEINDEX_INVALID);
}


// Remove unused instruments
bool CModCleanupDlg::RemoveUnusedInstruments()
//--------------------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();
	if(!sndFile.GetNumInstruments())
		return false;

	deleteInstrumentSamples removeSamples = doNoDeleteAssociatedSamples;
	if(!sndFile.m_SongFlags[SONG_ITPROJECT]) // Never remove an instrument's samples in ITP.
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

	std::vector<bool> instrUsed(sndFile.GetNumInstruments());
	bool prevUsed = true, reorder = false;
	INSTRUMENTINDEX numUsed = 0, lastUsed = 1;
	for(INSTRUMENTINDEX i = 0; i < sndFile.GetNumInstruments(); i++)
	{
		instrUsed[i] = (sndFile.IsInstrumentUsed(i + 1));
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

	const INSTRUMENTINDEX numRemoved = sndFile.GetNumInstruments() - numUsed;

	if(numRemoved != 0)
	{
		BeginWaitCursor();

		std::vector<INSTRUMENTINDEX> instrMap;
		instrMap.reserve(sndFile.GetNumInstruments());
		for(INSTRUMENTINDEX i = 0; i < sndFile.GetNumInstruments(); i++)
		{
			if(instrUsed[i])
			{
				instrMap.push_back(i + 1);
			} else if(!reorder && i < lastUsed)
			{
				instrMap.push_back(INSTRUMENTINDEX_INVALID);
			}
		}

		modDoc.ReArrangeInstruments(instrMap, removeSamples);

		EndWaitCursor();

		char s[64];
		wsprintf(s, "%d unused instrument%s removed\n", numRemoved, (numRemoved == 1) ? "" : "s");
		modDoc.AddToLog(s);
		return true;
	}
	return false;
}


// Remove ununsed plugins
bool CModCleanupDlg::RemoveUnusedPlugins()
//----------------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();

	std::vector<bool> usedmap(MAX_MIXPLUGINS, false);
	
	for(PLUGINDEX nPlug = 0; nPlug < MAX_MIXPLUGINS; nPlug++)
	{

		// Is the plugin assigned to a channel?
		for(CHANNELINDEX nChn = 0; nChn < sndFile.GetNumChannels(); nChn++)
		{
			if (sndFile.ChnSettings[nChn].nMixPlugin == nPlug + 1)
			{
				usedmap[nPlug] = true;
				break;
			}
		}

		// Is the plugin used by an instrument?
		for(INSTRUMENTINDEX nIns = 1; nIns <= sndFile.GetNumInstruments(); nIns++)
		{
			if (sndFile.Instruments[nIns] && (sndFile.Instruments[nIns]->nMixPlug == nPlug + 1))
			{
				usedmap[nPlug] = true;
				break;
			}
		}

		// Is the plugin assigned to master?
		if(sndFile.m_MixPlugins[nPlug].IsMasterEffect())
			usedmap[nPlug] = true;

		// All outputs of used plugins count as used
		if(usedmap[nPlug] != false)
		{
			if(!sndFile.m_MixPlugins[nPlug].IsOutputToMaster())
			{
				PLUGINDEX output = sndFile.m_MixPlugins[nPlug].GetOutputPlugin();
				if(output != PLUGINDEX_INVALID)
				{
					usedmap[output] = true;
				}
			}
		}

	}

	UINT nRemoved = modDoc.RemovePlugs(usedmap);

	return (nRemoved > 0);
}


// Reset variables (convert to IT, reset global/smp/ins vars, etc.)
bool CModCleanupDlg::ResetVariables()
//-----------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();

	if(Reporting::Confirm(TEXT("WARNING: OpenMPT will convert the module to IT format and reset all song, sample and instrument attributes to default values. Continue?"), TEXT("Resetting variables"), false, false, this) == cnfNo)
		return false;

	// Stop play.
	CMainFrame::GetMainFrame()->StopMod(&modDoc);

	BeginWaitCursor();
	CriticalSection cs;

	// convert to IT...
	modDoc.ChangeModType(MOD_TYPE_IT);
	sndFile.m_nMixLevels = mixLevels_compatible;
	sndFile.m_nTempoMode = tempo_mode_classic;
	sndFile.m_SongFlags = SONG_LINEARSLIDES;
	sndFile.m_MidiCfg.Reset();
	
	// Global vars
	sndFile.m_nDefaultTempo = 125;
	sndFile.m_nDefaultSpeed = 6;
	sndFile.m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	sndFile.m_nSamplePreAmp = 48;
	sndFile.m_nVSTiVolume = 48;
	sndFile.m_nRestartPos = 0;

	// reset instruments (if there are any)
	for(INSTRUMENTINDEX i = 1; i <= sndFile.GetNumInstruments(); i++) if(sndFile.Instruments[i])
	{
		sndFile.Instruments[i]->nFadeOut = 256;
		sndFile.Instruments[i]->nGlobalVol = 64;
		sndFile.Instruments[i]->nPan = 128;
		sndFile.Instruments[i]->dwFlags.reset(INS_SETPANNING);
		sndFile.Instruments[i]->nMixPlug = 0;

		sndFile.Instruments[i]->nVolSwing = 0;
		sndFile.Instruments[i]->nPanSwing = 0;
		sndFile.Instruments[i]->nCutSwing = 0;
		sndFile.Instruments[i]->nResSwing = 0;
	}

	for(CHANNELINDEX i = 0; i <= sndFile.GetNumChannels(); i++)
	{
		sndFile.ChnSettings[i].Reset();
	}

	// reset samples
	ctrlSmp::ResetSamples(sndFile, ctrlSmp::SmpResetCompo);

	// Set modflags.
	sndFile.SetModFlag(MSF_MIDICC_BUGEMULATION, false);
	sndFile.SetModFlag(MSF_OLDVOLSWING, false);
	sndFile.SetModFlag(MSF_COMPATIBLE_PLAY, true);

	cs.Leave();
	EndWaitCursor();

	return true;
}

// Remove all patterns
bool CModCleanupDlg::RemoveAllPatterns()
//--------------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();

	if(sndFile.Patterns.Size() == 0) return false;
	sndFile.Patterns.Init();
	sndFile.Patterns.Insert(0, 64);
	sndFile.SetCurrentOrder(0);
	return true;
}

// Remove all orders
bool CModCleanupDlg::RemoveAllOrders()
//------------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();

	sndFile.Order.SetSequence(0);
	while(sndFile.Order.GetNumSequences() > 1)
	{
		sndFile.Order.RemoveSequence(1);
	}
	sndFile.Order.Init();
	sndFile.Order[0] = 0;
	sndFile.SetCurrentOrder(0);
	return true;
}

// Remove all samples
bool CModCleanupDlg::RemoveAllSamples()
//-------------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();

	if (sndFile.GetNumSamples() == 0) return false;

	std::vector<bool> keepSamples(sndFile.GetNumSamples() + 1, false);
	sndFile.RemoveSelectedSamples(keepSamples);

	ctrlSmp::ResetSamples(sndFile, ctrlSmp::SmpResetInit, 1, MAX_SAMPLES - 1);

	return true;
}

// Remove all instruments
bool CModCleanupDlg::RemoveAllInstruments()
//-----------------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();

	if(sndFile.GetNumInstruments() == 0) return false;

	modDoc.ConvertInstrumentsToSamples();

	for(INSTRUMENTINDEX i = 1; i <= sndFile.GetNumInstruments(); i++)
	{
		sndFile.DestroyInstrument(i, doNoDeleteAssociatedSamples);
	}

	sndFile.m_nInstruments = 0;
	return true;
}

// Remove all plugins
bool CModCleanupDlg::RemoveAllPlugins()
//-------------------------------------
{
	std::vector<bool> keepMask(MAX_MIXPLUGINS, false);
	modDoc.RemovePlugs(keepMask);
	return true;
}


bool CModCleanupDlg::MergeSequences()
//-----------------------------------
{
	return modDoc.GetSoundFile()->Order.MergeSequences();
}
