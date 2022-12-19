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
#include "Moddoc.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "CleanupSong.h"
#include "ProgressDialog.h"
#include "../common/mptStringBuffer.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/modsmp_ctrl.h"
#include "../tracklib/SampleEdit.h"


OPENMPT_NAMESPACE_BEGIN


// Default checkbox state
bool CModCleanupDlg::m_CheckBoxes[kMaxCleanupOptions] =
{
	true,	false,	true,	true,	// patterns
	false,	false,					// orders
	true,	false,	false,	true,	// samples
	true,	false,					// instruments
	true,	false,					// plugins
	false,	true,					// misc
};

// Checkbox -> Control ID LUT
WORD const CModCleanupDlg::m_CleanupIDtoDlgID[kMaxCleanupOptions] =
{
	// patterns
	IDC_CHK_CLEANUP_PATTERNS,		IDC_CHK_REMOVE_PATTERNS,
	IDC_CHK_REARRANGE_PATTERNS,		IDC_CHK_REMOVE_DUPLICATES,
	// orders
	IDC_CHK_MERGE_SEQUENCES,		IDC_CHK_REMOVE_ORDERS,
	// samples
	IDC_CHK_CLEANUP_SAMPLES,		IDC_CHK_REMOVE_SAMPLES,
	IDC_CHK_REARRANGE_SAMPLES,		IDC_CHK_OPTIMIZE_SAMPLES,
	// instruments
	IDC_CHK_CLEANUP_INSTRUMENTS,	IDC_CHK_REMOVE_INSTRUMENTS,
	// plugins
	IDC_CHK_CLEANUP_PLUGINS,		IDC_CHK_REMOVE_PLUGINS,
	// misc
	IDC_CHK_RESET_VARIABLES,		IDC_CHK_UNUSED_CHANNELS,
};

// Options that are mutually exclusive to each other
CModCleanupDlg::CleanupOptions const CModCleanupDlg::m_MutuallyExclusive[CModCleanupDlg::kMaxCleanupOptions] =
{
	// patterns
	kRemovePatterns,		kCleanupPatterns,
	kRemovePatterns,		kRemovePatterns,
	// orders
	kRemoveOrders,			kMergeSequences,
	// samples
	kRemoveSamples,			kCleanupSamples,
	kRemoveSamples,			kRemoveSamples,
	// instruments
	kRemoveAllInstruments,	kCleanupInstruments,
	// plugins
	kRemoveAllPlugins,		kCleanupPlugins,
	// misc
	kNone,					kNone,

};

///////////////////////////////////////////////////////////////////////
// CModCleanupDlg

BEGIN_MESSAGE_MAP(CModCleanupDlg, CDialog)
	//{{AFX_MSG_MAP(CModTypeDlg)
	ON_COMMAND(IDC_BTN_CLEANUP_SONG,			&CModCleanupDlg::OnPresetCleanupSong)
	ON_COMMAND(IDC_BTN_COMPO_CLEANUP,			&CModCleanupDlg::OnPresetCompoCleanup)

	ON_COMMAND(IDC_CHK_CLEANUP_PATTERNS,		&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REMOVE_PATTERNS,			&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REARRANGE_PATTERNS,		&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REMOVE_DUPLICATES,		&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_MERGE_SEQUENCES,			&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REMOVE_ORDERS,			&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_CLEANUP_SAMPLES,			&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REMOVE_SAMPLES,			&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REARRANGE_SAMPLES,		&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_OPTIMIZE_SAMPLES,		&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_CLEANUP_INSTRUMENTS,		&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REMOVE_INSTRUMENTS,		&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_CLEANUP_PLUGINS,			&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_REMOVE_PLUGINS,			&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_RESET_VARIABLES,			&CModCleanupDlg::OnVerifyMutualExclusive)
	ON_COMMAND(IDC_CHK_UNUSED_CHANNELS,			&CModCleanupDlg::OnVerifyMutualExclusive)

	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, &CModCleanupDlg::OnToolTipNotify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CModCleanupDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	for(int i = 0; i < kMaxCleanupOptions; i++)
	{
		CheckDlgButton(m_CleanupIDtoDlgID[i], (m_CheckBoxes[i]) ? BST_CHECKED : BST_UNCHECKED);
	}

	CSoundFile &sndFile = modDoc.GetSoundFile();

	GetDlgItem(m_CleanupIDtoDlgID[kMergeSequences])->EnableWindow((sndFile.Order.GetNumSequences() > 1) ? TRUE : FALSE);

	GetDlgItem(m_CleanupIDtoDlgID[kRemoveSamples])->EnableWindow((sndFile.GetNumSamples() > 0) ? TRUE : FALSE);
	GetDlgItem(m_CleanupIDtoDlgID[kRearrangeSamples])->EnableWindow((sndFile.GetNumSamples() > 1) ? TRUE : FALSE);

	GetDlgItem(m_CleanupIDtoDlgID[kCleanupInstruments])->EnableWindow((sndFile.GetNumInstruments() > 0) ? TRUE : FALSE);
	GetDlgItem(m_CleanupIDtoDlgID[kRemoveAllInstruments])->EnableWindow((sndFile.GetNumInstruments() > 0) ? TRUE : FALSE);

	EnableToolTips(TRUE);
	return TRUE;
}


