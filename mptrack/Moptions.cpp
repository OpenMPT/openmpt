#include "stdafx.h"
#include <shlobj.h>
#include <afxpriv.h>
#include "mptrack.h"
#include "mainfrm.h"
#include "moptions.h"
#include "moddoc.h"

#pragma warning(disable:4244)

/////////////////////////////////////////////////////////
// CEditKey

BEGIN_MESSAGE_MAP(CEditKey, CEdit)
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()


void CEditKey::OnKeyDown(UINT nChar, UINT, UINT nFlags)
//-----------------------------------------------------
{
	CHAR s[16];
	if (!m_pParent) return;
	if (nChar == VK_TAB)
	{
		Default();
	} else
	if (nChar == VK_LEFT)
	{
		m_pParent->ActivateEdit((m_nId) ? m_nId-1 : 11);
	} else
	if (nChar == VK_RIGHT)
	{
		m_pParent->ActivateEdit((m_nId < 11) ? m_nId+1 : 0);
	} else
	{
		if (nFlags & 0x4000) return;
		if (m_pParent->SetKey(m_nId, nChar, nFlags))
		{
			s[0] = s[1] = 0;
			if (nChar != VK_SPACE) CMainFrame::GetKeyName(nFlags << 16, s, sizeof(s)-1);
			SetWindowText(s);
			m_pParent->ActivateEdit((m_nId+1)%12);
		}
	}
}


BOOL CEditKey::PreTranslateMessage(MSG *pMsg)
//-------------------------------------------
{
	if ((pMsg) && (pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_RETURN)) return TRUE;
	return CEdit::PreTranslateMessage(pMsg);
}


/////////////////////////////////////////////////////////
// COptionsKeyboard

// Id, MPT, FT2, IT
MPTHOTKEY gDefaultHotKeys[MAX_MPTHOTKEYS] =
{
	{IDC_PATTERN_RECORD,		VK_SPACE|MPTF_CTRL,	VK_SPACE,			VK_SPACE|MPTF_CTRL,	"Toggle record mode"},
	{ID_PATTERN_PLAYROW,		VK_RETURN|MPTF_CTRL,VK_RETURN|MPTF_CTRL,'8',				"Play current row"},
	{ID_CURSORCOPY,				VK_RETURN,			VK_RETURN,			VK_RETURN,			"Quick cursor copy"},
	{ID_CURSORPASTE,			VK_SPACE,			VK_RETURN|MPTF_SHIFT,VK_SPACE,			"Quick cursor paste"},
	{ID_PATTERN_MUTE,			0xffff,				0xffff,				VK_F9|MPTF_ALT,		"Mute current channel"},
	{ID_PATTERN_SOLO,			0xffff,				0xffff,				VK_F10|MPTF_ALT,	"Solo current channel"},
	{ID_EDIT_SELECTCOLUMN2,		'L'|MPTF_CTRL,		'L'|MPTF_CTRL,		'L'|MPTF_ALT,		"Select current column"},
	{ID_TRANSPOSE_UP,			'Q'|MPTF_CTRL,		'Q'|MPTF_CTRL,		'Q'|MPTF_ALT,		"Transpose +1"},
	{ID_TRANSPOSE_DOWN,			'A'|MPTF_CTRL,		'A'|MPTF_CTRL,		'A'|MPTF_ALT,		"Transpose -1"},
	{ID_TRANSPOSE_OCTUP,		'Q'|MPTF_CTRL|MPTF_SHIFT, 'Q'|MPTF_CTRL|MPTF_SHIFT,	'Q'|MPTF_CTRL|MPTF_ALT,	"Transpose +12"},
	{ID_TRANSPOSE_OCTDOWN,		'A'|MPTF_CTRL|MPTF_SHIFT, 'A'|MPTF_CTRL|MPTF_SHIFT,	'A'|MPTF_CTRL|MPTF_ALT,	"Transpose -12"},
	{ID_PATTERN_AMPLIFY,		'M'|MPTF_CTRL,		'M'|MPTF_CTRL,		'J'|MPTF_ALT,		"Amplify selection"},
	{ID_PATTERN_SETINSTRUMENT,	'I'|MPTF_CTRL,		'I'|MPTF_CTRL,		'S'|MPTF_ALT,		"Replace instrument"},
	{ID_PATTERN_INTERPOLATE_VOLUME,'J'|MPTF_CTRL,	'J'|MPTF_CTRL,		'K'|MPTF_ALT,		"Interpolate volume"},
	{ID_PATTERN_INTERPOLATE_EFFECT,'K'|MPTF_CTRL,	'K'|MPTF_CTRL,		'X'|MPTF_ALT,		"Interpolate effect"},
};

const UINT wEditId[12+2] =
{
	IDC_EDIT1,	IDC_EDIT2,	IDC_EDIT3,	IDC_EDIT4,
	IDC_EDIT5,	IDC_EDIT6,	IDC_EDIT7,	IDC_EDIT8,
	IDC_EDIT9,	IDC_EDIT10,	IDC_EDIT12,	IDC_EDIT13,
	IDC_EDIT14, IDC_EDIT15
};

