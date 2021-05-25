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
#include "../common/mptStringBuffer.h"


OPENMPT_NAMESPACE_BEGIN


static constexpr EffectCommand ExtendedCommands[] = {CMD_OFFSET, CMD_PATTERNBREAK, CMD_POSITIONJUMP, CMD_TEMPO, CMD_FINETUNE, CMD_FINETUNE_SMOOTH};

// For a given pattern cell, check if it contains a command supported by the X-Param mechanism.
// If so, calculate the multipler for this cell and the value of all the other cells belonging to this param.
void getXParam(ModCommand::COMMAND command, PATTERNINDEX nPat, ROWINDEX nRow, CHANNELINDEX nChannel, const CSoundFile &sndFile, UINT &xparam, UINT &multiplier)
{
	UINT xp = 0, mult = 1;
	int cmdRow = static_cast<int>(nRow);
	const auto &pattern = sndFile.Patterns[nPat];

	if(command == CMD_XPARAM)
	{
		// current command is a parameter extension command
		cmdRow--;

		// Try to find previous command parameter to be extended
		while(cmdRow >= 0)
		{
			const ModCommand &m = *pattern.GetpModCommand(cmdRow, nChannel);
			if(mpt::contains(ExtendedCommands, m.command))
				break;
			if(m.command != CMD_XPARAM)
			{
				cmdRow = -1;
				break;
			}
			cmdRow--;
		}
	} else if(!mpt::contains(ExtendedCommands, command))
	{
		// If current row do not own any satisfying command parameter to extend, set return state
		cmdRow = -1;
	}

	if(cmdRow >= 0)
	{
		// An 'extendable' command parameter has been found
		const ModCommand &m = *pattern.GetpModCommand(cmdRow, nChannel);

		// Find extension resolution (8 to 24 bits)
		uint32 n = 1;
		while(n < 4 && cmdRow + n < pattern.GetNumRows())
		{
			if(pattern.GetpModCommand(cmdRow + n, nChannel)->command != CMD_XPARAM)
				break;
			n++;
		}

		// Parameter extension found (above 8 bits non-standard parameters)
		if(n > 1)
		{
			// Limit offset command to 24 bits, other commands to 16 bits
			n = m.command == CMD_OFFSET ? n : (n > 2 ? 2 : n);

			// Compute extended value WITHOUT current row parameter value : this parameter
			// is being currently edited (this is why this function is being called) so we
			// only need to compute a multiplier so that we can add its contribution while
			// its value is changed by user
			for(uint32 j = 0; j < n; j++)
			{
				const ModCommand &mx = *pattern.GetpModCommand(cmdRow + j, nChannel);

				uint32 k = 8 * (n - j - 1);
				if(cmdRow + j == nRow)
					mult = 1 << k;
				else
					xp += (mx.param << k);
			}
		} else if(m.command == CMD_OFFSET || m.command == CMD_FINETUNE || m.command == CMD_FINETUNE_SMOOTH)
		{
			// No parameter extension to perform (8 bits standard parameter),
			// just care about offset command special case (16 bits, fake)
			mult <<= 8;
		}

		const auto modDoc = sndFile.GetpModDoc();
		if(m.command == CMD_OFFSET && m.volcmd == VOLCMD_OFFSET && modDoc != nullptr)
		{
			SAMPLEINDEX smp = modDoc->GetSampleIndex(m);
			if(m.vol == 0 && smp != 0)
			{
				xp = Util::muldivr_unsigned(sndFile.GetSample(smp).nLength, pattern.GetpModCommand(nRow, nChannel)->param  * mult + xp, 256u << (8u * (std::max(uint32(2), n) - 1u)));
				mult = 0;
			} else if(m.vol > 0 && smp != 0)
			{
				xp += sndFile.GetSample(smp).cues[m.vol - 1];
			}
		}
	}

	// Return x-parameter
	multiplier = mult;
	xparam = xp;
}


/////////////////////////////////////////////////////////////////////////////////////////////
// CPatternPropertiesDlg

BEGIN_MESSAGE_MAP(CPatternPropertiesDlg, CDialog)
	ON_COMMAND(IDC_BUTTON_HALF,		&CPatternPropertiesDlg::OnHalfRowNumber)
	ON_COMMAND(IDC_BUTTON_DOUBLE,	&CPatternPropertiesDlg::OnDoubleRowNumber)
	ON_COMMAND(IDC_CHECK1,			&CPatternPropertiesDlg::OnOverrideSignature)
	ON_COMMAND(IDC_BUTTON1,			&CPatternPropertiesDlg::OnTempoSwing)
END_MESSAGE_MAP()

