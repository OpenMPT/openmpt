#include "stdafx.h"
#include "mptrack.h"
#include "TuningDialog.h"
#include <algorithm>
#include "misc_util.h"

const string CTuningDialog::s_stringTypeGEN = "General";
const string CTuningDialog::s_stringTypeRP = "Ratio periodic";
const string CTuningDialog::s_stringTypeTET = "TET";

/*
TODOS:
-Clear tuning(when copying existing tuning, it might have
 e.g. unwanted note names which should able to be removed.
-Drag'n'drop tuningfiles
-Tooltips.
*/


// CTuningDialog dialog

IMPLEMENT_DYNAMIC(CTuningDialog, CDialog)
CTuningDialog::CTuningDialog(CWnd* pParent, const TUNINGVECTOR& rVec, CTuning* pTun)
	: CDialog(CTuningDialog::IDD, pParent),
	m_TuningCollections(rVec),
	m_TempTunings("Sandbox"),
	m_NoteEditApply(true),
	m_RatioEditApply(true)
//----------------------------------------
{
	m_pActiveTuning = pTun;
	m_RatioMapWnd.m_pTuning = pTun; //pTun is the tuning to show when dialog opens.

	m_TuningCollections.push_back(&m_TempTunings);
}

CTuningDialog::~CTuningDialog()
//----------------------------
{
	for(size_t i = 0; i<m_TuningCollections.size(); i++)
	{
		vector<CTuningCollection*>::iterator iter = find(m_DeletableTuningCollections.begin(), m_DeletableTuningCollections.end(), m_TuningCollections[i]);
		if(iter != m_DeletableTuningCollections.end())
		{
			delete m_TuningCollections[i];
			m_DeletableTuningCollections.erase(iter);
		}
	}
	m_TuningCollections.clear();
	m_DeletableTuningCollections.clear();
}

BOOL CTuningDialog::OnInitDialog()
//--------------------------------
{
	CDialog::OnInitDialog();

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

	m_EditFineTuneSteps.SetLimitText(3);

	UpdateView();

	return TRUE;
}

