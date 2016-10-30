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
#include "../common/StringFixer.h"


OPENMPT_NAMESPACE_BEGIN

static const struct ColorDescriptions
{
	const char *name;
	int previewImage;
	uint32 colorIndex1, colorIndex2, colorIndex3;
	const char *descText1, *descText2, *descText3;
} colorDefs[] =
{
	{"Pattern Editor",		0,	MODCOLOR_BACKNORMAL, MODCOLOR_TEXTNORMAL, MODCOLOR_BACKHILIGHT, "Background:", "Foreground:", "Highlighted:"},
	{"Active Row",			0,	MODCOLOR_BACKCURROW, MODCOLOR_TEXTCURROW, 0, "Background:", "Foreground:", nullptr },
	{"Pattern Selection",	0,	MODCOLOR_BACKSELECTED, MODCOLOR_TEXTSELECTED, 0, "Background:", "Foreground:", nullptr },
	{"Play Cursor",			0,	MODCOLOR_BACKPLAYCURSOR, MODCOLOR_TEXTPLAYCURSOR, 0, "Background:", "Foreground:", nullptr },
	{"Note Highlight",		0,	MODCOLOR_NOTE, MODCOLOR_INSTRUMENT, MODCOLOR_VOLUME, "Note:", "Instrument:", "Volume:"},
	{"Effect Highlight",	0,	MODCOLOR_PANNING, MODCOLOR_PITCH, MODCOLOR_GLOBALS, "Panning Effects:", "Pitch Effects:", "Global Effects:"},
	{"Invalid Commands",	0,	MODCOLOR_DODGY_COMMANDS, 0, 0, "Invalid Note:", NULL, NULL},
	{"Channel Separator",	0,	MODCOLOR_SEPHILITE, MODCOLOR_SEPFACE, MODCOLOR_SEPSHADOW, "Highlight:", "Face:", "Shadow:"},
	{"Next/Prev Pattern",	0,	MODCOLOR_BLENDCOLOR, 0, 0, "Blend Colour:", NULL, NULL},
	{"Sample Editor",		1,	MODCOLOR_SAMPLE, MODCOLOR_BACKSAMPLE, MODCOLOR_SAMPLESELECTED, "Sample Data:", "Background:", "Selection:"},
	{"Instrument Editor",	2,	MODCOLOR_ENVELOPES, MODCOLOR_BACKENV, 0, "Envelopes:", "Background:", nullptr },
	{"VU-Meters",			0,	MODCOLOR_VUMETER_HI, MODCOLOR_VUMETER_MED, MODCOLOR_VUMETER_LO, "Hi:", "Med:", "Lo:"},
	{"VU-Meters (Plugins)",	0,	MODCOLOR_VUMETER_HI_VST, MODCOLOR_VUMETER_MED_VST, MODCOLOR_VUMETER_LO_VST, "Hi:", "Med:", "Lo:"}
};

#define PREVIEWBMP_WIDTH	88
#define PREVIEWBMP_HEIGHT	39


BEGIN_MESSAGE_MAP(COptionsColors, CPropertyPage)
	ON_WM_DRAWITEM()
	ON_CBN_SELCHANGE(IDC_COMBO1,		OnColorSelChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,		OnSettingsChanged)
	ON_EN_CHANGE(IDC_PRIMARYHILITE,		OnSettingsChanged)
	ON_EN_CHANGE(IDC_SECONDARYHILITE,	OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON1,				OnSelectColor1)
	ON_COMMAND(IDC_BUTTON2,				OnSelectColor2)
	ON_COMMAND(IDC_BUTTON3,				OnSelectColor3)
	ON_COMMAND(IDC_BUTTON5,				OnPresetMPT)
	ON_COMMAND(IDC_BUTTON6,				OnPresetFT2)
	ON_COMMAND(IDC_BUTTON7,				OnPresetIT)
	ON_COMMAND(IDC_BUTTON8,				OnPresetBuzz)
	ON_COMMAND(IDC_BUTTON9,				OnChoosePatternFont)
	ON_COMMAND(IDC_BUTTON10,			OnChooseCommentFont)
	ON_COMMAND(IDC_BUTTON11,			OnClearWindowCache)
	ON_COMMAND(IDC_LOAD_COLORSCHEME,	OnLoadColorScheme)
	ON_COMMAND(IDC_SAVE_COLORSCHEME,	OnSaveColorScheme)
	ON_COMMAND(IDC_CHECK1,				OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,				OnPreviewChanged)
	ON_COMMAND(IDC_CHECK3,				OnSettingsChanged)
	ON_COMMAND(IDC_CHECK4,				OnPreviewChanged)
	ON_COMMAND(IDC_CHECK5,				OnSettingsChanged)
	ON_COMMAND(IDC_RADIO1,				OnSettingsChanged)
	ON_COMMAND(IDC_RADIO2,				OnSettingsChanged)
