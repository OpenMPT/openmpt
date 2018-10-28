/*
 * ExternalSamples.h
 * -----------------
 * Purpose: Dialog for locating missing external samples
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

class ExternalSamplesDlg : public ResizableDialog
{
protected:
	CModDoc &modDoc;
	CSoundFile &sndFile;
	CListCtrlEx m_List;
	bool isScanning = false;

public:
	ExternalSamplesDlg(CModDoc &modDoc, CWnd *parent);

protected:
	void GenerateList();
	bool SetSample(SAMPLEINDEX smp, const mpt::PathString &fileName);
	
	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;

	afx_msg void OnSetPath(NMHDR *, LRESULT *);
	afx_msg void OnScanFolder();

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
