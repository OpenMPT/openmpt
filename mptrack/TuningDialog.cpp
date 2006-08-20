// TuningDialog.cpp : implementation file
//

#include "stdafx.h"
#include "mptrack.h"
#include "TuningDialog.h"
#include ".\tuningdialog.h"

const string CTuningDialog::s_stringTypeGEN = "General";
const string CTuningDialog::s_stringTypeRP = "Ratio periodic";
const string CTuningDialog::s_stringTypeTET = "TET";

/*
TODOS:
-Clear tuning(when copying existing tuning, it might have
 e.g. unwanted note names which should able to be removed.
-Show note name and ratio value also in the set-editboxes
 (->clear note names)
*/


// CTuningDialog dialog

IMPLEMENT_DYNAMIC(CTuningDialog, CDialog)
CTuningDialog::CTuningDialog(CWnd* pParent, const TUNINGVECTOR& rVec, CTuning* pTun)
	: CDialog(CTuningDialog::IDD, pParent),
	m_TuningCollections(rVec),
	m_pTempTuning(0),
	m_pTempTuningCol(0),
	m_NoteEditApply(true),
	m_RatioEditApply(true)
//----------------------------------------
{
	m_RatioMapWnd.m_pTuning = pTun; //pTun is the tuning to show when dialog opens.

	//Requiring that there is always at least one tuning collection.
	if(m_TuningCollections.size() == 0)
	{
		m_pTempTuningCol = new CTuningCollection("Tuning dialog temp");
		m_TuningCollections.push_back(m_pTempTuningCol);
	}

}

CTuningDialog::~CTuningDialog()
//----------------------------
{
	delete m_pTempTuning;
	delete m_pTempTuningCol;
}

BOOL CTuningDialog::OnInitDialog()
//--------------------------------
{
	CDialog::OnInitDialog();
	
	//CWnd::EnableToolTips(true);
	//Todo: Tooltips.

	m_RatioMapWnd.Init(this, 0);

	//Adding tuning collection names to the combobox.
	for(size_t i = 0; i<m_TuningCollections.size(); i++)
	{
		m_CombobTuningCollection.AddString(m_TuningCollections[i]->GetName().c_str());
	}

	//Adding tuning type names to corresponding combobox
	m_CombobTuningType.AddString(s_stringTypeGEN.c_str());
	m_CombobTuningType.AddString(s_stringTypeRP.c_str());
	m_CombobTuningType.AddString(s_stringTypeTET.c_str());

	m_ButtonSet.EnableWindow(FALSE);

	SetView(m_RatioMapWnd.m_pTuning);

	return TRUE;
}

void CTuningDialog::SetView(const CTuning* pTun)
//--------------------------------------------
{
	if(!m_TuningCollections[0]) {ASSERT(false); return;}
	if(m_TuningCollections[0]->GetNumTunings() == 0)
	{
		ASSERT(false);
		return;
	}

	if(!pTun) pTun = &m_TuningCollections[0]->GetTuning(0);

	//Finding out where given tuning belongs to.
	size_t curTCol = 0, curT = 0; //cur <-> Current, T <-> Tuning, Col <-> Collection.
	for(size_t i = 0; i<m_TuningCollections.size(); i++)
	{
		CTuningCollection& rCurTCol = *m_TuningCollections.at(i);
		for(size_t j = 0; j<rCurTCol.GetNumTunings(); j++)
		{
			if(pTun == &rCurTCol.GetTuning(static_cast<unsigned short>(j)))
			{
				curTCol = i;
				curT = j;
				break;
			}
		}
	}

	m_CombobTuningCollection.SetCurSel(curTCol);
	OnCbnSelchangeComboTcol();
	m_CombobTuningName.SetCurSel(curT);
	OnCbnSelchangeComboT();
}


