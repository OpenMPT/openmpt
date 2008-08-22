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

// -> CODE#0029
// -> DESC="pitch shifting - time stretching"
#include "smbPitchShift.h"
#include "samplerate.h"

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
	ON_COMMAND(IDC_CHECK1,				OnSetPanningChanged)
	ON_COMMAND(ID_PREVINSTRUMENT,		OnPrevInstrument)
	ON_COMMAND(ID_NEXTINSTRUMENT,		OnNextInstrument)

// -> CODE#0029
// -> DESC="pitch shifting - time stretching"
	ON_COMMAND(IDC_BUTTON1,				OnPitchShiftTimeStretch)
	ON_COMMAND(IDC_BUTTON2,				OnEstimateSampleSize)
	ON_COMMAND(IDC_BUTTON3,				OnPitchShiftTimeStretchAccept)
	ON_COMMAND(IDC_BUTTON4,				OnPitchShiftTimeStretchCancel)
	ON_COMMAND(IDC_CHECK3,				OnEnableStretchToSize)
// -! TEST#0029

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

// -> CODE#0029
// -> DESC="pitch shifting - time stretching"
	DDX_Control(pDX, IDC_COMBO4,				m_ComboPitch);
	DDX_Control(pDX, IDC_COMBO5,				m_ComboQuality);
	DDX_Control(pDX, IDC_COMBO6,				m_ComboFFT);
	DDX_Text(pDX,	 IDC_EDIT6,					m_dTimeStretchRatio); //rewbs.timeStretchMods
// -! TEST#0029

	//}}AFX_DATA_MAP
}


CCtrlSamples::CCtrlSamples()
//--------------------------
{
	m_nSample = 1;
	m_nLockCount = 1;

// -> CODE#0029
// -> DESC="pitch shifting - time stretching"
	pSampleUndoBuffer = NULL;
	UndoBufferSize = 0;
// -! TEST#0029
}


// -> CODE#0029
// -> DESC="pitch shifting - time stretching"
CCtrlSamples::~CCtrlSamples()
{
	if(pSampleUndoBuffer) CSoundFile::FreeSample(pSampleUndoBuffer);
	pSampleUndoBuffer = NULL;
	UndoBufferSize = 0;
}
// -! TEST#0029


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
	m_ToolBar1.AddButton(IDC_SAMPLE_NEW, 6);
	m_ToolBar1.AddButton(IDC_SAMPLE_OPEN, 12);
	m_ToolBar1.AddButton(IDC_SAMPLE_SAVEAS, 13);
	// Edit ToolBar
	m_ToolBar2.Init();
	m_ToolBar2.AddButton(IDC_SAMPLE_PLAY, 14);
	m_ToolBar2.AddButton(IDC_SAMPLE_NORMALIZE, 8);
	m_ToolBar2.AddButton(IDC_SAMPLE_AMPLIFY, 9);
	m_ToolBar2.AddButton(IDC_SAMPLE_UPSAMPLE, 10);
	m_ToolBar2.AddButton(IDC_SAMPLE_DOWNSAMPLE, 29);
	m_ToolBar2.AddButton(IDC_SAMPLE_REVERSE, 11);
	m_ToolBar2.AddButton(IDC_SAMPLE_SILENCE, 22);
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
	m_ComboAutoVib.AddString("Sine");
	m_ComboAutoVib.AddString("Square");
	m_ComboAutoVib.AddString("Ramp Up");
	m_ComboAutoVib.AddString("Ramp Down");
	m_ComboAutoVib.AddString("Random");
	m_SpinVibSweep.SetRange(0, 64);
	m_SpinVibDepth.SetRange(0, 64);
	m_SpinVibRate.SetRange(0, 64);

	AppendNotesToControl(m_CbnBaseNote, BASENOTE_MIN, BASENOTE_MAX-1);

// -> CODE#0029
// -> DESC="pitch shifting - time stretching"
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

	//Disable preview buttons
	((CButton *)GetDlgItem(IDC_BUTTON3))->ShowWindow(SW_HIDE);
	((CButton *)GetDlgItem(IDC_BUTTON4))->ShowWindow(SW_HIDE);

	// Stretch ratio
	SetDlgItemInt(IDC_EDIT6,100,FALSE);

	// Processing state text label
	SetDlgItemText(IDC_STATIC1,"");

	// Stretch to size check box
	OnEnableStretchToSize();
// -! TEST#0029

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

// -> CODE#0029
// -> DESC="pitch shifting - time stretching"
	if(pSampleUndoBuffer) OnPitchShiftTimeStretchCancel();
// -! TEST#0029

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
				MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
				UINT nFreqHz = CSoundFile::TransposeToFrequency(pins->RelativeTone, pins->nFineTune);
				wsprintf(pszText, "%ldHz", nFreqHz);
				return TRUE;
			}
			break;
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
	}
	// Updating Values
	if (dwHintMask & (HINT_MODTYPE|HINT_SAMPLEINFO))
	{
		MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
		CHAR s[128];
		DWORD d;
		
		// Length / Type
		wsprintf(s, "%d-bit %s, len: %d", (pins->uFlags & CHN_16BIT) ? 16 : 8, (pins->uFlags & CHN_STEREO) ? "stereo" : "mono", pins->nLength);
		SetDlgItemText(IDC_TEXT5, s);
		// Name
		memcpy(s, m_pSndFile->m_szNames[m_nSample], 32);
		s[31] = 0;
		SetDlgItemText(IDC_SAMPLE_NAME, s);
		// File Name
		memcpy(s, pins->name, 22);
		s[21] = 0;
		if (m_pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)) s[0] = 0;
		SetDlgItemText(IDC_SAMPLE_FILENAME, s);
		// Volume
		SetDlgItemInt(IDC_EDIT7, pins->nVolume >> 2);
		// Global Volume
		SetDlgItemInt(IDC_EDIT8, pins->nGlobalVol);
		// Panning
		CheckDlgButton(IDC_CHECK1, (pins->uFlags & CHN_PANNING) ? MF_CHECKED : MF_UNCHECKED);
		//rewbs.fix36944
		if (m_pSndFile->m_nType == MOD_TYPE_XM) {
			SetDlgItemInt(IDC_EDIT9, pins->nPan);		//displayed panning with XM is 0-256, just like MPT's internal engine
		} else {
			SetDlgItemInt(IDC_EDIT9, pins->nPan>>2);	//displayed panning with anything but XM is 0-64 so we divide by 4
		}
		//end rewbs.fix36944
		// FineTune / C-4 Speed / BaseNote
        int transp = 0;
		if (m_pSndFile->m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
		{
			wsprintf(s, "%lu", pins->nC4Speed);
			m_EditFineTune.SetWindowText(s);
			transp = CSoundFile::FrequencyToTranspose(pins->nC4Speed) >> 7;
		} else
		{
			SetDlgItemInt(IDC_EDIT5, (int)pins->nFineTune);
			transp = (int)pins->RelativeTone;
		}
		int basenote = 60 - transp;
		if (basenote < BASENOTE_MIN) basenote = BASENOTE_MIN;
		if (basenote >= BASENOTE_MAX) basenote = BASENOTE_MAX-1;
		basenote -= BASENOTE_MIN;
		if (basenote != m_CbnBaseNote.GetCurSel()) m_CbnBaseNote.SetCurSel(basenote);
		// AutoVibrato
		m_ComboAutoVib.SetCurSel(pins->nVibType);
		SetDlgItemInt(IDC_EDIT14, (UINT)pins->nVibSweep);
		SetDlgItemInt(IDC_EDIT15, (UINT)pins->nVibDepth);
		SetDlgItemInt(IDC_EDIT16, (UINT)pins->nVibRate);
		// Loop
		d = 0;
		if (pins->uFlags & CHN_LOOP) d = (pins->uFlags & CHN_PINGPONGLOOP) ? 2 : 1;
		m_ComboLoopType.SetCurSel(d);
		wsprintf(s, "%lu", pins->nLoopStart);
		m_EditLoopStart.SetWindowText(s);
		wsprintf(s, "%lu", pins->nLoopEnd);
		m_EditLoopEnd.SetWindowText(s);
		// Sustain Loop
		d = 0;
		if (pins->uFlags & CHN_SUSTAINLOOP) d = (pins->uFlags & CHN_PINGPONGSUSTAIN) ? 2 : 1;
		m_ComboSustainType.SetCurSel(d);
		wsprintf(s, "%lu", pins->nSustainStart);
		m_EditSustainStart.SetWindowText(s);
		wsprintf(s, "%lu", pins->nSustainEnd);
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

// -> CODE#0029
// -> DESC="pitch shifting - time stretching"
	if(pSampleUndoBuffer) OnPitchShiftTimeStretchCancel();
// -! TEST#0029

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
			MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
			BEGIN_CRITICAL();
			m_pSndFile->DestroySample(m_nSample);
			pins->nLength = len;
			pins->uFlags = RS_PCM8S;
			pins->nGlobalVol = 64;
			pins->nVolume = 256;
			pins->nPan = 128;
			pins->name[0] = 0;
			if (!pins->nC4Speed) pins->nC4Speed = 22050;
			if (dlg.m_nFormat & 1)
			{
				pins->nLength >>= 1;
				pins->uFlags |= CHN_16BIT;
				flags = RS_PCM16S;
			}
			if (!(dlg.m_nFormat & 2))
			{
				flags++;
			}
			// Interleaved Stereo Sample
			if (dlg.m_nFormat & 4)
			{
				pins->uFlags |= CHN_STEREO;
				pins->nLength >>= 1;
				flags |= 0x40|RSF_STEREO;
			}
			LPSTR p16 = (LPSTR)lpFile;
			DWORD l16 = len;
			if ((pins->uFlags & CHN_16BIT) && (len & 1))
			{
				p16++;
				l16--;
			}
			if (m_pSndFile->ReadSample(pins, flags, p16, l16))
			{
				bOk = TRUE;
			}
			END_CRITICAL();
		}
	}
	f.Unlock();
