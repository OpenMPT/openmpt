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
#include "mptrack.h"
#include "mainfrm.h"
#include "InputHandler.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "ctrl_smp.h"
#include "view_smp.h"
#include "SampleEditorDialogs.h"
#include "dlg_misc.h"
#include "PSRatioCalc.h" //rewbs.timeStretchMods
#include "soundtouch/include/SoundTouch.h"
#include "soundtouch/source/SoundTouchDLL/SoundTouchDLL.h"
#include "smbPitchShift/smbPitchShift.h"
#include "modsmp_ctrl.h"
#include "Autotune.h"
#include "../common/StringFixer.h"
#include "MemoryMappedFile.h"
#include "../soundlib/FileReader.h"
#include "../soundlib/SampleFormatConverters.h"
#include "FileDialog.h"
#ifdef _DEBUG
#include <math.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


OPENMPT_NAMESPACE_BEGIN


#define FLOAT_ERROR 1.0e-20f

// Float point comparison
static bool EqualTof(const float a, const float b)
{
	return (fabs(a - b) <= FLOAT_ERROR);
}

// Round floating point value to "digit" number of digits
static float Round(const float value, const int digit)
{
	float v = 0.1f * (value * powf(10.0f, (float)(digit + 1)) + (value < 0.0f ? -5.0f : 5.0f));
	modff(v, &v);
	return v / powf(10.0f, (float)digit);
}

template<int v>
static int PowerOf2Exponent()
{
	if(v <= 1)
		return 0;
	else
		return 1 + PowerOf2Exponent<v / 2>();
}


#define	BASENOTE_MIN	(1*12)	// C-1
#define	BASENOTE_MAX	(9*12)	// C-9

BEGIN_MESSAGE_MAP(CCtrlSamples, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlSamples)
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_SAMPLE_NEW,			OnSampleNew)
	ON_COMMAND(IDC_SAMPLE_OPEN,			OnSampleOpen)
	ON_COMMAND(IDC_SAMPLE_SAVEAS,		OnSampleSave)
	ON_COMMAND(IDC_SAMPLE_PLAY,			OnSamplePlay)
	ON_COMMAND(IDC_SAMPLE_NORMALIZE,	OnNormalize)
	ON_COMMAND(IDC_SAMPLE_AMPLIFY,		OnAmplify)
	ON_COMMAND(IDC_SAMPLE_UPSAMPLE,		OnUpsample)
	ON_COMMAND(IDC_SAMPLE_DOWNSAMPLE,	OnDownsample)
	ON_COMMAND(IDC_SAMPLE_REVERSE,		OnReverse)
	ON_COMMAND(IDC_SAMPLE_SILENCE,		OnSilence)
	ON_COMMAND(IDC_SAMPLE_INVERT,		OnInvert)
	ON_COMMAND(IDC_SAMPLE_SIGN_UNSIGN,	OnSignUnSign)
	ON_COMMAND(IDC_SAMPLE_DCOFFSET,		OnRemoveDCOffset)
	ON_COMMAND(IDC_SAMPLE_XFADE,		OnXFade)
	ON_COMMAND(IDC_SAMPLE_AUTOTUNE,		OnAutotune)
	ON_COMMAND(IDC_CHECK1,				OnSetPanningChanged)
	ON_COMMAND(ID_PREVINSTRUMENT,		OnPrevInstrument)
	ON_COMMAND(ID_NEXTINSTRUMENT,		OnNextInstrument)
	ON_COMMAND(IDC_BUTTON1,				OnPitchShiftTimeStretch)
	ON_COMMAND(IDC_BUTTON2,				OnEstimateSampleSize)
	ON_COMMAND(IDC_CHECK3,				OnEnableStretchToSize)
	ON_EN_CHANGE(IDC_SAMPLE_NAME,		OnNameChanged)
	ON_EN_CHANGE(IDC_SAMPLE_FILENAME,	OnFileNameChanged)
	ON_EN_CHANGE(IDC_EDIT_SAMPLE,		OnSampleChanged)
	ON_EN_CHANGE(IDC_EDIT1,				OnLoopStartChanged)
	ON_EN_CHANGE(IDC_EDIT2,				OnLoopEndChanged)
	ON_EN_CHANGE(IDC_EDIT3,				OnSustainStartChanged)
	ON_EN_CHANGE(IDC_EDIT4,				OnSustainEndChanged)
	ON_EN_CHANGE(IDC_EDIT5,				OnFineTuneChanged)
	ON_EN_CHANGE(IDC_EDIT7,				OnVolumeChanged)
	ON_EN_CHANGE(IDC_EDIT8,				OnGlobalVolChanged)
	ON_EN_CHANGE(IDC_EDIT9,				OnPanningChanged)
	ON_EN_CHANGE(IDC_EDIT14,			OnVibSweepChanged)
	ON_EN_CHANGE(IDC_EDIT15,			OnVibDepthChanged)
	ON_EN_CHANGE(IDC_EDIT16,			OnVibRateChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_BASENOTE,OnBaseNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_ZOOM,	OnZoomChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1,		OnLoopTypeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,		OnSustainTypeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,		OnVibTypeChanged)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,		OnCustomKeyMsg)	//rewbs.customKeys
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
	m_nPreviousRawFormat(SampleIO::_8bit, SampleIO::mono, SampleIO::littleEndian, SampleIO::unsignedPCM)
{
	m_nSample = 1;
	m_nLockCount = 1;
	rememberRawFormat = false;
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

	// Zoom Selection
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("Auto"), 0);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("1:1"), 1);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("2:1"), (DWORD_PTR)-2);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("4:1"), (DWORD_PTR)-3);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("8:1"), (DWORD_PTR)-4);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("16:1"), (DWORD_PTR)-5);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("32:1"), (DWORD_PTR)-6);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("1:2"), 2);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("1:4"), 3);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("1:8"), 4);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("1:16"), 5);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("1:32"), 6);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("1:64"), 7);
	m_ComboZoom.SetItemData(m_ComboZoom.AddString("1:128"), 8);
	m_ComboZoom.SetCurSel(0);
	// File ToolBar
	m_ToolBar1.Init(CMainFrame::GetMainFrame()->m_PatternIcons,CMainFrame::GetMainFrame()->m_PatternIconsDisabled);
	m_ToolBar1.AddButton(IDC_SAMPLE_NEW, TIMAGE_SAMPLE_NEW);
	m_ToolBar1.AddButton(IDC_SAMPLE_OPEN, TIMAGE_OPEN);
	m_ToolBar1.AddButton(IDC_SAMPLE_SAVEAS, TIMAGE_SAVE);
	// Edit ToolBar
	m_ToolBar2.Init(CMainFrame::GetMainFrame()->m_PatternIcons,CMainFrame::GetMainFrame()->m_PatternIconsDisabled);
	m_ToolBar2.AddButton(IDC_SAMPLE_PLAY, TIMAGE_PREVIEW);
	m_ToolBar2.AddButton(IDC_SAMPLE_NORMALIZE, TIMAGE_SAMPLE_NORMALIZE);
	m_ToolBar2.AddButton(IDC_SAMPLE_AMPLIFY, TIMAGE_SAMPLE_AMPLIFY);
	m_ToolBar2.AddButton(IDC_SAMPLE_DCOFFSET, TIMAGE_SAMPLE_DCOFFSET);
	m_ToolBar2.AddButton(IDC_SAMPLE_UPSAMPLE, TIMAGE_SAMPLE_UPSAMPLE);
	m_ToolBar2.AddButton(IDC_SAMPLE_DOWNSAMPLE, TIMAGE_SAMPLE_DOWNSAMPLE);
	m_ToolBar2.AddButton(IDC_SAMPLE_REVERSE, TIMAGE_SAMPLE_REVERSE);
	m_ToolBar2.AddButton(IDC_SAMPLE_SILENCE, TIMAGE_SAMPLE_SILENCE);
	m_ToolBar2.AddButton(IDC_SAMPLE_INVERT, TIMAGE_SAMPLE_INVERT);
	m_ToolBar2.AddButton(IDC_SAMPLE_SIGN_UNSIGN, TIMAGE_SAMPLE_UNSIGN);
	m_ToolBar2.AddButton(IDC_SAMPLE_XFADE, TIMAGE_SAMPLE_FIXLOOP);
	m_ToolBar2.AddButton(IDC_SAMPLE_AUTOTUNE, TIMAGE_SAMPLE_AUTOTUNE);
	// Setup Controls
	m_SpinVolume.SetRange(0, 64);
	m_SpinGlobalVol.SetRange(0, 64);

	// Auto vibrato
	m_ComboAutoVib.AddString("Sine");
	m_ComboAutoVib.AddString("Square");
	m_ComboAutoVib.AddString("Ramp Up");
	m_ComboAutoVib.AddString("Ramp Down");
	m_ComboAutoVib.AddString("Random");

	for (UINT i = BASENOTE_MIN; i < BASENOTE_MAX; i++)
	{
		CHAR s[32];
		wsprintf(s, "%s%d", szNoteNames[i%12], i/12);
		m_CbnBaseNote.AddString(s);
	}


	m_ComboFFT.ShowWindow(SW_SHOW);
	m_ComboPitch.ShowWindow(SW_SHOW);
	m_ComboQuality.ShowWindow(SW_SHOW);
	m_ComboFFT.ShowWindow(SW_SHOW);

	GetDlgItem(IDC_BUTTON1)->ShowWindow(SW_SHOW); // PitchShiftTimeStretch
	GetDlgItem(IDC_BUTTON2)->ShowWindow(SW_SHOW); // EstimateSampleSize
	GetDlgItem(IDC_CHECK3)->ShowWindow(SW_SHOW);  // EnableStretchToSize
	GetDlgItem(IDC_EDIT6)->ShowWindow(SW_SHOW);   // 
	GetDlgItem(IDC_GROUPBOX_PITCH_TIME)->ShowWindow(SW_SHOW);  //
	GetDlgItem(IDC_TEXT_PITCH)->ShowWindow(SW_SHOW);  //
	GetDlgItem(IDC_TEXT_QUALITY)->ShowWindow(SW_SHOW);  //
	GetDlgItem(IDC_TEXT_FFT)->ShowWindow(SW_SHOW);  //
	GetDlgItem(IDC_GROUPBOX_PITCH_TIME)->ShowWindow(SW_SHOW);  //
	
	CHAR str[16];

	// Pitch selection
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO4);
	if(combo)
	{
		// Allow pitch from -12 (1 octave down) to +12 (1 octave up)
		for(int i = -12 ; i <= 12 ; i++){
			if(i == 0) wsprintf(str,"none");
			else wsprintf(str,i < 0 ? " %d" : "+%d",i);
			combo->SetItemData(combo->AddString(str), i+12);
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
			wsprintf(str,"%d",i);
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
		const int exponent = PowerOf2Exponent<MAX_FRAME_LENGTH>();
		// Allow FFT size from 2^8 (256) to 2^exponent (MAX_FRAME_LENGTH)
		for(int i = 8 ; i <= exponent ; i++)
		{
			wsprintf(str,"%d",1<<i);
			combo->SetItemData(combo->AddString(str), i-8);
		}
		// Set 4096 as default FFT size
		combo->SetCurSel(4);
	}

	// Stretch ratio
	SetDlgItemInt(IDC_EDIT6,100,FALSE);

	// Stretch to size check box
	OnEnableStretchToSize();

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
		UpdateView((m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO, NULL);
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
	SetCurrentSample((lParam > 0) ? ((SAMPLEINDEX)lParam) : m_nSample);

	// Initial Update
	if (!m_bInitialized) UpdateView((m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_MODTYPE, NULL);
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if ((pFrame) && (m_hWndView)) PostViewMessage(VIEWMSG_LOADSTATE, (LPARAM)pFrame->GetSampleViewState());
	SwitchToView();

	// Combo boxes randomly disappear without this... why?
	Invalidate();
}


void CCtrlSamples::OnDeactivatePage()
//-----------------------------------
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if ((pFrame) && (m_hWndView)) SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)pFrame->GetSampleViewState());
	m_modDoc.NoteOff(0, true);
}