void CModCleanupDlg::OnOK()
{
	ScopedLogCapturer logcapturer(modDoc, _T("cleanup"), this);
	for(int i = 0; i < kMaxCleanupOptions; i++)
	{
		m_CheckBoxes[i] = IsDlgButtonChecked(m_CleanupIDtoDlgID[i]) != BST_UNCHECKED;
	}

	bool modified = false;

	// Orders
	if(m_CheckBoxes[kMergeSequences]) modified |= MergeSequences();
	if(m_CheckBoxes[kRemoveOrders]) modified |= RemoveAllOrders();

	// Patterns
	if(m_CheckBoxes[kRemovePatterns]) modified |= RemoveAllPatterns();
	if(m_CheckBoxes[kCleanupPatterns]) modified |= RemoveUnusedPatterns();
	if(m_CheckBoxes[kRemoveDuplicatePatterns]) modified |= RemoveDuplicatePatterns();
	if(m_CheckBoxes[kRearrangePatterns]) modified |= RearrangePatterns();

	// Instruments
	if(modDoc.GetNumInstruments() > 0)
	{
		if(m_CheckBoxes[kRemoveAllInstruments]) modified |= RemoveAllInstruments();
		if(m_CheckBoxes[kCleanupInstruments]) modified |= RemoveUnusedInstruments();
	}

	// Samples
	if(m_CheckBoxes[kRemoveSamples]) modified |= RemoveAllSamples();
	if(m_CheckBoxes[kCleanupSamples]) modified |= RemoveUnusedSamples();
	if(m_CheckBoxes[kOptimizeSamples]) modified |= OptimizeSamples();
	if(modDoc.GetNumSamples() > 1)
	{
		if(m_CheckBoxes[kRearrangeSamples]) modified |= RearrangeSamples();
	}

	// Plugins
	if(m_CheckBoxes[kRemoveAllPlugins]) modified |= RemoveAllPlugins();
	if(m_CheckBoxes[kCleanupPlugins]) modified |= RemoveUnusedPlugins();

	// Create samplepack
	if(m_CheckBoxes[kResetVariables]) modified |= ResetVariables();

	// Remove unused channels
	if(m_CheckBoxes[kCleanupChannels]) modified |= RemoveUnusedChannels();

	if(modified) modDoc.SetModified();
	modDoc.UpdateAllViews(nullptr, UpdateHint().ModType());
	logcapturer.ShowLog(true);
	CDialog::OnOK();
}


void CModCleanupDlg::OnVerifyMutualExclusive()
{
	HWND hFocus = GetFocus()->m_hWnd;
	for(int i = 0; i < kMaxCleanupOptions; i++)	
	{
		// if this item is focussed, we have just (un)checked it.
		if(hFocus == GetDlgItem(m_CleanupIDtoDlgID[i])->m_hWnd)
		{
			// if we just unchecked it, there's nothing to verify.
			if(IsDlgButtonChecked(m_CleanupIDtoDlgID[i]) == BST_UNCHECKED)
				return;

			// now we can disable all elements that are mutually exclusive.
			if(m_MutuallyExclusive[i] != kNone)
				CheckDlgButton(m_CleanupIDtoDlgID[m_MutuallyExclusive[i]], BST_UNCHECKED);
			// find other elements which are mutually exclusive with the selected element.
			for(int j = 0; j < kMaxCleanupOptions; j++)	
			{
				if(m_MutuallyExclusive[j] == i)
					CheckDlgButton(m_CleanupIDtoDlgID[j], BST_UNCHECKED);
			}
			return;
		}
	}
}


