#ifndef TUNINGDIALOG_H
#define TUNINGDIALOG_H

#include "tuningratiomapwnd.h"
#include "tuningcollection.h"
#include <vector>
#include <string>

using std::vector;
using std::string;


// CTuningDialog dialog

//==================================
class CTuningDialog : public CDialog
//==================================
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
	CTuning::CTUNINGTYPE GetTuningTypeFromStr(const string& str) const;

	void UpdateView();
	void UpdateTuningType();
	

private:
	CTuningRatioMapWnd m_RatioMapWnd;
	TUNINGVECTOR m_TuningCollections;
	vector<CTuningCollection*> m_DeletableTuningCollections;

	CTuning* m_pActiveTuning;
	CTuningCollection m_TempTunings;

	CComboBox m_CombobTuningCollection;
	CComboBox m_CombobTuningName;
	CComboBox m_CombobTuningType;

	CEdit m_EditTableSize;
	CEdit m_EditBeginNote;
	CEdit m_EditSteps;
	CEdit m_EditRatioPeriod;
	CEdit m_EditRatio;
	CEdit m_EditNotename;
	CEdit m_EditMiscActions;
	CEdit m_EditFineTuneSteps;
	CEdit m_EditName;
	
	CButton m_CheckNewTuning;
	CButton m_ButtonAddTuning;
	CButton m_ButtonRemoveTuning;
	CButton m_ButtonSet;
	CButton m_ButtonExport;
	CButton m_ButtonImport;
	CButton m_ButtonReadOnly;


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
	afx_msg void OnEnChangeEditRatiovalue();
	afx_msg void OnBnClickedButtonExport();
	afx_msg void OnBnClickedButtonImport();
	afx_msg void OnEnChangeEditFinetunesteps();
	afx_msg void OnEnKillfocusEditFinetunesteps();
	afx_msg void OnBnClickedCheckReadonly();
	afx_msg void OnEnKillfocusEditName();
	afx_msg void OnEnKillfocusEditSteps();
	afx_msg void OnEnKillfocusEditRatioperiod();
	afx_msg void OnEnKillfocusEditRatiovalue();
	afx_msg void OnEnKillfocusEditNotename();

private:
	static const string s_stringTypeGEN;
	static const string s_stringTypeRP;
	static const string s_stringTypeTET;

	bool m_NoteEditApply;
	bool m_RatioEditApply;
	//To indicate whether to apply changes made to 
	//to those edit boxes(they are modified by non-user 
	//activies and in these cases the value should be applied
	//to the tuning data.

};

#endif