BOOL CPatternPropertiesDlg::OnInitDialog()
{
	CComboBox *combo;
	CDialog::OnInitDialog();
	combo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	const CSoundFile &sndFile = modDoc.GetSoundFile();

	if(m_nPattern < sndFile.Patterns.Size() && combo)
	{
		CString s;
		const CPattern &pattern = sndFile.Patterns[m_nPattern];
		ROWINDEX nrows = pattern.GetNumRows();

		const CModSpecifications &specs = sndFile.GetModSpecifications();
		combo->SetRedraw(FALSE);
		for(UINT irow = specs.patternRowsMin; irow <= specs.patternRowsMax; irow++)
		{
			combo->AddString(mpt::cfmt::dec(irow));
		}
		combo->SetCurSel(nrows - specs.patternRowsMin);
		combo->SetRedraw(TRUE);

		CheckRadioButton(IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);

		s = MPT_CFORMAT("Pattern #{}: {} row{} ({}K)")(
			m_nPattern,
			pattern.GetNumRows(),
			(pattern.GetNumRows() == 1) ? CString(_T("")) : CString(_T("s")),
			static_cast<int>((pattern.GetNumRows() * sndFile.GetNumChannels() * sizeof(ModCommand)) / 1024));
		SetDlgItemText(IDC_TEXT1, s);

		// Window title
		const CString patternName = mpt::ToCString(sndFile.GetCharsetInternal(), pattern.GetName());
		s = MPT_CFORMAT("Pattern Properties for Pattern #{}")(m_nPattern);
		if(!patternName.IsEmpty())
		{
			s += _T(" (");
			s += patternName;
			s += _T(")");
		}
		SetWindowText(s);

		// Pattern time signature
		const bool bOverride = pattern.GetOverrideSignature();
		ROWINDEX nRPB = pattern.GetRowsPerBeat(), nRPM = pattern.GetRowsPerMeasure();
		if(nRPB == 0 || !bOverride)
			nRPB = sndFile.m_nDefaultRowsPerBeat;
		if(nRPM == 0 || !bOverride)
			nRPM = sndFile.m_nDefaultRowsPerMeasure;

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
{
	const CSoundFile &sndFile = modDoc.GetSoundFile();

	UINT nRows = GetDlgItemInt(IDC_COMBO1, NULL, FALSE);
	nRows /= 2;
	if(nRows < sndFile.GetModSpecifications().patternRowsMin)
		nRows = sndFile.GetModSpecifications().patternRowsMin;
	SetDlgItemInt(IDC_COMBO1, nRows, FALSE);
}


void CPatternPropertiesDlg::OnDoubleRowNumber()
{
	const CSoundFile &sndFile = modDoc.GetSoundFile();

	UINT nRows = GetDlgItemInt(IDC_COMBO1, NULL, FALSE);
	nRows *= 2;
	if(nRows > sndFile.GetModSpecifications().patternRowsMax)
		nRows = sndFile.GetModSpecifications().patternRowsMax;
	SetDlgItemInt(IDC_COMBO1, nRows, FALSE);
}


void CPatternPropertiesDlg::OnOverrideSignature()
{
	GetDlgItem(IDC_ROWSPERBEAT)->EnableWindow(IsDlgButtonChecked(IDC_CHECK1));
	GetDlgItem(IDC_ROWSPERMEASURE)->EnableWindow(IsDlgButtonChecked(IDC_CHECK1));
	GetDlgItem(IDC_BUTTON1)->EnableWindow(IsDlgButtonChecked(IDC_CHECK1) && modDoc.GetSoundFile().m_nTempoMode == TempoMode::Modern);
}


void CPatternPropertiesDlg::OnTempoSwing()
{
	CPattern &pat = modDoc.GetSoundFile().Patterns[m_nPattern];
	const ROWINDEX oldRPB = pat.GetRowsPerBeat();
	const ROWINDEX oldRPM = pat.GetRowsPerMeasure();

	// Temporarily apply new tempo signature for preview
	ROWINDEX newRPB = std::max(1u, GetDlgItemInt(IDC_ROWSPERBEAT));
	ROWINDEX newRPM = std::max(newRPB, GetDlgItemInt(IDC_ROWSPERMEASURE));
	pat.SetSignature(newRPB, newRPM);

	m_tempoSwing.resize(newRPB, TempoSwing::Unity);
	CTempoSwingDlg dlg(this, m_tempoSwing, modDoc.GetSoundFile(), m_nPattern);
	if(dlg.DoModal() == IDOK)
	{
		m_tempoSwing = dlg.m_tempoSwing;
	}
	pat.SetSignature(oldRPB, oldRPM);
}


void CPatternPropertiesDlg::OnOK()
{
	CSoundFile &sndFile = modDoc.GetSoundFile();
	CPattern &pattern = sndFile.Patterns[m_nPattern];
	// Update pattern signature if necessary
	if(sndFile.GetModSpecifications().hasPatternSignatures)
	{
		if(IsDlgButtonChecked(IDC_CHECK1))
		{
			// Enable signature
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
		} else
		{
			// Disable signature
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
	bool resizeAtEnd = IsDlgButtonChecked(IDC_RADIO2) != BST_UNCHECKED;
	if(newSize < sndFile.Patterns[m_nPattern].GetNumRows())
	{
		ROWINDEX firstRow = resizeAtEnd ? newSize : 0;
		ROWINDEX lastRow = resizeAtEnd ? sndFile.Patterns[m_nPattern].GetNumRows() : sndFile.Patterns[m_nPattern].GetNumRows() - newSize;
		for(ROWINDEX row = firstRow; row < lastRow; row++)
		{
			if(!sndFile.Patterns[m_nPattern].IsEmptyRow(row))
			{
				resize = (Reporting::Confirm(MPT_AFORMAT("Data at the {} of the pattern will be lost.\nDo you want to continue?")(resizeAtEnd ? "end" : "start"), "Shrink Pattern") == cnfYes);
				break;
			}
		}
	}

	if(resize)
	{
		modDoc.BeginWaitCursor();
		modDoc.GetPatternUndo().PrepareUndo(m_nPattern, 0, 0, sndFile.Patterns[m_nPattern].GetNumChannels(), sndFile.Patterns[m_nPattern].GetNumRows(), "Resize");
		if(sndFile.Patterns[m_nPattern].Resize(newSize, true, resizeAtEnd))
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

	ON_CBN_SELCHANGE(IDC_COMBO1,	&CEditCommand::OnNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	&CEditCommand::OnNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,	&CEditCommand::OnVolCmdChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4,	&CEditCommand::OnCommandChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5,	&CEditCommand::OnPlugParamChanged)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


void CEditCommand::DoDataExchange(CDataExchange* pDX)
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


CEditCommand::CEditCommand(CSoundFile &sndFile)
    : sndFile(sndFile), effectInfo(sndFile)
{
	CDialog::Create(IDD_PATTERN_EDITCOMMAND);
}


BOOL CEditCommand::PreTranslateMessage(MSG *pMsg)
{
	if((pMsg) && (pMsg->message == WM_KEYDOWN))
	{
		if((pMsg->wParam == VK_ESCAPE) || (pMsg->wParam == VK_RETURN) || (pMsg->wParam == VK_APPS))
		{
			OnClose();
			return TRUE;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}


bool CEditCommand::ShowEditWindow(PATTERNINDEX pat, const PatternCursor &cursor, CWnd *parent)
{
	editPattern = pat;
	const ROWINDEX row = editRow = cursor.GetRow();
	const CHANNELINDEX chn = editChannel = cursor.GetChannel();

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
	SetWindowText(MPT_CFORMAT("Note Properties - Row {}, Channel {}")(row, chn + 1));

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
			CString s = mpt::cfmt::val(i) + _T(": ");
			// instrument / sample
			if(sndFile.GetNumInstruments())
			{
				if(sndFile.Instruments[i])
					s += mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.Instruments[i]->name);
			} else
				s += mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.m_szNames[i]);
			cbnInstr.SetItemData(cbnInstr.AddString(s), i);
		}
	}
	cbnInstr.SetCurSel(m->instr);
	cbnInstr.SetRedraw(TRUE);
}


void CEditCommand::InitVolume()
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
		cbnVolCmd.SetItemData(cbnVolCmd.AddString(_T(" None")), (DWORD_PTR)-1);
		cbnVolCmd.SetCurSel(0);
		UINT fxndx = effectInfo.GetIndexFromVolCmd(m->volcmd);
		for(uint32 i = 0; i < count; i++)
		{
			CString s;
			if(effectInfo.GetVolCmdInfo(i, &s))
			{
				int k = cbnVolCmd.AddString(s);
				cbnVolCmd.SetItemData(k, i);
				if(i == fxndx)
					cbnVolCmd.SetCurSel(k);
			}
		}
		UpdateVolCmdRange();
	}
	cbnVolCmd.SetRedraw(TRUE);
}


void CEditCommand::InitEffect()
{
	if(m->IsPcNote())
	{
		cbnCommand.ShowWindow(SW_HIDE);
		return;
	}
	cbnCommand.ShowWindow(SW_SHOW);
	xParam = 0;
	xMultiplier = 1;
	getXParam(m->command, editPattern, editRow, editChannel, sndFile, xParam, xMultiplier);

	cbnCommand.SetRedraw(FALSE);
	cbnCommand.ResetContent();
	uint32 numfx = effectInfo.GetNumEffects();
	uint32 fxndx = effectInfo.GetIndexFromEffect(m->command, m->param);
	cbnCommand.SetItemData(cbnCommand.AddString(_T(" None")), (DWORD_PTR)-1);
	if(m->command == CMD_NONE)
		cbnCommand.SetCurSel(0);

	CString s;
	for(uint32 i = 0; i < numfx; i++)
	{
		if(effectInfo.GetEffectInfo(i, &s, true))
		{
			int k = cbnCommand.AddString(s);
			cbnCommand.SetItemData(k, i);
			if(i == fxndx)
				cbnCommand.SetCurSel(k);
		}
	}
	UpdateEffectRange(false);
	cbnCommand.SetRedraw(TRUE);
	cbnCommand.Invalidate();
}


void CEditCommand::InitPlugParam()
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
		if(pos > rangeMax)
			pos = rangeMin | (pos & 0x0F);
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
{
	const bool wasParamControl = m->IsPcNote();
	ModCommand::NOTE newNote = m->note;
	ModCommand::INSTR newInstr = m->instr;

	int n = cbnNote.GetCurSel();
	if(n >= 0)
		newNote = static_cast<ModCommand::NOTE>(cbnNote.GetItemData(n));

	n = cbnInstr.GetCurSel();
	if(n >= 0)
		newInstr = static_cast<ModCommand::INSTR>(cbnInstr.GetItemData(n));

	if(m->note != newNote || m->instr != newInstr)
	{
		PrepareUndo("Note Entry");
		CModDoc *modDoc = sndFile.GetpModDoc();
		m->note = newNote;
		m->instr = newInstr;

		modDoc->UpdateAllViews(nullptr, RowHint(editRow), nullptr);

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
			InitPlugParam();
		}
	}
}


void CEditCommand::OnVolCmdChanged()
{
	ModCommand::VOLCMD newVolCmd = m->volcmd;
	ModCommand::VOL newVol = m->vol;

	int n = cbnVolCmd.GetCurSel();
	if(n >= 0)
	{
		newVolCmd = effectInfo.GetVolCmdFromIndex(static_cast<UINT>(cbnVolCmd.GetItemData(n)));
	}

	newVol = static_cast<ModCommand::VOL>(sldVolParam.GetPos());

	const bool volCmdChanged = m->volcmd != newVolCmd;
	if(volCmdChanged || m->vol != newVol)
	{
		PrepareUndo("Volume Entry");
		CModDoc *modDoc = sndFile.GetpModDoc();
		m->volcmd = newVolCmd;
		m->vol = newVol;

		modDoc->UpdateAllViews(nullptr, RowHint(editRow), nullptr);

		if(volCmdChanged)
			UpdateVolCmdRange();
		else
			UpdateVolCmdValue();
	}
}


void CEditCommand::OnCommandChanged()
{
	ModCommand::COMMAND newCommand = m->command;
	ModCommand::PARAM newParam = m->param;

	int n = cbnCommand.GetCurSel();
	if(n >= 0)
	{
		int ndx = static_cast<int>(cbnCommand.GetItemData(n));
		newCommand = static_cast<ModCommand::COMMAND>((ndx >= 0) ? effectInfo.GetEffectFromIndex(ndx, newParam) : CMD_NONE);
	}

	if(newCommand == CMD_XPARAM || mpt::contains(ExtendedCommands, newCommand))
	{
		xParam = 0;
		xMultiplier = 1;
		getXParam(newCommand, editPattern, editRow, editChannel, sndFile, xParam, xMultiplier);
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

		sndFile.GetpModDoc()->UpdateAllViews(nullptr, RowHint(editRow), nullptr);
	}
}


void CEditCommand::OnPlugParamChanged()
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
		sndFile.GetpModDoc()->UpdateAllViews(nullptr, RowHint(editRow), nullptr);
	}
}


