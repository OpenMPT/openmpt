/*
 * ColorConfigDlg.cpp
 * ------------------
 * Purpose: Implementation of the display setup dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mainfrm.h"
#include "ColorConfigDlg.h"
#include "Settings.h"
#include "FileDialog.h"
#include "ColorSchemes.h"
#include "../common/mptStringBuffer.h"


OPENMPT_NAMESPACE_BEGIN

static constexpr struct ColorDescriptions
{
	const TCHAR *name;
	int previewImage;
	ModColor colorIndex[3];
	const TCHAR *descText[3];
} colorDefs[] =
{
	{ _T("Pattern Editor"),      0, { MODCOLOR_BACKNORMAL, MODCOLOR_TEXTNORMAL, MODCOLOR_BACKHILIGHT }, { _T("Background:"), _T("Foreground:"), _T("Highlighted:") } },
	{ _T("Active Row"),          0, { MODCOLOR_BACKCURROW, MODCOLOR_TEXTCURROW, {} }, { _T("Background:"), _T("Foreground:"), nullptr } },
	{ _T("Pattern Selection"),   0, { MODCOLOR_BACKSELECTED, MODCOLOR_TEXTSELECTED, {} }, { _T("Background:"), _T("Foreground:"), nullptr } },
	{ _T("Play Cursor"),         0, { MODCOLOR_BACKPLAYCURSOR, MODCOLOR_TEXTPLAYCURSOR, {} }, { _T("Background:"), _T("Foreground:"), nullptr } },
	{ _T("Note Highlight"),      0, { MODCOLOR_NOTE, MODCOLOR_INSTRUMENT, MODCOLOR_VOLUME }, { _T("Note:"), _T("Instrument:"), _T("Volume:") } },
	{ _T("Effect Highlight"),    0, { MODCOLOR_PANNING, MODCOLOR_PITCH, MODCOLOR_GLOBALS }, { _T("Panning Effects:"), _T("Pitch Effects:"), _T("Global Effects:") } },
	{ _T("Invalid Commands"),    0, { MODCOLOR_DODGY_COMMANDS, {}, {} }, { _T("Invalid Note:"), nullptr, nullptr } },
	{ _T("Channel Separator"),   0, { MODCOLOR_SEPHILITE, MODCOLOR_SEPFACE, MODCOLOR_SEPSHADOW }, { _T("Highlight:"), _T("Face:"), _T("Shadow:") } },
	{ _T("Next/Prev Pattern"),   0, { MODCOLOR_BLENDCOLOR, {}, {} }, { _T("Blend Colour:"), nullptr, nullptr } },
	{ _T("Sample Waveform"),     1, { MODCOLOR_SAMPLE, MODCOLOR_BACKSAMPLE, MODCOLOR_SAMPLESELECTED }, { _T("Sample Data:"), _T("Background:"), _T("Selection:") } },
	{ _T("Sample Markers"),      1, { MODCOLOR_SAMPLE_LOOPMARKER, MODCOLOR_SAMPLE_SUSTAINMARKER, MODCOLOR_SAMPLE_CUEPOINT}, { _T("Loop Marker:"), _T("Sustain Marker:"), _T("Cue Point:") } },
	{ _T("Instrument Editor"),   2, { MODCOLOR_ENVELOPES, MODCOLOR_ENVELOPE_RELEASE, MODCOLOR_BACKENV }, { _T("Envelopes:"), _T("Release Envelope:"), _T("Background:") } },
	{ _T("VU-Meters"),           0, { MODCOLOR_VUMETER_HI, MODCOLOR_VUMETER_MED, MODCOLOR_VUMETER_LO }, { _T("Hi:"), _T("Med:"), _T("Lo:") } },
	{ _T("VU-Meters (Plugins)"), 0, { MODCOLOR_VUMETER_HI_VST, MODCOLOR_VUMETER_MED_VST, MODCOLOR_VUMETER_LO_VST }, { _T("Hi:"), _T("Med:"), _T("Lo:") } }
};

#define PREVIEWBMP_WIDTH	88
#define PREVIEWBMP_HEIGHT	39


BEGIN_MESSAGE_MAP(COptionsColors, CPropertyPage)
	ON_WM_DRAWITEM()
	ON_WM_VSCROLL()
	ON_CBN_SELCHANGE(IDC_COMBO1,		&COptionsColors::OnColorSelChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,		&COptionsColors::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,		&COptionsColors::OnPresetChange)
	ON_EN_CHANGE(IDC_PRIMARYHILITE,		&COptionsColors::OnSettingsChanged)
	ON_EN_CHANGE(IDC_SECONDARYHILITE,	&COptionsColors::OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON1,				&COptionsColors::OnSelectColor1)
	ON_COMMAND(IDC_BUTTON2,				&COptionsColors::OnSelectColor2)
	ON_COMMAND(IDC_BUTTON3,				&COptionsColors::OnSelectColor3)
	ON_COMMAND(IDC_BUTTON9,				&COptionsColors::OnChoosePatternFont)
	ON_COMMAND(IDC_BUTTON10,			&COptionsColors::OnChooseCommentFont)
	ON_COMMAND(IDC_BUTTON11,			&COptionsColors::OnClearWindowCache)
	ON_COMMAND(IDC_LOAD_COLORSCHEME,	&COptionsColors::OnLoadColorScheme)
	ON_COMMAND(IDC_SAVE_COLORSCHEME,	&COptionsColors::OnSaveColorScheme)
	ON_COMMAND(IDC_CHECK1,				&COptionsColors::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,				&COptionsColors::OnPreviewChanged)
	ON_COMMAND(IDC_CHECK3,				&COptionsColors::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK4,				&COptionsColors::OnPreviewChanged)
	ON_COMMAND(IDC_CHECK5,				&COptionsColors::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO1,				&COptionsColors::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO2,				&COptionsColors::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO3,				&COptionsColors::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO4,				&COptionsColors::OnSettingsChanged)
END_MESSAGE_MAP()


void COptionsColors::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsColors)
	DDX_Control(pDX, IDC_COMBO1,		m_ComboItem);
	DDX_Control(pDX, IDC_COMBO2,		m_ComboFont);
	DDX_Control(pDX, IDC_COMBO3,		m_ComboPreset);
	DDX_Control(pDX, IDC_BUTTON4,		m_BtnPreview);
	DDX_Control(pDX, IDC_TEXT1,			m_TxtColor[0]);
	DDX_Control(pDX, IDC_TEXT2,			m_TxtColor[1]);
	DDX_Control(pDX, IDC_TEXT3,			m_TxtColor[2]);
	DDX_Control(pDX, IDC_SPIN1,			m_ColorSpin);
	//}}AFX_DATA_MAP
}


COptionsColors::COptionsColors()
    : CPropertyPage(IDD_OPTIONS_COLORS)
    , CustomColors(TrackerSettings::Instance().rgbCustomColors)
{
}

static CString FormatFontName(const FontSetting &font)
{
	return mpt::ToCString(font.name + U_(", ") + mpt::ufmt::val(font.size / 10));
}


BOOL COptionsColors::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	m_pPreviewDib = LoadDib(MAKEINTRESOURCE(IDB_COLORSETUP));
	for (size_t i = 0; i < std::size(colorDefs); i++)
	{
		m_ComboItem.SetItemData(m_ComboItem.AddString(colorDefs[i].name), i);
	}
	m_ComboItem.SetCurSel(0);

	m_BtnColor[0].SubclassDlgItem(IDC_BUTTON1, this);
	m_BtnColor[1].SubclassDlgItem(IDC_BUTTON2, this);
	m_BtnColor[2].SubclassDlgItem(IDC_BUTTON3, this);

	m_BtnPreview.SetWindowPos(nullptr,
		0, 0,
		Util::ScalePixels(PREVIEWBMP_WIDTH * 2, m_hWnd) + 2, Util::ScalePixels(PREVIEWBMP_HEIGHT * 2, m_hWnd) + 2,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_STDHIGHLIGHT) CheckDlgButton(IDC_CHECK1, BST_CHECKED);
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_EFFECTHILIGHT) CheckDlgButton(IDC_CHECK2, BST_CHECKED);
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_2NDHIGHLIGHT) CheckDlgButton(IDC_CHECK4, BST_CHECKED);
	CheckDlgButton(IDC_CHECK5, TrackerSettings::Instance().rememberSongWindows ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(IDC_PRIMARYHILITE, TrackerSettings::Instance().m_nRowHighlightMeasures);
	SetDlgItemInt(IDC_SECONDARYHILITE, TrackerSettings::Instance().m_nRowHighlightBeats);
	CheckRadioButton(IDC_RADIO1, IDC_RADIO2, TrackerSettings::Instance().accidentalFlats ? IDC_RADIO2 : IDC_RADIO1);
	CheckRadioButton(IDC_RADIO3, IDC_RADIO4, TrackerSettings::Instance().defaultRainbowChannelColors ? IDC_RADIO4 : IDC_RADIO3);

	patternFont = TrackerSettings::Instance().patternFont;
	m_ComboFont.AddString(_T("Built-in (small)"));
	m_ComboFont.AddString(_T("Built-in (large)"));
	m_ComboFont.AddString(_T("Built-in (small, x2)"));
	m_ComboFont.AddString(_T("Built-in (large, x2)"));
	m_ComboFont.AddString(_T("Built-in (small, x3)"));
	m_ComboFont.AddString(_T("Built-in (large, x3)"));
	int sel = 0;
	if(patternFont.name == PATTERNFONT_SMALL)
	{
		sel = patternFont.size * 2;
	} else if(patternFont.name == PATTERNFONT_LARGE)
	{
		sel = patternFont.size * 2 + 1;
	} else
	{
		m_ComboFont.AddString(FormatFontName(patternFont));
		sel = 6;
	}
	m_ComboFont.SetCurSel(sel);

	commentFont = TrackerSettings::Instance().commentsFont;
	SetDlgItemText(IDC_BUTTON10, FormatFontName(commentFont));

	m_ComboPreset.SetRedraw(FALSE);
	m_ComboPreset.InitStorage(static_cast<int>(2 + std::size(ColorSchemes)), 20 * sizeof(TCHAR));
	m_ComboPreset.AddString(_T("Choose a Colour Scheme..."));
	m_ComboPreset.AddString(_T("OpenMPT (Default)"));
	for(const auto &preset : ColorSchemes)
	{
		m_ComboPreset.SetItemDataPtr(m_ComboPreset.AddString(preset.name), const_cast<ColorScheme *>(&preset));
	}
	m_ComboPreset.SetCurSel(0);
	m_ComboPreset.SetRedraw(TRUE);

	m_ColorSpin.SetRange32(-1, 1);
	
	OnColorSelChanged();
	return TRUE;
}


BOOL COptionsColors::OnKillActive()
{
	int highlightMeasures = GetDlgItemInt(IDC_PRIMARYHILITE);
	int highlightBeats = GetDlgItemInt(IDC_SECONDARYHILITE);

	if(highlightBeats > highlightMeasures)
	{
		Reporting::Warning("Error: Primary highlight must be greater than or equal secondary highlight.");
		::SetFocus(::GetDlgItem(m_hWnd, IDC_PRIMARYHILITE));
		return 0;
	}

	return CPropertyPage::OnKillActive();
}


void COptionsColors::OnOK()
{
	TrackerSettings::Instance().m_dwPatternSetup &= ~(PATTERN_STDHIGHLIGHT|PATTERN_2NDHIGHLIGHT|PATTERN_EFFECTHILIGHT);
	if(IsDlgButtonChecked(IDC_CHECK1)) TrackerSettings::Instance().m_dwPatternSetup |= PATTERN_STDHIGHLIGHT;
	if(IsDlgButtonChecked(IDC_CHECK2)) TrackerSettings::Instance().m_dwPatternSetup |= PATTERN_EFFECTHILIGHT;
	if(IsDlgButtonChecked(IDC_CHECK4)) TrackerSettings::Instance().m_dwPatternSetup |= PATTERN_2NDHIGHLIGHT;
	TrackerSettings::Instance().rememberSongWindows = IsDlgButtonChecked(IDC_CHECK5) != BST_UNCHECKED;
	TrackerSettings::Instance().accidentalFlats = IsDlgButtonChecked(IDC_RADIO2) != BST_UNCHECKED;
	TrackerSettings::Instance().defaultRainbowChannelColors = IsDlgButtonChecked(IDC_RADIO4) != BST_UNCHECKED;

	FontSetting newPatternFont = patternFont;
	const int fontSel = m_ComboFont.GetCurSel();
	switch(fontSel)
	{
	case 0:
	case 2:
	case 4:
	default:
		newPatternFont.name = PATTERNFONT_SMALL;
		newPatternFont.size = fontSel / 2;
		break;
	case 1:
	case 3:
	case 5:
		newPatternFont.name = PATTERNFONT_LARGE;
		newPatternFont.size = fontSel / 2;
		break;
	case 6:
		break;
	}
	TrackerSettings::Instance().patternFont = newPatternFont;
	TrackerSettings::Instance().commentsFont = commentFont;

	TrackerSettings::Instance().m_nRowHighlightMeasures = GetDlgItemInt(IDC_PRIMARYHILITE);
	TrackerSettings::Instance().m_nRowHighlightBeats = GetDlgItemInt(IDC_SECONDARYHILITE);

	TrackerSettings::Instance().rgbCustomColors = CustomColors;
	CMainFrame::UpdateColors();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->PostMessage(WM_MOD_INVALIDATEPATTERNS, HINT_MPTOPTIONS);
	CSoundFile::SetDefaultNoteNames();
	CPropertyPage::OnOK();
}


BOOL COptionsColors::OnSetActive()
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_COLORS;
	return CPropertyPage::OnSetActive();
}


void COptionsColors::OnChoosePatternFont()
{
	LOGFONT lf;
	MemsetZero(lf);
	const int32 size = patternFont.size < 10 ? 120 : patternFont.size;
	// Point size to pixels
	lf.lfHeight = -MulDiv(size, Util::GetDPIy(m_hWnd), 720);
	lf.lfWeight = patternFont.flags[FontSetting::Bold] ? FW_BOLD : FW_NORMAL;
	lf.lfItalic = patternFont.flags[FontSetting::Italic] ? TRUE : FALSE;
	mpt::String::WriteWinBuf(lf.lfFaceName) = mpt::ToWin(patternFont.name);
	CFontDialog dlg(&lf);
	dlg.m_cf.hwndOwner = m_hWnd;
	if(patternFont.name != PATTERNFONT_SMALL && patternFont.name != PATTERNFONT_LARGE)
	{
		dlg.m_cf.lpLogFont = &lf;
	}
	dlg.m_cf.Flags &= ~CF_EFFECTS;
	dlg.m_cf.Flags |= /*CF_FIXEDPITCHONLY | */CF_FORCEFONTEXIST | CF_NOSCRIPTSEL;
	if(dlg.DoModal() == IDOK)
	{
		while(m_ComboFont.GetCount() > 6)
		{
			m_ComboFont.DeleteString(6);
		}
		patternFont.name = mpt::ToUnicode(dlg.GetFaceName());
		patternFont.size = dlg.GetSize();
		patternFont.flags = FontSetting::None;
		if(dlg.IsBold()) patternFont.flags |= FontSetting::Bold;
		if(dlg.IsItalic()) patternFont.flags |= FontSetting::Italic;
		m_ComboFont.AddString(FormatFontName(patternFont));
		m_ComboFont.SetCurSel(6);
		OnSettingsChanged();
	}
}