LRESULT CCtrlSamples::OnModCtrlMsg(WPARAM wParam, LPARAM lParam)
//---------------------------------------------------------------
{
	switch(wParam)
	{
	case CTRLMSG_SMP_PREVINSTRUMENT:
		OnPrevInstrument();
		break;

	case CTRLMSG_SMP_NEXTINSTRUMENT:
		OnNextInstrument();
		break;

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
		OnSampleNew();
		break;
	//end rewbs.customKeys

	default:
		return CModControlDlg::OnModCtrlMsg(wParam, lParam);
	}
	return 0;
}


BOOL CCtrlSamples::GetToolTipText(UINT uId, LPSTR pszText)
//--------------------------------------------------------
{
	if ((pszText) && (uId))
	{
		switch(uId)
		{
		case IDC_EDIT5:
		case IDC_SPIN5:
		case IDC_COMBO_BASENOTE:
			if ((m_sndFile.GetType() & (MOD_TYPE_XM | MOD_TYPE_MOD)) && (m_nSample))
			{
				const ModSample &sample = m_sndFile.GetSample(m_nSample);
				UINT nFreqHz = ModSample::TransposeToFrequency(sample.RelativeTone, sample.nFineTune);
				wsprintf(pszText, "%ldHz", nFreqHz);
				return TRUE;
			}
			break;
		case IDC_EDIT_STRETCHPARAMS:
			wsprintf(pszText, "SequenceMs SeekwindowMs OverlapMs");
			return TRUE;
		}
	}
	return FALSE;
}