END_MESSAGE_MAP()


void COptionsColors::DoDataExchange(CDataExchange* pDX)
//-----------------------------------------------------
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsColors)
	DDX_Control(pDX, IDC_COMBO1,		m_ComboItem);
	DDX_Control(pDX, IDC_COMBO2,		m_ComboFont);
	DDX_Control(pDX, IDC_BUTTON1,		m_BtnColor1);
	DDX_Control(pDX, IDC_BUTTON2,		m_BtnColor2);
	DDX_Control(pDX, IDC_BUTTON3,		m_BtnColor3);
	DDX_Control(pDX, IDC_BUTTON4,		m_BtnPreview);
	DDX_Control(pDX, IDC_TEXT1,			m_TxtColor1);
	DDX_Control(pDX, IDC_TEXT2,			m_TxtColor2);
	DDX_Control(pDX, IDC_TEXT3,			m_TxtColor3);
	//}}AFX_DATA_MAP
}


static std::string FormatFontName(const FontSetting &font)
//--------------------------------------------------------
{
	return font.name + ", " + mpt::ToString(font.size / 10);
}


BOOL COptionsColors::OnInitDialog()
//---------------------------------
{
	CPropertyPage::OnInitDialog();
	m_pPreviewDib = LoadDib(MAKEINTRESOURCE(IDB_COLORSETUP));
	MemCopy(CustomColors, TrackerSettings::Instance().rgbCustomColors);
	for (UINT i = 0; i < CountOf(colorDefs); i++)
	{
		m_ComboItem.SetItemData(m_ComboItem.AddString(colorDefs[i].name), i);
	}
	m_ComboItem.SetCurSel(0);
	m_BtnPreview.SetWindowPos(NULL,
		0, 0,
		Util::ScalePixels(PREVIEWBMP_WIDTH * 2, m_hWnd) + 2, Util::ScalePixels(PREVIEWBMP_HEIGHT * 2, m_hWnd) + 2,
		SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_STDHIGHLIGHT) CheckDlgButton(IDC_CHECK1, BST_CHECKED);
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_EFFECTHILIGHT) CheckDlgButton(IDC_CHECK2, BST_CHECKED);
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_2NDHIGHLIGHT) CheckDlgButton(IDC_CHECK4, BST_CHECKED);
	CheckDlgButton(IDC_CHECK5, TrackerSettings::Instance().rememberSongWindows ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(IDC_PRIMARYHILITE, TrackerSettings::Instance().m_nRowHighlightMeasures);
	SetDlgItemInt(IDC_SECONDARYHILITE, TrackerSettings::Instance().m_nRowHighlightBeats);
	CheckRadioButton(IDC_RADIO1, IDC_RADIO2, TrackerSettings::Instance().accidentalFlats ? IDC_RADIO2 : IDC_RADIO1);

	patternFont = TrackerSettings::Instance().patternFont;
	m_ComboFont.AddString("Built-in (small)");
	m_ComboFont.AddString("Built-in (large)");
	m_ComboFont.AddString("Built-in (small, x2)");
	m_ComboFont.AddString("Built-in (large, x2)");
	m_ComboFont.AddString("Built-in (small, x3)");
	m_ComboFont.AddString("Built-in (large, x3)");
	int sel = 0;
	if(patternFont.name == PATTERNFONT_SMALL)
	{
		sel = patternFont.size * 2;
	} else if(patternFont.name == PATTERNFONT_LARGE)
	{
		sel = patternFont.size * 2 + 1;
	} else
	{
		m_ComboFont.AddString(FormatFontName(patternFont).c_str());
		sel = 6;
	}
	m_ComboFont.SetCurSel(sel);

	commentFont = TrackerSettings::Instance().commentsFont;
	SetDlgItemText(IDC_BUTTON10, FormatFontName(commentFont).c_str());
	
	OnColorSelChanged();
	return TRUE;
}