BEGIN_MESSAGE_MAP(COptionsKeyboard, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnKeyboardChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnOctaveChanged)
	ON_LBN_SELCHANGE(IDC_LIST1,		OnHotKeySelChanged)
	ON_COMMAND(IDC_CHECK1,			OnSettingsChanged)
	ON_COMMAND(IDC_HOTKEY1,			OnHotKeyChanged)
	ON_COMMAND(IDC_BUTTON1,			OnHotKeyReset)
END_MESSAGE_MAP()


BOOL CNotifyHotKey::PreTranslateMessage(MSG *pMsg)
//------------------------------------------------
{
	BOOL bNotify = FALSE, bRet = FALSE;
	if (pMsg)
	{
		if (pMsg->message == WM_KEYDOWN)
		{
			if ((pMsg->wParam == VK_RETURN) || (pMsg->wParam == VK_SPACE))
			{
				SetHotKey(pMsg->wParam, CMainFrame::gnHotKeyMask);
				bRet = TRUE;
			}
		}
		if (pMsg->message == WM_KEYUP) bNotify = TRUE;
		if (pMsg->message == WM_SYSKEYDOWN) bRet = TRUE;
	}
	if (!bRet) bRet = CHotKeyCtrl::PreTranslateMessage(pMsg);
	if ((bNotify) && (m_hParent)) ::PostMessage(m_hParent, WM_COMMAND, m_nCtrlId, 0);
	return bRet;
}


void COptionsKeyboard::DoDataExchange(CDataExchange *pDX)
//-------------------------------------------------------
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HOTKEY1,	m_HotKey);
	DDX_Control(pDX, IDC_LIST1,		m_lbnHotKeys);
	DDX_Control(pDX, IDC_BUTTON1,	m_bnReset);
}


BOOL COptionsKeyboard::OnSetActive()
//----------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_KEYBOARD;
	return CPropertyPage::OnSetActive();
}


BOOL COptionsKeyboard::SetKey(UINT nId, UINT nChar, UINT nFlags)
//--------------------------------------------------------------
{
	nFlags &= 0x1FF;
	if (nFlags & 0x100) return FALSE;
	if ((nId < 3*12+2) && (nFlags))
	{
		if (nChar == VK_SPACE) nFlags = 0;
		if (m_nCurKeyboard == KEYBOARD_CUSTOM)
		{
			if (nFlags != (UINT)KeyboardMap[nId])
			{
				KeyboardMap[nId] = nFlags;
				OnSettingsChanged();
			}
			return TRUE;
		}
	}
	return FALSE;
}


BOOL COptionsKeyboard::ActivateEdit(UINT nId)
//-------------------------------------------
{
	if (nId < 12+2)
	{
		edits[nId].SetFocus();
		return TRUE;
	}
	return FALSE;
}


BOOL COptionsKeyboard::OnInitDialog()
//-----------------------------------
{
	CPropertyPage::OnInitDialog();
	m_nCurHotKey = -1;
	m_nCurKeyboard = -1;
	memcpy(CustomKeys, CMainFrame::CustomKeys, sizeof(CustomKeys));
	memcpy(KeyboardMap, CMainFrame::KeyboardMap, sizeof(KeyboardMap));
	m_nKeyboardCfg = CMainFrame::m_nKeyboardCfg;
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	if (combo)
	{
		combo->AddString("Custom");
		combo->AddString("FastTracker II");
		combo->AddString("Impulse Tracker");
		combo->AddString("Modplug Tracker");
		combo->SetCurSel(m_nKeyboardCfg & KEYBOARD_MASK);
	}
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO2)) != NULL)
	{
		combo->AddString("Base Octave");
		combo->AddString("Octave +1");
		combo->AddString("Octave +2");
		combo->SetCurSel(0);
	}
	for (UINT i=0; i<12+2; i++)
	{
		edits[i].SubclassDlgItem(wEditId[i], this);
		edits[i].SetParent(this, (i < 12) ? i : 2*12+i);
	}
	if (m_nKeyboardCfg & KEYBOARD_FT2KEYS) CheckDlgButton(IDC_CHECK1, MF_CHECKED);
	for (UINT iHk=0; iHk<MAX_MPTHOTKEYS; iHk++)
	{
		if (gDefaultHotKeys[iHk].pszName)
		{
			m_lbnHotKeys.SetItemData( m_lbnHotKeys.AddString(gDefaultHotKeys[iHk].pszName), iHk);
		}
	}
	m_lbnHotKeys.SetCurSel(0);
	UpdateDialog();
	m_HotKey.SetParent(m_hWnd, IDC_HOTKEY1);
	m_HotKey.SetRules(HKCOMB_SCA, HOTKEYF_CONTROL|HOTKEYF_ALT);
	return TRUE;
}