void CModCleanupDlg::OnPresetCleanupSong()
{
	// patterns
	CheckDlgButton(IDC_CHK_CLEANUP_PATTERNS, BST_CHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_PATTERNS, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_REARRANGE_PATTERNS, BST_CHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_DUPLICATES, BST_CHECKED);
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
	CheckDlgButton(IDC_CHK_UNUSED_CHANNELS, BST_CHECKED);
}


void CModCleanupDlg::OnPresetCompoCleanup()
{
	// patterns
	CheckDlgButton(IDC_CHK_CLEANUP_PATTERNS, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_PATTERNS, BST_CHECKED);
	CheckDlgButton(IDC_CHK_REARRANGE_PATTERNS, BST_UNCHECKED);
	CheckDlgButton(IDC_CHK_REMOVE_DUPLICATES, BST_UNCHECKED);
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
	CheckDlgButton(IDC_CHK_UNUSED_CHANNELS, BST_CHECKED);
}


BOOL CModCleanupDlg::OnToolTipNotify(UINT, NMHDR *pNMHDR, LRESULT *)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
	UINT_PTR nID = pNMHDR->idFrom;
	if (pTTT->uFlags & TTF_IDISHWND)
	{
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	LPCTSTR lpszText = nullptr;
	switch(nID)
	{
	// patterns
	case IDC_CHK_CLEANUP_PATTERNS:
		lpszText = _T("Remove all unused patterns and rearrange them.");
		break;
	case IDC_CHK_REMOVE_PATTERNS:
		lpszText = _T("Remove all patterns.");
		break;
	case IDC_CHK_REARRANGE_PATTERNS:
		lpszText = _T("Number the patterns given by their order in the sequence.");
		break;
	case IDC_CHK_REMOVE_DUPLICATES:
		lpszText = _T("Merge patterns with identical content.");
		break;
	// orders
	case IDC_CHK_REMOVE_ORDERS:
		lpszText = _T("Reset the order list.");
		break;
	case IDC_CHK_MERGE_SEQUENCES:
		lpszText = _T("Merge multiple sequences into one.");
		break;
	// samples
	case IDC_CHK_CLEANUP_SAMPLES:
		lpszText = _T("Remove all unused samples.");
		break;
	case IDC_CHK_REMOVE_SAMPLES:
		lpszText = _T("Remove all samples.");
		break;
	case IDC_CHK_REARRANGE_SAMPLES:
		lpszText = _T("Reorder sample list by removing empty samples.");
		break;
	case IDC_CHK_OPTIMIZE_SAMPLES:
		lpszText = _T("Remove unused data after the sample loop end.");
		break;
	// instruments
	case IDC_CHK_CLEANUP_INSTRUMENTS:
		lpszText = _T("Remove all unused instruments.");
		break;
	case IDC_CHK_REMOVE_INSTRUMENTS:
		lpszText = _T("Remove all instruments and convert them to samples.");
		break;
	// plugins
	case IDC_CHK_CLEANUP_PLUGINS:
		lpszText = _T("Remove all unused plugins.");
		break;
	case IDC_CHK_REMOVE_PLUGINS:
		lpszText = _T("Remove all plugins.");
		break;
	// misc
	case IDC_CHK_SAMPLEPACK:
		lpszText = _T("Convert the module to .IT and reset song / sample / instrument variables");
		break;
	case IDC_CHK_UNUSED_CHANNELS:
		lpszText = _T("Removes all empty pattern channels.");
		break;
	default:
		lpszText = _T("");
		break;
	}
	pTTT->lpszText = const_cast<LPTSTR>(lpszText);
	return TRUE;
}


///////////////////////////////////////////////////////////////////////
// Actual cleanup implementations

