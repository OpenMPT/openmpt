/*
 * ExternalSamples.cpp
 * -------------------
 * Purpose: Dialogs for locating missing external samples and handling modified samples
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Moddoc.h"
#include "ExternalSamples.h"
#include "FileDialog.h"
#include "FolderScanner.h"
#include "TrackerSettings.h"
#include "Reporting.h"
#include "resource.h"

OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(MissingExternalSamplesDlg, ResizableDialog)
	//{{AFX_MSG_MAP(ExternalSamplesDlg)
	ON_NOTIFY(NM_DBLCLK,	IDC_LIST1,	&MissingExternalSamplesDlg::OnSetPath)
	ON_COMMAND(IDC_BUTTON1,				&MissingExternalSamplesDlg::OnScanFolder)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void MissingExternalSamplesDlg::DoDataExchange(CDataExchange *pDX)
{
	ResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1,		m_List);
}


MissingExternalSamplesDlg::MissingExternalSamplesDlg(CModDoc &modDoc, CWnd *parent)
    : ResizableDialog(IDD_MISSINGSAMPLES, parent)
    , m_modDoc(modDoc)
    , m_sndFile(modDoc.GetSoundFile())
{
}


BOOL MissingExternalSamplesDlg::OnInitDialog()
{
	ResizableDialog::OnInitDialog();

	// Initialize table
	const CListCtrlEx::Header headers[] =
	{
		{ _T("Sample"),				128, LVCFMT_LEFT },
		{ _T("External Filename"),	308, LVCFMT_LEFT },
	};
	m_List.SetHeaders(headers);
	m_List.SetExtendedStyle(m_List.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	GenerateList();
	SetWindowText((_T("Missing External Samples - ") + m_modDoc.GetPathNameMpt().GetFullFileName().AsNative()).c_str());

	return TRUE;
}


void MissingExternalSamplesDlg::GenerateList()
{
	m_List.SetRedraw(FALSE);
	m_List.DeleteAllItems();
	CString s;
	for(SAMPLEINDEX smp = 1; smp <= m_sndFile.GetNumSamples(); smp++)
	{
		if(m_sndFile.IsExternalSampleMissing(smp))
		{
			s.Format(_T("%02u: "), smp);
			s += mpt::ToCString(m_sndFile.GetCharsetInternal(), m_sndFile.GetSampleName(smp));
			int insertAt = m_List.InsertItem(m_List.GetItemCount(), s);
			if(insertAt == -1)
				continue;
			m_List.SetItemText(insertAt, 1, m_sndFile.GetSamplePath(smp).AsNative().c_str());
			m_List.SetItemData(insertAt, smp);
		}
	}
	m_List.SetRedraw(TRUE);

	// Yay, we managed to find all samples!
	if(!m_List.GetItemCount())
		OnOK();
}


void MissingExternalSamplesDlg::OnSetPath(NMHDR *, LRESULT *)
{
	const int item = m_List.GetSelectionMark();
	if(item == -1) return;
	const SAMPLEINDEX smp = static_cast<SAMPLEINDEX>(m_List.GetItemData(item));

	const mpt::PathString path = m_modDoc.GetSoundFile().GetSamplePath(smp);
	FileDialog dlg = OpenFileDialog()
		.ExtensionFilter("All Samples|*.wav;*.flac|All files(*.*)|*.*||");  // Only show samples that we actually can save as well.
	if(TrackerSettings::Instance().previewInFileDialogs)
		dlg.EnableAudioPreview();
	if(path.empty())
		dlg.WorkingDirectory(TrackerSettings::Instance().PathSamples.GetWorkingDir());
	else
		dlg.DefaultFilename(path);
	if(!dlg.Show()) return;
	TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetWorkingDirectory());

	SetSample(smp, dlg.GetFirstFile());
	m_modDoc.UpdateAllViews(nullptr, SampleHint(smp).Info().Names().Data());
	GenerateList();
}


void MissingExternalSamplesDlg::OnScanFolder()
{
	if(m_isScanning)
	{
		m_isScanning = false;
		return;
	}

	BrowseForFolder dlg(TrackerSettings::Instance().PathSamples.GetWorkingDir(), _T("Select a folder to search for missing samples..."));
	if(dlg.Show())
	{
		TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetDirectory());

		FolderScanner scan(dlg.GetDirectory(), FolderScanner::kOnlyFiles | FolderScanner::kFindInSubDirectories);
		mpt::PathString fileName;

		m_isScanning = true;
		SetDlgItemText(IDC_BUTTON1, _T("&Cancel"));
		GetDlgItem(IDOK)->EnableWindow(FALSE);
		BeginWaitCursor();

		DWORD64 lastTick = GetTickCount64();
		int foundFiles = 0;

		bool anyMissing = true;
		while(scan.Next(fileName) && m_isScanning && anyMissing)
		{
			anyMissing = false;
			for(SAMPLEINDEX smp = 1; smp <= m_sndFile.GetNumSamples(); smp++)
			{
				if(m_sndFile.IsExternalSampleMissing(smp))
				{
					if(!mpt::PathString::CompareNoCase(m_sndFile.GetSamplePath(smp).GetFullFileName(), fileName.GetFullFileName()))
					{
						if(SetSample(smp, fileName))
						{
							foundFiles++;
						}
					} else
					{
						anyMissing = true;
					}
				}
			}

			const DWORD64 tick = GetTickCount64();
			if(tick < lastTick || tick > lastTick + 100)
			{
				lastTick = tick;
				SetDlgItemText(IDC_STATIC1, fileName.AsNative().c_str());
				MSG msg;
				while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					::TranslateMessage(&msg);
					::DispatchMessage(&msg);
				}
			}
		}
		EndWaitCursor();
		GetDlgItem(IDOK)->EnableWindow(TRUE);
		SetDlgItemText(IDC_BUTTON1, _T("&Scan Folder..."));

		m_modDoc.UpdateAllViews(nullptr, SampleHint().Info().Data().Names());

		if(foundFiles)
		{
			SetDlgItemText(IDC_STATIC1, MPT_CFORMAT("{} sample paths were relocated.")(foundFiles));
		} else
		{
			SetDlgItemText(IDC_STATIC1, _T("No matching sample names found."));
		}
		m_isScanning = false;
		GenerateList();
	}

}


bool MissingExternalSamplesDlg::SetSample(SAMPLEINDEX smp, const mpt::PathString &fileName)
{
	m_modDoc.GetSampleUndo().PrepareUndo(smp, sundo_replace, "Replace");
	const mpt::PathString oldPath = m_sndFile.GetSamplePath(smp);
	if(!m_sndFile.LoadExternalSample(smp, fileName))
	{
		Reporting::Information(_T("Unable to load sample:\n") + fileName.AsNative());
		m_modDoc.GetSampleUndo().RemoveLastUndoStep(smp);
		return false;
	} else
	{
		// Maybe we just put the file into its regular place, in which case the module has not really been modified.
		if(oldPath != fileName)
		{
			m_modDoc.SetModified();
		}
		return true;
	}
}


BEGIN_MESSAGE_MAP(ModifiedExternalSamplesDlg, ResizableDialog)
	//{{AFX_MSG_MAP(ExternalSamplesDlg)
	ON_COMMAND(IDC_SAVE,                  &ModifiedExternalSamplesDlg::OnSaveSelected)
	ON_COMMAND(IDC_CHECK1,                &ModifiedExternalSamplesDlg::OnCheckAll)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &ModifiedExternalSamplesDlg::OnSelectionChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void ModifiedExternalSamplesDlg::DoDataExchange(CDataExchange *pDX)
{
	ResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_List);
}


ModifiedExternalSamplesDlg::ModifiedExternalSamplesDlg(CModDoc &modDoc, CWnd *parent)
    : ResizableDialog(IDD_MODIFIEDSAMPLES, parent)
    , m_modDoc(modDoc)
    , m_sndFile(modDoc.GetSoundFile())
{
}


BOOL ModifiedExternalSamplesDlg::OnInitDialog()
{
	ResizableDialog::OnInitDialog();

	// Initialize table
	const CListCtrlEx::Header headers[] =
	{
		{_T("Sample"),            120, LVCFMT_LEFT},
		{_T("Status"),            54,  LVCFMT_LEFT},
		{_T("External Filename"), 262, LVCFMT_LEFT},
	};
	m_List.SetHeaders(headers);
	m_List.SetExtendedStyle(m_List.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

	GenerateList();
	SetWindowText((_T("Modified External Samples - ") + m_modDoc.GetPathNameMpt().GetFullFileName().AsNative()).c_str());

	return TRUE;
}


void ModifiedExternalSamplesDlg::GenerateList()
{
	m_List.SetRedraw(FALSE);
	m_List.DeleteAllItems();
	CString s;
	mpt::winstring status;
	for(SAMPLEINDEX smp = 1; smp <= m_sndFile.GetNumSamples(); smp++)
	{
		if(!m_sndFile.GetSample(smp).uFlags[SMP_KEEPONDISK])
			continue;

		if(m_sndFile.GetSample(smp).uFlags[SMP_MODIFIED])
			status = _T("modified");
		else if(!m_sndFile.GetSamplePath(smp).IsFile())
			status = _T("missing");
		else
			continue;

		s.Format(_T("%02u: "), smp);
		s += mpt::ToCString(m_sndFile.GetCharsetInternal(), m_sndFile.GetSampleName(smp));
		int insertAt = m_List.InsertItem(m_List.GetItemCount(), s);
		if(insertAt == -1)
			continue;
		m_List.SetItemText(insertAt, 1, status.c_str());
		m_List.SetItemText(insertAt, 2, m_sndFile.GetSamplePath(smp).AsNative().c_str());
		m_List.SetCheck(insertAt, TRUE);
		m_List.SetItemData(insertAt, smp);
	}
	m_List.SetRedraw(TRUE);

	CheckDlgButton(IDC_CHECK1, BST_CHECKED);
	OnSelectionChanged(nullptr, nullptr);

	// Nothing modified?
	if(!m_List.GetItemCount())
		OnOK();
}


void ModifiedExternalSamplesDlg::OnCheckAll()
{
	const BOOL check = IsDlgButtonChecked(IDC_CHECK1) ? TRUE : FALSE;
	const int count = m_List.GetItemCount();
	for(int i = 0; i < count; i++)
	{
		m_List.SetCheck(i, check);
	}
}


void ModifiedExternalSamplesDlg::OnSelectionChanged(NMHDR *, LRESULT *)
{
	int numChecked = 0;
	const int count = m_List.GetItemCount();
	for(int i = 0; i < count; i++)
	{
		if(m_List.GetCheck(i))
			numChecked++;
	}
	const TCHAR *embedText, *saveText;
	if(numChecked == count)
	{
		embedText = _T("&Embed All");
		saveText = _T("&Save All");
	} else if(!numChecked)
	{
		embedText = _T("&Embed None");
		saveText = _T("&Save None");
	} else
	{
		embedText = _T("&Embed Selected");
		saveText = _T("&Save Selected");
	}

	GetDlgItem(IDOK)->SetWindowText(embedText);
	GetDlgItem(IDC_SAVE)->SetWindowText(saveText);
}


void ModifiedExternalSamplesDlg::Execute(bool doSave)
{
	ScopedLogCapturer log(m_modDoc, _T("Modified Samples"), this);

	bool ok = true;
	const int count = m_List.GetItemCount();
	for(int i = 0; i < count; i++)
	{
		if(!m_List.GetCheck(i))
			continue;

		SAMPLEINDEX smp = static_cast<SAMPLEINDEX>(m_List.GetItemData(i));
		if(doSave)
		{
			ok &= m_modDoc.SaveSample(smp);
		} else
		{
			m_sndFile.GetSample(smp).uFlags.reset(SMP_KEEPONDISK);
			m_modDoc.SetModified();
		}
	}
	
	if(ok)
		ResizableDialog::OnOK();
	else
		ResizableDialog::OnCancel();
}


OPENMPT_NAMESPACE_END
