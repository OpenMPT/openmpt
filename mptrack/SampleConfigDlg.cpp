/*
 * SampleConfigDlg.cpp
 * -------------------
 * Purpose: Implementation of the sample/instrument editor settings dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "SampleConfigDlg.h"


OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(COptionsSampleEditor, CPropertyPage)
	ON_WM_HSCROLL()
	ON_EN_CHANGE(IDC_EDIT_UNDOSIZE,			&COptionsSampleEditor::OnUndoSizeChanged)
	ON_EN_CHANGE(IDC_EDIT_FINETUNE,			&COptionsSampleEditor::OnSettingsChanged)
	ON_EN_CHANGE(IDC_FLAC_COMPRESSION,		&COptionsSampleEditor::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_DEFAULT_FORMAT,	&COptionsSampleEditor::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_VOLUME_HANDLING,	&COptionsSampleEditor::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO1,					&COptionsSampleEditor::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO2,					&COptionsSampleEditor::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO3,					&COptionsSampleEditor::OnSettingsChanged)
	ON_COMMAND(IDC_COMPRESS_ITI,			&COptionsSampleEditor::OnSettingsChanged)
	ON_COMMAND(IDC_PREVIEW_SAMPLES,			&COptionsSampleEditor::OnSettingsChanged)
	ON_COMMAND(IDC_NORMALIZE,				&COptionsSampleEditor::OnSettingsChanged)
	ON_COMMAND(IDC_CURSORINHEX,				&COptionsSampleEditor::OnSettingsChanged)
END_MESSAGE_MAP()


void COptionsSampleEditor::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsSampleEditor)
	DDX_Control(pDX, IDC_DEFAULT_FORMAT, m_cbnDefaultSampleFormat);
	DDX_Control(pDX, IDC_VOLUME_HANDLING, m_cbnDefaultVolumeHandling);
	DDX_Control(pDX, IDC_COMBO3, m_cbnFollowSamplePlayCursor);
	//}}AFX_DATA_MAP
}


BOOL COptionsSampleEditor::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	SetDlgItemInt(IDC_EDIT_UNDOSIZE, TrackerSettings::Instance().m_SampleUndoBufferSize.Get().GetSizeInPercent());
	SetDlgItemInt(IDC_EDIT_FINETUNE, TrackerSettings::Instance().m_nFinetuneStep);
	static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN1))->SetRange32(0, 100);
	static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN2))->SetRange32(1, 200);
	RecalcUndoSize();

	m_cbnDefaultSampleFormat.SetItemData(m_cbnDefaultSampleFormat.AddString(_T("FLAC")), dfFLAC);
	m_cbnDefaultSampleFormat.SetItemData(m_cbnDefaultSampleFormat.AddString(_T("WAV")), dfWAV);
	m_cbnDefaultSampleFormat.SetItemData(m_cbnDefaultSampleFormat.AddString(_T("RAW")), dfRAW);
	m_cbnDefaultSampleFormat.SetItemData(m_cbnDefaultSampleFormat.AddString(_T("S3I")), dfS3I);
	m_cbnDefaultSampleFormat.SetCurSel(TrackerSettings::Instance().m_defaultSampleFormat);

	CSliderCtrl *slider = static_cast<CSliderCtrl *>(GetDlgItem(IDC_SLIDER1));
	slider->SetRange(0, 8);
	slider->SetTicFreq(1);
	slider->SetPos(TrackerSettings::Instance().m_FLACCompressionLevel);

	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1 + TrackerSettings::Instance().sampleEditorKeyBehaviour);

	m_cbnFollowSamplePlayCursor.SetItemData(m_cbnFollowSamplePlayCursor.AddString(_T("Do not follow play cursor")), int(FollowSamplePlayCursor::DoNotFollow));
	m_cbnFollowSamplePlayCursor.SetItemData(m_cbnFollowSamplePlayCursor.AddString(_T("Follow")), int(FollowSamplePlayCursor::Follow));
	m_cbnFollowSamplePlayCursor.SetItemData(m_cbnFollowSamplePlayCursor.AddString(_T("Follow centered")), int(FollowSamplePlayCursor::FollowCentered));
	m_cbnFollowSamplePlayCursor.SetCurSel(static_cast<int>(TrackerSettings::Instance().m_followSamplePlayCursor.Get()));

	CheckDlgButton(IDC_COMPRESS_ITI, TrackerSettings::Instance().compressITI ? BST_CHECKED : BST_UNCHECKED);

	m_cbnDefaultVolumeHandling.SetItemData(m_cbnDefaultVolumeHandling.AddString(_T("MIDI volume")), PLUGIN_VOLUMEHANDLING_MIDI);
	m_cbnDefaultVolumeHandling.SetItemData(m_cbnDefaultVolumeHandling.AddString(_T("Dry/Wet ratio")), PLUGIN_VOLUMEHANDLING_DRYWET);
	m_cbnDefaultVolumeHandling.SetItemData(m_cbnDefaultVolumeHandling.AddString(_T("None")), PLUGIN_VOLUMEHANDLING_IGNORE);
	m_cbnDefaultVolumeHandling.SetCurSel(TrackerSettings::Instance().DefaultPlugVolumeHandling);

	CheckDlgButton(IDC_PREVIEW_SAMPLES, TrackerSettings::Instance().previewInFileDialogs ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_NORMALIZE, TrackerSettings::Instance().m_MayNormalizeSamplesOnLoad ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CURSORINHEX, TrackerSettings::Instance().cursorPositionInHex ? BST_CHECKED : BST_UNCHECKED);

	return TRUE;
}


void COptionsSampleEditor::OnOK()
{
	CPropertyPage::OnOK();

	TrackerSettings::Instance().m_nFinetuneStep = GetDlgItemInt(IDC_EDIT_FINETUNE);
	TrackerSettings::Instance().m_SampleUndoBufferSize = SampleUndoBufferSize(GetDlgItemInt(IDC_EDIT_UNDOSIZE));
	TrackerSettings::Instance().m_defaultSampleFormat = static_cast<SampleEditorDefaultFormat>(m_cbnDefaultSampleFormat.GetItemData(m_cbnDefaultSampleFormat.GetCurSel()));
	TrackerSettings::Instance().m_followSamplePlayCursor = static_cast<FollowSamplePlayCursor>(m_cbnFollowSamplePlayCursor.GetItemData(m_cbnFollowSamplePlayCursor.GetCurSel()));
	TrackerSettings::Instance().m_FLACCompressionLevel = static_cast<CSliderCtrl *>(GetDlgItem(IDC_SLIDER1))->GetPos();
	TrackerSettings::Instance().sampleEditorKeyBehaviour = static_cast<SampleEditorKeyBehaviour>(GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO3) - IDC_RADIO1);
	TrackerSettings::Instance().compressITI = IsDlgButtonChecked(IDC_COMPRESS_ITI) != BST_UNCHECKED;
	TrackerSettings::Instance().DefaultPlugVolumeHandling = static_cast<PlugVolumeHandling>(m_cbnDefaultVolumeHandling.GetItemData(m_cbnDefaultVolumeHandling.GetCurSel()));
	TrackerSettings::Instance().previewInFileDialogs = IsDlgButtonChecked(IDC_PREVIEW_SAMPLES) != BST_UNCHECKED;
	TrackerSettings::Instance().m_MayNormalizeSamplesOnLoad = IsDlgButtonChecked(IDC_NORMALIZE) != BST_UNCHECKED;
	TrackerSettings::Instance().cursorPositionInHex = IsDlgButtonChecked(IDC_CURSORINHEX) != BST_UNCHECKED;

	for(auto modDoc : theApp.GetOpenDocuments())
	{
		modDoc->GetSampleUndo().RestrictBufferSize();
	}
}


BOOL COptionsSampleEditor::OnSetActive()
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_SAMPLEDITOR;
	return CPropertyPage::OnSetActive();
}


void COptionsSampleEditor::OnUndoSizeChanged()
{
	RecalcUndoSize();
	OnSettingsChanged();
}


void COptionsSampleEditor::RecalcUndoSize()
{
	UINT sizePercent = GetDlgItemInt(IDC_EDIT_UNDOSIZE);
	uint32 sizeMB = mpt::saturate_cast<uint32>(SampleUndoBufferSize(sizePercent).GetSizeInBytes() >> 20);
	CString text = _T("% of physical memory (");
	if(sizePercent)
		text.AppendFormat(_T("%u MiB)"), sizeMB);
	else
		text.Append(_T("disabled)"));
	SetDlgItemText(IDC_UNDOSIZE, text);
}

OPENMPT_NAMESPACE_END