BOOL COptionsColors::OnKillActive()
//---------------------------------
{
	int temp_nRowSpacing = GetDlgItemInt(IDC_PRIMARYHILITE);
	int temp_nRowSpacing2 = GetDlgItemInt(IDC_SECONDARYHILITE);

	if ((temp_nRowSpacing2 > temp_nRowSpacing))
	{
		Reporting::Warning("Error: Primary highlight must be greater than or equal secondary highlight.");
		::SetFocus(::GetDlgItem(m_hWnd, IDC_PRIMARYHILITE));
		return 0;
	}

	return CPropertyPage::OnKillActive();
}


void COptionsColors::OnOK()
//-------------------------
{
	TrackerSettings::Instance().m_dwPatternSetup &= ~(PATTERN_STDHIGHLIGHT|PATTERN_2NDHIGHLIGHT|PATTERN_EFFECTHILIGHT);
	if (IsDlgButtonChecked(IDC_CHECK1)) TrackerSettings::Instance().m_dwPatternSetup |= PATTERN_STDHIGHLIGHT;
	if (IsDlgButtonChecked(IDC_CHECK2)) TrackerSettings::Instance().m_dwPatternSetup |= PATTERN_EFFECTHILIGHT;
	if (IsDlgButtonChecked(IDC_CHECK4)) TrackerSettings::Instance().m_dwPatternSetup |= PATTERN_2NDHIGHLIGHT;
	TrackerSettings::Instance().rememberSongWindows = IsDlgButtonChecked(IDC_CHECK5) != BST_UNCHECKED;
	TrackerSettings::Instance().accidentalFlats = IsDlgButtonChecked(IDC_RADIO2) != BST_UNCHECKED;

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

	MemCopy(TrackerSettings::Instance().rgbCustomColors, CustomColors);
	CMainFrame::UpdateColors();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->PostMessage(WM_MOD_INVALIDATEPATTERNS, HINT_MPTOPTIONS);
	CSoundFile::SetDefaultNoteNames();
	CPropertyPage::OnOK();
}


BOOL COptionsColors::OnSetActive()
//--------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_COLORS;
	return CPropertyPage::OnSetActive();
}


void COptionsColors::OnChoosePatternFont()
//----------------------------------------
{
	LOGFONT lf;
	MemsetZero(lf);
	const int32_t size = patternFont.size < 10 ? 120 : patternFont.size;
	// Point size to pixels
	lf.lfHeight = -MulDiv(size, Util::GetDPIy(m_hWnd), 720);
	lf.lfWeight = patternFont.flags[FontSetting::Bold] ? FW_BOLD : FW_NORMAL;
	lf.lfItalic = patternFont.flags[FontSetting::Italic] ? TRUE : FALSE;
	mpt::String::Copy(lf.lfFaceName, patternFont.name);
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
		patternFont.name = dlg.GetFaceName();
		patternFont.size = dlg.GetSize();
		patternFont.flags = FontSetting::None;
		if(dlg.IsBold()) patternFont.flags |= FontSetting::Bold;
		if(dlg.IsItalic()) patternFont.flags |= FontSetting::Italic;
		m_ComboFont.AddString(FormatFontName(patternFont).c_str());
		m_ComboFont.SetCurSel(6);
		OnSettingsChanged();
	}
}