void CCtrlSamples::UpdateView(DWORD dwHintMask, CObject *pObj)
//------------------------------------------------------------
{
	if(pObj == this) return;
	if (dwHintMask & HINT_MPTOPTIONS)
	{
		m_ToolBar1.UpdateStyle();
		m_ToolBar2.UpdateStyle();
	}
	if (!(dwHintMask & (HINT_SAMPLEINFO|HINT_MODTYPE))) return;

	const SAMPLEINDEX updateSmp = (dwHintMask >> HINT_SHIFT_SMP);

	if(updateSmp != m_nSample && updateSmp != 0 && !(dwHintMask & HINT_MODTYPE)) return;
	LockControls();
	if (!m_bInitialized) dwHintMask |= HINT_MODTYPE;
	// Updating Ranges
	if (dwHintMask & HINT_MODTYPE)
	{
		const CModSpecifications *specs = &m_sndFile.GetModSpecifications();

		// Limit text fields
		m_EditName.SetLimitText(specs->sampleNameLengthMax);
		m_EditFileName.SetLimitText(specs->sampleFilenameLengthMax);

		BOOL b;
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
		b = (m_sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
		m_ComboSustainType.EnableWindow(b);
		m_SpinSustainStart.EnableWindow(b);
		m_SpinSustainEnd.EnableWindow(b);
		m_EditSustainStart.EnableWindow(b);
		m_EditSustainEnd.EnableWindow(b);

		// Finetune / C-5 Speed / BaseNote
		b = (m_sndFile.GetType() & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
		SetDlgItemText(IDC_TEXT7, (b) ? "Freq. (Hz)" : "Finetune");
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
	if (dwHintMask & (HINT_MODTYPE|HINT_SAMPLEINFO))
	{
		if(m_nSample > m_sndFile.GetNumSamples())
		{
			SetCurrentSample(m_sndFile.GetNumSamples());
		}
		const ModSample &sample = m_sndFile.GetSample(m_nSample);
		CHAR s[128];
		DWORD d;

		m_SpinSample.SetRange(1, m_sndFile.GetNumSamples());
		
		// Length / Type
		wsprintf(s, "%d-bit %s, len: %d", sample.GetElementarySampleSize() * 8, sample.uFlags[CHN_STEREO] ? "stereo" : "mono", sample.nLength);
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
		CheckDlgButton(IDC_CHECK1, (sample.uFlags[CHN_PANNING]) ? MF_CHECKED : MF_UNCHECKED);
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
			wsprintf(s, "%lu", sample.nC5Speed);
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
		int basenote = 60 - transp;
		if (basenote < BASENOTE_MIN) basenote = BASENOTE_MIN;
		if (basenote >= BASENOTE_MAX) basenote = BASENOTE_MAX-1;
		basenote -= BASENOTE_MIN;
		if (basenote != m_CbnBaseNote.GetCurSel()) m_CbnBaseNote.SetCurSel(basenote);
		// AutoVibrato
		m_ComboAutoVib.SetCurSel(sample.nVibType);
		SetDlgItemInt(IDC_EDIT14, (UINT)sample.nVibSweep);
		SetDlgItemInt(IDC_EDIT15, (UINT)sample.nVibDepth);
		SetDlgItemInt(IDC_EDIT16, (UINT)sample.nVibRate);
		// Loop
		d = 0;
		if (sample.uFlags[CHN_LOOP]) d = sample.uFlags[CHN_PINGPONGLOOP] ? 2 : 1;
		if (sample.uFlags[CHN_REVERSE]) d |= 4;
		m_ComboLoopType.SetCurSel(d);
		wsprintf(s, "%lu", sample.nLoopStart);
		m_EditLoopStart.SetWindowText(s);
		wsprintf(s, "%lu", sample.nLoopEnd);
		m_EditLoopEnd.SetWindowText(s);
		// Sustain Loop
		d = 0;
		if (sample.uFlags[CHN_SUSTAINLOOP]) d = sample.uFlags[CHN_PINGPONGSUSTAIN] ? 2 : 1;
		m_ComboSustainType.SetCurSel(d);
		wsprintf(s, "%lu", sample.nSustainStart);
		m_EditSustainStart.SetWindowText(s);
		wsprintf(s, "%lu", sample.nSustainEnd);
		m_EditSustainEnd.SetWindowText(s);
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
void CCtrlSamples::SetModified(DWORD mask, bool updateAll)
//--------------------------------------------------------
{
	m_modDoc.SetModified();
	m_modDoc.UpdateAllViews(nullptr, mask | (m_nSample << HINT_SHIFT_SMP), updateAll ? nullptr : this);
}



bool CCtrlSamples::OpenSample(const mpt::PathString &fileName)
//------------------------------------------------------------
{
	CMappedFile f;
	BeginWaitCursor();
	if(!f.Open(fileName))
	{
		EndWaitCursor();
		return false;
	}

	FileReader file = f.GetFile();
	if(!file.IsValid())
	{
		EndWaitCursor();
		return false;
	}
	
	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_replace, "Replace");
	bool bOk = m_sndFile.ReadSampleFromFile(m_nSample, file, TrackerSettings::Instance().m_MayNormalizeSamplesOnLoad);
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	if (!bOk)
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
				sample.uFlags.reset(CHN_LOOP | CHN_SUSTAINLOOP);
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
	}

	EndWaitCursor();
	if (bOk)
	{
		TrackerDirectories::Instance().SetWorkingDirectory(fileName, DIR_SAMPLES, true);
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
		SetModified(HINT_SAMPLEDATA | HINT_SAMPLEINFO | HINT_SMPNAMES, true);
	}
	return true;
}


bool CCtrlSamples::OpenSample(const CSoundFile &sndFile, SAMPLEINDEX nSample)
//---------------------------------------------------------------------------
{
	if(!nSample || nSample > sndFile.GetNumSamples()) return false;

	BeginWaitCursor();

	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_replace, "Replace");
	m_sndFile.ReadSampleFromSong(m_nSample, sndFile, nSample);
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	if((m_sndFile.GetType() & MOD_TYPE_XM) && (!sample.uFlags[CHN_PANNING]))
	{
		sample.nPan = 128;
		sample.uFlags.set(CHN_PANNING);
	}

	EndWaitCursor();

	SetModified(HINT_SAMPLEDATA | HINT_SAMPLEINFO | HINT_SMPNAMES, true);

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


void CCtrlSamples::OnSampleNew()
//------------------------------
{
	const bool duplicate = CMainFrame::GetInputHandler()->ShiftPressed();

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

		m_modDoc.UpdateAllViews(NULL, (smp << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA | HINT_SMPNAMES);
		if(m_modDoc.GetNumInstruments() > 0 && m_modDoc.FindSampleParent(smp) == INSTRUMENTINDEX_INVALID)
		{
			if(Reporting::Confirm("This sample is not used by any instrument. Do you want to create a new instrument using this sample?") == cnfYes)
			{
				INSTRUMENTINDEX nins = m_modDoc.InsertInstrument(smp);
				m_modDoc.UpdateAllViews(NULL, (nins << HINT_SHIFT_INS) | HINT_INSTRUMENT | HINT_INSNAMES | HINT_ENVELOPE);
				m_parent.InstrumentChanged(nins);
			}
		}
	}
	SwitchToView();
}


void CCtrlSamples::OnSampleOpen()
//-------------------------------
{
	static int nLastIndex = 0;
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.EnableAudioPreview()
		.ExtensionFilter("All Samples|*.wav;*.flac;*.pat;*.s3i;*.smp;*.snd;*.raw;*.xi;*.aif;*.aiff;*.its;*.iff;*.8sv;*.8svx;*.svx;*.pcm;*.mp1;*.mp2;*.mp3|"
			"Wave Files (*.wav)|*.wav|"
	#ifndef NO_FLAC
			"FLAC Files (*.flac)|*.flac|"
	#endif // NO_FLAC
	#ifndef NO_MP3_SAMPLES
			"MPEG Files (*.mp1,*.mp2,*.mp3)|*.mp1;*.mp2;*.mp3|"
	#endif // NO_MP3_SAMPLES
			"XI Samples (*.xi)|*.xi|"
			"Impulse Tracker Samples (*.its)|*.its|"
			"ScreamTracker Samples (*.s3i,*.smp)|*.s3i;*.smp|"
			"GF1 Patches (*.pat)|*.pat|"
			"AIFF Files (*.aiff;*.8svx)|*.aif;*.aiff;*.iff;*.8sv;*.8svx;*.svx|"
			"Raw Samples (*.raw,*.snd,*.pcm)|*.raw;*.snd;*.pcm|"
			"All Files (*.*)|*.*||")
		.WorkingDirectory(TrackerDirectories::Instance().GetWorkingDirectory(DIR_SAMPLES))
		.FilterIndex(&nLastIndex);
	if(!dlg.Show(this)) return;

	TrackerDirectories::Instance().SetWorkingDirectory(dlg.GetWorkingDirectory(), DIR_SAMPLES);

	const FileDialog::PathList &files = dlg.GetFilenames();
	for(size_t counter = 0; counter < files.size(); counter++)
	{
		// If loading multiple samples, create new slots for them
		if(counter > 0)
		{
			OnSampleNew();
		}

		if(!OpenSample(files[counter]))
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
			.WorkingDirectory(TrackerDirectories::Instance().GetWorkingDirectory(DIR_SAMPLES))
			.FilterIndex(&filter);
	if(!dlg.Show(this)) return;

	BeginWaitCursor();

	const mpt::PathString ext = dlg.GetExtension();

	SAMPLEINDEX minSmp = m_nSample, maxSmp = m_nSample;
	CString sNumberFormat;
	if(doBatchSave)
	{
		minSmp = 1;
		maxSmp = m_sndFile.GetNumSamples();
		sNumberFormat.Format("%s%d%s", "%.", ((int)log10((float)maxSmp)) + 1, "d");
	}

	bool ok = false;
	for(SAMPLEINDEX smp = minSmp; smp <= maxSmp; smp++)
	{
		if (m_sndFile.GetSample(smp).pSample)
		{
			mpt::PathString fileName = dlg.GetFirstFile();
			if(doBatchSave)
			{
				CString sSampleNumber;
				CString sSampleName;
				CString sSampleFilename;
				sSampleNumber.Format(sNumberFormat, smp);

				sSampleName = (m_sndFile.m_szNames[smp][0]) ? m_sndFile.m_szNames[smp] : "untitled";
				sSampleFilename = (m_sndFile.GetSample(smp).filename[0]) ? m_sndFile.GetSample(smp).filename : m_sndFile.m_szNames[smp];
				SanitizeFilename(sSampleName);
				SanitizeFilename(sSampleFilename);

				fileName = mpt::PathString::FromWide(mpt::String::Replace(fileName.ToWide(), L"%sample_number%", mpt::ToWide(sSampleNumber)));
				fileName = mpt::PathString::FromWide(mpt::String::Replace(fileName.ToWide(), L"%sample_filename%", mpt::ToWide(sSampleFilename)));
				fileName = mpt::PathString::FromWide(mpt::String::Replace(fileName.ToWide(), L"%sample_name%", mpt::ToWide(sSampleName)));
			}
			if(!mpt::PathString::CompareNoCase(ext, MPT_PATHSTRING("raw")))
				ok = m_sndFile.SaveRAWSample(smp, fileName);
			else if(!mpt::PathString::CompareNoCase(ext, MPT_PATHSTRING("flac")))
				ok = m_sndFile.SaveFLACSample(smp, fileName);
			else
				ok = m_sndFile.SaveWAVSample(smp, fileName);
		}
	}
	EndWaitCursor();

	if (!ok)
	{
		ErrorBox(IDS_ERR_SAVESMP, this);
	} else
	{
		TrackerDirectories::Instance().SetWorkingDirectory(dlg.GetWorkingDirectory(), DIR_SAMPLES);
	}
	SwitchToView();
}


void CCtrlSamples::OnSamplePlay()
//-------------------------------
{
	// Commented out line to fix http://forum.openmpt.org/index.php?topic=1366.0
	// if ((m_pSndFile.IsPaused()) && (m_pModDoc.IsNotePlaying(0, m_nSample, 0)))
	if (m_modDoc.IsNotePlaying(0, m_nSample, 0))
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
		if(Reporting::Confirm(GetStrI18N(TEXT("This will normalize all samples independently. Continue?")), GetStrI18N(TEXT("Normalize"))) == cnfNo)
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
				int16 *p = (int16 *)sample.pSample;
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
				int8 *p = (int8 *)sample.pSample;
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
				m_modDoc.UpdateAllViews(NULL, (iSmp << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, NULL);
			}
		}
	}

	if(bModified)
	{
		SetModified(HINT_SAMPLEDATA, false);
	}
	EndWaitCursor();
	SwitchToView();
}


template<typename T>
void ApplyAmplifyImpl(void *pSample, SmpLength start, SmpLength end, int32 amp, bool fadeIn, bool fadeOut)
//--------------------------------------------------------------------------------------------------------
{
	T *p = static_cast<T *>(pSample) + start;
	SmpLength len = end - start;
	int64 l64 = static_cast<int64>(len);

	for(SmpLength i = 0; i < len; i++)
	{
		int32 l = (p[i] * amp) / 100;
		if(fadeIn) l = (int32)((l * (int64)i) / l64);
		if(fadeOut) l = (int32)((l * (int64)(len - i)) / l64);
		Limit(l, std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
		p[i] = static_cast<T>(l);
	}
}


void CCtrlSamples::ApplyAmplify(int32 lAmp, bool fadeIn, bool fadeOut)
//--------------------------------------------------------------------
{
	if((!m_sndFile.GetSample(m_nSample).pSample)) return;

	BeginWaitCursor();
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	SampleSelectionPoints selection = GetSelectionPoints();

	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_update, "Amplify", selection.nStart, selection.nEnd);

	if (sample.uFlags[CHN_STEREO]) { selection.nStart *= 2; selection.nEnd *= 2; }
	if ((fadeIn) && (fadeOut)) lAmp *= 4;
	if (sample.uFlags[CHN_16BIT])
	{
		ApplyAmplifyImpl<int16>(sample.pSample, selection.nStart, selection.nEnd, lAmp, fadeIn, fadeOut);
	} else
	{
		ApplyAmplifyImpl<int8>(sample.pSample, selection.nStart, selection.nEnd, lAmp, fadeIn, fadeOut);
	}
	sample.PrecomputeLoops(m_sndFile, false);
	SetModified(HINT_SAMPLEDATA, false);
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
		if(Reporting::Confirm(GetStrI18N(TEXT("This will process all samples independently. Continue?")), GetStrI18N(TEXT("DC Offset Removal"))) == cnfNo)
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
		m_modDoc.UpdateAllViews(nullptr, (iSmp << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO);
	}

	EndWaitCursor();
	SwitchToView();

	// fill the statusbar with some nice information

	CString dcInfo;
	if(numModified)
	{
		SetModified(HINT_SAMPLEDATA | HINT_SAMPLEINFO, true);
		if(numModified == 1)
		{
			dcInfo.Format(GetStrI18N(TEXT("Removed DC offset (%.1f%%)")), fReportOffset * 100);
		}
		else
		{
			dcInfo.Format(GetStrI18N(TEXT("Removed DC offset from %d samples (avg %0.1f%%)")), numModified, fReportOffset / numModified * 100);
		}
	}
	else
	{
		dcInfo.SetString(GetStrI18N(TEXT("No DC offset found")));
	}

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	pMainFrm->SetXInfoText(dcInfo);

}


void CCtrlSamples::OnAmplify()
//----------------------------
{
	static int16 snOldAmp = 100;
	CAmpDlg dlg(this, snOldAmp);
	if (dlg.DoModal() != IDOK) return;
	snOldAmp = dlg.m_nFactor;
	ApplyAmplify(dlg.m_nFactor, dlg.m_bFadeIn, dlg.m_bFadeOut);
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
		ApplyAmplify(100, (sel.nStart == 0), (sel.nEnd == m_sndFile.GetSample(m_nSample).nLength));
	} else
	{
		// Can't apply quick fade as no appropriate selection has been made, so ask the user to amplify the whole sample instead.
		OnAmplify();
	}
}


const int gSinc2x16Odd[16] =
{ // Kaiser window, beta=7.400
  -19,    97,  -295,   710, -1494,  2958, -6155, 20582, 20582, -6155,  2958, -1494,   710,  -295,    97,   -19,
};


void CCtrlSamples::OnUpsample()
//-----------------------------
{
	SmpLength dwStart, dwEnd, dwNewLen;
	UINT smplsize, newsmplsize;
	PVOID pOriginal, pNewSample;

	if((m_sndFile.GetSample(m_nSample).pSample == nullptr)) return;
	BeginWaitCursor();

	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SampleSelectionPoints selection = GetSelectionPoints();

	dwStart = selection.nStart;
	dwEnd = selection.nEnd;
	if (dwEnd > sample.nLength) dwEnd = sample.nLength;
	if (dwStart >= dwEnd)
	{
		dwStart = 0;
		dwEnd = sample.nLength;
	}

	smplsize = sample.GetBytesPerSample();
	newsmplsize = sample.GetNumChannels() * 2;	// new sample is always 16-Bit
	pOriginal = sample.pSample;
	dwNewLen = sample.nLength + (dwEnd - dwStart);
	pNewSample = NULL;
	if (dwNewLen <= MAX_SAMPLE_LENGTH) pNewSample = ModSample::AllocateSample(dwNewLen, newsmplsize);
	if (pNewSample)
	{
		m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_replace, "Upsample");

		const UINT nCh = sample.GetNumChannels();
		for (UINT iCh=0; iCh<nCh; iCh++)
		{
			SmpLength len = dwEnd - dwStart;
			int maxndx = sample.nLength;
			if (sample.uFlags[CHN_16BIT])
			{
				signed short *psrc = ((signed short *)pOriginal)+iCh;
				signed short *pdest = ((signed short *)pNewSample)+dwStart*nCh+iCh;
				for (SmpLength i=0; i<len; i++)
				{
					int accum = 0x80;
					for (int j=0; j<16; j++)
					{
						int ndx = dwStart + i + j - 7;
						if (ndx < 0) ndx = 0;
						if (ndx > maxndx) ndx = maxndx;
						accum += (psrc[ndx*nCh] * gSinc2x16Odd[j] + 0x40) >> 7;
					}
					accum >>= 8;
					if (accum > 0x7fff) accum = 0x7fff;
					if (accum < -0x8000) accum = -0x8000;
					pdest[0] = psrc[(dwStart+i)*nCh];
					pdest[nCh] = (short int)(accum);
					pdest += nCh*2;
				}
			} else
			{
				signed char *psrc = ((signed char *)pOriginal)+iCh;
				signed short *pdest = ((signed short *)pNewSample)+dwStart*nCh+iCh;
				for (SmpLength i=0; i<len; i++)
				{
					int accum = 0x40;
					for (int j=0; j<16; j++)
					{
						int ndx = dwStart + i + j - 7;
						if (ndx < 0) ndx = 0;
						if (ndx > maxndx) ndx = maxndx;
						accum += psrc[ndx*nCh] * gSinc2x16Odd[j];
					}
					accum >>= 7;
					if (accum > 0x7fff) accum = 0x7fff;
					if (accum < -0x8000) accum = -0x8000;
					pdest[0] = (signed short)((int)psrc[(dwStart+i)*nCh]<<8);
					pdest[nCh] = (signed short)(accum);
					pdest += nCh*2;
				}
			}
		}
		if (sample.uFlags[CHN_16BIT])
		{
			if (dwStart > 0) memcpy(pNewSample, pOriginal, dwStart*smplsize);
			if (dwEnd < sample.nLength) memcpy(((int8 *)pNewSample)+(dwStart+(dwEnd-dwStart)*2)*smplsize, ((int8 *)pOriginal)+(dwEnd*smplsize), (sample.nLength-dwEnd)*smplsize);
		} else
		{
			if (dwStart > 0)
			{
				for (SmpLength i=0; i<dwStart*nCh; i++)
				{
					((signed short *)pNewSample)[i] = (signed short)(((signed char *)pOriginal)[i] << 8);
				}
			}
			if (dwEnd < sample.nLength)
			{
				signed short *pdest = ((signed short *)pNewSample) + (dwEnd-dwStart)*nCh;
				for (SmpLength i=dwEnd*nCh; i<sample.nLength*nCh; i++)
				{
					pdest[i] = (signed short)(((signed char *)pOriginal)[i] << 8);
				}
			}
		}
		if (sample.nLoopStart >= dwEnd) sample.nLoopStart += (dwEnd-dwStart); else
		if (sample.nLoopStart > dwStart) sample.nLoopStart += (sample.nLoopStart - dwStart);
		if (sample.nLoopEnd >= dwEnd) sample.nLoopEnd += (dwEnd-dwStart); else
		if (sample.nLoopEnd > dwStart) sample.nLoopEnd += (sample.nLoopEnd - dwStart);
		if (sample.nSustainStart >= dwEnd) sample.nSustainStart += (dwEnd-dwStart); else
		if (sample.nSustainStart > dwStart) sample.nSustainStart += (sample.nSustainStart - dwStart);
		if (sample.nSustainEnd >= dwEnd) sample.nSustainEnd += (dwEnd-dwStart); else
		if (sample.nSustainEnd > dwStart) sample.nSustainEnd += (sample.nSustainEnd - dwStart);
		
		sample.uFlags.set(CHN_16BIT);
		ctrlSmp::ReplaceSample(sample, pNewSample, dwNewLen, m_sndFile);
		// Update loop wrap-around buffer
		sample.PrecomputeLoops(m_sndFile);
		
		if(!selection.selectionActive)
		{
			if(!(m_sndFile.GetType() & MOD_TYPE_MOD))
			{
				if(sample.nC5Speed < 1000000) sample.nC5Speed *= 2;
				if(sample.RelativeTone < 84) sample.RelativeTone += 12;
			}
		} else
		{
			SetSelectionPoints(dwStart, dwEnd + (dwEnd - dwStart));
		}
		SetModified(HINT_SAMPLEDATA | HINT_SAMPLEINFO, true);
	}
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnDownsample()
//-------------------------------
{
	SmpLength dwStart, dwEnd, dwRemove, dwNewLen;
	SmpLength smplsize;

	if((!m_sndFile.GetSample(m_nSample).pSample)) return;
	BeginWaitCursor();
	
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SampleSelectionPoints selection = GetSelectionPoints();

	dwStart = selection.nStart;
	dwEnd = selection.nEnd;
	if (dwEnd > sample.nLength) dwEnd = sample.nLength;
	if (dwStart >= dwEnd)
	{
		dwStart = 0;
		dwEnd = sample.nLength;
	}
	smplsize = sample.GetBytesPerSample();
	void *pOriginal = sample.pSample, *pNewSample = nullptr;
	dwRemove = (dwEnd-dwStart+1)>>1;
	dwNewLen = sample.nLength - dwRemove;
	dwEnd = dwStart+dwRemove*2;
	if ((dwNewLen >= 4) && (dwRemove)) pNewSample = ModSample::AllocateSample(dwNewLen, smplsize);
	if (pNewSample)
	{

		m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_replace, "Downsample");

		const UINT nCh = sample.GetNumChannels();
		for (UINT iCh=0; iCh<nCh; iCh++)
		{
			int len = dwRemove;
			int maxndx = sample.nLength;
			if (sample.uFlags[CHN_16BIT])
			{
				signed short *psrc = ((signed short *)pOriginal)+iCh;
				signed short *pdest = ((signed short *)pNewSample)+dwStart*nCh+iCh;
				for (int i=0; i<len; i++)
				{
					int accum = 0x100 + ((int)psrc[(dwStart+i*2)*nCh] << 8);
					for (int j=0; j<16; j++)
					{
						int ndx = dwStart + (i + j)*2 - 15;
						if (ndx < 0) ndx = 0;
						if (ndx > maxndx) ndx = maxndx;
						accum += (psrc[ndx*nCh] * gSinc2x16Odd[j] + 0x40) >> 7;
					}
					accum >>= 9;
					if (accum > 0x7fff) accum = 0x7fff;
					if (accum < -0x8000) accum = -0x8000;
					pdest[0] = (short int)accum;
					pdest += nCh;
				}
			} else
			{
				signed char *psrc = ((signed char *)pOriginal)+iCh;
				signed char *pdest = ((signed char *)pNewSample)+dwStart*nCh+iCh;
				for (int i=0; i<len; i++)
				{
					int accum = 0x100 + ((int)psrc[(dwStart+i*2)*nCh] << 8);
					for (int j=0; j<16; j++)
					{
						int ndx = dwStart + (i + j)*2 - 15;
						if (ndx < 0) ndx = 0;
						if (ndx > maxndx) ndx = maxndx;
						accum += (psrc[ndx*nCh] * gSinc2x16Odd[j] + 0x40) >> 7;
					}
					accum >>= 9;
					if (accum > 0x7f) accum = 0x7f;
					if (accum < -0x80) accum = -0x80;
					pdest[0] = (signed char)accum;
					pdest += nCh;
				}
			}
		}
		if (dwStart > 0) memcpy(pNewSample, pOriginal, dwStart*smplsize);
		if (dwEnd < sample.nLength) memcpy(((int8 *)pNewSample)+(dwStart+dwRemove)*smplsize, ((int8 *)pOriginal)+((dwStart+dwRemove*2)*smplsize), (sample.nLength-dwEnd)*smplsize);
		if (sample.nLoopStart >= dwEnd) sample.nLoopStart -= dwRemove; else
		if (sample.nLoopStart > dwStart) sample.nLoopStart -= (sample.nLoopStart - dwStart)/2;
		if (sample.nLoopEnd >= dwEnd) sample.nLoopEnd -= dwRemove; else
		if (sample.nLoopEnd > dwStart) sample.nLoopEnd -= (sample.nLoopEnd - dwStart)/2;
		if (sample.nLoopEnd > dwNewLen) sample.nLoopEnd = dwNewLen;
		if (sample.nSustainStart >= dwEnd) sample.nSustainStart -= dwRemove; else
		if (sample.nSustainStart > dwStart) sample.nSustainStart -= (sample.nSustainStart - dwStart)/2;
		if (sample.nSustainEnd >= dwEnd) sample.nSustainEnd -= dwRemove; else
		if (sample.nSustainEnd > dwStart) sample.nSustainEnd -= (sample.nSustainEnd - dwStart)/2;
		if (sample.nSustainEnd > dwNewLen) sample.nSustainEnd = dwNewLen;

		ctrlSmp::ReplaceSample(sample, pNewSample, dwNewLen, m_sndFile);
		// Update loop wrap-around buffer
		sample.PrecomputeLoops(m_sndFile);

		if(!selection.selectionActive)
		{
			if(!(m_sndFile.GetType() & MOD_TYPE_MOD))
			{
				if(sample.nC5Speed > 2000) sample.nC5Speed /= 2;
				if(sample.RelativeTone > -84) sample.RelativeTone -= 12;
			}
		} else
		{
			SetSelectionPoints(dwStart, dwStart + dwRemove);
		}
		SetModified(HINT_SAMPLEDATA | HINT_SAMPLEINFO, true);
	}
	EndWaitCursor();
	SwitchToView();
}


