/*
 * ctrl_smp.cpp
 * ------------
 * Purpose: Sample tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "Childfrm.h"
#include "ImageLists.h"
#include "Moddoc.h"
#include "../soundlib/mod_specifications.h"
#include "Globals.h"
#include "Ctrl_smp.h"
#include "View_smp.h"
#include "SampleEditorDialogs.h"
#include "dlg_misc.h"
#include "PSRatioCalc.h"
#include <soundtouch/include/SoundTouch.h>
#include <soundtouch/source/SoundTouchDLL/SoundTouchDLL.h>
#include <smbPitchShift/smbPitchShift.h>
#include "modsmp_ctrl.h"
#include "Autotune.h"
#include "../common/StringFixer.h"
#include "../common/mptFileIO.h"
#include "../common/FileReader.h"
#include "../soundbase/SampleFormatConverters.h"
#include "../soundbase/SampleFormatCopy.h"
#include "FileDialog.h"
#include "ProgressDialog.h"
#include "../common/ComponentManager.h"
#ifdef _DEBUG
#include <cmath>
#endif

#include "../include/r8brain/CDSPResampler.h"
#include "../soundlib/MixFuncTable.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


OPENMPT_NAMESPACE_BEGIN


template<unsigned int v>
struct PowerOf2Exponent { enum { value = 1 + PowerOf2Exponent<v / 2>::value }; };
template<>
struct PowerOf2Exponent<1> { enum { value = 0 }; };


#define	BASENOTE_MIN	(1*12)		// C-1
#define	BASENOTE_MAX	(10*12+11)	// B-10

BEGIN_MESSAGE_MAP(CCtrlSamples, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlSamples)
	ON_WM_VSCROLL()
	ON_WM_XBUTTONUP()
	ON_NOTIFY(TBN_DROPDOWN, IDC_TOOLBAR1, OnTbnDropDownToolBar)
	ON_COMMAND(IDC_SAMPLE_NEW,			OnSampleNew)
	ON_COMMAND(IDC_SAMPLE_DUPLICATE,	OnSampleDuplicate)
	ON_COMMAND(IDC_SAMPLE_OPEN,			OnSampleOpen)
	ON_COMMAND(IDC_SAMPLE_OPENKNOWN,	OnSampleOpenKnown)
	ON_COMMAND(IDC_SAMPLE_OPENRAW,		OnSampleOpenRaw)
	ON_COMMAND(IDC_SAMPLE_SAVEAS,		OnSampleSave)
	ON_COMMAND(IDC_SAMPLE_PLAY,			OnSamplePlay)
	ON_COMMAND(IDC_SAMPLE_NORMALIZE,	OnNormalize)
	ON_COMMAND(IDC_SAMPLE_AMPLIFY,		OnAmplify)
	ON_COMMAND(IDC_SAMPLE_RESAMPLE,		OnResample)
	ON_COMMAND(IDC_SAMPLE_REVERSE,		OnReverse)
	ON_COMMAND(IDC_SAMPLE_SILENCE,		OnSilence)
	ON_COMMAND(IDC_SAMPLE_INVERT,		OnInvert)
	ON_COMMAND(IDC_SAMPLE_SIGN_UNSIGN,	OnSignUnSign)
	ON_COMMAND(IDC_SAMPLE_DCOFFSET,		OnRemoveDCOffset)
	ON_COMMAND(IDC_SAMPLE_XFADE,		OnXFade)
	ON_COMMAND(IDC_SAMPLE_AUTOTUNE,		OnAutotune)
	ON_COMMAND(IDC_CHECK1,				OnSetPanningChanged)
	ON_COMMAND(IDC_CHECK2,				OnKeepSampleOnDisk)
	ON_COMMAND(ID_PREVINSTRUMENT,		OnPrevInstrument)
	ON_COMMAND(ID_NEXTINSTRUMENT,		OnNextInstrument)
	ON_COMMAND(IDC_BUTTON1,				OnPitchShiftTimeStretch)
	ON_COMMAND(IDC_BUTTON2,				OnEstimateSampleSize)
	ON_COMMAND(IDC_CHECK3,				OnEnableStretchToSize)
	ON_EN_CHANGE(IDC_SAMPLE_NAME,		OnNameChanged)
	ON_EN_CHANGE(IDC_SAMPLE_FILENAME,	OnFileNameChanged)
	ON_EN_CHANGE(IDC_EDIT_SAMPLE,		OnSampleChanged)
	ON_EN_CHANGE(IDC_EDIT1,				OnLoopPointsChanged)
	ON_EN_CHANGE(IDC_EDIT2,				OnLoopPointsChanged)
	ON_EN_CHANGE(IDC_EDIT3,				OnSustainPointsChanged)
	ON_EN_CHANGE(IDC_EDIT4,				OnSustainPointsChanged)
	ON_EN_CHANGE(IDC_EDIT5,				OnFineTuneChanged)
	ON_EN_CHANGE(IDC_EDIT7,				OnVolumeChanged)
	ON_EN_CHANGE(IDC_EDIT8,				OnGlobalVolChanged)
	ON_EN_CHANGE(IDC_EDIT9,				OnPanningChanged)
	ON_EN_CHANGE(IDC_EDIT14,			OnVibSweepChanged)
	ON_EN_CHANGE(IDC_EDIT15,			OnVibDepthChanged)
	ON_EN_CHANGE(IDC_EDIT16,			OnVibRateChanged)
	ON_EN_KILLFOCUS(IDC_EDIT5,			OnFineTuneChangedDone)
	ON_CBN_SELCHANGE(IDC_COMBO_BASENOTE,OnBaseNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_ZOOM,	OnZoomChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1,		OnLoopTypeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,		OnSustainTypeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,		OnVibTypeChanged)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,		OnCustomKeyMsg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CCtrlSamples::DoDataExchange(CDataExchange* pDX)
//---------------------------------------------------
{
	CModControlDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCtrlSamples)
	DDX_Control(pDX, IDC_TOOLBAR1,				m_ToolBar1);
	DDX_Control(pDX, IDC_TOOLBAR2,				m_ToolBar2);
	DDX_Control(pDX, IDC_SAMPLE_NAME,			m_EditName);
	DDX_Control(pDX, IDC_SAMPLE_FILENAME,		m_EditFileName);
	DDX_Control(pDX, IDC_SAMPLE_NAME,			m_EditName);
	DDX_Control(pDX, IDC_SAMPLE_FILENAME,		m_EditFileName);
	DDX_Control(pDX, IDC_COMBO_ZOOM,			m_ComboZoom);
	DDX_Control(pDX, IDC_COMBO_BASENOTE,		m_CbnBaseNote);
	DDX_Control(pDX, IDC_SPIN_SAMPLE,			m_SpinSample);
	DDX_Control(pDX, IDC_EDIT_SAMPLE,			m_EditSample);
	DDX_Control(pDX, IDC_CHECK1,				m_CheckPanning);
	DDX_Control(pDX, IDC_SPIN1,					m_SpinLoopStart);
	DDX_Control(pDX, IDC_SPIN2,					m_SpinLoopEnd);
	DDX_Control(pDX, IDC_SPIN3,					m_SpinSustainStart);
	DDX_Control(pDX, IDC_SPIN4,					m_SpinSustainEnd);
	DDX_Control(pDX, IDC_SPIN5,					m_SpinFineTune);
	DDX_Control(pDX, IDC_SPIN7,					m_SpinVolume);
	DDX_Control(pDX, IDC_SPIN8,					m_SpinGlobalVol);
	DDX_Control(pDX, IDC_SPIN9,					m_SpinPanning);
	DDX_Control(pDX, IDC_SPIN11,				m_SpinVibSweep);
	DDX_Control(pDX, IDC_SPIN12,				m_SpinVibDepth);
	DDX_Control(pDX, IDC_SPIN13,				m_SpinVibRate);
	DDX_Control(pDX, IDC_COMBO1,				m_ComboLoopType);
	DDX_Control(pDX, IDC_COMBO2,				m_ComboSustainType);
	DDX_Control(pDX, IDC_COMBO3,				m_ComboAutoVib);
	DDX_Control(pDX, IDC_EDIT1,					m_EditLoopStart);
	DDX_Control(pDX, IDC_EDIT2,					m_EditLoopEnd);
	DDX_Control(pDX, IDC_EDIT3,					m_EditSustainStart);
	DDX_Control(pDX, IDC_EDIT4,					m_EditSustainEnd);
	DDX_Control(pDX, IDC_EDIT5,					m_EditFineTune);
	DDX_Control(pDX, IDC_EDIT7,					m_EditVolume);
	DDX_Control(pDX, IDC_EDIT8,					m_EditGlobalVol);
	DDX_Control(pDX, IDC_EDIT9,					m_EditPanning);
	DDX_Control(pDX, IDC_EDIT14,				m_EditVibSweep);
	DDX_Control(pDX, IDC_EDIT15,				m_EditVibDepth);
	DDX_Control(pDX, IDC_EDIT16,				m_EditVibRate);
	DDX_Control(pDX, IDC_COMBO4,				m_ComboPitch);
	DDX_Control(pDX, IDC_COMBO5,				m_ComboQuality);
	DDX_Control(pDX, IDC_COMBO6,				m_ComboFFT);
	DDX_Text(pDX,	 IDC_EDIT6,					m_dTimeStretchRatio); //rewbs.timeStretchMods
	//}}AFX_DATA_MAP
}


CCtrlSamples::CCtrlSamples(CModControlView &parent, CModDoc &document) :
//----------------------------------------------------------------------
	CModControlDlg(parent, document),
	m_nSequenceMs(0),
	m_nSeekWindowMs(0),
	m_nOverlapMs(0),
	m_nPreviousRawFormat(SampleIO::_8bit, SampleIO::mono, SampleIO::littleEndian, SampleIO::unsignedPCM),
	rememberRawFormat(false),
	m_nSample(1)
{
	m_nLockCount = 1;
}



CCtrlSamples::~CCtrlSamples()
//---------------------------
{
}



CRuntimeClass *CCtrlSamples::GetAssociatedViewClass()
//---------------------------------------------------
{
	return RUNTIME_CLASS(CViewSample);
}


BOOL CCtrlSamples::OnInitDialog()
//-------------------------------
{
	CModControlDlg::OnInitDialog();
	m_bInitialized = FALSE;
	finetuneBoxActive = false;
	SetRedraw(FALSE);

	// Zoom Selection
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("Auto")), 0);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("1:1")), 1);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("2:1")), (DWORD_PTR)-2);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("4:1")), (DWORD_PTR)-3);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("8:1")), (DWORD_PTR)-4);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("16:1")), (DWORD_PTR)-5);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("32:1")), (DWORD_PTR)-6);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("1:2")), 2);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("1:4")), 3);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("1:8")), 4);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("1:16")), 5);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("1:32")), 6);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("1:64")), 7);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString(_T("1:128")), 8);
	m_ComboZoom.SetCurSel(0);
	// File ToolBar
	m_ToolBar1.SetExtendedStyle(m_ToolBar1.GetExtendedStyle() | TBSTYLE_EX_DRAWDDARROWS);
	m_ToolBar1.Init(CMainFrame::GetMainFrame()->m_PatternIcons,CMainFrame::GetMainFrame()->m_PatternIconsDisabled);
	m_ToolBar1.AddButton(IDC_SAMPLE_NEW, TIMAGE_SAMPLE_NEW, TBSTYLE_BUTTON | TBSTYLE_DROPDOWN);
	m_ToolBar1.AddButton(IDC_SAMPLE_OPEN, TIMAGE_OPEN, TBSTYLE_BUTTON | TBSTYLE_DROPDOWN);
	m_ToolBar1.AddButton(IDC_SAMPLE_SAVEAS, TIMAGE_SAVE);
	// Edit ToolBar
	m_ToolBar2.Init(CMainFrame::GetMainFrame()->m_PatternIcons,CMainFrame::GetMainFrame()->m_PatternIconsDisabled);
	m_ToolBar2.AddButton(IDC_SAMPLE_PLAY, TIMAGE_PREVIEW);
	m_ToolBar2.AddButton(IDC_SAMPLE_NORMALIZE, TIMAGE_SAMPLE_NORMALIZE);
	m_ToolBar2.AddButton(IDC_SAMPLE_AMPLIFY, TIMAGE_SAMPLE_AMPLIFY);
	m_ToolBar2.AddButton(IDC_SAMPLE_DCOFFSET, TIMAGE_SAMPLE_DCOFFSET);
	m_ToolBar2.AddButton(IDC_SAMPLE_RESAMPLE, TIMAGE_SAMPLE_RESAMPLE);
	//m_ToolBar2.AddButton(IDC_SAMPLE_DOWNSAMPLE, TIMAGE_SAMPLE_DOWNSAMPLE);
	m_ToolBar2.AddButton(IDC_SAMPLE_REVERSE, TIMAGE_SAMPLE_REVERSE);
	m_ToolBar2.AddButton(IDC_SAMPLE_SILENCE, TIMAGE_SAMPLE_SILENCE);
	m_ToolBar2.AddButton(IDC_SAMPLE_INVERT, TIMAGE_SAMPLE_INVERT);
	m_ToolBar2.AddButton(IDC_SAMPLE_SIGN_UNSIGN, TIMAGE_SAMPLE_UNSIGN);
	m_ToolBar2.AddButton(IDC_SAMPLE_XFADE, TIMAGE_SAMPLE_FIXLOOP);
	m_ToolBar2.AddButton(IDC_SAMPLE_AUTOTUNE, TIMAGE_SAMPLE_AUTOTUNE);
	// Setup Controls
	m_SpinVolume.SetRange(0, 64);
	m_SpinGlobalVol.SetRange(0, 64);

	for (ModCommand::NOTE i = BASENOTE_MIN; i <= BASENOTE_MAX; i++)
	{
		std::string noteName = CSoundFile::m_NoteNames[i % 12] + mpt::ToString(i / 12);
		m_CbnBaseNote.SetItemData(m_CbnBaseNote.AddString(noteName.c_str()), i - (NOTE_MIDDLEC - NOTE_MIN));
	}

	m_ComboFFT.ShowWindow(SW_SHOW);
	m_ComboPitch.ShowWindow(SW_SHOW);
	m_ComboQuality.ShowWindow(SW_SHOW);
	m_ComboFFT.ShowWindow(SW_SHOW);

	GetDlgItem(IDC_BUTTON1)->ShowWindow(SW_SHOW); // PitchShiftTimeStretch
	GetDlgItem(IDC_BUTTON2)->ShowWindow(SW_SHOW); // EstimateSampleSize
	GetDlgItem(IDC_CHECK3)->ShowWindow(SW_SHOW);  // EnableStretchToSize
	GetDlgItem(IDC_EDIT6)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_GROUPBOX_PITCH_TIME)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_TEXT_PITCH)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_TEXT_QUALITY)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_TEXT_FFT)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_GROUPBOX_PITCH_TIME)->ShowWindow(SW_SHOW);

	TCHAR str[16];

	// Pitch selection
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO4);
	if(combo)
	{
		// Allow pitch from -12 (1 octave down) to +12 (1 octave up)
		for(int i = -12 ; i <= 12 ; i++)
		{
			if(i == 0) _tcscpy(str, _T("none"));
			else wsprintf(str, i < 0 ? _T(" %d") : _T("+%d"), i);
			combo->SetItemData(combo->AddString(str), i + 12);
		}
		// Set "none" as default pitch
		combo->SetCurSel(12);
	}

	// Quality selection
	combo = (CComboBox *)GetDlgItem(IDC_COMBO5);
	if(combo)
	{
		// Allow quality from 4 to 128
		for(int i = 4 ; i <= 128 ; i++)
		{
			wsprintf(str, _T("%u"), i);
			combo->SetItemData(combo->AddString(str), i-4);
		}
		// Set 32 as default quality
		combo->SetCurSel(32 - 4);
	}

	// FFT size selection
	combo = (CComboBox *)GetDlgItem(IDC_COMBO6);
	if(combo)
	{
		// Deduce exponent from equation : MAX_FRAME_LENGTH = 2^exponent
		const int exponent = PowerOf2Exponent<MAX_FRAME_LENGTH>::value;
		// Allow FFT size from 2^8 (256) to 2^exponent (MAX_FRAME_LENGTH)
		for(int i = 8 ; i <= exponent ; i++)
		{
			wsprintf(str, _T("%u"), 1 << i);
			combo->SetItemData(combo->AddString(str), i - 8);
		}
		// Set 4096 as default FFT size
		combo->SetCurSel(4);
	}

	// Stretch ratio
	SetDlgItemInt(IDC_EDIT6, 100, FALSE);

	// Stretch to size check box
	OnEnableStretchToSize();

	SetRedraw(TRUE);
	return TRUE;
}


void CCtrlSamples::RecalcLayout()
//-------------------------------
{
}


bool CCtrlSamples::SetCurrentSample(SAMPLEINDEX nSmp, LONG lZoom, bool bUpdNum)
//-----------------------------------------------------------------------------
{
	if (m_sndFile.GetNumSamples() < 1) m_sndFile.m_nSamples = 1;
	if ((nSmp < 1) || (nSmp > m_sndFile.GetNumSamples())) return FALSE;

	LockControls();
	if (m_nSample != nSmp)
	{
		m_nSample = nSmp;
		UpdateView(SampleHint(m_nSample).Info());
	}
	if (bUpdNum)
	{
		SetDlgItemInt(IDC_EDIT_SAMPLE, m_nSample);
		m_SpinSample.SetRange(1, m_sndFile.GetNumSamples());
	}
	if (lZoom == -1)
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
	PostViewMessage(VIEWMSG_SETCURRENTSAMPLE, (lZoom << 16) | m_nSample);
	UnlockControls();
	return true;
}


void CCtrlSamples::OnActivatePage(LPARAM lParam)
//----------------------------------------------
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
			m_parent.InstrumentChanged(-1);
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
			m_parent.InstrumentChanged(lParam);
		}
	}

	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	SAMPLEVIEWSTATE &sampleState = pFrame->GetSampleViewState();
	if(sampleState.initialSample != 0)
	{
		m_nSample = sampleState.initialSample;
		sampleState.initialSample = 0;
	}

	SetCurrentSample((lParam > 0) ? ((SAMPLEINDEX)lParam) : m_nSample);

	// Initial Update
	if (!m_bInitialized) UpdateView(SampleHint(m_nSample).Info().ModType(), NULL);
	if ((pFrame) && (m_hWndView)) PostViewMessage(VIEWMSG_LOADSTATE, (LPARAM)&sampleState);
	SwitchToView();

	// Combo boxes randomly disappear without this... why?
	Invalidate();
}


void CCtrlSamples::OnDeactivatePage()
//-----------------------------------
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if ((pFrame) && (m_hWndView)) SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&pFrame->GetSampleViewState());
	m_modDoc.NoteOff(0, true);
}


LRESULT CCtrlSamples::OnModCtrlMsg(WPARAM wParam, LPARAM lParam)
//---------------------------------------------------------------
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

	case CTRLMSG_SMP_OPENFILE_NEW:
		if(!InsertSample(false))
			break;
		MPT_FALLTHROUGH;
	case CTRLMSG_SMP_OPENFILE:
		if (lParam) return OpenSample(*reinterpret_cast<const mpt::PathString *>(lParam));
		break;

	case CTRLMSG_SMP_SONGDROP:
		if (lParam)
		{
			const DRAGONDROP *pDropInfo = (const DRAGONDROP *)lParam;
			CSoundFile *pSndFile = (CSoundFile *)(pDropInfo->lDropParam);
			if (pDropInfo->pModDoc) pSndFile = pDropInfo->pModDoc->GetSoundFile();
			if (pSndFile) return OpenSample(*pSndFile, (SAMPLEINDEX)pDropInfo->dwDropItem) ? TRUE : FALSE;
		}
		break;

	case CTRLMSG_SMP_SETZOOM:
		SetCurrentSample(m_nSample, static_cast<int>(lParam), FALSE);
		break;

	case CTRLMSG_SETCURRENTINSTRUMENT:
		SetCurrentSample((SAMPLEINDEX)lParam, -1, TRUE);
		break;

	//rewbs.customKeys
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

	case IDC_SAMPLE_AUTOTUNE:
		OnAutotune();
		break;

	case IDC_SAMPLE_SIGN_UNSIGN:
		OnSignUnSign();
		break;

	case IDC_SAMPLE_DCOFFSET:
		OnRemoveDCOffset();
		break;

	case IDC_SAMPLE_NORMALIZE:
		OnNormalize();
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
	//end rewbs.customKeys

	default:
		return CModControlDlg::OnModCtrlMsg(wParam, lParam);
	}
	return 0;
}


BOOL CCtrlSamples::GetToolTipText(UINT uId, TCHAR *pszText)
//---------------------------------------------------------
{
	if ((pszText) && (uId))
	{
		switch(uId)
		{
		case IDC_EDIT7:
		case IDC_EDIT8:
			// Volume to dB
			_tcscpy(pszText, CModDoc::LinearToDecibels(GetDlgItemInt(uId), 64.0));
			return TRUE;

		case IDC_EDIT9:
			// Panning
			if(m_nSample)
			{
				const ModSample &sample = m_sndFile.GetSample(m_nSample);
				_tcscpy(pszText, CModDoc::PanningToString(sample.nPan, 128));
			}
			return TRUE;

		case IDC_EDIT5:
		case IDC_SPIN5:
		case IDC_COMBO_BASENOTE:
			// Transpose + Finetune to Frequency
			if ((m_sndFile.GetType() & (MOD_TYPE_XM | MOD_TYPE_MOD)) && (m_nSample))
			{
				const ModSample &sample = m_sndFile.GetSample(m_nSample);
				uint32 freqHz = ModSample::TransposeToFrequency(sample.RelativeTone, sample.nFineTune);
				wsprintf(pszText, _T("%luHz"), freqHz);
				return TRUE;
			}
			break;
		case IDC_EDIT_STRETCHPARAMS:
			_tcscpy(pszText, _T("SequenceMs SeekwindowMs OverlapMs"));
			return TRUE;
		case IDC_CHECK2:
			_tcscpy(pszText, _T("Keep a reference to the original waveform instead of saving it in the module."));
			return TRUE;
		}
	}
	return FALSE;
}


void CCtrlSamples::UpdateView(UpdateHint hint, CObject *pObj)
//-----------------------------------------------------------
{
	if(pObj == this) return;
	if (hint.GetType()[HINT_MPTOPTIONS])
	{
		m_ToolBar1.UpdateStyle();
		m_ToolBar2.UpdateStyle();
	}

	const SampleHint sampleHint = hint.ToType<SampleHint>();
	FlagSet<HintType> hintType = sampleHint.GetType();
	if (!m_bInitialized) hintType.set(HINT_MODTYPE);
	if(!hintType[HINT_SAMPLEINFO | HINT_MODTYPE]) return;

	const SAMPLEINDEX updateSmp = sampleHint.GetSample();
	if(updateSmp != m_nSample && updateSmp != 0 && !hintType[HINT_MODTYPE]) return;

	LockControls();
	// Updating Ranges
	if (hintType[HINT_MODTYPE])
	{
		const CModSpecifications *specs = &m_sndFile.GetModSpecifications();

		// Limit text fields
		m_EditName.SetLimitText(specs->sampleNameLengthMax);
		m_EditFileName.SetLimitText(specs->sampleFilenameLengthMax);

		// Loop Type
		m_ComboLoopType.ResetContent();
		m_ComboLoopType.AddString("Off");
		m_ComboLoopType.AddString("On");

		// Sustain Loop Type
		m_ComboSustainType.ResetContent();
		m_ComboSustainType.AddString("Off");
		m_ComboSustainType.AddString("On");

		// Bidirectional Loops
		if (m_sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			m_ComboLoopType.AddString("Bidi");
			m_ComboSustainType.AddString("Bidi");
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

		// Sustain Loops only available in IT
		BOOL b;
		b = (m_sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
		m_ComboSustainType.EnableWindow(b);
		m_SpinSustainStart.EnableWindow(b);
		m_SpinSustainEnd.EnableWindow(b);
		m_EditSustainStart.EnableWindow(b);
		m_EditSustainEnd.EnableWindow(b);

		// Finetune / C-5 Speed / BaseNote
		b = (m_sndFile.GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
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
		TCHAR s[128];
		DWORD d;

		m_SpinSample.SetRange(1, m_sndFile.GetNumSamples());
		m_SpinSample.Invalidate(FALSE);	// In case the spin button was previously disabled

		// Length / Type
		wsprintf(s, _T("%u-bit %s, len: %u"), sample.GetElementarySampleSize() * 8, sample.uFlags[CHN_STEREO] ? _T("stereo") : _T("mono"), sample.nLength);
		SetDlgItemText(IDC_TEXT5, s);
		// Name
		mpt::String::Copy(s, m_sndFile.m_szNames[m_nSample]);
		SetDlgItemText(IDC_SAMPLE_NAME, s);
		// File Name
		mpt::String::Copy(s, sample.filename);
		if (m_sndFile.GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM)) s[0] = 0;
		SetDlgItemText(IDC_SAMPLE_FILENAME, s);
		// Volume
		SetDlgItemInt(IDC_EDIT7, sample.nVolume >> 2);
		// Global Volume
		SetDlgItemInt(IDC_EDIT8, sample.nGlobalVol);
		// Panning
		CheckDlgButton(IDC_CHECK1, (sample.uFlags[CHN_PANNING]) ? BST_CHECKED : BST_UNCHECKED);
		if (m_sndFile.GetType() == MOD_TYPE_XM)
		{
			SetDlgItemInt(IDC_EDIT9, sample.nPan);		//displayed panning with XM is 0-256, just like MPT's internal engine
		} else
		{
			SetDlgItemInt(IDC_EDIT9, sample.nPan / 4);	//displayed panning with anything but XM is 0-64 so we divide by 4
		}
		// FineTune / C-4 Speed / BaseNote
		int transp = 0;
		if (m_sndFile.GetType() & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT))
		{
			wsprintf(s, _T("%lu"), sample.nC5Speed);
			m_EditFineTune.SetWindowText(s);
			if(sample.nC5Speed != 0)
				transp = ModSample::FrequencyToTranspose(sample.nC5Speed) >> 7;
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

		SetDlgItemInt(IDC_EDIT14, (UINT)sample.nVibSweep);
		SetDlgItemInt(IDC_EDIT15, (UINT)sample.nVibDepth);
		SetDlgItemInt(IDC_EDIT16, (UINT)sample.nVibRate);
		// Loop
		d = 0;
		if (sample.uFlags[CHN_LOOP]) d = sample.uFlags[CHN_PINGPONGLOOP] ? 2 : 1;
		if (sample.uFlags[CHN_REVERSE]) d |= 4;
		m_ComboLoopType.SetCurSel(d);
		wsprintf(s, _T("%lu"), sample.nLoopStart);
		m_EditLoopStart.SetWindowText(s);
		wsprintf(s, _T("%lu"), sample.nLoopEnd);
		m_EditLoopEnd.SetWindowText(s);
		// Sustain Loop
		d = 0;
		if (sample.uFlags[CHN_SUSTAINLOOP]) d = sample.uFlags[CHN_PINGPONGSUSTAIN] ? 2 : 1;
		m_ComboSustainType.SetCurSel(d);
		wsprintf(s, _T("%lu"), sample.nSustainStart);
		m_EditSustainStart.SetWindowText(s);
		wsprintf(s, _T("%lu"), sample.nSustainEnd);
		m_EditSustainEnd.SetWindowText(s);
	}
	if (hintType[HINT_MODTYPE | HINT_SAMPLEINFO | HINT_SMPNAMES])
	{
		CheckDlgButton(IDC_CHECK2, m_sndFile.GetSample(m_nSample).uFlags[SMP_KEEPONDISK] ? BST_CHECKED : BST_UNCHECKED);
		GetDlgItem(IDC_CHECK2)->EnableWindow((m_sndFile.SampleHasPath(m_nSample) && m_sndFile.GetType() == MOD_TYPE_MPT) ? TRUE : FALSE);
	}
	if (!m_bInitialized)
	{
		// First update
		m_bInitialized = TRUE;
		UnlockControls();
	}

	m_ComboLoopType.Invalidate(FALSE);
	m_ComboSustainType.Invalidate(FALSE);
	m_ComboAutoVib.Invalidate(FALSE);

	UnlockControls();
}


// updateAll: Update all views including this one. Otherwise, only update update other views.
void CCtrlSamples::SetModified(SampleHint hint, bool updateAll, bool waveformModified)
//------------------------------------------------------------------------------------
{
	m_modDoc.SetModified();

	if(waveformModified)
	{
		// Update on-disk sample status in tree
		ModSample &sample = m_sndFile.GetSample(m_nSample);
		if(sample.uFlags[SMP_KEEPONDISK] && !sample.uFlags[SMP_MODIFIED]) hint.Names();
		sample.uFlags.set(SMP_MODIFIED);
	}
	m_modDoc.UpdateAllViews(nullptr, hint.SetData(m_nSample), updateAll ? nullptr : this);
}



bool CCtrlSamples::OpenSample(const mpt::PathString &fileName, FlagSet<OpenSampleTypes> types)
//--------------------------------------------------------------------------------------------
{
	BeginWaitCursor();
	InputFile f(fileName);
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

	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_replace, "Replace");
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
		CRawSampleDlg dlg(this);
		if(rememberRawFormat)
		{
			dlg.SetSampleFormat(m_nPreviousRawFormat);
			dlg.SetRememberFormat(true);
		}
		EndWaitCursor();
		if(rememberRawFormat || dlg.DoModal() == IDOK)
		{
			m_nPreviousRawFormat = dlg.GetSampleFormat();
			rememberRawFormat = dlg.GetRemeberFormat();

			BeginWaitCursor();

			m_sndFile.DestroySampleThreadsafe(m_nSample);
			sample.nLength = file.GetLength();

			SampleIO sampleIO = dlg.GetSampleFormat();

			if(TrackerSettings::Instance().m_MayNormalizeSamplesOnLoad)
			{
				sampleIO.MayNormalize();
			}

			if(sampleIO.GetBitDepth() != 8)
			{
				sample.nLength /= 2;
			}

			// Interleaved Stereo Sample
			if(sampleIO.GetChannelFormat() != SampleIO::mono)
			{
				sample.nLength /= 2;
			}

			file.Rewind();
			if(sampleIO.ReadSample(sample, file))
			{
				bOk = true;

				sample.nGlobalVol = 64;
				sample.nVolume = 256;
				sample.nPan = 128;
				sample.uFlags.reset(CHN_LOOP | CHN_SUSTAINLOOP | SMP_MODIFIED);
				sample.filename[0] = '\0';
				m_sndFile.m_szNames[m_nSample][0] = '\0';
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
		if (!sample.filename[0])
		{
			mpt::PathString name, ext;
			fileName.SplitPath(nullptr, nullptr, &name, &ext);

			if(!m_sndFile.m_szNames[m_nSample][0]) mpt::String::Copy(m_sndFile.m_szNames[m_nSample], name.ToLocale());

			if(name.AsNative().length() < 9) name += ext;
			mpt::String::Copy(sample.filename, name.ToLocale());
		}
		if ((m_sndFile.GetType() & MOD_TYPE_XM) && !sample.uFlags[CHN_PANNING])
		{
			sample.nPan = 128;
			sample.uFlags.set(CHN_PANNING);
		}
		SetModified(SampleHint().Info().Data().Names(), true, false);
		sample.uFlags.reset(SMP_KEEPONDISK);
	}
	return true;
}


bool CCtrlSamples::OpenSample(const CSoundFile &sndFile, SAMPLEINDEX nSample)
//---------------------------------------------------------------------------
{
	if(!nSample || nSample > sndFile.GetNumSamples()) return false;

	BeginWaitCursor();

	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_replace, "Replace");
	if(m_sndFile.ReadSampleFromSong(m_nSample, sndFile, nSample))
	{
		SetModified(SampleHint().Info().Data().Names(), true, false);
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
//----------------------------------
{
	if(!IsLocked())
	{
		SAMPLEINDEX n = (SAMPLEINDEX)GetDlgItemInt(IDC_EDIT_SAMPLE);
		if ((n > 0) && (n <= m_sndFile.GetNumSamples()) && (n != m_nSample))
		{
			SetCurrentSample(n, -1, FALSE);
			if (m_sndFile.GetNumInstruments())
			{
				INSTRUMENTINDEX k = static_cast<INSTRUMENTINDEX>(m_parent.GetInstrumentChange());
				if(!m_modDoc.IsChildSample(k, m_nSample))
				{
					INSTRUMENTINDEX nins = m_modDoc.FindSampleParent(m_nSample);
					if(nins != INSTRUMENTINDEX_INVALID)
					{
						m_parent.InstrumentChanged(nins);
					}
				}
			} else
			{
				m_parent.InstrumentChanged(m_nSample);
			}
		}
	}
}


void CCtrlSamples::OnZoomChanged()
//--------------------------------
{
	if (!IsLocked()) SetCurrentSample(m_nSample);
	SwitchToView();
}


void CCtrlSamples::OnTbnDropDownToolBar(NMHDR* pNMHDR, LRESULT* pResult)
//----------------------------------------------------------------------
{
	LPNMTOOLBAR pToolBar = reinterpret_cast<LPNMTOOLBAR>(pNMHDR);
	ClientToScreen(&(pToolBar->rcButton)); // TrackPopupMenu uses screen coords
	const int offset = Util::ScalePixels(4, m_hWnd);	// Compared to the main toolbar, the offset seems to be a bit wrong here...?
	switch(pToolBar->iItem)
	{
	case IDC_SAMPLE_NEW:
		{
			CMenu menu;
			menu.CreatePopupMenu();
			menu.AppendMenu(MF_STRING, IDC_SAMPLE_DUPLICATE, _T("&Duplicate Sample"));
			menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pToolBar->rcButton.left + offset, pToolBar->rcButton.bottom + offset, this);
			menu.DestroyMenu();
		}
		break;
	case IDC_SAMPLE_OPEN:
		{
			CMenu menu;
			menu.CreatePopupMenu();
			menu.AppendMenu(MF_STRING, IDC_SAMPLE_OPENKNOWN, _T("Import &Sample..."));
			menu.AppendMenu(MF_STRING, IDC_SAMPLE_OPENRAW, _T("Import &RAW Sample..."));
			menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pToolBar->rcButton.left + offset, pToolBar->rcButton.bottom + offset, this);
			menu.DestroyMenu();
		}
		break;
	}
	*pResult = 0;
}


void CCtrlSamples::OnSampleNew()
//------------------------------
{
	InsertSample(CMainFrame::GetInputHandler()->ShiftPressed());
	SwitchToView();
}


bool CCtrlSamples::InsertSample(bool duplicate, int8 *confirm)
//------------------------------------------------------------
{
	SAMPLEINDEX smp = m_modDoc.InsertSample(true);
	if(smp != SAMPLEINDEX_INVALID)
	{
		SAMPLEINDEX nOldSmp = m_nSample;
		CSoundFile &sndFile = m_modDoc.GetrSoundFile();
		SetCurrentSample(smp);

		if(duplicate && nOldSmp >= 1 && nOldSmp <= sndFile.GetNumSamples())
		{
			m_modDoc.GetSampleUndo().PrepareUndo(smp, sundo_replace, "Duplicate");
			sndFile.ReadSampleFromSong(smp, sndFile, nOldSmp);
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
//-------------------------------
{
	static int nLastIndex = 0;
	std::vector<FileType> mediaFoundationTypes = CSoundFile::GetMediaFoundationFileTypes();
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.EnableAudioPreview()
		.ExtensionFilter("All Samples|*.wav;*.flac;*.oga;*.pat;*.s3i;*.smp;*.raw;*.xi;*.aif;*.aiff;*.au;*.snd;*.its;*.iff;*.8sv;*.8svx;*.svx;*.pcm;*.opus;*.ogg;*.oga;*.mp1;*.mp2;*.mp3" + ToFilterOnlyString(mediaFoundationTypes, true).ToLocale() + "|"
			"Wave Files (*.wav)|*.wav|"
	#ifdef MPT_WITH_FLAC
			"FLAC Files (*.flac,*.oga)|*.flac;*.oga|"
	#endif // MPT_WITH_FLAC
	#if defined(MPT_WITH_OPUSFILE)
			"Opus Files (*.opus,*.oga)|*.opus;*.oga|"
	#endif // MPT_WITH_OPUSFILE
	#if defined(MPT_WITH_VORBISFILE) || defined(MPT_WITH_STBVORBIS)
			"Ogg Vorbis Files (*.ogg,*.oga)|*.ogg;*.oga|"
	#endif // VORBIS
	#if defined(MPT_ENABLE_MP3_SAMPLES)
			"MPEG Files (*.mp1,*.mp2,*.mp3)|*.mp1;*.mp2;*.mp3|"
	#endif // MPT_ENABLE_MP3_SAMPLES
			"XI Samples (*.xi)|*.xi|"
			"Impulse Tracker Samples (*.its)|*.its|"
			"ScreamTracker Samples (*.s3i,*.smp)|*.s3i;*.smp|"
			"GF1 Patches (*.pat)|*.pat|"
			"AIFF Files (*.aiff;*.8svx)|*.aif;*.aiff;*.iff;*.8sv;*.8svx;*.svx|"
			"Sun Audio (*.au;*.snd)|*.au;*.snd|"
	#if defined(MPT_WITH_MEDIAFOUNDATION)
			+ ToFilterString(mediaFoundationTypes, FileTypeFormatShowExtensions).ToLocale() +
	#endif
			"Raw Samples (*.raw,*.snd,*.pcm)|*.raw;*.snd;*.pcm|"
			"All Files (*.*)|*.*||")
		.WorkingDirectory(TrackerSettings::Instance().PathSamples.GetWorkingDir())
		.FilterIndex(&nLastIndex);
	if(!dlg.Show(this)) return;

	TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetWorkingDirectory());

	const FileDialog::PathList &files = dlg.GetFilenames();
	int8 confirm = -1;
	for(size_t counter = 0; counter < files.size(); counter++)
	{
		// If loading multiple samples, create new slots for them
		if(counter > 0)
		{
			InsertSample(CMainFrame::GetInputHandler()->ShiftPressed(), &confirm);
		}

		if(!OpenSample(files[counter], OpenSampleKnown | OpenSampleRaw))
			ErrorBox(IDS_ERR_FILEOPEN, this);
	}
	SwitchToView();
}


void CCtrlSamples::OnSampleOpenKnown()
//------------------------------------
{
	static int nLastIndex = 0;
	std::vector<FileType> mediaFoundationTypes = CSoundFile::GetMediaFoundationFileTypes();
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.EnableAudioPreview()
		.ExtensionFilter("All Samples|*.wav;*.flac;*.oga;*.pat;*.s3i;*.smp;*.xi;*.aif;*.aiff;*.its;*.iff;*.8sv;*.8svx;*.svx;*.opus;*.ogg;*.oga;*.mp1;*.mp2;*.mp3" + ToFilterOnlyString(mediaFoundationTypes, true).ToLocale() + "|"
			"Wave Files (*.wav)|*.wav|"
	#ifdef MPT_WITH_FLAC
			"FLAC Files (*.flac,*.oga)|*.flac;*.oga|"
	#endif // MPT_WITH_FLAC
	#if defined(MPT_WITH_OPUSFILE)
			"Opus Files (*.opus,*.oga)|*.opus;*.oga|"
	#endif // MPT_WITH_OPUSFILE
	#if defined(MPT_WITH_VORBISFILE) || defined(MPT_WITH_STBVORBIS)
			"Ogg Vorbis Files (*.ogg,*.oga)|*.ogg;*.oga|"
	#endif // VORBIS
	#if defined(MPT_ENABLE_MP3_SAMPLES)
			"MPEG Files (*.mp1,*.mp2,*.mp3)|*.mp1;*.mp2;*.mp3|"
	#endif // MPT_ENABLE_MP3_SAMPLES
			"XI Samples (*.xi)|*.xi|"
			"Impulse Tracker Samples (*.its)|*.its|"
			"ScreamTracker Samples (*.s3i,*.smp)|*.s3i;*.smp|"
			"GF1 Patches (*.pat)|*.pat|"
			"AIFF Files (*.aiff;*.8svx)|*.aif;*.aiff;*.iff;*.8sv;*.8svx;*.svx|"
	#if defined(MPT_WITH_MEDIAFOUNDATION)
			+ ToFilterString(mediaFoundationTypes, FileTypeFormatShowExtensions).ToLocale() +
	#endif
			"All Files (*.*)|*.*||")
		.WorkingDirectory(TrackerSettings::Instance().PathSamples.GetWorkingDir())
		.FilterIndex(&nLastIndex);
	if(!dlg.Show(this)) return;

	TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetWorkingDirectory());

	const FileDialog::PathList &files = dlg.GetFilenames();
	int8 confirm = -1;
	for(size_t counter = 0; counter < files.size(); counter++)
	{
		// If loading multiple samples, create new slots for them
		if(counter > 0)
		{
			InsertSample(CMainFrame::GetInputHandler()->ShiftPressed(), &confirm);
		}

		if(!OpenSample(files[counter], OpenSampleKnown))
			ErrorBox(IDS_ERR_FILEOPEN, this);
	}
	SwitchToView();
}


void CCtrlSamples::OnSampleOpenRaw()
//----------------------------------
{
	static int nLastIndex = 0;
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.EnableAudioPreview()
		.ExtensionFilter("Raw Samples (*.raw,*.snd,*.pcm)|*.raw;*.snd;*.pcm|"
		"All Files (*.*)|*.*||")
		.WorkingDirectory(TrackerSettings::Instance().PathSamples.GetWorkingDir())
		.FilterIndex(&nLastIndex);
	if(!dlg.Show(this)) return;

	TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetWorkingDirectory());

	const FileDialog::PathList &files = dlg.GetFilenames();
	int8 confirm = -1;
	for(size_t counter = 0; counter < files.size(); counter++)
	{
		// If loading multiple samples, create new slots for them
		if(counter > 0)
		{
			InsertSample(CMainFrame::GetInputHandler()->ShiftPressed(), &confirm);
		}

		if(!OpenSample(files[counter], OpenSampleRaw))
			ErrorBox(IDS_ERR_FILEOPEN, this);
	}
	SwitchToView();
}


void CCtrlSamples::OnSampleSave()
//-------------------------------
{
	mpt::PathString fileName;
	bool doBatchSave = CMainFrame::GetInputHandler()->ShiftPressed();
	SampleEditorDefaultFormat defaultFormat = TrackerSettings::Instance().m_defaultSampleFormat;

	if(!doBatchSave)
	{
		// save this sample
		if((!m_nSample) || (m_sndFile.GetSample(m_nSample).pSample == nullptr))
		{
			SwitchToView();
			return;
		}
		if(m_sndFile.GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			fileName = mpt::PathString::FromLocale(m_sndFile.GetSample(m_nSample).filename);
		}
		if(fileName.empty()) fileName = mpt::PathString::FromLocale(m_sndFile.m_szNames[m_nSample]);
		if(fileName.empty()) fileName = MPT_PATHSTRING("untitled");

		const mpt::PathString ext = fileName.GetFileExt();
		if(!mpt::PathString::CompareNoCase(ext, MPT_PATHSTRING(".flac"))) defaultFormat = dfFLAC;
		else if(!mpt::PathString::CompareNoCase(ext, MPT_PATHSTRING(".wav"))) defaultFormat = dfWAV;
	} else
	{
		// save all samples
		fileName = m_sndFile.GetpModDoc()->GetPathNameMpt().GetFileName();
		if(fileName.empty()) fileName = MPT_PATHSTRING("untitled");

		fileName += MPT_PATHSTRING(" - %sample_number% - ");
		if(m_sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_MOD))
			fileName += MPT_PATHSTRING("%sample_name%");
		else
			fileName += MPT_PATHSTRING("%sample_filename%");
	}
	SanitizeFilename(fileName);

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
	case dfRAW:
		filter = 3;
	}

	FileDialog dlg = SaveFileDialog()
		.DefaultExtension(ToSettingValue(defaultFormat).as<std::string>())
		.DefaultFilename(fileName)
		.ExtensionFilter("Wave File (*.wav)|*.wav|"
			"FLAC File (*.flac)|*.flac|"
			"RAW Audio (*.raw)|*.raw||")
			.WorkingDirectory(TrackerSettings::Instance().PathSamples.GetWorkingDir())
			.FilterIndex(&filter);
	if(!dlg.Show(this)) return;

	BeginWaitCursor();

	const mpt::PathString ext = dlg.GetExtension();

	SAMPLEINDEX minSmp = m_nSample, maxSmp = m_nSample;
	CStringW sNumberFormat;
	if(doBatchSave)
	{
		minSmp = 1;
		maxSmp = m_sndFile.GetNumSamples();
		sNumberFormat.Format(L"%s%d%s", L"%.", ((int)log10((float)maxSmp)) + 1, L"d");
	}

	bool ok = false;
	for(SAMPLEINDEX smp = minSmp; smp <= maxSmp; smp++)
	{
		if (m_sndFile.GetSample(smp).pSample)
		{
			fileName = dlg.GetFirstFile();
			if(doBatchSave)
			{
				CStringW sSampleNumber;
				CString sSampleName;
				CString sSampleFilename;
				sSampleNumber.Format(sNumberFormat, smp);

				sSampleName = (m_sndFile.m_szNames[smp][0]) ? m_sndFile.m_szNames[smp] : "untitled";
				sSampleFilename = (m_sndFile.GetSample(smp).filename[0]) ? m_sndFile.GetSample(smp).filename : m_sndFile.m_szNames[smp];
				SanitizeFilename(sSampleName);
				SanitizeFilename(sSampleFilename);

				std::wstring fileNameW = fileName.ToWide();
				fileNameW = mpt::String::Replace(fileNameW, L"%sample_number%", sSampleNumber);
				fileNameW = mpt::String::Replace(fileNameW, L"%sample_filename%", mpt::ToWide(sSampleFilename));
				fileNameW = mpt::String::Replace(fileNameW, L"%sample_name%", mpt::ToWide(sSampleName));
				fileName = mpt::PathString::FromWide(fileNameW);
			}
			if(!mpt::PathString::CompareNoCase(ext, MPT_PATHSTRING("raw")))
				ok = m_sndFile.SaveRAWSample(smp, fileName);
			else if(!mpt::PathString::CompareNoCase(ext, MPT_PATHSTRING("flac")))
				ok = m_sndFile.SaveFLACSample(smp, fileName);
			else
				ok = m_sndFile.SaveWAVSample(smp, fileName);

			if(ok)
			{
				m_sndFile.SetSamplePath(smp, fileName);
				ModSample &sample = m_sndFile.GetSample(smp);
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
//-------------------------------
{
	if (m_modDoc.IsNotePlaying(NOTE_NONE, m_nSample, 0))
	{
		m_modDoc.NoteOff(0, true);
	} else
	{
		m_modDoc.PlayNote(NOTE_MIDDLEC, 0, m_nSample, false);
	}
	SwitchToView();
}


void CCtrlSamples::OnNormalize()
//------------------------------
{
	//Default case: Normalize current sample
	SAMPLEINDEX minSample = m_nSample, maxSample = m_nSample;
	//If only one sample is selected, parts of it may be amplified
	SmpLength selStart = 0, selEnd = 0;

	//Shift -> Normalize all samples
	if(CMainFrame::GetInputHandler()->ShiftPressed())
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
	bool bModified = false;

	for(SAMPLEINDEX iSmp = minSample; iSmp <= maxSample; iSmp++)
	{
		if (m_sndFile.GetSample(iSmp).pSample)
		{
			bool bOk = false;
			ModSample &sample = m_sndFile.GetSample(iSmp);

			if(minSample != maxSample)
			{
				//if more than one sample is selected, always amplify the whole sample.
				selStart = 0;
				selEnd = sample.nLength;
			} else
			{
				//one sample: correct the boundaries, if needed
				LimitMax(selEnd, sample.nLength);
				LimitMax(selStart, selEnd);
				if(selStart == selEnd)
				{
					selStart = 0;
					selEnd = sample.nLength;
				}
			}

			m_modDoc.GetSampleUndo().PrepareUndo(iSmp, sundo_update, "Normalize", selStart, selEnd);

			if(sample.uFlags[CHN_STEREO]) { selStart *= 2; selEnd *= 2; }

			if(sample.uFlags[CHN_16BIT])
			{
				int16 *p = sample.pSample16;
				int max = 1;
				for (SmpLength i = selStart; i < selEnd; i++)
				{
					if (p[i] > max) max = p[i];
					if (-p[i] > max) max = -p[i];
				}
				if (max < 32767)
				{
					max++;
					for (SmpLength j = selStart; j < selEnd; j++)
					{
						int l = (((int)p[j]) << 15) / max;
						p[j] = (int16)l;
					}
					bModified = bOk = true;
				}
			} else
			{
				int8 *p = sample.pSample8;
				int max = 1;
				for (SmpLength i = selStart; i < selEnd; i++)
				{
					if (p[i] > max) max = p[i];
					if (-p[i] > max) max = -p[i];
				}
				if (max < 127)
				{
					max++;
					for (SmpLength j = selStart; j < selEnd; j++)
					{
						int l = (((int)p[j]) << 7) / max;
						p[j] = (int8)l;
					}
					bModified = bOk = true;
				}
			}

			if (bOk)
			{
				sample.PrecomputeLoops(m_sndFile, false);
				m_modDoc.UpdateAllViews(NULL, SampleHint(iSmp).Data());
			}
		}
	}

	if(bModified)
	{
		SetModified(SampleHint().Data(), false, true);
	}
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnRemoveDCOffset()
//-----------------------------------
{
	SAMPLEINDEX minSample = m_nSample, maxSample = m_nSample;

	//Shift -> Process all samples
	if(CMainFrame::GetInputHandler()->ShiftPressed())
	{
		if(Reporting::Confirm(_T("This will process all samples independently. Continue?"), _T("DC Offset Removal")) == cnfNo)
			return;
		minSample = 1;
		maxSample = m_sndFile.GetNumSamples();
	}

	BeginWaitCursor();

	// for report / SetModified
	SAMPLEINDEX numModified = 0;
	float fReportOffset = 0;

	for(SAMPLEINDEX iSmp = minSample; iSmp <= maxSample; iSmp++)
	{
		SmpLength selStart, selEnd;

		if(m_sndFile.GetSample(iSmp).pSample == nullptr)
			continue;

		if (minSample != maxSample)
		{
			selStart = 0;
			selEnd = m_sndFile.GetSample(iSmp).nLength;
		}
		else
		{
			SampleSelectionPoints selection = GetSelectionPoints();
			selStart = selection.nStart;
			selEnd = selection.nEnd;
		}

		m_modDoc.GetSampleUndo().PrepareUndo(iSmp, sundo_update, "Remove DC Offset", selStart, selEnd);

		const float fOffset = ctrlSmp::RemoveDCOffset(m_sndFile.GetSample(iSmp), selStart, selEnd, m_sndFile.GetType(), m_sndFile);

		if(fOffset == 0.0f) // No offset removed.
			continue;

		fReportOffset += fOffset;
		numModified++;
		m_modDoc.UpdateAllViews(nullptr, SampleHint(iSmp).Info().Data());
	}

	EndWaitCursor();
	SwitchToView();

	// fill the statusbar with some nice information

	CString dcInfo;
	if(numModified)
	{
		SetModified(SampleHint().Info().Data(), true, true);
		if(numModified == 1)
		{
			dcInfo.Format(_T("Removed DC offset (%.1f%%)"), fReportOffset * 100);
		}
		else
		{
			dcInfo.Format(_T("Removed DC offset from %u samples (avg %0.1f%%)"), numModified, fReportOffset / numModified * 100);
		}
	}
	else
	{
		dcInfo.SetString(_T("No DC offset found"));
	}

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	pMainFrm->SetXInfoText(dcInfo);

}


template<typename T>
static void ApplyAmplifyImpl(T * MPT_RESTRICT pSample, SmpLength start, SmpLength end, int32 amp, bool fadeIn, bool fadeOut, Fade::Law fadeLaw)
//---------------------------------------------------------------------------------------------------------------------------------------------
{
	pSample += start;
	SmpLength len = end - start, len2 = len / 2;
	Fade::Func fadeFunc = Fade::GetFadeFunc(fadeLaw);

	for(SmpLength i = 0; i < len; i++)
	{
		int32 l = (pSample[i] * amp) / 100;
		if(fadeIn && fadeOut)
		{
			if(i < len2)
				l = fadeFunc(l, i, len2);
			else
				l = fadeFunc(l, len - i, len - len2);
		} else if(fadeIn)
		{
			l = fadeFunc(l, i, len);
		} else if(fadeOut)
		{
			l = fadeFunc(l, len - i, len);
		}
		Limit(l, std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
		pSample[i] = static_cast<T>(l);
	}
}


void CCtrlSamples::ApplyAmplify(int32 lAmp, bool fadeIn, bool fadeOut, Fade::Law fadeLaw)
//---------------------------------------------------------------------------------------
{
	if((!m_sndFile.GetSample(m_nSample).pSample)) return;

	BeginWaitCursor();
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	SampleSelectionPoints selection = GetSelectionPoints();

	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_update, "Amplify", selection.nStart, selection.nEnd);

	selection.nStart *= sample.GetNumChannels();
	selection.nEnd *= sample.GetNumChannels();
	if (sample.uFlags[CHN_16BIT])
	{
		ApplyAmplifyImpl(sample.pSample16, selection.nStart, selection.nEnd, lAmp, fadeIn, fadeOut, fadeLaw);
	} else
	{
		ApplyAmplifyImpl(sample.pSample8, selection.nStart, selection.nEnd, lAmp, fadeIn, fadeOut, fadeLaw);
	}
	sample.PrecomputeLoops(m_sndFile, false);
	SetModified(SampleHint().Data(), false, true);
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnAmplify()
//----------------------------
{
	static int16 oldAmp = 100;
	static Fade::Law fadeLaw = Fade::kLinear;
	CAmpDlg dlg(this, oldAmp, fadeLaw);
	if (dlg.DoModal() != IDOK) return;
	oldAmp = dlg.m_nFactor;
	fadeLaw = dlg.m_fadeLaw;
	ApplyAmplify(dlg.m_nFactor, dlg.m_bFadeIn, dlg.m_bFadeOut, dlg.m_fadeLaw);
}


// Quickly fade the selection in/out without asking the user.
// Fade-In is applied if the selection starts at the beginning of the sample.
// Fade-Out is applied if the selection ends and the end of the sample.
void CCtrlSamples::OnQuickFade()
//------------------------------
{
	if((m_sndFile.GetSample(m_nSample).pSample == nullptr)) return;

	SampleSelectionPoints sel = GetSelectionPoints();
	if(sel.selectionActive && (sel.nStart == 0 || sel.nEnd == m_sndFile.GetSample(m_nSample).nLength))
	{
		ApplyAmplify(100, (sel.nStart == 0), (sel.nEnd == m_sndFile.GetSample(m_nSample).nLength), Fade::kLinear);
	} else
	{
		// Can't apply quick fade as no appropriate selection has been made, so ask the user to amplify the whole sample instead.
		OnAmplify();
	}
}


void CCtrlSamples::OnResample()
//-----------------------------
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if(sample.pSample == nullptr) return;

	const uint32 oldRate = sample.GetSampleRate(m_sndFile.GetType());
	CResamplingDlg dlg(this, oldRate, TrackerSettings::Instance().sampleEditorDefaultResampler);
	if(dlg.DoModal() != IDOK || oldRate == dlg.GetFrequency())
	{
		return;
	}
	TrackerSettings::Instance().sampleEditorDefaultResampler = dlg.GetFilter();
	ApplyResample(dlg.GetFrequency(), dlg.GetFilter());
}


void CCtrlSamples::ApplyResample(uint32_t newRate, ResamplingMode mode)
//---------------------------------------------------------------------
{
	BeginWaitCursor();

	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if(sample.pSample == nullptr) return;

	SampleSelectionPoints selection = GetSelectionPoints();
	LimitMax(selection.nEnd, sample.nLength);
	if(selection.nStart >= selection.nEnd)
	{
		selection.nStart = 0;
		selection.nEnd = sample.nLength;
	}

	const uint32 oldRate = sample.GetSampleRate(m_sndFile.GetType());
	const SmpLength selLength = (selection.nEnd - selection.nStart);
	const SmpLength newSelLength = Util::muldivr_unsigned(selLength, newRate, oldRate);
	const SmpLength newSelEnd = selection.nStart + newSelLength;
	const SmpLength newTotalLength = sample.nLength - selLength + newSelLength;
	const uint8 numChannels = sample.GetNumChannels();

	if(newRate < 1 || oldRate < 1)
	{
		MessageBeep(MB_ICONWARNING);
		return;
	}

	void *newSample = ModSample::AllocateSample(newTotalLength, sample.GetBytesPerSample());

	if(newSample != nullptr)
	{
		// First, copy parts of the sample that are not affected by partial upsampling
		const SmpLength bps = sample.GetBytesPerSample();
		std::memcpy(newSample, sample.pSample, selection.nStart * bps);
		std::memcpy(static_cast<char *>(newSample) + newSelEnd * bps, static_cast<char *>(sample.pSample) + selection.nEnd * bps, (sample.nLength - selection.nEnd) * bps);

		if(mode == SRCMODE_DEFAULT)
		{
			// Resample using r8brain
			const SmpLength bufferSize = std::min(std::max(selLength, SmpLength(oldRate)), SmpLength(1024 * 1024));
			std::vector<double> convBuffer(bufferSize);
			r8b::CDSPResampler16 resampler(oldRate, newRate, convBuffer.size());

			for(uint8 chn = 0; chn < numChannels; chn++)
			{
				if(chn != 0) resampler.clear();

				SmpLength readCount = selLength, writeCount = newSelLength;
				SmpLength readOffset = selection.nStart * numChannels + chn, writeOffset = readOffset;
				SmpLength outLatency = newRate;
				double *outBuffer, lastVal = 0.0;

				{
					// Pre-fill the resampler with the first sampling point.
					// Otherwise, it will assume that all samples before the first sampling point are 0,
					// which can lead to unwanted artefacts (ripples) if the sample doesn't start with a zero crossing.
					double firstVal = 0.0;
					switch(sample.GetElementarySampleSize())
					{
					case 1:
						firstVal = SC::Convert<double, int8>()(sample.pSample8[readOffset]);
						lastVal = SC::Convert<double, int8>()(sample.pSample8[readOffset + selLength - numChannels]);
						break;
					case 2:
						firstVal = SC::Convert<double, int16>()(sample.pSample16[readOffset]);
						lastVal = SC::Convert<double, int16>()(sample.pSample16[readOffset + selLength - numChannels]);
						break;
					default:
						// When higher bit depth is added, feel free to also replace CDSPResampler16 by CDSPResampler24 above.
						MPT_ASSERT_MSG(false, "Bit depth not implemented");
					}

					// 10ms or less would probably be enough, but we will pre-fill the buffer with exactly "oldRate" samples
					// to prevent any further rounding errors when using smaller buffers or when dividing oldRate or newRate.
					uint32 remain = oldRate;
					for(SmpLength i = 0; i < bufferSize; i++) convBuffer[i] = firstVal;
					while(remain > 0)
					{
						uint32 procIn = std::min<uint32>(remain, bufferSize);
						SmpLength procCount = resampler.process(convBuffer.data(), procIn, outBuffer);
						MPT_ASSERT(procCount <= outLatency);
						LimitMax(procCount, outLatency);
						outLatency -= procCount;
						remain -= procIn;
					}
				}

				// Now we can start with the actual resampling work...
				while(writeCount > 0)
				{
					SmpLength smpCount = (SmpLength)convBuffer.size();
					if(readCount != 0)
					{
						LimitMax(smpCount, readCount);

						switch(sample.GetElementarySampleSize())
						{
						case 1:
							CopySample<SC::ConversionChain<SC::Convert<double, int8>, SC::DecodeIdentity<int8> > >(convBuffer.data(), smpCount, 1, sample.pSample8 + readOffset, sample.GetSampleSizeInBytes(), sample.GetNumChannels());
							break;
						case 2:
							CopySample<SC::ConversionChain<SC::Convert<double, int16>, SC::DecodeIdentity<int16> > >(convBuffer.data(), smpCount, 1, sample.pSample16 + readOffset, sample.GetSampleSizeInBytes(), sample.GetNumChannels());
							break;
						}
						readOffset += smpCount * numChannels;
						readCount -= smpCount;
					} else
					{
						// Nothing to read, but still to write (compensate for r8brain's output latency)
						for(SmpLength i = 0; i < smpCount; i++) convBuffer[i] = lastVal;
					}

					SmpLength procCount = resampler.process(convBuffer.data(), smpCount, outBuffer);
					const SmpLength procLatency = std::min(outLatency, procCount);
					procCount = std::min(procCount- procLatency, writeCount);

					switch(sample.GetElementarySampleSize())
					{
					case 1:
						CopySample<SC::ConversionChain<SC::Convert<int8, double>, SC::DecodeIdentity<double> > >(static_cast<int8 *>(newSample) + writeOffset, procCount, sample.GetNumChannels(), outBuffer + procLatency, procCount * sizeof(double), 1);
						break;
					case 2:
						CopySample<SC::ConversionChain<SC::Convert<int16, double>, SC::DecodeIdentity<double> > >(static_cast<int16 *>(newSample) + writeOffset, procCount, sample.GetNumChannels(), outBuffer + procLatency, procCount * sizeof(double), 1);
						break;
					}
					writeOffset += procCount * numChannels;
					writeCount -= procCount;
					outLatency -= procLatency;
				}
			}
		} else
		{
			// Resample using built-in filters
			uint32 functionNdx = MixFuncTable::ResamplingModeToMixFlags(mode);
			if(sample.uFlags[CHN_16BIT]) functionNdx |= MixFuncTable::ndx16Bit;
			if(sample.uFlags[CHN_STEREO]) functionNdx |= MixFuncTable::ndxStereo;
			ModChannel chn;
			chn.pCurrentSample = sample.pSample;
			chn.increment = SamplePosition::Ratio(oldRate, newRate);
			chn.position.Set(selection.nStart);
			chn.leftVol = chn.rightVol = (1 << 8);
			chn.nLength = sample.nLength;

			SmpLength writeCount = newSelLength;
			SmpLength writeOffset = selection.nStart * sample.GetNumChannels();
			while(writeCount > 0)
			{
				SmpLength procCount = std::min<SmpLength>(MIXBUFFERSIZE, writeCount);
				mixsample_t buffer[MIXBUFFERSIZE * 2];
				MemsetZero(buffer);
				MixFuncTable::Functions[functionNdx](chn, m_sndFile.m_Resampler, buffer, procCount);

				for(uint8 chn = 0; chn < numChannels; chn++)
				{
					switch(sample.GetElementarySampleSize())
					{
					case 1:
						CopySample<SC::ConversionChain<SC::ConvertFixedPoint<int8, mixsample_t, 23, false>, SC::DecodeIdentity<mixsample_t> > >(static_cast<int8 *>(newSample) + writeOffset + chn, procCount, sample.GetNumChannels(), buffer + chn, sizeof(buffer), 2);
						break;
					case 2:
						CopySample<SC::ConversionChain<SC::ConvertFixedPoint<int16, mixsample_t, 23, false>, SC::DecodeIdentity<mixsample_t> > >(static_cast<int16 *>(newSample) + writeOffset + chn, procCount, sample.GetNumChannels(), buffer + chn, sizeof(buffer), 2);
						break;
					}
				}

				writeCount -= procCount;
				writeOffset += procCount * sample.GetNumChannels();
			}
		}

		m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_replace, (newRate > oldRate) ? "Upsample" : "Downsample");

		// Adjust loops
		if(sample.nLoopStart >= selection.nEnd) sample.nLoopStart += newSelLength - selLength;
		else if(sample.nLoopStart > selection.nStart) sample.nLoopStart = selection.nStart + Util::muldivr_unsigned(sample.nLoopStart - selection.nStart, newRate, oldRate);
		if(sample.nLoopEnd >= selection.nEnd) sample.nLoopEnd += newSelLength - selLength;
		else if(sample.nLoopEnd > selection.nStart) sample.nLoopEnd = selection.nStart + Util::muldivr_unsigned(sample.nLoopEnd - selection.nStart, newRate, oldRate);
		if(sample.nLoopEnd > newTotalLength) sample.nLoopEnd = newTotalLength;

		if(sample.nSustainStart >= selection.nEnd) sample.nSustainStart += newSelLength - selLength;
		else if(sample.nSustainStart > selection.nStart) sample.nSustainStart = selection.nStart + Util::muldivr_unsigned(sample.nSustainStart - selection.nStart, newRate, oldRate);
		if(sample.nSustainEnd >= selection.nEnd) sample.nSustainEnd += newSelLength - selLength;
		else if(sample.nSustainEnd > selection.nStart) sample.nSustainEnd = selection.nStart + Util::muldivr_unsigned(sample.nSustainEnd - selection.nStart, newRate, oldRate);
		if(sample.nSustainEnd > newTotalLength) sample.nSustainEnd = newTotalLength;

		ctrlSmp::ReplaceSample(sample, newSample, newTotalLength, m_sndFile);
		// Update loop wrap-around buffer
		sample.PrecomputeLoops(m_sndFile);

		if(!selection.selectionActive)
		{
			if(m_sndFile.GetType() != MOD_TYPE_MOD)
			{
				sample.nC5Speed = newRate;
				sample.FrequencyToTranspose();
			}
		} else
		{
			SetSelectionPoints(selection.nStart, newSelEnd);
		}
		SetModified(SampleHint().Info().Data(), true, true);
	}

	EndWaitCursor();
	SwitchToView();
}


#define MAX_BUFFER_LENGTH	8192

void CCtrlSamples::ReadTimeStretchParameters()
//--------------------------------------------
{
	CString str;
	GetDlgItemText(IDC_EDIT_STRETCHPARAMS, str);
	_stscanf(str, _T("%u %u %u"),
		&m_nSequenceMs, &m_nSeekWindowMs, &m_nOverlapMs);
}


void CCtrlSamples::UpdateTimeStretchParameterString()
//---------------------------------------------------
{
	CString str;
	str.Format(_T("%u %u %u"),
				m_nSequenceMs,
				m_nSeekWindowMs,
				m_nOverlapMs);
	SetDlgItemText(IDC_EDIT_STRETCHPARAMS, str);
}

void CCtrlSamples::OnEnableStretchToSize()
//----------------------------------------
{
	// Enable time-stretching / disable unused pitch-shifting UI elements
	bool timeStretch = IsDlgButtonChecked(IDC_CHECK3) != BST_UNCHECKED;
	if(!timeStretch) ReadTimeStretchParameters();
	((CComboBox *)GetDlgItem(IDC_COMBO4))->EnableWindow(timeStretch ? FALSE : TRUE);
	((CEdit *)GetDlgItem(IDC_EDIT6))->EnableWindow(timeStretch ? TRUE : FALSE);
	((CButton *)GetDlgItem(IDC_BUTTON2))->EnableWindow(timeStretch ? TRUE : FALSE); //rewbs.timeStretchMods
	GetDlgItem(IDC_TEXT_QUALITY)->ShowWindow(timeStretch ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_COMBO5)->ShowWindow(timeStretch ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_TEXT_FFT)->ShowWindow(timeStretch ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_COMBO6)->ShowWindow(timeStretch ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_TEXT_STRETCHPARAMS)->ShowWindow(timeStretch ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_EDIT_STRETCHPARAMS)->ShowWindow(timeStretch ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_TEXT_PITCH)->ShowWindow(timeStretch ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_COMBO4)->ShowWindow(timeStretch ? SW_HIDE : SW_SHOW);
	SetDlgItemText(IDC_BUTTON1, timeStretch ? "Time Stretch" : "Pitch Shift");
	if(timeStretch) UpdateTimeStretchParameterString();
}

void CCtrlSamples::OnEstimateSampleSize()
//---------------------------------------
{
	if((m_sndFile.GetSample(m_nSample).pSample == nullptr)) return;

	//rewbs.timeStretchMods
	//Ensure m_dTimeStretchRatio is up-to-date with textbox content
	UpdateData(TRUE);

	//Open dialog
	CPSRatioCalc dlg(m_sndFile, m_nSample, m_dTimeStretchRatio, this);
	if (dlg.DoModal() != IDOK) return;

	//Update ratio value&textbox
	m_dTimeStretchRatio = dlg.m_dRatio;
	UpdateData(FALSE);
	//end rewbs.timeStretchMods
}


#ifdef MPT_BUILD_MSVC_SHARED

//=======================
class ComponentSoundTouch
//=======================
	: public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentSoundTouch()
		: ComponentBuiltin()	
	{
		return;
	}
};
MPT_REGISTERED_COMPONENT(ComponentSoundTouch, "SoundTouch")

#else

//=======================
class ComponentSoundTouch
//=======================
	: public ComponentBundledDLL
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentSoundTouch()
		: ComponentBundledDLL(MPT_PATHSTRING("OpenMPT_SoundTouch_f32"))	
	{
		return;
	}
};
MPT_REGISTERED_COMPONENT(ComponentSoundTouch, "SoundTouch")

#endif


enum TimeStretchPitchShiftResult
{
	kUnknown,
	kOK,
	kAbort,
	kInvalidRatio,
	kStretchTooShort,
	kStretchTooLong,
	kOutOfMemory,
	kSampleTooShort,
};

//=====================================================
class CDoPitchShiftTimeStretch : public CProgressDialog
//=====================================================
{
public:
	CCtrlSamples &m_parent;
	CModDoc &m_modDoc;
	const float m_ratio;
	TimeStretchPitchShiftResult m_result;
	uint32 m_updateInterval;
	const SAMPLEINDEX m_sample;
	const bool m_pitchShift;

	CDoPitchShiftTimeStretch(CCtrlSamples &parent, CModDoc &modDoc, SAMPLEINDEX sample, float ratio, bool pitchShift)
		: CProgressDialog(&parent)
		, m_parent(parent)
		, m_modDoc(modDoc)
		, m_ratio(ratio)
		, m_result(kUnknown)
		, m_sample(sample)
		, m_pitchShift(pitchShift)
	{
		m_updateInterval = TrackerSettings::Instance().GUIUpdateInterval;
		if(m_updateInterval < 15) m_updateInterval = 15;
	}

	void Run()
	//--------
	{
		SetTitle(m_pitchShift ? _T("Pitch Shift") : _T("Time Stretch"));
		SetRange(0, 100);
		if(m_pitchShift)
			m_result = PitchShift();
		else
			m_result = TimeStretch();
		EndDialog((m_result == kOK) ? IDOK : IDCANCEL);
	}

	TimeStretchPitchShiftResult TimeStretch()
	//---------------------------------------
	{
		ModSample &sample = m_modDoc.GetrSoundFile().GetSample(m_sample);
		const uint32 nSampleRate = sample.GetSampleRate(m_modDoc.GetModType());

		if(!sample.HasSampleData()) return kAbort;

		if(m_ratio == 1.0) return kAbort;
		if(m_ratio < 0.5f) return kStretchTooShort;
		if(m_ratio > 2.0f) return kStretchTooLong;

		// Check whether the DLL file exists.
		ComponentHandle<ComponentSoundTouch> soundTouch;
		if(!IsComponentAvailable(soundTouch))
		{
			MsgBox(IDS_SOUNDTOUCH_LOADFAILURE);
			return kAbort;
		}

		HANDLE handleSt = soundtouch_createInstance();
		if(handleSt == NULL)
		{
			MsgBox(IDS_SOUNDTOUCH_LOADFAILURE);
			return kAbort;
		}

		// Get number of channels & sample size
		uint8 smpsize = sample.GetElementarySampleSize();
		const uint8 nChn = sample.GetNumChannels();

		// Initialize soundtouch object.
		soundtouch_setSampleRate(handleSt, nSampleRate);
		soundtouch_setChannels(handleSt, nChn);

		// Given ratio is time stretch ratio, and must be converted to
		// tempo change ratio: for example time stretch ratio 2 means
		// tempo change ratio 0.5.
		soundtouch_setTempoChange(handleSt, (1.0f / m_ratio - 1.0f) * 100.0f);

		// Read settings from GUI.
		m_parent.ReadTimeStretchParameters();

		// Set settings to soundtouch. Zero value means 'use default', and
		// setting value is read back after setting because not all settings are accepted.
		if(m_parent.m_nSequenceMs != 0)
			soundtouch_setSetting(handleSt, SETTING_SEQUENCE_MS, m_parent.m_nSequenceMs);
		m_parent.m_nSequenceMs = soundtouch_getSetting(handleSt, SETTING_SEQUENCE_MS);

		if(m_parent.m_nSeekWindowMs != 0)
			soundtouch_setSetting(handleSt, SETTING_SEEKWINDOW_MS, m_parent.m_nSeekWindowMs);
		m_parent.m_nSeekWindowMs = soundtouch_getSetting(handleSt, SETTING_SEEKWINDOW_MS);

		if(m_parent.m_nOverlapMs != 0)
			soundtouch_setSetting(handleSt, SETTING_OVERLAP_MS, m_parent.m_nOverlapMs);
		m_parent.m_nOverlapMs = soundtouch_getSetting(handleSt, SETTING_OVERLAP_MS);

		// Update GUI with the actual SoundTouch parameters in effect.
		m_parent.UpdateTimeStretchParameterString();

		const SmpLength inBatchSize = soundtouch_getSetting(handleSt, SETTING_NOMINAL_INPUT_SEQUENCE) + 1; // approximate value, add 1 to play safe
		const SmpLength outBatchSize = soundtouch_getSetting(handleSt, SETTING_NOMINAL_OUTPUT_SEQUENCE) + 1; // approximate value, add 1 to play safe

		if(sample.nLength < inBatchSize)
		{
			soundtouch_destroyInstance(handleSt);
			return kSampleTooShort;
		}

		if((SmpLength)std::ceil((double)m_ratio * (double)sample.nLength) < outBatchSize)
		{
			soundtouch_destroyInstance(handleSt);
			return kSampleTooShort;
		}

		// Allocate new sample. Returned sample may not be exactly the size what ratio would suggest
		// so allocate a bit more (1.01).
		const SmpLength nNewSampleLength = static_cast<SmpLength>(1.01 * m_ratio * static_cast<double>(sample.nLength + inBatchSize)) + outBatchSize;
		void *pNewSample = nullptr;
		if(nNewSampleLength <= MAX_SAMPLE_LENGTH)
		{
			pNewSample = ModSample::AllocateSample(nNewSampleLength, sample.GetBytesPerSample());
		}
		if(pNewSample == nullptr)
		{
			soundtouch_destroyInstance(handleSt);
			return kOutOfMemory;
		}

		// Show wait mouse cursor
		BeginWaitCursor();

		static const SmpLength MaxInputChunkSize = 1024;

		std::vector<float> buffer(MaxInputChunkSize * nChn);
		std::vector<SC::Convert<float,int16> > convf32(nChn);
		std::vector<SC::Convert<int16,float> > convi16(nChn);
		std::vector<SC::Convert<float,int8> > conv8f32(nChn);
		std::vector<SC::Convert<int8,float> > convint8(nChn);

		SmpLength inPos = 0;
		SmpLength outPos = 0; // Keeps count of the sample length received from stretching process.

		DWORD timeLast = 0;

		// Process sample in steps.
		while(inPos < sample.nLength)
		{
			// Current chunk size limit test
			const SmpLength inChunkSize = std::min(MaxInputChunkSize, sample.nLength - inPos);

			DWORD timeNow = timeGetTime();
			if(timeNow - timeLast >= m_updateInterval)
			{
				// Show progress bar using process button painting & text label
				TCHAR progress[32];
				uint32 percent = static_cast<uint32>(100 * (inPos + inChunkSize) / sample.nLength);
				wsprintf(progress, _T("Time Stretch... %u%%"), percent);
				SetText(progress);
				SetProgress(percent);
				ProcessMessages();
				if(m_abort)
				{
					break;
				}

				timeLast = timeNow;
			}

			// Send sampledata for processing.
			switch(smpsize)
			{
			case 1:
				CopyInterleavedSampleStreams(&(buffer[0]), sample.pSample8 + inPos * nChn, inChunkSize, nChn, conv8f32);
				break;
			case 2:
				CopyInterleavedSampleStreams(&(buffer[0]), sample.pSample16 + inPos * nChn, inChunkSize, nChn, convf32);
				break;
			}
			soundtouch_putSamples(handleSt, &(buffer[0]), inChunkSize);

			// Receive some processed samples (it's not guaranteed that there is any available).
			{
				SmpLength outChunkSize = std::min<size_t>(soundtouch_numSamples(handleSt), nNewSampleLength - outPos);
				if(outChunkSize > 0)
				{
					buffer.resize(outChunkSize * nChn);
					soundtouch_receiveSamples(handleSt, &(buffer[0]), outChunkSize);
					switch(smpsize)
					{
					case 1:
						CopyInterleavedSampleStreams(static_cast<int8 *>(pNewSample) + nChn * outPos, &(buffer[0]), outChunkSize, nChn, convint8);
						break;
					case 2:
						CopyInterleavedSampleStreams(static_cast<int16 *>(pNewSample) + nChn * outPos, &(buffer[0]), outChunkSize, nChn, convi16);
						break;
					}
					outPos += outChunkSize;
				}
			}

			// Next buffer chunk
			inPos += inChunkSize;
		}

		if(!m_abort)
		{
			// The input sample should now be processed. Receive remaining samples.
			soundtouch_flush(handleSt);
			SmpLength outChunkSize = std::min<size_t>(soundtouch_numSamples(handleSt), nNewSampleLength - outPos);
			if(outChunkSize > 0)
			{
				buffer.resize(outChunkSize * nChn);
				soundtouch_receiveSamples(handleSt, &(buffer[0]), outChunkSize);
				switch(smpsize)
				{
				case 1:
					CopyInterleavedSampleStreams(static_cast<int8 *>(pNewSample) + nChn * outPos, &(buffer[0]), outChunkSize, nChn, convint8);
					break;
				case 2:
					CopyInterleavedSampleStreams(static_cast<int16 *>(pNewSample) + nChn * outPos, &(buffer[0]), outChunkSize, nChn, convi16);
					break;
				}
				outPos += outChunkSize;
			}

			soundtouch_clear(handleSt);
			MPT_ASSERT(soundtouch_isEmpty(handleSt) != 0);
			MPT_ASSERT(nNewSampleLength >= outPos);

			CSoundFile &sndFile = m_modDoc.GetrSoundFile();
			m_modDoc.GetSampleUndo().PrepareUndo(m_sample, sundo_replace, "Time Stretch");
			// Swap sample buffer pointer to new buffer, update song + sample data & free old sample buffer
			ctrlSmp::ReplaceSample(sample, pNewSample, std::min(outPos, nNewSampleLength), sndFile);
			// Update loops and wrap-around buffer
			sample.SetLoop(
				static_cast<SmpLength>(sample.nLoopStart * m_ratio),
				static_cast<SmpLength>(sample.nLoopEnd * m_ratio),
				sample.uFlags[CHN_LOOP],
				sample.uFlags[CHN_PINGPONGLOOP],
				sndFile);
			sample.SetSustainLoop(
				static_cast<SmpLength>(sample.nSustainStart * m_ratio),
				static_cast<SmpLength>(sample.nSustainEnd * m_ratio),
				sample.uFlags[CHN_SUSTAINLOOP],
				sample.uFlags[CHN_PINGPONGSUSTAIN],
				sndFile);
		} else
		{
			ModSample::FreeSample(pNewSample);
		}

		soundtouch_destroyInstance(handleSt);

		// Restore mouse cursor
		EndWaitCursor();

		return m_abort ? kAbort : kOK;
	}

	TimeStretchPitchShiftResult PitchShift()
	//--------------------------------------
	{
		ModSample &sample = m_modDoc.GetrSoundFile().GetSample(m_sample);

		if(!sample.HasSampleData() || m_ratio < 0.5f || m_ratio > 2.0f)
		{
			return kAbort;
		}

		// Get number of channels & sample size
		const uint8 smpsize = sample.GetElementarySampleSize();
		const uint8 nChn = sample.GetNumChannels();

		// Get selected oversampling - quality - (also refered as FFT overlapping) factor
		CComboBox *combo = (CComboBox *)m_parent.GetDlgItem(IDC_COMBO5);
		long ovs = combo->GetCurSel() + 4;

		// Get selected FFT size (power of 2 ; should not exceed MAX_BUFFER_LENGTH - see smbPitchShift.h)
		combo = (CComboBox *)m_parent.GetDlgItem(IDC_COMBO6);
		UINT fft = 1 << (combo->GetCurSel() + 8);
		while(fft > MAX_BUFFER_LENGTH) fft >>= 1;

		// Show wait mouse cursor
		BeginWaitCursor();

		// Get original sample rate
		const float sampleRate = static_cast<float>(sample.GetSampleRate(m_modDoc.GetModType()));

		// Allocate working buffer
		const size_t bufferSize = MAX_BUFFER_LENGTH + fft;
#ifdef _DEBUG
		float *buffer = new float[bufferSize];
#else
		float *buffer = new (std::nothrow) float[bufferSize];
#endif

		int8 *pNewSample = static_cast<int8 *>(ModSample::AllocateSample(sample.nLength, sample.GetBytesPerSample()));

		if(buffer == nullptr || pNewSample == nullptr)
		{
			delete[] buffer;
			ModSample::FreeSample(pNewSample);
			return kOutOfMemory;
		}

		DWORD timeLast = 0;

		// Process each channel separately
		for(uint8 i = 0 ; i < nChn ; i++)
		{

			SmpLength pos = 0;
			SmpLength len = MAX_BUFFER_LENGTH;

			// Process sample buffer using MAX_BUFFER_LENGTH (max) sized chunk steps (in order to allow
			// the processing of BIG samples...)
			while(pos < sample.nLength)
			{

				// Current chunk size limit test
				if(pos + len >= sample.nLength) len = sample.nLength - pos;

				// TRICK : output buffer offset management
				// as the pitch-shifter adds  some blank signal in head of output  buffer (matching FFT
				// length - in short it needs a certain amount of data before being able to output some
				// meaningful  processed samples) , in order  to avoid this behaviour , we will ignore
				// the  first FFT_length  samples and process  the same  amount of extra  blank samples
				// (all 0.0f) at the end of the buffer (those extra samples will benefit from  internal
				// FFT data  computed during the previous  steps resulting in a  correct and consistent
				// signal output).
				bool bufstart = ( pos == 0 );
				bool bufend   = ( pos + MAX_BUFFER_LENGTH >= sample.nLength );
				SmpLength startoffset = ( bufstart ? fft : 0 );
				SmpLength inneroffset = ( bufstart ? 0 : fft );
				SmpLength finaloffset = ( bufend   ? fft : 0 );

				DWORD timeNow = timeGetTime();
				if(timeNow - timeLast >= m_updateInterval)
				{
					TCHAR progress[32];
					uint32 percent = static_cast<uint32>((float)i * 50.0f + (100.0f / nChn) * (pos + len) / sample.nLength);
					wsprintf(progress,"Pitch Shift... %u%%", percent);
					SetText(progress);
					SetProgress(percent);
					ProcessMessages();
					if(m_abort)
					{
						break;
					}

					timeLast = timeNow;
				}

				// Re-initialize pitch-shifter with blank FFT before processing 1st chunk of current channel
				if(bufstart)
				{
					for(UINT j = 0; j < fft; j++) buffer[j] = 0.0f;
					smbPitchShift(m_ratio, fft, fft, ovs, sampleRate, buffer, buffer);
				}

				// Convert current channel's data chunk to float
				int8 *ptr = sample.pSample8 + pos * smpsize * nChn + i * smpsize;

				switch(smpsize)
				{
				case 1:
					CopySample<SC::ConversionChain<SC::Convert<float, int8>, SC::DecodeIdentity<int8> > >(buffer, len, 1, ptr, sizeof(int8) * len * nChn, nChn);
					break;
				case 2:
					CopySample<SC::ConversionChain<SC::Convert<float, int16>, SC::DecodeIdentity<int16> > >(buffer, len, 1, (int16 *)ptr, sizeof(int16) * len * nChn, nChn);
					break;
				}

				// Fills extra blank samples (read TRICK description comment above)
				if(bufend) for(SmpLength j = len ; j < len + finaloffset ; j++) buffer[j] = 0.0f;

				// Apply pitch shifting
				smbPitchShift(m_ratio, static_cast<long>(len + finaloffset), fft, ovs, sampleRate, buffer, buffer);

				// Restore pitched-shifted float sample into original sample buffer
				ptr = pNewSample + (pos - inneroffset) * smpsize * nChn + i * smpsize;
				const SmpLength copyLength = len + finaloffset - startoffset + 1;

				switch(smpsize)
				{
				case 1:
					CopySample<SC::ConversionChain<SC::Convert<int8, float>, SC::DecodeIdentity<float> > >((int8 *)ptr, copyLength, nChn, buffer + startoffset, sizeof(float) * bufferSize, 1);
					break;
				case 2:
					CopySample<SC::ConversionChain<SC::Convert<int16, float>, SC::DecodeIdentity<float> > >((int16 *)ptr, copyLength, nChn, buffer + startoffset, sizeof(float) * bufferSize, 1);
					break;
				}

				// Next buffer chunk
				pos += MAX_BUFFER_LENGTH;
			}
		}

		// Free working buffer
		delete[] buffer;

		if(!m_abort)
		{
			m_modDoc.GetSampleUndo().PrepareUndo(m_sample, sundo_replace, "Pitch Shift");
			ctrlSmp::ReplaceSample(sample, pNewSample, sample.nLength, m_modDoc.GetrSoundFile());
		}

		// Restore mouse cursor
		EndWaitCursor();

		return m_abort ? kAbort : kOK;
	}
};


void CCtrlSamples::OnPitchShiftTimeStretch()
//------------------------------------------
{
	TimeStretchPitchShiftResult errorcode = kAbort;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if(!sample.HasSampleData()) goto error;

	if(IsDlgButtonChecked(IDC_CHECK3))
	{
		// Time stretching
		UpdateData(TRUE); //Ensure m_dTimeStretchRatio is up-to-date with textbox content
		CDoPitchShiftTimeStretch timeStretch(*this, m_modDoc, m_nSample, static_cast<float>(m_dTimeStretchRatio / 100.0), false);
		timeStretch.DoModal();
		errorcode = timeStretch.m_result;
	} else
	{
		// Pitch shifting
		// Get selected pitch modifier [-12,+12]
		CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO4);
		float pm = float(combo->GetCurSel()) - 12.0f;
		if(pm == 0.0f) goto error;

		// Compute pitch ratio in range [0.5f ; 2.0f] (1.0f means output == input)
		// * pitch up -> 1.0f + n / 12.0f -> (12.0f + n) / 12.0f , considering n : pitch modifier > 0
		// * pitch dn -> 1.0f - n / 24.0f -> (24.0f - n) / 24.0f , considering n : pitch modifier > 0
		float pitch = pm < 0 ? ((24.0f + pm) / 24.0f) : ((12.0f + pm) / 12.0f);

		// Apply pitch modifier
		CDoPitchShiftTimeStretch pitchShift(*this, m_modDoc, m_nSample, pitch, true);
		pitchShift.DoModal();
		errorcode = pitchShift.m_result;
	}

	if(errorcode == kOK)
	{
		// Update sample view
		SetModified(SampleHint().Info().Data(), true, true);
		return;
	}

	// Error management
error:

	if(errorcode != kAbort)
	{
		TCHAR str[64];
		switch(errorcode)
		{
		case kInvalidRatio:
			wsprintf(str, _T("Invalid stretch ratio!"));
			break;
		case kStretchTooShort:
		case kStretchTooLong:
			wsprintf(str, _T("Stretch ratio is too %s. Must be between 50%% and 200%%."), (errorcode == kStretchTooShort) ? _T("low") : _T("high"));
			break;
		case kOutOfMemory:
			_tcscpy(str, _T("Out of memory."));
			break;
		case kSampleTooShort:
			_tcscpy(str, _T("Sample too short."));
			break;
		default:
			_tcscpy(str, _T("Unknown Error."));
			break;
		}
		Reporting::Error(str);
	}
}


void CCtrlSamples::OnReverse()
//----------------------------
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	SampleSelectionPoints selection = GetSelectionPoints();

	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_reverse, "Reverse", selection.nStart, selection.nEnd);
	if(ctrlSmp::ReverseSample(sample, selection.nStart, selection.nEnd, m_sndFile))
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
//---------------------------
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	SampleSelectionPoints selection = GetSelectionPoints();

	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_invert, "Invert", selection.nStart, selection.nEnd);
	if(ctrlSmp::InvertSample(sample, selection.nStart, selection.nEnd, m_sndFile) == true)
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
//-------------------------------
{
	if((m_sndFile.GetSample(m_nSample).pSample == nullptr)) return;

	if(m_modDoc.IsNotePlaying(0, m_nSample, 0))
		MsgBoxHidable(ConfirmSignUnsignWhenPlaying);

	BeginWaitCursor();
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SampleSelectionPoints selection = GetSelectionPoints();

	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_unsign, "Unsign", selection.nStart, selection.nEnd);
	if(ctrlSmp::UnsignSample(sample, selection.nStart, selection.nEnd, m_sndFile) == true)
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
//----------------------------
{
	if((m_sndFile.GetSample(m_nSample).pSample == nullptr)) return;
	BeginWaitCursor();
	SampleSelectionPoints selection = GetSelectionPoints();

	// never apply silence to a sample that has no selection
	const SmpLength len = selection.nEnd - selection.nStart;
	if(selection.selectionActive && len > 1)
	{
		ModSample &sample = m_sndFile.GetSample(m_nSample);
		m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_update, "Silence", selection.nStart, selection.nEnd);
		if(ctrlSmp::SilenceSample(sample, selection.nStart, selection.nEnd, m_sndFile))
		{
			SetModified(SampleHint().Data(), false, true);
		}
	}

	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnPrevInstrument()
//-----------------------------------
{
	if (m_nSample > 1)
		SetCurrentSample(m_nSample - 1);
	else
		SetCurrentSample(m_sndFile.GetNumSamples());
}


void CCtrlSamples::OnNextInstrument()
//-----------------------------------
{
	if (m_nSample < m_sndFile.GetNumSamples())
		SetCurrentSample(m_nSample + 1);
	else
		SetCurrentSample(1);
}


void CCtrlSamples::OnNameChanged()
//--------------------------------
{
	TCHAR s[MAX_SAMPLENAME] = _T("");

	if(IsLocked() || !m_nSample) return;
	m_EditName.GetWindowText(s, CountOf(s));
	if (_tcscmp(s, m_sndFile.m_szNames[m_nSample]))
	{
		mpt::String::Copy(m_sndFile.m_szNames[m_nSample], s);
		SetModified(SampleHint().Info().Names(), false, false);
	}
}


void CCtrlSamples::OnFileNameChanged()
//------------------------------------
{
	TCHAR s[MAX_SAMPLEFILENAME] = _T("");

	if(IsLocked()) return;
	m_EditFileName.GetWindowText(s, CountOf(s));

	if (_tcscmp(s, m_sndFile.GetSample(m_nSample).filename))
	{
		mpt::String::Copy(m_sndFile.GetSample(m_nSample).filename, s);
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnVolumeChanged()
//----------------------------------
{
	if (IsLocked()) return;
	int nVol = GetDlgItemInt(IDC_EDIT7);
	Limit(nVol, 0, 64);
	nVol *= 4;
	if (nVol != m_sndFile.GetSample(m_nSample).nVolume)
	{
		m_sndFile.GetSample(m_nSample).nVolume = (WORD)nVol;
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnGlobalVolChanged()
//-------------------------------------
{
	if (IsLocked()) return;
	int nVol = GetDlgItemInt(IDC_EDIT8);
	Limit(nVol, 0, 64);
	auto &sample = m_sndFile.GetSample(m_nSample);
	if (nVol != sample.nGlobalVol)
	{
		// Live-adjust volume
		sample.nGlobalVol = (uint16)nVol;
		for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
		{
			auto &chn = m_sndFile.m_PlayState.Chn[i];
			if(chn.pModSample == &sample)
			{
				chn.UpdateInstrumentVolume(chn.pModSample, chn.pModInstrument);
			}
		}
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnSetPanningChanged()
//--------------------------------------
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
		sample.uFlags.set(CHN_PANNING, b);
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnPanningChanged()
//-----------------------------------
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
		nPan = nPan << 2;			// so we x4 to get MPT's internal 0-256 range.
	}

	if (nPan != m_sndFile.GetSample(m_nSample).nPan)
	{
		m_sndFile.GetSample(m_nSample).nPan = (uint16)nPan;
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnFineTuneChanged()
//------------------------------------
{
	if (IsLocked()) return;
	int n = GetDlgItemInt(IDC_EDIT5);
	if(!finetuneBoxActive)
	{
		m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_none, "Finetune");
		finetuneBoxActive = true;
	}
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if (m_sndFile.m_nType & (MOD_TYPE_IT|MOD_TYPE_S3M|MOD_TYPE_MPT))
	{
		if ((n > 0) && (n <= (m_sndFile.GetType() == MOD_TYPE_S3M ? 65535 : 9999999)) && (n != (int)m_sndFile.GetSample(m_nSample).nC5Speed))
		{
			sample.nC5Speed = n;
			int transp = ModSample::FrequencyToTranspose(n) >> 7;
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
		if ((n >= -128) && (n <= 127))
		{
			sample.nFineTune = (signed char)n;
			SetModified(SampleHint().Info(), false, false);
		}
	}
}


void CCtrlSamples::OnFineTuneChangedDone()
//----------------------------------------
{
	finetuneBoxActive = false;
	// Update all playing channels
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
	{
		ModChannel &chn = m_sndFile.m_PlayState.Chn[i];
		if(chn.pModSample == &m_sndFile.GetSample(m_nSample))
		{
			chn.nTranspose = sample.RelativeTone;
			chn.nFineTune = sample.nFineTune;
			if(chn.nC5Speed != 0 && sample.nC5Speed != 0)
			{
				if(m_sndFile.m_SongFlags[SONG_LINEARSLIDES] && m_sndFile.m_playBehaviour[kHertzInLinearMode])
					chn.nPeriod = Util::muldivr(chn.nPeriod, sample.nC5Speed, chn.nC5Speed);
				else if(!m_sndFile.m_SongFlags[SONG_LINEARSLIDES])
					chn.nPeriod = Util::muldivr(chn.nPeriod, chn.nC5Speed, sample.nC5Speed);
			}
			chn.nC5Speed = sample.nC5Speed;
		}
	}
}


void CCtrlSamples::OnBaseNoteChanged()
//------------------------------------
{
	if (IsLocked()) return;
	int n = static_cast<int>(m_CbnBaseNote.GetItemData(m_CbnBaseNote.GetCurSel()));

	ModSample &sample = m_sndFile.GetSample(m_nSample);
	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_none, "Transpose");

	if (m_sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_S3M|MOD_TYPE_MPT))
	{
		const int oldTransp = ModSample::FrequencyToTranspose(sample.nC5Speed) >> 7;
		const uint32 newTrans = Util::Round<uint32>(sample.nC5Speed * pow(2.0, (n - oldTransp) / 12.0));
		if (newTrans > 0 && newTrans <= (m_sndFile.GetType() == MOD_TYPE_S3M ? 65535u : 9999999u) && newTrans != sample.nC5Speed)
		{
			sample.nC5Speed = newTrans;
			LockControls();
			SetDlgItemInt(IDC_EDIT5, newTrans, FALSE);
			OnFineTuneChangedDone();
			UnlockControls();
			SetModified(SampleHint().Info(), false, false);
		}
	} else
	{
		if ((n >= -128) && (n < 128))
		{
			sample.RelativeTone = (int8)n;
			OnFineTuneChangedDone();
			SetModified(SampleHint().Info(), false, false);
		}
	}
}


void CCtrlSamples::OnVibTypeChanged()
//-----------------------------------
{
	if (IsLocked()) return;
	int n = m_ComboAutoVib.GetCurSel();
	if (n >= 0)
	{
		m_sndFile.GetSample(m_nSample).nVibType = static_cast<uint8>(m_ComboAutoVib.GetItemData(n));

		PropagateAutoVibratoChanges();
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnVibDepthChanged()
//------------------------------------
{
	if (IsLocked()) return;
	int lmin = 0, lmax = 0;
	m_SpinVibDepth.GetRange(lmin, lmax);
	int n = GetDlgItemInt(IDC_EDIT15);
	if ((n >= lmin) && (n <= lmax))
	{
		m_sndFile.GetSample(m_nSample).nVibDepth = static_cast<uint8>(n);

		PropagateAutoVibratoChanges();
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnVibSweepChanged()
//------------------------------------
{
	if (IsLocked()) return;
	int lmin = 0, lmax = 0;
	m_SpinVibSweep.GetRange(lmin, lmax);
	int n = GetDlgItemInt(IDC_EDIT14);
	if ((n >= lmin) && (n <= lmax))
	{
		m_sndFile.GetSample(m_nSample).nVibSweep = static_cast<uint8>(n);

		PropagateAutoVibratoChanges();
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnVibRateChanged()
//-----------------------------------
{
	if (IsLocked()) return;
	int lmin = 0, lmax = 0;
	m_SpinVibRate.GetRange(lmin, lmax);
	int n = GetDlgItemInt(IDC_EDIT16);
	if ((n >= lmin) && (n <= lmax))
	{
		m_sndFile.GetSample(m_nSample).nVibRate = static_cast<uint8>(n);

		PropagateAutoVibratoChanges();
		SetModified(SampleHint().Info(), false, false);
	}
}


void CCtrlSamples::OnLoopTypeChanged()
//------------------------------------
{
	if(IsLocked()) return;
	const int n = m_ComboLoopType.GetCurSel();
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	bool wasDisabled = !sample.uFlags[CHN_LOOP];

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
		m_modDoc.UpdateAllViews(NULL, SampleHint(m_nSample).Info().Data());
	} else
	{
		sample.PrecomputeLoops(m_sndFile);
	}
	ctrlSmp::UpdateLoopPoints(sample, m_sndFile);
	SetModified(SampleHint().Info(), false, false);
}


void CCtrlSamples::OnLoopPointsChanged()
//--------------------------------------
{
	if(IsLocked()) return;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SmpLength start = GetDlgItemInt(IDC_EDIT1, NULL, FALSE), end = GetDlgItemInt(IDC_EDIT2, NULL, FALSE);
	if(start < end || !sample.uFlags[CHN_LOOP])
	{
		const int n = m_ComboLoopType.GetCurSel();
		sample.SetLoop(start, end, n > 0, n == 2, m_sndFile);
		SetModified(SampleHint().Info().Data(), false, false);
	}
}


void CCtrlSamples::OnSustainTypeChanged()
//---------------------------------------
{
	if(IsLocked()) return;
	const int n = m_ComboSustainType.GetCurSel();
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	bool wasDisabled = !sample.uFlags[CHN_SUSTAINLOOP];

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
		m_modDoc.UpdateAllViews(NULL, SampleHint(m_nSample).Info().Data());
	} else
	{
		sample.PrecomputeLoops(m_sndFile);
	}
	ctrlSmp::UpdateLoopPoints(sample, m_sndFile);
	SetModified(SampleHint().Info(), false, false);
}


void CCtrlSamples::OnSustainPointsChanged()
//-----------------------------------------
{
	if(IsLocked()) return;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SmpLength start = GetDlgItemInt(IDC_EDIT3, NULL, FALSE), end = GetDlgItemInt(IDC_EDIT4, NULL, FALSE);
	if(start < end || !sample.uFlags[CHN_SUSTAINLOOP])
	{
		const int n = m_ComboSustainType.GetCurSel();
		sample.SetSustainLoop(start, end, n > 0, n == 2, m_sndFile);
		SetModified(SampleHint().Info().Data(), false, false);
	}
}


#define SMPLOOP_ACCURACY	7	// 5%
#define BIDILOOP_ACCURACY	2	// 5%


bool MPT_LoopCheck(int sstart0, int sstart1, int send0, int send1)
//----------------------------------------------------------------
{
	int dse0 = send0 - sstart0;
	if ((dse0 < -SMPLOOP_ACCURACY) || (dse0 > SMPLOOP_ACCURACY)) return false;
	int dse1 = send1 - sstart1;
	if ((dse1 < -SMPLOOP_ACCURACY) || (dse1 > SMPLOOP_ACCURACY)) return false;
	int dstart = sstart1 - sstart0;
	int dend = send1 - send0;
	if (!dstart) dstart = dend >> 7;
	if (!dend) dend = dstart >> 7;
	if ((dstart ^ dend) < 0) return false;
	int delta = dend - dstart;
	return ((delta > -SMPLOOP_ACCURACY) && (delta < SMPLOOP_ACCURACY));
}


bool MPT_BidiEndCheck(int spos0, int spos1, int spos2)
//----------------------------------------------------
{
	int delta0 = spos1 - spos0;
	int delta1 = spos2 - spos1;
	int delta2 = spos2 - spos0;
	if (!delta0) delta0 = delta1 >> 7;
	if (!delta1) delta1 = delta0 >> 7;
	if ((delta1 ^ delta0) < 0) return false;
	return ((delta0 >= -1) && (delta0 <= 0) && (delta1 >= -1) && (delta1 <= 0) && (delta2 >= -1) && (delta2 <= 0));
}


bool MPT_BidiStartCheck(int spos0, int spos1, int spos2)
//------------------------------------------------------
{
	int delta1 = spos1 - spos0;
	int delta0 = spos2 - spos1;
	int delta2 = spos2 - spos0;
	if (!delta0) delta0 = delta1 >> 7;
	if (!delta1) delta1 = delta0 >> 7;
	if ((delta1 ^ delta0) < 0) return false;
	return ((delta0 >= -1) && (delta0 <= 0) && (delta1 > -1) && (delta1 <= 0) && (delta2 >= -1) && (delta2 <= 0));
}



void CCtrlSamples::OnVScroll(UINT nCode, UINT, CScrollBar *)
//----------------------------------------------------------
{
	CHAR s[256];
	if(IsLocked()) return;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	const uint8 *pSample = static_cast<const uint8 *>(sample.pSample);
	const uint32 pinc = sample.GetBytesPerSample();
	int pos;
	bool redraw = false;

	LockControls();
	if ((!sample.nLength) || (!pSample)) goto NoSample;
	if (sample.uFlags[CHN_16BIT])
	{
		pSample++;
	}
	// Loop Start
	if ((pos = m_SpinLoopStart.GetPos32()) != 0)
	{
		bool bOk = false;
		const uint8 *p = pSample + sample.nLoopStart * pinc;
		int find0 = (int)pSample[sample.nLoopEnd*pinc-pinc];
		int find1 = (int)pSample[sample.nLoopEnd*pinc];
		// Find Next LoopStart Point
		if (pos > 0)
		{
			for (SmpLength i=sample.nLoopStart+1; i+16<sample.nLoopEnd; i++)
			{
				p += pinc;
				bOk = sample.uFlags[CHN_PINGPONGLOOP] ? MPT_BidiStartCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					sample.nLoopStart = i;
					break;
				}
			}
		} else
		// Find Prev LoopStart Point
		{
			for (SmpLength i=sample.nLoopStart; i; )
			{
				i--;
				p -= pinc;
				bOk = sample.uFlags[CHN_PINGPONGLOOP] ? MPT_BidiStartCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					sample.nLoopStart = i;
					break;
				}
			}
		}
		if (bOk)
		{
			wsprintf(s, "%u", sample.nLoopStart);
			m_EditLoopStart.SetWindowText(s);
			redraw = true;
			sample.PrecomputeLoops(m_sndFile);
		}
		m_SpinLoopStart.SetPos(0);
	}
	// Loop End
	pos = m_SpinLoopEnd.GetPos32();
	if ((pos) && (sample.nLoopEnd))
	{
		bool bOk = false;
		const uint8 *p = pSample + sample.nLoopEnd * pinc;
		int find0 = (int)pSample[sample.nLoopStart*pinc];
		int find1 = (int)pSample[sample.nLoopStart*pinc+pinc];
		// Find Next LoopEnd Point
		if (pos > 0)
		{
			for (SmpLength i=sample.nLoopEnd+1; i<=sample.nLength; i++, p+=pinc)
			{
				bOk = sample.uFlags[CHN_PINGPONGLOOP] ? MPT_BidiEndCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					sample.nLoopEnd = i;
					break;
				}
			}
		} else
		// Find Prev LoopEnd Point
		{
			for (SmpLength i=sample.nLoopEnd; i>sample.nLoopStart+16; )
			{
				i--;
				p -= pinc;
				bOk = sample.uFlags[CHN_PINGPONGLOOP] ? MPT_BidiEndCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					sample.nLoopEnd = i;
					break;
				}
			}
		}
		if (bOk)
		{
			wsprintf(s, "%u", sample.nLoopEnd);
			m_EditLoopEnd.SetWindowText(s);
			redraw = true;
			sample.PrecomputeLoops(m_sndFile);
		}
		m_SpinLoopEnd.SetPos(0);
	}
	// Sustain Loop Start
	pos = m_SpinSustainStart.GetPos32();
	if ((pos) && (sample.nSustainEnd))
	{
		bool bOk = false;
		const uint8 *p = pSample + sample.nSustainStart * pinc;
		int find0 = (int)pSample[sample.nSustainEnd*pinc-pinc];
		int find1 = (int)pSample[sample.nSustainEnd*pinc];
		// Find Next Sustain LoopStart Point
		if (pos > 0)
		{
			for (SmpLength i=sample.nSustainStart+1; i+16<sample.nSustainEnd; i++)
			{
				p += pinc;
				bOk = sample.uFlags[CHN_PINGPONGSUSTAIN] ? MPT_BidiStartCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					sample.nSustainStart = i;
					break;
				}
			}
		} else
		// Find Prev Sustain LoopStart Point
		{
			for (SmpLength i=sample.nSustainStart; i; )
			{
				i--;
				p -= pinc;
				bOk = sample.uFlags[CHN_PINGPONGSUSTAIN] ? MPT_BidiStartCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					sample.nSustainStart = i;
					break;
				}
			}
		}
		if (bOk)
		{
			wsprintf(s, "%u", sample.nSustainStart);
			m_EditSustainStart.SetWindowText(s);
			redraw = true;
			sample.PrecomputeLoops(m_sndFile);
		}
		m_SpinSustainStart.SetPos(0);
	}
	// Sustain Loop End
	pos = m_SpinSustainEnd.GetPos32();
	if (pos)
	{
		bool bOk = false;
		const uint8 *p = pSample + sample.nSustainEnd * pinc;
		int find0 = (int)pSample[sample.nSustainStart*pinc];
		int find1 = (int)pSample[sample.nSustainStart*pinc+pinc];
		// Find Next LoopEnd Point
		if (pos > 0)
		{
			for (SmpLength i=sample.nSustainEnd+1; i+1<sample.nLength; i++, p+=pinc)
			{
				bOk = sample.uFlags[CHN_PINGPONGSUSTAIN] ? MPT_BidiEndCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					sample.nSustainEnd = i;
					break;
				}
			}
		} else
		// Find Prev LoopEnd Point
		{
			for (SmpLength i=sample.nSustainEnd; i>sample.nSustainStart+16; )
			{
				i--;
				p -= pinc;
				bOk = sample.uFlags[CHN_PINGPONGSUSTAIN] ? MPT_BidiEndCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					sample.nSustainEnd = i;
					break;
				}
			}
		}
		if (bOk)
		{
			wsprintf(s, "%u", sample.nSustainEnd);
			m_EditSustainEnd.SetWindowText(s);
			redraw = true;
			sample.PrecomputeLoops(m_sndFile);
		}
		m_SpinSustainEnd.SetPos(0);
	}
NoSample:
	// FineTune / C-5 Speed
	if ((pos = m_SpinFineTune.GetPos32()) != 0)
	{
		if (m_sndFile.m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			uint32 d = sample.nC5Speed;
			if (d < 1) d = 8363;
			d = Util::Round<uint32>(d * std::pow(2.0, (pos * TrackerSettings::Instance().m_nFinetuneStep) / 1200.0));
			sample.nC5Speed = Clamp(d, 1u, 9999999u); // 9999999 is max. in Impulse Tracker
			int transp = ModSample::FrequencyToTranspose(sample.nC5Speed) >> 7;
			int basenote = (NOTE_MIDDLEC - NOTE_MIN) + transp;
			Clamp(basenote, BASENOTE_MIN, BASENOTE_MAX);
			basenote -= BASENOTE_MIN;
			if (basenote != m_CbnBaseNote.GetCurSel()) m_CbnBaseNote.SetCurSel(basenote);
			SetDlgItemInt(IDC_EDIT5, sample.nC5Speed, FALSE);
		} else
		{
			int ftune = (int)sample.nFineTune;
			// MOD finetune range -8 to 7 translates to -128 to 112
			if(m_sndFile.GetType() & MOD_TYPE_MOD)
			{
				ftune = Clamp((ftune >> 4) + pos, -8, 7);
				sample.nFineTune = MOD2XMFineTune((signed char)ftune);
			} else
			{
				ftune = Clamp(ftune + pos, -128, 127);
				sample.nFineTune = (signed char)ftune;
			}
			SetDlgItemInt(IDC_EDIT5, ftune, TRUE);
		}
		redraw = true;
		m_SpinFineTune.SetPos(0);
		OnFineTuneChangedDone();
	}
	if(nCode == SB_ENDSCROLL) SwitchToView();
	if(redraw)
	{
		SetModified(SampleHint().Info().Data(), false, false);
	}
	UnlockControls();
}


//rewbs.customKeys
BOOL CCtrlSamples::PreTranslateMessage(MSG *pMsg)
//-----------------------------------------------
{
	if (pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) ||
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler* ih = CMainFrame::GetInputHandler();

			//Translate message manually
			UINT nChar = (UINT)pMsg->wParam;
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);
			InputTargetContext ctx = (InputTargetContext)(kCtxViewSamples);

			if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT) != kcNull)
				return true; // Mapped to a command, no need to pass message on.
		}

	}

	return CModControlDlg::PreTranslateMessage(pMsg);
}

LRESULT CCtrlSamples::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
//--------------------------------------------------------------------
{
	if (wParam == kcNull)
		return NULL;

	int transpose = 0;
	switch(wParam)
	{
	case kcSampleLoad:		OnSampleOpen(); return wParam;
	case kcSampleSave:		OnSampleSave(); return wParam;
	case kcSampleNew:		InsertSample(false); return wParam;
	case kcSampleDuplicate:	InsertSample(true); return wParam;

	case kcSampleTransposeUp: transpose = 1; break;
	case kcSampleTransposeDown: transpose = -1; break;
	case kcSampleTransposeOctUp: transpose = 12; break;
	case kcSampleTransposeOctDown: transpose = -12; break;

	case kcSampleUpsample:
	case kcSampleDownsample:
		{
			uint32_t oldRate = m_sndFile.GetSample(m_nSample).GetSampleRate(m_sndFile.GetType());
			ApplyResample(wParam == kcSampleUpsample ? oldRate * 2 : oldRate / 2, TrackerSettings::Instance().sampleEditorDefaultResampler);
		}
		break;
	case kcSampleResample:
		OnResample();
		break;
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

	return 0;
}
//end rewbs.customKeys


// Return currently selected part of the sample.
// The whole sample size will be returned if no part of the sample is selected.
// However, point.bSelected indicates whether a sample selection exists or not.
CCtrlSamples::SampleSelectionPoints CCtrlSamples::GetSelectionPoints()
//--------------------------------------------------------------------
{
	SampleSelectionPoints points;
	SAMPLEVIEWSTATE viewstate;
	const ModSample &sample = m_sndFile.GetSample(m_nSample);

	MemsetZero(viewstate);
	SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&viewstate);
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
//---------------------------------------------------------------------
{
	const ModSample &sample = m_sndFile.GetSample(m_nSample);

	Limit(nStart, SmpLength(0), sample.nLength);
	Limit(nEnd, SmpLength(0), sample.nLength);

	SAMPLEVIEWSTATE viewstate;
	MemsetZero(viewstate);
	SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&viewstate);

	viewstate.dwBeginSel = nStart;
	viewstate.dwEndSel = nEnd;
	SendViewMessage(VIEWMSG_LOADSTATE, (LPARAM)&viewstate);
}


// Crossfade loop to create smooth loop transitions
void CCtrlSamples::OnXFade()
//--------------------------
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	if(sample.pSample == nullptr || sample.nLength == 0)
	{
		MessageBeep(MB_ICONWARNING);
		SwitchToView();
		return;
	}
	if((sample.nLoopEnd <= sample.nLoopStart || sample.nLoopEnd > sample.nLength)
		&& (sample.nSustainEnd <= sample.nSustainStart|| sample.nSustainEnd > sample.nLength))
	{
		Reporting::Error("Crossfade requires a sample loop to work.", this);
		SwitchToView();
		return;
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
		const SmpLength loopStart = dlg.m_useSustainLoop ? sample.nSustainStart: sample.nLoopStart;
		const SmpLength loopEnd = dlg.m_useSustainLoop ? sample.nSustainEnd: sample.nLoopEnd;
		const SmpLength maxSamples = Util::Min(sample.nLength, loopStart, loopEnd / 2);
		SmpLength fadeSamples = dlg.PercentToSamples(dlg.m_fadeLength);
		LimitMax(fadeSamples, maxSamples);
		if(fadeSamples < 2) return;

		m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_update, "Crossfade",
			loopEnd - fadeSamples,
			loopEnd + (dlg.m_afterloopFade ? std::min(sample.nLength - loopEnd, fadeSamples) : 0));

		if(ctrlSmp::XFadeSample(sample, fadeSamples, dlg.m_fadeLaw, dlg.m_afterloopFade, dlg.m_useSustainLoop, m_sndFile))
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
//-----------------------------
{
	SampleSelectionPoints selection = GetSelectionPoints();
	if(!selection.selectionActive)
	{
		selection.nStart = selection.nEnd = 0;
	}

	Autotune at(m_sndFile.GetSample(m_nSample), m_sndFile.GetType(), selection.nStart, selection.nEnd);
	if(at.CanApply())
	{
		CAutotuneDlg dlg(this);
		if(dlg.DoModal() == IDOK)
		{
			BeginWaitCursor();
			m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_none, "Automatic Sample Tuning");
			at.Apply(static_cast<double>(dlg.GetPitchReference()), dlg.GetTargetNote());
			SetModified(SampleHint().Info(), true, false);
			EndWaitCursor();
		}
	}
	SwitchToView();
}


void CCtrlSamples::OnKeepSampleOnDisk()
//-------------------------------------
{
	SAMPLEINDEX first = m_nSample, last = m_nSample;
	if(CMainFrame::GetInputHandler()->ShiftPressed())
	{
		first = 1;
		last = m_sndFile.GetNumSamples();
	}

	const bool enable = IsDlgButtonChecked(IDC_CHECK2) != BST_UNCHECKED;
	for(SAMPLEINDEX i = first; i <= last; i++)
	{
		m_sndFile.GetSample(i).uFlags.set(SMP_KEEPONDISK, enable && m_sndFile.SampleHasPath(i));
		m_modDoc.UpdateAllViews(nullptr, SampleHint(i).Info().Names(), this);
	}
	m_modDoc.SetModified();
}


// When changing auto vibrato properties, propagate them to other samples of the same instrument in XM edit mode.
void CCtrlSamples::PropagateAutoVibratoChanges()
//----------------------------------------------
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
			for(auto it = referencedSamples.cbegin(); it != referencedSamples.cend(); it++)
			{
				m_modDoc.UpdateAllViews(nullptr, SampleHint(*it).Info(), this);
			}
		}
	}
}


void CCtrlSamples::OnXButtonUp(UINT nFlags, UINT nButton, CPoint point)
//---------------------------------------------------------------------
{
	if(nButton == XBUTTON1) OnPrevInstrument();
	else if(nButton == XBUTTON2) OnNextInstrument();
	CModControlDlg::OnXButtonUp(nFlags, nButton, point);
	SwitchToView();
}

OPENMPT_NAMESPACE_END