void COptionsKeyboard::UpdateDialog()
//-----------------------------------
{
	CHAR s[256];
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	if (combo)
	{
		BOOL bChanged = FALSE;
		const DWORD *lpKbdMap;
		int n = combo->GetCurSel();
		if (n != m_nCurKeyboard)
		{
			bChanged = TRUE;
			m_nCurKeyboard = n;
			switch(n)
			{
			case KEYBOARD_FT2:
				lpKbdMap = CMainFrame::KeyboardFT2;
				break;
			case KEYBOARD_IT:
				lpKbdMap = CMainFrame::KeyboardIT;
				break;
			case KEYBOARD_MPT:
				lpKbdMap = CMainFrame::KeyboardMPT;
				break;
			default:
				lpKbdMap = KeyboardMap;
			}
			for (int i=0; i<12+2; i++)
			{
				UINT n = edits[i].GetId();
				s[0] = 0;
				if (lpKbdMap[n]) CMainFrame::GetKeyName(lpKbdMap[n] << 16, s, 16);
				SetDlgItemText(wEditId[i], s);
			}
			m_bnReset.EnableWindow((m_nCurKeyboard == KEYBOARD_CUSTOM) ? TRUE : FALSE);
		}
		// Hot Keys
		int nHk = m_lbnHotKeys.GetItemData( m_lbnHotKeys.GetCurSel() );
		if ((nHk >= 0) && ((nHk != m_nCurHotKey) || (bChanged)))
		{
			m_nCurHotKey = nHk;
			DWORD dwVk=0;
			BOOL bEnable = FALSE;
			if (nHk < MAX_MPTHOTKEYS)
			{
				switch(m_nCurKeyboard)
				{
				case KEYBOARD_FT2:
					dwVk = gDefaultHotKeys[nHk].nFT2HotKey;
					break;
				case KEYBOARD_IT:
					dwVk = gDefaultHotKeys[nHk].nITHotKey;
					break;
				case KEYBOARD_MPT:
					dwVk = gDefaultHotKeys[nHk].nMPTHotKey;
					break;
				default:
					dwVk = CustomKeys[nHk];
					bEnable = TRUE;
				}
			}
			m_HotKey.SetHotKey(dwVk & 0xFFFF, dwVk >> 16);
			m_HotKey.EnableWindow(bEnable);
		}
	}
}


void COptionsKeyboard::OnKeyboardChanged()
//----------------------------------------
{
	OnSettingsChanged();
	UpdateDialog();
}


void COptionsKeyboard::OnOctaveChanged()
//--------------------------------------
{
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO2);
	if (combo)
	{
		UINT n = combo->GetCurSel();
		if (n < 3)
		{
			for (UINT i=0; i<12; i++)
			{
				edits[i].SetId(n*12+i);
			}
			m_nCurKeyboard = -1;
			UpdateDialog();
		}
	}
}


void COptionsKeyboard::OnHotKeySelChanged()
//-----------------------------------------
{
	UpdateDialog();
}


void COptionsKeyboard::OnHotKeyChanged()
//--------------------------------------
{
	if ((m_nCurKeyboard == KEYBOARD_CUSTOM) && (m_nCurHotKey >= 0) && (m_nCurHotKey < MAX_MPTHOTKEYS))
	{
		BOOL bChanged = FALSE;
		WORD wVk=0, wMod=0;
		
		m_HotKey.GetHotKey(wVk, wMod);
		DWORD dwHk = ((DWORD)wVk) | (((DWORD)wMod) << 16);
		for (UINT i = 0; i<MAX_MPTHOTKEYS; i++) if (i != (UINT)m_nCurHotKey)
		{
			if (CustomKeys[i] == dwHk)
			{
				CustomKeys[i] = 0;
				bChanged = TRUE;
			}
		}
		if (dwHk != CustomKeys[m_nCurHotKey])
		{
			CustomKeys[m_nCurHotKey] = dwHk;
			bChanged = TRUE;
		}
		if (bChanged) OnSettingsChanged();
	}
}


void COptionsKeyboard::OnHotKeyReset()
//------------------------------------
{
	if ((m_nCurKeyboard == KEYBOARD_CUSTOM) && (m_nCurHotKey >= 0) && (m_nCurHotKey < MAX_MPTHOTKEYS))
	{
		DWORD dwHk = gDefaultHotKeys[m_nCurHotKey].nMPTHotKey;
		if (dwHk != CustomKeys[m_nCurHotKey])
		{
			CustomKeys[m_nCurHotKey] = dwHk;
			OnSettingsChanged();
			m_nCurHotKey = -1;
			UpdateDialog();
		}
	}
}


void COptionsKeyboard::OnOK()
//---------------------------
{
	m_nKeyboardCfg = KEYBOARD_MPT;
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	if (combo) m_nKeyboardCfg = combo->GetCurSel();
	memcpy(CMainFrame::CustomKeys, CustomKeys, sizeof(CMainFrame::CustomKeys));
	memcpy(CMainFrame::KeyboardMap, KeyboardMap, sizeof(CMainFrame::KeyboardMap));
	// Keyboard options
	if (IsDlgButtonChecked(IDC_CHECK1)) m_nKeyboardCfg |= KEYBOARD_FT2KEYS;
	CMainFrame::m_nKeyboardCfg = m_nKeyboardCfg;
	CPropertyPage::OnOK();
}


//////////////////////////////////////////////////////////////
// COptionsColors

typedef struct MPTCOLORDEF
{
	LPCSTR pszName;
	int nPreview;
	UINT nColNdx1, nColNdx2, nColNdx3;
	LPCSTR pszTxt1, pszTxt2, pszTxt3;
} MPTCOLORDEF;