OpenError:
	f.Close();
	EndWaitCursor();
	if (bOk)
	{
		CHAR szPath[_MAX_PATH], szNewPath[_MAX_PATH];
		MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
		_splitpath(lpszFileName, szNewPath, szPath, szName, szExt);
		strcat(szNewPath, szPath);
		strcpy(CMainFrame::m_szCurSmpDir, szNewPath);
		if (!pins->name[0])
		{
			memset(szPath, 0, 32);
			strcpy(szPath, szName);
			if (m_pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
			{
				// MOD/XM
				strcat(szPath, szExt);
				szPath[31] = 0;
				memcpy(m_pSndFile->m_szNames[m_nSample], szPath, 32);
			} else
			{
				// S3M/IT
				szPath[31] = 0;
				if (!m_pSndFile->m_szNames[m_nSample][0]) memcpy(m_pSndFile->m_szNames[m_nSample], szPath, 32);
				if (strlen(szPath) < 9) strcat(szPath, szExt);
			}
			szPath[21] = 0;
			memcpy(pins->name, szPath, 22);
		}
		if ((m_pSndFile->m_nType & MOD_TYPE_XM) && (!(pins->uFlags & CHN_PANNING)))
		{
			pins->nPan = 128;
			pins->uFlags |= CHN_PANNING;
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

// -> CODE#0029
// -> DESC="pitch shifting - time stretching"
	if(pSampleUndoBuffer) OnPitchShiftTimeStretchCancel();
// -! TEST#0029

	BeginWaitCursor();
	BEGIN_CRITICAL();
	m_pSndFile->DestroySample(m_nSample);
	m_pSndFile->ReadSampleFromSong(m_nSample, pSndFile, nSample);
	MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	if ((m_pSndFile->m_nType & MOD_TYPE_XM) && (!(pins->uFlags & CHN_PANNING)))
	{
		pins->nPan = 128;
		pins->uFlags |= CHN_PANNING;
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

// -> CODE#0029
// -> DESC="pitch shifting - time stretching"
		if(pSampleUndoBuffer) OnPitchShiftTimeStretchCancel();
// -! TEST#0029

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
	if (smp > 0)
	{

// -> CODE#0029
// -> DESC="pitch shifting - time stretching"
		if(pSampleUndoBuffer) OnPitchShiftTimeStretchCancel();
// -! TEST#0029

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
	CFileDialog dlg(TRUE,
					NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT,
					"All Samples|*.wav;*.pat;*.s3i;*.smp;*.snd;*.raw;*.xi;*.aif;*.aiff;*.its;*.8sv;*.8svx;*.svx;*.pcm|"
					"Wave Files (*.wav)|*.wav|"
					"XI Samples (*.xi)|*.xi|"
					"Impulse Tracker Samples (*.its)|*.its|"
					"ScreamTracker Samples (*.s3i,*.smp)|*.s3i;*.smp|"
					"GF1 Patches (*.pat)|*.pat|"
					"AIFF Files (*.aiff;*.8svx)|*.aif;*.aiff;*.8sv;*.8svx;*.svx|"
					"Raw Samples (*.raw,*.snd,*.pcm)|*.raw;*.snd;*.pcm|"
					"All Files (*.*)|*.*||",
					this);
	if (CMainFrame::m_szCurSmpDir[0])
	{
		dlg.m_ofn.lpstrInitialDir = CMainFrame::m_szCurSmpDir;
	}
	dlg.m_ofn.nFilterIndex = nLastIndex;
	const size_t bufferSize = 2048; //Note: This is possibly the maximum buffer size in MFC 7(this note was written November 2006).
	vector<char> filenameBuffer(bufferSize, 0);
	dlg.GetOFN().lpstrFile = &filenameBuffer[0];
	dlg.GetOFN().nMaxFile = bufferSize;

	if (dlg.DoModal() != IDOK) return;

	nLastIndex = dlg.m_ofn.nFilterIndex;

	POSITION pos = dlg.GetStartPosition();
	size_t counter = 0;
	while(pos != NULL)
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

		if(!OpenSample(dlg.GetNextPathName(pos)))
			ErrorBox(IDS_ERR_FILEOPEN, this);

		counter++;
	}
	filenameBuffer.clear();
	SwitchToView();
}


void CCtrlSamples::OnSampleSave()
//-------------------------------
{
	CHAR szFileName[_MAX_PATH] = "";

	if ((!m_pSndFile) || (!m_nSample) || (!m_pSndFile->Ins[m_nSample].pSample))
	{
		SwitchToView();
		return;
	}
	if (m_pSndFile->m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT))
	{
		memcpy(szFileName, m_pSndFile->Ins[m_nSample].name, 22);
		szFileName[22] = 0;
	} else
	{
		memcpy(szFileName, m_pSndFile->m_szNames[m_nSample], 32);
		szFileName[32] = 0;
	}
	if (!szFileName[0]) strcpy(szFileName, "untitled");
	CFileDialog dlg(FALSE, "wav",
			szFileName,
			OFN_HIDEREADONLY| OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN,
			"Wave File (*.wav)|*.wav||",
			this);
	if (CMainFrame::m_szCurSmpDir[0])
	{
		dlg.m_ofn.lpstrInitialDir = CMainFrame::m_szCurSmpDir;
	}
	if (dlg.DoModal() != IDOK) return;
	BeginWaitCursor();
	BOOL bOk = m_pSndFile->SaveWAVSample(m_nSample, dlg.GetPathName());
	EndWaitCursor();
	if (!bOk)
	{
		ErrorBox(IDS_ERR_SAVESMP, this);
	} else
	{
		CHAR drive[_MAX_DRIVE], path[_MAX_PATH];
		_splitpath(dlg.GetPathName(), drive, path, NULL, NULL);
		strcpy(CMainFrame::m_szCurSmpDir, drive);
		strcat(CMainFrame::m_szCurSmpDir, path);
	}
	SwitchToView();
}


void CCtrlSamples::OnSamplePlay()
//-------------------------------
{
	if ((m_pModDoc) && (m_pSndFile))
	{
		if ((m_pSndFile->IsPaused()) && (m_pModDoc->IsNotePlaying(0, m_nSample, 0)))
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
	BeginWaitCursor();
	if ((m_pModDoc) && (m_pSndFile) && (m_pSndFile->Ins[m_nSample].pSample))
	{
		BOOL bOk = FALSE;
		MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	
		if (pins->uFlags & CHN_16BIT)
		{
			UINT len = pins->nLength;
			signed short *p = (signed short *)pins->pSample;
			if (pins->uFlags & CHN_STEREO) len *= 2;
			int max = 1;
			for (UINT i=0; i<len; i++)
			{
				if (p[i] > max) max = p[i];
				if (-p[i] > max) max = -p[i];
			}
			if (max < 32767)
			{
				max++;
				for (UINT j=0; j<len; j++)
				{
					int l = p[j];
					p[j] = (l << 15) / max;
				}
				bOk = TRUE;
			}
		} else
		{
			UINT len = pins->nLength;
			signed char *p = (signed char *)pins->pSample;
			if (pins->uFlags & CHN_STEREO) len *= 2;
			int max = 1;
			for (UINT i=0; i<len; i++)
			{
				if (p[i] > max) max = p[i];
				if (-p[i] > max) max = -p[i];
			}
			if (max < 127)
			{
				max++;
				for (UINT j=0; j<len; j++)
				{
					int l = p[j];
					p[j] = (l << 7) / max;
				}
				bOk = TRUE;
			}
		}
		if (bOk)
		{
			m_pModDoc->AdjustEndOfSample(m_nSample);
			// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
			m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, NULL);
			m_pModDoc->SetModified();
		}
	}
	EndWaitCursor();
	SwitchToView();
}


void CCtrlSamples::OnAmplify()
//----------------------------
{
	static UINT snOldAmp = 100;
	SAMPLEVIEWSTATE viewstate;
	DWORD dwStart, dwEnd;
	MODINSTRUMENT *pins;
	CAmpDlg dlg(this, snOldAmp);
	
	memset(&viewstate, 0, sizeof(viewstate));
	SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&viewstate);
	if ((!m_pModDoc) || (!m_pSndFile) || (!m_pSndFile->Ins[m_nSample].pSample)) return;
	if (dlg.DoModal() != IDOK) return;
	snOldAmp = dlg.m_nFactor;
	BeginWaitCursor();
	pins = &m_pSndFile->Ins[m_nSample];
	dwStart = viewstate.dwBeginSel;
	dwEnd = viewstate.dwEndSel;
	if (dwEnd > pins->nLength) dwEnd = pins->nLength;
	if (dwStart > dwEnd) dwStart = dwEnd;
	if (dwStart >= dwEnd)
	{
		dwStart = 0;
		dwEnd = pins->nLength;
	}
	if (pins->uFlags & CHN_STEREO) { dwStart *= 2; dwEnd *= 2; }
	UINT len = dwEnd - dwStart;
	LONG lAmp = dlg.m_nFactor;
	if ((dlg.m_bFadeIn) && (dlg.m_bFadeOut)) lAmp *= 4;
	if (pins->uFlags & CHN_16BIT)
	{
		signed short *p = ((signed short *)pins->pSample) + dwStart;

		for (UINT i=0; i<len; i++)
		{
			LONG l = (p[i] * lAmp) / 100;
			if (dlg.m_bFadeIn) l = (LONG)((l * (LONGLONG)i) / len);
			if (dlg.m_bFadeOut) l = (LONG)((l * (LONGLONG)(len-i)) / len);
			if (l < -32768) l = -32768;
			if (l > 32767) l = 32767;
			p[i] = (signed short)l;
		}
	} else
	{
		signed char *p = ((signed char *)pins->pSample) + dwStart;

		for (UINT i=0; i<len; i++)
		{
			LONG l = (p[i] * lAmp) / 100;
			if (dlg.m_bFadeIn) l = (LONG)((l * (LONGLONG)i) / len);
			if (dlg.m_bFadeOut) l = (LONG)((l * (LONGLONG)(len-i)) / len);
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


const int gSinc2x16Odd[16] =
{ // Kaiser window, beta=7.400
  -19,    97,  -295,   710, -1494,  2958, -6155, 20582, 20582, -6155,  2958, -1494,   710,  -295,    97,   -19,
};


void CCtrlSamples::OnUpsample()
//-----------------------------
{
	SAMPLEVIEWSTATE viewstate;
	MODINSTRUMENT *pins;
	DWORD dwStart, dwEnd, dwNewLen;
	UINT smplsize, newsmplsize;
	PVOID pOriginal, pNewSample;

	SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&viewstate);
	if ((!m_pSndFile) || (!m_pSndFile->Ins[m_nSample].pSample)) return;
	BeginWaitCursor();
	pins = &m_pSndFile->Ins[m_nSample];
	dwStart = viewstate.dwBeginSel;
	dwEnd = viewstate.dwEndSel;
	if (dwEnd > pins->nLength) dwEnd = pins->nLength;
	if (dwStart >= dwEnd)
	{
		dwStart = 0;
		dwEnd = pins->nLength;
	}
	smplsize = (pins->uFlags & CHN_16BIT) ? 2 : 1;
	if (pins->uFlags & CHN_STEREO) smplsize *= 2;
	newsmplsize = (pins->uFlags & CHN_STEREO) ? 4 : 2;
	pOriginal = pins->pSample;
	dwNewLen = pins->nLength + (dwEnd-dwStart);
	pNewSample = NULL;
	if (dwNewLen+4 <= MAX_SAMPLE_LENGTH) pNewSample = CSoundFile::AllocateSample((dwNewLen+4)*newsmplsize);
	if (pNewSample)
	{
		UINT nCh = (pins->uFlags & CHN_STEREO) ? 2 : 1;
		for (UINT iCh=0; iCh<nCh; iCh++)
		{
			int len = dwEnd-dwStart;
			int maxndx = pins->nLength;
			if (pins->uFlags & CHN_16BIT)
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
		if (pins->uFlags & CHN_16BIT)
		{
			if (dwStart > 0) memcpy(pNewSample, pOriginal, dwStart*smplsize);
			if (dwEnd < pins->nLength) memcpy(((LPSTR)pNewSample)+(dwStart+(dwEnd-dwStart)*2)*smplsize, ((LPSTR)pOriginal)+(dwEnd*smplsize), (pins->nLength-dwEnd)*smplsize);
		} else
		{
			if (dwStart > 0)
			{
				for (UINT i=0; i<dwStart*nCh; i++)
				{
					((signed short *)pNewSample)[i] = (signed short)(((signed char *)pOriginal)[i] << 8);
				}
			}
			if (dwEnd < pins->nLength)
			{
				signed short *pdest = ((signed short *)pNewSample) + (dwEnd-dwStart)*nCh;
				for (UINT i=dwEnd*nCh; i<pins->nLength*nCh; i++)
				{
					pdest[i] = (signed short)(((signed char *)pOriginal)[i] << 8);
				}
			}
		}
		if (pins->nLoopStart >= dwEnd) pins->nLoopStart += (dwEnd-dwStart); else
		if (pins->nLoopStart > dwStart) pins->nLoopStart += (pins->nLoopStart - dwStart);
		if (pins->nLoopEnd >= dwEnd) pins->nLoopEnd += (dwEnd-dwStart); else
		if (pins->nLoopEnd > dwStart) pins->nLoopEnd += (pins->nLoopEnd - dwStart);
		if (pins->nSustainStart >= dwEnd) pins->nSustainStart += (dwEnd-dwStart); else
		if (pins->nSustainStart > dwStart) pins->nSustainStart += (pins->nSustainStart - dwStart);
		if (pins->nSustainEnd >= dwEnd) pins->nSustainEnd += (dwEnd-dwStart); else
		if (pins->nSustainEnd > dwStart) pins->nSustainEnd += (pins->nSustainEnd - dwStart);
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
		pins->uFlags |= CHN_16BIT;
		pins->pSample = (LPSTR)pNewSample;
		pins->nLength = dwNewLen;
		if (viewstate.dwEndSel <= viewstate.dwBeginSel)
		{
			if (pins->nC4Speed < 200000) pins->nC4Speed *= 2;
			if (pins->RelativeTone < 84) pins->RelativeTone += 12;
		}
		CSoundFile::FreeSample(pOriginal);
		END_CRITICAL();
		m_pModDoc->AdjustEndOfSample(m_nSample);
		if (viewstate.dwEndSel > viewstate.dwBeginSel)
		{
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
	SAMPLEVIEWSTATE viewstate;
	MODINSTRUMENT *pins;
	DWORD dwStart, dwEnd, dwRemove, dwNewLen;
	UINT smplsize;
	PVOID pOriginal, pNewSample;

	SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&viewstate);
	if ((!m_pSndFile) || (!m_pSndFile->Ins[m_nSample].pSample)) return;
	BeginWaitCursor();
	pins = &m_pSndFile->Ins[m_nSample];
	dwStart = viewstate.dwBeginSel;
	dwEnd = viewstate.dwEndSel;
	if (dwEnd > pins->nLength) dwEnd = pins->nLength;
	if (dwStart >= dwEnd)
	{
		dwStart = 0;
		dwEnd = pins->nLength;
	}
	smplsize = (pins->uFlags & CHN_16BIT) ? 2 : 1;
	if (pins->uFlags & CHN_STEREO) smplsize *= 2;
	pOriginal = pins->pSample;
	dwRemove = (dwEnd-dwStart+1)>>1;
	dwNewLen = pins->nLength - dwRemove;
	dwEnd = dwStart+dwRemove*2;
	pNewSample = NULL;
	if ((dwNewLen > 32) && (dwRemove)) pNewSample = CSoundFile::AllocateSample((dwNewLen+4)*smplsize);
	if (pNewSample)
	{
		UINT nCh = (pins->uFlags & CHN_STEREO) ? 2 : 1;
		for (UINT iCh=0; iCh<nCh; iCh++)
		{
			int len = dwRemove;
			int maxndx = pins->nLength;
			if (pins->uFlags & CHN_16BIT)
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
		if (dwEnd < pins->nLength) memcpy(((LPSTR)pNewSample)+(dwStart+dwRemove)*smplsize, ((LPSTR)pOriginal)+((dwStart+dwRemove*2)*smplsize), (pins->nLength-dwEnd)*smplsize);
		if (pins->nLoopStart >= dwEnd) pins->nLoopStart -= dwRemove; else
		if (pins->nLoopStart > dwStart) pins->nLoopStart -= (pins->nLoopStart - dwStart)/2;
		if (pins->nLoopEnd >= dwEnd) pins->nLoopEnd -= dwRemove; else
		if (pins->nLoopEnd > dwStart) pins->nLoopEnd -= (pins->nLoopEnd - dwStart)/2;
		if (pins->nLoopEnd > dwNewLen) pins->nLoopEnd = dwNewLen;
		if (pins->nSustainStart >= dwEnd) pins->nSustainStart -= dwRemove; else
		if (pins->nSustainStart > dwStart) pins->nSustainStart -= (pins->nSustainStart - dwStart)/2;
		if (pins->nSustainEnd >= dwEnd) pins->nSustainEnd -= dwRemove; else
		if (pins->nSustainEnd > dwStart) pins->nSustainEnd -= (pins->nSustainEnd - dwStart)/2;
		if (pins->nSustainEnd > dwNewLen) pins->nSustainEnd = dwNewLen;
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
		if (viewstate.dwEndSel <= viewstate.dwBeginSel)
		{
			if (pins->nC4Speed > 2000) pins->nC4Speed /= 2;
			if (pins->RelativeTone > -84) pins->RelativeTone -= 12;
		}
		pins->nLength = dwNewLen;
		pins->pSample = (LPSTR)pNewSample;
		CSoundFile::FreeSample(pOriginal);
		END_CRITICAL();
		m_pModDoc->AdjustEndOfSample(m_nSample);
		if (viewstate.dwEndSel > viewstate.dwBeginSel)
		{
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


// -> CODE#0029
// -> DESC="pitch shifting - time stretching"

#define MAX_BUFFER_LENGTH	8192
#define CLIP_SOUND(v)		v = v < -1.0f ? -1.0f : v > 1.0f ? 1.0f : v

void CCtrlSamples::OnEnableStretchToSize()
{
	// Enable time-stretching / disable unused pitch-shifting UI elements
	if(IsDlgButtonChecked(IDC_CHECK3)){
		((CComboBox *)GetDlgItem(IDC_COMBO4))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_EDIT6))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_BUTTON2))->EnableWindow(TRUE); //rewbs.timeStretchMods
		SetDlgItemText(IDC_BUTTON1, "Time Stretch");
	}
	// Enable pitch-shifting / disable unused time-stretching UI elements
	else{
		((CComboBox *)GetDlgItem(IDC_COMBO4))->EnableWindow(TRUE);
		((CEdit *)GetDlgItem(IDC_EDIT6))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE); //rewbs.timeStretchMods
		SetDlgItemText(IDC_BUTTON1, "Pitch Shift");
	}
}

void CCtrlSamples::OnEstimateSampleSize()
{
	if((!m_pSndFile) || (!m_pSndFile->Ins[m_nSample].pSample)) return;
	MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	if(!pins) return;

	//rewbs.timeStretchMods
	//Ensure m_dTimeStretchRatio is up-to-date with textbox content
	UpdateData(TRUE);

	//Calculate/verify samplerate at C4.
	long lSampleRate = pins->nC4Speed;
	if(m_pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
		lSampleRate = (double)CSoundFile::TransposeToFrequency(pins->RelativeTone, pins->nFineTune);
	if(lSampleRate <= 0) 
		lSampleRate = 8363.0;

	//Open dialog
	CPSRatioCalc dlg(pins->nLength, lSampleRate, 
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

	if((!m_pSndFile) || (!m_pSndFile->Ins[m_nSample].pSample)) goto error;
	MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	if(!pins || pins->nLength == 0) goto error;

	// Preview management
	if(IsDlgButtonChecked(IDC_CHECK2)){
		// Free previous undo buffer
		if(pSampleUndoBuffer) CSoundFile::FreeSample(pSampleUndoBuffer);
		// Allocate sample undo buffer
		BYTE smpsize  = (pins->uFlags & CHN_16BIT)  ? 2 : 1;
		UINT nChn = (pins->uFlags & CHN_STEREO) ? 2 : 1;
		UndoBufferSize = pins->nLength * nChn * smpsize;
		pSampleUndoBuffer = CSoundFile::AllocateSample(UndoBufferSize);
		// Not enough memory...
		if(pSampleUndoBuffer == NULL){
			UndoBufferSize = 0;
			errorcode = 3;
			goto error;
		}
		// Copy sample to sample undo buffer
		memcpy(pSampleUndoBuffer,pins->pSample,UndoBufferSize);
	}

	// Time stretching
	if(IsDlgButtonChecked(IDC_CHECK3)){
		//rewbs.timeStretchMods
		UpdateData(TRUE); //Ensure m_dTimeStretchRatio is up-to-date with textbox content
		errorcode = TimeStretch(m_dTimeStretchRatio/100.0);

		//Update loop points
		pins->nLoopStart *= m_dTimeStretchRatio/100.0;
		pins->nLoopEnd *= m_dTimeStretchRatio/100.0;
		pins->nSustainStart *= m_dTimeStretchRatio/100.0;
		pins->nSustainEnd *= m_dTimeStretchRatio/100.0;
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

	// Preview management
	if(errorcode == 0 && IsDlgButtonChecked(IDC_CHECK2)){
		((CButton *)GetDlgItem(IDC_BUTTON1))->ShowWindow(SW_HIDE);
		((CButton *)GetDlgItem(IDC_BUTTON3))->ShowWindow(SW_SHOW);
		((CButton *)GetDlgItem(IDC_BUTTON4))->ShowWindow(SW_SHOW);
		SetDlgItemText(IDC_STATIC1,"Preview...");
		
		//rewbs.timeStretchMods
		//Disable all pitch shift / timestrech buttons until accepted or restored:
		GetDlgItem(IDC_COMBO5)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO6)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK3)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO4)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT6)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
		//end rewbs.timeStretchMods
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
			default: wsprintf(str,"Unknown Error...");
				break;
		}
		::MessageBox(NULL,str,"Error",MB_ICONERROR);
	}

	// Update sample view
	m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO, NULL); // !!!! see CODE#0006, update#3
	m_pModDoc->SetModified();
}

void CCtrlSamples::OnPitchShiftTimeStretchAccept()
{
	// Free sample undo buffer
	if(pSampleUndoBuffer) CSoundFile::FreeSample(pSampleUndoBuffer);
	pSampleUndoBuffer = NULL;
	UndoBufferSize = 0;

	// Restore UI buttons
	((CButton *)GetDlgItem(IDC_BUTTON1))->ShowWindow(SW_SHOW);
	((CButton *)GetDlgItem(IDC_BUTTON3))->ShowWindow(SW_HIDE);
	((CButton *)GetDlgItem(IDC_BUTTON4))->ShowWindow(SW_HIDE);
	SetDlgItemText(IDC_STATIC1,"");
	//rewbs.timeStretchMods
	//Disable all pitch shift / timestrech buttons until accepted or restored:
	GetDlgItem(IDC_COMBO5)->EnableWindow(TRUE);
	GetDlgItem(IDC_COMBO6)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK3)->EnableWindow(TRUE);
	if (IsDlgButtonChecked(IDC_CHECK3))
	{
		GetDlgItem(IDC_EDIT6)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON2)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_COMBO4)->EnableWindow(TRUE);
	}
	//end rewbs.timeStretchMods
}