void CEditCommand::UpdateVolCmdValue()
{
	CString s;
	if(m->IsPcNote())
	{
		// plugin param control note
		uint16 plugParam = static_cast<uint16>(sldVolParam.GetPos());
		s.Format(_T("Value: %u"), plugParam);
	} else
	{
		// process as effect
		effectInfo.GetVolCmdParamInfo(*m, &s);
	}
	SetDlgItemText(IDC_TEXT2, s);
}


void CEditCommand::UpdateEffectValue(bool set)
{
	CString s;

	uint16 newPlugParam = 0;
	ModCommand::PARAM newParam = 0;

	if(m->IsPcNote())
	{
		// plugin param control note
		newPlugParam = static_cast<uint16>(sldParam.GetPos());
		s.Format(_T("Value: %u"), newPlugParam);
	} else
	{
		// process as effect
		LONG fxndx = effectInfo.GetIndexFromEffect(m->command, m->param);
		if(fxndx >= 0)
		{
			newParam = static_cast<ModCommand::PARAM>(effectInfo.MapPosToValue(fxndx, sldParam.GetPos()));
			effectInfo.GetEffectNameEx(s, fxndx, newParam * xMultiplier + xParam, editChannel);
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

			modDoc->UpdateAllViews(nullptr, RowHint(editRow), nullptr);
		}
	}
}