static MPTCOLORDEF gColorDefs[] =
{
	{"Pattern Editor",	0,	MODCOLOR_BACKNORMAL, MODCOLOR_TEXTNORMAL, MODCOLOR_BACKHILIGHT, "Background:", "Foreground:", "Highlighted:"},
	{"Active Row",		0,	MODCOLOR_BACKCURROW, MODCOLOR_TEXTCURROW, 0, "Background:", "Foreground:", NULL},
	{"Pattern Selection",0,	MODCOLOR_BACKSELECTED, MODCOLOR_TEXTSELECTED, 0, "Background:", "Foreground:", NULL},
	{"Play Cursor",		0,	MODCOLOR_BACKPLAYCURSOR, MODCOLOR_TEXTPLAYCURSOR, 0, "Background:", "Foreground:", NULL},
	{"Note Highlight",	0,	MODCOLOR_NOTE, MODCOLOR_INSTRUMENT, MODCOLOR_VOLUME, "Note:", "Instrument:", "Volume:"},
	{"Effect Highlight",0,	MODCOLOR_PANNING, MODCOLOR_PITCH, MODCOLOR_GLOBALS, "Panning Effects:", "Pitch Effects:", "Global Effects:"},
	{"Sample Editor",	1,	MODCOLOR_SAMPLE, 0, 0, "Sample Data:", NULL, NULL},
	{"Instrument Editor",2,	MODCOLOR_ENVELOPES, 0, 0, "Envelopes:", NULL, NULL},
	{"VU-Meters",		0,	MODCOLOR_VUMETER_HI, MODCOLOR_VUMETER_MED, MODCOLOR_VUMETER_LO, "Hi:", "Med:", "Lo:"}
};

#define NUMCOLORDEFS		(sizeof(gColorDefs)/sizeof(MPTCOLORDEF))
#define PREVIEWBMP_WIDTH	88
#define PREVIEWBMP_HEIGHT	39


BEGIN_MESSAGE_MAP(COptionsColors, CPropertyPage)
	ON_WM_DRAWITEM()
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnColorSelChanged)
	ON_EN_CHANGE(IDC_EDIT1,			OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT2,			OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON1,			OnSelectColor1)
	ON_COMMAND(IDC_BUTTON2,			OnSelectColor2)
	ON_COMMAND(IDC_BUTTON3,			OnSelectColor3)
	ON_COMMAND(IDC_BUTTON5,			OnPresetMPT)
	ON_COMMAND(IDC_BUTTON6,			OnPresetFT2)
	ON_COMMAND(IDC_BUTTON7,			OnPresetIT)
	ON_COMMAND(IDC_BUTTON8,			OnPresetBuzz)
	ON_COMMAND(IDC_CHECK1,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,			OnPreviewChanged)
	ON_COMMAND(IDC_CHECK3,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK4,			OnPreviewChanged)
END_MESSAGE_MAP()


void COptionsColors::DoDataExchange(CDataExchange* pDX)
//-----------------------------------------------------
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsColors)
	DDX_Control(pDX, IDC_COMBO1,		m_ComboItem);
	DDX_Control(pDX, IDC_BUTTON1,		m_BtnColor1);
	DDX_Control(pDX, IDC_BUTTON2,		m_BtnColor2);
	DDX_Control(pDX, IDC_BUTTON3,		m_BtnColor3);
	DDX_Control(pDX, IDC_BUTTON4,		m_BtnPreview);
	DDX_Control(pDX, IDC_TEXT1,			m_TxtColor1);
	DDX_Control(pDX, IDC_TEXT2,			m_TxtColor2);
	DDX_Control(pDX, IDC_TEXT3,			m_TxtColor3);
	//}}AFX_DATA_MAP
}