void CTuningDialog::DoDataExchange(CDataExchange* pDX)
//-----------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATICRATIOMAP, m_RatioMapWnd);
	DDX_Control(pDX, IDC_COMBO_TCOL, m_CombobTuningCollection);
	DDX_Control(pDX, IDC_COMBO_T, m_CombobTuningName);
	DDX_Control(pDX, IDC_COMBO_TTYPE, m_CombobTuningType);
	DDX_Control(pDX, IDC_EDIT_TABLESIZE, m_EditTableSize);
	DDX_Control(pDX, IDC_EDIT_BEGINNOTE, m_EditBeginNote);
	DDX_Control(pDX, IDC_EDIT_STEPS, m_EditSteps);
	DDX_Control(pDX, IDC_EDIT_RATIOPERIOD, m_EditRatioPeriod);
	DDX_Control(pDX, IDC_CHECK_NEWTUNING, m_CheckNewTuning);
	DDX_Control(pDX, IDC_ADD_TUNING, m_ButtonAddTuning);
	DDX_Control(pDX, IDC_REMOVE_TUNING, m_ButtonRemoveTuning);
	DDX_Control(pDX, IDC_EDIT_RATIOVALUE, m_EditRatio);
	DDX_Control(pDX, IDC_EDIT_NOTENAME, m_EditNotename);
	DDX_Control(pDX, IDC_BUTTON_SETVALUES, m_ButtonSet);
}


BEGIN_MESSAGE_MAP(CTuningDialog, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_TCOL, OnCbnSelchangeComboTcol)
	ON_CBN_SELCHANGE(IDC_COMBO_T, OnCbnSelchangeComboT)
	ON_BN_CLICKED(IDC_CHECK_NEWTUNING, OnBnClickedCheckNewtuning)
	ON_CBN_SELCHANGE(IDC_COMBO_TTYPE, OnCbnSelchangeComboTtype)
	ON_EN_CHANGE(IDC_EDIT_STEPS, OnEnChangeEditSteps)
	ON_EN_CHANGE(IDC_EDIT_RATIOPERIOD, OnEnChangeEditRatioperiod)
	ON_BN_CLICKED(IDC_ADD_TUNING, OnBnClickedAddTuning)
	ON_BN_CLICKED(IDC_REMOVE_TUNING, OnBnClickedRemoveTuning)
	ON_CBN_EDITCHANGE(IDC_COMBO_T, OnCbnEditchangeComboT)
	ON_EN_CHANGE(IDC_EDIT_NOTENAME, OnEnChangeEditNotename)
	ON_BN_CLICKED(IDC_BUTTON_SETVALUES, OnBnClickedButtonSetvalues)
	ON_EN_CHANGE(IDC_EDIT_RATIOVALUE, OnEnChangeEditRatiovalue)
END_MESSAGE_MAP()


// CTuningDialog message handlers

void CTuningDialog::OnCbnSelchangeComboTcol()
//-------------------------------------------
{
	const int curTCol = m_CombobTuningCollection.GetCurSel();
	const int curT = m_CombobTuningName.GetCurSel();
	if(curTCol < 0 || curTCol >= static_cast<int>(m_TuningCollections.size())) return;

	//Checking whether collection allows removing
	if(m_TuningCollections[curTCol]->CanEdit(CTuningCollection::EM_REMOVE))
		m_ButtonRemoveTuning.EnableWindow(true);
	else
		m_ButtonRemoveTuning.EnableWindow(false);

	if(IsDlgButtonChecked(IDC_CHECK_NEWTUNING))
	{
		//Checking whether tuning collection allows adding
		//tunings...
		if(m_TuningCollections.at(curTCol)->CanEdit(CTuningCollection::EM_ADD))
			m_ButtonAddTuning.EnableWindow();
		else
			m_ButtonAddTuning.EnableWindow(false);
		
		return;
	}

	//Clearing existing items from name-combobox...
	while(m_CombobTuningName.GetCount() > 0)
		m_CombobTuningName.DeleteString(0);

	//...checking does the tuning collection have any
	//tunings in it...
	if(m_TuningCollections[curTCol]->GetNumTunings() == 0)
	{
		m_CombobTuningName.AddString("No tunings");
		m_CombobTuningName.SetCurSel(0);
		m_RatioMapWnd.m_pTuning = 0;
		//Hiding ratiomap window in this case.
		m_RatioMapWnd.ShowWindow(SW_HIDE);
		UpdateTuningType(0);
		return;
	}

	//...adding names of tunings in the current tuning collection...
	CTuningCollection& rTCol = *m_TuningCollections.at(curTCol);
	for(size_t i = 0; i<rTCol.GetNumTunings(); i++)
	{
		m_CombobTuningName.AddString(rTCol.GetTuning(static_cast<unsigned short>(i)).GetName().c_str());
	}
	
	//...checking can previous tuning item be used...
	if(curT < 0 || curT >= m_CombobTuningName.GetCount())
		m_CombobTuningName.SetCurSel(0);
	else
		m_CombobTuningName.SetCurSel(curT);

	OnCbnSelchangeComboT();
}