void CEditCommand::PrepareUndo(const char *description)
{
	CModDoc *modDoc = sndFile.GetpModDoc();
	if(!modified)
	{
		// Let's create just one undo step.
		modDoc->GetPatternUndo().PrepareUndo(editPattern, editChannel, editRow, 1, 1, description);
		modified = true;
	}
	modDoc->SetModified();
}


void CEditCommand::OnHScroll(UINT, UINT, CScrollBar *bar)
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
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);
	if(nState == WA_INACTIVE)
		ShowWindow(SW_HIDE);
}


////////////////////////////////////////////////////////////////////////////////////////////
// Chord Editor

BEGIN_MESSAGE_MAP(CChordEditor, ResizableDialog)
	ON_MESSAGE(WM_MOD_KBDNOTIFY, &CChordEditor::OnKeyboardNotify)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CChordEditor::OnChordChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CChordEditor::OnBaseNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CChordEditor::OnNote1Changed)
	ON_CBN_SELCHANGE(IDC_COMBO4, &CChordEditor::OnNote2Changed)
	ON_CBN_SELCHANGE(IDC_COMBO5, &CChordEditor::OnNote3Changed)
END_MESSAGE_MAP()


void CChordEditor::DoDataExchange(CDataExchange *pDX)
{
	ResizableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChordEditor)
	DDX_Control(pDX, IDC_KEYBOARD1, m_Keyboard);
	DDX_Control(pDX, IDC_COMBO1,    m_CbnShortcut);
	DDX_Control(pDX, IDC_COMBO2,    m_CbnBaseNote);
	DDX_Control(pDX, IDC_COMBO3,    m_CbnNote[0]);
	DDX_Control(pDX, IDC_COMBO4,    m_CbnNote[1]);
	DDX_Control(pDX, IDC_COMBO5,    m_CbnNote[2]);
	static_assert(mpt::array_size<decltype(m_CbnNote)>::size == 3);
	//}}AFX_DATA_MAP
}


CChordEditor::CChordEditor(CWnd *parent)
    : ResizableDialog(IDD_CHORDEDIT, parent)
{
	m_chords = TrackerSettings::GetChords();
}

BOOL CChordEditor::OnInitDialog()
{
	ResizableDialog::OnInitDialog();
	m_Keyboard.Init(this, (CHORD_MAX - CHORD_MIN) / 12, true);

	m_CbnShortcut.SetRedraw(FALSE);
	m_CbnBaseNote.SetRedraw(FALSE);
	for(auto &combo : m_CbnNote)
		combo.SetRedraw(FALSE);

	// Shortcut key combo box
	AppendNotesToControl(m_CbnShortcut, NOTE_MIN, NOTE_MIN + static_cast<int>(kcVPEndChords) - static_cast<int>(kcVPStartChords));

	m_CbnShortcut.SetCurSel(0);
	// Base Note combo box
	m_CbnBaseNote.SetItemData(m_CbnBaseNote.AddString(_T("Relative")), MPTChord::relativeMode);
	AppendNotesToControl(m_CbnBaseNote, NOTE_MIN, NOTE_MIN + 3 * 12 - 1);

	// Chord Note combo boxes
	CString s;
	for(int note = CHORD_MIN - 1; note < CHORD_MAX; note++)
	{
		int noteVal = note;
		if(note == CHORD_MIN - 1)
		{
			s = _T("--");
			noteVal = MPTChord::noNote;
		} else
		{
			s = mpt::ToCString(CSoundFile::GetDefaultNoteName(mpt::wrapping_modulo(note, 12)));
			const int octave = mpt::wrapping_divide(note, 12);
			if(octave > 0)
				s.AppendFormat(_T(" (+%d)"), octave);
			else if(octave < 0)
				s.AppendFormat(_T(" (%d)"), octave);
		}
		for(auto &combo : m_CbnNote)
			combo.SetItemData(combo.AddString(s), noteVal);
	}

	m_CbnShortcut.SetRedraw(TRUE);
	m_CbnBaseNote.SetRedraw(TRUE);
	for(auto &combo : m_CbnNote)
		combo.SetRedraw(TRUE);

	// Update Dialog
	OnChordChanged();
	return TRUE;
}


void CChordEditor::OnOK()
{
	TrackerSettings::GetChords() = m_chords;
	ResizableDialog::OnOK();
}


MPTChord &CChordEditor::GetChord()
{
	int chord = m_CbnShortcut.GetCurSel();
	if(chord >= 0)
		chord = static_cast<int>(m_CbnShortcut.GetItemData(chord)) - NOTE_MIN;
	if(chord < 0 || chord >= static_cast<int>(m_chords.size()))
		chord = 0;
	return m_chords[chord];
}