bool CModCleanupDlg::RemoveDuplicatePatterns()
{
	CSoundFile &sndFile = modDoc.GetSoundFile();
	const PATTERNINDEX numPatterns = sndFile.Patterns.Size();
	std::vector<PATTERNINDEX> patternMapping(numPatterns, PATTERNINDEX_INVALID);

	BeginWaitCursor();
	CriticalSection cs;

	PATTERNINDEX foundDupes = 0;
	for(PATTERNINDEX pat1 = 0; pat1 < numPatterns; pat1++)
	{
		if(!sndFile.Patterns.IsValidPat(pat1))
			continue;
		const CPattern &pattern1 = sndFile.Patterns[pat1];
		for(PATTERNINDEX pat2 = pat1 + 1; pat2 < numPatterns; pat2++)
		{
			if(!sndFile.Patterns.IsValidPat(pat2) || patternMapping[pat2] != PATTERNINDEX_INVALID)
				continue;
			const CPattern &pattern2 = sndFile.Patterns[pat2];
			if(pattern1 == pattern2)
			{
				modDoc.GetPatternUndo().PrepareUndo(pat2, 0, 0, pattern2.GetNumChannels(), pattern2.GetNumRows(), "Remove Duplicate Patterns", foundDupes != 0, false);
				sndFile.Patterns.Remove(pat2);

				patternMapping[pat2] = pat1;
				foundDupes++;
			}
		}
	}

	if(foundDupes != 0)
	{
		modDoc.AddToLog(MPT_AFORMAT("{} duplicate pattern{} merged.")(foundDupes, foundDupes == 1 ? "" : "s"));

		// Fix order list
		for(auto &order : sndFile.Order)
		{
			for(auto &pat : order)
			{
				if(pat < numPatterns && patternMapping[pat] != PATTERNINDEX_INVALID)
				{
					pat = patternMapping[pat];
				}
			}
		}
	}

	EndWaitCursor();

	return foundDupes != 0;
}


// Remove unused patterns
bool CModCleanupDlg::RemoveUnusedPatterns()
{
	CSoundFile &sndFile = modDoc.GetSoundFile();
	const PATTERNINDEX numPatterns = sndFile.Patterns.Size();
	std::vector<bool> patternUsed(numPatterns, false);

	BeginWaitCursor();
	// First, find all used patterns in all sequences.
	for(auto &order : sndFile.Order)
	{
		for(auto pat : order)
		{
			if(pat < numPatterns)
			{
				patternUsed[pat] = true;
			}
		}
	}

	// Remove all other patterns.
	CriticalSection cs;
	PATTERNINDEX numRemovedPatterns = 0;
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		if(!patternUsed[pat] && sndFile.Patterns.IsValidPat(pat))
		{
			numRemovedPatterns++;
			modDoc.GetPatternUndo().PrepareUndo(pat, 0, 0, sndFile.GetNumChannels(), sndFile.Patterns[pat].GetNumRows(), "Remove Unused Patterns", numRemovedPatterns != 0, false);
			sndFile.Patterns.Remove(pat);
		}
	}
	EndWaitCursor();

	if(numRemovedPatterns)
	{
		modDoc.AddToLog(MPT_AFORMAT("{} pattern{} removed.")(numRemovedPatterns, numRemovedPatterns == 1 ? "" : "s"));
		return true;
	}
	return false;
}


// Rearrange patterns (first pattern in order list = 0, etc...)
bool CModCleanupDlg::RearrangePatterns()
{
	CSoundFile &sndFile = modDoc.GetSoundFile();

	const PATTERNINDEX numPatterns = sndFile.Patterns.Size();
	std::vector<PATTERNINDEX> newIndex(numPatterns, PATTERNINDEX_INVALID);

	bool modified = false;

	BeginWaitCursor();
	CriticalSection cs;

	// First, find all used patterns in all sequences.
	PATTERNINDEX patOrder = 0;
	for(auto &order : sndFile.Order)
	{
		for(auto &pat : order)
		{
			if(pat < numPatterns)
			{
				if(newIndex[pat] == PATTERNINDEX_INVALID)
				{
					newIndex[pat] = patOrder++;
				}
				pat = newIndex[pat];
			}
		}
	}
	// All unused patterns are moved to the end of the pattern list.
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		PATTERNINDEX &index = newIndex[pat];
		if(index == PATTERNINDEX_INVALID && sndFile.Patterns.IsValidPat(pat))
		{
			index = patOrder++;
		}
	}
	// Also need new indices for any non-existent patterns
	for(auto &index : newIndex)
	{
		if(index == PATTERNINDEX_INVALID)
		{
			index = patOrder++;
		}
	}

	modDoc.GetPatternUndo().RearrangePatterns(newIndex);

	// Now rearrange the actual patterns
	for(PATTERNINDEX i = 0; i < static_cast<PATTERNINDEX>(newIndex.size()); i++)
	{
		PATTERNINDEX j = newIndex[i];
		if(i == j)
			continue;
		while(i < j)
			j = newIndex[j];
		std::swap(sndFile.Patterns[i], sndFile.Patterns[j]);
		modified = true;
	}

	EndWaitCursor();

	return modified;
}