void COptionsColors::OnChooseCommentFont()
{
	LOGFONT lf;
	MemsetZero(lf);
	// Point size to pixels
	lf.lfHeight = -MulDiv(commentFont.size, Util::GetDPIy(m_hWnd), 720);
	lf.lfWeight = commentFont.flags[FontSetting::Bold] ? FW_BOLD : FW_NORMAL;
	lf.lfItalic = commentFont.flags[FontSetting::Italic] ? TRUE : FALSE;
	mpt::String::WriteWinBuf(lf.lfFaceName) = mpt::ToWin(commentFont.name);
	CFontDialog dlg(&lf);
	dlg.m_cf.hwndOwner = m_hWnd;
	dlg.m_cf.lpLogFont = &lf;
	dlg.m_cf.Flags &= ~CF_EFFECTS;
	dlg.m_cf.Flags |= CF_FORCEFONTEXIST | CF_NOSCRIPTSEL;
	if(dlg.DoModal() == IDOK)
	{
		commentFont.name = mpt::ToUnicode(dlg.GetFaceName());
		commentFont.size = dlg.GetSize();
		commentFont.flags = FontSetting::None;
		if(dlg.IsBold()) commentFont.flags |= FontSetting::Bold;
		if(dlg.IsItalic()) commentFont.flags |= FontSetting::Italic;
		SetDlgItemText(IDC_BUTTON10, FormatFontName(commentFont));
		OnSettingsChanged();
	}
}