BOOL COptionsColors::OnInitDialog()
//---------------------------------
{
	CPropertyPage::OnInitDialog();
	m_pPreviewDib = LoadDib(MAKEINTRESOURCE(IDB_COLORSETUP));
	memcpy(CustomColors, CMainFrame::rgbCustomColors, sizeof(CustomColors));
	for (UINT i=0; i<NUMCOLORDEFS; i++)
	{
		m_ComboItem.SetItemData(m_ComboItem.AddString(gColorDefs[i].pszName), i);
	}
	m_ComboItem.SetCurSel(0);
	m_BtnPreview.SetWindowPos(NULL, 0,0, PREVIEWBMP_WIDTH*2+2, PREVIEWBMP_HEIGHT*2+2, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
	if (CMainFrame::m_dwPatternSetup & PATTERN_STDHIGHLIGHT) CheckDlgButton(IDC_CHECK1, MF_CHECKED);
	if (CMainFrame::m_dwPatternSetup & PATTERN_EFFECTHILIGHT) CheckDlgButton(IDC_CHECK2, MF_CHECKED);
	if (CMainFrame::m_dwPatternSetup & PATTERN_SMALLFONT) CheckDlgButton(IDC_CHECK3, MF_CHECKED);
	if (CMainFrame::m_dwPatternSetup & PATTERN_2NDHIGHLIGHT) CheckDlgButton(IDC_CHECK4, MF_CHECKED);
	SetDlgItemInt(IDC_EDIT1, CMainFrame::m_nRowSpacing);
	SetDlgItemInt(IDC_EDIT2, CMainFrame::m_nRowSpacing2);
	OnColorSelChanged();
	return TRUE;
}


void COptionsColors::OnOK()
//-------------------------
{
	CMainFrame::m_dwPatternSetup &= ~(PATTERN_STDHIGHLIGHT|PATTERN_2NDHIGHLIGHT|PATTERN_EFFECTHILIGHT|PATTERN_SMALLFONT);
	if (IsDlgButtonChecked(IDC_CHECK1)) CMainFrame::m_dwPatternSetup |= PATTERN_STDHIGHLIGHT;
	if (IsDlgButtonChecked(IDC_CHECK2)) CMainFrame::m_dwPatternSetup |= PATTERN_EFFECTHILIGHT;
	if (IsDlgButtonChecked(IDC_CHECK3)) CMainFrame::m_dwPatternSetup |= PATTERN_SMALLFONT;
	if (IsDlgButtonChecked(IDC_CHECK4)) CMainFrame::m_dwPatternSetup |= PATTERN_2NDHIGHLIGHT;
	CMainFrame::m_nRowSpacing = GetDlgItemInt(IDC_EDIT1);
	CMainFrame::m_nRowSpacing2 = GetDlgItemInt(IDC_EDIT2);
	if ((CMainFrame::m_nRowSpacing2 > CMainFrame::m_nRowSpacing)
	 && (CMainFrame::m_nRowSpacing) && (CMainFrame::m_nRowSpacing2))
	{
		UINT tmp = CMainFrame::m_nRowSpacing;
		CMainFrame::m_nRowSpacing = CMainFrame::m_nRowSpacing2;
		CMainFrame::m_nRowSpacing2 = tmp;
	}
	memcpy(CMainFrame::rgbCustomColors, CustomColors, sizeof(CMainFrame::rgbCustomColors));
	CMainFrame::UpdateColors();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->PostMessage(WM_MOD_INVALIDATEPATTERNS, HINT_MPTOPTIONS);
	CPropertyPage::OnOK();
}


BOOL COptionsColors::OnSetActive()
//--------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_COLORS;
	return CPropertyPage::OnSetActive();
}


void COptionsColors::OnDrawItem(int nIdCtl, LPDRAWITEMSTRUCT lpdis)
//-----------------------------------------------------------------
{
	int nColor = -1;
	switch(nIdCtl)
	{
	case IDC_BUTTON1:	nColor = gColorDefs[m_nColorItem].nColNdx1; break;
	case IDC_BUTTON2:	nColor = gColorDefs[m_nColorItem].nColNdx2; break;
	case IDC_BUTTON3:	nColor = gColorDefs[m_nColorItem].nColNdx3; break;
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
		int y = gColorDefs[m_nColorItem].nPreview;
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
	SelectColor(&CustomColors[gColorDefs[m_nColorItem].nColNdx1]);
}


void COptionsColors::OnSelectColor2()
//-----------------------------------
{
	SelectColor(&CustomColors[gColorDefs[m_nColorItem].nColNdx2]);
}


void COptionsColors::OnSelectColor3()
//-----------------------------------
{
	SelectColor(&CustomColors[gColorDefs[m_nColorItem].nColNdx3]);
}


void COptionsColors::OnColorSelChanged()
//--------------------------------------
{
	int sel = m_ComboItem.GetCurSel();
	if (sel >= 0)
	{
		m_nColorItem = m_ComboItem.GetItemData(sel);
		OnUpdateDialog();
	}
}


void COptionsColors::OnUpdateDialog()
//-----------------------------------
{
	MPTCOLORDEF *p = &gColorDefs[m_nColorItem];
	if (p->pszTxt1) m_TxtColor1.SetWindowText(p->pszTxt1);
	if (p->pszTxt2)
	{
		m_TxtColor2.SetWindowText(p->pszTxt2);
		m_TxtColor2.ShowWindow(SW_SHOW);
		m_BtnColor2.ShowWindow(SW_SHOW);
		m_BtnColor2.InvalidateRect(NULL, FALSE);
	} else
	{
		m_TxtColor2.ShowWindow(SW_HIDE);
		m_BtnColor2.ShowWindow(SW_HIDE);
	}
	if (p->pszTxt3)
	{
		m_TxtColor3.SetWindowText(p->pszTxt3);
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
	CustomColors[MODCOLOR_BACKNORMAL] = RGB(0xFF, 0xFF, 0xFF);
	CustomColors[MODCOLOR_TEXTNORMAL] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_BACKCURROW] = RGB(0xC0, 0xC0, 0xC0);
	CustomColors[MODCOLOR_TEXTCURROW] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_BACKSELECTED] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_TEXTSELECTED] = RGB(0xFF, 0xFF, 0xFF);
	CustomColors[MODCOLOR_BACKPLAYCURSOR] = RGB(0xFF, 0xFF, 0x80);
	CustomColors[MODCOLOR_TEXTPLAYCURSOR] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_BACKHILIGHT] = RGB(0xE0, 0xE8, 0xE0);
	CustomColors[MODCOLOR_NOTE] = RGB(0x00, 0x00, 0x80);
	CustomColors[MODCOLOR_INSTRUMENT] = RGB(0x00, 0x80, 0x80);
	CustomColors[MODCOLOR_VOLUME] = RGB(0x00, 0x80, 0x00);
	CustomColors[MODCOLOR_PANNING] = RGB(0x00, 0x80, 0x80);
	CustomColors[MODCOLOR_PITCH] = RGB(0x80, 0x80, 0x00);
	CustomColors[MODCOLOR_GLOBALS] = RGB(0x80, 0x00, 0x00);
	OnPreviewChanged();
}