void CTuningDialog::OnCbnSelchangeComboT()
//----------------------------------------
{
	//Checking that there is valid Tuning collection index...
	const int TCol = m_CombobTuningCollection.GetCurSel();
	if(TCol < 0 || TCol >= static_cast<int>(m_TuningCollections.size())) return;

	CTuningCollection& rTCol = *m_TuningCollections.at(TCol);

	//...checking that tuning index is valid...
	const unsigned short T = static_cast<unsigned short>(m_CombobTuningName.GetCurSel());
	if(T < 0 || T >= rTCol.GetNumTunings())
		return;

	//...updating tuning type-combob...
	UpdateTuningType(&rTCol.GetTuning(T));

	//...and making sure that ratiomap window is showing and
	//updating its content.
	m_RatioMapWnd.ShowWindow(SW_SHOW);
	m_RatioMapWnd.m_pTuning = &rTCol.GetTuning(T);
	m_RatioMapWnd.Invalidate();
	UpdateRatioMapEdits(m_RatioMapWnd.GetShownCentre());
}

void CTuningDialog::UpdateTuningType(const CTuning* pT)
//-----------------------------------------------------
{
	if(pT)
	{
		ASSERT(m_CombobTuningType.GetCount() > 0);
		if(pT->GetTuningType() == CTuning::TT_TET)
			m_CombobTuningType.SetCurSel(2);
		else
			//if(pT->GetPeriod())
			if(pT->GetTuningType() == CTuning::TT_RATIOPERIODIC)
				m_CombobTuningType.SetCurSel(1);
			else
				m_CombobTuningType.SetCurSel(0);


		//Updating ratioperiod and steps in ratio fields...
		CTuning::RATIOTYPE rp = pT->GetPeriodRatio();
		CTuning::STEPTYPE period = pT->GetPeriod();
		if(period)
		{
			m_EditSteps.EnableWindow();
			m_EditSteps.SetWindowText(Stringify(period).c_str());

			m_EditRatioPeriod.EnableWindow();
			m_EditRatioPeriod.SetWindowText(Stringify(rp).c_str());
		}
		else
		{
			if(!IsDlgButtonChecked(IDC_CHECK_NEWTUNING))
			{
				m_EditSteps.EnableWindow(false);
				m_EditRatioPeriod.EnableWindow(false);
			}
		}
	}
	m_CombobTuningType.Invalidate();
	m_EditSteps.Invalidate();
	m_EditRatioPeriod.Invalidate();
	
}