void COptionsColors::OnDrawItem(int nIdCtl, LPDRAWITEMSTRUCT lpdis)
{
	if(!lpdis || nIdCtl != IDC_BUTTON4 || !m_pPreviewDib)
	{
		CDialog::OnDrawItem(nIdCtl, lpdis);
		return;
	}

	const int img = colorDefs[m_nColorItem].previewImage;
	auto &p = m_pPreviewDib->bmiColors;
	if (IsDlgButtonChecked(IDC_CHECK2))
	{
		p[1] = rgb2quad(CustomColors[MODCOLOR_GLOBALS]);
		p[3] = rgb2quad(CustomColors[MODCOLOR_PITCH]);
		p[5] = rgb2quad(CustomColors[MODCOLOR_INSTRUMENT]);
		p[6] = rgb2quad(CustomColors[MODCOLOR_VOLUME]);
		p[12] = rgb2quad(CustomColors[MODCOLOR_NOTE]);
		p[14] = rgb2quad(CustomColors[MODCOLOR_PANNING]);
	} else
	{
		p[1] = rgb2quad(CustomColors[MODCOLOR_TEXTNORMAL]);
		p[3] = rgb2quad(CustomColors[MODCOLOR_TEXTNORMAL]);
		p[5] = rgb2quad(CustomColors[MODCOLOR_TEXTNORMAL]);
		p[6] = rgb2quad(CustomColors[MODCOLOR_TEXTNORMAL]);
		p[12] = rgb2quad(CustomColors[MODCOLOR_TEXTNORMAL]);
		p[14] = rgb2quad(CustomColors[MODCOLOR_TEXTNORMAL]);
	}
	p[4] = rgb2quad(CustomColors[MODCOLOR_TEXTNORMAL]);
	p[8] = rgb2quad(CustomColors[MODCOLOR_TEXTNORMAL]);
	p[9] = rgb2quad(CustomColors[MODCOLOR_SAMPLE]);
	p[10] = rgb2quad(CustomColors[MODCOLOR_BACKNORMAL]);
	p[11] = rgb2quad(CustomColors[MODCOLOR_BACKHILIGHT]);
	p[13] = rgb2quad(CustomColors[MODCOLOR_ENVELOPES]);
	p[15] = rgb2quad(img ? RGB(255, 255, 255) : CustomColors[MODCOLOR_BACKNORMAL]);
	// Special cases: same bitmap, different palette
	switch(m_nColorItem)
	{
	// Current Row
	case 1:
		p[8] = rgb2quad(CustomColors[MODCOLOR_TEXTCURROW]);
		p[11] = rgb2quad(CustomColors[MODCOLOR_BACKCURROW]);
		break;
	// Selection
	case 2:
		p[5] = rgb2quad(CustomColors[MODCOLOR_TEXTSELECTED]);
		p[6] = rgb2quad(CustomColors[MODCOLOR_TEXTSELECTED]);
		p[8] = rgb2quad(CustomColors[MODCOLOR_TEXTSELECTED]);
		p[11] = rgb2quad(CustomColors[MODCOLOR_BACKSELECTED]);
		p[12] = rgb2quad(CustomColors[MODCOLOR_TEXTSELECTED]);
		break;
	// Play Cursor
	case 3:
		p[8] = rgb2quad(CustomColors[MODCOLOR_TEXTPLAYCURSOR]);
		p[11] = rgb2quad(CustomColors[MODCOLOR_BACKPLAYCURSOR]);
		break;
	// Sample Editor
	case 9:
	case 10:
		p[0] = rgb2quad(CustomColors[MODCOLOR_BACKSAMPLE]);
		p[7] = rgb2quad(GetSysColor(COLOR_BTNFACE));
		p[8] = rgb2quad(GetSysColor(COLOR_BTNSHADOW));
		p[10] = rgb2quad(CustomColors[MODCOLOR_SAMPLE_LOOPMARKER]);
		p[11] = rgb2quad(CustomColors[MODCOLOR_SAMPLE_CUEPOINT]);
		p[14] = rgb2quad(CustomColors[MODCOLOR_SAMPLE_SUSTAINMARKER]);
		p[15] = rgb2quad(CustomColors[MODCOLOR_SAMPLESELECTED]);
		break;
	// Envelope Editor
	case 11:
		p[0] = rgb2quad(CustomColors[MODCOLOR_BACKENV]);
		p[2] = rgb2quad(CustomColors[MODCOLOR_ENVELOPE_RELEASE]);
		break;
	}

	CRect rect = lpdis->rcItem;
	HDC hdc = lpdis->hDC;
	::DrawEdge(hdc, rect, BDR_SUNKENINNER, BF_RECT | BF_ADJUST);
	StretchDIBits(	hdc,
					rect.left,
					rect.top,
					rect.Width(),
					rect.Height(),
					0,
					m_pPreviewDib->bmiHeader.biHeight - ((img+1) * PREVIEWBMP_HEIGHT),
					m_pPreviewDib->bmiHeader.biWidth,
					PREVIEWBMP_HEIGHT,
					m_pPreviewDib->lpDibBits,
					(LPBITMAPINFO)m_pPreviewDib,
					DIB_RGB_COLORS,
					SRCCOPY);
}