#define MAX_BUFFER_LENGTH	8192
#define CLIP_SOUND(v)		Limit(v, -1.0f, 1.0f);

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
	bool bTimeStretch = IsDlgButtonChecked(IDC_CHECK3) != BST_UNCHECKED;
	if(!bTimeStretch) ReadTimeStretchParameters();
	((CComboBox *)GetDlgItem(IDC_COMBO4))->EnableWindow(bTimeStretch ? FALSE : TRUE);
	((CEdit *)GetDlgItem(IDC_EDIT6))->EnableWindow(bTimeStretch ? TRUE : FALSE);
	((CButton *)GetDlgItem(IDC_BUTTON2))->EnableWindow(bTimeStretch ? TRUE : FALSE); //rewbs.timeStretchMods
	GetDlgItem(IDC_TEXT_QUALITY)->ShowWindow(bTimeStretch ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_COMBO5)->ShowWindow(bTimeStretch ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_TEXT_FFT)->ShowWindow(bTimeStretch ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_COMBO6)->ShowWindow(bTimeStretch ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_TEXT_STRETCHPARAMS)->ShowWindow(bTimeStretch ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_EDIT_STRETCHPARAMS)->ShowWindow(bTimeStretch ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_TEXT_PITCH)->ShowWindow(bTimeStretch ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_COMBO4)->ShowWindow(bTimeStretch ? SW_HIDE : SW_SHOW);
	SetDlgItemText(IDC_BUTTON1, bTimeStretch ? "Time Stretch" : "Pitch Shift");
	if(bTimeStretch) UpdateTimeStretchParameterString();
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