LRESULT CChordEditor::OnKeyboardNotify(WPARAM cmd, LPARAM nKey)
{
	const bool outside = static_cast<int>(nKey) == -1;
	if(cmd == KBDNOTIFY_LBUTTONUP && outside)
	{
		// Stopped dragging ouside of keyboard area
		m_mouseDownKey = m_dragKey = MPTChord::noNote;
		return 0;
	} else if (cmd == KBDNOTIFY_MOUSEMOVE || outside)
	{
		return 0;
	}

	MPTChord &chord = GetChord();
	const MPTChord::NoteType key = static_cast<MPTChord::NoteType>(nKey) + CHORD_MIN;
	bool update = false;

	if(cmd == KBDNOTIFY_LBUTTONDOWN && m_mouseDownKey == MPTChord::noNote)
	{
		// Initial mouse down
		m_mouseDownKey = key;
		m_dragKey = MPTChord::noNote;
		return 0;
	}
	if(cmd == KBDNOTIFY_LBUTTONDOWN && m_dragKey == MPTChord::noNote && key != m_mouseDownKey)
	{
		// Start dragging
		m_dragKey = m_mouseDownKey;
	}

	// Remove dragged note or toggle
	bool noteIsSet = false;
	for(auto &note : chord.notes)
	{
		if((m_dragKey != MPTChord::noNote && note == m_dragKey)
		   || (m_dragKey == MPTChord::noNote && note == m_mouseDownKey))
		{
			note = MPTChord::noNote;
			noteIsSet = update = true;
			break;
		}
	}

	// Move or toggle note
	if(cmd != KBDNOTIFY_LBUTTONUP || m_dragKey != MPTChord::noNote || !noteIsSet)
	{
		for(auto &note : chord.notes)
		{
			if(note == MPTChord::noNote)
			{
				note = key;
				update = true;
				break;
			}
		}
	}

	if(cmd == KBDNOTIFY_LBUTTONUP)
		m_mouseDownKey = m_dragKey = MPTChord::noNote;
	else
		m_dragKey = key;

	if(update)
	{
		std::sort(chord.notes.begin(), chord.notes.end(), [](MPTChord::NoteType left, MPTChord::NoteType right)
		{
			return (left == MPTChord::noNote)  ? false : (left < right);
		});
		OnChordChanged();
	}
	return 0;
}


void CChordEditor::OnChordChanged()
{
	const MPTChord &chord = GetChord();
	if(chord.key != MPTChord::relativeMode)
		m_CbnBaseNote.SetCurSel(chord.key + 1);
	else
		m_CbnBaseNote.SetCurSel(0);
	for(int i = 0; i < MPTChord::notesPerChord - 1; i++)
	{
		int note = chord.notes[i];
		if(note == MPTChord::noNote)
			note = 0;
		else
			note += 1 - CHORD_MIN;
		m_CbnNote[i].SetCurSel(note);
	}
	UpdateKeyboard();
}


void CChordEditor::UpdateKeyboard()
{
	MPTChord &chord = GetChord();
	const int baseNote = (chord.key == MPTChord::relativeMode) ? 0 : (chord.key % 12);
	for(int i = CHORD_MIN; i < CHORD_MAX; i++)
	{
		uint8 b = CKeyboardControl::KEYFLAG_NORMAL;
		for(const auto note : chord.notes)
		{
			if(i == note)
				b |= CKeyboardControl::KEYFLAG_REDDOT;
		}
		if(i == baseNote)
			b |= CKeyboardControl::KEYFLAG_BRIGHTDOT;
		m_Keyboard.SetFlags(i - CHORD_MIN, b);
	}
	m_Keyboard.InvalidateRect(nullptr, FALSE);
}


void CChordEditor::OnBaseNoteChanged()
{
	MPTChord &chord = GetChord();
	int basenote = static_cast<int>(m_CbnBaseNote.GetItemData(m_CbnBaseNote.GetCurSel()));
	if(basenote != MPTChord::relativeMode)
		basenote -= NOTE_MIN;
	chord.key = (uint8)basenote;
	UpdateKeyboard();
}


void CChordEditor::OnNoteChanged(int noteIndex)
{
	MPTChord &chord = GetChord();
	int note = m_CbnNote[noteIndex].GetCurSel();
	if(note < 0)
		return;
	chord.notes[noteIndex] = static_cast<int8>(m_CbnNote[noteIndex].GetItemData(note));
	UpdateKeyboard();
}


////////////////////////////////////////////////////////////////////////////////////////////
// Keyboard Split Settings (pattern editor)

BEGIN_MESSAGE_MAP(CSplitKeyboardSettings, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_OCTAVEMODIFIER, &CSplitKeyboardSettings::OnOctaveModifierChanged)
END_MESSAGE_MAP()


void CSplitKeyboardSettings::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSplitKeyboadSettings)
	DDX_Control(pDX, IDC_COMBO_SPLITINSTRUMENT, m_CbnSplitInstrument);
	DDX_Control(pDX, IDC_COMBO_SPLITNOTE,       m_CbnSplitNote);
	DDX_Control(pDX, IDC_COMBO_OCTAVEMODIFIER,  m_CbnOctaveModifier);
	DDX_Control(pDX, IDC_COMBO_SPLITVOLUME,     m_CbnSplitVolume);
	//}}AFX_DATA_MAP
}


