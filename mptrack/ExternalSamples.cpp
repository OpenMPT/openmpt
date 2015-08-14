/*
 * ExternalSamples.cpp
 * -------------------
 * Purpose: Dialog for locating missing external samples
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

BEGIN_MESSAGE_MAP(ExternalSamplesDlg, CDialog)
	//{{AFX_MSG_MAP(ExternalSamplesDlg)
	ON_NOTIFY(NM_DBLCLK,	IDC_LIST1,	OnSetPath)
	ON_COMMAND(IDC_BUTTON1,				OnScanFolder)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void ExternalSamplesDlg::DoDataExchange(CDataExchange* pDX)
//---------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModTypeDlg)
	DDX_Control(pDX, IDC_LIST1,		m_List);
	//}}AFX_DATA_MAP
}


ExternalSamplesDlg::ExternalSamplesDlg(CModDoc &modDoc, CWnd *parent) : CDialog(IDD_MISSINGSAMPLES, parent), modDoc(modDoc), sndFile(modDoc.GetrSoundFile()), isScanning(false)
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
}


BOOL ExternalSamplesDlg::OnInitDialog()
//-------------------------------------
{
	CDialog::OnInitDialog();

	// Initialize table
	const CListCtrlEx::Header headers[] =
	{
		{ _T("Sample"),				128, LVCFMT_LEFT },
		{ _T("External Filename"),	308, LVCFMT_LEFT },
	};
	m_List.SetHeaders(headers);
	m_List.SetExtendedStyle(m_List.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	GenerateList();
	::SetWindowTextW(m_hWnd, (L"Missing External Samples - " + modDoc.GetPathNameMpt().GetFullFileName().AsNative()).c_str());

	return TRUE;
}


void ExternalSamplesDlg::GenerateList()
//-------------------------------------
{
	m_List.SetRedraw(FALSE);
	m_List.DeleteAllItems();
	CString s;
	for(SAMPLEINDEX smp = 1; smp <= sndFile.GetNumSamples(); smp++)
	{
		if(sndFile.IsExternalSampleMissing(smp))
		{
			s.Format(_T("%02u: "), smp);
			s += sndFile.GetSampleName(smp);
			int insertAt = m_List.InsertItem(m_List.GetItemCount(), s);
			if(insertAt == -1)
				continue;
			m_List.SetItemText(insertAt, 1, sndFile.GetSamplePath(smp).AsNative().c_str());
			m_List.SetItemData(insertAt, smp);
		}
	}
	m_List.SetRedraw(TRUE);

	// Yay, we managed to find all samples!
	if(!m_List.GetItemCount())
	{
		OnOK();
	}
}


void ExternalSamplesDlg::OnSetPath(NMHDR *, LRESULT *)
//----------------------------------------------------
{
	const int item = m_List.GetSelectionMark();
	if(item == -1) return;
	const SAMPLEINDEX smp = static_cast<SAMPLEINDEX>(m_List.GetItemData(item));

	const mpt::PathString path = modDoc.GetrSoundFile().GetSamplePath(smp);
	FileDialog dlg = OpenFileDialog()
		.ExtensionFilter("All Samples|*.wav;*.flac|All files(*.*)|*.*||");	// Only show samples that we actually can save as well.
	if(TrackerSettings::Instance().previewInFileDialogs)
		dlg.EnableAudioPreview();
	if(path.empty())
		dlg.WorkingDirectory(TrackerSettings::Instance().PathSamples.GetWorkingDir());
	else
		dlg.DefaultFilename(path);
	if(!dlg.Show()) return;
	TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetWorkingDirectory());

	SetSample(smp, dlg.GetFirstFile());
	modDoc.UpdateAllViews(nullptr, SampleHint(smp).Info().Names().Data());
	GenerateList();
}


void ExternalSamplesDlg::OnScanFolder()
//-------------------------------------
{
	if(isScanning)
	{
		isScanning = false;
		return;
	}

	BrowseForFolder dlg(TrackerSettings::Instance().PathSamples.GetWorkingDir(), _T("Select a folder to search for missing samples..."));
	if(dlg.Show())
	{
		TrackerSettings::Instance().PathSamples.SetWorkingDir(dlg.GetDirectory());

		FolderScanner scan(dlg.GetDirectory(), true);
		mpt::PathString fileName;

		isScanning = true;
		SetDlgItemText(IDC_BUTTON1, _T("&Cancel"));
		GetDlgItem(IDOK)->EnableWindow(FALSE);
		BeginWaitCursor();

		DWORD lastTick = GetTickCount();
		int foundFiles = 0;

		bool anyMissing = true;
		while(scan.NextFile(fileName) && isScanning && anyMissing)
		{
			anyMissing = false;
			for(SAMPLEINDEX smp = 1; smp <= sndFile.GetNumSamples(); smp++)
			{
				if(sndFile.IsExternalSampleMissing(smp))
				{
					if(!mpt::PathString::CompareNoCase(sndFile.GetSamplePath(smp).GetFullFileName(), fileName.GetFullFileName()))
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

			const DWORD tick = GetTickCount();
			if(tick < lastTick || tick > lastTick + 100)
			{
				lastTick = tick;
				::SetDlgItemTextW(m_hWnd, IDC_STATIC1, fileName.AsNative().c_str());
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

		modDoc.UpdateAllViews(nullptr, SampleHint().Info().Data().Names());

		if(foundFiles)
		{
			SetDlgItemText(IDC_STATIC1, mpt::ToCString(mpt::String::Print(MPT_USTRING("%1 sample paths were relocated."), foundFiles)));
		} else
		{
			SetDlgItemText(IDC_STATIC1, _T("No matching sample names found."));
		}
		isScanning = false;
		GenerateList();
	}

}


bool ExternalSamplesDlg::SetSample(SAMPLEINDEX smp, const mpt::PathString &fileName)
//----------------------------------------------------------------------------------
{
	modDoc.GetSampleUndo().PrepareUndo(smp, sundo_replace, "Replace");
	const mpt::PathString oldPath = sndFile.GetSamplePath(smp);
	if(!sndFile.LoadExternalSample(smp, fileName))
	{
		Reporting::Information(L"Unable to load sample:\n" + fileName.AsNative());
		modDoc.GetSampleUndo().RemoveLastUndoStep(smp);
		return false;
	} else
	{
		// Maybe we just put the file into its regular place, in which case the module has not really been modified.
		if(oldPath != fileName)
		{
			modDoc.SetModified();
		}
		return true;
	}
}

OPENMPT_NAMESPACE_END