void CCtrlSamples::OnPitchShiftTimeStretch()
//------------------------------------------
{
	// Error management
	int errorcode = -1;

	if((m_sndFile.GetSample(m_nSample).pSample == nullptr)) goto error;

	ModSample &sample = m_sndFile.GetSample(m_nSample);

	// Time stretching
	if(IsDlgButtonChecked(IDC_CHECK3))
	{
		UpdateData(TRUE); //Ensure m_dTimeStretchRatio is up-to-date with textbox content
		errorcode = TimeStretch((float)(m_dTimeStretchRatio / 100.0));

		//Update loop points only if no error occured.
		if(errorcode == 0)
		{
			// Update loop wrap-around buffer
			sample.SetLoop(
				static_cast<SmpLength>(sample.nLoopStart * (m_dTimeStretchRatio / 100.0)),
				static_cast<SmpLength>(sample.nLoopEnd * (m_dTimeStretchRatio / 100.0)),
				sample.uFlags[CHN_LOOP],
				sample.uFlags[CHN_PINGPONGLOOP],
				m_sndFile);
			sample.SetSustainLoop(
				static_cast<SmpLength>(sample.nSustainStart * (m_dTimeStretchRatio / 100.0)),
				static_cast<SmpLength>(sample.nSustainEnd * (m_dTimeStretchRatio / 100.0)),
				sample.uFlags[CHN_SUSTAINLOOP],
				sample.uFlags[CHN_PINGPONGSUSTAIN],
				m_sndFile);
		}
		
	}
	// Pitch shifting
	else
	{
		// Get selected pitch modifier [-12,+12]
		CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO4);
		float pm = float(combo->GetCurSel()) - 12.0f;
		if(pm == 0.0f) goto error;

		// Compute pitch ratio in range [0.5f ; 2.0f] (1.0f means output == input)
		// * pitch up -> 1.0f + n / 12.0f -> (12.0f + n) / 12.0f , considering n : pitch modifier > 0
		// * pitch dn -> 1.0f - n / 24.0f -> (24.0f - n) / 24.0f , considering n : pitch modifier > 0
		float pitch = pm < 0 ? ((24.0f + pm) / 24.0f) : ((12.0f + pm) / 12.0f);
		pitch = Round(pitch, 4);

		// Apply pitch modifier
		errorcode = PitchShift(pitch);
	}

	// Error management
	error:

	if(errorcode > 0)
	{
		TCHAR str[64];
		switch(errorcode & 0xff)
		{
			case 1 : wsprintf(str, _T("Pitch %s..."), (errorcode >> 8) == 1 ? _T("< 0.5") : _T("> 2.0"));
				break;
			case 2 : wsprintf(str, _T("Stretch ratio is too %s. Must be between 50% and 200%."), (errorcode >> 8) == 1 ? _T("low") : _T("high"));
				break;
			case 3 : _tcscpy(str, _T("Not enough memory..."));
				break;
			case 6 : _tcscpy(str, _T("Sample too short"));
				break;
			default: _tcscpy(str, _T("Unknown Error..."));
				break;
		}
		Reporting::Error(str);
		return;
	}

	// Update sample view
	SetModified(HINT_SAMPLEDATA | HINT_SAMPLEINFO, true);
}