BOOL CSplitKeyboardSettings::OnInitDialog()
{
	if(sndFile.GetpModDoc() == nullptr)
		return TRUE;

	CDialog::OnInitDialog();

	CString s;

	// Split Notes
	AppendNotesToControl(m_CbnSplitNote, sndFile.GetModSpecifications().noteMin, sndFile.GetModSpecifications().noteMax);
	m_CbnSplitNote.SetCurSel(m_Settings.splitNote - (sndFile.GetModSpecifications().noteMin - NOTE_MIN));

	// Octave modifier
	m_CbnOctaveModifier.SetRedraw(FALSE);
	m_CbnSplitVolume.InitStorage(SplitKeyboardSettings::splitOctaveRange * 2 + 1, 9);
	for(int i = -SplitKeyboardSettings::splitOctaveRange; i < SplitKeyboardSettings::splitOctaveRange + 1; i++)
	{
		s.Format(i < 0 ? _T("Octave -%d") : i > 0 ? _T("Octave +%d") : _T("No Change"), std::abs(i));
		int n = m_CbnOctaveModifier.AddString(s);
		m_CbnOctaveModifier.SetItemData(n, i);
	}
	m_CbnOctaveModifier.SetRedraw(TRUE);
	m_CbnOctaveModifier.SetCurSel(m_Settings.octaveModifier + SplitKeyboardSettings::splitOctaveRange);
	CheckDlgButton(IDC_PATTERN_OCTAVELINK, (m_Settings.octaveLink && m_Settings.octaveModifier != 0) ? BST_CHECKED : BST_UNCHECKED);

	// Volume
	m_CbnSplitVolume.SetRedraw(FALSE);
	m_CbnSplitVolume.InitStorage(65, 4);
	m_CbnSplitVolume.AddString(_T("No Change"));
	m_CbnSplitVolume.SetItemData(0, 0);
	for(int i = 1; i <= 64; i++)
	{
		s.Format(_T("%d"), i);
		int n = m_CbnSplitVolume.AddString(s);
		m_CbnSplitVolume.SetItemData(n, i);
	}
	m_CbnSplitVolume.SetRedraw(TRUE);
	m_CbnSplitVolume.SetCurSel(m_Settings.splitVolume);

	// Instruments
	m_CbnSplitInstrument.SetRedraw(FALSE);
	m_CbnSplitInstrument.InitStorage(1 + (sndFile.GetNumInstruments() ? sndFile.GetNumInstruments() : sndFile.GetNumSamples()), 16);
	m_CbnSplitInstrument.SetItemData(m_CbnSplitInstrument.AddString(_T("No Change")), 0);

	if(sndFile.GetNumInstruments())
	{
		for(INSTRUMENTINDEX nIns = 1; nIns <= sndFile.GetNumInstruments(); nIns++)
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
			if(sndFile.GetSample(nSmp).HasSampleData())
			{
				s.Format(_T("%02d: "), nSmp);
				s += mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.m_szNames[nSmp]);
				int n = m_CbnSplitInstrument.AddString(s);
				m_CbnSplitInstrument.SetItemData(n, nSmp);
			}
		}
	}
	m_CbnSplitInstrument.SetRedraw(TRUE);
	m_CbnSplitInstrument.SetCurSel(m_Settings.splitInstrument);

	return TRUE;
}


void CSplitKeyboardSettings::OnOK()
{
	CDialog::OnOK();

	m_Settings.splitNote = static_cast<ModCommand::NOTE>(m_CbnSplitNote.GetItemData(m_CbnSplitNote.GetCurSel()) - 1);
	m_Settings.octaveModifier = m_CbnOctaveModifier.GetCurSel() - SplitKeyboardSettings::splitOctaveRange;
	m_Settings.octaveLink = (IsDlgButtonChecked(IDC_PATTERN_OCTAVELINK) != BST_UNCHECKED);
	m_Settings.splitVolume = static_cast<ModCommand::VOL>(m_CbnSplitVolume.GetCurSel());
	m_Settings.splitInstrument = static_cast<ModCommand::INSTR>(m_CbnSplitInstrument.GetItemData(m_CbnSplitInstrument.GetCurSel()));
}


void CSplitKeyboardSettings::OnCancel()
{
	CDialog::OnCancel();
}


void CSplitKeyboardSettings::OnOctaveModifierChanged()
{
	CheckDlgButton(IDC_PATTERN_OCTAVELINK, (m_CbnOctaveModifier.GetCurSel() != 9) ? BST_CHECKED : BST_UNCHECKED);
}


/////////////////////////////////////////////////////////////////////////
// Show channel properties from pattern editor

BEGIN_MESSAGE_MAP(QuickChannelProperties, CDialog)
	ON_WM_HSCROLL()		// Sliders
	ON_WM_ACTIVATE()	// Catch Window focus change
	ON_EN_UPDATE(IDC_EDIT1,	&QuickChannelProperties::OnVolChanged)
	ON_EN_UPDATE(IDC_EDIT2,	&QuickChannelProperties::OnPanChanged)
	ON_EN_UPDATE(IDC_EDIT3,	&QuickChannelProperties::OnNameChanged)
	ON_COMMAND(IDC_CHECK1,	&QuickChannelProperties::OnMuteChanged)
	ON_COMMAND(IDC_CHECK2,	&QuickChannelProperties::OnSurroundChanged)
	ON_COMMAND(IDC_BUTTON1,	&QuickChannelProperties::OnPrevChannel)
	ON_COMMAND(IDC_BUTTON2,	&QuickChannelProperties::OnNextChannel)
	ON_COMMAND(IDC_BUTTON3,	&QuickChannelProperties::OnChangeColor)
	ON_COMMAND(IDC_BUTTON4,	&QuickChannelProperties::OnChangeColor)
	ON_COMMAND(IDC_BUTTON5, &QuickChannelProperties::OnPickPrevColor)
	ON_COMMAND(IDC_BUTTON6, &QuickChannelProperties::OnPickNextColor)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,	&QuickChannelProperties::OnCustomKeyMsg)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xFFFF, &QuickChannelProperties::OnToolTipText)
END_MESSAGE_MAP()


void QuickChannelProperties::DoDataExchange(CDataExchange *pDX)
{
	DDX_Control(pDX, IDC_SLIDER1, m_volSlider);
	DDX_Control(pDX, IDC_SLIDER2, m_panSlider);
	DDX_Control(pDX, IDC_SPIN1,   m_volSpin);
	DDX_Control(pDX, IDC_SPIN2,   m_panSpin);
	DDX_Control(pDX, IDC_EDIT3,   m_nameEdit);
}


QuickChannelProperties::~QuickChannelProperties()
{
	DestroyWindow();
}


void QuickChannelProperties::OnActivate(UINT nState, CWnd *, BOOL)
{
	if(nState == WA_INACTIVE && !m_settingColor)
	{
		// Hide window when changing focus to another window.
		m_visible = false;
		ShowWindow(SW_HIDE);
	}
}


