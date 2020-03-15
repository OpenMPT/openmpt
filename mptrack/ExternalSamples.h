/*
 * ExternalSamples.h
 * -----------------
 * Purpose: Dialogs for locating missing external samples and handling modified samples
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "ResizableDialog.h"
#include "CListCtrl.h"

OPENMPT_NAMESPACE_BEGIN

class CModDoc;
class CSoundFile;

class MissingExternalSamplesDlg : public ResizableDialog
{
protected:
	CModDoc &m_modDoc;
	CSoundFile &m_sndFile;
	CListCtrlEx m_List;
	bool m_isScanning = false;

public:
	MissingExternalSamplesDlg(CModDoc &modDoc, CWnd *parent);

protected:
	void GenerateList();
	bool SetSample(SAMPLEINDEX smp, const mpt::PathString &fileName);
	
	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;

	afx_msg void OnSetPath(NMHDR *, LRESULT *);
	afx_msg void OnScanFolder();

	DECLARE_MESSAGE_MAP()
};


class ModifiedExternalSamplesDlg : public ResizableDialog
{
protected:
	CModDoc &m_modDoc;
	CSoundFile &m_sndFile;
	CListCtrlEx m_List;

public:
	ModifiedExternalSamplesDlg(CModDoc &modDoc, CWnd *parent);

protected:
	void GenerateList();
	void Execute(bool doSave);

	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override { Execute(false); }

	afx_msg void OnSaveSelected() { Execute(true); }
	afx_msg void OnCheckAll();
	afx_msg void OnSelectionChanged(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