class UnusedSampleScanner : public CProgressDialog
{
public:
	UnusedSampleScanner(CModDoc &modDoc, std::vector<bool> &samplesUsed, std::set<PATTERNINDEX> &patternsToAnalyze)
		: m_modDoc{modDoc}
		, m_samplesUsed{samplesUsed}
		, m_patternsToAnalyze{patternsToAnalyze}
	{
	}

protected:
	void Run() override
	{
		SetTitle(_T("Cleanup"));

		CSoundFile &sndFile = m_modDoc.GetSoundFile();

		const auto subSongs = sndFile.GetAllSubSongs();
		const auto totalSamples = mpt::saturate_round<uint64>(std::accumulate(subSongs.begin(), subSongs.end(), 0.0, [](double acc, const auto &song) { return acc + song.duration; }) * sndFile.GetSampleRate());
		SetRange(0, totalSamples);

		CMainFrame::GetMainFrame()->StopMod(&m_modDoc);

		// We're not interested in plugin rendering
		std::bitset<MAX_MIXPLUGINS> plugMuteStatus;
		for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
		{
			plugMuteStatus[i] = sndFile.m_MixPlugins[i].IsBypassed();
			sndFile.m_MixPlugins[i].SetBypass(true);
		}

		const auto origSequence = sndFile.Order.GetCurrentSequenceIndex();
		const auto origRepeatCount = sndFile.GetRepeatCount();
		sndFile.SetRepeatCount(0);
		sndFile.m_bIsRendering = true;
		auto origPlayState = std::make_unique<CSoundFile::PlayState>(std::move(sndFile.m_PlayState));
		mpt::reconstruct(sndFile.m_PlayState);
		auto prevTime = timeGetTime();
		uint64 renderedSamples = 0;
		for(const auto &song : subSongs)
		{
			sndFile.ResetPlayPos();
			sndFile.GetLength(eAdjust, GetLengthTarget(song.startOrder, song.startRow).StartPos(song.sequence, 0, 0));
			sndFile.m_SongFlags.reset(SONG_PLAY_FLAGS);
			while(!m_abort)
			{
				auto tickSamples = sndFile.ReadOneTick();
				if(!tickSamples)
					break;

				renderedSamples += tickSamples;
				for(const auto &chn : sndFile.m_PlayState.Chn)
				{
					if(chn.pModSample == nullptr)
						continue;
					SAMPLEINDEX smp = static_cast<SAMPLEINDEX>(std::distance<const ModSample *>(&sndFile.GetSample(0), chn.pModSample));
					m_samplesUsed[smp] = true;
				}
				m_patternsToAnalyze.erase(sndFile.m_PlayState.m_nPattern);

				auto currentTime = timeGetTime();
				if(currentTime - prevTime >= 16)
				{
					prevTime = currentTime;
					SetText(MPT_CFORMAT("Finding unused samples... {}%")(renderedSamples * 100 / totalSamples));
					SetProgress(renderedSamples);
					ProcessMessages();
				}
			}
		}
		sndFile.m_PlayState = std::move(*origPlayState);
		sndFile.SetRepeatCount(origRepeatCount);
		sndFile.Order.SetSequence(origSequence);
		sndFile.m_bIsRendering = false;

		for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
		{
			sndFile.m_MixPlugins[i].SetBypass(plugMuteStatus[i]);
		}

		EndDialog(m_abort ? IDCANCEL : IDOK);
	}

	CModDoc &m_modDoc;
	std::vector<bool> &m_samplesUsed;
	std::set<PATTERNINDEX> &m_patternsToAnalyze;
};