void CCtrlSamples::OnPitchShiftTimeStretchCancel()
{
	if((!m_pSndFile) || (!m_pSndFile->Ins[m_nSample].pSample)) return;
	MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	if(!pins) return;

	// Save processed sample buffer pointer
	PVOID oldbuffer = pins->pSample;
	BYTE smpsize  = (pins->uFlags & CHN_16BIT)  ? 2 : 1;
	UINT nChn = (pins->uFlags & CHN_STEREO) ? 2 : 1;

	// Restore undo buffer pointer & update song data
	BEGIN_CRITICAL();
	for(UINT i=0 ; i < MAX_CHANNELS ; i++){
		if((PVOID)m_pSndFile->Chn[i].pSample == oldbuffer){
			m_pSndFile->Chn[i].pSample = (LPSTR)pSampleUndoBuffer;
			m_pSndFile->Chn[i].pCurrentSample = (LPSTR)pSampleUndoBuffer;
			m_pSndFile->Chn[i].nLength = 0;
		}
	}
	pins->pSample = (LPSTR)pSampleUndoBuffer;
	pins->nLength = UndoBufferSize / (smpsize * nChn);
	
	//rewbs.timeStretchMods
	// Restore loop points, if they were time stretched:
	// Note: m_dTimeStretchRatio will not have changed since we disable  
	//       GUI controls until preview is accepted/restored.
	if(IsDlgButtonChecked(IDC_CHECK3)) {
		pins->nLoopStart /= m_dTimeStretchRatio/100.0;
		pins->nLoopEnd /= m_dTimeStretchRatio/100.0;
		pins->nSustainStart /= m_dTimeStretchRatio/100.0;
		pins->nSustainEnd /= m_dTimeStretchRatio/100.0;
	}
	//end rewbs.timeStretchMods

	END_CRITICAL();

	// Set processed sample buffer pointer as undo sample buffer pointer
	pSampleUndoBuffer = oldbuffer;

	// Free sample undo buffer & restore UI buttons
	OnPitchShiftTimeStretchAccept();

	// Update sample view
	m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA | HINT_SAMPLEINFO, NULL); // !!!! see CODE#0006, update#3
	m_pModDoc->SetModified();
}