void COptionsColors::OnChooseCommentFont()
//----------------------------------------
{
	LOGFONT lf;
	MemsetZero(lf);
	// Point size to pixels
	lf.lfHeight = -MulDiv(commentFont.size, Util::GetDPIy(m_hWnd), 720);
	lf.lfWeight = commentFont.flags[FontSetting::Bold] ? FW_BOLD : FW_NORMAL;
	lf.lfItalic = commentFont.flags[FontSetting::Italic] ? TRUE : FALSE;
	mpt::String::Copy(lf.lfFaceName, commentFont.name);
	CFontDialog dlg(&lf);
	dlg.m_cf.hwndOwner = m_hWnd;
	dlg.m_cf.lpLogFont = &lf;
	dlg.m_cf.Flags &= ~CF_EFFECTS;
	dlg.m_cf.Flags |= CF_FORCEFONTEXIST | CF_NOSCRIPTSEL;
	if(dlg.DoModal() == IDOK)
	{
		commentFont.name = dlg.GetFaceName();
		commentFont.size = dlg.GetSize();
		commentFont.flags = FontSetting::None;
		if(dlg.IsBold()) commentFont.flags |= FontSetting::Bold;
		if(dlg.IsItalic()) commentFont.flags |= FontSetting::Italic;
		SetDlgItemText(IDC_BUTTON10, FormatFontName(commentFont).c_str());
		OnSettingsChanged();
	}
}


void COptionsColors::OnDrawItem(int nIdCtl, LPDRAWITEMSTRUCT lpdis)
//-----------------------------------------------------------------
{
	int nColor = -1;
	switch(nIdCtl)
	{
	case IDC_BUTTON1:	nColor = colorDefs[m_nColorItem].colorIndex1; break;
	case IDC_BUTTON2:	nColor = colorDefs[m_nColorItem].colorIndex2; break;
	case IDC_BUTTON3:	nColor = colorDefs[m_nColorItem].colorIndex3; break;
	}
	if (!lpdis) return;
	if (nColor >= 0)
	{
		HPEN pen1, pen2;
		pen1 = (HPEN)::GetStockObject(WHITE_PEN);
		pen2 = (HPEN)::GetStockObject(BLACK_PEN);
		if (lpdis->itemState & ODS_SELECTED)
		{
			HPEN pentmp = pen1;
			pen1 = pen2;
			pen2 = pentmp;
		}
		HDC hdc = lpdis->hDC;
		HBRUSH brush = ::CreateSolidBrush(CustomColors[nColor]);
		::FillRect(hdc, &lpdis->rcItem, brush);
		::DeleteObject(brush);
		HPEN oldpen = (HPEN)::SelectObject(hdc, pen1);
		::MoveToEx(hdc, lpdis->rcItem.left, lpdis->rcItem.bottom-1, NULL);
		::LineTo(hdc, lpdis->rcItem.left, lpdis->rcItem.top);
		::LineTo(hdc, lpdis->rcItem.right, lpdis->rcItem.top);
		::SelectObject(hdc, pen2);
		::MoveToEx(hdc, lpdis->rcItem.right-1, lpdis->rcItem.top, NULL);
		::LineTo(hdc, lpdis->rcItem.right-1, lpdis->rcItem.bottom-1);
		::LineTo(hdc, lpdis->rcItem.left, lpdis->rcItem.bottom-1);
		if (oldpen) ::SelectObject(hdc, oldpen);
	} else
	if ((nIdCtl == IDC_BUTTON4) && (m_pPreviewDib))
	{
		int y = colorDefs[m_nColorItem].previewImage;
		RGBQUAD *p = m_pPreviewDib->bmiColors;
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
		p[15] = rgb2quad((y) ? RGB(255,255,255) : CustomColors[MODCOLOR_BACKNORMAL]);
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
			p[0] = rgb2quad(CustomColors[MODCOLOR_BACKSAMPLE]);
			p[15] = rgb2quad(CustomColors[MODCOLOR_SAMPLESELECTED]);
			break;
		// Envelope Editor
		case 10:
			p[0] = rgb2quad(CustomColors[MODCOLOR_BACKENV]);
			break;
		}
		HDC hdc = lpdis->hDC;
		HPEN oldpen = (HPEN)::SelectObject(hdc, CMainFrame::penDarkGray);
		::MoveToEx(hdc, lpdis->rcItem.left, lpdis->rcItem.bottom-1, NULL);
		::LineTo(hdc, lpdis->rcItem.left, lpdis->rcItem.top);
		::LineTo(hdc, lpdis->rcItem.right, lpdis->rcItem.top);
		::SelectObject(hdc, CMainFrame::penLightGray);
		::MoveToEx(hdc, lpdis->rcItem.right-1, lpdis->rcItem.top, NULL);
		::LineTo(hdc, lpdis->rcItem.right-1, lpdis->rcItem.bottom-1);
		::LineTo(hdc, lpdis->rcItem.left, lpdis->rcItem.bottom-1);
		if (oldpen) ::SelectObject(hdc, oldpen);
		StretchDIBits(	hdc,
						lpdis->rcItem.left+1,
						lpdis->rcItem.top+1,
						lpdis->rcItem.right - lpdis->rcItem.left - 2,
						lpdis->rcItem.bottom - lpdis->rcItem.top - 2,
						0,
						m_pPreviewDib->bmiHeader.biHeight - ((y+1) * PREVIEWBMP_HEIGHT),
						m_pPreviewDib->bmiHeader.biWidth,
						PREVIEWBMP_HEIGHT,
						m_pPreviewDib->lpDibBits,
						(LPBITMAPINFO)m_pPreviewDib,
						DIB_RGB_COLORS,
						SRCCOPY);
	}
}


