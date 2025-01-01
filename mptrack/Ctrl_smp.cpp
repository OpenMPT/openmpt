/*
 * Ctrl_smp.cpp
 * ------------
 * Purpose: Sample tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Ctrl_smp.h"
#include "Autotune.h"
#include "Childfrm.h"
#include "dlg_misc.h"
#include "FileDialog.h"
#include "Globals.h"
#include "HighDPISupport.h"
#include "ImageLists.h"
#include "InputHandler.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Mptrack.h"
#include "ProgressDialog.h"
#include "PSRatioCalc.h"
#include "Reporting.h"
#include "resource.h"
#include "SampleEditorDialogs.h"
#include "TrackerSettings.h"
#include "View_smp.h"
#include "WindowMessages.h"

#include "../common/mptStringBuffer.h"
#include "../common/mptFileIO.h"
#include "../common/FileReader.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/SampleCopy.h"
#include "../tracklib/SampleEdit.h"
#include "../tracklib/TimeStretchPitchShift.h"
#include "mpt/io_file/inputfile.hpp"
#include "mpt/io_file_read/inputfile_filecursor.hpp"
#include "mpt/io_file/outputfile.hpp"
#include "mpt/parse/parse.hpp"
#include "mpt/string/utility.hpp"
#include "openmpt/soundbase/Copy.hpp"


OPENMPT_NAMESPACE_BEGIN

inline constexpr int32 BASENOTE_MIN = (1 * 12);        // C-1
inline constexpr int32 BASENOTE_MAX = (10 * 12 + 11);  // B-10

static int32 GetSampleTransponse(const ModSample &sample)
{
	return mpt::saturate_round<int32>(std::log(sample.nC5Speed * (1.0 / 8363.0)) * (12.0 / mpt::numbers::ln2));
}


BEGIN_MESSAGE_MAP(CCtrlSamples, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlSamples)
	ON_WM_VSCROLL()
	ON_WM_XBUTTONUP()
	ON_NOTIFY(TBN_DROPDOWN, IDC_TOOLBAR1, &CCtrlSamples::OnTbnDropDownToolBar)
	ON_COMMAND(IDC_SAMPLE_NEW,			&CCtrlSamples::OnSampleNew)
	ON_COMMAND(IDC_SAMPLE_DUPLICATE,	&CCtrlSamples::OnSampleDuplicate)
	ON_COMMAND(IDC_SAMPLE_OPEN,			&CCtrlSamples::OnSampleOpen)
	ON_COMMAND(IDC_SAMPLE_OPENKNOWN,	&CCtrlSamples::OnSampleOpenKnown)
	ON_COMMAND(IDC_SAMPLE_OPENRAW,		&CCtrlSamples::OnSampleOpenRaw)
	ON_COMMAND(IDC_SAMPLE_SAVEAS,		&CCtrlSamples::OnSampleSave)
	ON_COMMAND(IDC_SAVE_ONE,			&CCtrlSamples::OnSampleSaveOne)
	ON_COMMAND(IDC_SAVE_ALL,			&CCtrlSamples::OnSampleSaveAll)
	ON_COMMAND(IDC_SAMPLE_PLAY,			&CCtrlSamples::OnSamplePlay)
	ON_COMMAND(IDC_SAMPLE_NORMALIZE,	&CCtrlSamples::OnNormalize)
	ON_COMMAND(IDC_SAMPLE_AMPLIFY,		&CCtrlSamples::OnAmplify)
	ON_COMMAND(IDC_SAMPLE_RESAMPLE,		&CCtrlSamples::OnResample)
	ON_COMMAND(IDC_SAMPLE_REVERSE,		&CCtrlSamples::OnReverse)
	ON_COMMAND(IDC_SAMPLE_SILENCE,		&CCtrlSamples::OnSilence)
	ON_COMMAND(IDC_SAMPLE_INVERT,		&CCtrlSamples::OnInvert)
	ON_COMMAND(IDC_SAMPLE_SIGN_UNSIGN,	&CCtrlSamples::OnSignUnSign)
	ON_COMMAND(IDC_SAMPLE_DCOFFSET,		&CCtrlSamples::OnRemoveDCOffset)
	ON_COMMAND(IDC_SAMPLE_XFADE,		&CCtrlSamples::OnXFade)
	ON_COMMAND(IDC_SAMPLE_STEREOSEPARATION, &CCtrlSamples::OnStereoSeparation)
	ON_COMMAND(IDC_SAMPLE_AUTOTUNE,		&CCtrlSamples::OnAutotune)
	ON_COMMAND(IDC_CHECK1,				&CCtrlSamples::OnSetPanningChanged)
	ON_COMMAND(IDC_CHECK2,				&CCtrlSamples::OnKeepSampleOnDisk)
	ON_COMMAND(ID_PREVINSTRUMENT,		&CCtrlSamples::OnPrevInstrument)
	ON_COMMAND(ID_NEXTINSTRUMENT,		&CCtrlSamples::OnNextInstrument)
	ON_COMMAND(IDC_BUTTON1,				&CCtrlSamples::OnPitchShiftTimeStretch)
	ON_COMMAND(IDC_BUTTON2,				&CCtrlSamples::OnEstimateSampleSize)
	ON_COMMAND(IDC_CHECK3,				&CCtrlSamples::OnToggleTimestretchQuality)
	ON_COMMAND(IDC_SAMPLE_INITOPL,		&CCtrlSamples::OnInitOPLInstrument)
	
	ON_EN_CHANGE(IDC_SAMPLE_NAME,		&CCtrlSamples::OnNameChanged)
	ON_EN_CHANGE(IDC_SAMPLE_FILENAME,	&CCtrlSamples::OnFileNameChanged)
	ON_EN_CHANGE(IDC_EDIT_SAMPLE,		&CCtrlSamples::OnSampleChanged)
	ON_EN_CHANGE(IDC_EDIT1,				&CCtrlSamples::OnLoopPointsChanged)
	ON_EN_CHANGE(IDC_EDIT2,				&CCtrlSamples::OnLoopPointsChanged)
	ON_EN_CHANGE(IDC_EDIT3,				&CCtrlSamples::OnSustainPointsChanged)
	ON_EN_CHANGE(IDC_EDIT4,				&CCtrlSamples::OnSustainPointsChanged)
	ON_EN_CHANGE(IDC_EDIT5,				&CCtrlSamples::OnFineTuneChanged)
	ON_EN_CHANGE(IDC_EDIT7,				&CCtrlSamples::OnVolumeChanged)
	ON_EN_CHANGE(IDC_EDIT8,				&CCtrlSamples::OnGlobalVolChanged)
	ON_EN_CHANGE(IDC_EDIT9,				&CCtrlSamples::OnPanningChanged)
	ON_EN_CHANGE(IDC_EDIT14,			&CCtrlSamples::OnVibSweepChanged)
	ON_EN_CHANGE(IDC_EDIT15,			&CCtrlSamples::OnVibDepthChanged)
	ON_EN_CHANGE(IDC_EDIT16,			&CCtrlSamples::OnVibRateChanged)

	ON_EN_SETFOCUS(IDC_SAMPLE_NAME,		&CCtrlSamples::OnEditFocus)
	ON_EN_SETFOCUS(IDC_SAMPLE_FILENAME, &CCtrlSamples::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT1,			&CCtrlSamples::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT2,			&CCtrlSamples::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT3,			&CCtrlSamples::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT4,			&CCtrlSamples::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT5,			&CCtrlSamples::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT7,			&CCtrlSamples::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT8,			&CCtrlSamples::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT9,			&CCtrlSamples::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT14,			&CCtrlSamples::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT15,			&CCtrlSamples::OnEditFocus)
	ON_EN_SETFOCUS(IDC_EDIT16,			&CCtrlSamples::OnEditFocus)

	ON_EN_KILLFOCUS(IDC_EDIT5,			&CCtrlSamples::OnFineTuneChangedDone)

	ON_CBN_SELCHANGE(IDC_COMBO_BASENOTE,&CCtrlSamples::OnBaseNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_ZOOM,	&CCtrlSamples::OnZoomChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1,		&CCtrlSamples::OnLoopTypeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,		&CCtrlSamples::OnSustainTypeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,		&CCtrlSamples::OnVibTypeChanged)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,		&CCtrlSamples::OnCustomKeyMsg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CCtrlSamples::DoDataExchange(CDataExchange* pDX)
{
	CModControlDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCtrlSamples)
	DDX_Control(pDX, IDC_TOOLBAR1, m_ToolBar1);
	DDX_Control(pDX, IDC_TOOLBAR2, m_ToolBar2);
	DDX_Control(pDX, IDC_SAMPLE_NAME, m_EditName);
	DDX_Control(pDX, IDC_SAMPLE_FILENAME, m_EditFileName);
	DDX_Control(pDX, IDC_SAMPLE_NAME, m_EditName);
	DDX_Control(pDX, IDC_SAMPLE_FILENAME, m_EditFileName);
	DDX_Control(pDX, IDC_COMBO_ZOOM, m_ComboZoom);
	DDX_Control(pDX, IDC_COMBO_BASENOTE, m_CbnBaseNote);
	DDX_Control(pDX, IDC_SPIN_SAMPLE, m_SpinSample);
	DDX_Control(pDX, IDC_EDIT_SAMPLE, m_EditSample);
	DDX_Control(pDX, IDC_CHECK1, m_CheckPanning);
	DDX_Control(pDX, IDC_SPIN1, m_SpinLoopStart);
	DDX_Control(pDX, IDC_SPIN2, m_SpinLoopEnd);
	DDX_Control(pDX, IDC_SPIN3, m_SpinSustainStart);
	DDX_Control(pDX, IDC_SPIN4, m_SpinSustainEnd);
	DDX_Control(pDX, IDC_SPIN5, m_SpinFineTune);
	DDX_Control(pDX, IDC_SPIN7, m_SpinVolume);
	DDX_Control(pDX, IDC_SPIN8, m_SpinGlobalVol);
	DDX_Control(pDX, IDC_SPIN9, m_SpinPanning);
	DDX_Control(pDX, IDC_SPIN11, m_SpinVibSweep);
	DDX_Control(pDX, IDC_SPIN12, m_SpinVibDepth);
	DDX_Control(pDX, IDC_SPIN13, m_SpinVibRate);
	DDX_Control(pDX, IDC_SPIN16, m_SpinTimeStretchRatio);
	DDX_Control(pDX, IDC_COMBO1, m_ComboLoopType);
	DDX_Control(pDX, IDC_COMBO2, m_ComboSustainType);
	DDX_Control(pDX, IDC_COMBO3, m_ComboAutoVib);
	DDX_Control(pDX, IDC_COMBO5, m_ComboGrainSize);
	DDX_Control(pDX, IDC_EDIT1, m_EditLoopStart);
	DDX_Control(pDX, IDC_EDIT2, m_EditLoopEnd);
	DDX_Control(pDX, IDC_EDIT3, m_EditSustainStart);
	DDX_Control(pDX, IDC_EDIT4, m_EditSustainEnd);
	DDX_Control(pDX, IDC_EDIT5, m_EditFineTune);
	DDX_Control(pDX, IDC_EDIT7, m_EditVolume);
	DDX_Control(pDX, IDC_EDIT8, m_EditGlobalVol);
	DDX_Control(pDX, IDC_EDIT9, m_EditPanning);
	DDX_Control(pDX, IDC_EDIT14, m_EditVibSweep);
	DDX_Control(pDX, IDC_EDIT15, m_EditVibDepth);
	DDX_Control(pDX, IDC_EDIT16, m_EditVibRate);
	DDX_Control(pDX, IDC_COMBO4, m_ComboPitch);
	//}}AFX_DATA_MAP
}


CCtrlSamples::CCtrlSamples(CModControlView &parent, CModDoc &document)
	: CModControlDlg(parent, document)
{
	m_nLockCount = 1;
	m_EditTimeStretchRatio.SetAccessibleSuffix(_T("%"));
}



CCtrlSamples::~CCtrlSamples()
{
}



CRuntimeClass *CCtrlSamples::GetAssociatedViewClass()
{
	return RUNTIME_CLASS(CViewSample);
}


void CCtrlSamples::OnEditFocus()
{
	m_startedEdit = false;
}


BOOL CCtrlSamples::OnInitDialog()
{
	CModControlDlg::OnInitDialog();
	m_initialized = false;
	SetRedraw(FALSE);

	// Zoom Selection
	static constexpr std::pair<const TCHAR *, int> ZoomLevels[] =
	{
		{_T("Auto"),  0},
		{_T("1:1"),   1},
		{_T("2:1"),  -2},
		{_T("4:1"),  -3},
		{_T("8:1"),  -4},
		{_T("16:1"), -5},
		{_T("32:1"), -6},
		{_T("1:2"),   2},
		{_T("1:4"),   3},
		{_T("1:8"),   4},
		{_T("1:16"),  5},
		{_T("1:32"),  6},
		{_T("1:64"),  7},
		{_T("1:128"), 8},
		{_T("1:256"), 9},
		{_T("1:512"), 10},
	};
	m_ComboZoom.SetRedraw(FALSE);
	m_ComboZoom.InitStorage(static_cast<int>(std::size(ZoomLevels)), 4);
	for(const auto &[str, data] : ZoomLevels)
	{
		m_ComboZoom.SetItemData(m_ComboZoom.AddString(str), static_cast<DWORD_PTR>(data));
	}
	m_ComboZoom.SetRedraw(TRUE);
	m_ComboZoom.SetCurSel(0);

	// File ToolBar
	m_ToolBar1.SetExtendedStyle(m_ToolBar1.GetExtendedStyle() | TBSTYLE_EX_DRAWDDARROWS);
	m_ToolBar1.Init(CMainFrame::GetMainFrame()->m_PatternIcons,CMainFrame::GetMainFrame()->m_PatternIconsDisabled);
	m_ToolBar1.AddButton(IDC_SAMPLE_NEW, TIMAGE_SAMPLE_NEW, TBSTYLE_BUTTON | TBSTYLE_DROPDOWN);
	m_ToolBar1.AddButton(IDC_SAMPLE_OPEN, TIMAGE_OPEN, TBSTYLE_BUTTON | TBSTYLE_DROPDOWN);
	m_ToolBar1.AddButton(IDC_SAMPLE_SAVEAS, TIMAGE_SAVE, TBSTYLE_BUTTON | TBSTYLE_DROPDOWN);
	// Edit ToolBar
	m_ToolBar2.Init(CMainFrame::GetMainFrame()->m_PatternIcons,CMainFrame::GetMainFrame()->m_PatternIconsDisabled);
	m_ToolBar2.AddButton(IDC_SAMPLE_PLAY, TIMAGE_PREVIEW);
	m_ToolBar2.AddButton(IDC_SAMPLE_NORMALIZE, TIMAGE_SAMPLE_NORMALIZE);
	m_ToolBar2.AddButton(IDC_SAMPLE_AMPLIFY, TIMAGE_SAMPLE_AMPLIFY);
	m_ToolBar2.AddButton(IDC_SAMPLE_DCOFFSET, TIMAGE_SAMPLE_DCOFFSET);
	m_ToolBar2.AddButton(IDC_SAMPLE_STEREOSEPARATION, TIMAGE_SAMPLE_STEREOSEP);
	m_ToolBar2.AddButton(IDC_SAMPLE_RESAMPLE, TIMAGE_SAMPLE_RESAMPLE);
	m_ToolBar2.AddButton(IDC_SAMPLE_REVERSE, TIMAGE_SAMPLE_REVERSE);
	m_ToolBar2.AddButton(IDC_SAMPLE_SILENCE, TIMAGE_SAMPLE_SILENCE);
	m_ToolBar2.AddButton(IDC_SAMPLE_INVERT, TIMAGE_SAMPLE_INVERT);
	m_ToolBar2.AddButton(IDC_SAMPLE_SIGN_UNSIGN, TIMAGE_SAMPLE_UNSIGN);
	m_ToolBar2.AddButton(IDC_SAMPLE_XFADE, TIMAGE_SAMPLE_FIXLOOP);
	m_ToolBar2.AddButton(IDC_SAMPLE_AUTOTUNE, TIMAGE_SAMPLE_AUTOTUNE);
	// Setup Controls
	m_SpinVolume.SetRange(0, 64);
	m_SpinGlobalVol.SetRange(0, 64);

	m_CbnBaseNote.InitStorage(BASENOTE_MAX - BASENOTE_MIN, 4);
	m_CbnBaseNote.SetRedraw(FALSE);
	for(ModCommand::NOTE i = BASENOTE_MIN; i <= BASENOTE_MAX; i++)
	{
		CString noteName = mpt::ToCString(CSoundFile::GetDefaultNoteName(i % 12)) + mpt::cfmt::val(i / 12);
		m_CbnBaseNote.SetItemData(m_CbnBaseNote.AddString(noteName), i - (NOTE_MIDDLEC - NOTE_MIN));
	}
	m_CbnBaseNote.SetRedraw(TRUE);

	// Pitch selection
	// Allow pitch from -12 (1 octave down) to +12 (1 octave up)
	m_ComboPitch.InitStorage(25, 4);
	m_ComboPitch.SetRedraw(FALSE);
	for(int i = -12 ; i <= 12 ; i++)
	{
		mpt::tstring str;
		if(i == 0)
			str = _T("unchanged");
		else if(i < 0)
			str = mpt::tfmt::dec(i);
		else
			str = _T("+") + mpt::tfmt::dec(i);
		m_ComboPitch.SetItemData(m_ComboPitch.AddString(str.c_str()), i + 12);
	}
	m_ComboPitch.SetRedraw(TRUE);
	// Set "unchanged" as default pitch
	m_ComboPitch.SetCurSel(12);
	m_ComboGrainSize.SetCurSel(3);
	OnToggleTimestretchQuality();
	m_EditTimeStretchRatio.SubclassDlgItem(IDC_EDIT6, this);
	m_EditTimeStretchRatio.AllowNegative(false);
	m_EditTimeStretchRatio.SetDecimalValue(100.0);
	m_SpinTimeStretchRatio.SetRange32(1, 100000);

	SetRedraw(TRUE);
	return TRUE;
}


void CCtrlSamples::OnDPIChanged()
{
	m_ToolBar1.OnDPIChanged();
	m_ToolBar2.OnDPIChanged();
	CModControlDlg::OnDPIChanged();
}


Setting<LONG> &CCtrlSamples::GetSplitPosRef() { return TrackerSettings::Instance().glSampleWindowHeight; }


void CCtrlSamples::RecalcLayout()
{
}


bool CCtrlSamples::SetCurrentSample(SAMPLEINDEX nSmp, LONG lZoom, bool bUpdNum)
{
	if(m_sndFile.GetNumSamples() < 1)
		m_sndFile.m_nSamples = 1;
	if((nSmp < 1) || (nSmp > m_sndFile.GetNumSamples()))
		return FALSE;

	LockControls();
	if(m_nSample != nSmp)
	{
		m_nSample = nSmp;
		UpdateView(SampleHint(m_nSample).Info());
		m_parent.SampleChanged(m_nSample);
	}
	if(bUpdNum)
	{
		SetDlgItemInt(IDC_EDIT_SAMPLE, m_nSample);
		m_SpinSample.SetRange(1, m_sndFile.GetNumSamples());
	}
	if(lZoom == -1)
	{
		lZoom = static_cast<int>(m_ComboZoom.GetItemData(m_ComboZoom.GetCurSel()));
	} else
	{
		for(int i = 0; i< m_ComboZoom.GetCount(); i++)
		{
			if(static_cast<int>(m_ComboZoom.GetItemData(i)) == lZoom)
			{
				m_ComboZoom.SetCurSel(i);
				break;
			}
		}
	}
	static_assert(MAX_SAMPLES < uint16_max);
	SendViewMessage(VIEWMSG_SETCURRENTSAMPLE, (lZoom << 16) | m_nSample);
	UnlockControls();
	return true;
}


void CCtrlSamples::OnActivatePage(LPARAM lParam)
{
	if (lParam < 0)
	{
		int nIns = m_parent.GetInstrumentChange();
		if (m_sndFile.GetNumInstruments())
		{
			if ((nIns > 0) && (!m_modDoc.IsChildSample((INSTRUMENTINDEX)nIns, m_nSample)))
			{
				SAMPLEINDEX k = m_modDoc.FindInstrumentChild((INSTRUMENTINDEX)nIns);
				if (k > 0) lParam = k;
			}
		} else
		{
			if (nIns > 0) lParam = nIns;
		}
	} else if (lParam > 0)
	{
		if (m_sndFile.GetNumInstruments())
		{
			INSTRUMENTINDEX k = (INSTRUMENTINDEX)m_parent.GetInstrumentChange();
			if (!m_modDoc.IsChildSample(k, (SAMPLEINDEX)lParam))
			{
				INSTRUMENTINDEX nins = m_modDoc.FindSampleParent((SAMPLEINDEX)lParam);
				if(nins != INSTRUMENTINDEX_INVALID)
				{
					m_parent.InstrumentChanged(nins);
				}
			}
		} else
		{
			m_parent.InstrumentChanged(static_cast<int>(lParam));
		}
	}

	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	SampleViewState &sampleState = pFrame->GetSampleViewState();
	if(sampleState.initialSample != 0)
	{
		m_nSample = sampleState.initialSample;
		sampleState.initialSample = 0;
	}

	SetCurrentSample((lParam > 0) ? ((SAMPLEINDEX)lParam) : m_nSample);

	// Initial Update
	if(!m_initialized)
		UpdateView(SampleHint(m_nSample).Info().ModType(), nullptr);
	if(m_hWndView)
		PostViewMessage(VIEWMSG_LOADSTATE, (LPARAM)&sampleState);
	SwitchToView();

	// Combo boxes randomly disappear without this... why?
	Invalidate();
}


void CCtrlSamples::OnDeactivatePage()
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if ((pFrame) && (m_hWndView)) SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&pFrame->GetSampleViewState());
	m_modDoc.NoteOff(0, true);
}


LRESULT CCtrlSamples::OnModCtrlMsg(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
	case CTRLMSG_GETCURRENTINSTRUMENT:
		return m_nSample;
		break;

	case CTRLMSG_SMP_PREVINSTRUMENT:
		OnPrevInstrument();
		break;

	case CTRLMSG_SMP_NEXTINSTRUMENT:
		OnNextInstrument();
		break;

	case CTRLMSG_SMP_OPENFILE:
		if(lParam)
			return OpenSample(*reinterpret_cast<const mpt::PathString *>(lParam));
		break;

	case CTRLMSG_SMP_SONGDROP:
		if(lParam)
		{
			const auto &dropInfo = *reinterpret_cast<const DRAGONDROP *>(lParam);
			if(dropInfo.sndFile)
				return OpenSample(*dropInfo.sndFile, static_cast<SAMPLEINDEX>(dropInfo.dropItem)) ? TRUE : FALSE;
		}
		break;

	case CTRLMSG_SMP_SETZOOM:
		SetCurrentSample(m_nSample, static_cast<int>(lParam), FALSE);
		break;

	case CTRLMSG_SETCURRENTINSTRUMENT:
		SetCurrentSample((SAMPLEINDEX)lParam, -1, TRUE);
		break;

	case CTRLMSG_SMP_INITOPL:
		OnInitOPLInstrument();
		break;

	case CTRLMSG_SMP_NEWSAMPLE:
		return InsertSample(false) ? 1 : 0;

	case IDC_SAMPLE_REVERSE:
		OnReverse();
		break;

	case IDC_SAMPLE_SILENCE:
		OnSilence();
		break;

	case IDC_SAMPLE_INVERT:
		OnInvert();
		break;

	case IDC_SAMPLE_XFADE:
		OnXFade();
		break;

	case IDC_SAMPLE_STEREOSEPARATION:
		OnStereoSeparation();
		break;

	case IDC_SAMPLE_AUTOTUNE:
		OnAutotune();
		break;

	case IDC_SAMPLE_SIGN_UNSIGN:
		OnSignUnSign();
		break;

	case IDC_SAMPLE_DCOFFSET:
		RemoveDCOffset(false);
		break;

	case IDC_SAMPLE_NORMALIZE:
		Normalize(false);
		break;

	case IDC_SAMPLE_AMPLIFY:
		OnAmplify();
		break;

	case IDC_SAMPLE_QUICKFADE:
		OnQuickFade();
		break;

	case IDC_SAMPLE_OPEN:
		OnSampleOpen();
		break;

	case IDC_SAMPLE_SAVEAS:
		OnSampleSave();
		break;

	case IDC_SAMPLE_NEW:
		InsertSample(false);
		break;

	default:
		return CModControlDlg::OnModCtrlMsg(wParam, lParam);
	}
	return 0;
}


CString CCtrlSamples::GetToolTipText(UINT uId, HWND) const
{
	CString s;
	if(uId)
	{
		UINT val = GetDlgItemInt(uId);
		CommandID cmd = kcNull;
		switch(uId)
		{
		case IDC_SAMPLE_NEW: s = _T("Insert Sample"); cmd = kcSampleNew; break;
		case IDC_SAMPLE_OPEN: s = _T("Import Sample"); cmd = kcSampleLoad; break;
		case IDC_SAMPLE_SAVEAS: s = _T("Save Sample"); cmd = kcSampleSave; break;
		case IDC_SAMPLE_PLAY: s = _T("Play Sample"); break;
		case IDC_SAMPLE_NORMALIZE: s = _T("Normalize (hold shift to normalize all samples)"); cmd = kcSampleNormalize; break;
		case IDC_SAMPLE_AMPLIFY: s = _T("Amplify"); cmd = kcSampleAmplify; break;
		case IDC_SAMPLE_DCOFFSET: s = _T("Remove DC Offset and Normalize (hold shift to process all samples)"); cmd = kcSampleRemoveDCOffset; break;
		case IDC_SAMPLE_STEREOSEPARATION: s = _T("Change Stereo Separation / Stereo Width of the sample"); cmd = kcSampleStereoSep; break;
		case IDC_SAMPLE_RESAMPLE: s = _T("Resample"); cmd = kcSampleResample; break;
		case IDC_SAMPLE_REVERSE: s = _T("Reverse"); cmd = kcSampleReverse; break;
		case IDC_SAMPLE_SILENCE: s = _T("Silence"); cmd = kcSampleSilence; break;
		case IDC_SAMPLE_INVERT: s = _T("Invert Phase"); cmd = kcSampleInvert; break;
		case IDC_SAMPLE_SIGN_UNSIGN: s = _T("Signed/Unsigned Conversion"); cmd = kcSampleSignUnsign; break;
		case IDC_SAMPLE_XFADE: s = _T("Crossfade Sample Loops"); cmd = kcSampleXFade; break;
		case IDC_SAMPLE_AUTOTUNE: s = _T("Tune the sample to a given note"); cmd = kcSampleAutotune; break;

		case IDC_EDIT7:
		case IDC_EDIT8:
			// Volume to dB
			if(IsOPLInstrument())
				s = mpt::cfmt::fix((static_cast<int32>(val) - 64) * 0.75, 2) + _T(" dB");
			else
				s = CModDoc::LinearToDecibels(val, 64.0);
			break;

		case IDC_EDIT9:
			// Panning
			if(m_nSample)
			{
				const ModSample &sample = m_sndFile.GetSample(m_nSample);
				s = CModDoc::PanningToString(sample.nPan, 128);
			}
			break;

		case IDC_EDIT5:
		case IDC_SPIN5:
		case IDC_COMBO_BASENOTE:
			if(m_nSample)
			{
				const ModSample &sample = m_sndFile.GetSample(m_nSample);
				const auto freqHz = sample.GetSampleRate(m_sndFile.GetType());
				if(sample.uFlags[CHN_ADLIB])
				{
					// Translate to actual note frequency
					s = MPT_CFORMAT("{}Hz")(mpt::tfmt::flt(freqHz * (261.625 / 8363.0), 6));
				}
				if(m_sndFile.UseFinetuneAndTranspose())
				{
					// Transpose + Finetune to Frequency
					s = MPT_CFORMAT("{}Hz")(freqHz);
				}
			}
			break;

		case IDC_EDIT14:
			// Vibrato Sweep
			if(!(m_sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM)))
			{
				s = _T("Only available in IT / MPTM / XM format");
				break;
			} else if(m_nSample)
			{
				const ModSample &sample = m_sndFile.GetSample(m_nSample);
				int ticks = -1;
				if(m_sndFile.m_playBehaviour[kITVibratoTremoloPanbrello])
				{
					if(val > 0)
						ticks = Util::muldivr_unsigned(sample.nVibDepth, 256, val);
				} else if(m_sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT))
				{
					if(val > 0)
						ticks = Util::muldivr_unsigned(sample.nVibDepth, 128, val);
				} else
				{
					ticks = val;
				}
				if(ticks >= 0)
					s = MPT_CFORMAT("{} ticks")(ticks);
				else
					s = _T("No Vibrato");
			}
			break;
		case IDC_EDIT15:
			// Vibrato Depth
			if(!(m_sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM)))
				s = _T("Only available in IT / MPTM / XM format");
			else
				s = MPT_CFORMAT("{} cents")(Util::muldivr_unsigned(val, 100, 64));
			break;
		case IDC_EDIT16:
			// Vibrato Rate
			if(!(m_sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM)))
			{
				s = _T("Only available in IT / MPTM / XM format");
			} else if(val == 0)
			{
				s = _T("Stopped");
			} else
			{
				const double ticksPerCycle = 256.0 / val;
				const uint32 ticksPerBeat = std::max(1u, m_sndFile.m_PlayState.m_nCurrentRowsPerBeat * m_sndFile.m_PlayState.m_nMusicSpeed);
				s = MPT_CFORMAT("{} beats per cycle ({} ticks)")(mpt::cfmt::flt(ticksPerCycle / ticksPerBeat, 3), mpt::cfmt::flt(ticksPerCycle, 3));
			}
			break;

		case IDC_CHECK1:
		case IDC_EDIT3:
		case IDC_EDIT4:
		case IDC_COMBO2:
			if(!(m_sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)))
				s = _T("Only available in IT / MPTM format");
			break;

		case IDC_COMBO3:
			if(!(m_sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_XM)))
				s = _T("Only available in IT / MPTM / XM format");
			break;

		case IDC_CHECK2:
			s = _T("Keep a reference to the original waveform instead of saving it in the module.");
			break;
		}
		if(cmd != kcNull)
		{
			auto keyText = CMainFrame::GetInputHandler()->m_activeCommandSet->GetKeyTextFromCommand(cmd, 0);
			if (!keyText.IsEmpty())
				s += MPT_CFORMAT(" ({})")(keyText);
		}
	}
	return s;
}


void CCtrlSamples::UpdateView(UpdateHint hint, CObject *pObj)
{
	if(pObj == this) return;
	if (hint.GetType()[HINT_MPTOPTIONS])
	{
		m_ToolBar1.UpdateStyle();
		m_ToolBar2.UpdateStyle();
	}

	const SampleHint sampleHint = hint.ToType<SampleHint>();
	FlagSet<HintType> hintType = sampleHint.GetType();
	if(!m_initialized)
		hintType.set(HINT_MODTYPE);
	if(!hintType[HINT_SMPNAMES | HINT_SAMPLEINFO | HINT_MODTYPE]) return;

	const SAMPLEINDEX updateSmp = sampleHint.GetSample();
	if(updateSmp != m_nSample && updateSmp != 0 && !hintType[HINT_MODTYPE]) return;

	const CModSpecifications &specs = m_sndFile.GetModSpecifications();
	const bool isOPL = IsOPLInstrument();

	LockControls();
	// Updating Ranges
	if(hintType[HINT_MODTYPE])
	{

		// Limit text fields
		m_EditName.SetLimitText(specs.sampleNameLengthMax);
		m_EditFileName.SetLimitText(specs.sampleFilenameLengthMax);

		// Loop Type
		m_ComboLoopType.ResetContent();
		m_ComboLoopType.AddString(_T("Off"));
		m_ComboLoopType.AddString(_T("On"));

		// Sustain Loop Type
		m_ComboSustainType.ResetContent();
		m_ComboSustainType.AddString(_T("Off"));
		m_ComboSustainType.AddString(_T("On"));

		// Bidirectional Loops
		if (m_sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			m_ComboLoopType.AddString(_T("Bidi"));
			m_ComboSustainType.AddString(_T("Bidi"));
		}

		// Loop Start
		m_SpinLoopStart.SetRange(-1, 1);
		m_SpinLoopStart.SetPos(0);

		// Loop End
		m_SpinLoopEnd.SetRange(-1, 1);
		m_SpinLoopEnd.SetPos(0);

		// Sustain Loop Start
		m_SpinSustainStart.SetRange(-1, 1);
		m_SpinSustainStart.SetPos(0);

		// Sustain Loop End
		m_SpinSustainEnd.SetRange(-1, 1);
		m_SpinSustainEnd.SetPos(0);

		// Finetune / C-5 Speed / BaseNote
		BOOL b = m_sndFile.UseFinetuneAndTranspose() ? FALSE : TRUE;
		SetDlgItemText(IDC_TEXT7, (b) ? _T("Freq. (Hz)") : _T("Finetune"));
		m_SpinFineTune.SetRange(-1, 1);
		m_EditFileName.EnableWindow(b);

		// AutoVibrato
		b = (m_sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
		m_ComboAutoVib.EnableWindow(b);
		m_SpinVibSweep.EnableWindow(b);
		m_SpinVibDepth.EnableWindow(b);
		m_SpinVibRate.EnableWindow(b);
		m_EditVibSweep.EnableWindow(b);
		m_EditVibDepth.EnableWindow(b);
		m_EditVibRate.EnableWindow(b);
		m_SpinVibSweep.SetRange(0, 255);
		if(m_sndFile.GetType() & MOD_TYPE_XM)
		{
			m_SpinVibDepth.SetRange(0, 15);
			m_SpinVibRate.SetRange(0, 63);
		} else
		{
			m_SpinVibDepth.SetRange(0, 32);
			m_SpinVibRate.SetRange(0, 64);
		}

		// Global Volume
		b = (m_sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
		m_EditGlobalVol.EnableWindow(b);
		m_SpinGlobalVol.EnableWindow(b);

		// Panning
		b = (m_sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
		m_CheckPanning.EnableWindow(b && !(m_sndFile.GetType() & MOD_TYPE_XM));
		m_EditPanning.EnableWindow(b);
		m_SpinPanning.EnableWindow(b);
		m_SpinPanning.SetRange(0, (m_sndFile.GetType() == MOD_TYPE_XM) ? 255 : 64);

		b = (m_sndFile.GetType() & MOD_TYPE_MOD) ? FALSE : TRUE;
		m_CbnBaseNote.EnableWindow(b);
	}
	// Updating Values
	if (hintType[HINT_MODTYPE | HINT_SAMPLEINFO])
	{
		if(m_nSample > m_sndFile.GetNumSamples())
		{
			SetCurrentSample(m_sndFile.GetNumSamples());
		}
		const ModSample &sample = m_sndFile.GetSample(m_nSample);
		CString s;
		DWORD d;

		m_SpinSample.SetRange(1, m_sndFile.GetNumSamples());
		m_SpinSample.Invalidate(FALSE);	// In case the spin button was previously disabled

		// Length / Type
		if(isOPL)
			s = _T("OPL instrument");
		else
			s = MPT_CFORMAT("{}-bit {}, len: {}")(sample.GetElementarySampleSize() * 8, CString(sample.uFlags[CHN_STEREO] ? _T("stereo") : _T("mono")), mpt::cfmt::dec(3, _T(","), sample.nLength));
		SetDlgItemText(IDC_TEXT5, s);
		// File Name
		s = mpt::ToCString(m_sndFile.GetCharsetInternal(), sample.filename);
		if (specs.sampleFilenameLengthMax == 0) s.Empty();
		SetDlgItemText(IDC_SAMPLE_FILENAME, s);
		// Volume
		if(sample.uFlags[SMP_NODEFAULTVOLUME])
			SetDlgItemText(IDC_EDIT7, _T("none"));
		else
			SetDlgItemInt(IDC_EDIT7, sample.nVolume / 4u);
		// Global Volume
		SetDlgItemInt(IDC_EDIT8, sample.nGlobalVol);
		// Panning
		CheckDlgButton(IDC_CHECK1, (sample.uFlags[CHN_PANNING]) ? BST_CHECKED : BST_UNCHECKED);
		if (m_sndFile.GetType() == MOD_TYPE_XM)
			SetDlgItemInt(IDC_EDIT9, sample.nPan);		//displayed panning with XM is 0-256, just like MPT's internal engine
		else
			SetDlgItemInt(IDC_EDIT9, sample.nPan / 4u);	//displayed panning with anything but XM is 0-64 so we divide by 4
		// FineTune / C-4 Speed / BaseNote
		int transp = 0;
		if (!m_sndFile.UseFinetuneAndTranspose())
		{
			s = mpt::cfmt::val(sample.nC5Speed);
			m_EditFineTune.SetWindowText(s);
			if(sample.nC5Speed != 0)
				transp = GetSampleTransponse(sample);
		} else
		{
			int ftune = ((int)sample.nFineTune);
			// MOD finetune range -8 to 7 translates to -128 to 112
			if(m_sndFile.GetType() & MOD_TYPE_MOD) ftune >>= 4;
			SetDlgItemInt(IDC_EDIT5, ftune);
			transp = (int)sample.RelativeTone;
		}
		int basenote = (NOTE_MIDDLEC - NOTE_MIN) + transp;
		Limit(basenote, BASENOTE_MIN, BASENOTE_MAX);
		basenote -= BASENOTE_MIN;
		if (basenote != m_CbnBaseNote.GetCurSel()) m_CbnBaseNote.SetCurSel(basenote);

		// Auto vibrato
		// Ramp up and ramp down are swapped in XM - probably because they ramp up the *period* instead of *frequency*.
		const VibratoType rampUp = m_sndFile.GetType() == MOD_TYPE_XM ? VIB_RAMP_DOWN : VIB_RAMP_UP;
		const VibratoType rampDown = m_sndFile.GetType() == MOD_TYPE_XM ? VIB_RAMP_UP : VIB_RAMP_DOWN;
		m_ComboAutoVib.ResetContent();
		m_ComboAutoVib.SetItemData(m_ComboAutoVib.AddString(_T("Sine")), VIB_SINE);
		m_ComboAutoVib.SetItemData(m_ComboAutoVib.AddString(_T("Square")), VIB_SQUARE);
		if(m_sndFile.GetType() != MOD_TYPE_IT || sample.nVibType == VIB_RAMP_UP)
			m_ComboAutoVib.SetItemData(m_ComboAutoVib.AddString(_T("Ramp Up")), rampUp);
		m_ComboAutoVib.SetItemData(m_ComboAutoVib.AddString(_T("Ramp Down")), rampDown);
		if(m_sndFile.GetType() != MOD_TYPE_XM || sample.nVibType == VIB_RANDOM)
			m_ComboAutoVib.SetItemData(m_ComboAutoVib.AddString(_T("Random")), VIB_RANDOM);

		for(int i = 0; i < m_ComboAutoVib.GetCount(); i++)
		{
			if(m_ComboAutoVib.GetItemData(i) == sample.nVibType)
			{
				m_ComboAutoVib.SetCurSel(i);
				break;
			}
		}

		SetDlgItemInt(IDC_EDIT14, sample.nVibSweep);
		SetDlgItemInt(IDC_EDIT15, sample.nVibDepth);
		SetDlgItemInt(IDC_EDIT16, sample.nVibRate);
		// Loop
		d = 0;
		if (sample.uFlags[CHN_LOOP]) d = sample.uFlags[CHN_PINGPONGLOOP] ? 2 : 1;
		if (sample.uFlags[CHN_REVERSE]) d |= 4;
		m_ComboLoopType.SetCurSel(d);
		s = mpt::cfmt::val(sample.nLoopStart);
		m_EditLoopStart.SetWindowText(s);
		s = mpt::cfmt::val(sample.nLoopEnd);
		m_EditLoopEnd.SetWindowText(s);
		// Sustain Loop
		d = 0;
		if (sample.uFlags[CHN_SUSTAINLOOP]) d = sample.uFlags[CHN_PINGPONGSUSTAIN] ? 2 : 1;
		m_ComboSustainType.SetCurSel(d);
		s = mpt::cfmt::val(sample.nSustainStart);
		m_EditSustainStart.SetWindowText(s);
		s = mpt::cfmt::val(sample.nSustainEnd);
		m_EditSustainEnd.SetWindowText(s);

		// Disable certain buttons for OPL instruments
		BOOL b = isOPL ? FALSE : TRUE;
		static constexpr int sampleButtons[] =
		{
			IDC_SAMPLE_NORMALIZE,   IDC_SAMPLE_AMPLIFY,
			IDC_SAMPLE_DCOFFSET,    IDC_SAMPLE_STEREOSEPARATION,
			IDC_SAMPLE_RESAMPLE,    IDC_SAMPLE_REVERSE,
			IDC_SAMPLE_SILENCE,     IDC_SAMPLE_INVERT,
			IDC_SAMPLE_SIGN_UNSIGN, IDC_SAMPLE_XFADE,
		};
		for(auto btn : sampleButtons)
		{
			m_ToolBar2.EnableButton(btn, b);
		}
		m_ComboLoopType.EnableWindow(b);
		m_SpinLoopStart.EnableWindow(b);
		m_SpinLoopEnd.EnableWindow(b);
		m_EditLoopStart.EnableWindow(b);
		m_EditLoopEnd.EnableWindow(b);

		const bool hasSustainLoop = !isOPL && ((m_sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) || (m_nSample <= m_sndFile.GetNumSamples() && m_sndFile.GetSample(m_nSample).uFlags[CHN_SUSTAINLOOP]));
		b = hasSustainLoop ? TRUE : FALSE;
		m_ComboSustainType.EnableWindow(b);
		m_SpinSustainStart.EnableWindow(b);
		m_SpinSustainEnd.EnableWindow(b);
		m_EditSustainStart.EnableWindow(b);
		m_EditSustainEnd.EnableWindow(b);

	}
	if(hintType[HINT_MODTYPE | HINT_SAMPLEINFO | HINT_SMPNAMES])
	{
		// Name
		SetDlgItemText(IDC_SAMPLE_NAME, mpt::ToWin(m_sndFile.GetCharsetInternal(), m_sndFile.m_szNames[m_nSample]).c_str());

		CheckDlgButton(IDC_CHECK2, m_sndFile.GetSample(m_nSample).uFlags[SMP_KEEPONDISK] ? BST_CHECKED : BST_UNCHECKED);
		GetDlgItem(IDC_CHECK2)->EnableWindow((m_sndFile.SampleHasPath(m_nSample) && m_sndFile.GetType() == MOD_TYPE_MPT) ? TRUE : FALSE);
	}

	if(!m_initialized)
	{
		// First update
		m_initialized = true;
		UnlockControls();
	}

	m_ComboLoopType.Invalidate(FALSE);
	m_ComboSustainType.Invalidate(FALSE);
	m_ComboAutoVib.Invalidate(FALSE);

	UnlockControls();
}


// updateAll: Update all views including this one. Otherwise, only update update other views.
void CCtrlSamples::SetModified(SAMPLEINDEX smp, SampleHint hint, bool updateAll, bool waveformModified)
{
	m_modDoc.SetModified();

	if(waveformModified)
	{
		// Update on-disk sample status in tree
		ModSample &sample = m_sndFile.GetSample(smp);
		if(sample.uFlags[SMP_KEEPONDISK] && !sample.uFlags[SMP_MODIFIED])
			hint.Names();
		sample.uFlags.set(SMP_MODIFIED);
	}
	m_modDoc.UpdateAllViews(nullptr, hint.SetData(smp), updateAll ? nullptr : this);
}


void CCtrlSamples::PrepareUndo(const char *description, sampleUndoTypes type, SmpLength start, SmpLength end)
{
	m_startedEdit = true;
	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, type, description, start, end);
}


bool CCtrlSamples::OpenSample(const mpt::PathString &fileName, FlagSet<OpenSampleTypes> types)
{
	BeginWaitCursor();
	mpt::IO::InputFile f(fileName, TrackerSettings::Instance().MiscCacheCompleteFileBeforeLoading);
	if(!f.IsValid())
	{
		EndWaitCursor();
		return false;
	}

	FileReader file = GetFileReader(f);
	if(!file.IsValid())
	{
		EndWaitCursor();
		return false;
	}

	PrepareUndo("Replace", sundo_replace);
	const auto parentIns = m_modDoc.GetParentInstrumentWithSameName(m_nSample);
	bool bOk = false;
	if(types[OpenSampleKnown])
	{
		bOk = m_sndFile.ReadSampleFromFile(m_nSample, file, TrackerSettings::Instance().m_MayNormalizeSamplesOnLoad);

		if(!bOk)
		{
			// Try loading as module
			bOk = CMainFrame::GetMainFrame()->SetTreeSoundfile(file);
			if(bOk)
			{
				m_modDoc.GetSampleUndo().RemoveLastUndoStep(m_nSample);
				return true;
			}
		}
	}
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	if(!bOk && types[OpenSampleRaw])
	{
		CRawSampleDlg dlg(file, this);
		EndWaitCursor();
		if(m_rememberRawFormat || dlg.DoModal() == IDOK)
		{
			SampleIO sampleIO   = dlg.GetSampleFormat();
			m_rememberRawFormat = m_rememberRawFormat || dlg.GetRemeberFormat();

			BeginWaitCursor();

			file.Seek(dlg.GetOffset());

			m_sndFile.DestroySampleThreadsafe(m_nSample);
			const auto bytesPerSample = sampleIO.GetNumChannels() * sampleIO.GetBitDepth() / 8u;
			sample.nLength            = mpt::saturate_cast<SmpLength>(file.BytesLeft() / bytesPerSample);

			if(TrackerSettings::Instance().m_MayNormalizeSamplesOnLoad)
			{
				sampleIO.MayNormalize();
			}

			if(sampleIO.ReadSample(sample, file))
			{
				bOk = true;

				sample.nGlobalVol = 64;
				sample.nVolume = 256;
				sample.nPan = 128;
				sample.uFlags.reset(CHN_LOOP | CHN_SUSTAINLOOP | SMP_MODIFIED);
				sample.filename = "";
				m_sndFile.m_szNames[m_nSample] = "";
				if(!sample.nC5Speed) sample.nC5Speed = 22050;
				sample.PrecomputeLoops(m_sndFile, false);
			} else
			{
				m_modDoc.GetSampleUndo().Undo(m_nSample);
			}

		} else
		{
			m_modDoc.GetSampleUndo().RemoveLastUndoStep(m_nSample);
		}
	} else
	{
		m_sndFile.SetSamplePath(m_nSample, fileName);
	}

	EndWaitCursor();
	if (bOk)
	{
		TrackerSettings::Instance().PathSamples.SetWorkingDir(fileName, true);
		if(sample.filename.empty())
		{
			mpt::PathString name, ext;
			fileName.SplitPath(nullptr, nullptr, nullptr, &name, &ext);

			if(m_sndFile.m_szNames[m_nSample].empty()) m_sndFile.m_szNames[m_nSample] = name.ToLocale();

			if(name.AsNative().length() < 9) name += ext;
			sample.filename = name.ToLocale();
		}
		if ((m_sndFile.GetType() & MOD_TYPE_XM) && !sample.uFlags[CHN_PANNING])
		{
			sample.nPan = 128;
			sample.uFlags.set(CHN_PANNING);
		}
		SetModified(SampleHint().Info().Data().Names(), true, false);
		sample.uFlags.reset(SMP_KEEPONDISK);

		if(parentIns <= m_sndFile.GetNumInstruments())
		{
			if(auto instr = m_sndFile.Instruments[parentIns]; instr != nullptr)
			{
				m_modDoc.GetInstrumentUndo().PrepareUndo(parentIns, "Set Name");
				instr->name = m_sndFile.m_szNames[m_nSample];
				m_modDoc.UpdateAllViews(nullptr, InstrumentHint(parentIns).Names(), this);
			}
		}
	}
	return true;
}


bool CCtrlSamples::OpenSample(const CSoundFile &sndFile, SAMPLEINDEX nSample)
{
	if(!nSample || nSample > sndFile.GetNumSamples()) return false;

	BeginWaitCursor();

	PrepareUndo("Replace", sundo_replace);
	const auto parentIns = m_modDoc.GetParentInstrumentWithSameName(m_nSample);
	if(m_sndFile.ReadSampleFromSong(m_nSample, sndFile, nSample))
	{
		SetModified(SampleHint().Info().Data().Names(), true, false);
		if(parentIns <= m_sndFile.GetNumInstruments())
		{
			if(auto instr = m_sndFile.Instruments[parentIns]; instr != nullptr)
			{
				m_modDoc.GetInstrumentUndo().PrepareUndo(parentIns, "Set Name");
				instr->name = m_sndFile.m_szNames[m_nSample];
				m_modDoc.UpdateAllViews(nullptr, InstrumentHint(parentIns).Names(), this);
			}
		}
	} else
	{
		m_modDoc.GetSampleUndo().RemoveLastUndoStep(m_nSample);
	}

	EndWaitCursor();

	return true;
}


//////////////////////////////////////////////////////////////////////////////////
// CCtrlSamples messages

void CCtrlSamples::OnSampleChanged()
{
	if(!IsLocked())
	{
		SAMPLEINDEX n = (SAMPLEINDEX)GetDlgItemInt(IDC_EDIT_SAMPLE);
		if ((n > 0) && (n <= m_sndFile.GetNumSamples()) && (n != m_nSample))
		{
			SetCurrentSample(n, -1, FALSE);
			m_parent.SampleChanged(m_nSample);
		}
	}
}


void CCtrlSamples::OnZoomChanged()
{
	if (!IsLocked()) SetCurrentSample(m_nSample);
	SwitchToView();
}


void CCtrlSamples::OnTbnDropDownToolBar(NMHDR *pNMHDR, LRESULT *pResult)
{
	CInputHandler *ih = CMainFrame::GetInputHandler();
	NMTOOLBAR *pToolBar = reinterpret_cast<NMTOOLBAR *>(pNMHDR);
	ClientToScreen(&(pToolBar->rcButton)); // TrackPopupMenu uses screen coords
	const int offset = HighDPISupport::ScalePixels(4, m_hWnd);  // Compared to the main toolbar, the offset seems to be a bit wrong here...?
	int x = pToolBar->rcButton.left + offset, y = pToolBar->rcButton.bottom + offset;
	CMenu menu;
	switch(pToolBar->iItem)
	{
	case IDC_SAMPLE_NEW:
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, IDC_SAMPLE_DUPLICATE, ih->GetKeyTextFromCommand(kcSampleDuplicate, m_sndFile.GetSample(m_nSample).uFlags[CHN_ADLIB] ? _T("&Duplicate Instrument") : _T("&Duplicate Sample")));
		menu.AppendMenu(MF_STRING | (m_sndFile.SupportsOPL() ? 0 : MF_DISABLED), IDC_SAMPLE_INITOPL, ih->GetKeyTextFromCommand(kcSampleInitializeOPL, _T("Initialize &OPL Instrument")));
		menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, this);
		menu.DestroyMenu();
		break;
	case IDC_SAMPLE_OPEN:
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, IDC_SAMPLE_OPENKNOWN, ih->GetKeyTextFromCommand(kcSampleLoad, _T("Import &Sample...")));
		menu.AppendMenu(MF_STRING, IDC_SAMPLE_OPENRAW, ih->GetKeyTextFromCommand(kcSampleLoadRaw, _T("Import &Raw Sample...")));
		menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, this);
		menu.DestroyMenu();
		break;
	case IDC_SAMPLE_SAVEAS:
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, IDC_SAVE_ALL, _T("Save &All..."));
		menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, this);
		menu.DestroyMenu();
		break;
	}
	*pResult = 0;
}


void CCtrlSamples::OnSampleNew()
{
	InsertSample(CInputHandler::ShiftPressed());
	SwitchToView();
}


bool CCtrlSamples::InsertSample(bool duplicate, int8 *confirm)
{
	const SAMPLEINDEX smp = m_modDoc.InsertSample();
	if(smp != SAMPLEINDEX_INVALID)
	{
		const SAMPLEINDEX oldSmp = m_nSample;
		CSoundFile &sndFile = m_modDoc.GetSoundFile();
		SetCurrentSample(smp);

		if(duplicate && oldSmp >= 1 && oldSmp <= sndFile.GetNumSamples())
		{
			m_modDoc.GetSampleUndo().PrepareUndo(smp, sundo_replace, "Duplicate");
			sndFile.ReadSampleFromSong(smp, sndFile, oldSmp);
		}

		m_modDoc.UpdateAllViews(nullptr, SampleHint(smp).Info().Data().Names());
		if(m_modDoc.GetNumInstruments() > 0 && m_modDoc.FindSampleParent(smp) == INSTRUMENTINDEX_INVALID)
		{
			bool insertInstrument;
			if(confirm == nullptr || *confirm == -1)
			{
				insertInstrument = Reporting::Confirm("This sample is not used by any instrument. Do you want to create a new instrument using this sample?") == cnfYes;
				if(confirm != nullptr) *confirm = insertInstrument;
			} else
			{
				insertInstrument = (*confirm) != 0;
			}
			if(insertInstrument)
			{
				INSTRUMENTINDEX nins = m_modDoc.InsertInstrument(smp);
				m_modDoc.UpdateAllViews(nullptr, InstrumentHint(nins).Info().Envelope().Names());
				m_parent.InstrumentChanged(nins);
			}
		}
	}
	return (smp != SAMPLEINDEX_INVALID);
}


void CCtrlSamples::OnSampleOpen()
{
	static int nLastIndex = 0;
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.EnableAudioPreview()
		.ExtensionFilter(ConstructSampleFormatFileFilter(true))
		.WorkingDirectory(TrackerSettings::Instance().PathSamples.GetWorkingDir())
		.FilterIndex(&nLastIndex);
	if(!dlg.Show(this)) return;

	TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetWorkingDirectory());

	OpenSamples(dlg.GetFilenames(), OpenSampleKnown | OpenSampleRaw);
	SwitchToView();
}


void CCtrlSamples::OnSampleOpenKnown()
{
	static int nLastIndex = 0;
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.EnableAudioPreview()
		.ExtensionFilter(ConstructSampleFormatFileFilter(false))
		.WorkingDirectory(TrackerSettings::Instance().PathSamples.GetWorkingDir())
		.FilterIndex(&nLastIndex);
	if(!dlg.Show(this)) return;

	TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetWorkingDirectory());

	OpenSamples(dlg.GetFilenames(), OpenSampleKnown);
}


void CCtrlSamples::OnSampleOpenRaw()
{
	static int nLastIndex = 0;
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.EnableAudioPreview()
		.ExtensionFilter(U_("Raw Samples (*.raw,*.snd,*.pcm,*.sam)|*.raw;*.snd;*.pcm;*.sam|All Files (*.*)|*.*||"))
		.WorkingDirectory(TrackerSettings::Instance().PathSamples.GetWorkingDir())
		.FilterIndex(&nLastIndex);
	if(!dlg.Show(this)) return;

	TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetWorkingDirectory());

	OpenSamples(dlg.GetFilenames(), OpenSampleRaw);
}


void CCtrlSamples::OpenSamples(const std::vector<mpt::PathString> &files, FlagSet<OpenSampleTypes> types)
{
	int8 confirm = -1;
	bool first = true;
	for(const auto &file : files)
	{
		// If loading multiple samples, create new slots for them
		if(!first)
		{
			if(!InsertSample(false, &confirm))
				break;
		}

		if(OpenSample(file, types))
			first = false;
		else
			ErrorBox(IDS_ERR_FILEOPEN, this);
	}
	SwitchToView();
}


void CCtrlSamples::OnSampleSave()
{
	SaveSample(CInputHandler::ShiftPressed());
}


void CCtrlSamples::SaveSample(bool doBatchSave)
{
	mpt::PathString fileName, defaultPath = TrackerSettings::Instance().PathSamples.GetWorkingDir();
	SampleEditorDefaultFormat defaultFormat = TrackerSettings::Instance().m_defaultSampleFormat;
	bool hasAdlib = false;

	if(!doBatchSave)
	{
		// Save this sample
		const ModSample &sample = m_sndFile.GetSample(m_nSample);
		if((!m_nSample) || (!sample.HasSampleData()))
		{
			SwitchToView();
			return;
		}
		if(m_sndFile.SampleHasPath(m_nSample))
		{
			// For on-disk samples, propose their original filename and location
			auto path = m_sndFile.GetSamplePath(m_nSample);
			fileName = path.GetFilename();
			defaultPath = path.GetDirectoryWithDrive();
		}
		if(fileName.empty()) fileName = mpt::PathString::FromLocale(sample.filename);
		if(fileName.empty()) fileName = mpt::PathString::FromLocale(m_sndFile.m_szNames[m_nSample]);
		if(fileName.empty()) fileName = P_("untitled");

		const mpt::PathString ext = fileName.GetFilenameExtension();
		if(!mpt::PathCompareNoCase(ext, P_(".flac"))) defaultFormat = dfFLAC;
		else if(!mpt::PathCompareNoCase(ext, P_(".wav"))) defaultFormat = dfWAV;
		else if(!mpt::PathCompareNoCase(ext, P_(".s3i"))) defaultFormat = dfS3I;
		else if(!mpt::PathCompareNoCase(ext, P_(".iff"))) defaultFormat = dfIFF;
		else if(!mpt::PathCompareNoCase(ext, P_(".8sv"))) defaultFormat = dfIFF;
		else if(!mpt::PathCompareNoCase(ext, P_(".8svx"))) defaultFormat = dfIFF;
		else if(!mpt::PathCompareNoCase(ext, P_(".16sv"))) defaultFormat = dfIFF;
		else if(!mpt::PathCompareNoCase(ext, P_(".16svx"))) defaultFormat = dfIFF;

		hasAdlib = sample.uFlags[CHN_ADLIB];
	} else
	{
		// Save all samples
		fileName = m_sndFile.GetpModDoc()->GetPathNameMpt().GetFilenameBase();
		if(fileName.empty()) fileName = P_("untitled");

		fileName += P_(" - %sample_number% - ");
		if(m_sndFile.GetModSpecifications().sampleFilenameLengthMax == 0)
			fileName += P_("%sample_name%");
		else
			fileName += P_("%sample_filename%");
	}
	fileName = fileName.AsSanitizedComponent();

	int filter;
	switch(defaultFormat)
	{
	case dfWAV:
		filter = 1;
		break;
	case dfFLAC:
	default:
		filter = 2;
		break;
	case dfS3I:
		filter = 3;
		break;
	case dfIFF:
		filter = 4;
		break;
	case dfRAW:
		filter = 5;
		break;
	}
	// Do we have to use a format that can save OPL instruments?
	if(hasAdlib)
		filter = 3;

	FileDialog dlg = SaveFileDialog()
		.DefaultExtension(ToSettingValue(defaultFormat).as<mpt::ustring>())
		.DefaultFilename(fileName)
		.ExtensionFilter("Wave File (*.wav)|*.wav|"
			"FLAC File (*.flac)|*.flac|"
			"S3I Scream Tracker 3 Instrument (*.s3i)|*.s3i|"
			"IFF (8SVX / 16SVX) (*.iff)|*.iff;*.8sv;*.8svx;*.16sv;*.16svx|"
			"RAW Audio (*.raw)|*.raw||")
			.WorkingDirectory(defaultPath)
			.FilterIndex(&filter);
	if(!dlg.Show(this)) return;

	BeginWaitCursor();

	const auto saveFormat = FromSettingValue<SampleEditorDefaultFormat>(dlg.GetExtension().ToUnicode());

	SAMPLEINDEX minSmp = m_nSample, maxSmp = m_nSample;
	if(doBatchSave)
	{
		minSmp = 1;
		maxSmp = m_sndFile.GetNumSamples();
	}
	const auto numberFmt = mpt::format_simple_spec<mpt::ustring>().Dec().FillNul().Width(1 + static_cast<int>(std::log10(maxSmp)));

	bool ok = false;
	CString sampleName, sampleFilename;

	for(SAMPLEINDEX smp = minSmp; smp <= maxSmp; smp++)
	{
		ModSample &sample = m_sndFile.GetSample(smp);
		if(sample.HasSampleData())
		{
			const bool isAdlib = sample.uFlags[CHN_ADLIB];

			fileName = dlg.GetFirstFile();
			if(doBatchSave)
			{
				sampleName = mpt::ToCString(m_sndFile.GetCharsetInternal(), (!m_sndFile.m_szNames[smp].empty()) ? std::string(m_sndFile.m_szNames[smp]) : "untitled");
				sampleFilename = mpt::ToCString(m_sndFile.GetCharsetInternal(), (!sample.filename.empty()) ? sample.GetFilename() : m_sndFile.m_szNames[smp]);
				sampleName = SanitizePathComponent(sampleName);
				sampleFilename = SanitizePathComponent(sampleFilename);

				mpt::ustring fileNameU = fileName.ToUnicode();
				fileNameU = mpt::replace(fileNameU, U_("%sample_number%"), mpt::ufmt::fmt(smp, numberFmt));
				fileNameU = mpt::replace(fileNameU, U_("%sample_filename%"), mpt::ToUnicode(sampleFilename));
				fileNameU = mpt::replace(fileNameU, U_("%sample_name%"), mpt::ToUnicode(sampleName));
				fileName = mpt::PathString::FromUnicode(fileNameU);

				// Need to enforce S3I for Adlib samples
				if(isAdlib && saveFormat != dfS3I)
					fileName = fileName.ReplaceExtension(P_(".s3i"));
			}

			try
			{
				mpt::IO::SafeOutputFile sf(fileName, std::ios::binary, mpt::IO::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
				mpt::IO::ofstream &f = sf;
				if(!f)
				{
					ok = false;
					continue;
				}
				f.exceptions(f.exceptions() | std::ios::badbit | std::ios::failbit);

				// Need to enforce S3I for Adlib samples
				const auto thisFormat = isAdlib ? dfS3I : saveFormat;
				if(thisFormat == dfRAW)
					ok = m_sndFile.SaveRAWSample(smp, f);
				else if(thisFormat == dfFLAC)
					ok = m_sndFile.SaveFLACSample(smp, f);
				else if(thisFormat == dfS3I)
					ok = m_sndFile.SaveS3ISample(smp, f);
				else if(thisFormat == dfIFF)
					ok = m_sndFile.SaveIFFSample(smp, f);
				else
					ok = m_sndFile.SaveWAVSample(smp, f);
			} catch(const std::exception &)
			{
				ok = false;
			}

			if(ok)
			{
				m_sndFile.SetSamplePath(smp, fileName);
				sample.uFlags.reset(SMP_MODIFIED);
				UpdateView(SampleHint().Info());

				// Check if any other samples refer to the same file - that would be dangerous.
				if(sample.uFlags[SMP_KEEPONDISK])
				{
					for(SAMPLEINDEX i = 1; i <= m_sndFile.GetNumSamples(); i++)
					{
						if(i != smp && m_sndFile.GetSample(i).uFlags[SMP_KEEPONDISK] && m_sndFile.GetSamplePath(i) == m_sndFile.GetSamplePath(smp))
						{
							m_sndFile.GetSample(i).uFlags.reset(SMP_KEEPONDISK);
							m_modDoc.UpdateAllViews(nullptr, SampleHint(i).Names().Info(), this);
						}
					}
				}
			}
		}
	}
	EndWaitCursor();

	if (!ok)
	{
		ErrorBox(IDS_ERR_SAVESMP, this);
	} else
	{
		TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetWorkingDirectory());
	}
	SwitchToView();
}


void CCtrlSamples::OnSamplePlay()
{
	if (m_modDoc.IsNotePlaying(NOTE_NONE, m_nSample, 0))
	{
		m_modDoc.NoteOff(0, true);
	} else
	{
		m_modDoc.PlayNote(PlayNoteParam(NOTE_MIDDLEC).Sample(m_nSample));
	}
	SwitchToView();
}


void CCtrlSamples::Normalize(bool allSamples)
{
	//Default case: Normalize current sample
	SAMPLEINDEX minSample = m_nSample, maxSample = m_nSample;
	//If only one sample is selected, parts of it may be amplified
	SmpLength selStart = 0, selEnd = 0;

	if(allSamples)
	{
		if(Reporting::Confirm(_T("This will normalize all samples independently. Continue?"), _T("Normalize")) == cnfNo)
			return;
		minSample = 1;
		maxSample = m_sndFile.GetNumSamples();
	} else
	{
		SampleSelectionPoints selection = GetSelectionPoints();
		selStart = selection.nStart;
		selEnd = selection.nEnd;
	}


	BeginWaitCursor();

	for(SAMPLEINDEX smp = minSample; smp <= maxSample; smp++)
	{
		if(m_sndFile.GetSample(smp).HasSampleData())
		{
			ModSample &sample = m_sndFile.GetSample(smp);

			if(minSample != maxSample)
			{
				// If more than one sample is selected, always amplify the whole sample.
				selStart = 0;
				selEnd = sample.nLength;
			} else
			{
				// One sample: correct the boundaries, if needed
				LimitMax(selEnd, sample.nLength);
				LimitMax(selStart, selEnd);
				if(selStart == selEnd)
				{
					selStart = 0;
					selEnd = sample.nLength;
				}
			}

			m_modDoc.GetSampleUndo().PrepareUndo(smp, sundo_update, "Normalize", selStart, selEnd);

			if(SampleEdit::NormalizeSample(sample, selStart, selEnd, m_sndFile))
			{
				SetModified(smp, SampleHint().Data(), smp == m_nSample, true);
			}
		}
	}

	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnNormalize()
{
	Normalize(CInputHandler::ShiftPressed());
}


void CCtrlSamples::RemoveDCOffset(bool allSamples)
{
	SAMPLEINDEX minSample = m_nSample, maxSample = m_nSample;

	//Shift -> Process all samples
	if(allSamples)
	{
		if(Reporting::Confirm(_T("This will process all samples independently. Continue?"), _T("DC Offset Removal")) == cnfNo)
			return;
		minSample = 1;
		maxSample = m_sndFile.GetNumSamples();
	}

	BeginWaitCursor();

	// for report / SetModified
	SAMPLEINDEX numModified = 0;
	double reportOffset = 0;

	for(SAMPLEINDEX smp = minSample; smp <= maxSample; smp++)
	{
		SmpLength selStart, selEnd;

		if(!m_sndFile.GetSample(smp).HasSampleData())
			continue;

		if (minSample != maxSample)
		{
			selStart = 0;
			selEnd = m_sndFile.GetSample(smp).nLength;
		} else
		{
			SampleSelectionPoints selection = GetSelectionPoints();
			selStart = selection.nStart;
			selEnd = selection.nEnd;
		}

		m_modDoc.GetSampleUndo().PrepareUndo(smp, sundo_update, "Remove DC Offset", selStart, selEnd);

		const double offset = SampleEdit::RemoveDCOffset(m_sndFile.GetSample(smp), selStart, selEnd, m_sndFile);

		if(offset == 0.0f) // No offset removed.
			continue;

		reportOffset += offset;
		numModified++;
		SetModified(smp, SampleHint().Info().Data(), smp == m_nSample, true);
	}

	EndWaitCursor();
	SwitchToView();

	// fill the statusbar with some nice information

	CString dcInfo;
	if(numModified)
	{
		if(numModified == 1)
		{
			dcInfo.Format(_T("Removed DC offset (%.1f%%)"), reportOffset * 100);
		} else
		{
			dcInfo.Format(_T("Removed DC offset from %u samples (avg %0.1f%%)"), numModified, reportOffset / numModified * 100);
		}
	} else
	{
		dcInfo.SetString(_T("No DC offset found"));
	}

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	pMainFrm->SetXInfoText(dcInfo);

}


void CCtrlSamples::OnRemoveDCOffset()
{
	RemoveDCOffset(CInputHandler::ShiftPressed());
}


void CCtrlSamples::ApplyAmplify(const double amp, const double fadeInStart, const double fadeOutEnd, const bool fadeIn, const bool fadeOut, const Fade::Law fadeLaw)
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if(!sample.HasSampleData() || sample.uFlags[CHN_ADLIB]) return;

	BeginWaitCursor();

	SampleSelectionPoints selection = GetSelectionPoints();
	const auto start = selection.nStart, end = selection.nEnd, mid = (start + end) / 2;

	PrepareUndo("Amplify", sundo_update, start, end);

	if(fadeIn && fadeOut)
	{
		SampleEdit::AmplifySample(sample, start, mid, fadeInStart, amp, true, fadeLaw, m_sndFile);
		SampleEdit::AmplifySample(sample, mid, end, amp, fadeOutEnd, false, fadeLaw, m_sndFile);
	} else if(fadeIn)
	{
		SampleEdit::AmplifySample(sample, start, end, fadeInStart, amp, true, fadeLaw, m_sndFile);
	} else if(fadeOut)
	{
		SampleEdit::AmplifySample(sample, start, end, amp, fadeOutEnd, false, fadeLaw, m_sndFile);
	} else
	{
		SampleEdit::AmplifySample(sample, start, end, amp, amp, true, Fade::kLinear, m_sndFile);
	}

	sample.PrecomputeLoops(m_sndFile, false);
	SetModified(SampleHint().Data(), false, true);
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnAmplify()
{
	static CAmpDlg::AmpSettings settings { Fade::kLinear, 0, 0, 100, false, false };

	CAmpDlg dlg(this, settings);
	if (dlg.DoModal() != IDOK) return;

	ApplyAmplify(settings.factor / 100.0, settings.fadeInStart / 100.0, settings.fadeOutEnd / 100.0, settings.fadeIn, settings.fadeOut, settings.fadeLaw);
}


// Quickly fade the selection in/out without asking the user.
// Fade-In is applied if the selection starts at the beginning of the sample.
// Fade-Out is applied if the selection ends and the end of the sample.
void CCtrlSamples::OnQuickFade()
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if(!sample.HasSampleData() || sample.uFlags[CHN_ADLIB]) return;

	SampleSelectionPoints sel = GetSelectionPoints();
	if(sel.selectionActive && (sel.nStart == 0 || sel.nEnd == sample.nLength))
	{
		ApplyAmplify(1.0, (sel.nStart == 0) ? 0.0 : 1.0, (sel.nEnd == sample.nLength) ? 0.0 : 1.0, sel.nStart == 0, sel.nEnd == sample.nLength, Fade::kLinear);
	} else
	{
		// Can't apply quick fade as no appropriate selection has been made, so ask the user to amplify the whole sample instead.
		OnAmplify();
	}
}


void CCtrlSamples::OnResample()
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if(!sample.HasSampleData() || sample.uFlags[CHN_ADLIB])
		return;

	SAMPLEINDEX first = m_nSample, last = m_nSample;
	if(CInputHandler::ShiftPressed())
	{
		first = 1;
		last = m_sndFile.GetNumSamples();
	}
	
	const uint32 oldRate = sample.GetSampleRate(m_sndFile.GetType());
	CResamplingDlg dlg(this, oldRate, TrackerSettings::Instance().sampleEditorDefaultResampler, first != last, m_sndFile.GetBestSaveFormat() == MOD_TYPE_MOD);
	if(dlg.DoModal() != IDOK)
		return;
	
	TrackerSettings::Instance().sampleEditorDefaultResampler = dlg.GetFilter();
	for(SAMPLEINDEX smp = first; smp <= last; smp++)
	{
		const uint32 sampleFreq = m_sndFile.GetSample(smp).GetSampleRate(m_sndFile.GetType());
		uint32 newFreq = dlg.GetFrequency();
		if(dlg.GetResamplingOption() == CResamplingDlg::Upsample)
			newFreq = sampleFreq * 2;
		else if(dlg.GetResamplingOption() == CResamplingDlg::Downsample)
			newFreq = sampleFreq / 2;
		else if(newFreq == sampleFreq)
			continue;
		ApplyResample(smp, newFreq, dlg.GetFilter(), first != last, dlg.UpdatePatternCommands(), dlg.UpdatePatternNotes());
	}
}


void CCtrlSamples::ApplyResample(SAMPLEINDEX smp, uint32 newRate, ResamplingMode mode, bool ignoreSelection, bool updatePatternCommands, bool updatePatternNotes)
{
	BeginWaitCursor();

	ModSample &sample = m_sndFile.GetSample(smp);
	if(!sample.HasSampleData() || sample.uFlags[CHN_ADLIB])
	{
		EndWaitCursor();
		return;
	}

	SampleSelectionPoints selection = GetSelectionPoints();
	LimitMax(selection.nEnd, sample.nLength);
	if(selection.nStart >= selection.nEnd || ignoreSelection)
	{
		selection.nStart = 0;
		selection.nEnd = sample.nLength;
	}

	const uint32 oldRate = sample.GetSampleRate(m_sndFile.GetType());
	if(newRate < 1 || oldRate < 1)
	{
		MessageBeep(MB_ICONWARNING);
		EndWaitCursor();
		return;
	}

	auto prepareSampleUndoFunc = [&]()
	{
		m_modDoc.GetSampleUndo().PrepareUndo(smp, sundo_replace, (newRate > oldRate) ? "Upsample" : "Downsample");
	};
	auto preparePatternUndoFunc = [&]()
	{
		m_modDoc.PrepareUndoForAllPatterns(false, "Resample (Adjust Offsets)");
	};
	SmpLength newSelEnd = SampleEdit::Resample(sample, selection.nStart, selection.nEnd, newRate, mode, m_sndFile, updatePatternCommands, updatePatternNotes, prepareSampleUndoFunc, preparePatternUndoFunc);
	if(!newSelEnd)
	{
		MessageBeep(MB_ICONWARNING);
		EndWaitCursor();
		return;
	}

	SetModified(smp, SampleHint().Info().Data(), smp == m_nSample, true);
	if(updatePatternCommands || updatePatternNotes)
	{
		m_modDoc.UpdateAllViews(nullptr, PatternHint().Data(), this);
	}

	if(selection.selectionActive && !ignoreSelection)
	{
		SetSelectionPoints(selection.nStart, newSelEnd);
	}

	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnToggleTimestretchQuality()
{
	// Enable time-stretching / disable unused pitch-shifting UI elements
	BOOL isLoFi = (IsDlgButtonChecked(IDC_CHECK3) != BST_UNCHECKED) ? TRUE : FALSE;
	m_ComboGrainSize.EnableWindow(isLoFi ? TRUE : FALSE);
}

void CCtrlSamples::OnEstimateSampleSize()
{
	if(!m_sndFile.GetSample(m_nSample).HasSampleData())
		return;

	double ratio = 100.0;
	m_EditTimeStretchRatio.GetDecimalValue(ratio);
	CPSRatioCalc dlg{m_sndFile, m_nSample, ratio, this};
	if (dlg.DoModal() != IDOK)
		return;

	m_EditTimeStretchRatio.SetDecimalValue(dlg.m_ratio);
}


class DoPitchShiftTimeStretch : public CProgressDialog
{
public:
	DoPitchShiftTimeStretch(CCtrlSamples &parent, CModDoc &modDoc, SAMPLEINDEX sample, SmpLength start, SmpLength end, float pitch, float stretchRatio, int grainSize, bool loFi)
		: CProgressDialog{&parent}
		, m_updateInterval{std::max(uint32(15), TrackerSettings::Instance().GUIUpdateInterval.Get())}
	{
		const auto updateFunc = [this](SmpLength current, SmpLength maximum) { return UpdateProgress(current, maximum); };
		const auto prepareUndo = [&parent]() { return parent.PrepareUndo("Pitch Shift / Time Stretch", sundo_replace); };

		CSoundFile &sndFile = modDoc.GetSoundFile();
		if(loFi)
			m_instance = std::make_unique<TimeStretchPitchShift::LoFi>(updateFunc, prepareUndo, sndFile, sample, pitch, stretchRatio, start, end, grainSize);
		else
			m_instance = std::make_unique<TimeStretchPitchShift::Signalsmith>(updateFunc, prepareUndo, sndFile, sample, pitch, stretchRatio, start, end);
	}

	SmpLength NewSelectionEnd() const { return m_instance->NewSelectionEnd(); }

private:
	void Run() override
	{
		SetTitle(_T("Pitch Shift / Time Stretch"));
		SetRange(0, 100);

		BeginWaitCursor();
		m_result = m_instance->Process();
		EndWaitCursor();

		EndDialog((m_result == TimeStretchPitchShift::Result::OK) ? IDOK : IDCANCEL);
	}

	bool UpdateProgress(SmpLength current, SmpLength maximum)
	{
		uint32 timeNow = timeGetTime();
		if(timeNow - m_lastUpdate < m_updateInterval)
			return m_abort;

		m_lastUpdate = timeNow;
		TCHAR progress[48];
		uint32 percent = Util::muldivr(current, 100, maximum);
		wsprintf(progress, _T("Pitch Shift / Time Stretch... %u%%"), percent);
		SetText(progress);
		SetProgress(percent);
		ProcessMessages();
		return m_abort;
	}

private:
	std::unique_ptr<TimeStretchPitchShift::Base> m_instance;
	const uint32 m_updateInterval;
	uint32 m_lastUpdate = 0;
public:
	TimeStretchPitchShift::Result m_result = TimeStretchPitchShift::Result::Abort;
};


void CCtrlSamples::OnPitchShiftTimeStretch()
{
	auto errorCode = TimeStretchPitchShift::Result::Abort;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if(!sample.HasSampleData())
		return;
	
	CString text;
	GetDlgItem(IDC_COMBO4)->GetWindowText(text);
	const float semitones = mpt::parse<float>(text);
	const float pitch = std::pow(2.0f, semitones / 12.0f);

	double ratio = 100.0;
	m_EditTimeStretchRatio.GetDecimalValue(ratio);

	const auto grainSize = static_cast<int>(GetDlgItemInt(IDC_COMBO5));

	if(pitch != 1.0f || ratio != 100.0)
	{
		auto selection = GetSelectionPoints();
		DoPitchShiftTimeStretch timeStretch(*this, m_modDoc, m_nSample, selection.nStart, selection.nEnd, pitch, static_cast<float>(ratio / 100.0), grainSize, IsDlgButtonChecked(IDC_CHECK3) != BST_UNCHECKED);
		timeStretch.DoModal();
		errorCode = timeStretch.m_result;
		if(selection.selectionActive)
			SetSelectionPoints(selection.nStart, timeStretch.NewSelectionEnd());

	}

	if(errorCode == TimeStretchPitchShift::Result::OK)
	{
		// Update sample view
		SetModified(SampleHint().Info().Data(), true, true);
		SwitchToView();
		return;
	}

	// Error management
	if(errorCode != TimeStretchPitchShift::Result::Abort)
	{
		CString str;
		switch(errorCode)
		{
		case TimeStretchPitchShift::Result::OK:
		case TimeStretchPitchShift::Result::Abort:
			break;
		case TimeStretchPitchShift::Result::StretchTooShort:
			str = _T("Stretch ratio is too low, not leaving any sample data.");
			break;
		case TimeStretchPitchShift::Result::StretchTooLong:
			str = _T("Stretch ratio is too high, exceeding maximum sample length.");
			break;
		case TimeStretchPitchShift::Result::InvalidGrainSize:
			str = _T("Grain size too small.");
			break;
		case TimeStretchPitchShift::Result::SampleTooShort:
			str = _T("Sample too short.");
			break;
		case TimeStretchPitchShift::Result::OutOfMemory:
			str = _T("Out of memory.");
			break;
		}
		Reporting::Error(str);
		SwitchToView();
	}
}


void CCtrlSamples::OnReverse()
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	SampleSelectionPoints selection = GetSelectionPoints();

	PrepareUndo("Reverse", sundo_reverse, selection.nStart, selection.nEnd);
	if(SampleEdit::ReverseSample(sample, selection.nStart, selection.nEnd, m_sndFile))
	{
		SetModified(SampleHint().Data(), false, true);
	} else
	{
		m_modDoc.GetSampleUndo().RemoveLastUndoStep(m_nSample);
	}
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnInvert()
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	SampleSelectionPoints selection = GetSelectionPoints();

	PrepareUndo("Invert", sundo_invert, selection.nStart, selection.nEnd);
	if(SampleEdit::InvertSample(sample, selection.nStart, selection.nEnd, m_sndFile) == true)
	{
		SetModified(SampleHint().Data(), false, true);
	} else
	{
		m_modDoc.GetSampleUndo().RemoveLastUndoStep(m_nSample);
	}
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnSignUnSign()
{
	if(!m_sndFile.GetSample(m_nSample).HasSampleData()) return;

	BeginWaitCursor();
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SampleSelectionPoints selection = GetSelectionPoints();

	PrepareUndo("Unsign", sundo_unsign, selection.nStart, selection.nEnd);
	if(SampleEdit::UnsignSample(sample, selection.nStart, selection.nEnd, m_sndFile) == true)
	{
		SetModified(SampleHint().Data(), false, true);
	} else
	{
		m_modDoc.GetSampleUndo().RemoveLastUndoStep(m_nSample);
	}
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnSilence()
{
	if(!m_sndFile.GetSample(m_nSample).HasSampleData()) return;
	BeginWaitCursor();
	SampleSelectionPoints selection = GetSelectionPoints();

	// never apply silence to a sample that has no selection
	const SmpLength len = selection.nEnd - selection.nStart;
	if(selection.selectionActive && len > 1)
	{
		ModSample &sample = m_sndFile.GetSample(m_nSample);
		PrepareUndo("Silence", sundo_update, selection.nStart, selection.nEnd);
		if(SampleEdit::SilenceSample(sample, selection.nStart, selection.nEnd, m_sndFile))
		{
			SetModified(SampleHint().Data(), false, true);
		}
	}

	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnPrevInstrument()
{
	if (m_nSample > 1)
		SetCurrentSample(m_nSample - 1);
	else
		SetCurrentSample(m_sndFile.GetNumSamples());
}


void CCtrlSamples::OnNextInstrument()
{
	if (m_nSample < m_sndFile.GetNumSamples())
		SetCurrentSample(m_nSample + 1);
	else
		SetCurrentSample(1);
}


void CCtrlSamples::OnNameChanged()
{
	if(IsLocked() || !m_nSample) return;
	CString tmp;
	m_EditName.GetWindowText(tmp);
	const std::string s = mpt::ToCharset(m_sndFile.GetCharsetInternal(), tmp);
	if(s != m_sndFile.m_szNames[m_nSample])
	{
		if(!m_startedEdit)
		{
			PrepareUndo("Set Name");
			m_editInstrumentName = m_modDoc.GetParentInstrumentWithSameName(m_nSample);
			if(m_editInstrumentName != INSTRUMENTINDEX_INVALID)
				m_modDoc.GetInstrumentUndo().PrepareUndo(m_editInstrumentName, "Set Name");
		}
		if(m_editInstrumentName <= m_sndFile.GetNumInstruments() && m_editInstrumentName < MAX_INSTRUMENTS)
		{
			if(auto instr = m_sndFile.Instruments[m_editInstrumentName]; instr != nullptr)
			{
				instr->name = s;
				m_modDoc.UpdateAllViews(nullptr, InstrumentHint(m_editInstrumentName).Names(), this);
			}
		}

		m_sndFile.m_szNames[m_nSample] = s;
		SetModified(SampleHint().Names(), false, false);
	}
}


void CCtrlSamples::OnFileNameChanged()
{
	if(IsLocked()) return;
	CString tmp;
	m_EditFileName.GetWindowText(tmp);
	const std::string s = mpt::ToCharset(m_sndFile.GetCharsetInternal(), tmp);
	if(s != m_sndFile.GetSample(m_nSample).filename)
	{
		if(!m_startedEdit) PrepareUndo("Set Filename");
		m_sndFile.GetSample(m_nSample).filename = s;
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnVolumeChanged()
{
	if (IsLocked()) return;
	int nVol = GetDlgItemInt(IDC_EDIT7);
	Limit(nVol, 0, 64);
	nVol *= 4;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if (nVol != sample.nVolume)
	{
		if(!m_startedEdit) PrepareUndo("Set Default Volume");
		sample.nVolume = static_cast<uint16>(nVol);
		sample.uFlags.reset(SMP_NODEFAULTVOLUME);
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnGlobalVolChanged()
{
	if (IsLocked()) return;
	int nVol = GetDlgItemInt(IDC_EDIT8);
	Limit(nVol, 0, 64);
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if (nVol != sample.nGlobalVol)
	{
		if(!m_startedEdit) PrepareUndo("Set Global Volume");
		sample.nGlobalVol = static_cast<uint16>(nVol);
		// Live-adjust volume
		for(auto &chn : m_sndFile.m_PlayState.Chn)
		{
			if(chn.pModSample == &sample)
			{
				chn.UpdateInstrumentVolume(chn.pModSample, chn.pModInstrument);
			}
		}
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnSetPanningChanged()
{
	if (IsLocked()) return;
	bool b = false;
	if (m_sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))
	{
		b = IsDlgButtonChecked(IDC_CHECK1) != FALSE;
	}

	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if(b != sample.uFlags[CHN_PANNING])
	{
		PrepareUndo("Toggle Panning");
		sample.uFlags.set(CHN_PANNING, b);
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnPanningChanged()
{
	if (IsLocked()) return;
	int nPan = GetDlgItemInt(IDC_EDIT9);
	if (nPan < 0) nPan = 0;

	if (m_sndFile.GetType() == MOD_TYPE_XM)
	{
		if (nPan > 255) nPan = 255;	// displayed panning will be 0-255 with XM
	} else
	{
		if (nPan > 64) nPan = 64;	// displayed panning will be 0-64 with anything but XM.
		nPan = nPan * 4;			// so we x4 to get MPT's internal 0-256 range.
	}

	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if (nPan != sample.nPan)
	{
		if(!m_startedEdit) PrepareUndo("Set Panning");
		sample.nPan = static_cast<uint16>(nPan);
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnFineTuneChanged()
{
	if (IsLocked()) return;
	int n = GetDlgItemInt(IDC_EDIT5);
	if(!m_startedEdit)
		PrepareUndo("Finetune");
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if (!m_sndFile.UseFinetuneAndTranspose())
	{
		if ((n > 0) && (n <= (m_sndFile.GetType() == MOD_TYPE_S3M ? 65535 : 9999999)) && (n != (int)m_sndFile.GetSample(m_nSample).nC5Speed))
		{
			sample.nC5Speed = n;
			int transp = GetSampleTransponse(sample);
			int basenote = (NOTE_MIDDLEC - NOTE_MIN) + transp;
			Clamp(basenote, BASENOTE_MIN, BASENOTE_MAX);
			basenote -= BASENOTE_MIN;
			if (basenote != m_CbnBaseNote.GetCurSel())
			{
				LockControls();
				m_CbnBaseNote.SetCurSel(basenote);
				UnlockControls();
			}
			SetModified(SampleHint().Info(), false, false);
		}
	} else
	{
		if(m_sndFile.GetType() & MOD_TYPE_MOD)
			n = MOD2XMFineTune(n);
		if((n >= -128) && (n <= 127))
		{
			sample.nFineTune = static_cast<int8>(n);
			SetModified(SampleHint().Info(), false, false);
		}
	}
}


void CCtrlSamples::OnFineTuneChangedDone()
{
	// Update all playing channels
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	for(auto &chn : m_sndFile.m_PlayState.Chn)
	{
		if(chn.pModSample == &sample)
		{
			chn.nTranspose = sample.RelativeTone;
			chn.nFineTune = sample.nFineTune;
			if(chn.nC5Speed != 0 && sample.nC5Speed != 0)
			{
				if(m_sndFile.PeriodsAreFrequencies())
					chn.nPeriod = Util::muldivr(chn.nPeriod, sample.nC5Speed, chn.nC5Speed);
				else if(!m_sndFile.m_SongFlags[SONG_LINEARSLIDES])
					chn.nPeriod = Util::muldivr(chn.nPeriod, chn.nC5Speed, sample.nC5Speed);
			}
			chn.nC5Speed = sample.nC5Speed;
		}
	}
}


void CCtrlSamples::OnBaseNoteChanged()
{
	if (IsLocked()) return;
	int n = static_cast<int>(m_CbnBaseNote.GetItemData(m_CbnBaseNote.GetCurSel()));

	ModSample &sample = m_sndFile.GetSample(m_nSample);
	PrepareUndo("Transpose");

	if(!m_sndFile.UseFinetuneAndTranspose())
	{
		const int oldTransp = GetSampleTransponse(sample);
		const uint32 newFreq = mpt::saturate_round<uint32>(sample.nC5Speed * std::pow(2.0, (n - oldTransp) / 12.0));
		if(newFreq > 0 && newFreq <= (m_sndFile.GetType() == MOD_TYPE_S3M ? 65535u : 9999999u) && newFreq != sample.nC5Speed)
		{
			sample.nC5Speed = newFreq;
			LockControls();
			SetDlgItemInt(IDC_EDIT5, newFreq, FALSE);

			// Due to rounding imprecisions if the base note is below 0, we recalculate it here to make sure that the value stays consistent.
			int basenote = (NOTE_MIDDLEC - NOTE_MIN) + GetSampleTransponse(sample);
			Limit(basenote, BASENOTE_MIN, BASENOTE_MAX);
			basenote -= BASENOTE_MIN;
			if(basenote != m_CbnBaseNote.GetCurSel())
				m_CbnBaseNote.SetCurSel(basenote);

			OnFineTuneChangedDone();
			UnlockControls();
			SetModified(SampleHint().Info(), false, false);
		}
	} else
	{
		if((n >= -128) && (n < 128))
		{
			sample.RelativeTone = (int8)n;
			OnFineTuneChangedDone();
			SetModified(SampleHint().Info(), false, false);
		}
	}
}


void CCtrlSamples::OnVibTypeChanged()
{
	if (IsLocked()) return;
	int n = m_ComboAutoVib.GetCurSel();
	if (n >= 0)
	{
		PrepareUndo("Set Vibrato Type");
		m_sndFile.GetSample(m_nSample).nVibType = static_cast<VibratoType>(m_ComboAutoVib.GetItemData(n));

		PropagateAutoVibratoChanges();
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnVibDepthChanged()
{
	if (IsLocked()) return;
	int lmin = 0, lmax = 0;
	m_SpinVibDepth.GetRange(lmin, lmax);
	int n = GetDlgItemInt(IDC_EDIT15);
	if ((n >= lmin) && (n <= lmax))
	{
		if(!m_startedEdit) PrepareUndo("Set Vibrato Depth");
		m_sndFile.GetSample(m_nSample).nVibDepth = static_cast<uint8>(n);

		PropagateAutoVibratoChanges();
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnVibSweepChanged()
{
	if (IsLocked()) return;
	int lmin = 0, lmax = 0;
	m_SpinVibSweep.GetRange(lmin, lmax);
	int n = GetDlgItemInt(IDC_EDIT14);
	if ((n >= lmin) && (n <= lmax))
	{
		if(!m_startedEdit) PrepareUndo("Set Vibrato Sweep");
		m_sndFile.GetSample(m_nSample).nVibSweep = static_cast<uint8>(n);

		PropagateAutoVibratoChanges();
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnVibRateChanged()
{
	if (IsLocked()) return;
	int lmin = 0, lmax = 0;
	m_SpinVibRate.GetRange(lmin, lmax);
	int n = GetDlgItemInt(IDC_EDIT16);
	if ((n >= lmin) && (n <= lmax))
	{
		if(!m_startedEdit) PrepareUndo("Set Vibrato Rate");
		m_sndFile.GetSample(m_nSample).nVibRate = static_cast<uint8>(n);

		PropagateAutoVibratoChanges();
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnLoopTypeChanged()
{
	if(IsLocked()) return;
	const int n = m_ComboLoopType.GetCurSel();
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	bool wasDisabled = !sample.uFlags[CHN_LOOP];

	PrepareUndo("Set Loop Type");

	// Loop type index: 0: Off, 1: On, 2: PingPong
	sample.uFlags.set(CHN_LOOP, n > 0);
	sample.uFlags.set(CHN_PINGPONGLOOP, n == 2);

	// set loop points if theren't any
	if(wasDisabled && sample.uFlags[CHN_LOOP] && sample.nLoopStart == sample.nLoopEnd)
	{
		SampleSelectionPoints selection = GetSelectionPoints();
		if(selection.selectionActive)
		{
			sample.SetLoop(selection.nStart, selection.nEnd, true, n == 2, m_sndFile);
		} else
		{
			sample.SetLoop(0, sample.nLength, true, n == 2, m_sndFile);
		}
		m_modDoc.UpdateAllViews(nullptr, SampleHint(m_nSample).Info());
	} else
	{
		sample.PrecomputeLoops(m_sndFile, true);
	}
	SetModified(SampleHint().Info(), false, false);
}


void CCtrlSamples::OnLoopPointsChanged()
{
	if(IsLocked()) return;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SmpLength start = GetDlgItemInt(IDC_EDIT1, NULL, FALSE), end = GetDlgItemInt(IDC_EDIT2, NULL, FALSE);
	if(start < end || !sample.uFlags[CHN_LOOP])
	{
		if(!m_startedEdit) PrepareUndo("Set Loop");
		const int n = m_ComboLoopType.GetCurSel();
		sample.SetLoop(start, end, n > 0, n == 2, m_sndFile);
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnSustainTypeChanged()
{
	if(IsLocked()) return;
	const int n = m_ComboSustainType.GetCurSel();
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	bool wasDisabled = !sample.uFlags[CHN_SUSTAINLOOP];

	PrepareUndo("Set Sustain Loop Type");

	// Loop type index: 0: Off, 1: On, 2: PingPong
	sample.uFlags.set(CHN_SUSTAINLOOP, n > 0);
	sample.uFlags.set(CHN_PINGPONGSUSTAIN, n == 2);

	// set sustain loop points if theren't any
	if(wasDisabled && sample.uFlags[CHN_SUSTAINLOOP] && sample.nSustainStart == sample.nSustainEnd)
	{
		SampleSelectionPoints selection = GetSelectionPoints();
		if(selection.selectionActive)
		{
			sample.SetSustainLoop(selection.nStart, selection.nEnd, true, n == 2, m_sndFile);
		} else
		{
			sample.SetSustainLoop(0, sample.nLength, true, n == 2, m_sndFile);
		}
		m_modDoc.UpdateAllViews(nullptr, SampleHint(m_nSample).Info());
	} else
	{
		sample.PrecomputeLoops(m_sndFile, true);
	}
	SetModified(SampleHint().Info(), false, false);
}


void CCtrlSamples::OnSustainPointsChanged()
{
	if(IsLocked()) return;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SmpLength start = GetDlgItemInt(IDC_EDIT3, NULL, FALSE), end = GetDlgItemInt(IDC_EDIT4, NULL, FALSE);
	if(start < end || !sample.uFlags[CHN_SUSTAINLOOP])
	{
		if(!m_startedEdit) PrepareUndo("Set Sustain Loop");
		const int n = m_ComboSustainType.GetCurSel();
		sample.SetSustainLoop(start, end, n > 0, n == 2, m_sndFile);
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnVScroll(UINT nCode, UINT, CScrollBar *scrollBar)
{
	TCHAR s[32];
	if(IsLocked()) return;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	const bool moveLoop = CMainFrame::GetInputHandler()->CtrlPressed();
	bool redraw = false;
	static CScrollBar *lastScrollbar = nullptr;

	LockControls();
	// Loop Start
	if(int pos = m_SpinLoopStart.GetPos32(); pos != 0 && sample.nLoopEnd > 0 && sample.HasSampleData())
	{
		if(SmpLength i = SampleEdit::FindLoopStart(sample, false, pos > 0, moveLoop); i < sample.nLength)
		{
			if(!m_startedEdit && lastScrollbar != scrollBar)
				PrepareUndo(moveLoop ? "Move Loop" : "Set Loop Start");
			if(moveLoop)
			{
				sample.nLoopEnd = i + (sample.nLoopEnd - sample.nLoopStart);
				wsprintf(s, _T("%u"), sample.nLoopEnd);
				m_EditLoopEnd.SetWindowText(s);
			}
			sample.nLoopStart = i;
			wsprintf(s, _T("%u"), sample.nLoopStart);
			m_EditLoopStart.SetWindowText(s);
			redraw = true;
			sample.PrecomputeLoops(m_sndFile);
		}
		m_SpinLoopStart.SetPos(0);
	}
	// Loop End
	if(int pos = m_SpinLoopEnd.GetPos32(); pos != 0 && sample.HasSampleData())
	{
		if(SmpLength i = SampleEdit::FindLoopEnd(sample, false, pos > 0, moveLoop); i > 0)
		{
			if(!m_startedEdit && lastScrollbar != scrollBar)
				PrepareUndo(moveLoop ? "Move Loop" : "Set Loop End");
			if(moveLoop)
			{
				sample.nLoopStart = i - (sample.nLoopEnd - sample.nLoopStart);
				wsprintf(s, _T("%u"), sample.nLoopStart);
				m_EditLoopStart.SetWindowText(s);
			}
			sample.nLoopEnd = i;
			wsprintf(s, _T("%u"), sample.nLoopEnd);
			m_EditLoopEnd.SetWindowText(s);
			redraw = true;
			sample.PrecomputeLoops(m_sndFile);
		}
		m_SpinLoopEnd.SetPos(0);
	}
	// Sustain Loop Start
	if(int pos = m_SpinSustainStart.GetPos32(); pos != 0 && sample.nSustainEnd > 0 && sample.HasSampleData())
	{
		if(SmpLength i = SampleEdit::FindLoopStart(sample, true, pos > 0, moveLoop); i < sample.nLength)
		{
			if(!m_startedEdit && lastScrollbar != scrollBar)
				PrepareUndo(moveLoop ? "Move Sustain Loop" : "Set Sustain Loop Start");
			if(moveLoop)
			{
				sample.nSustainEnd = i + (sample.nSustainEnd - sample.nSustainStart);
				wsprintf(s, _T("%u"), sample.nSustainEnd);
				m_EditSustainEnd.SetWindowText(s);
			}
			sample.nSustainStart = i;
			wsprintf(s, _T("%u"), sample.nSustainStart);
			m_EditSustainStart.SetWindowText(s);
			redraw = true;
			sample.PrecomputeLoops(m_sndFile);
		}
		m_SpinSustainStart.SetPos(0);
	}
	// Sustain Loop End
	if(int pos = m_SpinSustainEnd.GetPos32(); pos != 0 && sample.HasSampleData())
	{
		if(SmpLength i = SampleEdit::FindLoopEnd(sample, true, pos > 0, moveLoop); i > 0)
		{
			if(!m_startedEdit && lastScrollbar != scrollBar)
				PrepareUndo(moveLoop ? "Move Sustain Loop" : "Set Sustain Loop End");
			if(moveLoop)
			{
				sample.nSustainStart = i - (sample.nSustainEnd - sample.nSustainStart);
				wsprintf(s, _T("%u"), sample.nSustainStart);
				m_EditSustainStart.SetWindowText(s);
			}
			sample.nSustainEnd = i;
			wsprintf(s, _T("%u"), sample.nSustainEnd);
			m_EditSustainEnd.SetWindowText(s);
			redraw = true;
			sample.PrecomputeLoops(m_sndFile);
		}
		m_SpinSustainEnd.SetPos(0);
	}
	// FineTune / C-5 Speed
	if(int pos = m_SpinFineTune.GetPos32(); pos != 0)
	{
		if(!m_startedEdit && lastScrollbar != scrollBar)
			PrepareUndo("Finetune");
		SetFinetune(pos);
		m_SpinFineTune.SetPos(0);
	}
	if(nCode == SB_ENDSCROLL)
		SwitchToView();
	if(redraw)
	{
		SetModified(SampleHint().Info().Data(), false, false);
	}
	lastScrollbar = scrollBar;
	UnlockControls();
}


void CCtrlSamples::SetFinetune(int step)
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if(!m_sndFile.UseFinetuneAndTranspose())
	{
		if(sample.nC5Speed < 1)
			sample.nC5Speed = 8363;
		auto oldFreq = sample.nC5Speed;
		sample.Transpose((step * TrackerSettings::Instance().m_nFinetuneStep) / 1200.0);
		if(sample.nC5Speed == oldFreq)
			sample.nC5Speed += step;
		Limit(sample.nC5Speed, 1u, 9999999u);  // 9999999 is max. in Impulse Tracker
		int basenote = (NOTE_MIDDLEC - NOTE_MIN) + GetSampleTransponse(sample);
		Clamp(basenote, BASENOTE_MIN, BASENOTE_MAX);
		basenote -= BASENOTE_MIN;
		if(basenote != m_CbnBaseNote.GetCurSel())
			m_CbnBaseNote.SetCurSel(basenote);
		SetDlgItemInt(IDC_EDIT5, sample.nC5Speed, FALSE);
	} else
	{
		int ftune = (int)sample.nFineTune;
		// MOD finetune range -8 to 7 translates to -128 to 112
		if(m_sndFile.GetType() & MOD_TYPE_MOD)
		{
			ftune = Clamp((ftune >> 4) + step, -8, 7);
			sample.nFineTune = MOD2XMFineTune((signed char)ftune);
		} else
		{
			ftune = Clamp(ftune + step, -128, 127);
			sample.nFineTune = (signed char)ftune;
		}
		SetDlgItemInt(IDC_EDIT5, ftune, TRUE);
	}
	OnFineTuneChangedDone();
	SetModified(SampleHint().Info().Data(), false, false);
}


BOOL CCtrlSamples::PreTranslateMessage(MSG *pMsg)
{
	if (pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) ||
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler* ih = CMainFrame::GetInputHandler();
			if (ih->KeyEvent(kCtxViewSamples, ih->Translate(*pMsg)) != kcNull)
				return true; // Mapped to a command, no need to pass message on.
		}

	}
	return CModControlDlg::PreTranslateMessage(pMsg);
}


LRESULT CCtrlSamples::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
{
	int transpose = 0;
	switch(wParam)
	{
	case kcSampleLoad:		OnSampleOpen(); return wParam;
	case kcSampleLoadRaw:	OnSampleOpenRaw(); return wParam;
	case kcSampleSave:		OnSampleSaveOne(); return wParam;
	case kcSampleNew:		InsertSample(false); return wParam;
	case kcSampleDuplicate:	InsertSample(true); return wParam;

	case kcSampleTransposeUp: transpose = 1; break;
	case kcSampleTransposeDown: transpose = -1; break;
	case kcSampleTransposeOctUp: transpose = 12; break;
	case kcSampleTransposeOctDown: transpose = -12; break;
	case kcSampleFinetuneUp: SetFinetune(1); break;
	case kcSampleFinetuneDown: SetFinetune(-1); break;

	case kcSampleUpsample:
	case kcSampleDownsample:
		{
			uint32 oldRate = m_sndFile.GetSample(m_nSample).GetSampleRate(m_sndFile.GetType());
			ApplyResample(m_nSample, wParam == kcSampleUpsample ? oldRate * 2 : oldRate / 2, TrackerSettings::Instance().sampleEditorDefaultResampler);
		}
		return wParam;
	case kcSampleResample:
		OnResample();
		return wParam;
	case kcSampleStereoSep:
		OnStereoSeparation();
		return wParam;
	case kcSampleInitializeOPL:
		OnInitOPLInstrument();
		return wParam;
	}

	if(transpose)
	{
		if(m_CbnBaseNote.IsWindowEnabled())
		{
			int sel = Clamp(m_CbnBaseNote.GetCurSel() + transpose, 0, m_CbnBaseNote.GetCount() - 1);
			if(sel != m_CbnBaseNote.GetCurSel())
			{
				m_CbnBaseNote.SetCurSel(sel);
				OnBaseNoteChanged();
			}
		}
		return wParam;
	}

	return kcNull;
}


// Return currently selected part of the sample.
// The whole sample size will be returned if no part of the sample is selected.
// However, points.selectionActive indicates whether a sample selection exists or not.
CCtrlSamples::SampleSelectionPoints CCtrlSamples::GetSelectionPoints()
{
	SampleSelectionPoints points;
	SampleViewState viewstate{};
	const ModSample &sample = m_sndFile.GetSample(m_nSample);

	SendViewMessage(VIEWMSG_SAVESTATE, reinterpret_cast<LPARAM>(&viewstate));
	points.nStart = viewstate.dwBeginSel;
	points.nEnd = viewstate.dwEndSel;
	if(points.nEnd > sample.nLength) points.nEnd = sample.nLength;
	if(points.nStart > points.nEnd) points.nStart = points.nEnd;
	points.selectionActive = true;
	if(points.nStart >= points.nEnd)
	{
		points.nStart = 0;
		points.nEnd = sample.nLength;
		points.selectionActive = false;
	}
	return points;
}

// Set the currently selected part of the sample.
// To reset the selection, use nStart = nEnd = 0.
void CCtrlSamples::SetSelectionPoints(SmpLength nStart, SmpLength nEnd)
{
	const ModSample &sample = m_sndFile.GetSample(m_nSample);

	Limit(nStart, SmpLength(0), sample.nLength);
	Limit(nEnd, SmpLength(0), sample.nLength);

	SampleViewState viewstate{};
	SendViewMessage(VIEWMSG_SAVESTATE, reinterpret_cast<LPARAM>(&viewstate));

	viewstate.dwBeginSel = nStart;
	viewstate.dwEndSel = nEnd;
	SendViewMessage(VIEWMSG_LOADSTATE, reinterpret_cast<LPARAM>(&viewstate));
}


// Crossfade loop to create smooth loop transitions
void CCtrlSamples::OnXFade()
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	if(!sample.HasSampleData())
	{
		MessageBeep(MB_ICONWARNING);
		SwitchToView();
		return;
	}
	bool resetLoopOnCancel = false;
	if((sample.nLoopEnd <= sample.nLoopStart || sample.nLoopEnd > sample.nLength)
		&& (sample.nSustainEnd <= sample.nSustainStart || sample.nSustainEnd > sample.nLength))
	{
		const auto selection = GetSelectionPoints();
		if(selection.nStart > 0 && selection.nEnd > selection.nStart)
		{
			sample.SetLoop(selection.nStart, selection.nEnd, true, false, m_sndFile);
			resetLoopOnCancel = true;
		} else
		{
			Reporting::Error("Crossfade requires a sample loop to work.", this);
			SwitchToView();
			return;
		}
	}
	if(sample.nLoopStart == 0 && sample.nSustainStart == 0)
	{
		Reporting::Error("Crossfade requires the sample to have data before the loop start.", this);
		SwitchToView();
		return;
	}

	CSampleXFadeDlg dlg(this, sample);
	if(dlg.DoModal() == IDOK)
	{
		const auto [loopStart, loopEnd] = dlg.m_useSustainLoop ? sample.GetSustainLoop() : sample.GetLoop();
		const SmpLength maxSamples = std::min({ sample.nLength, loopStart, loopEnd / 2 });
		SmpLength fadeSamples = dlg.PercentToSamples(dlg.m_fadeLength);
		LimitMax(fadeSamples, maxSamples);
		if(fadeSamples < 2) return;

		PrepareUndo("Crossfade", sundo_update,
			loopEnd - fadeSamples,
			loopEnd + (dlg.m_afterloopFade ? std::min(sample.nLength - loopEnd, fadeSamples) : 0));

		if(SampleEdit::XFadeSample(sample, fadeSamples, dlg.m_fadeLaw, dlg.m_afterloopFade, dlg.m_useSustainLoop, m_sndFile))
		{
			SetModified(SampleHint().Info().Data(), true, true);
		} else
		{
			m_modDoc.GetSampleUndo().RemoveLastUndoStep(m_nSample);
		}
	} else if(resetLoopOnCancel)
	{
		sample.SetLoop(0, 0, false, false, m_sndFile);
	}
	SwitchToView();
}


void CCtrlSamples::OnStereoSeparation()
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	if(!sample.HasSampleData()
		|| sample.GetNumChannels() != 2
		|| sample.uFlags[CHN_ADLIB])
	{
		MessageBeep(MB_ICONWARNING);
		SwitchToView();
		return;
	}

	static double separation = 100.0;
	CInputDlg dlg(this, _T("Stereo separation amount\n0% = mono, 100% = no change, 200% = double separation\nNegative values swap channels"), -200.0, 200.0, separation);
	if(dlg.DoModal() == IDOK)
	{
		separation = dlg.resultAsDouble;

		SampleSelectionPoints selection = GetSelectionPoints();
		PrepareUndo("Stereo Separation", sundo_update,
			selection.nStart, selection.nEnd);

		if(SampleEdit::StereoSepSample(sample, selection.nStart, selection.nEnd, separation, m_sndFile))
		{
			SetModified(SampleHint().Info().Data(), true, true);
		} else
		{
			m_modDoc.GetSampleUndo().RemoveLastUndoStep(m_nSample);
		}
	}
	SwitchToView();
}


void CCtrlSamples::OnAutotune()
{
	SampleSelectionPoints selection = GetSelectionPoints();
	if(!selection.selectionActive)
	{
		selection.nStart = selection.nEnd = 0;
	}

	ModSample &sample = m_sndFile.GetSample(m_nSample);
	Autotune at(sample, m_sndFile.GetType(), selection.nStart, selection.nEnd);
	if(at.CanApply())
	{
		CAutotuneDlg dlg(this);
		if(dlg.DoModal() == IDOK)
		{
			BeginWaitCursor();
			PrepareUndo("Automatic Sample Tuning");
			bool modified = true;
			if(IsOPLInstrument())
			{
				const uint32 newFreq = mpt::saturate_round<uint32>(dlg.GetPitchReference() * (8363.0 / 440.0) * std::pow(2.0, dlg.GetTargetNote() / 12.0));
				modified = (newFreq != sample.nC5Speed);
				sample.nC5Speed = newFreq;
			} else
			{
				modified = at.Apply(static_cast<double>(dlg.GetPitchReference()), dlg.GetTargetNote());
			}
			OnFineTuneChangedDone();
			if(modified)
				SetModified(SampleHint().Info(), true, false);
			EndWaitCursor();
		}
	}
	SwitchToView();
}


void CCtrlSamples::OnKeepSampleOnDisk()
{
	SAMPLEINDEX first = m_nSample, last = m_nSample;
	if(CInputHandler::ShiftPressed())
	{
		first = 1;
		last = m_sndFile.GetNumSamples();
	}

	const bool enable = IsDlgButtonChecked(IDC_CHECK2) != BST_UNCHECKED;
	for(SAMPLEINDEX i = first; i <= last; i++)
	{
		if(bool newState = enable && m_sndFile.SampleHasPath(i); newState != m_sndFile.GetSample(i).uFlags[SMP_KEEPONDISK])
		{
			m_sndFile.GetSample(i).uFlags.set(SMP_KEEPONDISK, newState);
			m_modDoc.UpdateAllViews(nullptr, SampleHint(i).Info().Names(), this);
		}
	}
	m_modDoc.SetModified();
}


// When changing auto vibrato properties, propagate them to other samples of the same instrument in XM edit mode.
void CCtrlSamples::PropagateAutoVibratoChanges()
{
	if(!(m_sndFile.GetType() & MOD_TYPE_XM))
	{
		return;
	}

	for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
	{
		if(m_sndFile.IsSampleReferencedByInstrument(m_nSample, i))
		{
			const auto referencedSamples = m_sndFile.Instruments[i]->GetSamples();

			// Propagate changes to all samples that belong to this instrument.
			const ModSample &it = m_sndFile.GetSample(m_nSample);
			m_sndFile.PropagateXMAutoVibrato(i, it.nVibType, it.nVibSweep, it.nVibDepth, it.nVibRate);
			for(auto smp : referencedSamples)
			{
				m_modDoc.UpdateAllViews(nullptr, SampleHint(smp).Info(), this);
			}
		}
	}
}


void CCtrlSamples::OnXButtonUp(UINT nFlags, UINT nButton, CPoint point)
{
	if(nButton == XBUTTON1) OnPrevInstrument();
	else if(nButton == XBUTTON2) OnNextInstrument();
	CModControlDlg::OnXButtonUp(nFlags, nButton, point);
	SwitchToView();
}


bool CCtrlSamples::IsOPLInstrument() const
{
	return m_nSample >= 1 && m_nSample <= m_sndFile.GetNumSamples() && m_sndFile.GetSample(m_nSample).uFlags[CHN_ADLIB];
}


void CCtrlSamples::OnInitOPLInstrument()
{
	if(m_sndFile.SupportsOPL())
	{
		CriticalSection cs;
		PrepareUndo("Initialize OPL Instrument", sundo_replace);
		m_sndFile.DestroySample(m_nSample);
		m_sndFile.InitOPL();
		ModSample &sample = m_sndFile.GetSample(m_nSample);
		sample.nC5Speed = 8363;
		// Initialize with instant attack, release and enabled sustain for carrier and instant attack for modulator
		sample.SetAdlib(true, { 0x00, 0x20, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00 });
		SetModified(SampleHint().Info().Data().Names(), true, true);
		SwitchToView();
	}
}


OPENMPT_NAMESPACE_END
