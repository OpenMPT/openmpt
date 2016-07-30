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
#include "Mptrack.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "Moddoc.h"
#include "View_pat.h"
#include "PatternEditorDialogs.h"
#include "TempoSwingDialog.h"
#include "../soundlib/mod_specifications.h"


OPENMPT_NAMESPACE_BEGIN


// For a given pattern cell, check if it contains a command supported by the X-Param mechanism.
// If so, calculate the multipler for this cell and the value of all the other cells belonging to this param.
void getXParam(ModCommand::COMMAND command, PATTERNINDEX nPat, ROWINDEX nRow, CHANNELINDEX nChannel, const CSoundFile &sndFile, UINT &xparam, UINT &multiplier)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
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
			if(m->command == CMD_OFFSET || m->command == CMD_PATTERNBREAK || m->command == CMD_POSITIONJUMP || m->command == CMD_TEMPO)
				break;
			if(m->command != CMD_XPARAM)
			{
				nCmdRow = -1;
				break;
			}
			nCmdRow--;
		}
	} else if(command != CMD_OFFSET && command != CMD_PATTERNBREAK && command != CMD_POSITIONJUMP && command != CMD_TEMPO)
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


/////////////////////////////////////////////////////////////////////////////////////////////
// CPatternPropertiesDlg

BEGIN_MESSAGE_MAP(CPatternPropertiesDlg, CDialog)
	ON_COMMAND(IDC_BUTTON_HALF,		OnHalfRowNumber)
	ON_COMMAND(IDC_BUTTON_DOUBLE,	OnDoubleRowNumber)
	ON_COMMAND(IDC_CHECK1,			OnOverrideSignature)
	ON_COMMAND(IDC_BUTTON1,			OnTempoSwing)
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
		TCHAR s[256];
		const CPattern &pattern = sndFile.Patterns[m_nPattern];
		ROWINDEX nrows = pattern.GetNumRows();

		const CModSpecifications& specs = sndFile.GetModSpecifications();
		combo->SetRedraw(FALSE);
		for (UINT irow = specs.patternRowsMin; irow <= specs.patternRowsMax; irow++)
		{
			_stprintf(s, _T("%d"), irow);
			combo->AddString(s);
		}
		combo->SetCurSel(nrows - specs.patternRowsMin);
		combo->SetRedraw(TRUE);
		_stprintf(s, _T("Pattern #%d: %d row%s (%dK)"),
			m_nPattern,
			pattern.GetNumRows(),
			(pattern.GetNumRows() == 1) ? _T("") : _T("s"),
			static_cast<int>((pattern.GetNumRows() * sndFile.GetNumChannels() * sizeof(ModCommand)) / 1024));
		SetDlgItemText(IDC_TEXT1, s);

		// Window title
		const CString patternName = pattern.GetName().c_str();
		_stprintf(s, _T("Pattern Properties for Pattern #%d"), m_nPattern);
		if(!patternName.IsEmpty())
		{
			_tcscat(s, _T(" ("));
			_tcscat(s, patternName);
			_tcscat(s, _T(")"));
		}
		SetWindowText(s);

		// pattern time signature
		const bool bOverride = pattern.GetOverrideSignature();
		ROWINDEX nRPB = pattern.GetRowsPerBeat(), nRPM = pattern.GetRowsPerMeasure();
		if(nRPB == 0 || !bOverride) nRPB = sndFile.m_nDefaultRowsPerBeat;
		if(nRPM == 0 || !bOverride) nRPM = sndFile.m_nDefaultRowsPerMeasure;

		m_tempoSwing = pattern.HasTempoSwing() ? pattern.GetTempoSwing() : sndFile.m_tempoSwing;

		GetDlgItem(IDC_CHECK1)->EnableWindow(sndFile.GetModSpecifications().hasPatternSignatures ? TRUE : FALSE);
		CheckDlgButton(IDC_CHECK1, bOverride ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_ROWSPERBEAT, nRPB, FALSE);
		SetDlgItemInt(IDC_ROWSPERMEASURE, nRPM, FALSE);
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
	GetDlgItem(IDC_ROWSPERBEAT)->EnableWindow(IsDlgButtonChecked(IDC_CHECK1));
	GetDlgItem(IDC_ROWSPERMEASURE)->EnableWindow(IsDlgButtonChecked(IDC_CHECK1));
	GetDlgItem(IDC_BUTTON1)->EnableWindow(IsDlgButtonChecked(IDC_CHECK1) && modDoc.GetrSoundFile().m_nTempoMode == tempoModeModern);
}