static DWORD rgbCustomColors[16] =
{
	0x808080,	0x0000FF,	0x00FF00,	0x00FFFF,
	0xFF0000,	0xFF00FF,	0xFFFF00,	0xFFFFFF,
	0xC0C0C0,	0x80FFFF,	0xE0E8E0,	0x606060,
	0x505050,	0x404040,	0x004000,	0x000000,
};


void COptionsColors::SelectColor(COLORREF *lprgb)
//-----------------------------------------------
{
	CHOOSECOLOR cc;
	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = m_hWnd;
	cc.hInstance = NULL;
	cc.rgbResult = *lprgb;
	cc.lpCustColors = rgbCustomColors;
	cc.Flags = CC_RGBINIT;
	cc.lCustData = 0;
	cc.lpfnHook = NULL;
	cc.lpTemplateName = NULL;
	if (::ChooseColor(&cc))
	{
		*lprgb = cc.rgbResult;
		InvalidateRect(NULL, FALSE);
		OnSettingsChanged();
	}
}


void COptionsColors::OnSelectColor1()
//-----------------------------------
{
	SelectColor(&CustomColors[colorDefs[m_nColorItem].colorIndex1]);
}


void COptionsColors::OnSelectColor2()
//-----------------------------------
{
	SelectColor(&CustomColors[colorDefs[m_nColorItem].colorIndex2]);
}


void COptionsColors::OnSelectColor3()
//-----------------------------------
{
	SelectColor(&CustomColors[colorDefs[m_nColorItem].colorIndex3]);
}


void COptionsColors::OnColorSelChanged()
//--------------------------------------
{
	int sel = m_ComboItem.GetCurSel();
	if (sel >= 0)
	{
		m_nColorItem = static_cast<uint32>(m_ComboItem.GetItemData(sel));
		OnUpdateDialog();
	}
}

void COptionsColors::OnSettingsChanged()
//--------------------------------------
{
	SetModified(TRUE);
}

void COptionsColors::OnUpdateDialog()
//-----------------------------------
{
	const ColorDescriptions &cd = colorDefs[m_nColorItem];
	if (cd.descText1) m_TxtColor1.SetWindowText(cd.descText1);
	if (cd.descText2)
	{
		m_TxtColor2.SetWindowText(cd.descText2);
		m_TxtColor2.ShowWindow(SW_SHOW);
		m_BtnColor2.ShowWindow(SW_SHOW);
		m_BtnColor2.InvalidateRect(NULL, FALSE);
	} else
	{
		m_TxtColor2.ShowWindow(SW_HIDE);
		m_BtnColor2.ShowWindow(SW_HIDE);
	}
	if (cd.descText3)
	{
		m_TxtColor3.SetWindowText(cd.descText3);
		m_TxtColor3.ShowWindow(SW_SHOW);
		m_BtnColor3.ShowWindow(SW_SHOW);
		m_BtnColor3.InvalidateRect(NULL, FALSE);
	} else
	{
		m_TxtColor3.ShowWindow(SW_HIDE);
		m_BtnColor3.ShowWindow(SW_HIDE);
	}
	m_BtnColor1.InvalidateRect(NULL, FALSE);
	m_BtnPreview.InvalidateRect(NULL, FALSE);
}