// Remove unused samples
bool CModCleanupDlg::RemoveUnusedSamples()
{
	BeginWaitCursor();

	CSoundFile &sndFile = modDoc.GetSoundFile();
	SAMPLEINDEX numRemoved = 0;

	std::vector<bool> samplesUsed(sndFile.GetNumSamples() + 1, false);
	// Never count empty slots towards removed count
	for(SAMPLEINDEX smp = 1; smp <= sndFile.GetNumSamples(); smp++)
	{
		if(!sndFile.GetSample(smp).HasSampleData())
			samplesUsed[smp] = true;
	}

	std::set<PATTERNINDEX> patternsToAnalyze;
	for(PATTERNINDEX pat = 0; pat < sndFile.Patterns.GetNumPatterns(); pat++)
	{
		if(sndFile.Patterns.IsValidPat(pat))
			patternsToAnalyze.insert(pat);
	}

	if(sndFile.GetNumInstruments())
	{
		// Easy: Samples that aren't used by any instruments
		for(INSTRUMENTINDEX i = 1; i <= sndFile.GetNumInstruments(); i++)
		{
			if(sndFile.Instruments[i] != nullptr)
				sndFile.Instruments[i]->GetSamples(samplesUsed);
		}
		numRemoved = sndFile.RemoveSelectedSamples(samplesUsed);

		// Don't count the samples that we just removed twice towards total number of removed samples
		samplesUsed.assign(samplesUsed.size(), false);
		for(SAMPLEINDEX smp = 1; smp <= sndFile.GetNumSamples(); smp++)
		{
			if(!sndFile.GetSample(smp).HasSampleData())
				samplesUsed[smp] = true;
		}

		BypassInputHandler bih;
		UnusedSampleScanner dlg{modDoc, samplesUsed, patternsToAnalyze};
		if(dlg.DoModal() != IDOK)
			return (numRemoved > 0);
	}

	// Instrument mode: For patterns that were not played, just do basic checks
	// Sample mode: The heart of the unused sample detection
	std::vector<ModCommand::INSTR> lastIns;
	for(PATTERNINDEX pat : patternsToAnalyze)
	{
		lastIns.assign(sndFile.GetNumChannels(), 0);
		const auto &pattern = sndFile.Patterns[pat];
		for(ROWINDEX row = 0; row < pattern.GetNumRows(); row++)
		{
			for(CHANNELINDEX chn = 0; chn < pattern.GetNumChannels(); chn++)
			{
				const auto &m = *pattern.GetpModCommand(row, chn);
				if(m.IsPcNote())
					continue;

				auto instr = m.instr;
				if(instr)
					lastIns[chn] = instr;
				else
					instr = lastIns[chn];

				if(sndFile.GetNumInstruments())
				{
					if(m.IsNote() && sndFile.Instruments[instr])
					{
						auto sample = sndFile.Instruments[instr]->Keyboard[m.note - NOTE_MIN];
						if(sample < samplesUsed.size())
							samplesUsed[sample] = true;
					}
				} else
				{
					if(instr < samplesUsed.size())
						samplesUsed[instr] = true;
				}
			}
		}
	}

	EndWaitCursor();

	if(sndFile.GetNumInstruments())
	{
		const auto unusedInsSamples = std::count(samplesUsed.begin() + 1, samplesUsed.end(), false);
		if(unusedInsSamples)
		{
			mpt::ustring s = MPT_UFORMAT("OpenMPT detected {} sample{} referenced by an instrument,\nbut not used in the song. Do you want to remove them?")(unusedInsSamples, (unusedInsSamples == 1) ? U_("") : U_("s"));
			if(Reporting::Confirm(s, "Sample Cleanup", false, false, this) == cnfYes)
			{
				numRemoved += sndFile.RemoveSelectedSamples(samplesUsed);
			}
		}
	} else
	{
		numRemoved += sndFile.RemoveSelectedSamples(samplesUsed);
	}

	if(numRemoved > 0)
	{
		modDoc.AddToLog(LogNotification, MPT_UFORMAT("{} unused sample{} removed")(numRemoved, (numRemoved == 1) ? U_("") : U_("s")));
	}

	return (numRemoved > 0);
}


// Check if the left and right channel of a sample contain identical data
template<typename T>
static bool CompareStereoChannels(SmpLength length, const T *sampleData)
{
	for(SmpLength i = 0; i < length; i++, sampleData += 2)
	{
		if(sampleData[0] != sampleData[1])
		{
			return false;
		}
	}
	return true;
}