int CCtrlSamples::TimeStretch(float ratio)
//----------------------------------------
{
	if((m_sndFile.GetSample(m_nSample).pSample == nullptr)) return -1;

	ModSample &sample = m_sndFile.GetSample(m_nSample);

	const uint32 nSampleRate = sample.GetSampleRate(m_sndFile.GetType());

	// Refuse processing when ratio is negative, equal to zero or equal to 1.0
	if(ratio <= 0.0 || ratio == 1.0) return -1;

	// Convert to pitch factor
	float pitch = Round(ratio, 4);
	if(pitch < 0.5f) return 2 + (1<<8);
	if(pitch > 2.0f) return 2 + (2<<8);

	// Check whether the DLL file exists.
	mpt::Library libSoundTouch = mpt::Library(mpt::LibraryPath::AppFullName(MPT_PATHSTRING("OpenMPT_SoundTouch_f32")));
	if(!libSoundTouch.IsValid())
	{
		MsgBox(IDS_SOUNDTOUCH_LOADFAILURE);
		return -1;
	}
	HANDLE handleSt = soundtouch_createInstance();
	if(handleSt == NULL)
	{
		MsgBox(IDS_SOUNDTOUCH_LOADFAILURE);
		return -1;
	}

	// Get number of channels & sample size
	uint8 smpsize = sample.GetElementarySampleSize();
	const uint8 nChn = sample.GetNumChannels();

	// SoundTouch(v1.4.0) documentation says that sample rates 8000-48000 are supported.
	// Check whether sample rate is within that range, and if not,
	// ask user whether to proceed.
	if(nSampleRate < 8000 || nSampleRate > 48000)
	{
		CString str;
		str.Format(TEXT(GetStrI18N("Current samplerate, %u Hz, is not in the supported samplerate range 8000 Hz - 48000 Hz. Continue?")), nSampleRate);
		if(Reporting::Confirm(str) != cnfYes)
		{
			soundtouch_destroyInstance(handleSt);
			return -1;
		}
	}

	// Initialize soundtouch object.
	{	
		soundtouch_setSampleRate(handleSt, nSampleRate);
		soundtouch_setChannels(handleSt, nChn);

		// Given ratio is time stretch ratio, and must be converted to
		// tempo change ratio: for example time stretch ratio 2 means
		// tempo change ratio 0.5.
		soundtouch_setTempoChange(handleSt, (1.0f / ratio - 1.0f) * 100.0f);

		// Read settings from GUI.
		ReadTimeStretchParameters();

		// Set settings to soundtouch. Zero value means 'use default', and
		// setting value is read back after setting because not all settings are accepted.
		if(m_nSequenceMs != 0)
			soundtouch_setSetting(handleSt, SETTING_SEQUENCE_MS, m_nSequenceMs);
		m_nSequenceMs = soundtouch_getSetting(handleSt, SETTING_SEQUENCE_MS);

		if(m_nSeekWindowMs != 0)
			soundtouch_setSetting(handleSt, SETTING_SEEKWINDOW_MS, m_nSeekWindowMs);
		m_nSeekWindowMs = soundtouch_getSetting(handleSt, SETTING_SEEKWINDOW_MS);

		if(m_nOverlapMs != 0)
			soundtouch_setSetting(handleSt, SETTING_OVERLAP_MS, m_nOverlapMs);
		m_nOverlapMs = soundtouch_getSetting(handleSt, SETTING_OVERLAP_MS);

		// Update GUI with the actual SoundTouch parameters in effect.
		UpdateTimeStretchParameterString();
	}

	const SmpLength inBatchSize = soundtouch_getSetting(handleSt, SETTING_NOMINAL_INPUT_SEQUENCE) + 1; // approximate value, add 1 to play safe
	const SmpLength outBatchSize = soundtouch_getSetting(handleSt, SETTING_NOMINAL_OUTPUT_SEQUENCE) + 1; // approximate value, add 1 to play safe

	if(sample.nLength < inBatchSize)
	{
		soundtouch_destroyInstance(handleSt);
		return 6;
	}

	if((SmpLength)std::ceil((double)ratio * (double)sample.nLength) < outBatchSize)
	{
		soundtouch_destroyInstance(handleSt);
		return 6;
	}

	// Allocate new sample. Returned sample may not be exactly the size what ratio would suggest
	// so allocate a bit more (1.01).
	const SmpLength nNewSampleLength = static_cast<SmpLength>(1.01 * ratio * static_cast<double>(sample.nLength + inBatchSize)) + outBatchSize;
	void *pNewSample = nullptr;
	if(nNewSampleLength <= MAX_SAMPLE_LENGTH)
	{
		pNewSample = ModSample::AllocateSample(nNewSampleLength, nChn * smpsize);
	}
	if(pNewSample == nullptr)
	{
		soundtouch_destroyInstance(handleSt);
		return 3;
	}

	// Save process button text (to be used as "progress bar" indicator while processing)
	CHAR oldText[255];
	GetDlgItemText(IDC_BUTTON1, oldText, CountOf(oldText));

	// Get process button device context & client rect
	RECT progressBarRECT, processButtonRect;
	HDC processButtonDC = ::GetDC(((CButton*)GetDlgItem(IDC_BUTTON1))->m_hWnd);
	::GetClientRect(((CButton*)GetDlgItem(IDC_BUTTON1))->m_hWnd,&processButtonRect);
	processButtonRect.top += 1;
	processButtonRect.left += 1;
	processButtonRect.right -= 1;
	processButtonRect.bottom -= 2;
	progressBarRECT = processButtonRect;

	// Progress bar brushes
	HBRUSH green = CreateSolidBrush(RGB(96,192,96));
	HBRUSH red = CreateSolidBrush(RGB(192,96,96));

	// Show wait mouse cursor
	BeginWaitCursor();

	std::vector<float> buffer;
	std::vector<SC::Convert<float,int16> > convf32(nChn);
	std::vector<SC::Convert<int16,float> > convi16(nChn);
	std::vector<SC::Convert<float,int8> > conv8f32(nChn);
	std::vector<SC::Convert<int8,float> > convint8(nChn);

	static const SmpLength MaxInputChunkSize = 1024;

	SmpLength inPos = 0;
	SmpLength outPos = 0; // Keeps count of the sample length received from stretching process.

	uint32 updateInterval = TrackerSettings::Instance().GUIUpdateInterval;
	if(updateInterval == 0)
	{
		updateInterval = 15;
	}
	DWORD timeLast = 0;

	// Process sample in steps.
	while(inPos < sample.nLength)
	{

		// Current chunk size limit test
		const SmpLength inChunkSize = std::min(MaxInputChunkSize, sample.nLength - inPos);

		DWORD timeNow = timeGetTime();
		if(timeNow - timeLast >= updateInterval)
		{
			// Show progress bar using process button painting & text label
			CHAR progress[16];
			float percent = 100.0f * (inPos + inChunkSize) / sample.nLength;
			progressBarRECT.right = processButtonRect.left + (int)percent * (processButtonRect.right - processButtonRect.left) / 100;
			wsprintf(progress,"%d%%",(UINT)percent);

			::FillRect(processButtonDC,&processButtonRect,red);
			::FrameRect(processButtonDC,&processButtonRect,CMainFrame::brushBlack);
			::FillRect(processButtonDC,&progressBarRECT,green);
			::SelectObject(processButtonDC,(HBRUSH)HOLLOW_BRUSH);
			::SetBkMode(processButtonDC,TRANSPARENT);
			::DrawText(processButtonDC,progress,strlen(progress),&processButtonRect,DT_CENTER | DT_SINGLELINE | DT_VCENTER);
			::GdiFlush();

			timeLast = timeNow;
		}

		// Send sampledata for processing.
		buffer.resize(inChunkSize * nChn);
		switch(smpsize)
		{
		case 1:
			CopyInterleavedSampleStreams(&(buffer[0]), static_cast<int8 *>(sample.pSample) + inPos * nChn, inChunkSize, nChn, conv8f32);
			break;
		case 2:
			CopyInterleavedSampleStreams(&(buffer[0]), static_cast<int16 *>(sample.pSample) + inPos * nChn, inChunkSize, nChn, convf32);
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

	// The input sample should now be processed. Receive remaining samples.
	soundtouch_flush(handleSt);
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

	soundtouch_clear(handleSt);
	ASSERT(soundtouch_isEmpty(handleSt) != 0);

	ASSERT(nNewSampleLength >= outPos);

	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_replace, "Time Stretch");
	// Swap sample buffer pointer to new buffer, update song + sample data & free old sample buffer
	ctrlSmp::ReplaceSample(sample, pNewSample, std::min(outPos, nNewSampleLength), m_sndFile);

	// Free progress bar brushes
	DeleteObject((HBRUSH)green);
	DeleteObject((HBRUSH)red);

	// Restore process button text
	SetDlgItemText(IDC_BUTTON1, oldText);

	// Restore mouse cursor
	EndWaitCursor();

	soundtouch_destroyInstance(handleSt);

	return 0;
}


int CCtrlSamples::PitchShift(float pitch)
//---------------------------------------
{
	if((m_sndFile.GetSample(m_nSample).pSample == nullptr)) return -1;
	if(pitch < 0.5f) return 1 + (1<<8);
	if(pitch > 2.0f) return 1 + (2<<8);

	// Get sample struct pointer
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	// Get number of channels & sample size
	const uint8 smpsize = sample.GetElementarySampleSize();
	const uint8 nChn = sample.GetNumChannels();

	// Get selected oversampling - quality - (also refered as FFT overlapping) factor
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO5);
	long ovs = combo->GetCurSel() + 4;

	// Get selected FFT size (power of 2 ; should not exceed MAX_BUFFER_LENGTH - see smbPitchShift.h)
	combo = (CComboBox *)GetDlgItem(IDC_COMBO6);
	UINT fft = 1 << (combo->GetCurSel() + 8);
	while(fft > MAX_BUFFER_LENGTH) fft >>= 1;

	// Save process button text (to be used as "progress bar" indicator while processing)
	CHAR oldText[255];
	GetDlgItemText(IDC_BUTTON1, oldText, 255);

	// Get process button device context & client rect
	RECT progressBarRECT, processButtonRect;
	HDC processButtonDC = ::GetDC(((CButton*)GetDlgItem(IDC_BUTTON1))->m_hWnd);
	::GetClientRect(((CButton*)GetDlgItem(IDC_BUTTON1))->m_hWnd,&processButtonRect);
	processButtonRect.top += 1;
	processButtonRect.left += 1;
	processButtonRect.right -= 1;
	processButtonRect.bottom -= 2;
	progressBarRECT = processButtonRect;

	// Progress bar brushes
	HBRUSH green = CreateSolidBrush(RGB(96,192,96));
	HBRUSH red = CreateSolidBrush(RGB(192,96,96));

	// Show wait mouse cursor
	BeginWaitCursor();

	// Get original sample rate
	const float sampleRate = static_cast<float>(sample.GetSampleRate(m_sndFile.GetType()));

	// Allocate working buffer
	const size_t bufferSize = MAX_BUFFER_LENGTH + fft;
	float *buffer = new float[bufferSize];

	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_replace, "Pitch Shift");

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
			// meaningfull  processed samples) , in order  to avoid this behaviour , we will ignore
			// the  first FFT_length  samples and process  the same  amount of extra  blank samples
			// (all 0.0f) at the end of the buffer (those extra samples will benefit from  internal
			// FFT data  computed during the previous  steps resulting in a  correct and consistent
			// signal output).
			bool bufstart = ( pos == 0 );
			bool bufend   = ( pos + MAX_BUFFER_LENGTH >= sample.nLength );
			SmpLength startoffset = ( bufstart ? fft : 0 );
			SmpLength inneroffset = ( bufstart ? 0 : fft );
			SmpLength finaloffset = ( bufend   ? fft : 0 );

			// Show progress bar using process button painting & text label
			CHAR progress[16];
			float percent = (float)i * 50.0f + (100.0f / nChn) * (pos + len) / sample.nLength;
			progressBarRECT.right = processButtonRect.left + (int)percent * (processButtonRect.right - processButtonRect.left) / 100;
			wsprintf(progress,"%d%%",(UINT)percent);

			::FillRect(processButtonDC,&processButtonRect,red);
			::FrameRect(processButtonDC,&processButtonRect,CMainFrame::brushBlack);
			::FillRect(processButtonDC,&progressBarRECT,green);
			::SelectObject(processButtonDC,(HBRUSH)HOLLOW_BRUSH);
			::SetBkMode(processButtonDC,TRANSPARENT);
			::DrawText(processButtonDC,progress,strlen(progress),&processButtonRect,DT_CENTER | DT_SINGLELINE | DT_VCENTER);
			::GdiFlush();

			// Re-initialize pitch-shifter with blank FFT before processing 1st chunk of current channel
			if(bufstart)
			{
				for(UINT j = 0; j < fft; j++) buffer[j] = 0.0f;
				smbPitchShift(pitch, fft, fft, ovs, sampleRate, buffer, buffer);
			}

			// Convert current channel's data chunk to float
			int8 *ptr = (int8 *)sample.pSample + pos * smpsize * nChn + i * smpsize;

			switch(smpsize)
			{
			case 1:
				CopySample<SC::ConversionChain<SC::Convert<float, int8>, SC::DecodeIdentity<int8> > >(buffer, len, 1, ptr, sizeof(int8) * len, nChn);
				break;
			case 2:
				CopySample<SC::ConversionChain<SC::Convert<float, int16>, SC::DecodeIdentity<int16> > >(buffer, len, 1, (int16 *)ptr, sizeof(int16) * len, nChn);
				break;
			}

			// Fills extra blank samples (read TRICK description comment above)
			if(bufend) for(SmpLength j = len ; j < len + finaloffset ; j++) buffer[j] = 0.0f;

			// Apply pitch shifting
			smbPitchShift(pitch, static_cast<long>(len + finaloffset), fft, ovs, sampleRate, buffer, buffer);

			// Restore pitched-shifted float sample into original sample buffer
			ptr = (int8 *)sample.pSample + (pos - inneroffset) * smpsize * nChn + i * smpsize;
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
	delete [] buffer;

	// Free progress bar brushes
	DeleteObject((HBRUSH)green);
	DeleteObject((HBRUSH)red);

	// Restore process button text
	SetDlgItemText(IDC_BUTTON1, oldText);

	// Restore mouse cursor
	EndWaitCursor();

	return 0;
}