void COptionsColors::OnPreviewChanged()
//-------------------------------------
{
	OnSettingsChanged();
	m_BtnPreview.InvalidateRect(NULL, FALSE);
	m_BtnColor1.InvalidateRect(NULL, FALSE);
	m_BtnColor2.InvalidateRect(NULL, FALSE);
	m_BtnColor3.InvalidateRect(NULL, FALSE);
}

void COptionsColors::OnPresetMPT()
//--------------------------------
{
	TrackerSettings::GetDefaultColourScheme(CustomColors);
	OnPreviewChanged();
}


void COptionsColors::OnPresetFT2()
//--------------------------------
{
	// "blue"
	CustomColors[MODCOLOR_BACKNORMAL] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_TEXTNORMAL] = RGB(0xE0, 0xE0, 0x40);
	CustomColors[MODCOLOR_BACKCURROW] = RGB(0x70, 0x70, 0x70);
	CustomColors[MODCOLOR_TEXTCURROW] = RGB(0xF0, 0xF0, 0x50);
	CustomColors[MODCOLOR_BACKSELECTED] = RGB(0x40, 0x40, 0xA0);
	CustomColors[MODCOLOR_TEXTSELECTED] = RGB(0xFF, 0xFF, 0xFF);
	CustomColors[MODCOLOR_SAMPLE] = RGB(0xFF, 0x00, 0x00);
	CustomColors[MODCOLOR_BACKPLAYCURSOR] = RGB(0x50, 0x50, 0x70);
	CustomColors[MODCOLOR_TEXTPLAYCURSOR] = RGB(0xE0, 0xE0, 0x40);
	CustomColors[MODCOLOR_BACKHILIGHT] = RGB(0x40, 0x40, 0x80);
	CustomColors[MODCOLOR_NOTE] = RGB(0xE0, 0xE0, 0x40);
	CustomColors[MODCOLOR_INSTRUMENT] = RGB(0xFF, 0xFF, 0x00);
	CustomColors[MODCOLOR_VOLUME] = RGB(0x00, 0xFF, 0x00);
	CustomColors[MODCOLOR_PANNING] = RGB(0x00, 0xFF, 0xFF);
	CustomColors[MODCOLOR_PITCH] = RGB(0xFF, 0xFF, 0x00);
	CustomColors[MODCOLOR_GLOBALS] = RGB(0xFF, 0x40, 0x40);
	CustomColors[MODCOLOR_ENVELOPES] = RGB(0x00, 0x00, 0xFF);
	CustomColors[MODCOLOR_VUMETER_LO] = RGB(0x00, 0xC8, 0x00);
	CustomColors[MODCOLOR_VUMETER_MED] = RGB(0xFF, 0xC8, 0x00);
	CustomColors[MODCOLOR_VUMETER_HI] = RGB(0xE1, 0x00, 0x00);
	CustomColors[MODCOLOR_VUMETER_LO_VST] = RGB(0x18, 0x96, 0xE1);
	CustomColors[MODCOLOR_VUMETER_MED_VST] = RGB(0xFF, 0xC8, 0x00);
	CustomColors[MODCOLOR_VUMETER_HI_VST] = RGB(0xE1, 0x00, 0x00);
	CustomColors[MODCOLOR_SEPSHADOW] = RGB(0x2E, 0x2E, 0x5C);
	CustomColors[MODCOLOR_SEPFACE] = RGB(0x40, 0x40, 0x80);
	CustomColors[MODCOLOR_SEPHILITE] = RGB(0x99, 0x99, 0xCC);
	CustomColors[MODCOLOR_BLENDCOLOR] = RGB(0x2E, 0x2E, 0x5A);
	CustomColors[MODCOLOR_DODGY_COMMANDS] = RGB(0xC0, 0x40, 0x40);
	CustomColors[MODCOLOR_BACKSAMPLE] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_SAMPLESELECTED] = RGB(0xFF, 0xFF, 0xFF);
	CustomColors[MODCOLOR_BACKENV] = RGB(0x00, 0x00, 0x00);
	OnPreviewChanged();
}