void CTuningDialog::OnBnClickedCheckNewtuning()
//---------------------------------------------
{
	
	if(IsDlgButtonChecked(IDC_CHECK_NEWTUNING))
	{
		//Bug?
		if(m_pTempTuning != NULL)
		{
			MessageBox("Bug: m_pTempTuning != NULL in CTuningDialog::OnBnClickedCheckNewtuning()", 0);
			delete m_pTempTuning; m_pTempTuning = NULL;
			m_RatioMapWnd.m_pTuning = NULL;
			return;
		}

		m_ButtonSet.EnableWindow();
		m_ButtonRemoveTuning.ShowWindow(SW_HIDE);
		m_RatioMapWnd.ShowWindow(SW_SHOW);
		m_ButtonAddTuning.ShowWindow(SW_SHOW);
		
		const size_t TCol = static_cast<size_t>(m_CombobTuningCollection.GetCurSel());
		if(TCol >= 0 && TCol < m_TuningCollections.size())
			if(m_TuningCollections[TCol]->CanEdit(CTuningCollection::EM_ADD))
				m_ButtonAddTuning.EnableWindow();
			else
				m_ButtonAddTuning.EnableWindow(FALSE);

		//Deleting tuning names from combob.
		while(m_CombobTuningName.GetCount() > 0)
			m_CombobTuningName.DeleteString(0);
		//Invalidating later.


		//Creating temporary tuning object and using 
		//previously shown tuning as 'template'.
		m_pTempTuning = new CTuningRTI(m_RatioMapWnd.m_pTuning);
		m_RatioMapWnd.m_pTuning = m_pTempTuning;
		m_RatioMapWnd.Invalidate();
		UpdateRatioMapEdits(m_RatioMapWnd.GetShownCentre());

		//Allowing modifications to edit boxes

		m_EditSteps.SetReadOnly(FALSE);
		m_EditSteps.EnableWindow();
		m_EditRatioPeriod.SetReadOnly(FALSE);
		m_EditRatioPeriod.EnableWindow();
		m_EditSteps.Invalidate();
		m_EditRatioPeriod.Invalidate();

		//Updating name combob:
		m_CombobTuningName.SetWindowText(m_pTempTuning->GetName().c_str());
		m_CombobTuningName.Invalidate();

		//Enabling tuning type-combobox
		m_CombobTuningType.EnableWindow(true);

		
	}
	else //Case: Editing -> Inspection transition.
	{
		//Using m_pTempTuning == NULL as indicator that
		//tuning was saved.
		if(m_pTempTuning == NULL)
		{
			m_ButtonAddTuning.ShowWindow(SW_HIDE);
			m_ButtonRemoveTuning.ShowWindow(SW_SHOW);
			m_ButtonSet.EnableWindow(FALSE);
			m_EditSteps.SetReadOnly();
			m_EditRatioPeriod.SetReadOnly();
			SetView(m_RatioMapWnd.m_pTuning);
		}
		else
		{
			CheckDlgButton(IDC_CHECK_NEWTUNING, MF_CHECKED);
			if(MessageBox("Discard current unsaved tuning?", 0, MB_YESNO) == IDYES)
			{
				CheckDlgButton(IDC_CHECK_NEWTUNING, MF_UNCHECKED);
				m_RatioMapWnd.m_pTuning = 0;
				delete m_pTempTuning; m_pTempTuning = 0;
				m_ButtonSet.EnableWindow(FALSE);
				m_ButtonAddTuning.ShowWindow(SW_HIDE);
				m_ButtonRemoveTuning.ShowWindow(SW_SHOW);
				m_EditSteps.SetReadOnly();
				m_EditRatioPeriod.SetReadOnly();
				SetView();
			}
			else
				CheckDlgButton(IDC_CHECK_NEWTUNING, MF_CHECKED);
		}
	}
	



}

CTuning::CTUNINGTYPE CTuningDialog::GetTuningTypeFromStr(const string& str) const
//--------------------------------------------------------------------------------
{
	if(str == s_stringTypeTET)
		return CTuning::TT_TET;
	if(str == s_stringTypeRP)
		return CTuning::TT_RATIOPERIODIC;
	return CTuning::TT_GENERAL;
}

void CTuningDialog::OnCbnSelchangeComboTtype()
//--------------------------------------------
{
	if(IsDlgButtonChecked(IDC_CHECK_NEWTUNING))
	{
		if(!m_pTempTuning) return;

		const CTuning::CTUNINGTYPE oldType = m_pTempTuning->GetTuningType();

		const size_t BS = 20;
		char buffer[BS];
		m_CombobTuningType.GetWindowText(buffer, BS);
		const string strNewType = buffer;

		CTuning::CTUNINGTYPE newType = GetTuningTypeFromStr(strNewType);


		if(!m_pTempTuning->DoesTypeInclude(newType))
		{
			if(MessageBox("This will change the ratio values; continue?", 0, MB_YESNO) == IDYES)
			{
				m_EditRatioPeriod.EnableWindow();
				m_EditSteps.EnableWindow();
				m_EditRatioPeriod.Invalidate();
				m_EditSteps.Invalidate();
				UpdateTempTuning(true);
			}
			else //Not wanting to discard current values.
			{
				//Restoring tuning type combobox.
				if(oldType == CTuning::TT_TET)
					m_CombobTuningType.SetCurSel(2);
				else
				{
					if(oldType == CTuning::TT_RATIOPERIODIC)
						m_CombobTuningType.SetCurSel(1);
					else 
						m_CombobTuningType.SetCurSel(0);
				}
			}
		}
		else
			return;
	}
	else
	{
		//Should never come here?
		ASSERT(false);
	}
}