static COLORREF rgbCustomColors[16] =
{
	0x808080,	0x0000FF,	0x00FF00,	0x00FFFF,
	0xFF0000,	0xFF00FF,	0xFFFF00,	0xFFFFFF,
	0xC0C0C0,	0x80FFFF,	0xE0E8E0,	0x606060,
	0x505050,	0x404040,	0x004000,	0x000000,
};


void COptionsColors::SelectColor(int colorIndex)
{
	auto &color = CustomColors[colorDefs[m_nColorItem].colorIndex[colorIndex]];
	CHOOSECOLOR cc;
	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = m_hWnd;
	cc.hInstance = NULL;
	cc.rgbResult = color;
	cc.lpCustColors = rgbCustomColors;
	cc.Flags = CC_RGBINIT | CC_FULLOPEN;
	cc.lCustData = 0;
	cc.lpfnHook = nullptr;
	cc.lpTemplateName = nullptr;
	if(::ChooseColor(&cc))
	{
		color = cc.rgbResult;
		m_BtnColor[colorIndex].SetColor(color);
		OnSettingsChanged();
	}
}


void COptionsColors::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
	CPropertyPage::OnVScroll(nSBCode, nPos, pScrollBar);
	int newSel = m_ComboItem.GetCurSel() - m_ColorSpin.GetPos32();
	if(newSel >= 0)
	{
		m_ComboItem.SetCurSel(newSel);
		OnColorSelChanged();
	}
	m_ColorSpin.SetPos(0);
}