void COptionsColors::OnPresetIT()
//-------------------------------
{
	// "green"
	CustomColors[MODCOLOR_BACKNORMAL] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_TEXTNORMAL] = RGB(0x00, 0xE0, 0x00);
	CustomColors[MODCOLOR_BACKCURROW] = RGB(0x70, 0x70, 0x70);
	CustomColors[MODCOLOR_TEXTCURROW] = RGB(0x00, 0xE0, 0x00);
	CustomColors[MODCOLOR_BACKSELECTED] = RGB(0xE0, 0xE0, 0xE0);
	CustomColors[MODCOLOR_TEXTSELECTED] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_SAMPLE] = RGB(0xFF, 0x00, 0x00);
	CustomColors[MODCOLOR_BACKPLAYCURSOR] = RGB(0x80, 0x80, 0x00);
	CustomColors[MODCOLOR_TEXTPLAYCURSOR] = RGB(0x00, 0xE0, 0x00);
	CustomColors[MODCOLOR_BACKHILIGHT] = RGB(0x40, 0x68, 0x40);
	CustomColors[MODCOLOR_NOTE] = RGB(0x00, 0xFF, 0x00);
	CustomColors[MODCOLOR_INSTRUMENT] = RGB(0xFF, 0xFF, 0x00);
	CustomColors[MODCOLOR_VOLUME] = RGB(0x00, 0xFF, 0x00);
	CustomColors[MODCOLOR_PANNING] = RGB(0x00, 0xFF, 0xFF);
	CustomColors[MODCOLOR_PITCH] = RGB(0xFF, 0xFF, 0x00);
	CustomColors[MODCOLOR_GLOBALS] = RGB(0xFF, 0x40, 0x40);
	CustomColors[MODCOLOR_ENVELOPES] = RGB(0x00, 0x00, 0xFF);
	CustomColors[MODCOLOR_VUMETER_LO] = RGB(0x00, 0xC8, 0x00);
	CustomColors[MODCOLOR_VUMETER_MED] = RGB(0xFF, 0xC8, 0x00);
	CustomColors[MODCOLOR_VUMETER_HI] = RGB(0xE1, 0x00, 0x00);
	CustomColors[MODCOLOR_VUMETER_LO_VST] = RGB(0x18, 0x96, 0xE1);
	CustomColors[MODCOLOR_VUMETER_MED_VST] = RGB(0xFF, 0xC8, 0x00);
	CustomColors[MODCOLOR_VUMETER_HI_VST] = RGB(0xE1, 0x00, 0x00);
	CustomColors[MODCOLOR_SEPSHADOW] = RGB(0x23, 0x38, 0x23);
	CustomColors[MODCOLOR_SEPFACE] = RGB(0x40, 0x68, 0x40);
	CustomColors[MODCOLOR_SEPHILITE] = RGB(0x94, 0xBC, 0x94);
	CustomColors[MODCOLOR_BLENDCOLOR] = RGB(0x00, 0x40, 0x00);
	CustomColors[MODCOLOR_DODGY_COMMANDS] = RGB(0xFF, 0x80, 0x80);
	CustomColors[MODCOLOR_BACKSAMPLE] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_SAMPLESELECTED] = RGB(0xFF, 0xFF, 0xFF);
	CustomColors[MODCOLOR_BACKENV] = RGB(0x00, 0x00, 0x00);
	OnPreviewChanged();
}