void CTuningDialog::OnEnChangeEditSteps()
//---------------------------------------
{
	UpdateTempTuning();
}

void CTuningDialog::OnEnChangeEditRatioperiod()
//---------------------------------------------
{
	UpdateTempTuning();
}

void CTuningDialog::UpdateTempTuning(const bool forceUpdate)
//------------------------------------
{
	if(m_pTempTuning == NULL)
		return;

	const size_t BS = 20;
	char buffer[BS];
	m_EditSteps.GetWindowText(buffer, BS);
	CTuning::STEPTYPE steps = static_cast<CTuning::STEPTYPE>(atoi(buffer));

	m_EditRatioPeriod.GetWindowText(buffer, BS);
	CTuning::RATIOTYPE pr = static_cast<CTuning::RATIOTYPE>(atof(buffer));

	if(steps <= 0 || pr <= 0)
	{
		if(forceUpdate)
		{
			steps = 2; //Just putting some figures.
			pr = 2; //Just putting some figures.
			m_EditSteps.SetWindowText(Stringify(steps).c_str());
			m_EditRatioPeriod.SetWindowText(Stringify(pr).c_str());
			m_EditSteps.Invalidate();
			m_EditRatioPeriod.Invalidate();
		}
		else
			return;
	}

	m_CombobTuningType.GetWindowText(buffer, BS);
	if(string(buffer) == s_stringTypeRP)
	{
		//Using values in the current temp tuning for creating
		//the new RP tuning.
		vector<CTuning::RATIOTYPE> v;
		for(CTuning::STEPTYPE i = 0; i<steps; i++)
			v.push_back(m_pTempTuning->GetFrequencyRatio(i, 0));
		m_pTempTuning->CreateRatioPeriodic(v, pr);
	}
	else
		if(string(buffer) == s_stringTypeTET)
		{
			m_pTempTuning->CreateTET(steps, pr);
		}
	
	m_RatioMapWnd.Invalidate();
	UpdateRatioMapEdits(m_RatioMapWnd.GetShownCentre());
}

void CTuningDialog::OnBnClickedAddTuning()
//-----------------------------------------
{
	if(m_pTempTuning == NULL) //Should never be true.
		return;

	if(IsDlgButtonChecked(IDC_CHECK_NEWTUNING))
	{
		if(m_pTempTuning->GetName().length() == 0)
		{
			MessageBox("Please give name for the tuning", 0, MB_OK);
			return;
		}

		string detailStr;
		detailStr += "Add tuning?\n\nTuning details:\n";
		detailStr += string("Name: ") + m_pTempTuning->GetName() + string("\n");
		detailStr += string("Type: ") + m_pTempTuning->GetTuningTypeStr(m_pTempTuning->GetTuningType());
		if(MessageBox(detailStr.c_str(), 0, MB_YESNO) == IDYES)
		{
			if(m_CombobTuningCollection.GetCurSel() < 0 || m_CombobTuningCollection.GetCurSel() >= static_cast<int>(m_TuningCollections.size()))
			{
				ASSERT(false); 
				return;
			}
			CTuningCollection& rTCol = *m_TuningCollections[m_CombobTuningCollection.GetCurSel()];
			if(rTCol.AddTuning(m_pTempTuning))
			{
				MessageBox("Add tuning failed");
				return;
			}
			m_pTempTuning = NULL; //Used as an indicator that tuning was saved.
			CheckDlgButton(IDC_CHECK_NEWTUNING, MF_UNCHECKED);
			OnBnClickedCheckNewtuning();
		}
	}
}