void COptionsColors::OnColorSelChanged()
{
	int sel = m_ComboItem.GetCurSel();
	if (sel >= 0)
	{
		m_nColorItem = static_cast<uint32>(m_ComboItem.GetItemData(sel));
		OnUpdateDialog();
	}
}

void COptionsColors::OnSettingsChanged()
{
	SetModified(TRUE);
}

void COptionsColors::OnUpdateDialog()
{
	const ColorDescriptions &cd = colorDefs[m_nColorItem];
	for(int i = 0; i < 3; i++)
	{
		if(cd.descText[i])
		{
			m_TxtColor[i].SetWindowText(cd.descText[i]);
			m_BtnColor[i].SetColor(CustomColors[cd.colorIndex[i]]);
		}
		m_TxtColor[i].ShowWindow(cd.descText[i] ? SW_SHOW : SW_HIDE);
		m_BtnColor[i].ShowWindow(cd.descText[i] ? SW_SHOW : SW_HIDE);
	}

	m_BtnPreview.Invalidate(FALSE);
}


void COptionsColors::OnPreviewChanged()
{
	OnSettingsChanged();
	m_BtnPreview.Invalidate(FALSE);
	for(int i = 0; i < 3; i++)
	{
		m_BtnColor[i].SetColor(CustomColors[colorDefs[m_nColorItem].colorIndex[i]]);
	}
}