void COptionsColors::OnPresetBuzz()
//---------------------------------
{
	CustomColors[MODCOLOR_BACKNORMAL] = RGB(0xE1, 0xDB, 0xD0);
	CustomColors[MODCOLOR_TEXTNORMAL] = RGB(0x3A, 0x34, 0x27);
	CustomColors[MODCOLOR_BACKCURROW] = RGB(0xC0, 0xB4, 0x9E);
	CustomColors[MODCOLOR_TEXTCURROW] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_BACKSELECTED] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_TEXTSELECTED] = RGB(0xDD, 0xD7, 0xCC);
	CustomColors[MODCOLOR_SAMPLE] = RGB(0x00, 0xFF, 0x00);
	CustomColors[MODCOLOR_BACKPLAYCURSOR] = RGB(0xA9, 0x99, 0x7A);
	CustomColors[MODCOLOR_TEXTPLAYCURSOR] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_BACKHILIGHT] = RGB(0xCE, 0xC5, 0xB5);
	CustomColors[MODCOLOR_NOTE] = RGB(0x00, 0x00, 0x5B);
	CustomColors[MODCOLOR_INSTRUMENT] = RGB(0x00, 0x55, 0x55);
	CustomColors[MODCOLOR_VOLUME] = RGB(0x00, 0x5E, 0x00);
	CustomColors[MODCOLOR_PANNING] = RGB(0x00, 0x68, 0x68);
	CustomColors[MODCOLOR_PITCH] = RGB(0x62, 0x62, 0x00);
	CustomColors[MODCOLOR_GLOBALS] = RGB(0x66, 0x00, 0x00);
	CustomColors[MODCOLOR_ENVELOPES] = RGB(0xFF, 0x00, 0x00);
	CustomColors[MODCOLOR_VUMETER_LO] = RGB(0x00, 0xC8, 0x00);
	CustomColors[MODCOLOR_VUMETER_MED] = RGB(0xFF, 0xC8, 0x00);
	CustomColors[MODCOLOR_VUMETER_HI] = RGB(0xE1, 0x00, 0x00);
	CustomColors[MODCOLOR_VUMETER_LO_VST] = RGB(0x18, 0x96, 0xE1);
	CustomColors[MODCOLOR_VUMETER_MED_VST] = RGB(0xFF, 0xC8, 0x00);
	CustomColors[MODCOLOR_VUMETER_HI_VST] = RGB(0xE1, 0x00, 0x00);
	CustomColors[MODCOLOR_SEPSHADOW] = RGB(0xAC, 0xA8, 0xA1);
	CustomColors[MODCOLOR_SEPFACE] = RGB(0xD6, 0xD0, 0xC6);
	CustomColors[MODCOLOR_SEPHILITE] = RGB(0xEC, 0xE8, 0xE1);
	CustomColors[MODCOLOR_BLENDCOLOR] = RGB(0xE1, 0xDB, 0xD0);
	CustomColors[MODCOLOR_DODGY_COMMANDS] = RGB(0xC0, 0x00, 0x00);
	CustomColors[MODCOLOR_BACKSAMPLE] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_SAMPLESELECTED] = RGB(0xFF, 0xFF, 0xFF);
	CustomColors[MODCOLOR_BACKENV] = RGB(0x00, 0x00, 0x00);
	OnPreviewChanged();
}

void COptionsColors::OnLoadColorScheme()
//--------------------------------------
{
	FileDialog dlg = OpenFileDialog()
		.DefaultExtension("mptcolor")
		.ExtensionFilter("OpenMPT Color Schemes|*.mptcolor||")
		.WorkingDirectory(theApp.GetConfigPath());
	if(!dlg.Show(this)) return;

	// Ensure that all colours are reset (for outdated colour schemes)
	OnPresetMPT();
	{
		IniFileSettingsContainer file(dlg.GetFirstFile());
		for(uint32 i = 0; i < MAX_MODCOLORS; i++)
		{
			TCHAR sKeyName[16];
			wsprintf(sKeyName, _T("Color%02u"), i);
			CustomColors[i] = file.Read<int32>("Colors", sKeyName, CustomColors[i]);
		}
	}
	OnPreviewChanged();
}

void COptionsColors::OnSaveColorScheme()
//--------------------------------------
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
			TCHAR sKeyName[16];
			wsprintf(sKeyName, _T("Color%02u"), i);
			file.Write<int32>("Colors", sKeyName, CustomColors[i]);
		}
	}
}


void COptionsColors::OnClearWindowCache()
//---------------------------------------
{
	SettingsContainer &settings = theApp.GetSongSettings();
	// First, forget all settings...
	settings.ForgetAll();
	// Then make sure they are gone for good.
	::DeleteFileW(theApp.GetSongSettingsFilename().AsNative().c_str());
}


OPENMPT_NAMESPACE_END