// Remove unused sample data
bool CModCleanupDlg::OptimizeSamples()
{
	CSoundFile &sndFile = modDoc.GetSoundFile();

	SAMPLEINDEX numLoopOpt = 0, numStereoOpt = 0;
	std::vector<bool> stereoOptSamples(sndFile.GetNumSamples(), false);
	
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
				loopLength = std::max(sample.nLoopEnd, sample.nSustainEnd);
			}
		}

		// Check if the sample contains identical stereo channels
		if(sample.GetNumChannels() == 2)
		{
			bool identicalChannels = false;
			if(sample.GetElementarySampleSize() == 1)
			{
				identicalChannels = CompareStereoChannels(loopLength, sample.sample8());
			} else if(sample.GetElementarySampleSize() == 2)
			{
				identicalChannels = CompareStereoChannels(loopLength, sample.sample16());
			}
			if(identicalChannels)
			{
				numStereoOpt++;
				stereoOptSamples[smp - 1] = true;
			}
		}

		if(sample.HasSampleData() && sample.nLength > loopLength) numLoopOpt++;
	}
	if(!numLoopOpt && !numStereoOpt) return false;

	std::string s;
	if(numLoopOpt)
		s = MPT_AFORMAT("{} sample{} unused data after the loop end point.\n")(numLoopOpt, (numLoopOpt == 1) ? " has" : "s have");
	if(numStereoOpt)
		s += MPT_AFORMAT("{} stereo sample{} actually mono.\n")(numStereoOpt, (numStereoOpt == 1) ? " is" : "s are");
	if(numLoopOpt + numStereoOpt == 1)
		s += "Do you want to optimize it and remove this unused data?";
	else
		s += "Do you want to optimize them and remove this unused data?";

	if(Reporting::Confirm(s.c_str(), "Sample Optimization", false, false, this) != cnfYes)
	{
		return false;
	}

	for(SAMPLEINDEX smp = 1; smp <= sndFile.m_nSamples; smp++)
	{
		ModSample &sample = sndFile.GetSample(smp);

		// Determine how much of the sample will be played
		SmpLength loopLength = sample.nLength;
		if(sample.uFlags[CHN_LOOP])
		{
			loopLength = sample.nLoopEnd;

			// Sustain loop is played before normal loop, and it can actually be located after the normal loop.
			if(sample.uFlags[CHN_SUSTAINLOOP])
			{
				loopLength = std::max(sample.nLoopEnd, sample.nSustainEnd);
			}
		}

		if(sample.nLength > loopLength && loopLength >= 2)
		{
			modDoc.GetSampleUndo().PrepareUndo(smp, sundo_delete, "Trim Unused Data", loopLength, sample.nLength);
			SampleEdit::ResizeSample(sample, loopLength, sndFile);
		}

		// Convert stereo samples with identical channels to mono
		if(stereoOptSamples[smp - 1])
		{
			modDoc.GetSampleUndo().PrepareUndo(smp, sundo_replace, "Mono Conversion");
			ctrlSmp::ConvertToMono(sample, sndFile, ctrlSmp::onlyLeft);
		}
	}
	if(numLoopOpt)
	{
		s = MPT_AFORMAT("{} sample loop{} optimized")(numLoopOpt, (numLoopOpt == 1) ? "" : "s");
		modDoc.AddToLog(s);
	}
	if(numStereoOpt)
	{
		s = MPT_AFORMAT("{} sample{} converted to mono")(numStereoOpt, (numStereoOpt == 1) ? "" : "s");
		modDoc.AddToLog(s);
	}
	return true;
}