// Show channel properties for a given channel at a given screen position.
void QuickChannelProperties::Show(CModDoc *modDoc, CHANNELINDEX chn, CPoint position)
{
	if(!m_hWnd)
	{
		Create(IDD_CHANNELSETTINGS, nullptr);
		EnableToolTips();
		m_colorBtn.SubclassDlgItem(IDC_BUTTON4, this);
		m_colorBtnPrev.SubclassDlgItem(IDC_BUTTON5, this);
		m_colorBtnNext.SubclassDlgItem(IDC_BUTTON6, this);

		m_volSlider.SetRange(0, 64);
		m_volSlider.SetTicFreq(8);
		m_volSpin.SetRange(0, 64);

		m_panSlider.SetRange(0, 64);
		m_panSlider.SetTicFreq(8);
		m_panSpin.SetRange(0, 256);

		m_nameEdit.SetFocus();
	}
	m_document = modDoc;
	m_channel = chn;

	SetParent(nullptr);

	// Center window around point where user clicked.
	CRect rect, screenRect;
	GetWindowRect(rect);
	::GetWindowRect(::GetDesktopWindow(), &screenRect);
	rect.MoveToXY(
	    Clamp(static_cast<int>(position.x) - rect.Width() / 2, 0, static_cast<int>(screenRect.right) - rect.Width()),
	    Clamp(static_cast<int>(position.y) - rect.Height() / 2, 0, static_cast<int>(screenRect.bottom) - rect.Height()));
	MoveWindow(rect);

	SetWindowText(MPT_TFORMAT("Settings for Channel {}")(chn + 1).c_str());

	UpdateDisplay();

	const BOOL enablePan = (m_document->GetModType() & (MOD_TYPE_XM | MOD_TYPE_MOD)) ? FALSE : TRUE;
	const BOOL itOnly = (m_document->GetModType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) ? TRUE : FALSE;

	// Volume controls
	m_volSlider.EnableWindow(itOnly);
	m_volSpin.EnableWindow(itOnly);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT1), itOnly);

	// Pan controls
	m_panSlider.EnableWindow(enablePan);
	m_panSpin.EnableWindow(enablePan);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT2), enablePan);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK2), itOnly);

	// Channel name
	m_nameEdit.EnableWindow((m_document->GetModType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM)) ? TRUE : FALSE);

	ShowWindow(SW_SHOW);
	m_visible = true;
}


void QuickChannelProperties::UpdateDisplay()
{
	// Set up channel properties
	m_visible = false;
	const ModChannelSettings &settings = m_document->GetSoundFile().ChnSettings[m_channel];
	SetDlgItemInt(IDC_EDIT1, settings.nVolume, FALSE);
	SetDlgItemInt(IDC_EDIT2, settings.nPan, FALSE);
	m_volSlider.SetPos(settings.nVolume);
	m_panSlider.SetPos(settings.nPan / 4u);
	CheckDlgButton(IDC_CHECK1, (settings.dwFlags[CHN_MUTE]) ? TRUE : FALSE);
	CheckDlgButton(IDC_CHECK2, (settings.dwFlags[CHN_SURROUND]) ? TRUE : FALSE);

	TCHAR description[16];
	wsprintf(description, _T("Channel %d:"), m_channel + 1);
	SetDlgItemText(IDC_STATIC_CHANNEL_NAME, description);
	m_nameEdit.LimitText(MAX_CHANNELNAME - 1);
	m_nameEdit.SetWindowText(mpt::ToCString(m_document->GetSoundFile().GetCharsetInternal(), settings.szName));

	const bool isFirst = (m_channel <= 0), isLast = (m_channel >= m_document->GetNumChannels() - 1);

	m_colorBtn.SetColor(settings.color);
	m_colorBtnPrev.EnableWindow(isFirst ? FALSE : TRUE);
	if(!isFirst)
		m_colorBtnPrev.SetColor(m_document->GetSoundFile().ChnSettings[m_channel - 1].color);
	m_colorBtnNext.EnableWindow(isLast ? FALSE : TRUE);
	if(!isLast)
		m_colorBtnNext.SetColor(m_document->GetSoundFile().ChnSettings[m_channel + 1].color);

	m_settingsChanged = false;
	m_visible = true;

	::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1), isFirst ? FALSE : TRUE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON2), isLast ? FALSE : TRUE);
}

void QuickChannelProperties::PrepareUndo()
{
	if(!m_settingsChanged)
	{
		// Backup old channel settings through pattern undo.
		m_settingsChanged = true;
		m_document->GetPatternUndo().PrepareChannelUndo(m_channel, 1, "Channel Settings");
	}
}


void QuickChannelProperties::OnVolChanged()
{
	if(!m_visible)
	{
		return;
	}

	uint16 volume = static_cast<uint16>(GetDlgItemInt(IDC_EDIT1));
	if(volume >= 0 && volume <= 64)
	{
		PrepareUndo();
		m_document->SetChannelGlobalVolume(m_channel, volume);
		m_volSlider.SetPos(volume);
		m_document->UpdateAllViews(nullptr, GeneralHint(m_channel).Channels(), this);
	}
}


void QuickChannelProperties::OnPanChanged()
{
	if(!m_visible)
	{
		return;
	}

	uint16 panning = static_cast<uint16>(GetDlgItemInt(IDC_EDIT2));
	if(panning >= 0 && panning <= 256)
	{
		PrepareUndo();
		m_document->SetChannelDefaultPan(m_channel, panning);
		m_panSlider.SetPos(panning / 4u);
		m_document->UpdateAllViews(nullptr, GeneralHint(m_channel).Channels(), this);
		// Surround is forced off when changing pan, so uncheck the checkbox.
		CheckDlgButton(IDC_CHECK2, BST_UNCHECKED);
	}
}


void QuickChannelProperties::OnHScroll(UINT, UINT, CScrollBar *bar)
{
	if(!m_visible)
	{
		return;
	}

	bool update = false;

	// Volume slider
	if(bar == reinterpret_cast<CScrollBar *>(&m_volSlider))
	{
		uint16 pos = static_cast<uint16>(m_volSlider.GetPos());
		PrepareUndo();
		if(m_document->SetChannelGlobalVolume(m_channel, pos))
		{
			SetDlgItemInt(IDC_EDIT1, pos);
			update = true;
		}
	}
	// Pan slider
	if(bar == reinterpret_cast<CScrollBar *>(&m_panSlider))
	{
		uint16 pos = static_cast<uint16>(m_panSlider.GetPos());
		PrepareUndo();
		if(m_document->SetChannelDefaultPan(m_channel, pos * 4u))
		{
			SetDlgItemInt(IDC_EDIT2, pos * 4u);
			CheckDlgButton(IDC_CHECK2, BST_UNCHECKED);
			update = true;
		}
	}

	if(update)
	{
		m_document->UpdateAllViews(nullptr, GeneralHint(m_channel).Channels(), this);
	}
}


