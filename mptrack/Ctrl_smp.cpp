#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "ctrl_smp.h"
#include "view_smp.h"
#include "dlg_misc.h"
#include "PSRatioCalc.h" //rewbs.timeStretchMods
#include "mpdlgs.h"
#include "soundtouch/SoundTouch.h"
#include "soundtouch/TDStretch.h"
#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"
#include "smbPitchShift.cpp"
#pragma warning(default:4244) //"conversion from 'type1' to 'type2', possible loss of data"
#include "modsmp_ctrl.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

#ifdef _DEBUG
#include <math.h>
#endif

#define FLOAT_ERROR 1.0e-20f

// Float point comparison
bool EqualTof(const float a, const float b)
{
    return (fabs(a - b) <= FLOAT_ERROR);
}

// Round floating point value to "digit" number of digits
float Round(const float value, const int digit)
{
	float v = 0.1f * (value * powf(10.0f, (float)(digit + 1)) + (value < 0.0f ? -5.0f : 5.0f));
    modff(v, &v);    
    return v / powf(10.0f, (float)digit);
}

// Deduce exponent from equation : v = 2^exponent
int PowerOf2Exponent(const unsigned int v)
{
	float v2f = (float)v;
	return ( (*(int *)&v2f >> 23) & 0xff ) - 127;
}
// -! TEST#0029

#define	BASENOTE_MIN	(1*12)	// C-1
#define	BASENOTE_MAX	(9*12)	// C-9

#pragma warning(disable:4244)

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
	ON_COMMAND(IDC_SAMPLE_SIGN_UNSIGN,  OnSignUnSign)
	ON_COMMAND(IDC_SAMPLE_DCOFFSET,		OnRemoveDCOffset)
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


CCtrlSamples::CCtrlSamples() : 
//----------------------------
	m_nStretchProcessStepLength(nDefaultStretchChunkSize),
	m_nSequenceMs(DEFAULT_SEQUENCE_MS),
	m_nSeekWindowMs(DEFAULT_SEEKWINDOW_MS),
	m_nOverlapMs(DEFAULT_OVERLAP_MS)
{
	m_nSample = 1;
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
	if (!m_pSndFile) return TRUE;
	m_bInitialized = FALSE;
	// Zoom Selection
	m_ComboZoom.AddString("Auto");
	m_ComboZoom.AddString("1:1");
	m_ComboZoom.AddString("1:2");
	m_ComboZoom.AddString("1:4");
	m_ComboZoom.AddString("1:8");
	m_ComboZoom.AddString("1:16");
	m_ComboZoom.AddString("1:32");
	m_ComboZoom.AddString("1:64");
	m_ComboZoom.AddString("1:128");
	m_ComboZoom.SetCurSel(0);
	// File ToolBar
	m_ToolBar1.Init();
	m_ToolBar1.AddButton(IDC_SAMPLE_NEW, TIMAGE_SAMPLE_NEW);
	m_ToolBar1.AddButton(IDC_SAMPLE_OPEN, TIMAGE_OPEN);
	m_ToolBar1.AddButton(IDC_SAMPLE_SAVEAS, TIMAGE_SAVE);
	// Edit ToolBar
	m_ToolBar2.Init();
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
	// Setup Controls
	m_EditName.SetLimitText(32);
	m_EditFileName.SetLimitText(22);
	m_SpinVolume.SetRange(0, 64);
	m_SpinGlobalVol.SetRange(0, 64);
	//rewbs.fix36944
	if (m_pSndFile->m_nType == MOD_TYPE_XM) {
		m_SpinPanning.SetRange(0, 255);
	} else 	{
		m_SpinPanning.SetRange(0, 64);
	}
	//end rewbs.fix36944

	// Auto vibrato
	m_ComboAutoVib.AddString("Sine");
	m_ComboAutoVib.AddString("Square");
	m_ComboAutoVib.AddString("Ramp Up");
	m_ComboAutoVib.AddString("Ramp Down");
	m_ComboAutoVib.AddString("Random");
	m_SpinVibSweep.SetRange(0, 255);
	if(m_pSndFile->m_nType & MOD_TYPE_XM)
	{
		m_SpinVibDepth.SetRange(0, 15);
		m_SpinVibRate.SetRange(0, 63);
	} else
	{
		m_SpinVibDepth.SetRange(0, 32);
		m_SpinVibRate.SetRange(0, 64);
	}

	for (UINT i=BASENOTE_MIN; i<BASENOTE_MAX; i++)
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
	if(combo){
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
	if(combo){
		// Allow quality from 4 to 128
		for(int i = 4 ; i <= 128 ; i++){
			wsprintf(str,"%d",i);
			combo->SetItemData(combo->AddString(str), i-4);
		}
		// Set 4 as default quality
		combo->SetCurSel(0);
	}

	// FFT size selection
	combo = (CComboBox *)GetDlgItem(IDC_COMBO6);
	if(combo){
		// Deduce exponent from equation : MAX_FRAME_LENGTH = 2^exponent
		int exponent = PowerOf2Exponent(MAX_FRAME_LENGTH);
		// Allow FFT size from 2^8 (256) to 2^exponent (MAX_FRAME_LENGTH)
		for(int i = 8 ; i <= exponent ; i++){
			wsprintf(str,"%d",1<<i);
			combo->SetItemData(combo->AddString(str), i-8);
		}
		// Set 2048 as default FFT size
		combo->SetCurSel(3);
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


BOOL CCtrlSamples::SetCurrentSample(UINT nSmp, LONG lZoom, BOOL bUpdNum)
//----------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	if (!pModDoc) return FALSE;
	pSndFile = pModDoc->GetSoundFile();
	if (pSndFile->m_nSamples < 1) pSndFile->m_nSamples = 1;
	if ((nSmp < 1) || (nSmp > pSndFile->m_nSamples)) return FALSE;

	LockControls();
	if (m_nSample != nSmp)
	{
		m_nSample = nSmp;
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		UpdateView((m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO, NULL);
	}
	if (bUpdNum)
	{
		SetDlgItemInt(IDC_EDIT_SAMPLE, m_nSample);
		m_SpinSample.SetRange(1, pSndFile->m_nSamples);
	}
	if (lZoom < 0)
		lZoom = m_ComboZoom.GetCurSel();
	else
		m_ComboZoom.SetCurSel(lZoom);
	PostViewMessage(VIEWMSG_SETCURRENTSAMPLE, (lZoom << 16) | m_nSample);
	UnlockControls();
	return TRUE;
}


void CCtrlSamples::OnActivatePage(LPARAM lParam)
//----------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pModDoc) && (m_pParent))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		if (lParam < 0)
		{
			int nIns = m_pParent->GetInstrumentChange();
			if (pSndFile->m_nInstruments)
			{
				if ((nIns > 0) && (!pModDoc->IsChildSample(m_nSample, nIns)))
				{
					UINT k = pModDoc->FindInstrumentChild(nIns);
					if (k > 0) lParam = k;
				}
			} else
			{
				if (nIns > 0) lParam = nIns;
				m_pParent->InstrumentChanged(-1);
			}
		} else
		if (lParam > 0)
		{
			if (pSndFile->m_nInstruments)
			{
				UINT k = m_pParent->GetInstrumentChange();
				if (!pModDoc->IsChildSample(k, lParam))
				{
					UINT nins = pModDoc->FindSampleParent(lParam);
					if (nins) m_pParent->InstrumentChanged(nins);
				}
			} else
			{
				m_pParent->InstrumentChanged(lParam);
			}
		}
	}
	SetCurrentSample((lParam > 0) ? lParam : m_nSample);

	m_nFinetuneStep = CMainFrame::GetPrivateProfileLong("Sample Editor", "FinetuneStep", 25, theApp.GetConfigFileName());

	// Initial Update
	// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
	if (!m_bInitialized) UpdateView((m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_MODTYPE, NULL);
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if ((pFrame) && (m_hWndView)) PostViewMessage(VIEWMSG_LOADSTATE, (LPARAM)pFrame->GetSampleViewState());
	SwitchToView();
}


void CCtrlSamples::OnDeactivatePage()
//-----------------------------------
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if ((pFrame) && (m_hWndView)) SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)pFrame->GetSampleViewState());
	if (m_pModDoc) m_pModDoc->NoteOff(0, TRUE);
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
		if (lParam) return OpenSample((LPCSTR)lParam);
		break;

	case CTRLMSG_SMP_SONGDROP:
		if (lParam)
		{
			LPDRAGONDROP pDropInfo = (LPDRAGONDROP)lParam;
			CSoundFile *pSndFile = (CSoundFile *)(pDropInfo->lDropParam);
			if (pDropInfo->pModDoc) pSndFile = pDropInfo->pModDoc->GetSoundFile();
			if (pSndFile) return OpenSample(pSndFile, pDropInfo->dwDropItem);
		}
		break;

	case CTRLMSG_SMP_SETZOOM:
		SetCurrentSample(m_nSample, lParam, FALSE);
		break;

	case CTRLMSG_SETCURRENTINSTRUMENT:
		SetCurrentSample(lParam, -1, TRUE);
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
			if ((m_pSndFile) && (m_pSndFile->m_nType & MOD_TYPE_XM) && (m_nSample))
			{
				MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
				UINT nFreqHz = CSoundFile::TransposeToFrequency(pSmp->RelativeTone, pSmp->nFineTune);
				wsprintf(pszText, "%ldHz", nFreqHz);
				return TRUE;
			}
			break;
		case IDC_EDIT_STRETCHPARAMS:
			wsprintf(pszText, "SequenceMs SeekwindowMs OverlapMs ProcessStepLength");
			return TRUE;
		}
	}
	return FALSE;
}


