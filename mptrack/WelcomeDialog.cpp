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
#include "../common/mptStringBuffer.h"
#include "InputHandler.h"
#include "CommandSet.h"
#include "SelectPluginDialog.h"
#include "UpdateCheck.h"


OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(WelcomeDlg, CDialog)
	ON_COMMAND(IDC_BUTTON1,	&WelcomeDlg::OnOptions)
	ON_COMMAND(IDC_BUTTON2,	&WelcomeDlg::OnScanPlugins)
END_MESSAGE_MAP()


WelcomeDlg::WelcomeDlg(CWnd *parent)
{
	Create(IDD_WECLOME, parent);
	CenterWindow(parent);
}


static mpt::PathString GetFullKeyPath(const char *keyFile)
{
	return theApp.GetInstallPkgPath() + P_("extraKeymaps\\") + mpt::PathString::FromUTF8(keyFile) + P_(".mkb");
}


BOOL WelcomeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

#ifndef NO_VST
	HKEY hkEnum = NULL;
	TCHAR str[MAX_PATH];
	DWORD datasize = sizeof(str);
	DWORD datatype = REG_SZ;
	if(RegOpenKey(HKEY_LOCAL_MACHINE, _T("Software\\VST"), &hkEnum) == ERROR_SUCCESS
		&& RegQueryValueEx(hkEnum, _T("VSTPluginsPath"), 0, &datatype, (LPBYTE)str, &datasize) == ERROR_SUCCESS)
	{
		m_vstPath = mpt::PathString::FromNative(ParseMaybeNullTerminatedStringFromBufferWithSizeInBytes<mpt::winstring>(str, datasize));
	} else if(SHGetSpecialFolderPath(0, str, CSIDL_PROGRAM_FILES, FALSE))
	{
		m_vstPath = mpt::PathString::FromNative(ParseMaybeNullTerminatedStringFromBufferWithSizeInBytes<mpt::winstring>(str, datasize)) + P_("\\Steinberg\\VstPlugins\\");
		if(!m_vstPath.IsDirectory())
		{
			m_vstPath = mpt::PathString();
		}
	}
	if(!m_vstPath.empty())
	{
		SetDlgItemText(IDC_EDIT1, m_vstPath.AsNative().c_str());
		if(TrackerSettings::Instance().PathPlugins.GetDefaultDir().empty())
		{
			TrackerSettings::Instance().PathPlugins.SetDefaultDir(m_vstPath);
		}
	} else
#endif
	{
		SetDlgItemText(IDC_EDIT1, _T("No plugin path found!"));
		GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
	}

	const char *keyFile = nullptr;
	const TCHAR *keyFileName = nullptr;
	const uint16 language = LOWORD(GetKeyboardLayout(0)), primaryLang = language & 0x3FF;
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

			// As this is presented as the default, load it right now, even if the user closes the dialog through the close button
			auto cmdSet = std::make_unique<CCommandSet>();
			cmdSet->LoadFile(GetFullKeyPath(keyFile));
			CMainFrame::GetInputHandler()->SetNewCommandSet(cmdSet.get());
		}
	}
	combo->SetItemDataPtr(combo->AddString(_T("Impulse Tracker")), (void*)("US_mpt-it2_classic"));
	combo->SetItemDataPtr(combo->AddString(_T("FastTracker 2")), (void*)("US_mpt-ft2_classic"));

	CheckDlgButton(IDC_CHECK1, BST_CHECKED);
	CheckDlgButton(IDC_CHECK3, BST_CHECKED);
#if defined(MPT_ENABLE_UPDATE)
	GetDlgItem(IDC_STATIC_WELCOME_STATISTICS)->SetWindowText(mpt::ToCString(mpt::String::Replace(CUpdateCheck::GetStatisticsUserInformation(false), U_("\n"), U_(" "))));
#endif // MPT_ENABLE_UPDATE
	CheckDlgButton(IDC_CHECK2, (TrackerSettings::Instance().patternFont.Get().name == PATTERNFONT_LARGE) ? BST_CHECKED : BST_UNCHECKED);

	ShowWindow(SW_SHOW);

	return TRUE;
}


void WelcomeDlg::OnOptions()
{
	OnOK();
	CMainFrame::GetMainFrame()->PostMessage(WM_COMMAND, ID_VIEW_OPTIONS);
}


void WelcomeDlg::OnScanPlugins()
{
#ifndef NO_VST
	CSelectPluginDlg::ScanPlugins(m_vstPath, this);
#endif
}


void WelcomeDlg::OnOK()
{
	CDialog::OnOK();

#if defined(MPT_ENABLE_UPDATE)
	bool runUpdates = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;
	TrackerSettings::Instance().UpdateIntervalDays = (runUpdates ? 7 : -1);
	TrackerSettings::Instance().UpdateStatistics = (IsDlgButtonChecked(IDC_CHECK3) != BST_UNCHECKED);
	TrackerSettings::Instance().UpdateShowUpdateHint = false;
	TrackerSettings::Instance().UpdateStatisticsConsentAsked = true;
#endif // MPT_ENABLE_UPDATE
	if(IsDlgButtonChecked(IDC_CHECK2) != BST_UNCHECKED)
	{
		FontSetting font = TrackerSettings::Instance().patternFont;
		font.name = PATTERNFONT_LARGE;
		TrackerSettings::Instance().patternFont = font;
	}

	CComboBox *combo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	const char *keyFile = static_cast<char *>(combo->GetItemDataPtr(combo->GetCurSel()));
	auto cmdSet = std::make_unique<CCommandSet>();
	if(keyFile != nullptr)
		cmdSet->LoadFile(GetFullKeyPath(keyFile));
	else
		cmdSet->LoadDefaultKeymap();
	CMainFrame::GetInputHandler()->SetNewCommandSet(cmdSet.get());

#if defined(MPT_ENABLE_UPDATE)
	if(runUpdates)
	{
		CUpdateCheck::DoAutoUpdateCheck();
	}
#endif // MPT_ENABLE_UPDATE
	CMainFrame::GetMainFrame()->PostMessage(WM_MOD_INVALIDATEPATTERNS, HINT_MPTOPTIONS);

	DestroyWindow();
}

void WelcomeDlg::OnCancel()
{
	CDialog::OnCancel();
	DestroyWindow();
}

OPENMPT_NAMESPACE_END