// Rearrange sample list
bool CModCleanupDlg::RearrangeSamples()
{
	CSoundFile &sndFile = modDoc.GetSoundFile();
	if(sndFile.GetNumSamples() < 2)
		return false;

	std::vector<SAMPLEINDEX> sampleMap;
	sampleMap.reserve(sndFile.GetNumSamples());

	// First, find out which sample slots are unused and create the new sample map only with used samples
	for(SAMPLEINDEX i = 1; i <= sndFile.GetNumSamples(); i++)
	{
		if(sndFile.GetSample(i).HasSampleData())
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
{
	CSoundFile &sndFile = modDoc.GetSoundFile();
	if(!sndFile.GetNumInstruments())
		return false;

	deleteInstrumentSamples removeSamples = doNoDeleteAssociatedSamples;
	if(Reporting::Confirm("Remove samples associated with unused instruments?", "Removing unused instruments", false, false, this) == cnfYes)
	{
		removeSamples = deleteAssociatedSamples;
	}

	BeginWaitCursor();

	std::vector<bool> instrUsed(sndFile.GetNumInstruments());
	bool prevUsed = true, reorder = false;
	INSTRUMENTINDEX numUsed = 0, lastUsed = 1;
	for(INSTRUMENTINDEX i = 0; i < sndFile.GetNumInstruments(); i++)
	{
		instrUsed[i] = (modDoc.IsInstrumentUsed(i + 1));
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

		modDoc.AddToLog(LogNotification, MPT_UFORMAT("{} unused instrument{} removed")(numRemoved, (numRemoved == 1) ? U_("") : U_("s")));
		return true;
	}
	return false;
}


// Remove ununsed plugins
bool CModCleanupDlg::RemoveUnusedPlugins()
{
	CSoundFile &sndFile = modDoc.GetSoundFile();

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
		if(usedmap[nPlug])
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

	PLUGINDEX numRemoved = modDoc.RemovePlugs(usedmap);
	if(numRemoved != 0)
	{
		modDoc.AddToLog(LogInformation, MPT_UFORMAT("{} unused plugin{} removed")(numRemoved, (numRemoved == 1) ? U_("") : U_("s")));
		return true;
	}
	return false;
}


// Reset variables (convert to IT, reset global/smp/ins vars, etc.)
bool CModCleanupDlg::ResetVariables()
{
	CSoundFile &sndFile = modDoc.GetSoundFile();

	if(Reporting::Confirm(_T("OpenMPT will convert the module to IT format and reset all song, sample and instrument attributes to default values. Continue?"), _T("Resetting variables"), false, false, this) == cnfNo)
		return false;

	// Stop play.
	CMainFrame::GetMainFrame()->StopMod(&modDoc);

	BeginWaitCursor();
	CriticalSection cs;

	// Convert to IT...
	modDoc.ChangeModType(MOD_TYPE_IT);
	sndFile.SetDefaultPlaybackBehaviour(sndFile.GetType());
	sndFile.SetMixLevels(MixLevels::Compatible);
	sndFile.m_songArtist.clear();
	sndFile.m_nTempoMode = TempoMode::Classic;
	sndFile.m_SongFlags = SONG_LINEARSLIDES;
	sndFile.m_MidiCfg.Reset();
	
	// Global vars
	sndFile.m_nDefaultTempo.Set(125);
	sndFile.m_nDefaultSpeed = 6;
	sndFile.m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	sndFile.m_nSamplePreAmp = 48;
	sndFile.m_nVSTiVolume = 48;
	sndFile.Order().SetRestartPos(0);

	if(sndFile.Order().empty())
	{
		modDoc.InsertPattern(64, 0);
	}

	// Reset instruments (if there are any)
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

	for(CHANNELINDEX chn = 0; chn < sndFile.GetNumChannels(); chn++)
	{
		sndFile.InitChannel(chn);
	}

	// reset samples
	SampleEdit::ResetSamples(sndFile, SampleEdit::SmpResetCompo);

	cs.Leave();
	EndWaitCursor();

	return true;
}


bool CModCleanupDlg::RemoveUnusedChannels()
{
	// Avoid M.K. modules to become xCHN modules if some channels are unused.
	if(modDoc.GetModType() == MOD_TYPE_MOD && modDoc.GetNumChannels() == 4)
		return false;

	std::vector<bool> usedChannels;
	modDoc.CheckUsedChannels(usedChannels, modDoc.GetNumChannels() - modDoc.GetSoundFile().GetModSpecifications().channelsMin);
	return modDoc.RemoveChannels(usedChannels);
}


// Remove all patterns
bool CModCleanupDlg::RemoveAllPatterns()
{
	CSoundFile &sndFile = modDoc.GetSoundFile();

	if(sndFile.Patterns.Size() == 0) return false;
	modDoc.GetPatternUndo().ClearUndo();
	sndFile.Patterns.ResizeArray(0);
	sndFile.SetCurrentOrder(0);
	return true;
}

// Remove all orders
bool CModCleanupDlg::RemoveAllOrders()
{
	CSoundFile &sndFile = modDoc.GetSoundFile();

	sndFile.Order.Initialize();
	sndFile.SetCurrentOrder(0);
	return true;
}

// Remove all samples
bool CModCleanupDlg::RemoveAllSamples()
{
	CSoundFile &sndFile = modDoc.GetSoundFile();

	if (sndFile.GetNumSamples() == 0) return false;

	std::vector<bool> keepSamples(sndFile.GetNumSamples() + 1, false);
	sndFile.RemoveSelectedSamples(keepSamples);

	SampleEdit::ResetSamples(sndFile, SampleEdit::SmpResetInit, 1, MAX_SAMPLES - 1);

	return true;
}

// Remove all instruments
bool CModCleanupDlg::RemoveAllInstruments()
{
	CSoundFile &sndFile = modDoc.GetSoundFile();

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
{
	std::vector<bool> keepMask(MAX_MIXPLUGINS, false);
	modDoc.RemovePlugs(keepMask);
	return true;
}


bool CModCleanupDlg::MergeSequences()
{
	return modDoc.GetSoundFile().Order.MergeSequences();
}


OPENMPT_NAMESPACE_END