void CPatternPropertiesDlg::OnTempoSwing()
//----------------------------------------
{
	CPattern &pat = modDoc.GetrSoundFile().Patterns[m_nPattern];
	const ROWINDEX oldRPB = pat.GetRowsPerBeat();
	const ROWINDEX oldRPM = pat.GetRowsPerMeasure();

	// Temporarily apply new tempo signature for preview
	ROWINDEX newRPB = std::max(1u, GetDlgItemInt(IDC_ROWSPERBEAT));
	ROWINDEX newRPM = std::max(newRPB, GetDlgItemInt(IDC_ROWSPERMEASURE));
	pat.SetSignature(newRPB, newRPM);

	m_tempoSwing.resize(newRPB, TempoSwing::Unity);
	CTempoSwingDlg dlg(this, m_tempoSwing, modDoc.GetrSoundFile(), m_nPattern);
	if(dlg.DoModal() == IDOK)
	{
		m_tempoSwing = dlg.m_tempoSwing;
	}
	pat.SetSignature(oldRPB, oldRPM);
}


void CPatternPropertiesDlg::OnOK()
//--------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();
	CPattern &pattern = sndFile.Patterns[m_nPattern];
	// Update pattern signature if necessary
	if(sndFile.GetModSpecifications().hasPatternSignatures)
	{
		if(IsDlgButtonChecked(IDC_CHECK1))	// Enable signature
		{
			ROWINDEX nNewBeat = (ROWINDEX)GetDlgItemInt(IDC_ROWSPERBEAT, NULL, FALSE), nNewMeasure = (ROWINDEX)GetDlgItemInt(IDC_ROWSPERMEASURE, NULL, FALSE);
			if(nNewBeat != pattern.GetRowsPerBeat() || nNewMeasure != pattern.GetRowsPerMeasure() || m_tempoSwing != pattern.GetTempoSwing())
			{
				if(!pattern.SetSignature(nNewBeat, nNewMeasure))
				{
					Reporting::Error("Invalid time signature!", "Pattern Properties");
					GetDlgItem(IDC_ROWSPERBEAT)->SetFocus();
					return;
				}
				m_tempoSwing.resize(nNewBeat, TempoSwing::Unity);
				pattern.SetTempoSwing(m_tempoSwing);
				modDoc.SetModified();
			}
		} else	// Disable signature
		{
			if(pattern.GetOverrideSignature() || pattern.HasTempoSwing())
			{
				pattern.RemoveSignature();
				pattern.RemoveTempoSwing();
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
		modDoc.BeginWaitCursor();
		modDoc.GetPatternUndo().PrepareUndo(m_nPattern, 0, 0, sndFile.Patterns[m_nPattern].GetNumChannels(), sndFile.Patterns[m_nPattern].GetNumRows(), "Resize");
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

BEGIN_MESSAGE_MAP(CEditCommand, CDialog)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()

	ON_CBN_SELCHANGE(IDC_COMBO1,	OnNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,	OnVolCmdChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,	OnCommandChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5,	OnPlugParamChanged)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


void CEditCommand::DoDataExchange(CDataExchange* pDX)
//---------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSplitKeyboadSettings)
	DDX_Control(pDX, IDC_COMBO1,	cbnNote);
	DDX_Control(pDX, IDC_COMBO2,	cbnInstr);
	DDX_Control(pDX, IDC_COMBO3,	cbnVolCmd);
	DDX_Control(pDX, IDC_COMBO4,	cbnCommand);
	DDX_Control(pDX, IDC_COMBO5,	cbnPlugParam);
	DDX_Control(pDX, IDC_SLIDER1,	sldVolParam);
	DDX_Control(pDX, IDC_SLIDER2,	sldParam);
	//}}AFX_DATA_MAP
}


CEditCommand::CEditCommand(CSoundFile &sndFile) : sndFile(sndFile), oldSpecs(nullptr), effectInfo(sndFile), m(nullptr), modified(false)
//-------------------------------------------------------------------------------------------------------------------------------------
{
	CDialog::Create(IDD_PATTERN_EDITCOMMAND);
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
	return CDialog::PreTranslateMessage(pMsg);
}


bool CEditCommand::ShowEditWindow(PATTERNINDEX pat, const PatternCursor &cursor, CWnd *parent)
//--------------------------------------------------------------------------------------------
{
	editPos.pattern = pat;
	const ROWINDEX row = editPos.row = cursor.GetRow();
	const CHANNELINDEX chn = editPos.channel = cursor.GetChannel();

	if(!sndFile.Patterns.IsValidPat(pat)
		|| !sndFile.Patterns[pat].IsValidRow(row)
		|| chn >= sndFile.GetNumChannels())
	{
		ShowWindow(SW_HIDE);
		return false;
	}

	m = sndFile.Patterns[pat].GetpModCommand(row, chn);
	modified = false;

	InitAll();

	switch(cursor.GetColumnType())
	{
	case PatternCursor::noteColumn:
		cbnNote.SetFocus();
		break;
	case PatternCursor::instrColumn:
		cbnInstr.SetFocus();
		break;
	case PatternCursor::volumeColumn:
		if(m->IsPcNote())
			cbnPlugParam.SetFocus();
		else
			cbnVolCmd.SetFocus();
		break;
	case PatternCursor::effectColumn:
		if(m->IsPcNote())
			sldParam.SetFocus();
		else
			cbnCommand.SetFocus();
		break;
	case PatternCursor::paramColumn:
		sldParam.SetFocus();
		break;
	}

	// Update Window Title
	TCHAR s[64];
	_stprintf(s, _T("Note Properties - Row %d, Channel %d"), row, chn + 1);
	SetWindowText(s);

	CRect rectParent, rectWnd;
	parent->GetWindowRect(&rectParent);
	GetClientRect(&rectWnd);
	SetWindowPos(CMainFrame::GetMainFrame(),
		rectParent.left + (rectParent.Width() - rectWnd.right) / 2,
		rectParent.top + (rectParent.Height() - rectWnd.bottom) / 2,
		-1, -1, SWP_NOSIZE | SWP_NOACTIVATE);
	ShowWindow(SW_RESTORE);
	return true;
}


void CEditCommand::InitNote()
//---------------------------
{
	// Note
	cbnNote.SetRedraw(FALSE);
	if(oldSpecs != &sndFile.GetModSpecifications())
	{
		cbnNote.ResetContent();
		cbnNote.SetItemData(cbnNote.AddString(_T("No Note")), 0);
		AppendNotesToControlEx(cbnNote, sndFile, m->instr);
		oldSpecs = &sndFile.GetModSpecifications();
	}

	if(m->IsNote())
	{
		// Normal note / no note
		const ModCommand::NOTE noteStart = sndFile.GetModSpecifications().noteMin;
		cbnNote.SetCurSel(m->note - (noteStart - 1));
	} else if(m->note == NOTE_NONE)
	{
		cbnNote.SetCurSel(0);
	} else
	{
		// Special notes
		for(int i = cbnNote.GetCount() - 1; i >= 0; --i)
		{
			if(cbnNote.GetItemData(i) == m->note)
			{
				cbnNote.SetCurSel(i);
				break;
			}
		}
	}
	cbnNote.SetRedraw(TRUE);

	// Instrument
	cbnInstr.SetRedraw(FALSE);
	cbnInstr.ResetContent();

	if(m->IsPcNote())
	{
		// control plugin param note
		cbnInstr.SetItemData(cbnInstr.AddString(_T("No Effect")), 0);
		AddPluginNamesToCombobox(cbnInstr, sndFile.m_MixPlugins, false);
	} else
	{
		// instrument / sample
		cbnInstr.SetItemData(cbnInstr.AddString(_T("No Instrument")), 0);
		const uint32 nmax = sndFile.GetNumInstruments() ? sndFile.GetNumInstruments() : sndFile.GetNumSamples();
		for(uint32 i = 1; i <= nmax; i++)
		{
			std::string s = mpt::ToString(i) + ": ";
			// instrument / sample
			if(sndFile.GetNumInstruments())
			{
				if(sndFile.Instruments[i])
					s += sndFile.Instruments[i]->name;
			} else
				s += sndFile.m_szNames[i];
			cbnInstr.SetItemData(cbnInstr.AddString(s.c_str()), i);
		}
	}
	cbnInstr.SetCurSel(m->instr);
	cbnInstr.SetRedraw(TRUE);
}


void CEditCommand::InitVolume()
//-----------------------------
{
	cbnVolCmd.SetRedraw(FALSE);
	cbnVolCmd.ResetContent();
	if(sndFile.GetType() == MOD_TYPE_MOD || m->IsPcNote())
	{
		cbnVolCmd.EnableWindow(FALSE);
		sldVolParam.EnableWindow(FALSE);
	} else
	{
		// Normal volume column effect
		cbnVolCmd.EnableWindow(TRUE);
		sldVolParam.EnableWindow(TRUE);
		uint32 count = effectInfo.GetNumVolCmds();
		cbnVolCmd.SetItemData(cbnVolCmd.AddString(" None"), (DWORD_PTR)-1);
		cbnVolCmd.SetCurSel(0);
		UINT fxndx = effectInfo.GetIndexFromVolCmd(m->volcmd);
		for(uint32 i = 0; i < count; i++)
		{
			CHAR s[64];
			if(effectInfo.GetVolCmdInfo(i, s))
			{
				int k = cbnVolCmd.AddString(s);
				cbnVolCmd.SetItemData(k, i);
				if(i == fxndx) cbnVolCmd.SetCurSel(k);
			}
		}
		UpdateVolCmdRange();
	}
	cbnVolCmd.SetRedraw(TRUE);
}


void CEditCommand::InitEffect()
//-----------------------------
{
	if(m->IsPcNote())
	{
		cbnCommand.ShowWindow(SW_HIDE);
		return;
	}
	cbnCommand.ShowWindow(SW_SHOW);
	xParam = 0;
	xMultiplier = 1;
	getXParam(m->command, editPos.pattern, editPos.row, editPos.channel, sndFile, xParam, xMultiplier);

	cbnCommand.SetRedraw(FALSE);
	cbnCommand.ResetContent();
	uint32 numfx = effectInfo.GetNumEffects();
	uint32 fxndx = effectInfo.GetIndexFromEffect(m->command, m->param);
	cbnCommand.SetItemData(cbnCommand.AddString(" None"), (DWORD_PTR)-1);
	if(m->command == CMD_NONE) cbnCommand.SetCurSel(0);

	CHAR s[128];
	for(uint32 i = 0; i < numfx; i++)
	{
		if(effectInfo.GetEffectInfo(i, s, true))
		{
			int k = cbnCommand.AddString(s);
			cbnCommand.SetItemData(k, i);
			if (i == fxndx) cbnCommand.SetCurSel(k);
		}
	}
	UpdateEffectRange(false);
	cbnCommand.SetRedraw(TRUE);
	cbnCommand.Invalidate();
}


void CEditCommand::InitPlugParam()
//--------------------------------
{
	if(!m->IsPcNote())
	{
		cbnPlugParam.ShowWindow(SW_HIDE);
		return;
	}
	cbnPlugParam.ShowWindow(SW_SHOW);
	
	cbnPlugParam.SetRedraw(FALSE);
	cbnPlugParam.ResetContent();
	
	if(m->instr > 0 && m->instr <= MAX_MIXPLUGINS)
	{
		AddPluginParameternamesToCombobox(cbnPlugParam, sndFile.m_MixPlugins[m->instr - 1]);
		cbnPlugParam.SetCurSel(m->GetValueVolCol());
	}
	UpdateEffectRange(false);

	cbnPlugParam.SetRedraw(TRUE);
	cbnPlugParam.Invalidate();
}


void CEditCommand::UpdateVolCmdRange()
//------------------------------------
{
	ModCommand::VOL rangeMin = 0, rangeMax = 0;
	LONG fxndx = effectInfo.GetIndexFromVolCmd(m->volcmd);
	bool ok = effectInfo.GetVolCmdInfo(fxndx, NULL, &rangeMin, &rangeMax);
	if(ok && rangeMax > rangeMin)
	{
		sldVolParam.EnableWindow(TRUE);
		sldVolParam.SetRange(rangeMin, rangeMax);
		Limit(m->vol, rangeMin, rangeMax);
		sldVolParam.SetPos(m->vol);
	} else
	{
		// Why does this not update the display at all?
		sldVolParam.SetRange(0, 0);
		sldVolParam.SetPos(0);
		sldVolParam.EnableWindow(FALSE);
	}
	UpdateVolCmdValue();
}


void CEditCommand::UpdateEffectRange(bool set)
//--------------------------------------------
{
	DWORD pos;
	bool enable = true;

	if(m->IsPcNote())
	{
		// plugin param control note
		sldParam.SetRange(0, ModCommand::maxColumnValue);
		pos = m->GetValueEffectCol();
	} else
	{
		// process as effect
		ModCommand::PARAM rangeMin = 0, rangeMax = 0;
		LONG fxndx = effectInfo.GetIndexFromEffect(m->command, m->param);
		enable = ((fxndx >= 0) && (effectInfo.GetEffectInfo(fxndx, NULL, false, &rangeMin, &rangeMax)));

		pos = effectInfo.MapValueToPos(fxndx, m->param);
		if(pos > rangeMax) pos = rangeMin | (pos & 0x0F);
		Limit(pos, rangeMin, rangeMax);

		sldParam.SetRange(rangeMin, rangeMax);
	}

	if(enable)
	{
		sldParam.EnableWindow(TRUE);
		sldParam.SetPageSize(1);
		sldParam.SetPos(pos);
	} else
	{
		// Why does this not update the display at all?
		sldParam.SetRange(0, 0);
		sldParam.SetPos(0);
		sldParam.EnableWindow(FALSE);
	}
	UpdateEffectValue(set);
}


void CEditCommand::OnNoteChanged()
//--------------------------------
{
	const bool wasParamControl = m->IsPcNote();
	ModCommand::NOTE newNote = m->note;
	ModCommand::INSTR newInstr = m->instr;

	int n = cbnNote.GetCurSel();
	if(n >= 0) newNote = static_cast<ModCommand::NOTE>(cbnNote.GetItemData(n));

	n = cbnInstr.GetCurSel();
	if(n >= 0) newInstr = static_cast<ModCommand::INSTR>(cbnInstr.GetItemData(n));

	if(m->note != newNote || m->instr != newInstr)
	{
		PrepareUndo("Note Entry");
		CModDoc *modDoc = sndFile.GetpModDoc();
		m->note = newNote;
		m->instr = newInstr;

		modDoc->UpdateAllViews(NULL, RowHint(editPos.row), NULL);

		if(wasParamControl != m->IsPcNote())
		{
			InitAll();
		} else if(!m->IsPcNote()
			&& m->instr <= sndFile.GetNumInstruments()
			&& newInstr <= sndFile.GetNumInstruments()
			&& sndFile.Instruments[m->instr] != nullptr
			&& sndFile.Instruments[newInstr] != nullptr
			&& sndFile.Instruments[newInstr]->pTuning != sndFile.Instruments[m->instr]->pTuning)
		{
			//Checking whether note names should be recreated.
			InitNote();
		} else if(m->IsPcNote())
		{
			// Update parameter list
			InitEffect();
		}
	}
}


void CEditCommand::OnVolCmdChanged()
//----------------------------------
{
	ModCommand::VOLCMD newVolCmd = m->volcmd;
	ModCommand::VOL newVol = m->vol;

	int n = cbnVolCmd.GetCurSel();
	if(n >= 0)
	{
		newVolCmd = effectInfo.GetVolCmdFromIndex(cbnVolCmd.GetItemData(n));
	}

	newVol = static_cast<ModCommand::VOL>(sldVolParam.GetPos());

	const bool volCmdChanged = m->volcmd != newVolCmd;
	if(volCmdChanged || m->vol != newVol)
	{
		PrepareUndo("Volume Entry");
		CModDoc *modDoc = sndFile.GetpModDoc();
		m->volcmd = newVolCmd;
		m->vol = newVol;

		modDoc->UpdateAllViews(NULL, RowHint(editPos.row), NULL);

		if(volCmdChanged)
			UpdateVolCmdRange();
		else
			UpdateVolCmdValue();
	}
}


void CEditCommand::OnCommandChanged()
//-----------------------------------
{
	ModCommand::COMMAND newCommand = m->command;
	ModCommand::PARAM newParam = m->param;

	int n = cbnCommand.GetCurSel();
	if(n >= 0)
	{
		int ndx = cbnCommand.GetItemData(n);
		newCommand = static_cast<ModCommand::COMMAND>((ndx >= 0) ? effectInfo.GetEffectFromIndex(ndx, newParam) : CMD_NONE);
	}

	if(newCommand == CMD_OFFSET || newCommand == CMD_PATTERNBREAK || newCommand == CMD_POSITIONJUMP || newCommand == CMD_TEMPO || newCommand == CMD_XPARAM)
	{
		xParam = 0;
		xMultiplier = 1;
		getXParam(newCommand, editPos.pattern, editPos.row, editPos.channel, sndFile, xParam, xMultiplier);
	}

	if(m->command != newCommand || m->param != newParam)
	{
		PrepareUndo("Effect Entry");

		m->command = newCommand;
		if(newCommand != CMD_NONE)
		{
			m->param = newParam;
		}
		UpdateEffectRange(true);

		sndFile.GetpModDoc()->UpdateAllViews(NULL, RowHint(editPos.row), NULL);
	}
}


void CEditCommand::OnPlugParamChanged()
//-------------------------------------
{
	uint16 newPlugParam = m->GetValueVolCol();

	int n = cbnPlugParam.GetCurSel();
	if(n >= 0)
	{
		newPlugParam = static_cast<uint16>(cbnPlugParam.GetItemData(n));
	}

	if(m->GetValueVolCol() != newPlugParam)
	{
		PrepareUndo("Effect Entry");
		m->SetValueVolCol(newPlugParam);
		sndFile.GetpModDoc()->UpdateAllViews(NULL, RowHint(editPos.row), NULL);
	}
}


void CEditCommand::UpdateVolCmdValue()
//------------------------------------
{
	CHAR s[64] = "";
	if(m->IsPcNote())
	{
		// plugin param control note
		uint16 plugParam = static_cast<uint16>(sldVolParam.GetPos());
		wsprintf(s, "Value: %u", plugParam);
	} else
	{
		// process as effect
		effectInfo.GetVolCmdParamInfo(*m, s);
	}
	SetDlgItemText(IDC_TEXT2, s);
}


void CEditCommand::UpdateEffectValue(bool set)
//--------------------------------------------
{
	TCHAR s[128] = _T("");

	uint16 newPlugParam = 0;
	ModCommand::PARAM newParam = 0;

	if(m->IsPcNote())
	{
		// plugin param control note
		newPlugParam = static_cast<uint16>(sldParam.GetPos());
		wsprintf(s, _T("Value: %u"), newPlugParam);
	} else
	{
		// process as effect
		LONG fxndx = effectInfo.GetIndexFromEffect(m->command, m->param);
		if(fxndx >= 0)
		{
			newParam = static_cast<ModCommand::PARAM>(effectInfo.MapPosToValue(fxndx, sldParam.GetPos()));
			effectInfo.GetEffectNameEx(s, fxndx, newParam * xMultiplier + xParam, editPos.channel);
		}
	}
	SetDlgItemText(IDC_TEXT1, s);

	if(set)
	{
		if((!m->IsPcNote() && m->param != newParam)
			|| (m->IsPcNote() && m->GetValueVolCol() != newPlugParam))
		{
			PrepareUndo("Effect Entry");
			CModDoc *modDoc = sndFile.GetpModDoc();
			if(m->IsPcNote())
			{
				m->SetValueEffectCol(newPlugParam);
			} else
			{
				m->param = newParam;
			}

			modDoc->UpdateAllViews(NULL, RowHint(editPos.row), NULL);
		}
	}
}


void CEditCommand::PrepareUndo(const char *description)
//-----------------------------------------------------
{
	CModDoc *modDoc = sndFile.GetpModDoc();
	if(!modified)
	{
		// Let's create just one undo step.
		modDoc->GetPatternUndo().PrepareUndo(editPos.pattern, editPos.channel, editPos.row, 1, 1, description);
		modified = true;
	}
	modDoc->SetModified();
}


void CEditCommand::OnHScroll(UINT, UINT, CScrollBar *bar)
//-------------------------------------------------------
{
	if(bar == static_cast<CWnd *>(&sldVolParam))
	{
		OnVolCmdChanged();
	} else if(bar == static_cast<CWnd *>(&sldParam))
	{
		UpdateEffectValue(true);
	}
}


void CEditCommand::OnActivate(UINT nState, CWnd *pWndOther, BOOL bMinimized)
//--------------------------------------------------------------------------
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);
	if(nState == WA_INACTIVE) ShowWindow(SW_HIDE);
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
	AppendNotesToControl(m_CbnShortcut, NOTE_MIN, NOTE_MIN + 3 * 12 - 1);

	m_CbnShortcut.SetCurSel(0);
	// Base Note combo box
	m_CbnBaseNote.SetItemData(m_CbnBaseNote.AddString("Relative"), MPTChord::relativeMode);
	AppendNotesToControl(m_CbnBaseNote, NOTE_MIN, NOTE_MIN + 3 * 12 - 1);

	// Minor notes
	for (int inotes=-1; inotes<24; inotes++)
	{
		if (inotes < 0) strcpy(s, "--"); else
			if (inotes < 12) wsprintf(s, "%s", CSoundFile::m_NoteNames[inotes % 12]);
			else wsprintf(s, "%s (+%d)", CSoundFile::m_NoteNames[inotes % 12], inotes / 12);
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
	if(chord >= 0) chord = m_CbnShortcut.GetItemData(chord) - NOTE_MIN;
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
		uint8 b = CKeyboardControl::KEYFLAG_NORMAL;
		if(i == note) b = CKeyboardControl::KEYFLAG_REDDOT;
		else if(chord.notes[0] && i + 1 == chord.notes[0]) b = CKeyboardControl::KEYFLAG_REDDOT;
		else if(chord.notes[1] && i + 1 == chord.notes[1]) b = CKeyboardControl::KEYFLAG_REDDOT;
		else if(chord.notes[2] && i + 1 == chord.notes[2]) b = CKeyboardControl::KEYFLAG_REDDOT;
		m_Keyboard.SetFlags(i, b);
	}
	m_Keyboard.InvalidateRect(NULL, FALSE);
}


