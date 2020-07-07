/*
 * SampleEditorDialogs.cpp
 * -----------------------
 * Purpose: Code for various dialogs that are used in the sample editor.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "resource.h"
#include "Reporting.h"
#include "MPTrackUtil.h"
#include "Mptrack.h"
#include "../common/misc_util.h"
#include "../soundlib/Snd_defs.h"
#include "../soundlib/ModSample.h"
#include "SampleEditorDialogs.h"


OPENMPT_NAMESPACE_BEGIN


//////////////////////////////////////////////////////////////////////////
// Sample amplification dialog

BEGIN_MESSAGE_MAP(CAmpDlg, CDialog)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_EDIT2, &CAmpDlg::EnableFadeIn)
	ON_EN_CHANGE(IDC_EDIT3, &CAmpDlg::EnableFadeOut)
END_MESSAGE_MAP()

void CAmpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAmpDlg)
	DDX_Control(pDX, IDC_COMBO1, m_fadeBox);
	//}}AFX_DATA_MAP
}

CAmpDlg::CAmpDlg(CWnd *parent, AmpSettings &settings, int16 factorMin, int16 factorMax)
	: CDialog(IDD_SAMPLE_AMPLIFY, parent)
	, m_settings(settings)
	, m_nFactorMin(factorMin)
	, m_nFactorMax(factorMax)
{}

BOOL CAmpDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CSpinButtonCtrl *spin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN1);
	spin->SetRange32(m_nFactorMin, m_nFactorMax);
	spin->SetPos32(m_settings.factor);
	spin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN2);
	spin->SetRange32(0, 100);
	spin->SetPos32(m_settings.fadeInStart);
	spin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN3);
	spin->SetRange32(0, 100);
	spin->SetPos32(m_settings.fadeOutEnd);

	SetDlgItemInt(IDC_EDIT1, m_settings.factor);
	SetDlgItemInt(IDC_EDIT2, m_settings.fadeInStart);
	SetDlgItemInt(IDC_EDIT3, m_settings.fadeOutEnd);
	m_edit.SubclassDlgItem(IDC_EDIT1, this);
	m_edit.AllowFractions(false);
	m_edit.AllowNegative(m_nFactorMin < 0);
	m_editFadeIn.SubclassDlgItem(IDC_EDIT2, this);
	m_editFadeIn.AllowFractions(false);
	m_editFadeIn.AllowNegative(m_nFactorMin < 0);
	m_editFadeOut.SubclassDlgItem(IDC_EDIT3, this);
	m_editFadeOut.AllowFractions(false);
	m_editFadeOut.AllowNegative(m_nFactorMin < 0);

	const struct
	{
		const TCHAR *name;
		Fade::Law id;
	} fadeLaws[] =
	{
		{ _T("Linear"),       Fade::kLinear },
		{ _T("Exponential"),  Fade::kPow },
		{ _T("Square Root"),  Fade::kSqrt },
		{ _T("Logarithmic"),  Fade::kLog },
		{ _T("Quarter Sine"), Fade::kQuarterSine },
		{ _T("Half Sine"),    Fade::kHalfSine },
	};
	// Create icons for fade laws
	const int cx = Util::ScalePixels(16, m_hWnd);
	const int cy = Util::ScalePixels(16, m_hWnd);
	const int imgWidth = cx * static_cast<int>(std::size(fadeLaws));
	m_list.Create(cx, cy, ILC_COLOR32 | ILC_MASK, 0, 1);
	std::vector<COLORREF> bits(imgWidth * cy, RGB(255, 0, 255));
	const COLORREF col = GetSysColor(COLOR_WINDOWTEXT);
	for(int i = 0, baseX = 0; i < static_cast<int>(std::size(fadeLaws)); i++, baseX += cx)
	{
		Fade::Func fadeFunc = Fade::GetFadeFunc(fadeLaws[i].id);
		int oldVal = cy - 1;
		for(int x = 0; x < cx; x++)
		{
			int val = cy - 1 - mpt::saturate_round<int>(cy * fadeFunc(static_cast<double>(x) / cx));
			Limit(val, 0, cy - 1);
			if(oldVal > val && x > 0)
			{
				int dy = (oldVal - val) / 2;
				for(int y = oldVal * imgWidth; dy != 0; y -= imgWidth, dy--)
				{
					bits[baseX + (x - 1) + y] = col;
				}
				oldVal -= dy + 1;
			}
			for(int y = oldVal * imgWidth; y >= val * imgWidth; y -= imgWidth)
			{
				bits[baseX + x + y] = col;
			}
			oldVal = val;
		}
	}
	CBitmap bitmap;
	bitmap.CreateBitmap(cx * static_cast<int>(std::size(fadeLaws)), cy, 1, 32, bits.data());
	m_list.Add(&bitmap, RGB(255, 0, 255));
	bitmap.DeleteObject();
	m_fadeBox.SetImageList(&m_list);

	// Add fade laws to list
	COMBOBOXEXITEM cbi;
	MemsetZero(cbi);
	cbi.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_TEXT | CBEIF_LPARAM;
	for(int i = 0; i < static_cast<int>(std::size(fadeLaws)); i++)
	{
		cbi.iItem = i;
		cbi.pszText = const_cast<LPTSTR>(fadeLaws[i].name);
		cbi.iImage = cbi.iSelectedImage = i;
		cbi.lParam = fadeLaws[i].id;
		m_fadeBox.InsertItem(&cbi);
		if(fadeLaws[i].id == m_settings.fadeLaw) m_fadeBox.SetCurSel(i);
	}

	m_locked = false;

	return TRUE;
}


void CAmpDlg::OnDestroy()
{
	m_list.DeleteImageList();
}


void CAmpDlg::OnOK()
{
	m_settings.factor = static_cast<int16>(Clamp(static_cast<int>(GetDlgItemInt(IDC_EDIT1)), m_nFactorMin, m_nFactorMax));
	m_settings.fadeInStart = Clamp(static_cast<int>(GetDlgItemInt(IDC_EDIT2)), m_nFactorMin, m_nFactorMax);
	m_settings.fadeOutEnd = Clamp(static_cast<int>(GetDlgItemInt(IDC_EDIT3)), m_nFactorMin, m_nFactorMax);
	m_settings.fadeIn = (IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED);
	m_settings.fadeOut = (IsDlgButtonChecked(IDC_CHECK2) != BST_UNCHECKED);
	m_settings.fadeLaw = static_cast<Fade::Law>(m_fadeBox.GetItemData(m_fadeBox.GetCurSel()));
	CDialog::OnOK();
}


//////////////////////////////////////////////////////////////
// Sample import dialog

SampleIO CRawSampleDlg::m_nFormat(SampleIO::_8bit, SampleIO::mono, SampleIO::littleEndian, SampleIO::unsignedPCM);

BOOL CRawSampleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	UpdateDialog();
	return TRUE;
}


void CRawSampleDlg::OnOK()
{
	if(IsDlgButtonChecked(IDC_RADIO1)) m_nFormat |= SampleIO::_8bit;
	if(IsDlgButtonChecked(IDC_RADIO2)) m_nFormat |= SampleIO::_16bit;
	if(IsDlgButtonChecked(IDC_RADIO3)) m_nFormat |= SampleIO::unsignedPCM;
	if(IsDlgButtonChecked(IDC_RADIO4)) m_nFormat |= SampleIO::signedPCM;
	if(IsDlgButtonChecked(IDC_RADIO5)) m_nFormat |= SampleIO::mono;
	if(IsDlgButtonChecked(IDC_RADIO6)) m_nFormat |= SampleIO::stereoInterleaved;
	m_bRememberFormat = IsDlgButtonChecked(IDC_CHK_REMEMBERSETTINGS) != BST_UNCHECKED;
	CDialog::OnOK();
}


void CRawSampleDlg::UpdateDialog()
{
	CheckRadioButton(IDC_RADIO1, IDC_RADIO2, (m_nFormat.GetBitDepth() == 8) ? IDC_RADIO1 : IDC_RADIO2 );
	CheckRadioButton(IDC_RADIO3, IDC_RADIO4, (m_nFormat.GetEncoding() == SampleIO::unsignedPCM) ? IDC_RADIO3 : IDC_RADIO4);
	CheckRadioButton(IDC_RADIO5, IDC_RADIO6, (m_nFormat.GetChannelFormat() == SampleIO::mono) ? IDC_RADIO5 : IDC_RADIO6);
	CheckDlgButton(IDC_CHK_REMEMBERSETTINGS, (m_bRememberFormat ? BST_CHECKED : BST_UNCHECKED));
}


/////////////////////////////////////////////////////////////////////////
// Add silence / resize sample dialog

BEGIN_MESSAGE_MAP(AddSilenceDlg, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO1,           &AddSilenceDlg::OnUnitChanged)
	ON_COMMAND(IDC_RADIO_ADDSILENCE_BEGIN, &AddSilenceDlg::OnEditModeChanged)
	ON_COMMAND(IDC_RADIO_ADDSILENCE_END,   &AddSilenceDlg::OnEditModeChanged)
	ON_COMMAND(IDC_RADIO_RESIZETO,         &AddSilenceDlg::OnEditModeChanged)
	ON_COMMAND(IDC_RADIO1,                 &AddSilenceDlg::OnEditModeChanged)
END_MESSAGE_MAP()

SmpLength AddSilenceDlg::m_addSamples = 32;
SmpLength AddSilenceDlg::m_createSamples = 64;

AddSilenceDlg::AddSilenceDlg(CWnd *parent, SmpLength origLength, uint32 sampleRate, bool allowOPL)
	: CDialog(IDD_ADDSILENCE, parent)
	, m_numSamples(m_addSamples)
	, m_sampleRate(sampleRate)
	, m_allowOPL(allowOPL)
{
	if(origLength > 0)
	{
		m_length = origLength;
		m_editOption = kSilenceAtEnd;
	} else
	{
		m_length = m_createSamples;
		m_editOption = kResize;
	}
}


BOOL AddSilenceDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CSpinButtonCtrl *spin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_ADDSILENCE);
	if(spin)
	{
		spin->SetRange32(0, int32_max);
		spin->SetPos32(m_numSamples);
	}

	CComboBox *box = (CComboBox *)GetDlgItem(IDC_COMBO1);
	if(box)
	{
		box->AddString(_T("samples"));
		box->AddString(_T("ms"));
		box->SetCurSel(m_unit);
		if(m_sampleRate == 0)
		{
			// Can't do any conversions if samplerate is unknown
			box->EnableWindow(FALSE);
		}
	}

	int buttonID = IDC_RADIO_ADDSILENCE_END;
	switch(m_editOption)
	{
	case kSilenceAtBeginning: buttonID = IDC_RADIO_ADDSILENCE_BEGIN; break;
	case kSilenceAtEnd:       buttonID = IDC_RADIO_ADDSILENCE_END; break;
	case kResize:             buttonID = IDC_RADIO_RESIZETO; break;
	}
	CheckDlgButton(buttonID, BST_CHECKED);

	SetDlgItemInt(IDC_EDIT_ADDSILENCE, (m_editOption == kResize) ? m_length : m_numSamples, FALSE);
	GetDlgItem(IDC_RADIO1)->EnableWindow(m_allowOPL ? TRUE : FALSE);

	return TRUE;
}


void AddSilenceDlg::OnOK()
{
	m_numSamples = GetDlgItemInt(IDC_EDIT_ADDSILENCE, nullptr, FALSE);
	if(m_unit == kMilliseconds)
	{
		m_numSamples = Util::muldivr_unsigned(m_numSamples, m_sampleRate, 1000);
	}
	switch(m_editOption = GetEditMode())
	{
	case kSilenceAtBeginning:
	case kSilenceAtEnd:
		m_addSamples = m_numSamples;
		break;
	case kResize:
		m_createSamples = m_numSamples;
		break;
	}
	CDialog::OnOK();
}


void AddSilenceDlg::OnEditModeChanged()
{
	AddSilenceOptions newEditOption = GetEditMode();
	GetDlgItem(IDC_EDIT_ADDSILENCE)->EnableWindow((newEditOption == kOPLInstrument) ? FALSE : TRUE);
	if(newEditOption != kResize && m_editOption == kResize)
	{
		// Switch to "add silence"
		m_length = GetDlgItemInt(IDC_EDIT_ADDSILENCE);
		SetDlgItemInt(IDC_EDIT_ADDSILENCE, m_numSamples);
	} else if(newEditOption == kResize && m_editOption != kResize)
	{
		// Switch to "resize"
		m_numSamples = GetDlgItemInt(IDC_EDIT_ADDSILENCE);
		SetDlgItemInt(IDC_EDIT_ADDSILENCE, m_length);
	}
	m_editOption = newEditOption;
}


void AddSilenceDlg::OnUnitChanged()
{
	m_unit = static_cast<Unit>(static_cast<CComboBox *>(GetDlgItem(IDC_COMBO1))->GetCurSel());
	SmpLength duration = GetDlgItemInt(IDC_EDIT_ADDSILENCE);
	if(m_unit == kSamples)
	{
		// Convert from milliseconds
		duration = Util::muldivr_unsigned(duration, m_sampleRate, 1000);
	} else
	{
		// Convert from samples
		duration = Util::muldivr_unsigned(duration, 1000, m_sampleRate);
	}
	SetDlgItemInt(IDC_EDIT_ADDSILENCE, duration);
}


AddSilenceDlg::AddSilenceOptions AddSilenceDlg::GetEditMode() const
{
	if(IsDlgButtonChecked(IDC_RADIO_ADDSILENCE_BEGIN)) return kSilenceAtBeginning;
	else if(IsDlgButtonChecked(IDC_RADIO_ADDSILENCE_END)) return kSilenceAtEnd;
	else if(IsDlgButtonChecked(IDC_RADIO_RESIZETO)) return kResize;
	else if(IsDlgButtonChecked(IDC_RADIO1)) return kOPLInstrument;
	MPT_ASSERT_NOTREACHED();
	return kSilenceAtEnd;
}


/////////////////////////////////////////////////////////////////////////
// Sample grid dialog

void CSampleGridDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSampleGridDlg)
	DDX_Control(pDX, IDC_EDIT1,			m_EditSegments);
	DDX_Control(pDX, IDC_SPIN1,			m_SpinSegments);
	//}}AFX_DATA_MAP
}


BOOL CSampleGridDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_SpinSegments.SetRange32(0, m_nMaxSegments);
	m_SpinSegments.SetPos(m_nSegments);
	SetDlgItemInt(IDC_EDIT1, m_nSegments, FALSE);
	GetDlgItem(IDC_EDIT1)->SetFocus();
	return TRUE;
}


void CSampleGridDlg::OnOK()
{
	m_nSegments = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////
// Sample cross-fade dialog

uint32 CSampleXFadeDlg::m_fadeLength  = 20000;
uint32 CSampleXFadeDlg::m_fadeLaw = 50000;
bool CSampleXFadeDlg::m_afterloopFade = true;
bool CSampleXFadeDlg::m_useSustainLoop = false;

BEGIN_MESSAGE_MAP(CSampleXFadeDlg, CDialog)
	ON_WM_HSCROLL()
	ON_COMMAND(IDC_RADIO1,	&CSampleXFadeDlg::OnLoopTypeChanged)
	ON_COMMAND(IDC_RADIO2,	&CSampleXFadeDlg::OnLoopTypeChanged)
	ON_EN_CHANGE(IDC_EDIT1,	&CSampleXFadeDlg::OnFadeLengthChanged)
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, &CSampleXFadeDlg::OnToolTipText)
END_MESSAGE_MAP()


void CSampleXFadeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSampleGridDlg)
	DDX_Control(pDX, IDC_EDIT1,			m_EditSamples);
	DDX_Control(pDX, IDC_SPIN1,			m_SpinSamples);
	DDX_Control(pDX, IDC_SLIDER1,		m_SliderLength);
	DDX_Control(pDX, IDC_SLIDER2,		m_SliderFadeLaw);
	DDX_Control(pDX, IDC_RADIO1,		m_RadioNormalLoop);
	DDX_Control(pDX, IDC_RADIO2,		m_RadioSustainLoop);
	//}}AFX_DATA_MAP
}


BOOL CSampleXFadeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	const bool hasNormal = m_sample.uFlags[CHN_LOOP] && m_sample.nLoopStart > 0;
	const bool hasSustain = m_sample.uFlags[CHN_SUSTAINLOOP] && m_sample.nSustainStart > 0;
	const bool hasBothLoops = hasNormal && hasSustain;
	m_RadioNormalLoop.EnableWindow(hasBothLoops);
	m_RadioSustainLoop.EnableWindow(hasBothLoops);
	CheckRadioButton(IDC_RADIO1, IDC_RADIO2, ((m_useSustainLoop && hasSustain) || !hasNormal) ? IDC_RADIO2 : IDC_RADIO1);

	m_SliderLength.SetRange(0, 100000);
	m_SliderLength.SetPos(m_fadeLength);
	m_SliderFadeLaw.SetRange(0, 100000);
	m_SliderFadeLaw.SetPos(m_fadeLaw);

	OnLoopTypeChanged();

	return TRUE;
}


void CSampleXFadeDlg::OnOK()
{
	m_fadeLength = m_SliderLength.GetPos();
	m_fadeLaw = m_SliderFadeLaw.GetPos();
	m_afterloopFade = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;
	m_useSustainLoop = IsDlgButtonChecked(IDC_RADIO2) != BST_UNCHECKED;
	Limit(m_fadeLength, uint32(0), uint32(100000));
	CDialog::OnOK();
}


void CSampleXFadeDlg::OnLoopTypeChanged()
{
	SmpLength loopStart = m_sample.nLoopStart, loopEnd = m_sample.nLoopEnd;
	if(IsDlgButtonChecked(IDC_RADIO2))
	{
		loopStart = m_sample.nSustainStart;
		loopEnd = m_sample.nSustainEnd;
	}
	m_maxLength = std::min({ m_sample.nLength, loopStart, loopEnd / 2u });
	m_loopLength = loopEnd - loopStart;

	m_editLocked = true;
	m_SpinSamples.SetRange32(0, std::min(m_loopLength, m_maxLength));
	GetDlgItem(IDC_EDIT1)->SetFocus();
	CheckDlgButton(IDC_CHECK1, m_afterloopFade ? BST_CHECKED : BST_UNCHECKED);

	SmpLength numSamples = PercentToSamples(m_SliderLength.GetPos());
	numSamples = std::min({ numSamples, m_loopLength, m_maxLength });
	m_SpinSamples.SetPos(numSamples);
	SetDlgItemInt(IDC_EDIT1, numSamples, FALSE);

	m_editLocked = false;
}


void CSampleXFadeDlg::OnFadeLengthChanged()
{
	if(m_editLocked) return;
	SmpLength numSamples = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
	numSamples = std::min({ numSamples, m_loopLength, m_maxLength });
	m_SliderLength.SetPos(SamplesToPercent(numSamples));
}


void CSampleXFadeDlg::OnHScroll(UINT, UINT, CScrollBar *sb)
{
	if(sb == (CScrollBar *)(&m_SliderLength))
	{
		m_editLocked = true;
		SmpLength numSamples = PercentToSamples(m_SliderLength.GetPos());
		if(numSamples > m_maxLength)
		{
			numSamples = m_maxLength;
			m_SliderLength.SetPos(SamplesToPercent(numSamples));
		}
		m_SpinSamples.SetPos(numSamples);
		SetDlgItemInt(IDC_EDIT1, numSamples, FALSE);
		m_editLocked = false;
	}
}


BOOL CSampleXFadeDlg::OnToolTipText(UINT, NMHDR *pNMHDR, LRESULT *pResult)
{
	TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
	UINT_PTR nID = pNMHDR->idFrom;
	if(pTTT->uFlags & TTF_IDISHWND)
	{
		// idFrom is actually the HWND of the tool
		nID = (UINT_PTR)::GetDlgCtrlID((HWND)nID);
	}
	switch(nID)
	{
	case IDC_SLIDER1:
		{
			uint32 percent = m_SliderLength.GetPos();
			wsprintf(pTTT->szText, _T("%u.%03u%% of the loop (%u samples)"), percent / 1000, percent % 1000, PercentToSamples(percent));
		}
		break;
	case IDC_SLIDER2:
		_tcscpy(pTTT->szText, _T("Slide towards constant power for fixing badly looped samples."));
		break;
	default:
		return FALSE;
	}
	*pResult = 0;

	// bring the tooltip window above other popup windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
		SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////
// Resampling dialog

CResamplingDlg::ResamplingOption CResamplingDlg::lastChoice = CResamplingDlg::Upsample;
uint32 CResamplingDlg::lastFrequency = 0;

BEGIN_MESSAGE_MAP(CResamplingDlg, CDialog)
	ON_EN_SETFOCUS(IDC_EDIT1, &CResamplingDlg::OnFocusEdit)
END_MESSAGE_MAP()

BOOL CResamplingDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1 + lastChoice);
	TCHAR s[32];
	wsprintf(s, _T("&Upsample (%u Hz)"), m_frequency * 2);
	SetDlgItemText(IDC_RADIO1, s);
	wsprintf(s, _T("&Downsample (%u Hz)"), m_frequency / 2);
	SetDlgItemText(IDC_RADIO2, s);
	
	if(!lastFrequency) lastFrequency = m_frequency;
	SetDlgItemInt(IDC_EDIT1, lastFrequency, FALSE);
	CSpinButtonCtrl *spin = static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN1));
	spin->SetRange32(1, 999999);
	spin->SetPos32(lastFrequency);

	CComboBox *cbnResampling = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_FILTER));
	cbnResampling->SetRedraw(FALSE);
	const auto resamplingModes = Resampling::AllModesWithDefault();
	for(auto mode : resamplingModes)
	{
		CString desc = _T("r8brain (High Quality)");
		if(mode != SRCMODE_DEFAULT)
			desc = CTrackApp::GetResamplingModeName(mode, 1, true);

		int index = cbnResampling->AddString(desc);
		cbnResampling->SetItemData(index, mode);
		if(m_srcMode == mode)
			cbnResampling->SetCurSel(index);
	}
	cbnResampling->SetRedraw(TRUE);

	return TRUE;
}


void CResamplingDlg::OnOK()
{
	if(IsDlgButtonChecked(IDC_RADIO1))
	{
		lastChoice = Upsample;
		m_frequency *= 2;
	} else if(IsDlgButtonChecked(IDC_RADIO2))
	{
		lastChoice = Downsample;
		m_frequency /= 2;
	} else
	{
		lastChoice = Custom;
		uint32 newFrequency = GetDlgItemInt(IDC_EDIT1, NULL, FALSE);
		if(newFrequency > 0)
		{
			lastFrequency = m_frequency = newFrequency;
		} else
		{
			MessageBeep(MB_ICONWARNING);
			GetDlgItem(IDC_EDIT1)->SetFocus();
			return;
		}
	}

	CComboBox *cbnResampling = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_FILTER));
	m_srcMode = static_cast<ResamplingMode>(cbnResampling->GetItemData(cbnResampling->GetCurSel()));

	CDialog::OnOK();
}


////////////////////////////////////////////////////////////////////////////////////////////
// Sample mix dialog

SmpLength CMixSampleDlg::sampleOffset = 0;
int CMixSampleDlg::amplifyOriginal = 50;
int CMixSampleDlg::amplifyMix = 50;


void CMixSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMixSampleDlg)
	DDX_Control(pDX, IDC_EDIT_OFFSET,			m_EditOffset);
	DDX_Control(pDX, IDC_SPIN_OFFSET,			m_SpinOffset);
	DDX_Control(pDX, IDC_SPIN_SAMPVOL1,			m_SpinVolOriginal);
	DDX_Control(pDX, IDC_SPIN_SAMPVOL2,			m_SpinVolMix);
	//}}AFX_DATA_MAP
}


CMixSampleDlg::CMixSampleDlg(CWnd *parent)
	: CDialog(IDD_MIXSAMPLES, parent)
{ }


BOOL CMixSampleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Offset
	m_SpinOffset.SetRange32(0, MAX_SAMPLE_LENGTH);
	SetDlgItemInt(IDC_EDIT_OFFSET, sampleOffset);

	// Volumes
	m_SpinVolOriginal.SetRange(-10000, 10000);
	m_SpinVolMix.SetRange(-10000, 10000);

	m_EditVolOriginal.SubclassDlgItem(IDC_EDIT_SAMPVOL1, this);
	m_EditVolOriginal.AllowNegative(true);
	m_EditVolMix.SubclassDlgItem(IDC_EDIT_SAMPVOL2, this);
	m_EditVolMix.AllowNegative(true);

	SetDlgItemInt(IDC_EDIT_SAMPVOL1, amplifyOriginal);
	SetDlgItemInt(IDC_EDIT_SAMPVOL2, amplifyMix);

	return TRUE;
}


void CMixSampleDlg::OnOK()
{
	CDialog::OnOK();
	sampleOffset = Clamp<SmpLength, SmpLength>(GetDlgItemInt(IDC_EDIT_OFFSET), 0, MAX_SAMPLE_LENGTH);
	amplifyOriginal = Clamp<int, int>(GetDlgItemInt(IDC_EDIT_SAMPVOL1), -10000, 10000);
	amplifyMix = Clamp<int, int>(GetDlgItemInt(IDC_EDIT_SAMPVOL2), -10000, 10000);
}

OPENMPT_NAMESPACE_END