void CCtrlSamples::OnReverse()
//----------------------------
{
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	SampleSelectionPoints selection = GetSelectionPoints();

	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_reverse, "Reverse", selection.nStart, selection.nEnd);
	if(ctrlSmp::ReverseSample(sample, selection.nStart, selection.nEnd, m_sndFile))
	{
		SetModified(HINT_SAMPLEDATA, false);
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
		SetModified(HINT_SAMPLEDATA, false);
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

	if(m_modDoc.IsNotePlaying(0, m_nSample, 0) == TRUE)
		MsgBoxHidable(ConfirmSignUnsignWhenPlaying);

	BeginWaitCursor();
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SampleSelectionPoints selection = GetSelectionPoints();

	m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_unsign, "Unsign", selection.nStart, selection.nEnd);
	if(ctrlSmp::UnsignSample(sample, selection.nStart, selection.nEnd, m_sndFile) == true)
	{
		SetModified(HINT_SAMPLEDATA, false);
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
	if(selection.selectionActive == true)
	{
		ModSample &sample = m_sndFile.GetSample(m_nSample);
		m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_update, "Silence", selection.nStart, selection.nEnd);

		SmpLength len = selection.nEnd - selection.nStart;
		if (sample.uFlags[CHN_STEREO])
		{
			int smplsize = sample.GetBytesPerSample();
			signed char *p = ((signed char *)sample.pSample) + selection.nStart * smplsize;
			memset(p, 0, len * smplsize);
		} else
			if (sample.uFlags[CHN_16BIT])
			{
				short int *p = ((short int *)sample.pSample) + selection.nStart;
				int dest = (selection.nEnd < sample.nLength) ? p[len-1] : 0;
				int base = (selection.nStart) ? p[0] : 0;
				int delta = dest - base;
				for (SmpLength i=0; i<len; i++)
				{
					int n = base + (int)(((LONGLONG)delta * (LONGLONG)i) / (len-1));
					p[i] = (signed short)n;
				}
			} else
			{
				signed char *p = ((signed char *)sample.pSample) + selection.nStart;
				int dest = (selection.nEnd < sample.nLength) ? p[len-1] : 0;
				int base = (selection.nStart) ? p[0] : 0;
				int delta = dest - base;
				for (SmpLength i=0; i<len; i++)
				{
					int n = base + (delta * i) / (len-1);
					p[i] = (signed char)n;
				}
			}
			sample.PrecomputeLoops(m_sndFile, false);
			SetModified(HINT_SAMPLEDATA, false);
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
		SetModified(HINT_SMPNAMES | HINT_SAMPLEINFO, false);
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
		SetModified(HINT_SAMPLEINFO, false);
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
		SetModified(HINT_SAMPLEINFO, false);
	}
}


void CCtrlSamples::OnGlobalVolChanged()
//-------------------------------------
{
	if (IsLocked()) return;
	int nVol = GetDlgItemInt(IDC_EDIT8);
	Limit(nVol, 0, 64);
	if (nVol != m_sndFile.GetSample(m_nSample).nGlobalVol)
	{
		m_sndFile.GetSample(m_nSample).nGlobalVol = (uint16)nVol;
		SetModified(HINT_SAMPLEINFO, false);
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
		SetModified(HINT_SAMPLEINFO, false);
	}
}


void CCtrlSamples::OnPanningChanged()
//-----------------------------------
{
	if (IsLocked()) return;
	int nPan = GetDlgItemInt(IDC_EDIT9);
	if (nPan < 0) nPan = 0;
	//rewbs.fix36944: sample pan range to 0-64.
	if (m_sndFile.GetType() == MOD_TYPE_XM)
	{
		if (nPan > 255) nPan = 255;	// displayed panning will be 0-255 with XM
	} else
	{
		if (nPan > 64) nPan = 64;	// displayed panning will be 0-64 with anything but XM.
		nPan = nPan << 2;			// so we x4 to get MPT's internal 0-256 range.
	}
	//end rewbs.fix36944
	if (nPan != m_sndFile.GetSample(m_nSample).nPan)
	{
		m_sndFile.GetSample(m_nSample).nPan = (uint16)nPan;
		SetModified(HINT_SAMPLEINFO, false);
	}
}


void CCtrlSamples::OnFineTuneChanged()
//------------------------------------
{
	if (IsLocked()) return;
	int n = GetDlgItemInt(IDC_EDIT5);
	if (m_sndFile.m_nType & (MOD_TYPE_IT|MOD_TYPE_S3M|MOD_TYPE_MPT))
	{
		if ((n > 0) && (n <= (m_sndFile.GetType() == MOD_TYPE_S3M ? 65535 : 9999999)) && (n != (int)m_sndFile.GetSample(m_nSample).nC5Speed))
		{
			m_sndFile.GetSample(m_nSample).nC5Speed = n;
			int transp = ModSample::FrequencyToTranspose(n) >> 7;
			int basenote = 60 - transp;
			if (basenote < BASENOTE_MIN) basenote = BASENOTE_MIN;
			if (basenote >= BASENOTE_MAX) basenote = BASENOTE_MAX-1;
			basenote -= BASENOTE_MIN;
			if (basenote != m_CbnBaseNote.GetCurSel())
			{
				LockControls();
				m_CbnBaseNote.SetCurSel(basenote);
				UnlockControls();
			}
			SetModified(HINT_SAMPLEINFO, false);
		}
	} else
	{
		if(m_sndFile.GetType() & MOD_TYPE_MOD)
			n = MOD2XMFineTune(n);
		if ((n >= -128) && (n <= 127))
		{
			m_sndFile.GetSample(m_nSample).nFineTune = (signed char)n;
			SetModified(HINT_SAMPLEINFO, false);
		}

	}
}


void CCtrlSamples::OnBaseNoteChanged()
//-------------------------------------
{
	if (IsLocked()) return;
	int n = (NOTE_MIDDLEC - NOTE_MIN) - (m_CbnBaseNote.GetCurSel() + BASENOTE_MIN);

	ModSample &sample = m_sndFile.GetSample(m_nSample);

	if (m_sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_S3M|MOD_TYPE_MPT))
	{
		const int oldTransp = ModSample::FrequencyToTranspose(sample.nC5Speed) >> 7;
		const uint32 newTrans = Util::Round<uint32>(sample.nC5Speed * pow(2.0, (n - oldTransp) / 12.0));
		if (newTrans > 0 && newTrans <= (m_sndFile.GetType() == MOD_TYPE_S3M ? 65535u : 9999999u) && newTrans != sample.nC5Speed)
		{
			CHAR s[32];
			sample.nC5Speed = newTrans;
			wsprintf(s, "%lu", newTrans);
			LockControls();
			m_EditFineTune.SetWindowText(s);
			UnlockControls();
			SetModified(HINT_SAMPLEINFO, false);
		}
	} else
	{
		if ((n >= -128) && (n < 128))
		{
			sample.RelativeTone = (int8)n;
			SetModified(HINT_SAMPLEINFO, false);
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
		m_sndFile.GetSample(m_nSample).nVibType = static_cast<uint8>(n);

		PropagateAutoVibratoChanges();
		SetModified(HINT_SAMPLEINFO, false);
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
		SetModified(HINT_SAMPLEINFO, false);
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
		SetModified(HINT_SAMPLEINFO, false);
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
		SetModified(HINT_SAMPLEINFO, false);
	}
}


void CCtrlSamples::OnLoopTypeChanged()
//------------------------------------
{
	if(IsLocked()) return;
	int n = m_ComboLoopType.GetCurSel();
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
		m_modDoc.UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA);
	} else
	{
		sample.PrecomputeLoops(m_sndFile);
	}
	ctrlSmp::UpdateLoopPoints(sample, m_sndFile);
	SetModified(HINT_SAMPLEINFO, false);
}