void COptionsColors::OnPresetChange()
{
	auto curSel = m_ComboPreset.GetCurSel();
	if(curSel == 0)
		return;
	TrackerSettings::GetDefaultColourScheme(CustomColors);
	auto scheme = static_cast<const ColorScheme *>(m_ComboPreset.GetItemDataPtr(curSel));
	if(scheme != nullptr)
	{
		for(const auto &c : scheme->colors)
		{
			CustomColors[c.id] = c.color;
		}
	}
	OnPreviewChanged();
}


void COptionsColors::OnLoadColorScheme()
{
	FileDialog dlg = OpenFileDialog()
		.DefaultExtension("mptcolor")
		.ExtensionFilter("OpenMPT Color Schemes|*.mptcolor||")
		.WorkingDirectory(theApp.GetConfigPath());
	if(!dlg.Show(this)) return;

	// Ensure that all colours are reset (for outdated colour schemes)
	TrackerSettings::GetDefaultColourScheme(CustomColors);
	{
		IniFileSettingsContainer file(dlg.GetFirstFile());
		for(uint32 i = 0; i < MAX_MODCOLORS; i++)
		{
			CustomColors[i] = file.Read<int32>(U_("Colors"), MPT_UFORMAT("Color{}")(mpt::ufmt::dec0<2>(i)), CustomColors[i]);
		}
	}
	OnPreviewChanged();
}

void COptionsColors::OnSaveColorScheme()
{
	FileDialog dlg = SaveFileDialog()
		.DefaultExtension("mptcolor")
		.ExtensionFilter("OpenMPT Color Schemes|*.mptcolor||")
		.WorkingDirectory(theApp.GetConfigPath());
	if(!dlg.Show(this)) return;

	{
		IniFileSettingsContainer file(dlg.GetFirstFile());
		for(uint32 i = 0; i < MAX_MODCOLORS; i++)
		{
			file.Write<int32>(U_("Colors"), MPT_UFORMAT("Color{}")(mpt::ufmt::dec0<2>(i)), CustomColors[i]);
		}
	}
}


void COptionsColors::OnClearWindowCache()
{
	SettingsContainer &settings = theApp.GetSongSettings();
	// First, forget all settings...
	settings.ForgetAll();
	// Then make sure they are gone for good.
	::DeleteFile(theApp.GetSongSettingsFilename().AsNative().c_str());
}


OPENMPT_NAMESPACE_END
