/*
 * PatternEditorDialogs.cpp
 * ------------------------
 * Purpose: Code for various dialogs that are used in the pattern editor.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "view_pat.h"
#include "PatternEditorDialogs.h"


// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
static void getXParam(BYTE command, PATTERNINDEX nPat, ROWINDEX nRow, CHANNELINDEX nChannel, CSoundFile &sndFile, UINT &xparam, UINT &multiplier)
//-----------------------------------------------------------------------------------------------------------------------------------------------
{
	UINT xp = 0, mult = 1;
	int nCmdRow = (int)nRow;

	if(command == CMD_XPARAM)
	{
		// current command is a parameter extension command
		nCmdRow--;

		// Try to find previous command parameter to be extended
		while(nCmdRow >= 0)
		{
			const ModCommand *m = sndFile.Patterns[nPat].GetpModCommand(nCmdRow, nChannel);
			if(m->command == CMD_OFFSET || m->command == CMD_PATTERNBREAK || m->command == CMD_PATTERNBREAK)
				break;
			if(m->command != CMD_XPARAM)
			{
				nCmdRow = -1;
				break;
			}
			nCmdRow--;
		}
	} else if(command != CMD_OFFSET && command != CMD_PATTERNBREAK && command != CMD_TEMPO)
	{
		// If current row do not own any satisfying command parameter to extend, set return state
		nCmdRow = -1;
	}

	if(nCmdRow >= 0)
	{
		// An 'extendable' command parameter has been found
		const ModCommand *m = sndFile.Patterns[nPat].GetpModCommand(nCmdRow, nChannel);

		// Find extension resolution (8 to 24 bits)
		ROWINDEX n = 1;
		while(n < 4 && nCmdRow + n < sndFile.Patterns[nPat].GetNumRows())
		{
			if(sndFile.Patterns[nPat].GetpModCommand(nCmdRow + n, nChannel)->command != CMD_XPARAM) break;
			n++;
		}

		// Parameter extension found (above 8 bits non-standard parameters)
		if(n > 1)
		{
			// Limit offset command to 24 bits, other commands to 16 bits
			n = m->command == CMD_OFFSET ? n : (n > 2 ? 2 : n);

			// Compute extended value WITHOUT current row parameter value : this parameter
			// is being currently edited (this is why this function is being called) so we
			// only need to compute a multiplier so that we can add its contribution while
			// its value is changed by user
			for(UINT j = 0; j < n; j++)
			{
				m = sndFile.Patterns[nPat].GetpModCommand(nCmdRow + j, nChannel);

				UINT k = 8 * (n - j - 1);
				if(nCmdRow + j == nRow) 
					mult = 1 << k;
				else
					xp += (m->param << k);
			}
		} else if(m->command == CMD_OFFSET)
		{
			// No parameter extension to perform (8 bits standard parameter),
			// just care about offset command special case (16 bits, fake)
			mult <<= 8;
		}
	}

	// Return x-parameter
	multiplier = mult;
	xparam = xp;
}
// -! NEW_FEATURE#0010


//////////////////////////////////////////////////////////////////////////////////////////
// Find/Replace Dialog

BEGIN_MESSAGE_MAP(CFindReplaceTab, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnInstrChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,	OnVolCmdChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,	OnVolumeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5,	OnEffectChanged)
	ON_CBN_SELCHANGE(IDC_COMBO6,	OnParamChanged)
	ON_COMMAND(IDC_CHECK1,			OnCheckNote)
	ON_COMMAND(IDC_CHECK2,			OnCheckInstr)
	ON_COMMAND(IDC_CHECK3,			OnCheckVolCmd)
	ON_COMMAND(IDC_CHECK4,			OnCheckVolume)
	ON_COMMAND(IDC_CHECK5,			OnCheckEffect)
	ON_COMMAND(IDC_CHECK6,			OnCheckParam)

	ON_COMMAND(IDC_CHECK7,			OnCheckChannelSearch)
END_MESSAGE_MAP()


BOOL CFindReplaceTab::OnInitDialog()
//----------------------------------
{
	CHAR s[256];
	CComboBox *combo;

	CPropertyPage::OnInitDialog();
	// Search flags
	CheckDlgButton(IDC_CHECK1, m_Flags[FindReplace::Note] ? MF_CHECKED : MF_UNCHECKED);
	CheckDlgButton(IDC_CHECK2, m_Flags[FindReplace::Instr] ? MF_CHECKED : MF_UNCHECKED);
	CheckDlgButton(IDC_CHECK3, m_Flags[FindReplace::VolCmd] ? MF_CHECKED : MF_UNCHECKED);
	CheckDlgButton(IDC_CHECK4, m_Flags[FindReplace::Volume] ? MF_CHECKED : MF_UNCHECKED);
	CheckDlgButton(IDC_CHECK5, m_Flags[FindReplace::Command] ? MF_CHECKED : MF_UNCHECKED);
	CheckDlgButton(IDC_CHECK6, m_Flags[FindReplace::Param] ? MF_CHECKED : MF_UNCHECKED);
	if(m_bReplace)
	{
		CheckDlgButton(IDC_CHECK7, m_Flags[FindReplace::Replace] ? MF_CHECKED : MF_UNCHECKED);
		CheckDlgButton(IDC_CHECK8, m_Flags[FindReplace::ReplaceAll] ? MF_CHECKED : MF_UNCHECKED);
	} else
	{
		CheckDlgButton(IDC_CHECK7, m_Flags[FindReplace::InChannels] ? MF_CHECKED : MF_UNCHECKED);
		int nButton = IDC_RADIO1;
		if(m_Flags[FindReplace::FullSearch])
		{
			nButton = IDC_RADIO2;
		} else if(m_bPatSel)
		{
			nButton = IDC_RADIO3;
		}
		CheckRadioButton(IDC_RADIO1, IDC_RADIO3, nButton);
		GetDlgItem(IDC_RADIO3)->EnableWindow(m_bPatSel ? TRUE : FALSE);
		SetDlgItemInt(IDC_EDIT1, m_nMinChannel + 1);
		SetDlgItemInt(IDC_EDIT2, m_nMaxChannel + 1);
	}
	// Note
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL)
	{
		combo->InitStorage(150, 6);
		combo->SetItemData(combo->AddString("..."), 0);
		if (m_bReplace)
		{
			combo->SetItemData(combo->AddString("note -1"), replaceNoteMinusOne);
			combo->SetItemData(combo->AddString("note +1"), replaceNotePlusOne);
			combo->SetItemData(combo->AddString("-1 oct"), replaceNoteMinusOctave);
			combo->SetItemData(combo->AddString("+1 oct"), replaceNotePlusOctave);
		} else
		{
			combo->SetItemData(combo->AddString("any"), findAny);
		}
		AppendNotesToControlEx(*combo, &sndFile);

		UINT ncount = combo->GetCount();
		for (UINT i=0; i<ncount; i++) if (m_Cmd.note == combo->GetItemData(i))
		{
			combo->SetCurSel(i);
			break;
		}
	}
	// Instrument
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO2)) != NULL)
	{
		combo->SetItemData(combo->AddString(".."), 0);
		if (m_bReplace)
		{
			combo->SetItemData(combo->AddString("ins -1"), replaceInstrumentMinusOne);
			combo->SetItemData(combo->AddString("ins +1"), replaceInstrumentPlusOne);
		}
		for(INSTRUMENTINDEX n = 1; n < MAX_INSTRUMENTS; n++)
		{
			if(sndFile.GetNumInstruments())
			{
				wsprintf(s, "%03d:%s", n, sndFile.GetInstrumentName(n));
			} else
			{
				wsprintf(s, "%03d:%s", n, sndFile.m_szNames[n]);
			}
			combo->SetItemData(combo->AddString(s), n);
		}
		UINT ncount = combo->GetCount();
		for (UINT i=0; i<ncount; i++)
		{
			if (m_Cmd.instr == combo->GetItemData(i) || (cInstrRelChange == -1 && combo->GetItemData(i) == replaceInstrumentMinusOne) || (cInstrRelChange == 1 && combo->GetItemData(i) == replaceInstrumentPlusOne))
			{
				combo->SetCurSel(i);
				break;
			}
		}
	}
	// Volume Command
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO3)) != NULL)
	{
		combo->InitStorage(effectInfo.GetNumVolCmds(), 15);
		combo->SetItemData(combo->AddString(" None"), (DWORD)-1);
		UINT count = effectInfo.GetNumVolCmds();
		for (UINT n=0; n<count; n++)
		{
			if(effectInfo.GetVolCmdInfo(n, s) && s[0])
			{
				combo->SetItemData(combo->AddString(s), n);
			}
		}
		combo->SetCurSel(0);
		UINT fxndx = effectInfo.GetIndexFromVolCmd(ModCommand::VOLCMD(m_Cmd.volcmd));
		for (UINT i=0; i<=count; i++) if (fxndx == combo->GetItemData(i))
		{
			combo->SetCurSel(i);
			break;
		}
	}
	// Volume
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO4)) != NULL)
	{
		combo->InitStorage(64, 4);
		for (UINT n=0; n<=64; n++)
		{
			wsprintf(s, "%02d", n);
			combo->SetItemData(combo->AddString(s), n);
		}
		UINT ncount = combo->GetCount();
		for (UINT i=0; i<ncount; i++) if (m_Cmd.vol == combo->GetItemData(i))
		{
			combo->SetCurSel(i);
			break;
		}
	}
	// Command
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO5)) != NULL)
	{
		combo->InitStorage(effectInfo.GetNumEffects(), 20);
		combo->SetItemData(combo->AddString(" None"), (DWORD)-1);
		UINT count = effectInfo.GetNumEffects();
		for (UINT n=0; n<count; n++)
		{
			if(effectInfo.GetEffectInfo(n, s, true) && s[0])
			{
				combo->SetItemData(combo->AddString(s), n);
			}
		}
		combo->SetCurSel(0);
		UINT fxndx = effectInfo.GetIndexFromEffect(ModCommand::COMMAND(m_Cmd.command), ModCommand::PARAM(m_Cmd.param));
		for (UINT i=0; i<=count; i++) if (fxndx == combo->GetItemData(i))
		{
			combo->SetCurSel(i);
			break;
		}
	}
	ChangeEffect();
	ChangeVolCmd();
	OnCheckChannelSearch();
	return TRUE;
}


void CFindReplaceTab::ChangeEffect()
//----------------------------------
{
	int fxndx = -1;
	CComboBox *combo;
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO5)) != NULL)
	{
		fxndx = combo->GetItemData(combo->GetCurSel());
	}
	// Update Param range
	if (((combo = (CComboBox *)GetDlgItem(IDC_COMBO6)) != NULL))
	{
		UINT oldcount = combo->GetCount();
		UINT newcount = effectInfo.IsExtendedEffect(fxndx) ? 16 : 256;
		if (oldcount != newcount)
		{
			CHAR s[16];
			int newpos;
			if (oldcount) newpos = combo->GetCurSel() % newcount; else newpos = m_Cmd.param % newcount;
			combo->ResetContent();
			combo->InitStorage(newcount, 4);
			for (UINT i=0; i<newcount; i++)
			{
				wsprintf(s, (newcount == 256) ? "%02X" : "%X", i);
				combo->SetItemData(combo->AddString(s), i);
			}
			combo->SetCurSel(newpos);
		}
	}
}


void CFindReplaceTab::ChangeVolCmd()
//----------------------------------
{
	int fxndx = -1;
	CComboBox *combo;
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO3)) != NULL)
	{
		fxndx = combo->GetItemData(combo->GetCurSel());
	}
	// Update Param range
	if (((combo = (CComboBox *)GetDlgItem(IDC_COMBO4)) != NULL))
	{
		DWORD rangeMin, rangeMax;
		if(!effectInfo.GetVolCmdInfo(fxndx, nullptr, &rangeMin, &rangeMax))
		{
			rangeMin = 0;
			rangeMax = 64;
		}
		UINT oldcount = combo->GetCount();
		UINT newcount = rangeMax - rangeMin + 1;
		if (oldcount != newcount)
		{
			CHAR s[16];
			int newpos;
			if (oldcount) newpos = combo->GetCurSel() % newcount; else newpos = m_Cmd.param % newcount;
			combo->ResetContent();
			for (UINT i = rangeMin; i <= rangeMax; i++)
			{
				wsprintf(s, (rangeMax < 10) ? "%d" : "%02d", i);
				combo->SetItemData(combo->AddString(s), i);
			}
			combo->SetCurSel(newpos);
		}
	}
}


void CFindReplaceTab::OnCheckChannelSearch()
//------------------------------------------
{
	if (!m_bReplace)
	{
		BOOL b = IsDlgButtonChecked(IDC_CHECK7);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT1), b);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT2), b);
	}
}


void CFindReplaceTab::OnOK()
//--------------------------
{
	CComboBox *combo;

	// Search flags
	m_Flags.reset();
	m_Flags.set(FindReplace::Note, !!IsDlgButtonChecked(IDC_CHECK1));
	m_Flags.set(FindReplace::Instr, !!IsDlgButtonChecked(IDC_CHECK2));
	m_Flags.set(FindReplace::VolCmd, !!IsDlgButtonChecked(IDC_CHECK3));
	m_Flags.set(FindReplace::Volume, !!IsDlgButtonChecked(IDC_CHECK4));
	m_Flags.set(FindReplace::Command, !!IsDlgButtonChecked(IDC_CHECK5));
	m_Flags.set(FindReplace::Param, !!IsDlgButtonChecked(IDC_CHECK6));
	if(m_bReplace)
	{
		m_Flags.set(FindReplace::Replace, !!IsDlgButtonChecked(IDC_CHECK7));
		m_Flags.set(FindReplace::ReplaceAll, !!IsDlgButtonChecked(IDC_CHECK8));
	} else
	{
		m_Flags.set(FindReplace::InChannels, !!IsDlgButtonChecked(IDC_CHECK7));
		m_Flags.set(FindReplace::FullSearch, !!IsDlgButtonChecked(IDC_RADIO2));
		m_Flags.set(FindReplace::InPatSelection, !!IsDlgButtonChecked(IDC_RADIO3));
	}
	// Note
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL)
	{
		m_Cmd.note = static_cast<ModCommand::NOTE>(combo->GetItemData(combo->GetCurSel()));
	}
	// Instrument
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO2)) != NULL)
	{
		m_Cmd.instr = 0;
		cInstrRelChange = 0;
		switch(combo->GetItemData(combo->GetCurSel()))
		{
		case replaceInstrumentMinusOne:
			cInstrRelChange = -1;
			break;
		case replaceInstrumentPlusOne:
			cInstrRelChange = 1;
			break;
		default:
			m_Cmd.instr = static_cast<ModCommand::INSTR>(combo->GetItemData(combo->GetCurSel()));
			break;
		}
	}
	// Volume Command
	if (((combo = (CComboBox *)GetDlgItem(IDC_COMBO3)) != NULL))
	{
		m_Cmd.volcmd = effectInfo.GetVolCmdFromIndex(combo->GetItemData(combo->GetCurSel()));
	}
	// Volume
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO4)) != NULL)
	{
		m_Cmd.vol = static_cast<ModCommand::VOL>(combo->GetItemData(combo->GetCurSel()));
	}
	// Effect
	int effectIndex = -1;
	if (((combo = (CComboBox *)GetDlgItem(IDC_COMBO5)) != NULL))
	{
		int n = -1; // unused parameter adjustment
		effectIndex = combo->GetItemData(combo->GetCurSel());
		m_Cmd.command = effectInfo.GetEffectFromIndex(effectIndex, n);
	}
	// Param
	m_Cmd.param = 0;
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO6)) != NULL)
	{
		m_Cmd.param = static_cast<ModCommand::PARAM>(combo->GetItemData(combo->GetCurSel()));

		// Apply parameter value mask if required (e.g. SDx has mask D0).
		if (effectIndex > -1)
		{
			m_Cmd.param |= effectInfo.GetEffectMaskFromIndex(effectIndex);
		}
	}
	// Min/Max channels
	if (!m_bReplace)
	{
		m_nMinChannel = static_cast<CHANNELINDEX>(GetDlgItemInt(IDC_EDIT1) - 1);
		m_nMaxChannel = static_cast<CHANNELINDEX>(GetDlgItemInt(IDC_EDIT2) - 1);
		if (m_nMaxChannel < m_nMinChannel)
		{
			std::swap(m_nMinChannel, m_nMaxChannel);
		}
	}
	CPropertyPage::OnOK();
}


/////////////////////////////////////////////////////////////////////////////////////////////
// CPatternPropertiesDlg

BEGIN_MESSAGE_MAP(CPatternPropertiesDlg, CDialog)
	ON_COMMAND(IDC_BUTTON_HALF,		OnHalfRowNumber)
	ON_COMMAND(IDC_BUTTON_DOUBLE,	OnDoubleRowNumber)
	ON_COMMAND(IDC_CHECK1,			OnOverrideSignature)
END_MESSAGE_MAP()

BOOL CPatternPropertiesDlg::OnInitDialog()
//----------------------------------------
{
	CComboBox *combo;
	CDialog::OnInitDialog();
	combo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	const CSoundFile &sndFile = modDoc.GetrSoundFile();

	if(m_nPattern < sndFile.Patterns.Size() && combo)
	{
		CHAR s[256];
		UINT nrows = sndFile.Patterns[m_nPattern].GetNumRows();

		const CModSpecifications& specs = sndFile.GetModSpecifications();
		for (UINT irow = specs.patternRowsMin; irow <= specs.patternRowsMax; irow++)
		{
			wsprintf(s, "%d", irow);
			combo->AddString(s);
		}
		combo->SetCurSel(nrows - specs.patternRowsMin);
		wsprintf(s, "Pattern #%d: %d row%s (%dK)",
			m_nPattern,
			sndFile.Patterns[m_nPattern].GetNumRows(),
			(sndFile.Patterns[m_nPattern].GetNumRows() == 1) ? "" : "s",
			(sndFile.Patterns[m_nPattern].GetNumRows() * sndFile.GetNumChannels() * sizeof(ModCommand)) / 1024);
		SetDlgItemText(IDC_TEXT1, s);

		// Window title
		const CString patternName = sndFile.Patterns[m_nPattern].GetName();
		wsprintf(s, "Pattern Properties for Pattern #%d", m_nPattern);
		if(!patternName.IsEmpty())
		{
			strcat(s, " (");
			strcat(s, patternName);
			strcat(s, ")");
		}
		SetWindowText(s);

		// pattern time signature
		const bool bOverride = sndFile.Patterns[m_nPattern].GetOverrideSignature();
		UINT nRPB = sndFile.Patterns[m_nPattern].GetRowsPerBeat(), nRPM = sndFile.Patterns[m_nPattern].GetRowsPerMeasure();
		if(nRPB == 0 || !bOverride) nRPB = sndFile.m_nDefaultRowsPerBeat;
		if(nRPM == 0 || !bOverride) nRPM = sndFile.m_nDefaultRowsPerMeasure;

		GetDlgItem(IDC_CHECK1)->EnableWindow(sndFile.GetModSpecifications().hasPatternSignatures ? TRUE : FALSE);
		CheckDlgButton(IDC_CHECK1, bOverride ? MF_CHECKED : MF_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_ROWSPERBEAT, nRPB, FALSE);
		SetDlgItemInt(IDC_EDIT_ROWSPERMEASURE, nRPM, FALSE);
		OnOverrideSignature();
	}
	return TRUE;
}


void CPatternPropertiesDlg::OnHalfRowNumber()
//-------------------------------------------
{
	const CSoundFile &sndFile = modDoc.GetrSoundFile();

	UINT nRows = GetDlgItemInt(IDC_COMBO1, NULL, FALSE);
	nRows /= 2;
	if(nRows < sndFile.GetModSpecifications().patternRowsMin)
		nRows = sndFile.GetModSpecifications().patternRowsMin;
	SetDlgItemInt(IDC_COMBO1, nRows, FALSE);
}


void CPatternPropertiesDlg::OnDoubleRowNumber()
//---------------------------------------------
{
	const CSoundFile &sndFile = modDoc.GetrSoundFile();

	UINT nRows = GetDlgItemInt(IDC_COMBO1, NULL, FALSE);
	nRows *= 2;
	if(nRows > sndFile.GetModSpecifications().patternRowsMax)
		nRows = sndFile.GetModSpecifications().patternRowsMax;
	SetDlgItemInt(IDC_COMBO1, nRows, FALSE);
}


void CPatternPropertiesDlg::OnOverrideSignature()
//-----------------------------------------------
{	
	GetDlgItem(IDC_EDIT_ROWSPERBEAT)->EnableWindow(IsDlgButtonChecked(IDC_CHECK1));
	GetDlgItem(IDC_EDIT_ROWSPERMEASURE)->EnableWindow(IsDlgButtonChecked(IDC_CHECK1));
}


void CPatternPropertiesDlg::OnOK()
//--------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();
	// Update pattern signature if necessary
	if(sndFile.GetModSpecifications().hasPatternSignatures)
	{
		if(IsDlgButtonChecked(IDC_CHECK1))	// Enable signature
		{
			ROWINDEX nNewBeat = (ROWINDEX)GetDlgItemInt(IDC_EDIT_ROWSPERBEAT, NULL, FALSE), nNewMeasure = (ROWINDEX)GetDlgItemInt(IDC_EDIT_ROWSPERMEASURE, NULL, FALSE);
			if(nNewBeat != sndFile.Patterns[m_nPattern].GetRowsPerBeat() || nNewMeasure != sndFile.Patterns[m_nPattern].GetRowsPerMeasure())
			{
				if(!sndFile.Patterns[m_nPattern].SetSignature(nNewBeat, nNewMeasure))
				{
					Reporting::Error("Invalid time signature!", "Pattern Properties");
					GetDlgItem(IDC_EDIT_ROWSPERBEAT)->SetFocus();
					return;
				}
				modDoc.SetModified();
			}
		} else	// Disable signature
		{
			if(sndFile.Patterns[m_nPattern].GetOverrideSignature())
			{
				sndFile.Patterns[m_nPattern].RemoveSignature();
				modDoc.SetModified();
			}
		}
	}

	const ROWINDEX newSize = (ROWINDEX)GetDlgItemInt(IDC_COMBO1, NULL, FALSE);

	// Check if any pattern data would be removed.
	bool resize = (newSize != sndFile.Patterns[m_nPattern].GetNumRows());
	for(ROWINDEX row = newSize; row < sndFile.Patterns[m_nPattern].GetNumRows(); row++)
	{
		if(!sndFile.Patterns[m_nPattern].IsEmptyRow(row))
		{
			resize = (Reporting::Confirm("Data at the end of the pattern will be lost.\nDo you want to continue?", "Shrink Pattern") == cnfYes);
			break;
		}
	}

	if(resize)
	{
		modDoc.GetPatternUndo().PrepareUndo(m_nPattern, 0, 0, sndFile.Patterns[m_nPattern].GetNumChannels(), sndFile.Patterns[m_nPattern].GetNumRows());
		modDoc.BeginWaitCursor();
		if(sndFile.Patterns[m_nPattern].Resize(newSize))
		{
			modDoc.SetModified();
		}
		modDoc.EndWaitCursor();
	}
	CDialog::OnOK();
}


////////////////////////////////////////////////////////////////////////////////////////////
// CEditCommand

BEGIN_MESSAGE_MAP(CEditCommand, CPropertySheet)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


CEditCommand::CEditCommand()
//--------------------------
{
	m_pModDoc = NULL;
	m_hWndView = NULL;
	m_nPattern = 0;
	m_nRow = 0;
	m_nChannel = 0;
	m_pageNote = NULL;
	m_pageVolume = NULL;
	m_pageEffect = NULL;
	m_bModified = false;
}


BOOL CEditCommand::SetParent(CWnd *parent, CModDoc *pModDoc)
//----------------------------------------------------------
{
	if ((!parent) || (!pModDoc)) return FALSE;
	m_hWndView = parent->m_hWnd;
	m_pModDoc = pModDoc;
	m_pageNote = new CPageEditNote(m_pModDoc->GetrSoundFile(), this);
	m_pageVolume = new CPageEditVolume(m_pModDoc->GetrSoundFile(), this);
	m_pageEffect = new CPageEditEffect(m_pModDoc->GetrSoundFile(), this);
	AddPage(m_pageNote);
	AddPage(m_pageVolume);
	AddPage(m_pageEffect);
	if (!CPropertySheet::Create(parent,
		WS_SYSMENU|WS_POPUP|WS_CAPTION,	WS_EX_DLGMODALFRAME)) return FALSE;
	ModifyStyleEx(0, WS_EX_TOOLWINDOW|WS_EX_PALETTEWINDOW, SWP_FRAMECHANGED);
	return TRUE;
}

void CEditCommand::OnDestroy()
//----------------------------
{
	CPropertySheet::OnDestroy();

	if (m_pageNote)
	{
		m_pageNote->DestroyWindow();
		delete m_pageNote;
	}
	if (m_pageVolume)
	{
		m_pageVolume->DestroyWindow();
		delete m_pageVolume;
	}
	if (m_pageEffect)
	{
		m_pageEffect->DestroyWindow();
		delete m_pageEffect;
	}
}

BOOL CEditCommand::PreTranslateMessage(MSG *pMsg)
//-----------------------------------------------
{
	if ((pMsg) && (pMsg->message == WM_KEYDOWN))
	{
		if ((pMsg->wParam == VK_ESCAPE) || (pMsg->wParam == VK_RETURN) || (pMsg->wParam == VK_APPS))
		{
			OnClose();
			return TRUE;
		}
	}
	return CPropertySheet::PreTranslateMessage(pMsg);
}


BOOL CEditCommand::ShowEditWindow(PATTERNINDEX nPat, const PatternCursor &cursor)
//-------------------------------------------------------------------------------
{
	CHAR s[64];
	CSoundFile &sndFile = m_pModDoc->GetrSoundFile();
	const ROWINDEX nRow = cursor.GetRow();
	const CHANNELINDEX nChannel = cursor.GetChannel();

	if ((nPat >= sndFile.Patterns.Size()) || (!m_pModDoc)
		|| (nRow >= sndFile.Patterns[nPat].GetNumRows()) || (nChannel >= sndFile.GetNumChannels())
		|| (!sndFile.Patterns[nPat])) return FALSE;
	m_Command = *sndFile.Patterns[nPat].GetpModCommand(nRow, nChannel);
	m_nRow = nRow;
	m_nChannel = nChannel;
	m_nPattern = nPat;
	m_bModified = false;
	// Init Pages
	if (m_pageNote) m_pageNote->Init(m_Command);
	if (m_pageVolume) m_pageVolume->Init(m_Command);

	// -> CODE#0010
	// -> DESC="add extended parameter mechanism to pattern effects"
	//	if (m_pageEffect) m_pageEffect->Init(m_Command);
	if (m_pageEffect)
	{
		UINT xp = 0, ml = 1;
		getXParam(m_Command.command, nPat, nRow, nChannel, sndFile, xp, ml);
		m_pageEffect->Init(m_Command);
		m_pageEffect->XInit(xp,ml);
	}
	// -! NEW_FEATURE#0010

	// Update Window Title
	wsprintf(s, "Note Properties - Row %d, Channel %d", m_nRow, m_nChannel + 1);
	SetTitle(s);
	// Activate Page
	UINT nPage = 2;
	if(cursor.GetColumnType() < PatternCursor::volumeColumn) nPage = 0;
	else if (cursor.GetColumnType() < PatternCursor::effectColumn) nPage = 1;
	SetActivePage(nPage);
	if (m_pageNote) m_pageNote->UpdateDialog();
	if (m_pageVolume) m_pageVolume->UpdateDialog();
	if (m_pageEffect) m_pageEffect->UpdateDialog();
	//ShowWindow(SW_SHOW);
	ShowWindow(SW_RESTORE);
	return TRUE;
}


void CEditCommand::UpdateNote(ModCommand::NOTE note, ModCommand::INSTR instr)
//---------------------------------------------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if ((m_nPattern >= pSndFile->Patterns.Size()) || (!m_pModDoc)
		|| (m_nRow >= pSndFile->Patterns[m_nPattern].GetNumRows())
		|| (m_nChannel >= pSndFile->GetNumChannels())
		|| (!pSndFile->Patterns[m_nPattern])) return;
	ModCommand *m = pSndFile->Patterns[m_nPattern].GetpModCommand(m_nRow, m_nChannel);
	if ((m->note != note) || (m->instr != instr))
	{
		if(!m_bModified)	// let's create just one undo step.
		{
			m_pModDoc->GetPatternUndo().PrepareUndo(m_nPattern, m_nChannel, m_nRow, 1, 1);
			m_bModified = true;
		}
		m->note = note;
		m->instr = instr;
		m_Command = *m;
		m_pModDoc->SetModified();
		// -> CODE#0008
		// -> DESC"#define to set pattern max size (number of rows) limit (now set to 1024 instead of 256)"
		//		m_pModDoc->UpdateAllViews(NULL, (m_nRow << 24) | HINT_PATTERNROW, NULL);
		m_pModDoc->UpdateAllViews(NULL, (m_nRow << HINT_SHIFT_ROW) | HINT_PATTERNROW, NULL);
		// -! BEHAVIOUR_CHANGE#0008
	}
}


void CEditCommand::UpdateVolume(ModCommand::VOLCMD volcmd, ModCommand::VOL vol)
//-----------------------------------------------------------------------------
{
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if ((m_nPattern >= pSndFile->Patterns.Size()) || (!m_pModDoc)
		|| (m_nRow >= pSndFile->Patterns[m_nPattern].GetNumRows())
		|| (m_nChannel >= pSndFile->GetNumChannels())
		|| (!pSndFile->Patterns[m_nPattern])) return;
	ModCommand *m = pSndFile->Patterns[m_nPattern].GetpModCommand(m_nRow, m_nChannel);
	if ((m->volcmd != volcmd) || (m->vol != vol))
	{
		if(!m_bModified)	// let's create just one undo step.
		{
			m_pModDoc->GetPatternUndo().PrepareUndo(m_nPattern, m_nChannel, m_nRow, 1, 1);
			m_bModified = true;
		}
		m->volcmd = volcmd;
		m->vol = vol;
		m_pModDoc->SetModified();
		// -> CODE#0008
		// -> DESC"#define to set pattern max size (number of rows) limit (now set to 1024 instead of 256)"
		//		m_pModDoc->UpdateAllViews(NULL, (m_nRow << 24) | HINT_PATTERNROW, NULL);
		m_pModDoc->UpdateAllViews(NULL, (m_nRow << HINT_SHIFT_ROW) | HINT_PATTERNROW, NULL);
		// -! BEHAVIOUR_CHANGE#0008
	}
}


void CEditCommand::UpdateEffect(ModCommand::COMMAND command, ModCommand::PARAM param)
//-----------------------------------------------------------------------------------
{
	CSoundFile &sndFile = m_pModDoc->GetrSoundFile();
	if ((m_nPattern >= sndFile.Patterns.Size()) || (!m_pModDoc)
		|| (m_nRow >= sndFile.Patterns[m_nPattern].GetNumRows())
		|| (m_nChannel >= sndFile.GetNumChannels())
		|| (!sndFile.Patterns[m_nPattern])) return;
	ModCommand *m = sndFile.Patterns[m_nPattern].GetpModCommand(m_nRow, m_nChannel);

	// -> CODE#0010
	// -> DESC="add extended parameter mechanism to pattern effects"
	if(command == CMD_OFFSET || command == CMD_PATTERNBREAK || command == CMD_TEMPO || command == CMD_XPARAM)
	{
		UINT xp = 0, ml = 1;
		getXParam(command, m_nPattern, m_nRow, m_nChannel, sndFile, xp, ml);
		m_pageEffect->XInit(xp,ml);
		m_pageEffect->UpdateDialog();
	}
	// -! NEW_FEATURE#0010

	if ((m->command != command) || (m->param != param))
	{
		if(!m_bModified)	// let's create just one undo step.
		{
			m_pModDoc->GetPatternUndo().PrepareUndo(m_nPattern, m_nChannel, m_nRow, 1, 1);
			m_bModified = true;
		}
		m->command = command;
		m->param = param;
		m_pModDoc->SetModified();
		// -> CODE#0008
		// -> DESC"#define to set pattern max size (number of rows) limit (now set to 1024 instead of 256)"
		//		m_pModDoc->UpdateAllViews(NULL, (m_nRow << 24) | HINT_PATTERNROW, NULL);
		m_pModDoc->UpdateAllViews(NULL, (m_nRow << HINT_SHIFT_ROW) | HINT_PATTERNROW, NULL);
		// -! BEHAVIOUR_CHANGE#0008
	}
}


void CEditCommand::OnActivate(UINT nState, CWnd *pWndOther, BOOL bMinimized)
//--------------------------------------------------------------------------
{
	CWnd::OnActivate(nState, pWndOther, bMinimized);
	if (nState == WA_INACTIVE) ShowWindow(SW_HIDE);
}


//////////////////////////////////////////////////////////////////////////////////////
// CPageEditCommand

BOOL CPageEditCommand::OnInitDialog()
//-----------------------------------
{
	CPropertyPage::OnInitDialog();
	m_bInitialized = true;
	UpdateDialog();
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// CPageEditNote

BEGIN_MESSAGE_MAP(CPageEditNote, CPageEditCommand)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnInstrChanged)
END_MESSAGE_MAP()


void CPageEditNote::UpdateDialog()
//--------------------------------
{
	char s[64];
	CComboBox *combo;

	if ((!m_bInitialized)) return;

	// Note
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL)
	{	
		combo->ResetContent();
		combo->SetItemData(combo->AddString("No note"), 0);
		AppendNotesToControlEx(*combo, &sndFile, m_nInstr);

		if (ModCommand::IsNoteOrEmpty(m_nNote))
		{
			// Normal note / no note
			const ModCommand::NOTE noteStart = sndFile.GetModSpecifications().noteMin;
			combo->SetCurSel(m_nNote - (noteStart - 1));
		}
		else
		{
			// Special notes
			for(int i = combo->GetCount() - 1; i >= 0; --i)
			{
				if(combo->GetItemData(i) == m_nNote)
				{
					combo->SetCurSel(i);
					break;
				}
			}
		}

	}
	// Instrument
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO2)) != NULL)
	{
		combo->ResetContent();

		if(ModCommand::IsPcNote(m_nNote))
		{
			// control plugin param note
			combo->SetItemData(combo->AddString("No Effect"), 0);
			AddPluginNamesToCombobox(*combo, sndFile.m_MixPlugins, false);
		} else
		{
			// instrument / sample
			combo->SetItemData(combo->AddString("No Instrument"), 0);
			const UINT nmax = sndFile.GetNumInstruments() ? sndFile.GetNumInstruments() : sndFile.GetNumSamples();
			for (UINT i = 1; i <= nmax; i++)
			{
				wsprintf(s, "%02d: ", i);
				int k = strlen(s);
				// instrument / sample
				if (sndFile.GetNumInstruments())
				{
					if (sndFile.Instruments[i])
						memcpy(s + k, sndFile.Instruments[i]->name, CountOf(sndFile.Instruments[i]->name));
				} else
					memcpy(s+k, sndFile.m_szNames[i], MAX_SAMPLENAME);
				s[k+32] = 0;
				combo->SetItemData(combo->AddString(s), i);
			}
		}
		combo->SetCurSel(m_nInstr);
	}
}


void CPageEditNote::OnNoteChanged()
//---------------------------------
{
	const bool bWasParamControl = ModCommand::IsPcNote(m_nNote);

	CComboBox *combo;
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL)
	{
		int n = combo->GetCurSel();
		if (n >= 0) m_nNote = static_cast<ModCommand::NOTE>(combo->GetItemData(n));
	}
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO2)) != NULL)
	{
		int n = combo->GetCurSel();
		if(n >= 0)
		{
			const ModCommand::INSTR oldInstr = m_nInstr;
			m_nInstr = static_cast<ModCommand::INSTR>(combo->GetItemData(n));
			//Checking whether note names should be recreated.
			if(!ModCommand::IsPcNote(m_nNote) && sndFile.Instruments[m_nInstr] && sndFile.Instruments[oldInstr])
			{
				if(sndFile.Instruments[m_nInstr]->pTuning != sndFile.Instruments[oldInstr]->pTuning)
					UpdateDialog();
			}
		}
	}
	const bool bIsNowParamControl = ModCommand::IsPcNote(m_nNote);
	if(bWasParamControl != bIsNowParamControl)
		UpdateDialog();

	if (m_pParent) m_pParent->UpdateNote(m_nNote, m_nInstr);
}


void CPageEditNote::OnInstrChanged()
//----------------------------------
{
	OnNoteChanged();
}


//////////////////////////////////////////////////////////////////////////////////////
// CPageEditVolume

BEGIN_MESSAGE_MAP(CPageEditVolume, CPageEditCommand)
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnVolCmdChanged)
END_MESSAGE_MAP()


void CPageEditVolume::UpdateDialog()
//----------------------------------
{
	CComboBox *combo;

	if ((!m_bInitialized)) return;
	UpdateRanges();
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL)
	{
		if (sndFile.GetType() == MOD_TYPE_MOD || m_bIsParamControl)
		{
			combo->EnableWindow(FALSE);
			return;
		}
		combo->EnableWindow(TRUE);
		combo->ResetContent();
		UINT count = effectInfo.GetNumVolCmds();
		combo->SetItemData(combo->AddString(" None"), (DWORD)-1);
		combo->SetCurSel(0);
		UINT fxndx = effectInfo.GetIndexFromVolCmd(m_nVolCmd);
		for (UINT i=0; i<count; i++)
		{
			CHAR s[64];
			if (effectInfo.GetVolCmdInfo(i, s))
			{
				int k = combo->AddString(s);
				combo->SetItemData(k, i);
				if (i == fxndx) combo->SetCurSel(k);
			}
		}
	}
}


void CPageEditVolume::UpdateRanges()
//----------------------------------
{
	CSliderCtrl *slider = (CSliderCtrl *)GetDlgItem(IDC_SLIDER1);
	if (slider != nullptr)
	{
		DWORD rangeMin = 0, rangeMax = 0;
		LONG fxndx = effectInfo.GetIndexFromVolCmd(m_nVolCmd);
		BOOL bOk = effectInfo.GetVolCmdInfo(fxndx, NULL, &rangeMin, &rangeMax);
		if ((bOk) && (rangeMax > rangeMin))
		{
			slider->EnableWindow(TRUE);
			slider->SetRange(rangeMin, rangeMax);
			UINT pos = m_nVolume;
			if (pos < rangeMin) pos = rangeMin;
			if (pos > rangeMax) pos = rangeMax;
			slider->SetPos(pos);
		} else
		{
			slider->EnableWindow(FALSE);
		}
	}
}


void CPageEditVolume::OnVolCmdChanged()
//-------------------------------------
{
	CComboBox *combo;
	CSliderCtrl *slider;
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL)
	{
		int n = combo->GetCurSel();
		if (n >= 0)
		{
			ModCommand::VOLCMD volcmd = effectInfo.GetVolCmdFromIndex(combo->GetItemData(n));
			if (volcmd != m_nVolCmd)
			{
				m_nVolCmd = volcmd;
				UpdateRanges();
			}
		}
	}
	if ((slider = (CSliderCtrl *)GetDlgItem(IDC_SLIDER1)) != NULL)
	{
		m_nVolume = static_cast<ModCommand::VOL>(slider->GetPos());
	}
	if (m_pParent) m_pParent->UpdateVolume(m_nVolCmd, m_nVolume);
}


void CPageEditVolume::OnHScroll(UINT, UINT, CScrollBar *)
//-------------------------------------------------------
{
	OnVolCmdChanged();
}


//////////////////////////////////////////////////////////////////////////////////////
// CPageEditEffect

BEGIN_MESSAGE_MAP(CPageEditEffect, CPageEditCommand)
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnCommandChanged)
END_MESSAGE_MAP()


void CPageEditEffect::UpdateDialog()
//----------------------------------
{
	CComboBox *combo;

	if (!m_bInitialized) return;

	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL)
	{
		combo->ResetContent();
		if(m_bIsParamControl)
		{
			// plugin param control note
			if(m_nPlugin > 0 && m_nPlugin <= MAX_MIXPLUGINS)
			{
				combo->ModifyStyle(CBS_SORT, 0);	// Y U NO WORK?
				AddPluginParameternamesToCombobox(*combo, sndFile.m_MixPlugins[m_nPlugin - 1]);
				combo->SetCurSel(m_nPluginParam);
			}
		} else
		{
			// process as effect
			UINT numfx = effectInfo.GetNumEffects();
			UINT fxndx = effectInfo.GetIndexFromEffect(m_nCommand, m_nParam);
			combo->ModifyStyle(0, CBS_SORT);
			combo->SetItemData(combo->AddString(" None"), (DWORD)-1);
			if (m_nCommand == CMD_NONE) combo->SetCurSel(0);

			CHAR s[128];
			for (UINT i=0; i<numfx; i++)
			{
				if (effectInfo.GetEffectInfo(i, s, true))
				{
					int k = combo->AddString(s);
					combo->SetItemData(k, i);
					if (i == fxndx) combo->SetCurSel(k);
				}
			}
			combo->ModifyStyle(CBS_SORT, 0);
		}
	}
	UpdateRange(FALSE);
}


void CPageEditEffect::UpdateRange(BOOL bSet)
//------------------------------------------
{
	CSliderCtrl *slider = (CSliderCtrl *)GetDlgItem(IDC_SLIDER1);
	if (slider)
	{
		DWORD rangeMin = 0, rangeMax = 0, pos;
		bool enable = true;

		if(m_bIsParamControl)
		{
			// plugin param control note
			rangeMax = ModCommand::maxColumnValue;
			pos = ModCommand::GetValueEffectCol(m_nCommand, m_nParam);
		} else
		{
			// process as effect
			LONG fxndx = effectInfo.GetIndexFromEffect(m_nCommand, m_nParam);
			enable = ((fxndx >= 0) && (effectInfo.GetEffectInfo(fxndx, NULL, false, &rangeMin, &rangeMax)));

			pos = effectInfo.MapValueToPos(fxndx, m_nParam);
			if (pos > rangeMax) pos = rangeMin | (pos & 0x0F);
			if (pos < rangeMin) pos = rangeMin;
			if (pos > rangeMax) pos = rangeMax;
		}

		if (enable)
		{
			slider->EnableWindow(TRUE);
			slider->SetPageSize(1);
			slider->SetRange(rangeMin, rangeMax);
			slider->SetPos(pos);
		} else
		{
			slider->SetRange(0, 0);
			slider->EnableWindow(FALSE);
		}
		UpdateValue(bSet);
	}
}


void CPageEditEffect::UpdateValue(BOOL bSet)
//------------------------------------------
{
	CHAR s[128] = "";

	if(m_bIsParamControl)
	{
		// plugin param control note
		wsprintf(s, "Value: %u", ModCommand::GetValueEffectCol(m_nCommand, m_nParam));
	} else
	{
		// process as effect
		LONG fxndx = effectInfo.GetIndexFromEffect(m_nCommand, m_nParam);
		if (fxndx >= 0) effectInfo.GetEffectNameEx(s, fxndx, m_nParam * m_nMultiplier + m_nXParam);
	}
	SetDlgItemText(IDC_TEXT1, s);

	if ((m_pParent) && (bSet)) m_pParent->UpdateEffect(m_nCommand, m_nParam);
}


void CPageEditEffect::OnCommandChanged()
//--------------------------------------
{
	CComboBox *combo;

	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL)
	{
		int n = combo->GetCurSel();

		if(m_bIsParamControl)
		{
			// plugin param control note
			if(n >= 0)
			{
				// TODO update in pattern
				m_nPluginParam = n;
			}
		} else
		{
			// process as effect
			BOOL bSet = FALSE;
			if (n >= 0)
			{
				int param = -1, ndx = combo->GetItemData(n);
				m_nCommand = (ndx >= 0) ? effectInfo.GetEffectFromIndex(ndx, param) : 0;
				if (param >= 0) m_nParam = static_cast<ModCommand::PARAM>(param);
				bSet = TRUE;
			}
			UpdateRange(bSet);
		}
	}
}



void CPageEditEffect::OnHScroll(UINT, UINT, CScrollBar *)
//-------------------------------------------------------
{
	CSliderCtrl *slider = (CSliderCtrl *)GetDlgItem(IDC_SLIDER1);
	if (slider != nullptr)
	{
		if(m_bIsParamControl)
		{
			// plugin param control note
			// HACK
			ModCommand m;
			m.SetValueEffectCol(static_cast<int16>(slider->GetPos()));
			m_nCommand = m.command;
			m_nParam = m.param;
			UpdateValue(TRUE);
		} else
		{
			// process as effect
			LONG fxndx = effectInfo.GetIndexFromEffect(m_nCommand, m_nParam);
			if (fxndx >= 0)
			{
				int pos = slider->GetPos();
				UINT param = effectInfo.MapPosToValue(fxndx, pos);
				if (param != m_nParam)
				{
					m_nParam = static_cast<ModCommand::PARAM>(param);
					UpdateValue(TRUE);
				}
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////
// Chord Editor

BEGIN_MESSAGE_MAP(CChordEditor, CDialog)
	ON_MESSAGE(WM_MOD_KBDNOTIFY,	OnKeyboardNotify)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnChordChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnBaseNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,	OnNote1Changed)
	ON_CBN_SELCHANGE(IDC_COMBO4,	OnNote2Changed)
	ON_CBN_SELCHANGE(IDC_COMBO5,	OnNote3Changed)
END_MESSAGE_MAP()


void CChordEditor::DoDataExchange(CDataExchange* pDX)
//---------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChordEditor)
	DDX_Control(pDX, IDC_KEYBOARD1,		m_Keyboard);
	DDX_Control(pDX, IDC_COMBO1,		m_CbnShortcut);
	DDX_Control(pDX, IDC_COMBO2,		m_CbnBaseNote);
	DDX_Control(pDX, IDC_COMBO3,		m_CbnNote1);
	DDX_Control(pDX, IDC_COMBO4,		m_CbnNote2);
	DDX_Control(pDX, IDC_COMBO5,		m_CbnNote3);
	//}}AFX_DATA_MAP
}


BOOL CChordEditor::OnInitDialog()
//-------------------------------
{
	CMainFrame *pMainFrm;
	CHAR s[128];

	CDialog::OnInitDialog();
	m_Keyboard.Init(m_hWnd, 2);
	pMainFrm = CMainFrame::GetMainFrame();
	if (!pMainFrm) return TRUE;
	// Fills the shortcut key combo box
	AppendNotesToControl(m_CbnShortcut, 0, 3 * 12 - 1);

	m_CbnShortcut.SetCurSel(0);
	// Base Note combo box
	m_CbnBaseNote.SetItemData(m_CbnBaseNote.AddString("Relative"), MPTChord::relativeMode);
	AppendNotesToControl(m_CbnBaseNote, 0, 3 * 12 - 1);

	// Minor notes
	for (int inotes=-1; inotes<24; inotes++)
	{
		if (inotes < 0) strcpy(s, "--"); else
			if (inotes < 12) wsprintf(s, "%s", szNoteNames[inotes % 12]);
			else wsprintf(s, "%s (+%d)", szNoteNames[inotes % 12], inotes / 12);
			m_CbnNote1.AddString(s);
			m_CbnNote2.AddString(s);
			m_CbnNote3.AddString(s);
	}
	// Update Dialog
	OnChordChanged();
	return TRUE;
}


MPTChord &CChordEditor::GetChord()
//--------------------------------
{
	MPTChords &chords = TrackerSettings::GetChords();
	int chord = m_CbnShortcut.GetCurSel();
	if(chord >= 0) chord = m_CbnShortcut.GetItemData(chord);
	if(chord < 0 || chord >= CountOf(chords)) chord = 0;
	return chords[chord];
}


LRESULT CChordEditor::OnKeyboardNotify(WPARAM wParam, LPARAM nKey)
//----------------------------------------------------------------
{
	if (wParam != KBDNOTIFY_LBUTTONDOWN) return 0;
	MPTChord &chord = GetChord();
	UINT cnote = NOTE_NONE;
	chord.notes[0] = NOTE_NONE;
	chord.notes[1] = NOTE_NONE;
	chord.notes[2] = NOTE_NONE;
	for(UINT i = 0; i < 2 * 12; i++)
	{
		if(chord.key == MPTChord::relativeMode)
		{
			if(!i) continue;
		} else
		{
			if(i == chord.key % 12u) continue;
		}

		UINT n = m_Keyboard.GetFlags(i);
		if (i == (UINT)nKey) n = (n) ? 0 : 1;
		if (n)
		{
			if ((cnote < 3) || (i == (UINT)nKey))
			{
				UINT k = (cnote < 3) ? cnote : 2;
				chord.notes[k] = static_cast<BYTE>(i+1);
				if (cnote < 3) cnote++;
			}
		}
	}
	OnChordChanged();
	return 0;
}


void CChordEditor::OnChordChanged()
//---------------------------------
{
	MPTChord &chord = GetChord();
	if(chord.key != MPTChord::relativeMode)
		m_CbnBaseNote.SetCurSel(chord.key + 1);
	else
		m_CbnBaseNote.SetCurSel(0);
	m_CbnNote1.SetCurSel(chord.notes[0]);
	m_CbnNote2.SetCurSel(chord.notes[1]);
	m_CbnNote3.SetCurSel(chord.notes[2]);
	UpdateKeyboard();
}


void CChordEditor::UpdateKeyboard()
//---------------------------------
{
	MPTChord &chord = GetChord();
	UINT note = chord.key % 12;
	if(chord.key == MPTChord::relativeMode)
	{
		note = 0;
	}
	for(UINT i = 0; i < 2 * 12; i++)
	{
		UINT b = 0;
		if(i == note) b = 1;
		else if(chord.notes[0] && i + 1 == chord.notes[0]) b = 1;
		else if(chord.notes[1] && i + 1 == chord.notes[1]) b = 1;
		else if(chord.notes[2] && i + 1 == chord.notes[2]) b = 1;
		m_Keyboard.SetFlags(i, b);
	}
	m_Keyboard.InvalidateRect(NULL, FALSE);
}


void CChordEditor::OnBaseNoteChanged()
//------------------------------------
{
	MPTChord &chord = GetChord();
	int basenote = m_CbnBaseNote.GetItemData(m_CbnBaseNote.GetCurSel());
	chord.key = (uint8)basenote;
	UpdateKeyboard();
}


void CChordEditor::OnNote1Changed()
//---------------------------------
{
	MPTChord &chord = GetChord();
	int note = m_CbnNote1.GetCurSel();
	if(note >= 0)
	{
		chord.notes[0] = (uint8)note;
		UpdateKeyboard();
	}
}


void CChordEditor::OnNote2Changed()
//---------------------------------
{
	MPTChord &chord = GetChord();
	int note = m_CbnNote2.GetCurSel();
	if(note >= 0)
	{
		chord.notes[1] = (uint8)note;
		UpdateKeyboard();
	}
}


void CChordEditor::OnNote3Changed()
//---------------------------------
{
	MPTChord &chord = GetChord();
	int note = m_CbnNote3.GetCurSel();
	if(note >= 0)
	{
		chord.notes[2] = (uint8)note;
		UpdateKeyboard();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////
// Keyboard Split Settings (pattern editor)

BEGIN_MESSAGE_MAP(CSplitKeyboadSettings, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_OCTAVEMODIFIER,	OnOctaveModifierChanged)
END_MESSAGE_MAP()


void CSplitKeyboadSettings::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSplitKeyboadSettings)
	DDX_Control(pDX, IDC_COMBO_SPLITINSTRUMENT,	m_CbnSplitInstrument);
	DDX_Control(pDX, IDC_COMBO_SPLITNOTE,		m_CbnSplitNote);
	DDX_Control(pDX, IDC_COMBO_OCTAVEMODIFIER,	m_CbnOctaveModifier);
	DDX_Control(pDX, IDC_COMBO_SPLITVOLUME,		m_CbnSplitVolume);
	//}}AFX_DATA_MAP
}


BOOL CSplitKeyboadSettings::OnInitDialog()
//----------------------------------------
{
	if(sndFile.GetpModDoc() == nullptr) return FALSE;

	CDialog::OnInitDialog();

	CHAR s[64];

	// Split Notes
	AppendNotesToControl(m_CbnSplitNote, sndFile.GetModSpecifications().noteMin - NOTE_MIN, sndFile.GetModSpecifications().noteMax - NOTE_MIN);
	m_CbnSplitNote.SetCurSel(m_Settings.splitNote - (sndFile.GetModSpecifications().noteMin - NOTE_MIN));

	// Octave modifier
	for(int i = -SplitKeyboardSettings::splitOctaveRange; i < SplitKeyboardSettings::splitOctaveRange + 1; i++)
	{
		wsprintf(s, i < 0 ? "Octave -%d" : i > 0 ? "Octave +%d" : "No Change", abs(i));
		int n = m_CbnOctaveModifier.AddString(s);
		m_CbnOctaveModifier.SetItemData(n, i);
	}

	m_CbnOctaveModifier.SetCurSel(m_Settings.octaveModifier + SplitKeyboardSettings::splitOctaveRange);
	CheckDlgButton(IDC_PATTERN_OCTAVELINK, (m_Settings.octaveLink && m_Settings.octaveModifier != 0) ? MF_CHECKED : MF_UNCHECKED);

	// Volume
	m_CbnSplitVolume.AddString("No Change");
	m_CbnSplitVolume.SetItemData(0, 0);
	for(int i = 1; i <= 64 ; i++)
	{
		wsprintf(s, "%d", i);
		int n = m_CbnSplitVolume.AddString(s);
		m_CbnSplitVolume.SetItemData(n, i);
	}
	m_CbnSplitVolume.SetCurSel(m_Settings.splitVolume);

	// Instruments
	m_CbnSplitInstrument.ResetContent();
	m_CbnSplitInstrument.SetItemData(m_CbnSplitInstrument.AddString("No Change"), 0);

	if(sndFile.GetNumInstruments())
	{
		for (INSTRUMENTINDEX nIns = 1; nIns <= sndFile.GetNumInstruments(); nIns++)
		{
			if(sndFile.Instruments[nIns] == nullptr)
				continue;

			CString displayName = sndFile.GetpModDoc()->GetPatternViewInstrumentName(nIns);
			int n = m_CbnSplitInstrument.AddString(displayName);
			m_CbnSplitInstrument.SetItemData(n, nIns);
		}
	} else
	{
		for(SAMPLEINDEX nSmp = 1; nSmp <= sndFile.GetNumSamples(); nSmp++)
		{
			if(sndFile.GetSample(nSmp).pSample)
			{
				wsprintf(s, "%02d: %s", nSmp, sndFile.m_szNames[nSmp]);
				int n = m_CbnSplitInstrument.AddString(s);
				m_CbnSplitInstrument.SetItemData(n, nSmp);
			}
		}
	}
	m_CbnSplitInstrument.SetCurSel(m_Settings.splitInstrument);

	return TRUE;
}


void CSplitKeyboadSettings::OnOK()
//--------------------------------
{
	CDialog::OnOK();

	m_Settings.splitNote = static_cast<ModCommand::NOTE>(m_CbnSplitNote.GetCurSel() + (sndFile.GetModSpecifications().noteMin - NOTE_MIN));
	m_Settings.octaveModifier = m_CbnOctaveModifier.GetCurSel() - SplitKeyboardSettings::splitOctaveRange;
	m_Settings.octaveLink = (IsDlgButtonChecked(IDC_PATTERN_OCTAVELINK) == TRUE);
	m_Settings.splitVolume = static_cast<ModCommand::VOL>(m_CbnSplitVolume.GetCurSel());
	m_Settings.splitInstrument = static_cast<ModCommand::INSTR>(m_CbnSplitInstrument.GetItemData(m_CbnSplitInstrument.GetCurSel()));
}


void CSplitKeyboadSettings::OnCancel()
//------------------------------------
{
	CDialog::OnCancel();
}


void CSplitKeyboadSettings::OnOctaveModifierChanged()
//---------------------------------------------------
{
	CheckDlgButton(IDC_PATTERN_OCTAVELINK, (m_CbnOctaveModifier.GetCurSel() != 9) ? MF_CHECKED : MF_UNCHECKED);
}


/////////////////////////////////////////////////////////////////////////
// Show channel properties from pattern editor

BEGIN_MESSAGE_MAP(QuickChannelProperties, CDialog)
	ON_WM_HSCROLL()		// Sliders
	ON_WM_ACTIVATE()	// Catch Window focus change
	ON_EN_UPDATE(IDC_EDIT1,	OnVolChanged)
	ON_EN_UPDATE(IDC_EDIT2,	OnPanChanged)
	ON_EN_UPDATE(IDC_EDIT3,	OnNameChanged)
	ON_COMMAND(IDC_CHECK1,	OnMuteChanged)
	ON_COMMAND(IDC_CHECK2,	OnSurroundChanged)
	ON_COMMAND(IDC_BUTTON1,	OnPrevChannel)
	ON_COMMAND(IDC_BUTTON2,	OnNextChannel)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,	OnCustomKeyMsg)
END_MESSAGE_MAP()


void QuickChannelProperties::DoDataExchange(CDataExchange* pDX)
//-------------------------------------------------------------
{
	DDX_Control(pDX, IDC_SLIDER1,	volSlider);
	DDX_Control(pDX, IDC_SLIDER2,	panSlider);
	DDX_Control(pDX, IDC_SPIN1,		volSpin);
	DDX_Control(pDX, IDC_SPIN2,		panSpin);
	DDX_Control(pDX, IDC_EDIT3,		nameEdit);
}


QuickChannelProperties::QuickChannelProperties()
//----------------------------------------------
{
	visible = false;
	Create(IDD_CHANNELSETTINGS, nullptr);

	volSlider.SetRange(0, 64);
	volSlider.SetTicFreq(8);
	volSpin.SetRange(0, 64);

	panSlider.SetRange(0, 64);
	panSlider.SetTicFreq(8);
	panSpin.SetRange(0, 256);

	nameEdit.SetFocus();
};


QuickChannelProperties::~QuickChannelProperties()
//-----------------------------------------------
{
	CDialog::OnCancel();
}


void QuickChannelProperties::OnActivate(UINT nState, CWnd *, BOOL)
//----------------------------------------------------------------
{
	if(nState == WA_INACTIVE)
	{
		// Hide window when changing focus to another window.
		visible = false;
		ShowWindow(SW_HIDE);
	}
}


// Show channel properties for a given channel at a given screen position.
void QuickChannelProperties::Show(CModDoc *modDoc, CHANNELINDEX chn, PATTERNINDEX ptn, CPoint position)
//-----------------------------------------------------------------------------------------------------
{
	document = modDoc;
	channel = chn;
	pattern = ptn;
	
	SetParent(nullptr);

	// Center window around point where user clicked.
	CRect rect, screenRect;
	GetWindowRect(rect);
	::GetWindowRect(::GetDesktopWindow(), &screenRect);
	rect.MoveToXY(
		Clamp(static_cast<int>(position.x) - rect.Width() / 2, 0, static_cast<int>(screenRect.right) - rect.Width()),
		Clamp(static_cast<int>(position.y) - rect.Height() / 2, 0, static_cast<int>(screenRect.bottom) - rect.Height()));
	MoveWindow(rect);

	UpdateDisplay();

	const BOOL enablePan = (document->GetModType() & (MOD_TYPE_XM | MOD_TYPE_MOD)) ? FALSE : TRUE;
	const BOOL itOnly = (document->GetModType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) ? TRUE : FALSE;

	// Volume controls
	volSlider.EnableWindow(itOnly);
	volSpin.EnableWindow(itOnly);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT1), itOnly);

	// Pan controls
	panSlider.EnableWindow(enablePan);
	panSpin.EnableWindow(enablePan);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT2), enablePan);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK2), itOnly);

	// Channel name
	nameEdit.EnableWindow((document->GetModType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM)) ? TRUE : FALSE);

	ShowWindow(SW_SHOW);
	visible = true;
}


void QuickChannelProperties::UpdateDisplay()
//------------------------------------------
{
	// Set up channel properties
	visible = false;
	const ModChannelSettings &settings = document->GetSoundFile()->ChnSettings[channel];
	SetDlgItemInt(IDC_EDIT1, settings.nVolume, FALSE);
	SetDlgItemInt(IDC_EDIT2, settings.nPan, FALSE);
	volSlider.SetPos(settings.nVolume);
	panSlider.SetPos(settings.nPan / 4u);
	CheckDlgButton(IDC_CHECK1, (settings.dwFlags[CHN_MUTE]) ? TRUE : FALSE);
	CheckDlgButton(IDC_CHECK2, (settings.dwFlags[CHN_SURROUND]) ? TRUE : FALSE);

	char description[16];
	sprintf(description, "Channel %d:", channel + 1);
	SetDlgItemText(IDC_STATIC_CHANNEL_NAME, description);
	nameEdit.LimitText(MAX_CHANNELNAME - 1);
	nameEdit.SetWindowText(settings.szName);

	settingsChanged = false;
	visible = true;

	::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1), channel > 0 ? TRUE : FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON2), channel < document->GetNumChannels() - 1 ? TRUE : FALSE);
}

void QuickChannelProperties::PrepareUndo()
//----------------------------------------
{
	if(!settingsChanged)
	{
		// Backup old channel settings through pattern undo.
		settingsChanged = true;
		document->GetPatternUndo().PrepareUndo(pattern, 0, 0, 1, 1, false, true);
	}
}


void QuickChannelProperties::OnVolChanged()
//-----------------------------------------
{
	if(!visible)
	{
		return;
	}

	uint16 volume = static_cast<uint16>(GetDlgItemInt(IDC_EDIT1));
	if(volume >= 0 && volume <= 64)
	{
		PrepareUndo();
		document->SetChannelGlobalVolume(channel, volume);
		volSlider.SetPos(volume);
		document->UpdateAllViews(nullptr, HINT_MODCHANNELS);
	}
}


void QuickChannelProperties::OnPanChanged()
//-----------------------------------------
{
	if(!visible)
	{
		return;
	}

	uint16 panning = static_cast<uint16>(GetDlgItemInt(IDC_EDIT2));
	if(panning >= 0 && panning <= 256)
	{
		PrepareUndo();
		document->SetChannelDefaultPan(channel, panning);
		panSlider.SetPos(panning / 4u);
		document->UpdateAllViews(nullptr, HINT_MODCHANNELS);
	}
}


void QuickChannelProperties::OnHScroll(UINT, UINT, CScrollBar *)
//--------------------------------------------------------------
{
	if(!visible)
	{
		return;
	}

	bool update = false;

	// Volume slider
	uint16 pos = static_cast<uint16>(volSlider.GetPos());
	if(pos >= 0 && pos <= 64)
	{
		PrepareUndo();
		if(document->SetChannelGlobalVolume(channel, pos))
		{
			SetDlgItemInt(IDC_EDIT1, pos);
			update = true;
		}
	}
	// Pan slider
	pos = static_cast<uint16>(panSlider.GetPos());
	if(pos >= 0 && pos <= 64)
	{
		PrepareUndo();
		if(document->SetChannelDefaultPan(channel, pos * 4u))
		{
			SetDlgItemInt(IDC_EDIT2, pos * 4u);
			CheckDlgButton(IDC_CHECK2, BST_UNCHECKED);
			update = true;
		}
	}

	if(update)
	{
		document->UpdateAllViews(nullptr, HINT_MODCHANNELS);
	}
}


void QuickChannelProperties::OnMuteChanged()
//------------------------------------------
{
	if(!visible)
	{
		return;
	}

	document->MuteChannel(channel, IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED);
	document->UpdateAllViews(nullptr, HINT_MODCHANNELS);
}


void QuickChannelProperties::OnSurroundChanged()
//----------------------------------------------
{
	if(!visible)
	{
		return;
	}

	PrepareUndo();
	document->SurroundChannel(channel, IsDlgButtonChecked(IDC_CHECK2) != BST_UNCHECKED);
	document->UpdateAllViews(nullptr, HINT_MODCHANNELS);
	UpdateDisplay();
}


void QuickChannelProperties::OnNameChanged()
//------------------------------------------
{
	if(!visible)
	{
		return;
	}
	
	ModChannelSettings &settings = document->GetrSoundFile().ChnSettings[channel];
	char newName[MAX_CHANNELNAME];
	nameEdit.GetWindowText(newName, MAX_CHANNELNAME);

	if(strcmp(newName, settings.szName))
	{
		PrepareUndo();
		strcpy(settings.szName, newName);
		document->SetModified();
		document->UpdateAllViews(nullptr, HINT_MODCHANNELS);
	}
}


void QuickChannelProperties::OnPrevChannel()
//------------------------------------------
{
	if(channel > 0)
	{
		channel--;
		UpdateDisplay();
	}
}


void QuickChannelProperties::OnNextChannel()
//------------------------------------------
{
	if(channel < document->GetNumChannels() - 1)
	{
		channel++;
		UpdateDisplay();
	}
}


BOOL QuickChannelProperties::PreTranslateMessage(MSG *pMsg)
//---------------------------------------------------------
{
	if(pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if((pMsg->message == WM_SYSKEYUP) || (pMsg->message == WM_KEYUP) ||
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();

			//Translate message manually
			UINT nChar = pMsg->wParam;
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);

			if(ih->KeyEvent(kCtxChannelSettings, nChar, nRepCnt, nFlags, kT, this) != kcNull)
			{
				return true;	// Mapped to a command, no need to pass message on.
			}
		}

	}

	return CDialog::PreTranslateMessage(pMsg);
}


LRESULT QuickChannelProperties::OnCustomKeyMsg(WPARAM wParam, LPARAM)
//-------------------------------------------------------------------
{
	if (wParam == kcNull)
		return 0;

	switch(wParam)
	{
	case kcChnSettingsPrev:
		OnPrevChannel();
		return wParam;
	case kcChnSettingsNext:
		OnNextChannel();
		return wParam;
	case kcChnSettingsClose:
		OnActivate(WA_INACTIVE, nullptr, FALSE);
		return wParam;
	}

	return 0;
}