void CChordEditor::OnBaseNoteChanged()
//------------------------------------
{
	MPTChord &chord = GetChord();
	int basenote = m_CbnBaseNote.GetItemData(m_CbnBaseNote.GetCurSel()) - NOTE_MIN;
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
	AppendNotesToControl(m_CbnSplitNote, sndFile.GetModSpecifications().noteMin, sndFile.GetModSpecifications().noteMax);
	m_CbnSplitNote.SetCurSel(m_Settings.splitNote - (sndFile.GetModSpecifications().noteMin - NOTE_MIN));

	// Octave modifier
	for(int i = -SplitKeyboardSettings::splitOctaveRange; i < SplitKeyboardSettings::splitOctaveRange + 1; i++)
	{
		wsprintf(s, i < 0 ? "Octave -%d" : i > 0 ? "Octave +%d" : "No Change", mpt::abs(i));
		int n = m_CbnOctaveModifier.AddString(s);
		m_CbnOctaveModifier.SetItemData(n, i);
	}

	m_CbnOctaveModifier.SetCurSel(m_Settings.octaveModifier + SplitKeyboardSettings::splitOctaveRange);
	CheckDlgButton(IDC_PATTERN_OCTAVELINK, (m_Settings.octaveLink && m_Settings.octaveModifier != 0) ? BST_CHECKED : BST_UNCHECKED);

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

	m_Settings.splitNote = static_cast<ModCommand::NOTE>(m_CbnSplitNote.GetItemData(m_CbnSplitNote.GetCurSel()) - 1);
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
	CheckDlgButton(IDC_PATTERN_OCTAVELINK, (m_CbnOctaveModifier.GetCurSel() != 9) ? BST_CHECKED : BST_UNCHECKED);
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
		document->GetPatternUndo().PrepareUndo(pattern, 0, 0, 1, 1, "Channel Settings", false, true);
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
		document->UpdateAllViews(nullptr, GeneralHint(channel).Channels());
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
		document->UpdateAllViews(nullptr, GeneralHint(channel).Channels());
	}
}


void QuickChannelProperties::OnHScroll(UINT, UINT, CScrollBar *bar)
//-----------------------------------------------------------------
{
	if(!visible)
	{
		return;
	}

	bool update = false;

	// Volume slider
	if(bar == reinterpret_cast<CScrollBar *>(&volSlider))
	{
		uint16 pos = static_cast<uint16>(volSlider.GetPos());
		PrepareUndo();
		if(document->SetChannelGlobalVolume(channel, pos))
		{
			SetDlgItemInt(IDC_EDIT1, pos);
			update = true;
		}
	}
	// Pan slider
	if(bar == reinterpret_cast<CScrollBar *>(&panSlider))
	{
		uint16 pos = static_cast<uint16>(panSlider.GetPos());
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
		document->UpdateAllViews(nullptr, GeneralHint(channel).Channels());
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
	document->UpdateAllViews(nullptr, GeneralHint(channel).Channels());
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
	document->UpdateAllViews(nullptr, GeneralHint(channel).Channels());
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
		document->UpdateAllViews(nullptr, GeneralHint(channel).Channels());
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
			CInputHandler* ih = CMainFrame::GetInputHandler();

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


OPENMPT_NAMESPACE_END