void CTuningDialog::UpdateView()
//------------------------------
{

	if(m_pActiveTuning == NULL && (m_TuningCollections.size() == 0 || m_TuningCollections[0]->GetNumTunings() == 0))
	{
		ASSERT(false);
		return;
	}

	if(m_pActiveTuning == NULL)
		m_pActiveTuning = &m_TuningCollections[0]->GetTuning(0);


	//Finding out where given tuning belongs to.
	size_t curTCol = 0, curT = 0; //cur <-> Current, T <-> Tuning, Col <-> Collection.
	bool found = false;
	for(size_t i = 0; i<m_TuningCollections.size(); i++)
	{
		CTuningCollection& rCurTCol = *m_TuningCollections.at(i);
		for(size_t j = 0; j<rCurTCol.GetNumTunings(); j++)
		{
			if(m_pActiveTuning == &rCurTCol.GetTuning(static_cast<unsigned short>(j)))
			{
				curTCol = i;
				curT = j;
				found = true;
				break;
			}
		}
	}
	ASSERT(found);

	m_CombobTuningCollection.SetCurSel(curTCol);
	OnCbnSelchangeComboTcol();
	m_CombobTuningCollection.Invalidate();

    m_CombobTuningName.SetCurSel(curT);
	m_CombobTuningName.Invalidate();


	UpdateTuningType();

	m_EditName.SetWindowText(m_pActiveTuning->GetName().c_str());
	m_EditName.Invalidate();

	//Finetunesteps-edit
	m_EditFineTuneSteps.SetWindowText(Stringify(m_pActiveTuning->GetFineStepCount()).c_str());
	m_EditFineTuneSteps.Invalidate();

	//.Making sure that ratiomap window is showing and
	//updating its content.
	m_RatioMapWnd.ShowWindow(SW_SHOW);
	m_RatioMapWnd.m_pTuning = m_pActiveTuning;
	m_RatioMapWnd.Invalidate();
	UpdateRatioMapEdits(m_RatioMapWnd.GetShownCentre());
	
	const CTuning::STEPTYPE period = m_pActiveTuning->GetPeriod();
	const CTuning::RATIOTYPE periodRatio = m_pActiveTuning->GetPeriodRatio();
	if(period > 0)
	{
		m_EditSteps.EnableWindow();
		m_EditSteps.SetWindowText(Stringify(period).c_str());

		m_EditRatioPeriod.EnableWindow();
		m_EditRatioPeriod.SetWindowText(Stringify(periodRatio).c_str());
	}
	else //case: m_pActiveTuning is of type general.
	{
		m_EditSteps.EnableWindow(false);
		m_EditRatioPeriod.EnableWindow(false);
	}

	m_EditRatioPeriod.Invalidate();
	m_EditSteps.Invalidate();

	bool enableControls = true;
	if(m_pActiveTuning->GetEditMask() == CTuning::EM_CONST ||
		m_pActiveTuning->GetEditMask() == CTuning::EM_CONST_STRICT)
	{
		CheckDlgButton(IDC_CHECK_READONLY, MF_CHECKED);
		if(m_pActiveTuning->GetEditMask() == CTuning::EM_CONST_STRICT)
			m_ButtonReadOnly.EnableWindow(FALSE);
		else
			m_ButtonReadOnly.EnableWindow(TRUE);

		enableControls = false;
	}
	else
	{
		CheckDlgButton(IDC_CHECK_READONLY, MF_UNCHECKED);
		m_ButtonReadOnly.EnableWindow();
	}

	m_CombobTuningType.EnableWindow(enableControls);
	m_EditTableSize.SetReadOnly(!enableControls);
	m_EditBeginNote.SetReadOnly(!enableControls);
	m_EditSteps.SetReadOnly(!enableControls);
	m_EditRatioPeriod.SetReadOnly(!enableControls);
	m_EditRatio.SetReadOnly(!enableControls);
	m_EditNotename.SetReadOnly(!enableControls);
	m_EditMiscActions.SetReadOnly(!enableControls);
	m_EditFineTuneSteps.SetReadOnly(!enableControls);
	m_EditName.SetReadOnly(!enableControls);
	
	m_ButtonSet.EnableWindow(enableControls);
	
	m_CombobTuningType.Invalidate();
	m_EditSteps.Invalidate();
	m_EditRatioPeriod.Invalidate();

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
	DDX_Control(pDX, IDC_BUTTON_EXPORT, m_ButtonExport);
	DDX_Control(pDX, IDC_BUTTON_IMPORT, m_ButtonImport);
	DDX_Control(pDX, IDC_EDIT_MISC_ACTIONS, m_EditMiscActions);
	DDX_Control(pDX, IDC_EDIT_FINETUNESTEPS, m_EditFineTuneSteps);
	DDX_Control(pDX, IDC_CHECK_READONLY, m_ButtonReadOnly);
	DDX_Control(pDX, IDC_EDIT_NAME, m_EditName);
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
	ON_BN_CLICKED(IDC_BUTTON_EXPORT, OnBnClickedButtonExport)
	ON_BN_CLICKED(IDC_BUTTON_IMPORT, OnBnClickedButtonImport)
	ON_EN_CHANGE(IDC_EDIT_FINETUNESTEPS, OnEnChangeEditFinetunesteps)
	ON_EN_KILLFOCUS(IDC_EDIT_FINETUNESTEPS, OnEnKillfocusEditFinetunesteps)
	ON_BN_CLICKED(IDC_CHECK_READONLY, OnBnClickedCheckReadonly)
	ON_EN_KILLFOCUS(IDC_EDIT_NAME, OnEnKillfocusEditName)
	ON_EN_KILLFOCUS(IDC_EDIT_STEPS, OnEnKillfocusEditSteps)
	ON_EN_KILLFOCUS(IDC_EDIT_RATIOPERIOD, OnEnKillfocusEditRatioperiod)
	ON_EN_KILLFOCUS(IDC_EDIT_RATIOVALUE, OnEnKillfocusEditRatiovalue)
	ON_EN_KILLFOCUS(IDC_EDIT_NOTENAME, OnEnKillfocusEditNotename)
END_MESSAGE_MAP()


// CTuningDialog message handlers

void CTuningDialog::OnCbnSelchangeComboTcol()
//-------------------------------------------
{
	const int curTCol = m_CombobTuningCollection.GetCurSel();
	if(curTCol < 0 || curTCol >= static_cast<int>(m_TuningCollections.size())) return;

	//Checking whether collection allows removing
	if(m_TuningCollections[curTCol]->CanEdit(CTuningCollection::EM_REMOVE))
		m_ButtonRemoveTuning.EnableWindow(true);
	else
		m_ButtonRemoveTuning.EnableWindow(false);

	//Clearing existing items from name-combobox...
	while(m_CombobTuningName.GetCount() > 0)
		m_CombobTuningName.DeleteString(0);

	//...adding names of tunings in the current tuning collection...
	CTuningCollection& rTCol = *m_TuningCollections.at(curTCol);
	for(size_t i = 0; i<rTCol.GetNumTunings(); i++)
	{
		m_CombobTuningName.AddString(rTCol.GetTuning(static_cast<unsigned short>(i)).GetName().c_str());
	}

	//Checking whether tuning collection allows adding
	//tunings...
	if(m_TuningCollections.at(curTCol)->CanEdit(CTuningCollection::EM_ADD))
		m_ButtonAddTuning.EnableWindow();
	else
		m_ButtonAddTuning.EnableWindow(false);

	if(m_TuningCollections.at(curTCol)->CanEdit(CTuningCollection::EM_REMOVE))
		m_ButtonRemoveTuning.EnableWindow();
	else
		m_ButtonRemoveTuning.EnableWindow(FALSE);
}

void CTuningDialog::OnCbnSelchangeComboT()
//----------------------------------------
{
	const int TCol = m_CombobTuningCollection.GetCurSel();
	if(TCol < 0 || TCol >= static_cast<int>(m_TuningCollections.size())) return;

	CTuningCollection& rTCol = *m_TuningCollections.at(TCol);

	//...checking that tuning index is valid...
	const unsigned short T = static_cast<unsigned short>(m_CombobTuningName.GetCurSel());
	if(T < 0 || T >= rTCol.GetNumTunings())
		return;

	m_pActiveTuning = &rTCol.GetTuning(T);
	m_RatioMapWnd.m_pTuning = m_pActiveTuning;

	UpdateView();
}

void CTuningDialog::UpdateTuningType()
//------------------------------------
{
	if(m_pActiveTuning)
	{
		ASSERT(m_CombobTuningType.GetCount() > 0);
		if(m_pActiveTuning->GetTuningType() == CTuning::TT_TET)
			m_CombobTuningType.SetCurSel(2);
		else
			if(m_pActiveTuning->GetTuningType() == CTuning::TT_RATIOPERIODIC)
				m_CombobTuningType.SetCurSel(1);
			else
				m_CombobTuningType.SetCurSel(0);
	}
}

void CTuningDialog::OnBnClickedCheckNewtuning()
//---------------------------------------------
{
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
	if(m_pActiveTuning == NULL)
		return;


	const CTuning::CTUNINGTYPE oldType = m_pActiveTuning->GetTuningType();
	const size_t BS = 20;
	char buffer[BS];
	m_CombobTuningType.GetWindowText(buffer, BS);
	const string strNewType = buffer;
	CTuning::CTUNINGTYPE newType = GetTuningTypeFromStr(strNewType);
	if(!m_pActiveTuning->DoesTypeInclude(newType))
	{
		if(MessageBox("This will change the ratio values; continue?", 0, MB_YESNO) == IDYES)
		{
			const size_t BS = 20;
			char buffer[BS];
			m_EditSteps.GetWindowText(buffer, BS);
			CTuning::STEPTYPE steps = static_cast<CTuning::STEPTYPE>(atoi(buffer));

			m_EditRatioPeriod.GetWindowText(buffer, BS);
			CTuning::RATIOTYPE pr = static_cast<CTuning::RATIOTYPE>(atof(buffer));

			if(steps <= 0)
					steps = 1;
				if(pr <= 0)
					pr = 1;

			if(newType == CTuning::TT_RATIOPERIODIC)
				m_pActiveTuning->CreateRatioPeriodic(steps, pr);
			else
				if(newType == CTuning::TT_TET)
					m_pActiveTuning->CreateTET(steps, pr);
			
			UpdateView();
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
}


void CTuningDialog::OnEnChangeEditSteps()
//---------------------------------------
{
}

void CTuningDialog::OnEnChangeEditRatioperiod()
//---------------------------------------------
{
}


void CTuningDialog::OnBnClickedAddTuning()
//-----------------------------------------
{
	if(m_pActiveTuning == NULL) //Should never be true.
		return;

	if(m_pActiveTuning->GetName().length() == 0)
	{
		MessageBox("Please give name for the tuning", 0, MB_OK);
		return;
	}

	if(m_CombobTuningCollection.GetCurSel() < 0 || m_CombobTuningCollection.GetCurSel() >= static_cast<int>(m_TuningCollections.size()))
	{
		MessageBox("No tuning collection chosen", 0, MB_OK); 
		return;
	}

	CTuningCollection& rTCol = *m_TuningCollections[m_CombobTuningCollection.GetCurSel()];

	string detailStr = string("Add tuning to '") + rTCol.GetName() + string("'(it will be create as a copy of current tuning)?");
	if(MessageBox(detailStr.c_str(), 0, MB_YESNO) == IDYES)
	{
		CTuning* pNewTuning = new CTuningRTI(m_pActiveTuning);
		if(rTCol.AddTuning(pNewTuning))
		{
			MessageBox("Add tuning failed");
			delete pNewTuning;
			return;
		}
		m_pActiveTuning = pNewTuning;
		UpdateView();
	}
}

void CTuningDialog::OnBnClickedRemoveTuning()
//-------------------------------------------
{
	UpdateView();
	//Now TCol and T should have the values pointing to m_pActiveTuning.
	const size_t TCol = static_cast<size_t>(m_CombobTuningCollection.GetCurSel());
	const size_t T = static_cast<size_t>(m_CombobTuningName.GetCurSel());
	if(TCol >= m_TuningCollections.size() ||
		T >= m_TuningCollections[TCol]->GetNumTunings())
		return;

	CTuningCollection& rTCol = *m_TuningCollections[TCol];
	const CTuning& rT = m_TuningCollections[TCol]->GetTuning(static_cast<WORD>(T));
	ASSERT(&rT == m_pActiveTuning);
	string str = string("Remove tuning '") + rT.GetName() + string("' from collection ") + rTCol.GetName() + string("?");
	if(MessageBox(str.c_str(), 0, MB_YESNO) == IDYES)
	{
		if(m_TuningCollections[TCol]->Remove(T))
		{
			MessageBox("Tuning removal failed");
			return;
		}
		if(rTCol.GetNumTunings() > 0)
			m_pActiveTuning = &rTCol.GetTuning(static_cast<WORD>(min(T, rTCol.GetNumTunings()-1)));
		else
			if(m_TuningCollections.size() > 0 && m_TuningCollections[0] && m_TuningCollections[0]->GetNumTunings() > 0)
				m_pActiveTuning = &m_TuningCollections[0]->GetTuning(0);
			else
				m_pActiveTuning = NULL;

		UpdateView();
	}
}

void CTuningDialog::OnCbnEditchangeComboT()
//-----------------------------------------
{
	if(m_pActiveTuning != NULL)
	{
		const size_t BS = 40;
		char buffer[BS];
		m_CombobTuningName.GetWindowText(buffer, BS);
		m_pActiveTuning->SetName(buffer);
		UpdateView();
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

	if(!m_pActiveTuning)
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
		m_pActiveTuning->SetNoteName(currentNote, str);
	}
	else
		m_pActiveTuning->ClearNoteName(currentNote);

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

	if(!m_pActiveTuning)
		return;

	const CTuning::STEPTYPE currentNote = m_RatioMapWnd.GetShownCentre();

	const size_t BS = 12;
	char buffer[BS];
	m_EditRatio.GetWindowText(buffer, BS);
	string str = buffer;
	if(str.length() > 0)
	{
		m_pActiveTuning->SetRatio(currentNote, static_cast<CTuning::RATIOTYPE>(atof(buffer)));
		UpdateTuningType();
		m_RatioMapWnd.Invalidate();
	}
	
}

void CTuningDialog::OnBnClickedButtonSetvalues()
//----------------------------------------------
{
	if(m_pActiveTuning)
	{
		if(m_EditMiscActions.GetWindowTextLength() < 1)
			return;

		const size_t BS = 20;
		char buffer[BS];
		m_EditMiscActions.GetWindowText(buffer, BS);
		m_pActiveTuning->Multiply(static_cast<CTuning::RATIOTYPE>(atof(buffer)));
		m_EditMiscActions.SetWindowText("");
		m_RatioMapWnd.Invalidate();
	}
}

void CTuningDialog::UpdateRatioMapEdits(const CTuning::STEPTYPE& note)
//-----------------------------------------------------------
{
	if(m_pActiveTuning == NULL)
		return;

	m_RatioEditApply = false;
	m_EditRatio.SetWindowText(Stringify(m_pActiveTuning->GetFrequencyRatio(note)).c_str());
	m_NoteEditApply = false;
	m_EditNotename.SetWindowText(m_pActiveTuning->GetNoteName(note).c_str());

	m_EditRatio.Invalidate();
	m_EditNotename.Invalidate();
}



void CTuningDialog::OnBnClickedButtonExport()
//-------------------------------------------
{	
	CTuningCollection* pTC = NULL; //Used if exporting tuning collection.

	const CTuning* pT = m_pActiveTuning;

	if(pT == NULL)
		return;

	size_t TCol = static_cast<size_t>(m_CombobTuningCollection.GetCurSel());
	if(TCol < m_TuningCollections.size())
	pTC = m_TuningCollections[TCol];
		
	string filter = string("Tuning files (*") + CTuning::s_FileExtension + string(")|*") + CTuning::s_FileExtension + string("|");
	if(pTC != NULL)
		filter += string("Tuning collection files (") + CTuningCollection::s_FileExtension + string(")|*") + CTuningCollection::s_FileExtension + string("|");

	CFileDialog dlg(FALSE, CTuning::s_FileExtension.c_str(),
			NULL,
			OFN_HIDEREADONLY| OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN,
			 filter.c_str(), this);

	bool failure = true;
	
	if (dlg.DoModal() != IDOK) return;
	BeginWaitCursor();
	ofstream fout(dlg.GetPathName(), ios::binary);
	const string ext = "." + dlg.GetFileExt();

	if(ext == CTuning::s_FileExtension)
	{
		if(pT != NULL)
			failure = pT->SerializeBinary(fout);
	}
	else //Case: Saving tuning collection.
		if(ext == CTuningCollection::s_FileExtension)
			if(pTC != NULL)
				failure = pTC->SerializeBinary(fout);

	fout.close();
	EndWaitCursor();

	
	if(failure)
		ErrorBox(IDS_ERR_EXPORT_TUNING, this);
}

void CTuningDialog::OnBnClickedButtonImport()
//-------------------------------------------
{
	//TODO: Ability to import ratios from text file.
	string filter = string("Tuning files (*") + CTuning::s_FileExtension + string(")|*") + CTuning::s_FileExtension + string("|") + 
	string("Tuning collection files (*") + CTuningCollection::s_FileExtension + string(")|*") + CTuningCollection::s_FileExtension + string("|");
	CFileDialog dlg(TRUE,
					NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
					filter.c_str(),
					this);

	if (dlg.DoModal() != IDOK) return;

	const string ext = string(".") + string(dlg.GetFileExt());

	bool failure = true;

	if(ext == CTuning::s_FileExtension)
	{
		ifstream fin(dlg.GetPathName(), ios::binary);
		CTuning* pT = new CTuningRTI;
		failure = pT->UnSerializeBinary(fin);
		fin.close();
		if(failure)
			delete pT;
		else
		{
			m_TempTunings.AddTuning(pT);
			m_pActiveTuning = pT;
			UpdateView();
		}
	}
	else
	{
		if(ext == CTuningCollection::s_FileExtension)
		{
			//For now only loading tuning collection as 
			//a separate collection - no possibility to 
			//directly replace some collection.
			CTuningCollection* pNewTCol = new CTuningCollection;
			ifstream fin(dlg.GetPathName(), ios::binary);
			failure = pNewTCol->UnSerializeBinary(fin);
			if(failure)
			{
				delete pNewTCol; pNewTCol = 0;
			}
			else
			{
				m_TuningCollections.push_back(pNewTCol);
				m_DeletableTuningCollections.push_back(pNewTCol);
				m_CombobTuningCollection.AddString(pNewTCol->GetName().c_str());
				m_CombobTuningCollection.SetCurSel(m_CombobTuningCollection.GetCount()-1);
				UpdateView();
			}


		}
	}
	

	if(failure)
		ErrorBox(IDS_ERR_FILEOPEN, this);
}

void CTuningDialog::OnEnChangeEditFinetunesteps()
//-----------------------------------------------
{
}


void CTuningDialog::OnEnKillfocusEditFinetunesteps()
//--------------------------------------------------
{
	if(m_pActiveTuning)
	{
		const BYTE BS = 5;
		char buffer[BS];
		m_EditFineTuneSteps.GetWindowText(buffer, BS);
		m_EditFineTuneSteps.SetWindowText(Stringify(m_pActiveTuning->SetFineStepCount(static_cast<CTuning::FINESTEPTYPE>(atoi(buffer)))).c_str());
		m_EditFineTuneSteps.Invalidate();
	}
}


void CTuningDialog::OnBnClickedCheckReadonly()
//--------------------------------------------
{
	if(m_pActiveTuning == NULL)
		return;

	if(IsDlgButtonChecked(IDC_CHECK_READONLY))
	{
		if(m_pActiveTuning->SetEditMask(CTuning::EM_CONST))
			CheckDlgButton(IDC_CHECK_READONLY, MF_UNCHECKED);
		else
			UpdateView();
	}
	else
	{
		if(m_pActiveTuning->SetEditMask(CTuning::EM_ALLOWALL))
			CheckDlgButton(IDC_CHECK_READONLY, MF_CHECKED);
		else
			UpdateView();
	}
}


void CTuningDialog::OnEnKillfocusEditName()
//-----------------------------------------
{
	if(m_pActiveTuning != NULL)
	{
		const size_t BS = 40;
		char buffer[BS];
		m_EditName.GetWindowText(buffer, BS);
		m_pActiveTuning->SetName(buffer);
		UpdateView();
	}
}


void CTuningDialog::OnEnKillfocusEditSteps()
//------------------------------------------
{
	if(m_pActiveTuning)
	{
		const size_t BS = 20;
		char buffer[BS];
		m_EditSteps.GetWindowText(buffer, BS);
		m_pActiveTuning->ChangePeriod(static_cast<CTuning::STEPTYPE>(atoi(buffer)));
		UpdateView();
	}	
}


void CTuningDialog::OnEnKillfocusEditRatioperiod()
//--------------------------------------------
{
	if(m_pActiveTuning)
	{
		const size_t BS = 20;
		char buffer[BS];
		m_EditRatioPeriod.GetWindowText(buffer, BS);
		m_pActiveTuning->ChangePeriodRatio(static_cast<CTuning::RATIOTYPE>(atof(buffer)));
		UpdateView();
	}
}

void CTuningDialog::OnEnKillfocusEditRatiovalue()
//-----------------------------------------------
{
	UpdateView();
}


void CTuningDialog::OnEnKillfocusEditNotename()
//-----------------------------------------------
{
	UpdateView();
}