void QuickChannelProperties::OnMuteChanged()
{
	if(!m_visible)
	{
		return;
	}

	m_document->MuteChannel(m_channel, IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED);
	m_document->UpdateAllViews(nullptr, GeneralHint(m_channel).Channels(), this);
}


void QuickChannelProperties::OnSurroundChanged()
{
	if(!m_visible)
	{
		return;
	}

	PrepareUndo();
	m_document->SurroundChannel(m_channel, IsDlgButtonChecked(IDC_CHECK2) != BST_UNCHECKED);
	m_document->UpdateAllViews(nullptr, GeneralHint(m_channel).Channels(), this);
	UpdateDisplay();
}


void QuickChannelProperties::OnNameChanged()
{
	if(!m_visible)
	{
		return;
	}

	ModChannelSettings &settings = m_document->GetSoundFile().ChnSettings[m_channel];
	CString newNameTmp;
	m_nameEdit.GetWindowText(newNameTmp);
	std::string newName = mpt::ToCharset(m_document->GetSoundFile().GetCharsetInternal(), newNameTmp);

	if(newName != settings.szName)
	{
		PrepareUndo();
		settings.szName = newName;
		m_document->SetModified();
		m_document->UpdateAllViews(nullptr, GeneralHint(m_channel).Channels(), this);
	}
}


void QuickChannelProperties::OnChangeColor()
{
	m_settingColor = true;
	if(auto color = m_colorBtn.PickColor(m_document->GetSoundFile(), m_channel); color.has_value())
	{
		PrepareUndo();
		m_document->GetSoundFile().ChnSettings[m_channel].color = *color;
		if(m_document->SupportsChannelColors())
			m_document->SetModified();
		m_document->UpdateAllViews(nullptr, GeneralHint(m_channel).Channels(), this);
	}
	m_settingColor = false;
}


void QuickChannelProperties::OnPickPrevColor()
{
	if(m_channel > 0)
		PickColorFromChannel(m_channel - 1);
}


void QuickChannelProperties::OnPickNextColor()
{
	if(m_channel < m_document->GetNumChannels() - 1)
		PickColorFromChannel(m_channel + 1);
}


void QuickChannelProperties::PickColorFromChannel(CHANNELINDEX channel)
{
	auto &channels = m_document->GetSoundFile().ChnSettings;
	if(channels[channel].color != channels[m_channel].color)
	{
		PrepareUndo();
		channels[m_channel].color = channels[channel].color;
		m_colorBtn.SetColor(channels[m_channel].color);
		if(m_document->SupportsChannelColors())
			m_document->SetModified();
		m_document->UpdateAllViews(nullptr, GeneralHint(m_channel).Channels(), this);
	}
}


void QuickChannelProperties::OnPrevChannel()
{
	if(m_channel > 0)
	{
		m_channel--;
		UpdateDisplay();
	}
}


void QuickChannelProperties::OnNextChannel()
{
	if(m_channel < m_document->GetNumChannels() - 1)
	{
		m_channel++;
		UpdateDisplay();
	}
}


BOOL QuickChannelProperties::PreTranslateMessage(MSG *pMsg)
{
	if(pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if((pMsg->message == WM_SYSKEYUP) || (pMsg->message == WM_KEYUP) ||
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler *ih = CMainFrame::GetInputHandler();

			//Translate message manually
			UINT nChar = static_cast<UINT>(pMsg->wParam);
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);

			if(ih->KeyEvent(kCtxChannelSettings, nChar, nRepCnt, nFlags, kT, this) != kcNull)
			{
				return TRUE;  // Mapped to a command, no need to pass message on.
			}
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}


LRESULT QuickChannelProperties::OnCustomKeyMsg(WPARAM wParam, LPARAM)
{
	switch(wParam)
	{
	case kcChnSettingsPrev:
		OnPrevChannel();
		return wParam;
	case kcChnSettingsNext:
		OnNextChannel();
		return wParam;
	case kcChnColorFromPrev:
		OnPickPrevColor();
		return wParam;
	case kcChnColorFromNext:
		OnPickNextColor();
		return wParam;
	case kcChnSettingsClose:
		OnActivate(WA_INACTIVE, nullptr, FALSE);
		return wParam;
	}

	return kcNull;
}


BOOL QuickChannelProperties::OnToolTipText(UINT, NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pTTT = reinterpret_cast<TOOLTIPTEXT *>(pNMHDR);
	UINT_PTR id = pNMHDR->idFrom;
	if(pTTT->uFlags & TTF_IDISHWND)
	{
		// idFrom is actually the HWND of the tool
		id = static_cast<UINT_PTR>(::GetDlgCtrlID(reinterpret_cast<HWND>(id)));
	}

	mpt::tstring text;
	CommandID cmd = kcNull;
	switch (id)
	{
	case IDC_EDIT1:
	case IDC_SLIDER1:
		text = CModDoc::LinearToDecibels(m_document->GetSoundFile().ChnSettings[m_channel].nVolume, 64.0);
		break;
	case IDC_EDIT2:
	case IDC_SLIDER2:
		text = CModDoc::PanningToString(m_document->GetSoundFile().ChnSettings[m_channel].nPan, 128);
		break;
	case IDC_BUTTON1:
		text = _T("Previous Channel");
		cmd = kcChnSettingsPrev;
		break;
	case IDC_BUTTON2:
		text = _T("Next Channel");
		cmd = kcChnSettingsNext;
		break;
	case IDC_BUTTON5:
		text = _T("Take color from previous channel");
		cmd = kcChnColorFromPrev;
		break;
	case IDC_BUTTON6:
		text = _T("Take color from next channel");
		cmd = kcChnColorFromNext;
		break;
	default:
		return FALSE;
	}
	
	if(cmd != kcNull)
	{
		auto keyText = CMainFrame::GetInputHandler()->m_activeCommandSet->GetKeyTextFromCommand(cmd, 0);
		if(!keyText.IsEmpty())
			text += MPT_TFORMAT(" ({})")(keyText);
	}

	mpt::String::WriteWinBuf(pTTT->szText) = text;
	*pResult = 0;

	// bring the tooltip window above other popup windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER);

	return TRUE;  // message was handled
}


OPENMPT_NAMESPACE_END