void COptionsColors::OnPresetFT2()
//--------------------------------
{
	CustomColors[MODCOLOR_BACKNORMAL] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_TEXTNORMAL] = RGB(0xE0, 0xE0, 0x40);
	CustomColors[MODCOLOR_BACKCURROW] = RGB(0x70, 0x70, 0x70);
	CustomColors[MODCOLOR_TEXTCURROW] = RGB(0xF0, 0xF0, 0x50);
	CustomColors[MODCOLOR_BACKSELECTED] = RGB(0x40, 0x40, 0xA0);
	CustomColors[MODCOLOR_TEXTSELECTED] = RGB(0xFF, 0xFF, 0xFF);
	CustomColors[MODCOLOR_BACKPLAYCURSOR] = RGB(0x50, 0x50, 0x70);
	CustomColors[MODCOLOR_TEXTPLAYCURSOR] = RGB(0xE0, 0xE0, 0x40);
	CustomColors[MODCOLOR_BACKHILIGHT] = RGB(0x40, 0x40, 0x80);
	CustomColors[MODCOLOR_NOTE] = RGB(0xE0, 0xE0, 0x40);
	CustomColors[MODCOLOR_INSTRUMENT] = RGB(0xFF, 0xFF, 0x00);
	CustomColors[MODCOLOR_VOLUME] = RGB(0x00, 0xFF, 0x00);
	CustomColors[MODCOLOR_PANNING] = RGB(0x00, 0xFF, 0xFF);
	CustomColors[MODCOLOR_PITCH] = RGB(0xFF, 0xFF, 0x00);
	CustomColors[MODCOLOR_GLOBALS] = RGB(0xFF, 0x40, 0x40);
	OnPreviewChanged();
}


void COptionsColors::OnPresetIT()
//-------------------------------
{
	CustomColors[MODCOLOR_BACKNORMAL] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_TEXTNORMAL] = RGB(0x00, 0xE0, 0x00);
	CustomColors[MODCOLOR_BACKCURROW] = RGB(0x70, 0x70, 0x70);
	CustomColors[MODCOLOR_TEXTCURROW] = RGB(0x00, 0xE0, 0x00);
	CustomColors[MODCOLOR_BACKSELECTED] = RGB(0xE0, 0xE0, 0xE0);
	CustomColors[MODCOLOR_TEXTSELECTED] = RGB(0x00, 0x00, 0x00);
	CustomColors[MODCOLOR_BACKPLAYCURSOR] = RGB(0x80, 0x80, 0x00);
	CustomColors[MODCOLOR_TEXTPLAYCURSOR] = RGB(0x00, 0xE0, 0x00);
	CustomColors[MODCOLOR_BACKHILIGHT] = RGB(0x40, 0x68, 0x40);
	CustomColors[MODCOLOR_NOTE] = RGB(0x00, 0xFF, 0x00);
	CustomColors[MODCOLOR_INSTRUMENT] = RGB(0xFF, 0xFF, 0x00);
	CustomColors[MODCOLOR_VOLUME] = RGB(0x00, 0xFF, 0x00);
	CustomColors[MODCOLOR_PANNING] = RGB(0x00, 0xFF, 0xFF);
	CustomColors[MODCOLOR_PITCH] = RGB(0xFF, 0xFF, 0x00);
	CustomColors[MODCOLOR_GLOBALS] = RGB(0xFF, 0x40, 0x40);
	OnPreviewChanged();
}


void COptionsColors::OnPresetBuzz()
//---------------------------------
{
	CustomColors[MODCOLOR_BACKNORMAL] = 0x00d0dbe1;
	CustomColors[MODCOLOR_TEXTNORMAL] = 0x0027343a;
	CustomColors[MODCOLOR_BACKCURROW] = 0x009eb4c0;
	CustomColors[MODCOLOR_TEXTCURROW] = 0x00000000;
	CustomColors[MODCOLOR_BACKSELECTED] = 0x00000000;
	CustomColors[MODCOLOR_TEXTSELECTED] = 0x00ccd7dd;
	//CustomColors[MODCOLOR_SAMPLE] = 0x0000ff00;
	CustomColors[MODCOLOR_BACKPLAYCURSOR] = 0x007a99a9;
	CustomColors[MODCOLOR_TEXTPLAYCURSOR] = 0x00000000;
	CustomColors[MODCOLOR_BACKHILIGHT] = 0x00b5c5ce;
	CustomColors[MODCOLOR_NOTE] = 0x005b0000;
	CustomColors[MODCOLOR_INSTRUMENT] = 0x00555500;
	CustomColors[MODCOLOR_VOLUME] = 0x00005e00;
	CustomColors[MODCOLOR_PANNING] = 0x00686800;
	CustomColors[MODCOLOR_PITCH] = 0x00006262;
	CustomColors[MODCOLOR_GLOBALS] = 0x00000066;
	//CustomColors[MODCOLOR_ENVELOPES] = 0x000000ff;
	OnPreviewChanged();
}

