#ifndef TUNINGDIALOG_H
#define TUNINGDIALOG_H

#include "tuningratiomapwnd.h"
#include <vector>
#include <string>
#include "afxwin.h"

using std::vector;
using std::string;


// CTuningDialog dialog

class CTuningDialog : public CDialog
{
	DECLARE_DYNAMIC(CTuningDialog)

public:
	typedef vector<CTuningCollection*> TUNINGVECTOR;

public:
	CTuningDialog(CWnd* pParent = NULL, const TUNINGVECTOR& = TUNINGVECTOR(), CTuning* pTun = NULL);   // standard constructor
	virtual ~CTuningDialog();

	BOOL OnInitDialog();

	void AddTuningCollection(CTuningCollection* pTC) {if(pTC) m_TuningCollections.push_back(pTC);}
	void UpdateRatioMapEdits(const CTuning::STEPTYPE&);

// Dialog Data
	enum { IDD = IDD_TUNING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	void UpdateTuningType(const CTuning* pT);
	void SetView(const CTuning* pT = 0);
	void UpdateTempTuning(bool forceUpdate = false);
	CTuning::CTUNINGTYPE GetTuningTypeFromStr(const string& str) const;
	

private:
	CTuningRatioMapWnd m_RatioMapWnd;
	TUNINGVECTOR m_TuningCollections;

	CTuning* m_pTempTuning;
	CTuningCollection* m_pTempTuningCol;

	CComboBox m_CombobTuningCollection;
	CComboBox m_CombobTuningName;
	CComboBox m_CombobTuningType;

	CEdit m_EditTableSize;
	CEdit m_EditBeginNote;
	CEdit m_EditSteps;
	CEdit m_EditRatioPeriod;
	CEdit m_EditRatio;
	CEdit m_EditNotename;
	
	CButton m_CheckNewTuning;
	CButton m_ButtonAddTuning;
	CButton m_ButtonRemoveTuning;
	CButton m_ButtonSet;


public:
	afx_msg void OnCbnSelchangeComboTcol();
	afx_msg void OnCbnSelchangeComboT();
	afx_msg void OnBnClickedCheckNewtuning();
	afx_msg void OnCbnSelchangeComboTtype();
	afx_msg void OnEnChangeEditSteps();
	afx_msg void OnEnChangeEditRatioperiod();
	afx_msg void OnBnClickedAddTuning();
	afx_msg void OnBnClickedRemoveTuning();
	afx_msg void OnCbnEditchangeComboT();
	afx_msg void OnEnChangeEditNotename();
	afx_msg void OnBnClickedButtonSetvalues();

private:
	static const string s_stringTypeGEN;
	static const string s_stringTypeRP;
	static const string s_stringTypeTET;

	bool m_NoteEditApply;
	bool m_RatioEditApply;
	//To indicate whether to apply changes made to 
	//to those edit boxes.
	
public:
	afx_msg void OnEnChangeEditRatiovalue();
};

#endif
