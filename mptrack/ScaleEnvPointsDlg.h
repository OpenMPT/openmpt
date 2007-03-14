#pragma once
#include "sndfile.h"
#include "afxwin.h"

// CScaleEnvPointsDlg dialog

//=======================================
class CScaleEnvPointsDlg : public CDialog
//=======================================
{
	DECLARE_DYNAMIC(CScaleEnvPointsDlg)

public:
	CScaleEnvPointsDlg(CWnd* pParent, INSTRUMENTHEADER* pInst, BYTE env);   // standard constructor
	virtual ~CScaleEnvPointsDlg();

// Dialog Data
	enum { IDD = IDD_SCALE_ENV_POINTS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	INSTRUMENTHEADER* m_pInstrument;
	BYTE m_Env; //To tell which envelope to process.
	CEdit m_EditFactor;
protected:
	virtual void OnOK();
public:
	virtual BOOL OnInitDialog();
};