/////////////////////////////////////////////////////////////////////////////////
// COptionsGeneral

BEGIN_MESSAGE_MAP(COptionsGeneral, CPropertyPage)
	ON_EN_CHANGE(IDC_EDIT1,		OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT2,		OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT3,		OnSettingsChanged)
	ON_LBN_SELCHANGE(IDC_LIST1,	OnOptionSelChanged)
	ON_COMMAND(IDC_BUTTON1,		OnBrowseSongs)
	ON_COMMAND(IDC_BUTTON2,		OnBrowseSamples)
	ON_COMMAND(IDC_BUTTON3,		OnBrowseInstruments)
	ON_CLBN_CHKCHANGE(IDC_LIST1, OnSettingsChanged)
END_MESSAGE_MAP()

typedef struct OPTGENDESC
{
	LPCSTR pszListName, pszDescription;
} OPTGENDESC;

enum {
	OPTGEN_PLAYNEWNOTES=0,
	OPTGEN_CENTERROW,
	OPTGEN_LARGECOMMENTSFONT,
	OPTGEN_HEXROWDISP,
	OPTGEN_CURSORWRAP,
	OPTGEN_CREATEBACKUP,
	OPTGEN_DRAGNDROPEDIT,
	OPTGEN_FLATBUTTONS,
	OPTGEN_SINGLEEXPAND,
	OPTGEN_MUTECHNMODE,
	OPTGEN_AUTOSPACEBAR,
	OPTGEN_NOEXTRALOUD,
	OPTGEN_SHOWPREVIOUS,
	OPTGEN_CONTSCROLL,
	OPTGEN_KBDNOTEOFF,
	OPTGEN_MAXOPTIONS
};

static OPTGENDESC gOptGenDesc[OPTGEN_MAXOPTIONS] =
{
	{"Play new notes while recording",	"When this option is enabled, notes entered in the pattern editor will always be played (If not checked, notes won't be played in record mode)."},
	{"Always center active row",		"Turn on this option to have the active row always centered in the pattern editor."},
	{"Use large font for comments",		"With this option enabled, the song message editor will use a larger font."},
	{"Display rows in hex",				"With this option enabled, row numbers and sequence numbers will be displayed in hexadecimal."},
	{"Cursor wrap in pattern editor",	"When this option is active, going past the end of a pattern will move the cursor to the beginning."},
	{"Create backup files (*.bak)",		"When this option is active, saving a file will create a backup copy of the original."},
	{"Drag and Drop Editing",			"Enable moving a selection in the pattern editor (copying if pressing shift while dragging)\n"},
	{"Flat Buttons",					"Use flat buttons in toolbars"},
	{"Single click to expand tree",		"Single-clicking in the left tree view will expand a branch"},
	{"Ignored muted channels",			"Notes will not be played on muted channels (unmuting will only start on a new note)."},
	{"Quick cursor paste Auto-Repeat",	"Leaving the space bar pressed will auto-repeat the action"},
	{"No loud samples",					"Disable loud playback of samples in the sample/instrument editor"},
	{"Show Prev/Next patterns",			"Displays grayed-out version of the previous/next patterns in the pattern editor"},
	{"Continuous scroll",				"Jumps to the next pattern when moving past the end of a pattern"},
	{"Record note off",					"Record note off when a key is released on the PC keyboard (Only works in instrument mode)."},
};


void COptionsGeneral::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModTypeDlg)
	DDX_Control(pDX, IDC_LIST1,			m_CheckList);
	//}}AFX_DATA_MAP
}


BOOL COptionsGeneral::OnInitDialog()
//----------------------------------
{
	CHAR sname[32], s[256];

	CPropertyPage::OnInitDialog();
	for (UINT i=0; i<OPTGEN_MAXOPTIONS; i++)
	{
		BOOL bCheck;
		wsprintf(sname, "Setup.Gen.Opt%d.Name", i+1);
		if ((theApp.GetLocalizedString(sname, s, sizeof(s))) && (s[0]))
			m_CheckList.AddString(s);
		else
			m_CheckList.AddString(gOptGenDesc[i].pszListName);
		bCheck = FALSE;
		switch(i)
		{
		case OPTGEN_PLAYNEWNOTES:		bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_PLAYNEWNOTE); break;
		case OPTGEN_CENTERROW:			bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_CENTERROW); break;
		case OPTGEN_LARGECOMMENTSFONT:	bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_LARGECOMMENTS); break;
		case OPTGEN_HEXROWDISP:			bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_HEXDISPLAY); break;
		case OPTGEN_CURSORWRAP:			bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_WRAP); break;
		case OPTGEN_CREATEBACKUP:		bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_CREATEBACKUP); break;
		case OPTGEN_DRAGNDROPEDIT:		bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_DRAGNDROPEDIT); break;
		case OPTGEN_FLATBUTTONS:		bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_FLATBUTTONS); break;
		case OPTGEN_SINGLEEXPAND:		bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_SINGLEEXPAND); break;
		case OPTGEN_MUTECHNMODE:		bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_MUTECHNMODE); break;
		case OPTGEN_AUTOSPACEBAR:		bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_AUTOSPACEBAR); break;
		case OPTGEN_NOEXTRALOUD:		bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_NOEXTRALOUD); break;
		case OPTGEN_SHOWPREVIOUS:		bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_SHOWPREVIOUS); break;
		case OPTGEN_CONTSCROLL:			bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_CONTSCROLL); break;
		case OPTGEN_KBDNOTEOFF:			bCheck = (CMainFrame::m_dwPatternSetup & PATTERN_KBDNOTEOFF); break;
		}
		m_CheckList.SetCheck(i, (bCheck) ? TRUE : FALSE);
	}
	m_CheckList.SetCurSel(0);
	OnOptionSelChanged();
	SetDlgItemText(IDC_EDIT1, CMainFrame::m_szModDir);
	SetDlgItemText(IDC_EDIT2, CMainFrame::m_szSmpDir);
	SetDlgItemText(IDC_EDIT3, CMainFrame::m_szInsDir);
	return TRUE;
}