void CCtrlSamples::UpdateView(DWORD dwHintMask, CObject *pObj)
//------------------------------------------------------------
{
	if ((pObj == this) || (!m_pModDoc) || (!m_pSndFile)) return;
	if (dwHintMask & HINT_MPTOPTIONS)
	{
		m_ToolBar1.UpdateStyle();
		m_ToolBar2.UpdateStyle();
	}
	if (!(dwHintMask & (HINT_SAMPLEINFO|HINT_MODTYPE))) return;
	// 05/01/05 : ericus replaced ">> 24" by ">> 20" : 4000 samples -> 12bits [see Moddoc.h]
	//if (((dwHintMask >> 20) != (m_nSample&0x0fff)) && (!(dwHintMask & HINT_MODTYPE))) return;
	if (((dwHintMask >> HINT_SHIFT_SMP) != m_nSample) && (!(dwHintMask & HINT_MODTYPE))) return;
	LockControls();
	if (!m_bInitialized) dwHintMask |= HINT_MODTYPE;
	// Updating Ranges
	if (dwHintMask & HINT_MODTYPE)
	{
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
		if (m_pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT))
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
		b = (m_pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
		m_ComboSustainType.EnableWindow(b);
		m_SpinSustainStart.EnableWindow(b);
		m_SpinSustainEnd.EnableWindow(b);
		m_EditSustainStart.EnableWindow(b);
		m_EditSustainEnd.EnableWindow(b);
		// Finetune / C-4 Speed / BaseNote
		b = (m_pSndFile->m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
		SetDlgItemText(IDC_TEXT7, (b) ? "Freq. (Hz)" : "Finetune");
		m_SpinFineTune.SetRange(-1, 1);
		m_EditFileName.EnableWindow(b);
		// AutoVibrato
		b = (m_pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
		m_ComboAutoVib.EnableWindow(b);
		m_SpinVibSweep.EnableWindow(b);
		m_SpinVibDepth.EnableWindow(b);
		m_SpinVibRate.EnableWindow(b);
		m_EditVibSweep.EnableWindow(b);
		m_EditVibDepth.EnableWindow(b);
		m_EditVibRate.EnableWindow(b);
		// Global Volume
		b = (m_pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
		m_EditGlobalVol.EnableWindow(b);
		m_SpinGlobalVol.EnableWindow(b);
		// Panning
		b = (m_pSndFile->GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
		m_CheckPanning.EnableWindow(b && !(m_pSndFile->GetType() & MOD_TYPE_XM));
		m_EditPanning.EnableWindow(b);
		m_SpinPanning.EnableWindow(b);

		b = (m_pSndFile->m_nType & MOD_TYPE_MOD) ? FALSE : TRUE;
		m_CbnBaseNote.EnableWindow(b);
	}
	// Updating Values
	if (dwHintMask & (HINT_MODTYPE|HINT_SAMPLEINFO))
	{
		MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
		CHAR s[128];
		DWORD d;
		
		// Length / Type
		wsprintf(s, "%d-bit %s, len: %d", pSmp->GetElementarySampleSize() * 8, (pSmp->uFlags & CHN_STEREO) ? "stereo" : "mono", pSmp->nLength);
		SetDlgItemText(IDC_TEXT5, s);
		// Name
		memcpy(s, m_pSndFile->m_szNames[m_nSample], 32);
		s[31] = 0;
		SetDlgItemText(IDC_SAMPLE_NAME, s);
		// File Name
		memcpy(s, pSmp->filename, 22);
		s[21] = 0;
		if (m_pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)) s[0] = 0;
		SetDlgItemText(IDC_SAMPLE_FILENAME, s);
		// Volume
		SetDlgItemInt(IDC_EDIT7, pSmp->nVolume >> 2);
		// Global Volume
		SetDlgItemInt(IDC_EDIT8, pSmp->nGlobalVol);
		// Panning
		CheckDlgButton(IDC_CHECK1, (pSmp->uFlags & CHN_PANNING) ? MF_CHECKED : MF_UNCHECKED);
		//rewbs.fix36944
		if (m_pSndFile->m_nType == MOD_TYPE_XM) {
			SetDlgItemInt(IDC_EDIT9, pSmp->nPan);		//displayed panning with XM is 0-256, just like MPT's internal engine
		} else {
			SetDlgItemInt(IDC_EDIT9, pSmp->nPan>>2);	//displayed panning with anything but XM is 0-64 so we divide by 4
		}
		//end rewbs.fix36944
		// FineTune / C-4 Speed / BaseNote
        int transp = 0;
		if (m_pSndFile->m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			wsprintf(s, "%lu", pSmp->nC5Speed);
			m_EditFineTune.SetWindowText(s);
			transp = CSoundFile::FrequencyToTranspose(pSmp->nC5Speed) >> 7;
		} else
		{
			SetDlgItemInt(IDC_EDIT5, (int)pSmp->nFineTune);
			transp = (int)pSmp->RelativeTone;
		}
		int basenote = 60 - transp;
		if (basenote < BASENOTE_MIN) basenote = BASENOTE_MIN;
		if (basenote >= BASENOTE_MAX) basenote = BASENOTE_MAX-1;
		basenote -= BASENOTE_MIN;
		if (basenote != m_CbnBaseNote.GetCurSel()) m_CbnBaseNote.SetCurSel(basenote);
		// AutoVibrato
		m_ComboAutoVib.SetCurSel(pSmp->nVibType);
		SetDlgItemInt(IDC_EDIT14, (UINT)pSmp->nVibSweep);
		SetDlgItemInt(IDC_EDIT15, (UINT)pSmp->nVibDepth);
		SetDlgItemInt(IDC_EDIT16, (UINT)pSmp->nVibRate);
		// Loop
		d = 0;
		if (pSmp->uFlags & CHN_LOOP) d = (pSmp->uFlags & CHN_PINGPONGLOOP) ? 2 : 1;
		m_ComboLoopType.SetCurSel(d);
		wsprintf(s, "%lu", pSmp->nLoopStart);
		m_EditLoopStart.SetWindowText(s);
		wsprintf(s, "%lu", pSmp->nLoopEnd);
		m_EditLoopEnd.SetWindowText(s);
		// Sustain Loop
		d = 0;
		if (pSmp->uFlags & CHN_SUSTAINLOOP) d = (pSmp->uFlags & CHN_PINGPONGSUSTAIN) ? 2 : 1;
		m_ComboSustainType.SetCurSel(d);
		wsprintf(s, "%lu", pSmp->nSustainStart);
		m_EditSustainStart.SetWindowText(s);
		wsprintf(s, "%lu", pSmp->nSustainEnd);
		m_EditSustainEnd.SetWindowText(s);
	}
	if (!m_bInitialized)
	{
		// First update
		m_bInitialized = TRUE;
		UnlockControls();
	}
	UnlockControls();
}


BOOL CCtrlSamples::OpenSample(LPCSTR lpszFileName)
//------------------------------------------------
{
	CMappedFile f;
	CHAR szName[_MAX_FNAME], szExt[_MAX_EXT];
	LPBYTE lpFile;
	DWORD len;
	BOOL bOk;

	BeginWaitCursor();
	if ((!lpszFileName) || (!f.Open(lpszFileName)))
	{
		EndWaitCursor();
		return FALSE;
	}
	len = f.GetLength();
	if (len > CTrackApp::gMemStatus.dwTotalPhys) len = CTrackApp::gMemStatus.dwTotalPhys;
	bOk = FALSE;
	lpFile = f.Lock(len);
	if (!lpFile) goto OpenError;
	BEGIN_CRITICAL();

	m_pModDoc->GetSampleUndo()->PrepareUndo(m_nSample, sundo_replace);

	if (m_pSndFile->ReadSampleFromFile(m_nSample, lpFile, len))
	{
		bOk = TRUE;
	}
	END_CRITICAL();
	if (!bOk)
	{
		CRawSampleDlg dlg(this);
		EndWaitCursor();
		if (dlg.DoModal() == IDOK)
		{
			BeginWaitCursor();
			UINT flags = 0;
			MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
			BEGIN_CRITICAL();
			m_pSndFile->DestroySample(m_nSample);
			pSmp->nLength = len;
			pSmp->uFlags = RS_PCM8S;
			pSmp->nGlobalVol = 64;
			pSmp->nVolume = 256;
			pSmp->nPan = 128;
			pSmp->filename[0] = 0;
			if (!pSmp->nC5Speed) pSmp->nC5Speed = 22050;
			if (dlg.m_nFormat & 1)
			{
				pSmp->nLength >>= 1;
				pSmp->uFlags |= CHN_16BIT;
				flags = RS_PCM16S;
			}
			if (!(dlg.m_nFormat & 2))
			{
				flags++;
			}
			// Interleaved Stereo Sample
			if (dlg.m_nFormat & 4)
			{
				pSmp->uFlags |= CHN_STEREO;
				pSmp->nLength >>= 1;
				flags |= 0x40|RSF_STEREO;
			}
			LPSTR p16 = (LPSTR)lpFile;
			DWORD l16 = len;
			if ((pSmp->uFlags & CHN_16BIT) && (len & 1))
			{
				p16++;
				l16--;
			}
			if (m_pSndFile->ReadSample(pSmp, flags, p16, l16))
			{
				bOk = TRUE;
			}
			END_CRITICAL();
		} else
		{
			m_pModDoc->GetSampleUndo()->RemoveLastUndoStep(m_nSample);
		}
	}
	f.Unlock();
OpenError:
	f.Close();
	EndWaitCursor();
	if (bOk)
	{
		MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
		CMainFrame::SetWorkingDirectory(lpszFileName, DIR_SAMPLES, true);
		if (!pSmp->filename[0])
		{
			CHAR szFullFilename[_MAX_PATH];
			_splitpath(lpszFileName, 0, 0, szName, szExt);

			memset(szFullFilename, 0, 32);
			strcpy(szFullFilename, szName);
			if (m_pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
			{
				// MOD/XM
				strcat(szFullFilename, szExt);
				szFullFilename[31] = 0;
				memcpy(m_pSndFile->m_szNames[m_nSample], szFullFilename, 32);
			} else
			{
				// S3M/IT
				szFullFilename[31] = 0;
				if (!m_pSndFile->m_szNames[m_nSample][0]) memcpy(m_pSndFile->m_szNames[m_nSample], szFullFilename, 32);
				if (strlen(szFullFilename) < 9) strcat(szFullFilename, szExt);
			}
			szFullFilename[21] = 0;
			memcpy(pSmp->filename, szFullFilename, 22);
		}
		if ((m_pSndFile->m_nType & MOD_TYPE_XM) && (!(pSmp->uFlags & CHN_PANNING)))
		{
			pSmp->nPan = 128;
			pSmp->uFlags |= CHN_PANNING;
		}
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO | HINT_SMPNAMES, NULL);
		m_pModDoc->SetModified();
	}
	return TRUE;
}


BOOL CCtrlSamples::OpenSample(CSoundFile *pSndFile, UINT nSample)
//---------------------------------------------------------------
{
	if ((!pSndFile) || (!nSample) || (nSample > pSndFile->m_nSamples)) return FALSE;

	BeginWaitCursor();
	BEGIN_CRITICAL();
	m_pModDoc->GetSampleUndo()->PrepareUndo(m_nSample, sundo_replace);
	m_pSndFile->DestroySample(m_nSample);
	m_pSndFile->ReadSampleFromSong(m_nSample, pSndFile, nSample);
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
	if ((m_pSndFile->m_nType & MOD_TYPE_XM) && (!(pSmp->uFlags & CHN_PANNING)))
	{
		pSmp->nPan = 128;
		pSmp->uFlags |= CHN_PANNING;
	}
	END_CRITICAL();
	// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
	m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO | HINT_SMPNAMES, NULL);
	m_pModDoc->SetModified();
	EndWaitCursor();
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////
// CCtrlSamples messages

void CCtrlSamples::OnSampleChanged()
//----------------------------------
{
	if ((!IsLocked()) && (m_pSndFile))
	{
		UINT n = GetDlgItemInt(IDC_EDIT_SAMPLE);
		if ((n > 0) && (n <= m_pSndFile->m_nSamples) && (n != m_nSample))
		{
			SetCurrentSample(n, -1, FALSE);
			if (m_pParent)
			{
				if (m_pSndFile->m_nInstruments)
				{
					UINT k = m_pParent->GetInstrumentChange();
					if (!m_pModDoc->IsChildSample(k, m_nSample))
					{
						UINT nins = m_pModDoc->FindSampleParent(m_nSample);
						if (nins) m_pParent->InstrumentChanged(nins);
					}
				} else
				{
					m_pParent->InstrumentChanged(m_nSample);
				}
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
	LONG smp = m_pModDoc->InsertSample(TRUE);
	if (smp != SAMPLEINDEX_INVALID)
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		SetCurrentSample(smp);
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (smp << HINT_SHIFT_SMP) | HINT_SAMPLEINFO | HINT_SAMPLEDATA | HINT_SMPNAMES);
		if ((pSndFile->m_nInstruments) && (!m_pModDoc->FindSampleParent(smp)))
		{
			if (MessageBox("This sample is not used by any instrument. Do you want to create a new instrument using this sample ?",
					NULL, MB_YESNO|MB_ICONQUESTION) == IDYES)
			{
				UINT nins = m_pModDoc->InsertInstrument(smp);
				m_pModDoc->UpdateAllViews(NULL, (nins << HINT_SHIFT_INS) | HINT_INSTRUMENT | HINT_INSNAMES | HINT_ENVELOPE);
				m_pParent->InstrumentChanged(nins);
			}
		}
	}
	SwitchToView();
}


void CCtrlSamples::OnSampleOpen()
//-------------------------------
{
	static int nLastIndex = 0;
	
	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(true, "", "",
		"All Samples|*.wav;*.pat;*.s3i;*.smp;*.snd;*.raw;*.xi;*.aif;*.aiff;*.its;*.8sv;*.8svx;*.svx;*.pcm|"
		"Wave Files (*.wav)|*.wav|"
		"XI Samples (*.xi)|*.xi|"
		"Impulse Tracker Samples (*.its)|*.its|"
		"ScreamTracker Samples (*.s3i,*.smp)|*.s3i;*.smp|"
		"GF1 Patches (*.pat)|*.pat|"
		"AIFF Files (*.aiff;*.8svx)|*.aif;*.aiff;*.8sv;*.8svx;*.svx|"
		"Raw Samples (*.raw,*.snd,*.pcm)|*.raw;*.snd;*.pcm|"
		"All Files (*.*)|*.*||",
		CMainFrame::GetWorkingDirectory(DIR_SAMPLES),
		true,
		&nLastIndex);
	if(files.abort) return;

	CMainFrame::SetWorkingDirectory(files.workingDirectory.c_str(), DIR_SAMPLES, true);

	for(size_t counter = 0; counter < files.filenames.size(); counter++)
	{
		//If loading multiple samples, advancing to next sample and creating
		//new one if necessary.
		if(counter > 0)	
		{
			if(m_nSample >= MAX_SAMPLES-1)
				break;
			else
				m_nSample++;

			if(m_nSample > m_pSndFile->GetNumSamples())
				OnSampleNew();
		}

		if(!OpenSample(files.filenames[counter].c_str()))
			ErrorBox(IDS_ERR_FILEOPEN, this);
	}
	SwitchToView();
}


void CCtrlSamples::OnSampleSave()
//-------------------------------
{
	if(!m_pSndFile) return;

	TCHAR szFileName[_MAX_PATH];
	bool bBatchSave = CMainFrame::GetInputHandler()->ShiftPressed();

	if(!bBatchSave)
	{
		// save this sample
		if ((!m_nSample) || (!m_pSndFile->Samples[m_nSample].pSample))
		{
			SwitchToView();
			return;
		}
		if (m_pSndFile->m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			memcpy(szFileName, m_pSndFile->Samples[m_nSample].filename, 22);
			szFileName[22] = 0;
		} else
		{
			memcpy(szFileName, m_pSndFile->m_szNames[m_nSample], 32);
			szFileName[32] = 0;		}
		if (!szFileName[0]) strcpy(szFileName, "untitled");
	}
	else
	{
		// save all samples
		CString sPath = m_pSndFile->m_pModDoc->GetPathName();
		if(sPath.IsEmpty()) sPath = "untitled";

		sPath += " - %sample_number% - ";
		if(m_pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_MOD))
			sPath += "%sample_name%.wav";
		else
			sPath += "%sample_filename%.wav";
		sPath += ".wav";
		_splitpath(sPath, NULL, NULL, szFileName, NULL);
	}
	SanitizeFilename(szFileName);

	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, "wav", szFileName,
		"Wave File (*.wav)|*.wav|"
		"RAW Audio (*.raw)|*.raw||",
		CMainFrame::GetWorkingDirectory(DIR_SAMPLES));
	if(files.abort) return;

	BeginWaitCursor();

	TCHAR ext[_MAX_EXT];
	_splitpath(files.first_file.c_str(), NULL, NULL, NULL, ext);

	bool bOk = false;
	UINT iMinSmp = m_nSample, iMaxSmp = m_nSample;
	CString sFilename = files.first_file.c_str(), sNumberFormat;
	if(bBatchSave)
	{
		iMinSmp = 1;
		iMaxSmp = m_pSndFile->m_nSamples;
		sNumberFormat.Format("%s%d%s", "%.", ((int)log10((float)iMaxSmp)) + 1, "d");
	}

	for(UINT iSmp = iMinSmp; iSmp <= iMaxSmp; iSmp++)
	{
		if (m_pSndFile->Samples[iSmp].pSample)
		{
			if(bBatchSave)
			{
				CString sSampleNumber;
				TCHAR sSampleName[64], sSampleFilename[64];
				sSampleNumber.Format(sNumberFormat, iSmp);

				strcpy(sSampleName, (m_pSndFile->m_szNames[iSmp]) ? m_pSndFile->m_szNames[iSmp] : "untitled");
				strcpy(sSampleFilename, (m_pSndFile->Samples[iSmp].filename[0]) ? m_pSndFile->Samples[iSmp].filename : m_pSndFile->m_szNames[iSmp]);
				SanitizeFilename(sSampleName);
				SanitizeFilename(sSampleFilename);

				sFilename = files.first_file.c_str();
				sFilename.Replace("%sample_number%", sSampleNumber);
				sFilename.Replace("%sample_filename%", sSampleFilename);
				sFilename.Replace("%sample_name%", sSampleName);
			}
			if (!lstrcmpi(ext, ".raw"))
				bOk = m_pSndFile->SaveRAWSample(iSmp, sFilename);
			else
				bOk = m_pSndFile->SaveWAVSample(iSmp, sFilename);
		}
	}
	EndWaitCursor();

	if (!bOk)
	{
		ErrorBox(IDS_ERR_SAVESMP, this);
	} else
	{
		CMainFrame::SetWorkingDirectory(files.workingDirectory.c_str(), DIR_SAMPLES, true);
	}
	SwitchToView();
}


void CCtrlSamples::OnSamplePlay()
//-------------------------------
{
	if ((m_pModDoc) && (m_pSndFile))
	{
		// Fix (bug report 1366).
		// if ((m_pSndFile->IsPaused()) && (m_pModDoc->IsNotePlaying(0, m_nSample, 0)))
		if (m_pModDoc->IsNotePlaying(0, m_nSample, 0))
		{
			m_pModDoc->NoteOff(0, TRUE);
		} else
		{
			m_pModDoc->PlayNote(NOTE_MIDDLEC, 0, m_nSample, FALSE);
		}
	}
	SwitchToView();
}


void CCtrlSamples::OnNormalize()
//------------------------------
{
	if(!m_pModDoc || !m_pSndFile)
		return;

	//Default case: Normalize current sample
	UINT iMinSample = m_nSample, iMaxSample = m_nSample;
	//If only one sample is selected, parts of it may be amplified
	UINT iStart = 0, iEnd = 0;

	//Shift -> Normalize all samples
	if(CMainFrame::GetInputHandler()->ShiftPressed())
	{
		if(MessageBox(GetStrI18N(TEXT("This will normalize all samples independently. Continue?")), GetStrI18N(TEXT("Normalize")), MB_YESNO | MB_ICONQUESTION) == IDNO)
			return;
		iMinSample = 1;
		iMaxSample = m_pSndFile->m_nSamples;
	} else {
		SAMPLEVIEWSTATE viewstate;
		memset(&viewstate, 0, sizeof(viewstate));
		SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&viewstate);

		iStart = viewstate.dwBeginSel;
		iEnd = viewstate.dwEndSel;
	}


	BeginWaitCursor();
	BOOL bModified = FALSE;

	for(UINT iSmp = iMinSample; iSmp <= iMaxSample; iSmp++)
	{
		if (m_pSndFile->Samples[iSmp].pSample)
		{
			BOOL bOk = FALSE;
			MODSAMPLE *pSmp = &m_pSndFile->Samples[iSmp];
		
			if(iMinSample != iMaxSample) {
				//if more than one sample is selected, always amplify the whole sample.
				iStart = 0;
				iEnd = pSmp->nLength;
			} else {
				//one sample: correct the boundaries, if needed
				if (iEnd > pSmp->nLength) iEnd = pSmp->nLength;
				if (iStart > iEnd) iStart = iEnd;
				if (iStart == iEnd) {
					iStart = 0;
					iEnd = pSmp->nLength;
				}
			}

			m_pModDoc->GetSampleUndo()->PrepareUndo(iSmp, sundo_update, iStart, iEnd);

			if (pSmp->uFlags & CHN_STEREO) { iStart *= 2; iEnd *= 2; }

			if (pSmp->uFlags & CHN_16BIT)
			{
				signed short *p = (signed short *)pSmp->pSample;
				int max = 1;
				for (UINT i = iStart; i < iEnd; i++)
				{
					if (p[i] > max) max = p[i];
					if (-p[i] > max) max = -p[i];
				}
				if (max < 32767)
				{
					max++;
					for (UINT j = iStart; j < iEnd; j++)
					{
						int l = p[j];
						p[j] = (l << 15) / max;
					}
					bModified = bOk = TRUE;
				}
			} else
			{
				signed char *p = (signed char *)pSmp->pSample;
				int max = 1;
				for (UINT i = iStart; i < iEnd; i++)
				{
					if (p[i] > max) max = p[i];
					if (-p[i] > max) max = -p[i];
				}
				if (max < 127)
				{
					max++;
					for (UINT j = iStart; j < iEnd; j++)
					{
						int l = p[j];
						p[j] = (l << 7) / max;
					}
					bModified = bOk = TRUE;
				}
			}

			if (bOk)
			{
				m_pModDoc->AdjustEndOfSample(iSmp);
				m_pModDoc->UpdateAllViews(NULL, (iSmp << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, NULL);
			}
		}
	}

	if(bModified)
	{
		m_pModDoc->SetModified();
	}
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::ApplyAmplify(LONG lAmp, bool bFadeIn, bool bFadeOut)
//-----------------------------------------------------------------------
{
	MODSAMPLE *pSmp;
	
	if ((!m_pModDoc) || (!m_pSndFile) || (!m_pSndFile->Samples[m_nSample].pSample)) return;

	BeginWaitCursor();
	pSmp = &m_pSndFile->Samples[m_nSample];

	SELECTIONPOINTS selection = GetSelectionPoints();

	m_pModDoc->GetSampleUndo()->PrepareUndo(m_nSample, sundo_update, selection.nStart, selection.nEnd);

	if (pSmp->uFlags & CHN_STEREO) { selection.nStart *= 2; selection.nEnd *= 2; }
	UINT len = selection.nEnd - selection.nStart;
	if ((bFadeIn) && (bFadeOut)) lAmp *= 4;
	if (pSmp->uFlags & CHN_16BIT)
	{
		signed short *p = ((signed short *)pSmp->pSample) + selection.nStart;

		for (UINT i=0; i<len; i++)
		{
			LONG l = (p[i] * lAmp) / 100;
			if (bFadeIn) l = (LONG)((l * (LONGLONG)i) / len);
			if (bFadeOut) l = (LONG)((l * (LONGLONG)(len-i)) / len);
			if (l < -32768) l = -32768;
			if (l > 32767) l = 32767;
			p[i] = (signed short)l;
		}
	} else
	{
		signed char *p = ((signed char *)pSmp->pSample) + selection.nStart;

		for (UINT i=0; i<len; i++)
		{
			LONG l = (p[i] * lAmp) / 100;
			if (bFadeIn) l = (LONG)((l * (LONGLONG)i) / len);
			if (bFadeOut) l = (LONG)((l * (LONGLONG)(len-i)) / len);
			if (l < -128) l = -128;
			if (l > 127) l = 127;
			p[i] = (signed char)l;
		}
	}
	m_pModDoc->AdjustEndOfSample(m_nSample);
	// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
	m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, NULL);
	m_pModDoc->SetModified();
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnRemoveDCOffset()
//-----------------------------------
{
	if(!m_pModDoc || !m_pSndFile)
		return;

	UINT iMinSample = m_nSample, iMaxSample = m_nSample;

	//Shift -> Process all samples
	if(CMainFrame::GetInputHandler()->ShiftPressed())
	{
		if(MessageBox(GetStrI18N(TEXT("This will process all samples independently. Continue?")), GetStrI18N(TEXT("DC Offset Removal")), MB_YESNO | MB_ICONQUESTION) == IDNO)
			return;
		iMinSample = 1;
		iMaxSample = m_pSndFile->m_nSamples;
	}

	BeginWaitCursor();

	// for report / SetModified
	UINT iModified = 0;
	float fReportOffset = 0;

	for(UINT iSmp = iMinSample; iSmp <= iMaxSample; iSmp++)
	{
		UINT iStart, iEnd;

		if( m_pSndFile->Samples[iSmp].pSample == nullptr )
			continue;

		if (iMinSample != iMaxSample)
		{
			iStart = 0;
			iEnd = m_pSndFile->Samples[iSmp].nLength;
		}
		else
		{
			SELECTIONPOINTS selection = GetSelectionPoints();
			iStart = selection.nStart;
			iEnd = selection.nEnd;
		}

		m_pModDoc->GetSampleUndo()->PrepareUndo(iSmp, sundo_update, iStart, iEnd);

		const float fOffset = ctrlSmp::RemoveDCOffset(m_pSndFile->Samples[iSmp], iStart, iEnd, m_pSndFile->GetType(), m_pSndFile);

		if(fOffset == 0.0f) // No offset removed.
			continue;

		fReportOffset += fOffset;
		iModified++;
		m_pModDoc->UpdateAllViews(NULL, (iSmp << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO, NULL);
	}

	EndWaitCursor();
	SwitchToView();

	// fill the statusbar with some nice information

	CString dcInfo;
	if(iModified)
	{
		m_pModDoc->SetModified();
		if(iModified == 1)
		{
			dcInfo.Format(GetStrI18N(TEXT("Removed DC offset (%.1f%%)")), fReportOffset * 100);
		}
		else
		{
			dcInfo.Format(GetStrI18N(TEXT("Removed DC offset from %d samples (avg %0.1f%%)")), iModified, fReportOffset / iModified * 100);
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


const int gSinc2x16Odd[16] =
{ // Kaiser window, beta=7.400
  -19,    97,  -295,   710, -1494,  2958, -6155, 20582, 20582, -6155,  2958, -1494,   710,  -295,    97,   -19,
};


void CCtrlSamples::OnUpsample()
//-----------------------------
{
	MODSAMPLE *pSmp;
	DWORD dwStart, dwEnd, dwNewLen;
	UINT smplsize, newsmplsize;
	PVOID pOriginal, pNewSample;

	if ((!m_pSndFile) || (!m_pSndFile->Samples[m_nSample].pSample)) return;
	BeginWaitCursor();
	pSmp = &m_pSndFile->Samples[m_nSample];
	SELECTIONPOINTS selection = GetSelectionPoints();

	dwStart = selection.nStart;
	dwEnd = selection.nEnd;
	if (dwEnd > pSmp->nLength) dwEnd = pSmp->nLength;
	if (dwStart >= dwEnd)
	{
		dwStart = 0;
		dwEnd = pSmp->nLength;
	}

	smplsize = (pSmp->uFlags & CHN_16BIT) ? 2 : 1;
	if (pSmp->uFlags & CHN_STEREO) smplsize *= 2;
	newsmplsize = (pSmp->uFlags & CHN_STEREO) ? 4 : 2;
	pOriginal = pSmp->pSample;
	dwNewLen = pSmp->nLength + (dwEnd-dwStart);
	pNewSample = NULL;
	if (dwNewLen+4 <= MAX_SAMPLE_LENGTH) pNewSample = CSoundFile::AllocateSample((dwNewLen+4)*newsmplsize);
	if (pNewSample)
	{
		m_pModDoc->GetSampleUndo()->PrepareUndo(m_nSample, sundo_replace);

		UINT nCh = (pSmp->uFlags & CHN_STEREO) ? 2 : 1;
		for (UINT iCh=0; iCh<nCh; iCh++)
		{
			int len = dwEnd-dwStart;
			int maxndx = pSmp->nLength;
			if (pSmp->uFlags & CHN_16BIT)
			{
				signed short *psrc = ((signed short *)pOriginal)+iCh;
				signed short *pdest = ((signed short *)pNewSample)+dwStart*nCh+iCh;
				for (int i=0; i<len; i++)
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
				for (int i=0; i<len; i++)
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
		if (pSmp->uFlags & CHN_16BIT)
		{
			if (dwStart > 0) memcpy(pNewSample, pOriginal, dwStart*smplsize);
			if (dwEnd < pSmp->nLength) memcpy(((LPSTR)pNewSample)+(dwStart+(dwEnd-dwStart)*2)*smplsize, ((LPSTR)pOriginal)+(dwEnd*smplsize), (pSmp->nLength-dwEnd)*smplsize);
		} else
		{
			if (dwStart > 0)
			{
				for (UINT i=0; i<dwStart*nCh; i++)
				{
					((signed short *)pNewSample)[i] = (signed short)(((signed char *)pOriginal)[i] << 8);
				}
			}
			if (dwEnd < pSmp->nLength)
			{
				signed short *pdest = ((signed short *)pNewSample) + (dwEnd-dwStart)*nCh;
				for (UINT i=dwEnd*nCh; i<pSmp->nLength*nCh; i++)
				{
					pdest[i] = (signed short)(((signed char *)pOriginal)[i] << 8);
				}
			}
		}
		if (pSmp->nLoopStart >= dwEnd) pSmp->nLoopStart += (dwEnd-dwStart); else
		if (pSmp->nLoopStart > dwStart) pSmp->nLoopStart += (pSmp->nLoopStart - dwStart);
		if (pSmp->nLoopEnd >= dwEnd) pSmp->nLoopEnd += (dwEnd-dwStart); else
		if (pSmp->nLoopEnd > dwStart) pSmp->nLoopEnd += (pSmp->nLoopEnd - dwStart);
		if (pSmp->nSustainStart >= dwEnd) pSmp->nSustainStart += (dwEnd-dwStart); else
		if (pSmp->nSustainStart > dwStart) pSmp->nSustainStart += (pSmp->nSustainStart - dwStart);
		if (pSmp->nSustainEnd >= dwEnd) pSmp->nSustainEnd += (dwEnd-dwStart); else
		if (pSmp->nSustainEnd > dwStart) pSmp->nSustainEnd += (pSmp->nSustainEnd - dwStart);
		BEGIN_CRITICAL();
		for (UINT iFix=0; iFix<MAX_CHANNELS; iFix++)
		{
			if ((PVOID)m_pSndFile->Chn[iFix].pSample == pOriginal)
			{
				m_pSndFile->Chn[iFix].pSample = (LPSTR)pNewSample;
				m_pSndFile->Chn[iFix].pCurrentSample = (LPSTR)pNewSample;
				m_pSndFile->Chn[iFix].dwFlags |= CHN_16BIT;
			}
		}
		if (selection.bSelected == false)
		{
			if(!(m_pSndFile->m_nType & MOD_TYPE_MOD))
			{
				if (pSmp->nC5Speed < 200000) pSmp->nC5Speed *= 2;
				if (pSmp->RelativeTone < 84) pSmp->RelativeTone += 12;
			}
		}
		pSmp->uFlags |= CHN_16BIT;
		pSmp->pSample = (LPSTR)pNewSample;
		pSmp->nLength = dwNewLen;

		CSoundFile::FreeSample(pOriginal);
		END_CRITICAL();
		m_pModDoc->AdjustEndOfSample(m_nSample);
		if (selection.bSelected == true)
		{
			SAMPLEVIEWSTATE viewstate;
			memset(&viewstate, 0, sizeof(viewstate));
			SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&viewstate);

			viewstate.dwBeginSel = dwStart;
			viewstate.dwEndSel = dwEnd + (dwEnd-dwStart);
			SendViewMessage(VIEWMSG_LOADSTATE, (LPARAM)&viewstate);
		}
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO, NULL);
		m_pModDoc->SetModified();
	}
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnDownsample()
//-------------------------------
{
	MODSAMPLE *pSmp;
	DWORD dwStart, dwEnd, dwRemove, dwNewLen;
	UINT smplsize;
	PVOID pOriginal, pNewSample;

	if ((!m_pSndFile) || (!m_pSndFile->Samples[m_nSample].pSample)) return;
	BeginWaitCursor();
	pSmp = &m_pSndFile->Samples[m_nSample];
	SELECTIONPOINTS selection = GetSelectionPoints();

	dwStart = selection.nStart;
	dwEnd = selection.nEnd;
	if (dwEnd > pSmp->nLength) dwEnd = pSmp->nLength;
	if (dwStart >= dwEnd)
	{
		dwStart = 0;
		dwEnd = pSmp->nLength;
	}
	smplsize = (pSmp->uFlags & CHN_16BIT) ? 2 : 1;
	if (pSmp->uFlags & CHN_STEREO) smplsize *= 2;
	pOriginal = pSmp->pSample;
	dwRemove = (dwEnd-dwStart+1)>>1;
	dwNewLen = pSmp->nLength - dwRemove;
	dwEnd = dwStart+dwRemove*2;
	pNewSample = NULL;
	if ((dwNewLen > 32) && (dwRemove)) pNewSample = CSoundFile::AllocateSample((dwNewLen+4)*smplsize);
	if (pNewSample)
	{

		m_pModDoc->GetSampleUndo()->PrepareUndo(m_nSample, sundo_replace);

		UINT nCh = (pSmp->uFlags & CHN_STEREO) ? 2 : 1;
		for (UINT iCh=0; iCh<nCh; iCh++)
		{
			int len = dwRemove;
			int maxndx = pSmp->nLength;
			if (pSmp->uFlags & CHN_16BIT)
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
		if (dwEnd < pSmp->nLength) memcpy(((LPSTR)pNewSample)+(dwStart+dwRemove)*smplsize, ((LPSTR)pOriginal)+((dwStart+dwRemove*2)*smplsize), (pSmp->nLength-dwEnd)*smplsize);
		if (pSmp->nLoopStart >= dwEnd) pSmp->nLoopStart -= dwRemove; else
		if (pSmp->nLoopStart > dwStart) pSmp->nLoopStart -= (pSmp->nLoopStart - dwStart)/2;
		if (pSmp->nLoopEnd >= dwEnd) pSmp->nLoopEnd -= dwRemove; else
		if (pSmp->nLoopEnd > dwStart) pSmp->nLoopEnd -= (pSmp->nLoopEnd - dwStart)/2;
		if (pSmp->nLoopEnd > dwNewLen) pSmp->nLoopEnd = dwNewLen;
		if (pSmp->nSustainStart >= dwEnd) pSmp->nSustainStart -= dwRemove; else
		if (pSmp->nSustainStart > dwStart) pSmp->nSustainStart -= (pSmp->nSustainStart - dwStart)/2;
		if (pSmp->nSustainEnd >= dwEnd) pSmp->nSustainEnd -= dwRemove; else
		if (pSmp->nSustainEnd > dwStart) pSmp->nSustainEnd -= (pSmp->nSustainEnd - dwStart)/2;
		if (pSmp->nSustainEnd > dwNewLen) pSmp->nSustainEnd = dwNewLen;
		BEGIN_CRITICAL();
		for (UINT iFix=0; iFix<MAX_CHANNELS; iFix++)
		{
			if ((PVOID)m_pSndFile->Chn[iFix].pSample == pOriginal)
			{
				m_pSndFile->Chn[iFix].pSample = (LPSTR)pNewSample;
				m_pSndFile->Chn[iFix].pCurrentSample = NULL;
				m_pSndFile->Chn[iFix].nLength = 0;
			}
		}
		if (selection.bSelected == false)
		{
			if(!(m_pSndFile->m_nType & MOD_TYPE_MOD))
			{
				if (pSmp->nC5Speed > 2000) pSmp->nC5Speed /= 2;
				if (pSmp->RelativeTone > -84) pSmp->RelativeTone -= 12;
			}
		}
		pSmp->nLength = dwNewLen;
		pSmp->pSample = (LPSTR)pNewSample;
		CSoundFile::FreeSample(pOriginal);
		END_CRITICAL();
		m_pModDoc->AdjustEndOfSample(m_nSample);
		if (selection.bSelected == true)
		{
			SAMPLEVIEWSTATE viewstate;
			memset(&viewstate, 0, sizeof(viewstate));
			SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&viewstate);

			viewstate.dwBeginSel = dwStart;
			viewstate.dwEndSel = dwStart + dwRemove;
			SendViewMessage(VIEWMSG_LOADSTATE, (LPARAM)&viewstate);
		}
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO, NULL);
		m_pModDoc->SetModified();
	}
	EndWaitCursor();
	SwitchToView();
}


#define MAX_BUFFER_LENGTH	8192
#define CLIP_SOUND(v)		v = v < -1.0f ? -1.0f : v > 1.0f ? 1.0f : v

void CCtrlSamples::ReadTimeStretchParameters()
//--------------------------------------------
{
	CString str;
	GetDlgItemText(IDC_EDIT_STRETCHPARAMS, str);
	_stscanf(str, __TEXT("%u %u %u %u"),
		&m_nSequenceMs, &m_nSeekWindowMs, &m_nOverlapMs, &m_nStretchProcessStepLength);
}


void CCtrlSamples::UpdateTimeStretchParameterString()
//---------------------------------------------------
{
	CString str;
	str.Format(__TEXT("%u %u %u %u"),
				m_nSequenceMs,
				m_nSeekWindowMs,
				m_nOverlapMs,
				m_nStretchProcessStepLength
	          );
	SetDlgItemText(IDC_EDIT_STRETCHPARAMS, str);
}

void CCtrlSamples::OnEnableStretchToSize()
//----------------------------------------
{
	// Enable time-stretching / disable unused pitch-shifting UI elements
	bool bTimeStretch = IsDlgButtonChecked(IDC_CHECK3) ? true : false;
	if(!bTimeStretch) ReadTimeStretchParameters();
	((CComboBox *)GetDlgItem(IDC_COMBO4))->EnableWindow(bTimeStretch ? false : true);
	((CEdit *)GetDlgItem(IDC_EDIT6))->EnableWindow(bTimeStretch ? true : false);
	((CButton *)GetDlgItem(IDC_BUTTON2))->EnableWindow(bTimeStretch ? true : false); //rewbs.timeStretchMods
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
{
	if((!m_pSndFile) || (!m_pSndFile->Samples[m_nSample].pSample)) return;
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
	if(!pSmp) return;

	//rewbs.timeStretchMods
	//Ensure m_dTimeStretchRatio is up-to-date with textbox content
	UpdateData(TRUE);

	//Calculate/verify samplerate at C5.
	long lSampleRate = pSmp->nC5Speed;
	if(m_pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
		lSampleRate = (double)CSoundFile::TransposeToFrequency(pSmp->RelativeTone, pSmp->nFineTune);
	if(lSampleRate <= 0) 
		lSampleRate = 8363.0;

	//Open dialog
	CPSRatioCalc dlg(pSmp->nLength, lSampleRate, 
					 m_pSndFile->m_nMusicSpeed,  m_pSndFile->m_nMusicTempo, 
					 m_pSndFile->m_nRowsPerBeat, m_pSndFile->m_nTempoMode,
					 m_dTimeStretchRatio, this);
	if (dlg.DoModal() != IDOK) return;
	
    //Update ratio value&textbox
	m_dTimeStretchRatio = dlg.m_dRatio;
	UpdateData(FALSE);
	//end rewbs.timeStretchMods
}


void CCtrlSamples::OnPitchShiftTimeStretch()
{
	// Error management
	int errorcode = -1;

	if((!m_pSndFile) || (!m_pSndFile->Samples[m_nSample].pSample)) goto error;
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
	if(!pSmp || pSmp->nLength == 0) goto error;

	// Time stretching
	if(IsDlgButtonChecked(IDC_CHECK3)){
		//rewbs.timeStretchMods
		UpdateData(TRUE); //Ensure m_dTimeStretchRatio is up-to-date with textbox content
		errorcode = TimeStretch(m_dTimeStretchRatio/100.0);

		//Update loop points only if no error occured.
		if(errorcode == 0)
		{
			pSmp->nLoopStart *= m_dTimeStretchRatio/100.0;
			pSmp->nLoopEnd *= m_dTimeStretchRatio/100.0;
			pSmp->nSustainStart *= m_dTimeStretchRatio/100.0;
			pSmp->nSustainEnd *= m_dTimeStretchRatio/100.0;
		}
		//end rewbs.timeStretchMods
		
	}
	// Pitch shifting
	else{
		// Get selected pitch modifier [-12,+12]
		CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO4);
		short pm = combo->GetCurSel() - 12;
		if(pm == 0) goto error;

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

	if(errorcode > 0){
		CHAR str[64];
		switch(errorcode & 0xff){
			case 1 : wsprintf(str,"Pitch %s...",(errorcode>>8) == 1 ? "< 0.5" : "> 2.0");
				break;
			case 2 : wsprintf(str,"Stretch ratio is too %s. Must be between 50% and 200%.",(errorcode>>8) == 1 ? "low" : "high");
				break;
			case 3 : wsprintf(str,"Not enough memory...");
				break;
			case 5 : wsprintf(str, "Too low sample rate");
				break;
			case 6 : wsprintf(str, "Too short sample");
				break;
			default: wsprintf(str,"Unknown Error...");
				break;
		}
		AfxMessageBox(str, MB_ICONERROR);
	}

	// Update sample view
	m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO, NULL); // !!!! see CODE#0006, update#3
	m_pModDoc->SetModified();
}


int CCtrlSamples::TimeStretch(double ratio)
//-----------------------------------------
{
	if((!m_pSndFile) || (!m_pSndFile->Samples[m_nSample].pSample)) return -1;
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
	if(!pSmp) return -1;

	const uint32 nSampleRate = pSmp->GetSampleRate(m_pSndFile->GetType());

	// SoundTouch(1.3.1) seems to crash with short samples. Don't know what
	// the actual limit or whether it depends on sample rate,
	// but simply set some semiarbitrary threshold here.
	if(pSmp->nLength < 256)
		return 6;

	// Refuse processing when ratio is negative, equal to zero or equal to 1.0
	if(ratio <= 0.0 || ratio == 1.0) return -1;

	// Convert to pitch factor
	float pitch = Round((float)ratio, 4);
	if(pitch < 0.5f) return 2 + (1<<8);
	if(pitch > 2.0f) return 2 + (2<<8);

	soundtouch::SoundTouch* pSoundTouch = 0;
	try
	{
		pSoundTouch = new soundtouch::SoundTouch;
	}
	catch(...)
	{   // Assuming that thrown exception means that soundtouch library could not be loaded.
		MessageBox("Failed to load soundtouch library.", 0, MB_ICONERROR);
		return -1;
	}

	// Get number of channels & sample size
	BYTE smpsize = pSmp->GetElementarySampleSize();
	const UINT nChn = pSmp->GetNumChannels();

	// Stretching is implemented only for 16-bit samples.
	if(smpsize != 2)
	{
		// This has to be converted to 16-bit first.
		OnUpsample();
		smpsize = pSmp->GetElementarySampleSize();
	}

	// SoundTouch(v1.4.0) documentation says that sample rates 8000-48000 are supported.
	// Check whether sample rate is within that range, and if not,
	// ask user whether to proceed.
	if(nSampleRate < 8000 || nSampleRate > 48000)
	{
		CString str;
		str.Format(TEXT(GetStrI18N("Current samplerate, %u Hz, is not in the supported samplerate range 8000 Hz - 48000 Hz. Continue?")), nSampleRate);
		if(AfxMessageBox(str, MB_ICONQUESTION|MB_YESNO) != IDYES)
			return -1;

	}

	// Allocate new sample. Returned sample may not be exactly the size what ratio would suggest
	// so allocate a bit more(1.03*).
	const DWORD nNewSampleLength = (DWORD)(1.03 * ratio * (double)pSmp->nLength);
	//const DWORD nNewSampleLength = (DWORD)(0.5 + ratio * (double)pSmp->nLength);
	PVOID pNewSample = CSoundFile::AllocateSample(nNewSampleLength * nChn * smpsize);
	if(pNewSample == NULL)
	{
		delete pSoundTouch;
		return 3;
	}

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

	UINT pos = 0;
	UINT len = 0; //To contain length of processing step.

	// Initialize soundtouch object.
	{	
		if(nSampleRate < 300) // Too low samplerate crashes soundtouch.
		{                     // Limiting it to value 300(quite arbitrarily chosen).
			delete pSoundTouch;
			return 5;         
		}
		pSoundTouch->setSampleRate(nSampleRate);
		pSoundTouch->setChannels(nChn);
		// Given ratio is time stretch ratio, and must be converted to
		// tempo change ratio: for example time stretch ratio 2 means
		// tempo change ratio 0.5.
		pSoundTouch->setTempoChange( (1.0f / ratio - 1.0f) * 100.0f);
		pSoundTouch->setSetting(SETTING_USE_QUICKSEEK, 0);

		// Read settings from GUI.
		ReadTimeStretchParameters();	

		if(m_nStretchProcessStepLength == 0) m_nStretchProcessStepLength = nDefaultStretchChunkSize;
		if(m_nStretchProcessStepLength < 64) m_nStretchProcessStepLength = nDefaultStretchChunkSize;
		len = m_nStretchProcessStepLength;
		
		// Set settings to soundtouch. Zero value means 'use default', and
        // setting value is read back after setting because not all settings are accepted.
		if(m_nSequenceMs == 0) m_nSequenceMs = DEFAULT_SEQUENCE_MS;
		pSoundTouch->setSetting(SETTING_SEQUENCE_MS, m_nSequenceMs);
		m_nSequenceMs = pSoundTouch->getSetting(SETTING_SEQUENCE_MS);
		
		if(m_nSeekWindowMs == 0) m_nSeekWindowMs = DEFAULT_SEEKWINDOW_MS;
		pSoundTouch->setSetting(SETTING_SEEKWINDOW_MS, m_nSeekWindowMs);
		m_nSeekWindowMs = pSoundTouch->getSetting(SETTING_SEEKWINDOW_MS);
		
		if(m_nOverlapMs == 0) m_nOverlapMs = DEFAULT_OVERLAP_MS;
		pSoundTouch->setSetting(SETTING_OVERLAP_MS, m_nOverlapMs);
		m_nOverlapMs = pSoundTouch->getSetting(SETTING_OVERLAP_MS);
		
		// Update GUI with the actual SoundTouch parameters in effect.
		UpdateTimeStretchParameterString();
	}

	// Keeps count of the sample length received from stretching process.
	UINT nLengthCounter = 0;

	// Process sample in steps.
	while(pos < pSmp->nLength)
	{
		// Current chunk size limit test
		if(len >= pSmp->nLength - pos) len = pSmp->nLength - pos;

		// Show progress bar using process button painting & text label
		CHAR progress[16];
		float percent = 100.0f * (pos + len) / pSmp->nLength;
		progressBarRECT.right = processButtonRect.left + (int)percent * (processButtonRect.right - processButtonRect.left) / 100;
		wsprintf(progress,"%d%%",(UINT)percent);

		::FillRect(processButtonDC,&processButtonRect,red);
		::FrameRect(processButtonDC,&processButtonRect,CMainFrame::brushBlack);
		::FillRect(processButtonDC,&progressBarRECT,green);
		::SelectObject(processButtonDC,(HBRUSH)HOLLOW_BRUSH);
		::SetBkMode(processButtonDC,TRANSPARENT);
		::DrawText(processButtonDC,progress,strlen(progress),&processButtonRect,DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		::GdiFlush();

		// Send sampledata for processing.
		pSoundTouch->putSamples(reinterpret_cast<int16*>(pSmp->pSample + pos * smpsize * nChn), len);

		// Receive some processed samples (it's not guaranteed that there is any available).
		nLengthCounter += pSoundTouch->receiveSamples(reinterpret_cast<int16*>(pNewSample) + nChn * nLengthCounter, nNewSampleLength - nLengthCounter);

		// Next buffer chunk
		pos += len;
	}

	// The input sample should now be processed. Receive remaining samples.
	pSoundTouch->flush();
	while(pSoundTouch->numSamples() > 0 && nNewSampleLength > nLengthCounter)
	{
		nLengthCounter += pSoundTouch->receiveSamples(reinterpret_cast<int16*>(pNewSample) + nChn * nLengthCounter, nNewSampleLength - nLengthCounter);
	}
	delete pSoundTouch; pSoundTouch = 0;

	ASSERT(nNewSampleLength >= nLengthCounter);

	m_pModDoc->GetSampleUndo()->PrepareUndo(m_nSample, sundo_replace);
	// Swap sample buffer pointer to new buffer, update song + sample data & free old sample buffer
	ctrlSmp::ReplaceSample(*pSmp, (LPSTR)pNewSample, min(nLengthCounter, nNewSampleLength), m_pSndFile);

	// Free progress bar brushes
	DeleteObject((HBRUSH)green);
	DeleteObject((HBRUSH)red);

	// Restore process button text
	SetDlgItemText(IDC_BUTTON1, oldText);

	// Restore mouse cursor
	EndWaitCursor();

	return 0;
}

int CCtrlSamples::PitchShift(float pitch)
//---------------------------------------
{
	if((!m_pSndFile) || (!m_pSndFile->Samples[m_nSample].pSample)) return -1;
	if(pitch < 0.5f) return 1 + (1<<8);
	if(pitch > 2.0f) return 1 + (2<<8);

	// Get sample struct pointer
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
	if(!pSmp) return 2;

	// Get number of channels & sample size
	BYTE smpsize = pSmp->GetElementarySampleSize();
	UINT nChn = pSmp->GetNumChannels();

	// Get selected oversampling - quality - (also refered as FFT overlapping) factor
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO5);
	short ovs = combo->GetCurSel() + 4;

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


	// PitchShift seems to work only with 16-bit samples.
	if(smpsize != 2)
	{
		// This has to be converted to 16-bit first.
		OnUpsample();
		smpsize = pSmp->GetElementarySampleSize();
	}

	// Get original sample rate
	long lSampleRate = pSmp->GetSampleRate(m_pSndFile->GetType());

	// Deduce max sample value (float conversion step)
	float maxSampleValue = ( 1 << (smpsize * 8 - 1) ) - 1;

	// Allocate working buffers
	float * buffer = new float[MAX_BUFFER_LENGTH + fft];
	float * outbuf = new float[MAX_BUFFER_LENGTH + fft];

	m_pModDoc->GetSampleUndo()->PrepareUndo(m_nSample, sundo_replace);

	// Process each channel separately
	for(UINT i = 0 ; i < nChn ; i++){

		UINT pos = 0;
		UINT len = MAX_BUFFER_LENGTH;

		// Process sample buffer using MAX_BUFFER_LENGTH (max) sized chunk steps (in order to allow
		// the processing of BIG samples...)
		while(pos < pSmp->nLength){

			// Current chunk size limit test
			if(pos + len >= pSmp->nLength) len = pSmp->nLength - pos;

			// TRICK : output buffer offset management
			// as the pitch-shifter adds  some blank signal in head of output  buffer (matching FFT
			// length - in short it needs a certain amount of data before being able to output some
			// meaningfull  processed samples) , in order  to avoid this behaviour , we will ignore  
			// the  first FFT_length  samples and process  the same  amount of extra  blank samples
			// (all 0.0f) at the end of the buffer (those extra samples will benefit from  internal
			// FFT data  computed during the previous  steps resulting in a  correct and consistent
			// signal output).
			bool bufstart = ( pos == 0 );
			bool bufend   = ( pos + MAX_BUFFER_LENGTH >= pSmp->nLength );
			UINT startoffset = ( bufstart ? fft : 0 );
			UINT inneroffset = ( bufstart ? 0 : fft );
			UINT finaloffset = ( bufend   ? fft : 0 );

			// Show progress bar using process button painting & text label
			CHAR progress[16];
			float percent = (float)i * 50.0f + (100.0f / nChn) * (pos + len) / pSmp->nLength;
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
			if(bufstart){
				for(UINT j = 0 ; j < fft ; j++) buffer[j] = 0.0f;
				smbPitchShift(pitch, fft, fft, ovs, (float)lSampleRate, buffer, outbuf, true);
			}

			// Convert current channel's data chunk to float
			BYTE * ptr = (BYTE *)pSmp->pSample + pos * smpsize * nChn + i * smpsize;

			for(UINT j = 0 ; j < len ; j++){
				switch(smpsize){
					case 2:
						buffer[j] = ((float)(*(SHORT *)ptr)) / maxSampleValue;
						break;
					case 1:
						buffer[j] = ((float)*ptr) / maxSampleValue;
						break;
				}
				ptr += smpsize * nChn;
			}

			// Fills extra blank samples (read TRICK description comment above)
			if(bufend) for(UINT j = len ; j < len + finaloffset ; j++) buffer[j] = 0.0f;

			// Apply pitch shifting
			smbPitchShift(pitch, len + finaloffset, fft, ovs, (float)lSampleRate, buffer, outbuf, false);

			// Restore pitched-shifted float sample into original sample buffer
			ptr = (BYTE *)pSmp->pSample + (pos - inneroffset) * smpsize * nChn + i * smpsize;

			for(UINT j = startoffset ; j < len + finaloffset ; j++){
				// Just perform a little bit of clipping...
				float v = outbuf[j]; CLIP_SOUND(v);
				// ...before converting back to buffer
				switch(smpsize){
					case 2:
						*(SHORT *)ptr = (SHORT)(v * maxSampleValue);
						break;
					case 1:
						*ptr = (BYTE)(v * maxSampleValue);
						break;
				}
				ptr += smpsize * nChn;
			}

			// Next buffer chunk
			pos += MAX_BUFFER_LENGTH;
		}
	}

	// Free working buffers
	delete [] buffer;
	delete [] outbuf;

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
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];

	SELECTIONPOINTS selection = GetSelectionPoints();

	m_pModDoc->GetSampleUndo()->PrepareUndo(m_nSample, sundo_reverse, selection.nStart, selection.nEnd);
	if(ctrlSmp::ReverseSample(pSmp, selection.nStart, selection.nEnd, m_pSndFile) == true)
	{
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, NULL);
		m_pModDoc->SetModified();
	} else
	{
		m_pModDoc->GetSampleUndo()->RemoveLastUndoStep(m_nSample);
	}
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnInvert()
//---------------------------
{
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];

	SELECTIONPOINTS selection = GetSelectionPoints();

	m_pModDoc->GetSampleUndo()->PrepareUndo(m_nSample, sundo_invert, selection.nStart, selection.nEnd);
	if(ctrlSmp::InvertSample(pSmp, selection.nStart, selection.nEnd, m_pSndFile) == true)
	{
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, NULL);
		m_pModDoc->SetModified();
	} else
	{
		m_pModDoc->GetSampleUndo()->RemoveLastUndoStep(m_nSample);
	}
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnSignUnSign()
//-------------------------------
{
	MODSAMPLE *pSmp;
	
	if ((!m_pModDoc) || (!m_pSndFile) || (!m_pSndFile->Samples[m_nSample].pSample)) return;

	if(m_pModDoc->IsNotePlaying(0, m_nSample, 0) == TRUE)
		MsgBoxHidable(ConfirmSignUnsignWhenPlaying);

	BeginWaitCursor();
	pSmp = &m_pSndFile->Samples[m_nSample];
	SELECTIONPOINTS selection = GetSelectionPoints();

	m_pModDoc->GetSampleUndo()->PrepareUndo(m_nSample, sundo_unsign, selection.nStart, selection.nEnd);
	if(ctrlSmp::UnsignSample(pSmp, selection.nStart, selection.nEnd, m_pSndFile) == true)
	{
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, NULL);
		m_pModDoc->SetModified();
	} else
	{
		m_pModDoc->GetSampleUndo()->RemoveLastUndoStep(m_nSample);
	}
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnSilence()
//----------------------------
{
	MODSAMPLE *pSmp;
	
	if ((!m_pSndFile) || (!m_pSndFile->Samples[m_nSample].pSample)) return;
	BeginWaitCursor();
	SELECTIONPOINTS selection = GetSelectionPoints();

	// never apply silence to a sample that has no selection
	if(selection.bSelected == true)
	{
		pSmp = &m_pSndFile->Samples[m_nSample];
		m_pModDoc->GetSampleUndo()->PrepareUndo(m_nSample, sundo_update, selection.nStart, selection.nEnd);

		int len = selection.nEnd - selection.nStart;
		if (pSmp->uFlags & CHN_STEREO)
		{
			int smplsize = (pSmp->uFlags & CHN_16BIT) ? 4 : 2;
			signed char *p = ((signed char *)pSmp->pSample) + selection.nStart * smplsize;
			memset(p, 0, len*smplsize);
		} else
			if (pSmp->uFlags & CHN_16BIT)
			{
				short int *p = ((short int *)pSmp->pSample) + selection.nStart;
				int dest = (selection.nEnd < pSmp->nLength) ? p[len-1] : 0;
				int base = (selection.nStart) ? p[0] : 0;
				int delta = dest - base;
				for (int i=0; i<len; i++)
				{
					int n = base + (int)(((LONGLONG)delta * (LONGLONG)i) / (len-1));
					p[i] = (signed short)n;
				}
			} else
			{
				signed char *p = ((signed char *)pSmp->pSample) + selection.nStart;
				int dest = (selection.nEnd < pSmp->nLength) ? p[len-1] : 0;
				int base = (selection.nStart) ? p[0] : 0;
				int delta = dest - base;
				for (int i=0; i<len; i++)
				{
					int n = base + (delta * i) / (len-1);
					p[i] = (signed char)n;
				}
			}
			m_pModDoc->AdjustEndOfSample(m_nSample);
			// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
			m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, NULL);
			m_pModDoc->SetModified();
	}

	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnPrevInstrument()
//-----------------------------------
{
	if (m_pSndFile)
	{
		if (m_nSample > 1)
			SetCurrentSample(m_nSample-1);
		else
			SetCurrentSample(m_pSndFile->m_nSamples);
	}
}


void CCtrlSamples::OnNextInstrument()
//-----------------------------------
{
	if (m_pSndFile)
	{
		if (m_nSample < m_pSndFile->m_nSamples)
			SetCurrentSample(m_nSample+1);
		else
			SetCurrentSample(1);
	}
}


void CCtrlSamples::OnNameChanged()
//--------------------------------
{
	CHAR s[64];

	if ((IsLocked()) || (!m_pSndFile) || (!m_nSample)) return;
	s[0] = 0;
	m_EditName.GetWindowText(s, sizeof(s));
	for (UINT i=strlen(s); i<32; i++) s[i] = 0;
	s[31] = 0;
	if (strncmp(s, m_pSndFile->m_szNames[m_nSample], 32))
	{
		memcpy(m_pSndFile->m_szNames[m_nSample], s, 32);
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | (HINT_SMPNAMES|HINT_SAMPLEINFO), this);
		m_pModDoc->UpdateAllViews(NULL, HINT_INSNAMES, this);
		m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnFileNameChanged()
//------------------------------------
{
	CHAR s[32];
	
	if ((IsLocked()) || (!m_pSndFile)) return;
	s[0] = 0;
	m_EditFileName.GetWindowText(s, sizeof(s));
	s[21] = 0;
	for (UINT i=strlen(s); i<22; i++) s[i] = 0;
	if (strncmp(s, m_pSndFile->Samples[m_nSample].filename, 22))
	{
		memcpy(m_pSndFile->Samples[m_nSample].filename, s, 22);
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEINFO, this);
		if (m_pSndFile->m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)) m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnVolumeChanged()
//----------------------------------
{
	if (IsLocked()) return;
	int nVol = GetDlgItemInt(IDC_EDIT7);
	if (nVol < 0) nVol = 0;
	if (nVol > 64) nVol = 64;
	nVol <<= 2;
	if (nVol != m_pSndFile->Samples[m_nSample].nVolume)
	{
		m_pSndFile->Samples[m_nSample].nVolume = nVol;
		m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnGlobalVolChanged()
//-------------------------------------
{
	if (IsLocked()) return;
	int nVol = GetDlgItemInt(IDC_EDIT8);
	if (nVol < 0) nVol = 0;
	if (nVol > 64) nVol = 64;
	if (nVol != m_pSndFile->Samples[m_nSample].nGlobalVol)
	{
		m_pSndFile->Samples[m_nSample].nGlobalVol = nVol;
		m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnSetPanningChanged()
//--------------------------------------
{
	if (IsLocked()) return;
	BOOL b = FALSE;
	if (m_pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))
	{
		b = IsDlgButtonChecked(IDC_CHECK1);
	}
	if (b)
	{
		if (!(m_pSndFile->Samples[m_nSample].uFlags & CHN_PANNING))
		{
			m_pSndFile->Samples[m_nSample].uFlags |= CHN_PANNING;
			m_pModDoc->SetModified();
		}
	} else
	{
		if (m_pSndFile->Samples[m_nSample].uFlags & CHN_PANNING)
		{
			m_pSndFile->Samples[m_nSample].uFlags &= ~CHN_PANNING;
			m_pModDoc->SetModified();
		}
	}
}


void CCtrlSamples::OnPanningChanged()
//-----------------------------------
{
	if (IsLocked()) return;
	int nPan = GetDlgItemInt(IDC_EDIT9);
	if (nPan < 0) nPan = 0;
	//rewbs.fix36944: sample pan range to 0-64.
	if (m_pSndFile->m_nType == MOD_TYPE_XM) {
		if (nPan>255) nPan = 255;	// displayed panning will be 0-255 with XM
	} else {
		if (nPan>64) nPan = 64;		// displayed panning will be 0-64 with anything but XM.
		nPan = nPan << 2;			// so we x4 to get MPT's internal 0-256 range.
	}
	//end rewbs.fix36944
	if (nPan != m_pSndFile->Samples[m_nSample].nPan)
	{
		m_pSndFile->Samples[m_nSample].nPan = nPan;
		if (m_pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnFineTuneChanged()
//------------------------------------
{
	if (IsLocked()) return;
	int n = GetDlgItemInt(IDC_EDIT5);
	if (m_pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_S3M|MOD_TYPE_MPT))
	{
		if ((n > 0) && (n <= (m_pSndFile->m_nType & MOD_TYPE_S3M) ? 65535 : 9999999) && (n != (int)m_pSndFile->Samples[m_nSample].nC5Speed))
		{
			m_pSndFile->Samples[m_nSample].nC5Speed = n;
			int transp = CSoundFile::FrequencyToTranspose(n) >> 7;
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
			m_pModDoc->SetModified();
		}
	} else
	{
		if ((n >= -128) && (n <= 127)) {
			m_pSndFile->Samples[m_nSample].nFineTune = n;
			m_pModDoc->SetModified();
		}

	}
}


void CCtrlSamples::OnBaseNoteChanged()
//-------------------------------------
{
	if (IsLocked()) return;
	int n = 60 - (m_CbnBaseNote.GetCurSel() + BASENOTE_MIN);
	if (m_pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_S3M|MOD_TYPE_MPT))
	{
		LONG ft = CSoundFile::FrequencyToTranspose(m_pSndFile->Samples[m_nSample].nC5Speed) & 0x7f;
		n = CSoundFile::TransposeToFrequency(n, ft);
		if ((n > 0) && (n <= (m_pSndFile->m_nType & MOD_TYPE_S3M) ? 65535 : 9999999) && (n != (int)m_pSndFile->Samples[m_nSample].nC5Speed))
		{
			CHAR s[32];
			m_pSndFile->Samples[m_nSample].nC5Speed = n;
			wsprintf(s, "%lu", n);
			LockControls();
			m_EditFineTune.SetWindowText(s);
			UnlockControls();
			m_pModDoc->SetModified();
		}
	} else
	{
		if ((n >= -128) && (n < 128)) {
			m_pSndFile->Samples[m_nSample].RelativeTone = n;
			m_pModDoc->SetModified();
		}
	}
}


void CCtrlSamples::OnVibTypeChanged()
//-----------------------------------
{
	if (IsLocked()) return;
	int n = m_ComboAutoVib.GetCurSel();
	if (n >= 0) m_pSndFile->Samples[m_nSample].nVibType = (BYTE)n;
	m_pModDoc->SetModified();
}


void CCtrlSamples::OnVibDepthChanged()
//------------------------------------
{
	if (IsLocked()) return;
	int lmin = 0, lmax = 0;
	m_SpinVibDepth.GetRange(lmin, lmax);
	int n = GetDlgItemInt(IDC_EDIT15);
	if ((n >= lmin) && (n <= lmax)) {
		m_pSndFile->Samples[m_nSample].nVibDepth = n;
		m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnVibSweepChanged()
//------------------------------------
{
	if (IsLocked()) return;
	int lmin = 0, lmax = 0;
	m_SpinVibSweep.GetRange(lmin, lmax);
	int n = GetDlgItemInt(IDC_EDIT14);
	if ((n >= lmin) && (n <= lmax)) {
		m_pSndFile->Samples[m_nSample].nVibSweep = n;
		m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnVibRateChanged()
//-----------------------------------
{
	if (IsLocked()) return;
	int lmin = 0, lmax = 0;
	m_SpinVibRate.GetRange(lmin, lmax);
	int n = GetDlgItemInt(IDC_EDIT16);
	if ((n >= lmin) && (n <= lmax)) {
		m_pSndFile->Samples[m_nSample].nVibRate = n;
		m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnLoopTypeChanged()
//------------------------------------
{
	if ((IsLocked()) || (!m_pSndFile)) return;
	int n = m_ComboLoopType.GetCurSel();
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
	switch(n)
	{
	case 0:	// Off
		pSmp->uFlags &= ~(CHN_LOOP|CHN_PINGPONGLOOP|CHN_PINGPONGFLAG);
		break;
	case 1:	// On
		pSmp->uFlags &= ~CHN_PINGPONGLOOP;
		pSmp->uFlags |= CHN_LOOP;
		break;
	case 2:	// PingPong
		pSmp->uFlags |= CHN_LOOP|CHN_PINGPONGLOOP;
		break;
	}
	m_pModDoc->AdjustEndOfSample(m_nSample);
	m_pModDoc->SetModified();
}


void CCtrlSamples::OnLoopStartChanged()
//--------------------------------------
{
	if ((IsLocked()) || (!m_pSndFile)) return;
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
	LONG n = GetDlgItemInt(IDC_EDIT1);
	if ((n >= 0) && (n < (LONG)pSmp->nLength) && ((n < (LONG)pSmp->nLoopEnd) || (!(pSmp->uFlags & CHN_LOOP))))
	{
		pSmp->nLoopStart = n;
		if(pSmp->uFlags & CHN_LOOP) 
		{
			/* only update sample buffer if the loop is actually enabled
			  (resets sound without any reason otherwise) - bug report 1874 */
			m_pModDoc->AdjustEndOfSample(m_nSample);
		}
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, this);
		m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnLoopEndChanged()
//------------------------------------
{
	if ((IsLocked()) || (!m_pSndFile)) return;
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
	LONG n = GetDlgItemInt(IDC_EDIT2);
	if ((n >= 0) && (n <= (LONG)pSmp->nLength) && ((n > (LONG)pSmp->nLoopStart) || (!(pSmp->uFlags & CHN_LOOP))))
	{
		pSmp->nLoopEnd = n;
		if(pSmp->uFlags & CHN_LOOP)
		{
			/* only update sample buffer if the loop is actually enabled
			  (resets sound without any reason otherwise) - bug report 1874 */
			m_pModDoc->AdjustEndOfSample(m_nSample);
		}
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, this);
		m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnSustainTypeChanged()
//---------------------------------------
{
	if ((IsLocked()) || (!m_pSndFile)) return;
	int n = m_ComboSustainType.GetCurSel();
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
	switch(n)
	{
	case 0:	// Off
		pSmp->uFlags &= ~(CHN_SUSTAINLOOP|CHN_PINGPONGSUSTAIN|CHN_PINGPONGFLAG);
		break;
	case 1:	// On
		pSmp->uFlags &= ~CHN_PINGPONGSUSTAIN;
		pSmp->uFlags |= CHN_SUSTAINLOOP;
		break;
	case 2:	// PingPong
		pSmp->uFlags |= CHN_SUSTAINLOOP|CHN_PINGPONGSUSTAIN;
		break;
	}
	m_pModDoc->SetModified();
}


void CCtrlSamples::OnSustainStartChanged()
//----------------------------------------
{
	if ((IsLocked()) || (!m_pSndFile)) return;
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
	LONG n = GetDlgItemInt(IDC_EDIT3);
	if ((n >= 0) && (n <= (LONG)pSmp->nLength)
	 && ((n < (LONG)pSmp->nSustainEnd) || (!(pSmp->uFlags & CHN_SUSTAINLOOP))))
	{
		pSmp->nSustainStart = n;
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, this);
		m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnSustainEndChanged()
//--------------------------------------
{
	if ((IsLocked()) || (!m_pSndFile)) return;
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];
	LONG n = GetDlgItemInt(IDC_EDIT4);
	if ((n >= 0) && (n <= (LONG)pSmp->nLength)
	 && ((n > (LONG)pSmp->nSustainStart) || (!(pSmp->uFlags & CHN_SUSTAINLOOP))))
	{
		pSmp->nSustainEnd = n;
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, this);
		m_pModDoc->SetModified();
	}
}


#define SMPLOOP_ACCURACY	7	// 5%
#define BIDILOOP_ACCURACY	2	// 5%


BOOL MPT_LoopCheck(int sstart0, int sstart1, int send0, int send1)
{
	int dse0 = send0 - sstart0;
	if ((dse0 < -SMPLOOP_ACCURACY) || (dse0 > SMPLOOP_ACCURACY)) return FALSE;
	int dse1 = send1 - sstart1;
	if ((dse1 < -SMPLOOP_ACCURACY) || (dse1 > SMPLOOP_ACCURACY)) return FALSE;
	int dstart = sstart1 - sstart0;
	int dend = send1 - send0;
	if (!dstart) dstart = dend >> 7;
	if (!dend) dend = dstart >> 7;
	if ((dstart ^ dend) < 0) return FALSE;
	int delta = dend - dstart;
	return ((delta > -SMPLOOP_ACCURACY) && (delta < SMPLOOP_ACCURACY)) ? TRUE : FALSE;
}


BOOL MPT_BidiEndCheck(int spos0, int spos1, int spos2)
{
	int delta0 = spos1 - spos0;
	int delta1 = spos2 - spos1;
	int delta2 = spos2 - spos0;
	if (!delta0) delta0 = delta1 >> 7;
	if (!delta1) delta1 = delta0 >> 7;
	if ((delta1 ^ delta0) < 0) return FALSE;
	return ((delta0 >= -1) && (delta0 <= 0) && (delta1 >= -1) && (delta1 <= 0) && (delta2 >= -1) && (delta2 <= 0));
}


BOOL MPT_BidiStartCheck(int spos0, int spos1, int spos2)
{
	int delta1 = spos1 - spos0;
	int delta0 = spos2 - spos1;
	int delta2 = spos2 - spos0;
	if (!delta0) delta0 = delta1 >> 7;
	if (!delta1) delta1 = delta0 >> 7;
	if ((delta1 ^ delta0) < 0) return FALSE;
	return ((delta0 >= -1) && (delta0 <= 0) && (delta1 > -1) && (delta1 <= 0) && (delta2 >= -1) && (delta2 <= 0));
}



void CCtrlSamples::OnVScroll(UINT nCode, UINT, CScrollBar *)
//----------------------------------------------------------
{
	CHAR s[256];
	if ((IsLocked()) || (!m_pSndFile)) return;
	UINT nsample = m_nSample, pinc = 1;
	MODSAMPLE *pSmp = &m_pSndFile->Samples[nsample];
	LPSTR pSample = pSmp->pSample;
	short int pos;
	BOOL bRedraw = FALSE;
	
	LockControls();
	if ((!pSmp->nLength) || (!pSample)) goto NoSample;
	if (pSmp->uFlags & CHN_16BIT)
	{
		pSample++;
		pinc *= 2;
	}
	if (pSmp->uFlags & CHN_STEREO) pinc *= 2;
	// Loop Start
	if ((pos = (short int)m_SpinLoopStart.GetPos()) != 0)
	{
		BOOL bOk = FALSE;
		LPSTR p = pSample+pSmp->nLoopStart*pinc;
		int find0 = (int)pSample[pSmp->nLoopEnd*pinc-pinc];
		int find1 = (int)pSample[pSmp->nLoopEnd*pinc];
		// Find Next LoopStart Point
		if (pos > 0)
		{
			for (UINT i=pSmp->nLoopStart+1; i+16<pSmp->nLoopEnd; i++)
			{
				p += pinc;
				bOk = (pSmp->uFlags & CHN_PINGPONGLOOP) ? MPT_BidiStartCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pSmp->nLoopStart = i;
					break;
				}
			}
		} else
		// Find Prev LoopStart Point
		{
			for (UINT i=pSmp->nLoopStart; i; )
			{
				i--;
				p -= pinc;
				bOk = (pSmp->uFlags & CHN_PINGPONGLOOP) ? MPT_BidiStartCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pSmp->nLoopStart = i;
					break;
				}
			}
		}
		if (bOk)
		{
			wsprintf(s, "%u", pSmp->nLoopStart);
			m_EditLoopStart.SetWindowText(s);
			m_pModDoc->AdjustEndOfSample(m_nSample);
			bRedraw = TRUE;
		}
		m_SpinLoopStart.SetPos(0);
	}
	// Loop End
	pos = (short int)m_SpinLoopEnd.GetPos();
	if ((pos) && (pSmp->nLoopEnd))
	{
		BOOL bOk = FALSE;
		LPSTR p = pSample+pSmp->nLoopEnd*pinc;
		int find0 = (int)pSample[pSmp->nLoopStart*pinc];
		int find1 = (int)pSample[pSmp->nLoopStart*pinc+pinc];
		// Find Next LoopEnd Point
		if (pos > 0)
		{
			for (UINT i=pSmp->nLoopEnd+1; i<=pSmp->nLength; i++, p+=pinc)
			{
				bOk = (pSmp->uFlags & CHN_PINGPONGLOOP) ? MPT_BidiEndCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pSmp->nLoopEnd = i;
					break;
				}
			}
		} else
		// Find Prev LoopEnd Point
		{
			for (UINT i=pSmp->nLoopEnd; i>pSmp->nLoopStart+16; )
			{
				i--;
				p -= pinc;
				bOk = (pSmp->uFlags & CHN_PINGPONGLOOP) ? MPT_BidiEndCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pSmp->nLoopEnd = i;
					break;
				}
			}
		}
		if (bOk)
		{
			wsprintf(s, "%u", pSmp->nLoopEnd);
			m_EditLoopEnd.SetWindowText(s);
			m_pModDoc->AdjustEndOfSample(m_nSample);
			bRedraw = TRUE;
		}
		m_SpinLoopEnd.SetPos(0);
	}
	// Sustain Loop Start
	pos = (short int)m_SpinSustainStart.GetPos();
	if ((pos) && (pSmp->nSustainEnd))
	{
		BOOL bOk = FALSE;
		LPSTR p = pSample+pSmp->nSustainStart*pinc;
		int find0 = (int)pSample[pSmp->nSustainEnd*pinc-pinc];
		int find1 = (int)pSample[pSmp->nSustainEnd*pinc];
		// Find Next Sustain LoopStart Point
		if (pos > 0)
		{
			for (UINT i=pSmp->nSustainStart+1; i+16<pSmp->nSustainEnd; i++)
			{
				p += pinc;
				bOk = (pSmp->uFlags & CHN_PINGPONGSUSTAIN) ? MPT_BidiStartCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pSmp->nSustainStart = i;
					break;
				}
			}
		} else
		// Find Prev Sustain LoopStart Point
		{
			for (UINT i=pSmp->nSustainStart; i; )
			{
				i--;
				p -= pinc;
				bOk = (pSmp->uFlags & CHN_PINGPONGSUSTAIN) ? MPT_BidiStartCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pSmp->nSustainStart = i;
					break;
				}
			}
		}
		if (bOk)
		{
			wsprintf(s, "%u", pSmp->nSustainStart);
			m_EditSustainStart.SetWindowText(s);
			bRedraw = TRUE;
		}
		m_SpinSustainStart.SetPos(0);
	}
	// Sustain Loop End
	pos = (short int)m_SpinSustainEnd.GetPos();
	if (pos)
	{
		BOOL bOk = FALSE;
		LPSTR p = pSample+pSmp->nSustainEnd*pinc;
		int find0 = (int)pSample[pSmp->nSustainStart*pinc];
		int find1 = (int)pSample[pSmp->nSustainStart*pinc+pinc];
		// Find Next LoopEnd Point
		if (pos > 0)
		{
			for (UINT i=pSmp->nSustainEnd+1; i+1<pSmp->nLength; i++, p+=pinc)
			{
				bOk = (pSmp->uFlags & CHN_PINGPONGSUSTAIN) ? MPT_BidiEndCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pSmp->nSustainEnd = i;
					break;
				}
			}
		} else
		// Find Prev LoopEnd Point
		{
			for (UINT i=pSmp->nSustainEnd; i>pSmp->nSustainStart+16; )
			{
				i--;
				p -= pinc;
				bOk = (pSmp->uFlags & CHN_PINGPONGSUSTAIN) ? MPT_BidiEndCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pSmp->nSustainEnd = i;
					break;
				}
			}
		}
		if (bOk)
		{
			wsprintf(s, "%u", pSmp->nSustainEnd);
			m_EditSustainEnd.SetWindowText(s);
			bRedraw = TRUE;
		}
		m_SpinSustainEnd.SetPos(0);
	}
NoSample:
	// FineTune / C-5 Speed
	if ((pos = (short int)m_SpinFineTune.GetPos()) != 0)
	{
		if (m_pSndFile->m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			UINT d = pSmp->nC5Speed;
			if (d < 1) d = 8363;
			if(d < m_nFinetuneStep) d = m_nFinetuneStep;
			d += (pos * m_nFinetuneStep);
			pSmp->nC5Speed = CLAMP(d, 1, 9999999); // 9999999 is max. in Impulse Tracker
			int transp = CSoundFile::FrequencyToTranspose(pSmp->nC5Speed) >> 7;
			int basenote = 60 - transp;
			if (basenote < BASENOTE_MIN) basenote = BASENOTE_MIN;
			if (basenote >= BASENOTE_MAX) basenote = BASENOTE_MAX-1;
			basenote -= BASENOTE_MIN;
			if (basenote != m_CbnBaseNote.GetCurSel()) m_CbnBaseNote.SetCurSel(basenote);
			wsprintf(s, "%lu", pSmp->nC5Speed);
			m_EditFineTune.SetWindowText(s);
		} else
		{
			LONG d = pSmp->nFineTune + pos;
			if (d < -128) d = -128;
			if (d > 127) d = 127;
			pSmp->nFineTune = d;
			wsprintf(s, "%d", d);
			m_EditFineTune.SetWindowText(s);
		}
		m_SpinFineTune.SetPos(0);
	}
	if ((nCode == SB_ENDSCROLL) || (nCode == SB_THUMBPOSITION)) SwitchToView();
	if (bRedraw) {
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, this);
		m_pModDoc->SetModified();
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
			UINT nChar = pMsg->wParam;
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);
			InputTargetContext ctx = (InputTargetContext)(kCtxCtrlSamples);
			
			if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT) != kcNull)
				return true; // Mapped to a command, no need to pass message on.
		}

	}
	
	return CModControlDlg::PreTranslateMessage(pMsg);
}

LRESULT CCtrlSamples::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
//----------------------------------------------------------------
{
	if (wParam == kcNull)
		return NULL;
	
	switch(wParam)
	{
		case kcSampleCtrlLoad: OnSampleOpen(); return wParam;
		case kcSampleCtrlSave: OnSampleSave(); return wParam;
		case kcSampleCtrlNew:  OnSampleNew();  return wParam;
	}
	
	return 0;
}
//end rewbs.customKeys


// Return currently selected part of the sample.
// The whole sample size will be returned if no part of the sample is selected.
// However, point.bSelected indicates whether a sample selection exists or not.
SELECTIONPOINTS CCtrlSamples::GetSelectionPoints()
//------------------------------------------------
{
	SELECTIONPOINTS points;
	SAMPLEVIEWSTATE viewstate;
	MODSAMPLE *pSmp = &m_pSndFile->Samples[m_nSample];

	memset(&viewstate, 0, sizeof(viewstate));
	SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&viewstate);
	points.nStart = viewstate.dwBeginSel;
	points.nEnd = viewstate.dwEndSel;
	if(points.nEnd > pSmp->nLength) points.nEnd = pSmp->nLength;
	if(points.nStart > points.nEnd) points.nStart = points.nEnd;
	points.bSelected = true;
	if(points.nStart >= points.nEnd)
	{
		points.nStart = 0;
		points.nEnd = pSmp->nLength;
		points.bSelected = false;
	}
	return points;
}

