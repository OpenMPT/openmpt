/*
 * WelcomeDialog.cpp
 * -----------------
 * Purpose: "First run" OpenMPT welcome dialog
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "WelcomeDialog.h"
#include "resource.h"
#include "Mainfrm.h"
#include "../common/StringFixer.h"
#include "InputHandler.h"
#include "CommandSet.h"
#include "SelectPluginDialog.h"
#include "UpdateCheck.h"


OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(WelcomeDlg, CDialog)
	ON_COMMAND(IDC_BUTTON1,	OnOptions)
	ON_COMMAND(IDC_BUTTON2,	OnScanPlugins)
END_MESSAGE_MAP()


WelcomeDlg::WelcomeDlg(CWnd *parent)
//----------------------------------
{
	Create(IDD_WECLOME, parent);
	CenterWindow(parent);
}


static mpt::PathString GetFullKeyPath(const char *keyFile)
//--------------------------------------------------------
{
	return theApp.GetAppDirPath() + MPT_PATHSTRING("extraKeymaps\\") + mpt::PathString::FromUTF8(keyFile) + MPT_PATHSTRING(".mkb");
}


BOOL WelcomeDlg::OnInitDialog()
//-----------------------------
{
	CDialog::OnInitDialog();

#ifndef NO_VST
	HKEY hkEnum = NULL;
	WCHAR str[MAX_PATH];
	DWORD datasize = sizeof(str);
	DWORD datatype = REG_SZ;
	if(RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\VST", &hkEnum) == ERROR_SUCCESS
		&& RegQueryValueExW(hkEnum, L"VSTPluginsPath", 0, &datatype, (LPBYTE)str, &datasize) == ERROR_SUCCESS)
	{
		mpt::String::SetNullTerminator(str);
		vstPath = mpt::PathString::FromNative(str);
	} else if(SHGetSpecialFolderPathW(0, str, CSIDL_PROGRAM_FILES, FALSE))
	{
		mpt::String::SetNullTerminator(str);
		vstPath = mpt::PathString::FromNative(str) + MPT_PATHSTRING("\\Steinberg\\VstPlugins\\");
		if(!vstPath.IsDirectory())
		{
			vstPath = mpt::PathString();
		}
	}
	if(!vstPath.empty())
	{
		::SetDlgItemTextW(m_hWnd, IDC_EDIT1, vstPath.AsNative().c_str());
		if(TrackerSettings::Instance().PathPlugins.GetDefaultDir().empty())
		{
			TrackerSettings::Instance().PathPlugins.SetDefaultDir(vstPath);
		}
	} else
#endif
	{
		SetDlgItemText(IDC_EDIT1, _T("No plugin path found!"));
		GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
	}

	const char *keyFile = nullptr;
	const TCHAR *keyFileName = nullptr;
	const uint16_t language = LOWORD(GetKeyboardLayout(0)), primaryLang = language & 0x3FF;
	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	combo->AddString(_T("OpenMPT / Chromatic (Default)"));
	combo->SetCurSel(0);
	switch(primaryLang)
	{
		case LANG_GERMAN:
			keyFile = "DE_jojo";
			keyFileName = _T("German");
			break;
		case LANG_SPANISH:
			// Spanish latin-american keymap, so we ignore Spain.
			if(language != SUBLANG_SPANISH_MODERN && language != SUBLANG_SPANISH)
			{
				keyFile = "es-LA_mpt_(jmkz)";
				keyFileName = _T("Spanish");
			}
			break;
		case LANG_FRENCH:
			keyFile = "FR_mpt_(legovitch)";
			keyFileName = _T("French");
			break;
		case LANG_NORWEGIAN:
			keyFile = "NO_mpt_classic_(rakib)";
			keyFileName = _T("Norwegian");
			break;
	}
	if(keyFile != nullptr)
	{
		if(GetFullKeyPath(keyFile).IsFile())
		{
			int i = combo->AddString(_T("OpenMPT / Chromatic (") + CString(keyFileName) + _T(")"));
			combo->SetItemDataPtr(i, (void *)keyFile);
			combo->SetCurSel(i);
		}
	}
	combo->SetItemDataPtr(combo->AddString(_T("Impulse Tracker")), "US_mpt-ft2_classic");
	combo->SetItemDataPtr(combo->AddString(_T("FastTracker 2")), "US_mpt-it2_classic");

	CheckDlgButton(IDC_CHECK1, BST_CHECKED);
	CheckDlgButton(IDC_CHECK2, (TrackerSettings::Instance().patternFont.Get().name == PATTERNFONT_LARGE) ? BST_CHECKED : BST_UNCHECKED);

	ShowWindow(SW_SHOW);

	return TRUE;
}


void WelcomeDlg::OnOptions()
//--------------------------
{
	OnOK();
	CMainFrame::GetMainFrame()->PostMessage(WM_COMMAND, ID_VIEW_OPTIONS);
}


void WelcomeDlg::OnScanPlugins()
//------------------------------
{
#ifndef NO_VST
	CSelectPluginDlg::ScanPlugins(vstPath, this);
#endif
}


void WelcomeDlg::OnOK()
//---------------------
{
	CDialog::OnOK();

	bool runUpdates = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;
	TrackerSettings::Instance().UpdateUpdateCheckPeriod = (runUpdates ? 7 : 0);
	if(IsDlgButtonChecked(IDC_CHECK2) != BST_UNCHECKED)
	{
		FontSetting font = TrackerSettings::Instance().patternFont;
		font.name = PATTERNFONT_LARGE;
		TrackerSettings::Instance().patternFont = font;
	}

	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	const char *keyFile = static_cast<char *>(combo->GetItemDataPtr(combo->GetCurSel()));
	if(keyFile != nullptr)
	{
		auto cmdSet = mpt::make_unique<CCommandSet>();
		cmdSet->LoadFile(GetFullKeyPath(keyFile));
		CMainFrame::GetInputHandler()->SetNewCommandSet(cmdSet.get());
	}
	if(runUpdates)
	{
		CUpdateCheck::DoAutoUpdateCheck();
	}
	CMainFrame::GetMainFrame()->PostMessage(WM_MOD_INVALIDATEPATTERNS, HINT_MPTOPTIONS);
}

OPENMPT_NAMESPACE_END