void COptionsGeneral::OnOK()
//--------------------------
{
	CHAR szModDir[_MAX_DIR], szSmpDir[_MAX_PATH], szInsDir[_MAX_PATH];
	szModDir[0] = szInsDir[0] = szSmpDir[0] = 0;
	GetDlgItemText(IDC_EDIT1, szModDir, _MAX_PATH);
	GetDlgItemText(IDC_EDIT2, szSmpDir, _MAX_PATH);
	GetDlgItemText(IDC_EDIT3, szInsDir, _MAX_PATH);
	for (UINT i=0; i<OPTGEN_MAXOPTIONS; i++)
	{
		DWORD mask = 0;
		BOOL bCheck = m_CheckList.GetCheck(i);
		switch(i)
		{
		case OPTGEN_PLAYNEWNOTES:		mask = PATTERN_PLAYNEWNOTE; break;
		case OPTGEN_CENTERROW:			mask = PATTERN_CENTERROW; break;
		case OPTGEN_LARGECOMMENTSFONT:	mask = PATTERN_LARGECOMMENTS; break;
		case OPTGEN_HEXROWDISP:			mask = PATTERN_HEXDISPLAY; break;
		case OPTGEN_CURSORWRAP:			mask = PATTERN_WRAP; break;
		case OPTGEN_CREATEBACKUP:		mask = PATTERN_CREATEBACKUP; break;
		case OPTGEN_DRAGNDROPEDIT:		mask = PATTERN_DRAGNDROPEDIT; break;
		case OPTGEN_FLATBUTTONS:		mask = PATTERN_FLATBUTTONS; break;
		case OPTGEN_SINGLEEXPAND:		mask = PATTERN_SINGLEEXPAND; break;
		case OPTGEN_MUTECHNMODE:		mask = PATTERN_MUTECHNMODE; break;
		case OPTGEN_AUTOSPACEBAR:		mask = PATTERN_AUTOSPACEBAR; break;
		case OPTGEN_NOEXTRALOUD:		mask = PATTERN_NOEXTRALOUD; break;
		case OPTGEN_SHOWPREVIOUS:		mask = PATTERN_SHOWPREVIOUS; break;
		case OPTGEN_CONTSCROLL:			mask = PATTERN_CONTSCROLL; break;
		case OPTGEN_KBDNOTEOFF:			mask = PATTERN_KBDNOTEOFF; break;
		}
		if (bCheck) CMainFrame::m_dwPatternSetup |= mask; else CMainFrame::m_dwPatternSetup &= ~mask;
		m_CheckList.SetCheck(i, (bCheck) ? TRUE : FALSE);
	}
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm)
	{
		pMainFrm->SetupDirectories(szModDir, szSmpDir, szInsDir);
		pMainFrm->UpdateTree(NULL, HINT_MPTOPTIONS);
	}
	CPropertyPage::OnOK();
}


BOOL COptionsGeneral::OnSetActive()
//---------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_GENERAL;
	return CPropertyPage::OnSetActive();
}


void COptionsGeneral::BrowseForFolder(UINT nID)
//---------------------------------------------
{
	CHAR szPath[_MAX_PATH] = "";
	BROWSEINFO bi;

	GetDlgItemText(nID, szPath, sizeof(szPath));
	memset(&bi, 0, sizeof(bi));
	bi.hwndOwner = m_hWnd;
	bi.pszDisplayName = szPath;
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	LPITEMIDLIST pid = SHBrowseForFolder(&bi);
	if (pid != NULL)
	{
		SHGetPathFromIDList(pid, szPath);
		SetDlgItemText(nID, szPath);
		OnSettingsChanged();
	}
}


void COptionsGeneral::OnOptionSelChanged()
//----------------------------------------
{
	CHAR sname[32], s[256];
	LPCSTR pszDesc = NULL;
	int sel = m_CheckList.GetCurSel();
	if ((sel >= 0) && (sel < OPTGEN_MAXOPTIONS))
	{
		pszDesc = gOptGenDesc[sel].pszDescription;
		wsprintf(sname, "Setup.Gen.Opt%d.Desc", sel+1);
		if ((theApp.GetLocalizedString(sname, s, sizeof(s))) && (s[0])) pszDesc = s;
	}
	SetDlgItemText(IDC_TEXT1, (pszDesc) ? pszDesc : "");
}