void CCtrlSamples::OnLoopStartChanged()
//--------------------------------------
{
	if(IsLocked()) return;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SmpLength n = GetDlgItemInt(IDC_EDIT1);
	if ((n >= 0) && (n < sample.nLength) && ((n < sample.nLoopEnd) || !sample.uFlags[CHN_LOOP]))
	{
		sample.SetLoop(n, sample.nLoopEnd, sample.uFlags[CHN_LOOP], sample.uFlags[CHN_PINGPONGLOOP], m_sndFile);
		SetModified(HINT_SAMPLEINFO | HINT_SAMPLEDATA, false);
	}
}


void CCtrlSamples::OnLoopEndChanged()
//------------------------------------
{
	if(IsLocked()) return;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SmpLength n = GetDlgItemInt(IDC_EDIT2);
	if ((n >= 0) && (n <= sample.nLength) && ((n > sample.nLoopStart) || !sample.uFlags[CHN_LOOP]))
	{
		sample.SetLoop(sample.nLoopStart, n, sample.uFlags[CHN_LOOP], sample.uFlags[CHN_PINGPONGLOOP], m_sndFile);
		SetModified(HINT_SAMPLEINFO | HINT_SAMPLEDATA, false);
	}
}


void CCtrlSamples::OnSustainTypeChanged()
//---------------------------------------
{
	if(IsLocked()) return;
	int n = m_ComboSustainType.GetCurSel();
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
		m_modDoc.UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA);
	} else
	{
		sample.PrecomputeLoops(m_sndFile);
	}
	ctrlSmp::UpdateLoopPoints(sample, m_sndFile);
	SetModified(HINT_SAMPLEINFO, false);
}


void CCtrlSamples::OnSustainStartChanged()
//----------------------------------------
{
	if(IsLocked()) return;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SmpLength n = GetDlgItemInt(IDC_EDIT3);
	if ((n >= 0) && (n <= sample.nLength)
	 && ((n < sample.nSustainEnd) || !sample.uFlags[CHN_SUSTAINLOOP]))
	{
		sample.SetSustainLoop(n, sample.nSustainEnd, sample.uFlags[CHN_SUSTAINLOOP], sample.uFlags[CHN_PINGPONGSUSTAIN], m_sndFile);
		SetModified(HINT_SAMPLEINFO | HINT_SAMPLEDATA, false);
	}
}


void CCtrlSamples::OnSustainEndChanged()
//--------------------------------------
{
	if(IsLocked()) return;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	SmpLength n = GetDlgItemInt(IDC_EDIT4);
	if ((n >= 0) && (n <= sample.nLength)
	 && ((n > sample.nSustainStart) || !sample.uFlags[CHN_SUSTAINLOOP]))
	{
		sample.SetSustainLoop(sample.nSustainStart, n, sample.uFlags[CHN_SUSTAINLOOP], sample.uFlags[CHN_PINGPONGSUSTAIN], m_sndFile);
		SetModified(HINT_SAMPLEINFO | HINT_SAMPLEDATA, false);
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
	UINT pinc = 1;
	ModSample &sample = m_sndFile.GetSample(m_nSample);
	const uint8 *pSample = static_cast<const uint8 *>(sample.pSample);
	short int pos;
	bool redraw = false;
	
	LockControls();
	if ((!sample.nLength) || (!pSample)) goto NoSample;
	if (sample.uFlags[CHN_16BIT])
	{
		pSample++;
		pinc *= 2;
	}
	if (sample.uFlags[CHN_STEREO]) pinc *= 2;
	// Loop Start
	if ((pos = (short int)m_SpinLoopStart.GetPos()) != 0)
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
	pos = (short int)m_SpinLoopEnd.GetPos();
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
	pos = (short int)m_SpinSustainStart.GetPos();
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
	pos = (short int)m_SpinSustainEnd.GetPos();
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
	if ((pos = (short int)m_SpinFineTune.GetPos()) != 0)
	{
		if (m_sndFile.m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			uint32 d = sample.nC5Speed;
			if (d < 1) d = 8363;
			if(d < TrackerSettings::Instance().m_nFinetuneStep) d = TrackerSettings::Instance().m_nFinetuneStep;
			d += (pos * TrackerSettings::Instance().m_nFinetuneStep);
			sample.nC5Speed = Clamp(d, 1u, 9999999u); // 9999999 is max. in Impulse Tracker
			int transp = ModSample::FrequencyToTranspose(sample.nC5Speed) >> 7;
			int basenote = 60 - transp;
			if (basenote < BASENOTE_MIN) basenote = BASENOTE_MIN;
			if (basenote >= BASENOTE_MAX) basenote = BASENOTE_MAX-1;
			basenote -= BASENOTE_MIN;
			if (basenote != m_CbnBaseNote.GetCurSel()) m_CbnBaseNote.SetCurSel(basenote);
			wsprintf(s, "%lu", sample.nC5Speed);
			m_EditFineTune.SetWindowText(s);
		} else
		{
			int ftune = (int)sample.nFineTune;
			// MOD finetune range -8 to 7 translates to -128 to 112
			if(m_sndFile.GetType() & MOD_TYPE_MOD)
			{
				ftune = Clamp((ftune >> 4) + pos, -8, 7);
				sample.nFineTune = MOD2XMFineTune((signed char)ftune);
			}
			else
			{
				ftune = Clamp(ftune + pos, -128, 127);
				sample.nFineTune = (signed char)ftune;
			}
			SetDlgItemInt(IDC_EDIT5, ftune, TRUE);
		}
		redraw = true;
		m_SpinFineTune.SetPos(0);
	}
	if(nCode == SB_ENDSCROLL) SwitchToView();
	if(redraw)
	{
		SetModified(HINT_SAMPLEINFO | HINT_SAMPLEDATA, false);
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
			CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();
			
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
	case kcSampleNew:		OnSampleNew(); return wParam;

	case kcSampleTransposeUp: transpose = 1; break;
	case kcSampleTransposeDown: transpose = -1; break;
	case kcSampleTransposeOctUp: transpose = 12; break;
	case kcSampleTransposeOctDown: transpose = -12; break;
	}

	if(transpose)
	{
		if(m_CbnBaseNote.IsWindowEnabled())
		{
			int sel = Clamp(m_CbnBaseNote.GetCurSel() - transpose, 0, m_CbnBaseNote.GetCount() - 1);
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
#define DEFAULT_XFADE_LENGTH 16384 //4096

SmpLength LimitXFadeLength(SmpLength len, const ModSample &sample)
{
	return Util::Min(len, sample.nLoopEnd - sample.nLoopStart, sample.nLoopEnd / 2);
}

void CCtrlSamples::OnXFade()
//--------------------------
{
	static SmpLength fadeLength = DEFAULT_XFADE_LENGTH;
	ModSample &sample = m_sndFile.GetSample(m_nSample);

	if(sample.pSample == nullptr) return;
	if(sample.nLoopEnd <= sample.nLoopStart || sample.nLoopEnd > sample.nLength) return;
	// Case 1: The loop start is preceeded by > XFADE_LENGTH samples. Nothing has to be adjusted.
	// Case 2: The actual loop is shorter than XFADE_LENGTH samples.
	// Case 3: There is not enough sample material before the loop start. Move the loop start.
	fadeLength = LimitXFadeLength(fadeLength, sample);

	CSampleXFadeDlg dlg(this, fadeLength, LimitXFadeLength(sample.nLength, sample));
	if(dlg.DoModal() == IDOK)
	{
		fadeLength = dlg.m_nSamples;

		if(fadeLength < 4) return;

		m_modDoc.GetSampleUndo().PrepareUndo(m_nSample, sundo_update, "Crossfade", sample.nLoopEnd - fadeLength, sample.nLoopEnd);
		// If we want to fade nFadeLength bytes, we need as much sample material before the loop point. nFadeLength has been adjusted above.
		if(sample.nLoopStart < fadeLength)
		{
			sample.nLoopStart = fadeLength;
		}

		if(ctrlSmp::XFadeSample(sample, fadeLength, m_sndFile))
		{
			SetModified(HINT_SAMPLEINFO | HINT_SAMPLEDATA, true);
		} else
		{
			m_modDoc.GetSampleUndo().RemoveLastUndoStep(m_nSample);
		}

	}
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
			SetModified(HINT_SAMPLEINFO, true);
			EndWaitCursor();
		}
	}
}


// When changing auto vibrato properties, propagate them to other samples of the same instrument in XM edit mode.
void CCtrlSamples::PropagateAutoVibratoChanges() const
//----------------------------------------------------
{
	if(!(m_sndFile.GetType() & MOD_TYPE_XM))
	{
		return;
	}

	for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
	{
		if(m_sndFile.IsSampleReferencedByInstrument(m_nSample, i))
		{
			const std::set<SAMPLEINDEX> referencedSamples = m_sndFile.Instruments[i]->GetSamples();

			// Propagate changes to all samples that belong to this instrument.
			for(std::set<SAMPLEINDEX>::const_iterator sample = referencedSamples.begin(); sample != referencedSamples.end(); sample++)
			{
				if(*sample <= m_sndFile.GetNumSamples())
				{
					m_sndFile.GetSample(*sample).nVibDepth = m_sndFile.GetSample(m_nSample).nVibDepth;
					m_sndFile.GetSample(*sample).nVibType = m_sndFile.GetSample(m_nSample).nVibType;
					m_sndFile.GetSample(*sample).nVibRate = m_sndFile.GetSample(m_nSample).nVibRate;
					m_sndFile.GetSample(*sample).nVibSweep = m_sndFile.GetSample(m_nSample).nVibSweep;
					m_modDoc.UpdateAllViews(nullptr, HINT_SAMPLEINFO | ((*sample) << HINT_SHIFT_SMP));
				}
			}
		}
	}
}


OPENMPT_NAMESPACE_END