int CCtrlSamples::TimeStretch(double ratio)
{
	if((!m_pSndFile) || (!m_pSndFile->Ins[m_nSample].pSample)) return -1;
	MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	if(!pins) return -1;

	// Refuse processing when ratio is negative, equal to zero or equal to 1.0
	if(ratio <= 0.0 || ratio == 1.0) return -1;

	// Convert to pitch factor
	float pitch = Round((float)ratio, 4);
	if(pitch < 0.5f) return 2 + (1<<8);
	if(pitch > 2.0f) return 2 + (2<<8);

	// Get number of channels & sample size
	BYTE smpsize  = (pins->uFlags & CHN_16BIT)  ? 2 : 1;
	UINT nChn = (pins->uFlags & CHN_STEREO) ? 2 : 1;

	// Allocate new sample
	DWORD newsize = (DWORD)(0.5 + ratio * (double)pins->nLength);
	PVOID pSample = pins->pSample;
	PVOID pNewSample = CSoundFile::AllocateSample(newsize * nChn * smpsize);
	if(pNewSample == NULL) return 3;

	// Apply pitch-shifting step
	PitchShift(pitch);

	// Allocate working buffers
	UINT outputsize = (UINT)(0.5 + ratio * (double)MAX_BUFFER_LENGTH);
	if( (outputsize & 1) && (smpsize == 2 || nChn == 2) ) outputsize++;

	float * buffer = new float[MAX_BUFFER_LENGTH * nChn];
	float * outbuf = new float[outputsize * nChn];

	// Create resampler
	int error;
	SRC_STATE * resampler = src_new(SRC_SINC_BEST_QUALITY, nChn, &error) ;

	// Fill in resampler data struct
	SRC_DATA data;
	data.data_in = &buffer[0];
	data.input_frames = MAX_BUFFER_LENGTH;
	data.data_out = &outbuf[0];
	data.output_frames = outputsize;
	data.src_ratio = ratio;
	data.end_of_input = 0;

	// Deduce max sample value (float conversion step)
	float maxSampleValue = ( 1 << (smpsize * 8 - 1) ) - 1;

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
	SetDlgItemText(IDC_STATIC1,"Resampling...");

	// Apply stretching (resampling) step
	UINT pos = 0, posnew = 0;
	UINT len = MAX_BUFFER_LENGTH;

	// Process sample buffer using MAX_BUFFER_LENGTH (max) sized chunk steps (in order to allow
	// the processing of BIG samples...)
	while(pos < pins->nLength){

		// Current chunk size limit test
		if(pos + len >= pins->nLength) len = pins->nLength - pos;

		// Show progress bar using process button painting & text label
		CHAR progress[16];
		float percent = 100.0f * (pos + len) / pins->nLength;
		progressBarRECT.right = processButtonRect.left + (int)percent * (processButtonRect.right - processButtonRect.left) / 100;
		wsprintf(progress,"%d%%",(UINT)percent);

		::FillRect(processButtonDC,&processButtonRect,red);
		::FrameRect(processButtonDC,&processButtonRect,CMainFrame::brushBlack);
		::FillRect(processButtonDC,&progressBarRECT,green);
		::SelectObject(processButtonDC,(HBRUSH)HOLLOW_BRUSH);
		::SetBkMode(processButtonDC,TRANSPARENT);
		::DrawText(processButtonDC,progress,strlen(progress),&processButtonRect,DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		::GdiFlush();

		// Convert current channel's data chunk to float
		BYTE * ptr = (BYTE *)pSample + pos * smpsize * nChn;

		for(UINT j = 0 ; j < len ; j++){
			switch(smpsize){
				case 2:
					buffer[j*nChn] = ((float)(*(SHORT *)ptr)) / maxSampleValue;
					if(nChn == 2) { ptr += 2; buffer[j*nChn+1] = ((float)(*(SHORT *)ptr)) / maxSampleValue; }
					break;
				case 1:
					buffer[j*nChn] = ((float)*ptr) / maxSampleValue;
					if(nChn == 2) { ptr++; buffer[j*nChn+1] = ((float)*ptr) / maxSampleValue; }
					break;
			}
			ptr += smpsize;
		}

		// Wake up resampler...
		if(pos + MAX_BUFFER_LENGTH >= pins->nLength) data.end_of_input = 1;
		src_process(resampler, &data);

		// New buffer limit test (seems like, whatever the size of the last chunk, the resampler return buffer size instead of processed size)
		if(data.end_of_input && posnew + (UINT)data.output_frames_gen >= newsize) data.output_frames_gen = newsize - posnew;

		// Convert resampled float buffer into new sample buffer
		ptr = (BYTE *)pNewSample + posnew * smpsize * nChn;

		for(UINT j = 0 ; j < (UINT)data.output_frames_gen ; j++){
			// Just perform a little bit of clipping...
			float v = outbuf[j*nChn]; CLIP_SOUND(v);
			// ...before converting back to buffer
			switch(smpsize){
				case 2:
					*(SHORT *)ptr = (SHORT)(v * maxSampleValue);
					if(nChn == 2) { ptr += 2; v = outbuf[j*nChn+1]; CLIP_SOUND(v); *(SHORT *)ptr = (SHORT)(v * maxSampleValue); }
					break;
				case 1:
					*ptr = (BYTE)(v * maxSampleValue);
					if(nChn == 2) { ptr++; v = outbuf[j*nChn+1]; CLIP_SOUND(v); *ptr = (BYTE)(v * maxSampleValue); }
					break;
			}
			ptr += smpsize;
		}

		// Next buffer chunk
		posnew += data.output_frames_gen;
		pos += MAX_BUFFER_LENGTH;
	}

	// Update newsize with generated size
	newsize = posnew;

	// Swap sample buffer pointer to new buffer, update song + sample data & free old sample buffer
	BEGIN_CRITICAL();
	for(UINT i=0 ; i < MAX_CHANNELS ; i++){
		if((PVOID)m_pSndFile->Chn[i].pSample == pSample){
			m_pSndFile->Chn[i].pSample = (LPSTR)pNewSample;
			m_pSndFile->Chn[i].pCurrentSample = (LPSTR)pNewSample;
			m_pSndFile->Chn[i].nLength = 0;
		}
	}
	pins->pSample = (LPSTR)pNewSample;
	CSoundFile::FreeSample(pSample);
	pins->nLength = newsize;
	END_CRITICAL();

	// Free working buffers
	data.data_in = data.data_out = NULL;
	if(buffer) delete [] buffer;
	if(outbuf) delete [] outbuf;

	// Free resampler
	if(resampler) src_delete(resampler);

	// Free progress bar brushes
	DeleteObject((HBRUSH)green);
	DeleteObject((HBRUSH)red);

	// Restore process button text
	SetDlgItemText(IDC_BUTTON1, oldText);

	// Restore mouse cursor
	SetDlgItemText(IDC_STATIC1,"");
	EndWaitCursor();

	return 0;
}

int CCtrlSamples::PitchShift(float pitch)
{
	if((!m_pSndFile) || (!m_pSndFile->Ins[m_nSample].pSample)) return -1;
	if(pitch < 0.5f) return 1 + (1<<8);
	if(pitch > 2.0f) return 1 + (2<<8);

	// Get sample struct pointer
	MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	if(!pins) return 2;

	// Get number of channels & sample size
	BYTE smpsize  = (pins->uFlags & CHN_16BIT)  ? 2 : 1;
	UINT nChn = (pins->uFlags & CHN_STEREO) ? 2 : 1;

	// Get selected oversampling - quality - (also refered as FFT overlapping) factor
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO5);
	short ovs = combo->GetCurSel() + 4;

	// Get selected FFT size (power of 2 ; should not exceed MAX_BUFFER_LENGTH - see smbPitchShift.h)
	combo = (CComboBox *)GetDlgItem(IDC_COMBO6);
	UINT fft = 1 << (combo->GetCurSel() + 8);
	while(fft > MAX_BUFFER_LENGTH) fft >>= 1;

	// Get original sample rate
	long lSampleRate = pins->nC4Speed;
	if(m_pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)) lSampleRate = CSoundFile::TransposeToFrequency(pins->RelativeTone, pins->nFineTune);
	if(lSampleRate <= 0) lSampleRate = 8363;

	// Deduce max sample value (float conversion step)
	float maxSampleValue = ( 1 << (smpsize * 8 - 1) ) - 1;

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
	SetDlgItemText(IDC_STATIC1,"Pitch shifting...");

	// Allocate working buffers
	float * buffer = new float[MAX_BUFFER_LENGTH + fft];
	float * outbuf = new float[MAX_BUFFER_LENGTH + fft];

	// Process each channel separately
	for(UINT i = 0 ; i < nChn ; i++){

		UINT pos = 0;
		UINT len = MAX_BUFFER_LENGTH;

		// Process sample buffer using MAX_BUFFER_LENGTH (max) sized chunk steps (in order to allow
		// the processing of BIG samples...)
		while(pos < pins->nLength){

			// Current chunk size limit test
			if(pos + len >= pins->nLength) len = pins->nLength - pos;

			// TRICK : output buffer offset management
			// as the pitch-shifter adds  some blank signal in head of output  buffer (matching FFT
			// length - in short it needs a certain amount of data before being able to output some
			// meaningfull  processed samples) , in order  to avoid this behaviour , we will ignore  
			// the  first FFT_length  samples and process  the same  amount of extra  blank samples
			// (all 0.0f) at the end of the buffer (those extra samples will benefit from  internal
			// FFT data  computed during the previous  steps resulting in a  correct and consistent
			// signal output).
			bool bufstart = ( pos == 0 );
			bool bufend   = ( pos + MAX_BUFFER_LENGTH >= pins->nLength );
			UINT startoffset = ( bufstart ? fft : 0 );
			UINT inneroffset = ( bufstart ? 0 : fft );
			UINT finaloffset = ( bufend   ? fft : 0 );

			// Show progress bar using process button painting & text label
			CHAR progress[16];
			float percent = (float)i * 50.0f + (100.0f / nChn) * (pos + len) / pins->nLength;
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
			BYTE * ptr = (BYTE *)pins->pSample + pos * smpsize * nChn + i * smpsize;

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
			ptr = (BYTE *)pins->pSample + (pos - inneroffset) * smpsize * nChn + i * smpsize;

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
	SetDlgItemText(IDC_STATIC1,"");
	EndWaitCursor();

	return 0;
}
// -! TEST#0029


void CCtrlSamples::OnReverse()
//----------------------------
{
	SAMPLEVIEWSTATE viewstate;
	MODINSTRUMENT *pins;
	DWORD dwBeginSel, dwEndSel;
	LPVOID pSample;
	UINT rlen, smplsize;
	
	memset(&viewstate, 0, sizeof(viewstate));
	SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&viewstate);
	if ((!m_pSndFile) || (!m_pSndFile->Ins[m_nSample].pSample)) return;
	BeginWaitCursor();
	dwBeginSel = viewstate.dwBeginSel;
	dwEndSel = viewstate.dwEndSel;
	pins = &m_pSndFile->Ins[m_nSample];
	rlen = pins->nLength;
	pSample = pins->pSample;
	smplsize = (pins->uFlags & CHN_16BIT) ? 2 : 1;
	if (pins->uFlags & CHN_STEREO) smplsize *= 2;
	if ((dwEndSel > dwBeginSel) && (dwEndSel <= rlen))
	{
		rlen = dwEndSel - dwBeginSel;
		pSample = ((LPBYTE)pins->pSample) +  dwBeginSel * smplsize;
	}
	if (rlen >= 2)
	{
		
		if (smplsize == 4)
		{
			UINT len = rlen / 2;
			UINT max = rlen - 1;
			int *p = (int *)pSample;

			for (UINT i=0; i<len; i++)
			{
				int tmp = p[max-i];
				p[max-i] = p[i];
				p[i] = tmp;
			}
		} else
		if (smplsize == 2)
		{
			UINT len = rlen / 2;
			UINT max = rlen - 1;
			signed short *p = (signed short *)pSample;

			for (UINT i=0; i<len; i++)
			{
				signed short tmp = p[max-i];
				p[max-i] = p[i];
				p[i] = tmp;
			}
		} else
		{
			UINT len = rlen / 2;
			UINT max = rlen - 1;
			signed char *p = (signed char *)pSample;

			for (UINT i=0; i<len; i++)
			{
				signed char tmp = p[max-i];
				p[max-i] = p[i];
				p[i] = tmp;
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


void CCtrlSamples::OnSilence()
//----------------------------
{
	SAMPLEVIEWSTATE viewstate;
	MODINSTRUMENT *pins;
	DWORD dwBeginSel, dwEndSel;
	
	memset(&viewstate, 0, sizeof(viewstate));
	SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&viewstate);
	if ((!m_pSndFile) || (!m_pSndFile->Ins[m_nSample].pSample)) return;
	BeginWaitCursor();
	dwBeginSel = viewstate.dwBeginSel;
	dwEndSel = viewstate.dwEndSel;
	pins = &m_pSndFile->Ins[m_nSample];
	if (dwEndSel > pins->nLength) dwEndSel = pins->nLength;
	if (dwEndSel > dwBeginSel+1)
	{
		int len = dwEndSel - dwBeginSel;
		if (pins->uFlags & CHN_STEREO)
		{
			int smplsize = (pins->uFlags & CHN_16BIT) ? 4 : 2;
			signed char *p = ((signed char *)pins->pSample) + dwBeginSel*smplsize;
			memset(p, 0, len*smplsize);
		} else
		if (pins->uFlags & CHN_16BIT)
		{
			short int *p = ((short int *)pins->pSample) + dwBeginSel;
			int dest = (dwEndSel < pins->nLength) ? p[len-1] : 0;
			int base = (dwBeginSel) ? p[0] : 0;
			int delta = dest - base;
			for (int i=0; i<len; i++)
			{
				int n = base + (int)(((LONGLONG)delta * (LONGLONG)i) / (len-1));
				p[i] = (signed short)n;
			}
		} else
		{
			signed char *p = ((signed char *)pins->pSample) + dwBeginSel;
			int dest = (dwEndSel < pins->nLength) ? p[len-1] : 0;
			int base = (dwBeginSel) ? p[0] : 0;
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
	if (strncmp(s, m_pSndFile->Ins[m_nSample].name, 22))
	{
		memcpy(m_pSndFile->Ins[m_nSample].name, s, 22);
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
	if (nVol != m_pSndFile->Ins[m_nSample].nVolume)
	{
		m_pSndFile->Ins[m_nSample].nVolume = nVol;
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
	if (nVol != m_pSndFile->Ins[m_nSample].nGlobalVol)
	{
		m_pSndFile->Ins[m_nSample].nGlobalVol = nVol;
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
		if (!(m_pSndFile->Ins[m_nSample].uFlags & CHN_PANNING))
		{
			m_pSndFile->Ins[m_nSample].uFlags |= CHN_PANNING;
			m_pModDoc->SetModified();
		}
	} else
	{
		if (m_pSndFile->Ins[m_nSample].uFlags & CHN_PANNING)
		{
			m_pSndFile->Ins[m_nSample].uFlags &= ~CHN_PANNING;
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
	if (nPan != m_pSndFile->Ins[m_nSample].nPan)
	{
		m_pSndFile->Ins[m_nSample].nPan = nPan;
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
		if ((n >= 2000) && (n <= 256000) && (n != (int)m_pSndFile->Ins[m_nSample].nC4Speed))
		{
			m_pSndFile->Ins[m_nSample].nC4Speed = n;
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
			m_pSndFile->Ins[m_nSample].nFineTune = n;
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
		LONG ft = CSoundFile::FrequencyToTranspose(m_pSndFile->Ins[m_nSample].nC4Speed) & 0x7f;
		n = CSoundFile::TransposeToFrequency(n, ft);
		if ((n >= 500) && (n <= 256000) && (n != (int)m_pSndFile->Ins[m_nSample].nC4Speed))
		{
			CHAR s[32];
			m_pSndFile->Ins[m_nSample].nC4Speed = n;
			wsprintf(s, "%lu", n);
			LockControls();
			m_EditFineTune.SetWindowText(s);
			UnlockControls();
			m_pModDoc->SetModified();
		}
	} else
	{
		if ((n >= -128) && (n < 128)) {
			m_pSndFile->Ins[m_nSample].RelativeTone = n;
			m_pModDoc->SetModified();
		}
	}
}


void CCtrlSamples::OnVibTypeChanged()
//-----------------------------------
{
	if (IsLocked()) return;
	int n = m_ComboAutoVib.GetCurSel();
	if (n >= 0) m_pSndFile->Ins[m_nSample].nVibType = (BYTE)n;
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
		m_pSndFile->Ins[m_nSample].nVibDepth = n;
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
		m_pSndFile->Ins[m_nSample].nVibSweep = n;
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
		m_pSndFile->Ins[m_nSample].nVibRate = n;
		m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnLoopTypeChanged()
//------------------------------------
{
	if ((IsLocked()) || (!m_pSndFile)) return;
	int n = m_ComboLoopType.GetCurSel();
	MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	switch(n)
	{
	case 0:	// Off
		pins->uFlags &= ~(CHN_LOOP|CHN_PINGPONGLOOP|CHN_PINGPONGFLAG);
		break;
	case 1:	// On
		pins->uFlags &= ~CHN_PINGPONGLOOP;
		pins->uFlags |= CHN_LOOP;
		break;
	case 2:	// PingPong
		pins->uFlags |= CHN_LOOP|CHN_PINGPONGLOOP;
		break;
	}
	m_pModDoc->AdjustEndOfSample(m_nSample);
	m_pModDoc->SetModified();
}


void CCtrlSamples::OnLoopStartChanged()
//--------------------------------------
{
	if ((IsLocked()) || (!m_pSndFile)) return;
	MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	LONG n = GetDlgItemInt(IDC_EDIT1);
	if ((n >= 0) && (n < (LONG)pins->nLength) && ((n < (LONG)pins->nLoopEnd) || (!(pins->uFlags & CHN_LOOP))))
	{
		pins->nLoopStart = n;
		m_pModDoc->AdjustEndOfSample(m_nSample);
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, this);
		m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnLoopEndChanged()
//------------------------------------
{
	if ((IsLocked()) || (!m_pSndFile)) return;
	MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	LONG n = GetDlgItemInt(IDC_EDIT2);
	if ((n >= 0) && (n <= (LONG)pins->nLength) && ((n > (LONG)pins->nLoopStart) || (!(pins->uFlags & CHN_LOOP))))
	{
		pins->nLoopEnd = n;
		m_pModDoc->AdjustEndOfSample(m_nSample);
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
	MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	switch(n)
	{
	case 0:	// Off
		pins->uFlags &= ~(CHN_SUSTAINLOOP|CHN_PINGPONGSUSTAIN|CHN_PINGPONGFLAG);
		break;
	case 1:	// On
		pins->uFlags &= ~CHN_PINGPONGSUSTAIN;
		pins->uFlags |= CHN_SUSTAINLOOP;
		break;
	case 2:	// PingPong
		pins->uFlags |= CHN_SUSTAINLOOP|CHN_PINGPONGSUSTAIN;
		break;
	}
	m_pModDoc->SetModified();
}


void CCtrlSamples::OnSustainStartChanged()
//----------------------------------------
{
	if ((IsLocked()) || (!m_pSndFile)) return;
	MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	LONG n = GetDlgItemInt(IDC_EDIT3);
	if ((n >= 0) && (n <= (LONG)pins->nLength)
	 && ((n < (LONG)pins->nSustainEnd) || (!(pins->uFlags & CHN_SUSTAINLOOP))))
	{
		pins->nSustainStart = n;
		// 05/01/05 : ericus replaced "m_nSample << 24" by "m_nSample << 20" : 4000 samples -> 12bits [see Moddoc.h]
		m_pModDoc->UpdateAllViews(NULL, (m_nSample << HINT_SHIFT_SMP) | HINT_SAMPLEDATA, this);
		m_pModDoc->SetModified();
	}
}


void CCtrlSamples::OnSustainEndChanged()
//--------------------------------------
{
	if ((IsLocked()) || (!m_pSndFile)) return;
	MODINSTRUMENT *pins = &m_pSndFile->Ins[m_nSample];
	LONG n = GetDlgItemInt(IDC_EDIT4);
	if ((n >= 0) && (n <= (LONG)pins->nLength)
	 && ((n > (LONG)pins->nSustainStart) || (!(pins->uFlags & CHN_SUSTAINLOOP))))
	{
		pins->nSustainEnd = n;
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
	MODINSTRUMENT *pins = &m_pSndFile->Ins[nsample];
	LPSTR pSample = pins->pSample;
	short int pos;
	BOOL bRedraw = FALSE;
	
	LockControls();
	if ((!pins->nLength) || (!pSample)) goto NoSample;
	if (pins->uFlags & CHN_16BIT)
	{
		pSample++;
		pinc *= 2;
	}
	if (pins->uFlags & CHN_STEREO) pinc *= 2;
	// Loop Start
	if ((pos = (short int)m_SpinLoopStart.GetPos()) != 0)
	{
		BOOL bOk = FALSE;
		LPSTR p = pSample+pins->nLoopStart*pinc;
		int find0 = (int)pSample[pins->nLoopEnd*pinc-pinc];
		int find1 = (int)pSample[pins->nLoopEnd*pinc];
		// Find Next LoopStart Point
		if (pos > 0)
		{
			for (UINT i=pins->nLoopStart+1; i+16<pins->nLoopEnd; i++)
			{
				p += pinc;
				bOk = (pins->uFlags & CHN_PINGPONGLOOP) ? MPT_BidiStartCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pins->nLoopStart = i;
					break;
				}
			}
		} else
		// Find Prev LoopStart Point
		{
			for (UINT i=pins->nLoopStart; i; )
			{
				i--;
				p -= pinc;
				bOk = (pins->uFlags & CHN_PINGPONGLOOP) ? MPT_BidiStartCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pins->nLoopStart = i;
					break;
				}
			}
		}
		if (bOk)
		{
			wsprintf(s, "%u", pins->nLoopStart);
			m_EditLoopStart.SetWindowText(s);
			m_pModDoc->AdjustEndOfSample(m_nSample);
			bRedraw = TRUE;
		}
		m_SpinLoopStart.SetPos(0);
	}
	// Loop End
	pos = (short int)m_SpinLoopEnd.GetPos();
	if ((pos) && (pins->nLoopEnd))
	{
		BOOL bOk = FALSE;
		LPSTR p = pSample+pins->nLoopEnd*pinc;
		int find0 = (int)pSample[pins->nLoopStart*pinc];
		int find1 = (int)pSample[pins->nLoopStart*pinc+pinc];
		// Find Next LoopEnd Point
		if (pos > 0)
		{
			for (UINT i=pins->nLoopEnd+1; i<=pins->nLength; i++, p+=pinc)
			{
				bOk = (pins->uFlags & CHN_PINGPONGLOOP) ? MPT_BidiEndCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pins->nLoopEnd = i;
					break;
				}
			}
		} else
		// Find Prev LoopEnd Point
		{
			for (UINT i=pins->nLoopEnd; i>pins->nLoopStart+16; )
			{
				i--;
				p -= pinc;
				bOk = (pins->uFlags & CHN_PINGPONGLOOP) ? MPT_BidiEndCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pins->nLoopEnd = i;
					break;
				}
			}
		}
		if (bOk)
		{
			wsprintf(s, "%u", pins->nLoopEnd);
			m_EditLoopEnd.SetWindowText(s);
			m_pModDoc->AdjustEndOfSample(m_nSample);
			bRedraw = TRUE;
		}
		m_SpinLoopEnd.SetPos(0);
	}
	// Sustain Loop Start
	pos = (short int)m_SpinSustainStart.GetPos();
	if ((pos) && (pins->nSustainEnd))
	{
		BOOL bOk = FALSE;
		LPSTR p = pSample+pins->nSustainStart*pinc;
		int find0 = (int)pSample[pins->nSustainEnd*pinc-pinc];
		int find1 = (int)pSample[pins->nSustainEnd*pinc];
		// Find Next Sustain LoopStart Point
		if (pos > 0)
		{
			for (UINT i=pins->nSustainStart+1; i+16<pins->nSustainEnd; i++)
			{
				p += pinc;
				bOk = (pins->uFlags & CHN_PINGPONGSUSTAIN) ? MPT_BidiStartCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pins->nSustainStart = i;
					break;
				}
			}
		} else
		// Find Prev Sustain LoopStart Point
		{
			for (UINT i=pins->nSustainStart; i; )
			{
				i--;
				p -= pinc;
				bOk = (pins->uFlags & CHN_PINGPONGSUSTAIN) ? MPT_BidiStartCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pins->nSustainStart = i;
					break;
				}
			}
		}
		if (bOk)
		{
			wsprintf(s, "%u", pins->nSustainStart);
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
		LPSTR p = pSample+pins->nSustainEnd*pinc;
		int find0 = (int)pSample[pins->nSustainStart*pinc];
		int find1 = (int)pSample[pins->nSustainStart*pinc+pinc];
		// Find Next LoopEnd Point
		if (pos > 0)
		{
			for (UINT i=pins->nSustainEnd+1; i+1<pins->nLength; i++, p+=pinc)
			{
				bOk = (pins->uFlags & CHN_PINGPONGSUSTAIN) ? MPT_BidiEndCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pins->nSustainEnd = i;
					break;
				}
			}
		} else
		// Find Prev LoopEnd Point
		{
			for (UINT i=pins->nSustainEnd; i>pins->nSustainStart+16; )
			{
				i--;
				p -= pinc;
				bOk = (pins->uFlags & CHN_PINGPONGSUSTAIN) ? MPT_BidiEndCheck(p[0], p[pinc], p[pinc*2]) : MPT_LoopCheck(find0, find1, p[0], p[pinc]);
				if (bOk)
				{
					pins->nSustainEnd = i;
					break;
				}
			}
		}
		if (bOk)
		{
			wsprintf(s, "%u", pins->nSustainEnd);
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
			LONG d = pins->nC4Speed;
			if (d < 1) d = 8363;
			d += (pos * 25);
			if (d > 96000) d = 96000;
			if (d < 2000) d = 2000;
			pins->nC4Speed = d;
			int transp = CSoundFile::FrequencyToTranspose(pins->nC4Speed) >> 7;
			int basenote = 60 - transp;
			if (basenote < BASENOTE_MIN) basenote = BASENOTE_MIN;
			if (basenote >= BASENOTE_MAX) basenote = BASENOTE_MAX-1;
			basenote -= BASENOTE_MIN;
			if (basenote != m_CbnBaseNote.GetCurSel()) m_CbnBaseNote.SetCurSel(basenote);
			wsprintf(s, "%lu", pins->nC4Speed);
			m_EditFineTune.SetWindowText(s);
		} else
		{
			LONG d = pins->nFineTune + pos;
			if (d < -128) d = -128;
			if (d > 127) d = 127;
			pins->nFineTune = d;
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