void CTuningDialog::OnBnClickedRemoveTuning()
//-------------------------------------------
{
	const size_t TCol = static_cast<size_t>(m_CombobTuningCollection.GetCurSel());
	const size_t T = static_cast<size_t>(m_CombobTuningName.GetCurSel());
	if(TCol >= m_TuningCollections.size() ||
		T >= m_TuningCollections[TCol]->GetNumTunings())
		return;
	string str = string("Remove tuning?");
	if(MessageBox(str.c_str(), 0, MB_YESNO) == IDYES)
	{
		if(m_TuningCollections[TCol]->Remove(T))
		{
			MessageBox("Tuning removal failed");
		}
		SetView();
	}
}

void CTuningDialog::OnCbnEditchangeComboT()
//-----------------------------------------
{
	if(IsDlgButtonChecked(IDC_CHECK_NEWTUNING))
	{
		if(m_pTempTuning == NULL) //Should never be true.
			return;
		const size_t BS = 40;
		char buffer[BS];
		m_CombobTuningName.GetWindowText(buffer, BS);
		m_pTempTuning->SetName(buffer);
	}
}

void CTuningDialog::OnEnChangeEditNotename()
//------------------------------------------
{
	if(!m_NoteEditApply)
	{
		m_NoteEditApply = true;
		return;
	}

	if(!m_pTempTuning)
		return;

	const CTuning::STEPTYPE currentNote = m_RatioMapWnd.GetShownCentre();
	const size_t BS = 5;
	char buffer[BS];
	m_EditNotename.GetWindowText(buffer, BS);
	string str = string(buffer);
	if(str.length() > 0)
	{
		if(str.size() > 3)
			str.resize(3);
		m_pTempTuning->SetNoteName(currentNote, str);
	}
	else
		m_pTempTuning->ClearNoteName(currentNote);

	m_RatioMapWnd.Invalidate();
}

void CTuningDialog::OnEnChangeEditRatiovalue()
//--------------------------------------------
{
	if(!m_RatioEditApply)
	{
		m_RatioEditApply = true;
		return;
	}

	if(!m_pTempTuning)
		return;

	const CTuning::STEPTYPE currentNote = m_RatioMapWnd.GetShownCentre();

	const size_t BS = 12;
	char buffer[BS];
	m_EditRatio.GetWindowText(buffer, BS);
	string str = buffer;
	if(str.length() > 0)
	{
		m_pTempTuning->SetRatio(currentNote, static_cast<CTuning::RATIOTYPE>(atof(buffer)));
		UpdateTuningType(m_pTempTuning);
		m_RatioMapWnd.Invalidate();
	}
}

void CTuningDialog::OnBnClickedButtonSetvalues()
//----------------------------------------------
{
	if(IsDlgButtonChecked(IDC_CHECK_NEWTUNING))
	{
		if(m_pTempTuning == NULL)
		{
			ASSERT(false);
			return;
		}

		const CTuning::STEPTYPE currentNote = m_RatioMapWnd.GetShownCentre();
		const size_t BS = 30;
		char buffer[BS];

		m_EditRatio.GetWindowText(buffer, BS);
		string str = buffer;
		if(str.length() > 0)
		{
			m_pTempTuning->SetRatio(currentNote, static_cast<CTuning::RATIOTYPE>(atof(buffer)));
			UpdateTuningType(m_pTempTuning);
			m_EditRatio.SetWindowText("");
		}

		m_EditNotename.GetWindowText(buffer, BS);
		str = string(buffer);
		if(str.length() > 0)
		{
			m_pTempTuning->SetNoteName(currentNote, str);
			m_EditNotename.SetWindowText("");
		}

		m_RatioMapWnd.Invalidate();
		UpdateRatioMapEdits(m_RatioMapWnd.GetShownCentre());
	}
}

void CTuningDialog::UpdateRatioMapEdits(const CTuning::STEPTYPE& note)
//---------------------------------------
{
	if(m_pTempTuning == NULL)
		return;

	m_RatioEditApply = false;
	m_EditRatio.SetWindowText(Stringify(m_pTempTuning->GetFrequencyRatio(note, 0)).c_str());
	m_NoteEditApply = false;
	m_EditNotename.SetWindowText(m_pTempTuning->GetNoteName(note).c_str());

	
}


