/*
 * TuningDialog.cpp
 * ----------------
 * Purpose: Alternative sample tuning configuration dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "TuningDialog.h"
#include "TrackerSettings.h"
#include <algorithm>
#include "../common/mptFileIO.h"
#include "../common/misc_util.h"
#include "TuningDialog.h"
#include "FileDialog.h"
#include "Mainfrm.h"


OPENMPT_NAMESPACE_BEGIN


const mpt::Charset TuningCharsetFallback = mpt::Charset::Locale;


const CTuningDialog::TUNINGTREEITEM CTuningDialog::s_notFoundItemTuning = TUNINGTREEITEM();
const HTREEITEM CTuningDialog::s_notFoundItemTree = NULL;

using UNOTEINDEXTYPE = Tuning::UNOTEINDEXTYPE;
using RATIOTYPE = Tuning::RATIOTYPE;
using NOTEINDEXTYPE = Tuning::NOTEINDEXTYPE;


// CTuningDialog dialog
CTuningDialog::CTuningDialog(CWnd* pParent, INSTRUMENTINDEX inst, CSoundFile &csf)
	: CDialog(CTuningDialog::IDD, pParent),
	m_sndFile(csf),
	m_pActiveTuningCollection(NULL),
	m_TreeCtrlTuning(this),
	m_TreeItemTuningItemMap(s_notFoundItemTree, s_notFoundItemTuning),
	m_NoteEditApply(true),
	m_RatioEditApply(true),
	m_DoErrorExit(false)
{
	m_TuningCollections.push_back(&(m_sndFile.GetTuneSpecificTunings()));
	m_TuningCollectionsNames[&(m_sndFile.GetTuneSpecificTunings())] = _T("Tunings");
	m_pActiveTuning = m_sndFile.Instruments[inst]->pTuning;
	m_RatioMapWnd.m_pTuning = m_pActiveTuning; //pTun is the tuning to show when dialog opens.
}

CTuningDialog::~CTuningDialog()
{
	for(auto &tuningCol : m_TuningCollections)
	{
		if(IsDeletable(tuningCol))
		{
			delete tuningCol;
			tuningCol = nullptr;
		}
	}
	m_TuningCollections.clear();
	m_DeletableTuningCollections.clear();
}

HTREEITEM CTuningDialog::AddTreeItem(CTuningCollection* pTC, HTREEITEM parent, HTREEITEM insertAfter)
{
	const HTREEITEM temp = m_TreeCtrlTuning.InsertItem((IsDeletable(pTC) ? CString(_T("loaded: ")) : CString()) +  m_TuningCollectionsNames[pTC], parent, insertAfter);
	HTREEITEM temp2 = NULL;
	m_TreeItemTuningItemMap.AddPair(temp, TUNINGTREEITEM(pTC));
	for(const auto &tuning : *pTC)
	{
		temp2 = AddTreeItem(tuning.get(), temp, temp2);
	}
	m_TreeCtrlTuning.EnsureVisible(temp);
	return temp;
}

HTREEITEM CTuningDialog::AddTreeItem(CTuning* pT, HTREEITEM parent, HTREEITEM insertAfter)
{
	const HTREEITEM temp = m_TreeCtrlTuning.InsertItem(mpt::ToCString(pT->GetName()), parent, insertAfter);
	m_TreeItemTuningItemMap.AddPair(temp, TUNINGTREEITEM(pT));
	m_TreeCtrlTuning.EnsureVisible(temp);
	return temp;
}

void CTuningDialog::DeleteTreeItem(CTuning* pT)
{
	if(!pT)
		return;

	HTREEITEM temp = m_TreeItemTuningItemMap.GetMapping_21(TUNINGTREEITEM(pT));
	if(temp)
	{
		HTREEITEM nextitem = m_TreeCtrlTuning.GetNextItem(temp, TVGN_NEXT);
		if(!nextitem) nextitem = m_TreeCtrlTuning.GetNextItem(temp, TVGN_PREVIOUS);
		m_pActiveTuning = m_TreeItemTuningItemMap.GetMapping_12(nextitem).GetT();
		m_TreeCtrlTuning.DeleteItem(temp);
		//Note: Item from map is deleted 'automatically' in
		//OnTvnDeleteitemTreeTuning.

	}
}

void CTuningDialog::DeleteTreeItem(CTuningCollection* pTC)
{
	if(!pTC)
		return;

	m_pActiveTuning = nullptr;
	const HTREEITEM temp = m_TreeItemTuningItemMap.GetMapping_21(TUNINGTREEITEM(pTC));
	if(temp)
	{
		TUNINGTREEITEM prevTTI = m_TreeItemTuningItemMap.GetMapping_12(m_TreeCtrlTuning.GetNextItem(temp, TVGN_PREVIOUS));
		TUNINGTREEITEM nextTTI = m_TreeItemTuningItemMap.GetMapping_12(m_TreeCtrlTuning.GetNextItem(temp, TVGN_NEXT));

		CTuningCollection* pTCprev = prevTTI.GetTC();
		CTuningCollection* pTCnext = nextTTI.GetTC();
		if(pTCnext == nullptr)
			pTCnext = GetpTuningCollection(nextTTI.GetT());
		if(pTCprev == nullptr)
			pTCprev = GetpTuningCollection(prevTTI.GetT());

		if(pTCnext != nullptr && pTCnext != m_pActiveTuningCollection)
			m_pActiveTuningCollection = pTCnext;
		else
		{
			if(pTCprev != m_pActiveTuningCollection)
				m_pActiveTuningCollection = pTCprev;
			else
				m_pActiveTuningCollection = NULL;
		}

		m_TreeCtrlTuning.DeleteItem(temp);
		//Note: Item from map is deleted 'automatically' in
		//OnTvnDeleteitemTreeTuning.
	}
	else
	{
		ASSERT(false);
		m_DoErrorExit = true;
		m_pActiveTuningCollection = NULL;
	}
}

BOOL CTuningDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_EditRatioPeriod.SubclassDlgItem(IDC_EDIT_RATIOPERIOD, this);
	m_EditRatio.SubclassDlgItem(IDC_EDIT_RATIOVALUE, this);
	m_EditRatioPeriod.AllowNegative(false);
	m_EditRatioPeriod.AllowFractions(true);
	m_EditRatio.AllowNegative(false);
	m_EditRatio.AllowFractions(true);

	m_RatioMapWnd.Init(this, 0);

	//-->Creating treeview
	m_TreeItemTuningItemMap.ClearMapping();
	for(const auto &tuningCol : m_TuningCollections)
	{
		AddTreeItem(tuningCol, NULL, NULL);
	}
	//<-- Creating treeview

	m_pActiveTuningCollection = GetpTuningCollection(m_pActiveTuning);

	//Adding tuning type names to corresponding combobox.
	m_CombobTuningType.SetItemData(m_CombobTuningType.AddString(_T("General")), static_cast<uint16>(Tuning::Type::GENERAL));
	m_CombobTuningType.SetItemData(m_CombobTuningType.AddString(_T("GroupGeometric")), static_cast<uint16>(Tuning::Type::GROUPGEOMETRIC));
	m_CombobTuningType.SetItemData(m_CombobTuningType.AddString(_T("Geometric")), static_cast<uint16>(Tuning::Type::GEOMETRIC));
	m_CombobTuningType.EnableWindow(FALSE);

	m_ButtonSet.EnableWindow(FALSE);

	m_EditSteps.SetLimitText(2);
	m_EditFineTuneSteps.SetLimitText(3);

	if(m_pActiveTuning) m_RatioMapWnd.m_nNote =  m_RatioMapWnd.m_nNoteCentre + m_pActiveTuning->GetNoteRange().first + (m_pActiveTuning->GetNoteRange().last - m_pActiveTuning->GetNoteRange().first)/2 + 1;

	UpdateView();

	return TRUE;
}


bool CTuningDialog::CanEdit(CTuning * pT, CTuningCollection * pTC) const
{
	if(!pT)
	{
		return false;
	}
	if(!pTC)
	{
		return false;
	}
	if(pTC != m_TuningCollections[0])
	{
		return false;
	}
	return true;
}


bool CTuningDialog::CanEdit(CTuningCollection * pTC) const
{
	if(!pTC)
	{
		return false;
	}
	if(pTC != m_TuningCollections[0])
	{
		return false;
	}
	return true;
}


void CTuningDialog::UpdateView(const int updateMask)
{
	if(m_DoErrorExit)
	{
		DoErrorExit();
		return;
	}

	//-->Updating treeview
	if(updateMask != UM_TUNINGDATA)
	{
		TUNINGTREEITEM tuningitem;
		if(m_pActiveTuning)
			tuningitem.Set(m_pActiveTuning);
		else
		{
			if(m_pActiveTuningCollection)
				tuningitem.Set(m_pActiveTuningCollection);
		}
		HTREEITEM treeitem = m_TreeItemTuningItemMap.GetMapping_21(tuningitem);
		if(treeitem)
		{
			m_TreeCtrlTuning.Select(treeitem, TVGN_CARET);
			if(m_pActiveTuning)
				m_TreeCtrlTuning.SetItemText(treeitem, mpt::ToCString(m_pActiveTuning->GetName()));
			else
				m_TreeCtrlTuning.SetItemText(treeitem, (IsDeletable(m_pActiveTuningCollection) ? CString(_T("loaded: ")) : CString()) + m_TuningCollectionsNames[m_pActiveTuningCollection]);
		}
	}
	//<--Updating treeview


	if(m_pActiveTuningCollection == NULL)
	{
		return;
	}

	m_ButtonNew.EnableWindow(TRUE);
	m_ButtonImport.EnableWindow(TRUE);
	m_ButtonExport.EnableWindow((m_pActiveTuning || m_pActiveTuningCollection) ? TRUE : FALSE);
	m_ButtonRemove.EnableWindow(((m_pActiveTuning && (m_pActiveTuningCollection == m_TuningCollections[0])) || (!m_pActiveTuning && m_pActiveTuningCollection && m_pActiveTuningCollection != m_TuningCollections[0])) ? TRUE : FALSE);

	//Updating tuning part-->
	if(m_pActiveTuning != NULL && (updateMask & UM_TUNINGDATA || updateMask == 0))
	{
		UpdateTuningType();

		m_EditName.SetWindowText(mpt::ToCString(m_pActiveTuning->GetName()));
		m_EditName.Invalidate();

		//Finetunesteps-edit
		m_EditFineTuneSteps.SetWindowText(mpt::cfmt::val(m_pActiveTuning->GetFineStepCount()));
		m_EditFineTuneSteps.Invalidate();

		//Making sure that ratiomap window is showing and
		//updating its content.
		m_RatioMapWnd.ShowWindow(SW_SHOW);
		m_RatioMapWnd.m_pTuning = m_pActiveTuning;
		m_RatioMapWnd.Invalidate();
		UpdateRatioMapEdits(m_RatioMapWnd.GetShownCentre());


		const UNOTEINDEXTYPE period = m_pActiveTuning->GetGroupSize();
		const RATIOTYPE GroupRatio = m_pActiveTuning->GetGroupRatio();
		if(m_pActiveTuning->GetType() == Tuning::Type::GROUPGEOMETRIC || m_pActiveTuning->GetType() == Tuning::Type::GEOMETRIC)
		{
			m_EditSteps.EnableWindow(TRUE);
			m_EditRatioPeriod.EnableWindow(TRUE);
			m_EditSteps.SetWindowText(mpt::cfmt::val(period));
			m_EditRatioPeriod.SetWindowText(mpt::cfmt::flt(GroupRatio, 6));
		} else
		{
			m_EditSteps.EnableWindow(FALSE);
			m_EditRatioPeriod.EnableWindow(FALSE);
			m_EditSteps.SetWindowText(_T(""));
			m_EditRatioPeriod.SetWindowText(_T(""));
		}

		m_EditRatioPeriod.Invalidate();
		m_EditSteps.Invalidate();

		bool enableControls = CanEdit(m_pActiveTuning, m_pActiveTuningCollection);

		m_CombobTuningType.EnableWindow(FALSE);
		m_EditSteps.SetReadOnly(!enableControls);
		m_EditRatioPeriod.SetReadOnly(!enableControls);
		m_EditRatio.SetReadOnly((m_pActiveTuning->GetType() == Tuning::Type::GEOMETRIC) ? TRUE : !enableControls);
		m_EditNotename.SetReadOnly(!enableControls);
		m_EditMiscActions.SetReadOnly((m_pActiveTuning->GetType() == Tuning::Type::GEOMETRIC) ? TRUE : !enableControls);
		m_EditFineTuneSteps.SetReadOnly(!enableControls);
		m_EditName.SetReadOnly(!enableControls);

		m_ButtonSet.EnableWindow((m_pActiveTuning->GetType() == Tuning::Type::GEOMETRIC) ? FALSE : enableControls);

		m_CombobTuningType.Invalidate();
		m_EditSteps.Invalidate();
		m_EditRatioPeriod.Invalidate();
	}
	else
	{
		if(m_pActiveTuning == NULL) //No active tuning, clearing tuning part.
		{
			m_EditName.SetWindowText(_T(""));
			m_EditSteps.SetWindowText(_T(""));
			m_EditRatioPeriod.SetWindowText(_T(""));
			m_EditRatio.SetWindowText(_T(""));
			m_EditNotename.SetWindowText(_T(""));
			m_EditMiscActions.SetWindowText(_T(""));
			m_EditFineTuneSteps.SetWindowText(_T(""));
			m_EditName.SetWindowText(_T(""));

			m_CombobTuningType.SetCurSel(-1);

			m_RatioMapWnd.ShowWindow(SW_HIDE);
			m_RatioMapWnd.m_pTuning = NULL;
			m_RatioMapWnd.Invalidate();
		}
	}
	//<--Updating tuning part
}


void CTuningDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATICRATIOMAP, m_RatioMapWnd);
	DDX_Control(pDX, IDC_COMBO_TTYPE, m_CombobTuningType);
	DDX_Control(pDX, IDC_EDIT_STEPS, m_EditSteps);
	DDX_Control(pDX, IDC_EDIT_NOTENAME, m_EditNotename);
	DDX_Control(pDX, IDC_BUTTON_SETVALUES, m_ButtonSet);
	DDX_Control(pDX, IDC_BUTTON_TUNING_NEW, m_ButtonNew);
	DDX_Control(pDX, IDC_BUTTON_IMPORT, m_ButtonImport);
	DDX_Control(pDX, IDC_BUTTON_EXPORT, m_ButtonExport);
	DDX_Control(pDX, IDC_BUTTON_TUNING_REMOVE, m_ButtonRemove);
	DDX_Control(pDX, IDC_EDIT_MISC_ACTIONS, m_EditMiscActions);
	DDX_Control(pDX, IDC_EDIT_FINETUNESTEPS, m_EditFineTuneSteps);
	DDX_Control(pDX, IDC_EDIT_NAME, m_EditName);
	DDX_Control(pDX, IDC_TREE_TUNING, m_TreeCtrlTuning);
}



BEGIN_MESSAGE_MAP(CTuningDialog, CDialog)
	ON_EN_CHANGE(IDC_EDIT_STEPS, &CTuningDialog::OnEnChangeEditSteps)
	ON_EN_CHANGE(IDC_EDIT_RATIOPERIOD, &CTuningDialog::OnEnChangeEditRatioperiod)
	ON_EN_CHANGE(IDC_EDIT_NOTENAME, &CTuningDialog::OnEnChangeEditNotename)
	ON_BN_CLICKED(IDC_BUTTON_SETVALUES, &CTuningDialog::OnBnClickedButtonSetvalues)
	ON_EN_CHANGE(IDC_EDIT_RATIOVALUE, &CTuningDialog::OnEnChangeEditRatiovalue)
	ON_BN_CLICKED(IDC_BUTTON_TUNING_NEW, &CTuningDialog::OnBnClickedButtonNew)
	ON_BN_CLICKED(IDC_BUTTON_IMPORT, &CTuningDialog::OnBnClickedButtonImport)
	ON_BN_CLICKED(IDC_BUTTON_EXPORT, &CTuningDialog::OnBnClickedButtonExport)
	ON_BN_CLICKED(IDC_BUTTON_TUNING_REMOVE, &CTuningDialog::OnBnClickedButtonRemove)
	ON_EN_CHANGE(IDC_EDIT_FINETUNESTEPS, &CTuningDialog::OnEnChangeEditFinetunesteps)
	ON_EN_KILLFOCUS(IDC_EDIT_FINETUNESTEPS, &CTuningDialog::OnEnKillfocusEditFinetunesteps)
	ON_EN_KILLFOCUS(IDC_EDIT_NAME, &CTuningDialog::OnEnKillfocusEditName)
	ON_EN_KILLFOCUS(IDC_EDIT_STEPS, &CTuningDialog::OnEnKillfocusEditSteps)
	ON_EN_KILLFOCUS(IDC_EDIT_RATIOPERIOD, &CTuningDialog::OnEnKillfocusEditRatioperiod)
	ON_EN_KILLFOCUS(IDC_EDIT_RATIOVALUE, &CTuningDialog::OnEnKillfocusEditRatiovalue)
	ON_EN_KILLFOCUS(IDC_EDIT_NOTENAME, &CTuningDialog::OnEnKillfocusEditNotename)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_TUNING, &CTuningDialog::OnTvnSelchangedTreeTuning)
	ON_NOTIFY(TVN_DELETEITEM, IDC_TREE_TUNING, &CTuningDialog::OnTvnDeleteitemTreeTuning)
	ON_NOTIFY(NM_RCLICK, IDC_TREE_TUNING, &CTuningDialog::OnNMRclickTreeTuning)
	ON_NOTIFY(TVN_BEGINDRAG, IDC_TREE_TUNING, &CTuningDialog::OnTvnBegindragTreeTuning)
	ON_COMMAND(ID_REMOVETUNING, &CTuningDialog::OnRemoveTuning)
	ON_COMMAND(ID_ADDTUNINGGENERAL, &CTuningDialog::OnAddTuningGeneral)
	ON_COMMAND(ID_ADDTUNINGGROUPGEOMETRIC, &CTuningDialog::OnAddTuningGroupGeometric)
	ON_COMMAND(ID_ADDTUNINGGEOMETRIC, &CTuningDialog::OnAddTuningGeometric)
	ON_COMMAND(ID_COPYTUNING, &CTuningDialog::OnCopyTuning)
	ON_COMMAND(ID_REMOVETUNINGCOLLECTION, &CTuningDialog::OnRemoveTuningCollection)
END_MESSAGE_MAP()


void CTuningDialog::DoErrorExit()
{
	m_DoErrorExit = false;
	m_pActiveTuning = NULL;
	m_pActiveTuningCollection = NULL;
	Reporting::Message(LogInformation, _T("Dialog encountered an error and needs to close"), this);
	OnOK();
}


// CTuningDialog message handlers

void CTuningDialog::UpdateTuningType()
{
	if(m_pActiveTuning)
	{
		if(m_CombobTuningType.GetCount() < 3) m_DoErrorExit = true;

		if(m_pActiveTuning->GetType() == Tuning::Type::GEOMETRIC)
			m_CombobTuningType.SetCurSel(2);
		else
			if(m_pActiveTuning->GetType() == Tuning::Type::GROUPGEOMETRIC)
				m_CombobTuningType.SetCurSel(1);
			else
				m_CombobTuningType.SetCurSel(0);
	}
}



bool CTuningDialog::AddTuning(CTuningCollection* pTC, Tuning::Type type)
{
	if(!pTC)
	{
		Reporting::Notification("No tuning collection chosen");
		return false;
	}

	std::unique_ptr<CTuning> pNewTuning;
	if(type == Tuning::Type::GROUPGEOMETRIC)
	{
		std::vector<Tuning::RATIOTYPE> ratios;
		for(Tuning::NOTEINDEXTYPE n = 0; n < 12; ++n)
		{
			ratios.push_back(std::pow(static_cast<Tuning::RATIOTYPE>(2.0), static_cast<Tuning::RATIOTYPE>(n) / static_cast<Tuning::RATIOTYPE>(12)));
		}
		pNewTuning = CTuning::CreateGroupGeometric(U_("Unnamed"), ratios, 2, 15);
	} else if(type == Tuning::Type::GEOMETRIC)
	{
		pNewTuning = CTuning::CreateGeometric(U_("Unnamed"), 12, 2, 15);
	} else
	{
		pNewTuning = CTuning::CreateGeneral(U_("Unnamed"));
	}

	CTuning *pT = pTC->AddTuning(std::move(pNewTuning));
	if(!pT)
	{
		Reporting::Notification("Add tuning failed");
		return false;
	}
	AddTreeItem(pT, m_TreeItemTuningItemMap.GetMapping_21(TUNINGTREEITEM(pTC)), NULL);
	m_pActiveTuning = pT;
	m_ModifiedTCs[pTC] = true;
	UpdateView();

	return true;
}


void CTuningDialog::OnEnChangeEditSteps()
{
}

void CTuningDialog::OnEnChangeEditRatioperiod()
{
}


void CTuningDialog::OnEnChangeEditNotename()
{

	if(!m_NoteEditApply)
	{
		m_NoteEditApply = true;
		return;
	}

	if(!m_pActiveTuning)
		return;

	const NOTEINDEXTYPE currentNote = m_RatioMapWnd.GetShownCentre();
	CString buffer;
	m_EditNotename.GetWindowText(buffer);
	mpt::ustring str = mpt::ToUnicode(buffer);
	{
		if(str.size() > 3)
			str.resize(3);
		m_pActiveTuning->SetNoteName(currentNote, str);
	}

	m_ModifiedTCs[GetpTuningCollection(m_pActiveTuning)] = true;
	m_RatioMapWnd.Invalidate();

}

void CTuningDialog::OnEnChangeEditRatiovalue()
{

	if(!m_RatioEditApply)
	{
		m_RatioEditApply = true;
		return;
	}

	if(!m_pActiveTuning)
		return;

	const NOTEINDEXTYPE currentNote = m_RatioMapWnd.GetShownCentre();

	double ratio = 0.0;
	if(m_EditRatio.GetDecimalValue(ratio))
	{
		m_pActiveTuning->SetRatio(currentNote, static_cast<RATIOTYPE>(ratio));
		m_ModifiedTCs[GetpTuningCollection(m_pActiveTuning)] = true;
		UpdateTuningType();
		m_RatioMapWnd.Invalidate();
	}

}

void CTuningDialog::OnBnClickedButtonSetvalues()
{
	if(m_pActiveTuning)
	{
		if(m_EditMiscActions.GetWindowTextLength() < 1)
			return;

		CString buffer;
		m_EditMiscActions.GetWindowText(buffer);
		m_pActiveTuning->Multiply(ConvertStrTo<RATIOTYPE>(buffer));
		m_ModifiedTCs[GetpTuningCollection(m_pActiveTuning)] = true;
		m_EditMiscActions.SetWindowText(_T(""));
		m_RatioMapWnd.Invalidate();
	}
}

void CTuningDialog::UpdateRatioMapEdits(const NOTEINDEXTYPE& note)
{
	if(m_pActiveTuning == NULL)
		return;

	m_RatioEditApply = false;
	m_EditRatio.SetWindowText(mpt::cfmt::val(m_pActiveTuning->GetRatio(note)));
	m_NoteEditApply = false;
	m_EditNotename.SetWindowText(mpt::ToCString(m_pActiveTuning->GetNoteName(note, false)));

	m_EditRatio.Invalidate();
	m_EditNotename.Invalidate();
}


void CTuningDialog::OnBnClickedButtonNew()
{
	POINT point;
	GetCursorPos(&point);

	HMENU popUpMenu = CreatePopupMenu();
	if(popUpMenu == NULL) return;

	AppendMenu(popUpMenu, MF_STRING, ID_ADDTUNINGGROUPGEOMETRIC, _T("Add &GroupGeometric tuning"));
	AppendMenu(popUpMenu, MF_STRING, ID_ADDTUNINGGEOMETRIC, _T("Add G&eometric tuning"));
	AppendMenu(popUpMenu, MF_STRING, ID_ADDTUNINGGENERAL, _T("Add Ge&neral tuning"));

	m_CommandItemDest.Set(m_TuningCollections[0]);

	TrackPopupMenu(popUpMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, point.x, point.y, 0, m_hWnd, NULL);
	DestroyMenu(popUpMenu);
}


void CTuningDialog::OnBnClickedButtonExport()
{

	if(m_pActiveTuning == NULL && m_pActiveTuningCollection == NULL)
	{
		Reporting::Message(LogInformation, _T("Operation failed - No tuning file selected."), this);
		return;
	}

	bool failure = true;

	if(m_pActiveTuning)
	{

		const CTuning* pT = m_pActiveTuning;

		std::string filter;
		int filters = 0;
		int tuningFilter = -1;
		int sclFilter = -1;
		{
			filters++;
			filter += std::string("Tuning files (*") + CTuning::s_FileExtension + std::string(")|*") + CTuning::s_FileExtension + std::string("|");
			tuningFilter = filters;
		}
		{
			filters++;
			filter += std::string("Scala scale (*.scl)|*") + std::string(".scl")+ std::string("|");
			sclFilter = filters;
		}

		int filterIndex = 0;
		FileDialog dlg = SaveFileDialog()
			.DefaultExtension(CTuning::s_FileExtension)
			.ExtensionFilter(filter)
			.WorkingDirectory(TrackerSettings::Instance().PathTunings.GetWorkingDir())
			.FilterIndex(&filterIndex);

		if (!dlg.Show(this)) return;

		BeginWaitCursor();
		try
		{
			mpt::SafeOutputFile sfout(dlg.GetFirstFile(), std::ios::binary, mpt::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
			mpt::ofstream &fout = sfout;
			fout.exceptions(fout.exceptions() | std::ios::badbit | std::ios::failbit);

			if(tuningFilter != -1 && filterIndex == tuningFilter)
			{
				failure = (pT->Serialize(fout) != Tuning::SerializationResult::Success);
			} else if(sclFilter != -1 && filterIndex == sclFilter)
			{
				failure = !pT->WriteSCL(fout, dlg.GetFirstFile());
				if(!failure)
				{
					if(m_pActiveTuning->GetType() == Tuning::Type::GENERAL)
					{
						Reporting::Message(LogWarning, _T("The Scala SCL file format does not contain enough information to represent General Tunings without data loss.\n\nOpenMPT exported as much information as possible, but other software as well as OpenMPT itself will not be able to re-import the just exported Scala SCL in a way that resembles the original data completely.\n\nPlease consider additionally exporting the Tuning as an OpenMPT .tun file."), _T("Tuning - Incompatible export"), this);
					}
				}
			}
		} catch(const std::exception &)
		{
			failure = true;
		}
		EndWaitCursor();

	} else
	{

		const CTuningCollection* pTC = m_pActiveTuningCollection;

		std::string filter = std::string("Multiple Tuning files (") + CTuning::s_FileExtension + std::string(")|*") + CTuning::s_FileExtension + std::string("|");

		mpt::PathString fileName;
		if(!m_TuningCollectionsFilenames[pTC].empty())
		{
			fileName = m_TuningCollectionsFilenames[pTC] + P_(" - ");
		}
		if(!m_TuningCollectionsNames[pTC].IsEmpty())
		{
			mpt::PathString name = mpt::PathString::FromUnicode(mpt::ToUnicode(m_TuningCollectionsNames[pTC]));
			SanitizeFilename(name);
			fileName += name + P_(" - ");
		}
		fileName += P_("%tuning_number% - %tuning_name%");

		int filterIndex = 0;
		FileDialog dlg = SaveFileDialog()
			.DefaultExtension(CTuning::s_FileExtension)
			.ExtensionFilter(filter)
			.WorkingDirectory(TrackerSettings::Instance().PathTunings.GetWorkingDir())
			.FilterIndex(&filterIndex);
		dlg.DefaultFilename(fileName);

		if (!dlg.Show(this)) return;

		BeginWaitCursor();

		failure = false;

		auto numberFmt = mpt::FormatSpec().Dec().FillNul().Width(1 + static_cast<int>(std::log10(pTC->GetNumTunings())));

		for(std::size_t i = 0; i < pTC->GetNumTunings(); ++i)
		{
			const CTuning & tuning = pTC->GetTuning(i);
			fileName = dlg.GetFirstFile();
			mpt::ustring tuningName = mpt::ToUnicode(tuning.GetName());
			if(tuningName.empty())
			{
				tuningName = U_("untitled");
			}
			mpt::ustring fileNameW = fileName.ToUnicode();
			mpt::ustring numberW = mpt::ufmt::fmt(i + 1, numberFmt);
			SanitizeFilename(numberW);
			fileNameW = mpt::String::Replace(fileNameW, U_("%tuning_number%"), numberW);
			mpt::ustring nameW = mpt::ToUnicode(tuningName);
			SanitizeFilename(nameW);
			fileNameW = mpt::String::Replace(fileNameW, U_("%tuning_name%"), nameW);
			fileName = mpt::PathString::FromUnicode(fileNameW);

			try
			{
				mpt::SafeOutputFile sfout(fileName, std::ios::binary, mpt::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
				mpt::ofstream &fout = sfout;
				fout.exceptions(fout.exceptions() | std::ios::badbit | std::ios::failbit);
				if(tuning.Serialize(fout) != Tuning::SerializationResult::Success)
				{
					failure = true;
				}
			} catch(const std::exception &)
			{
				failure = true;
			}
		}

		EndWaitCursor();

	}

	if(failure)
	{
		Reporting::Message(LogError, _T("Export failed"), _T("Error!"), this);
	}

}


void CTuningDialog::OnBnClickedButtonRemove()
{
	if(m_pActiveTuning)
	{
		if(CanEdit(m_pActiveTuning, m_pActiveTuningCollection))
		{
			m_CommandItemDest.Set(m_pActiveTuning);
			OnRemoveTuning();
		}
	} else if(m_pActiveTuningCollection)
	{
		if(IsDeletable(m_pActiveTuningCollection))
		{
			m_CommandItemDest.Set(m_pActiveTuningCollection);
			OnRemoveTuningCollection();
		}
	}
}


template <typename Tfile, std::size_t N> static bool CheckMagic(Tfile &f, mpt::IO::Offset offset, const uint8(&magic)[N])
{
	if(!mpt::IO::SeekAbsolute(f, offset))
	{
		return false;
	}
	uint8 buffer[N];
	MemsetZero(buffer);
	if(mpt::IO::ReadRaw(f, buffer, N).size() != N)
	{
		return false;
	}
	bool result = (std::memcmp(magic, buffer, N) == 0);
	mpt::IO::SeekBegin(f);
	return result;
}


void CTuningDialog::OnBnClickedButtonImport()
{
	std::string sFilter = MPT_FORMAT("Tuning files (*{}, *{}, *.scl)|*{};*{};*.scl|")(
		CTuning::s_FileExtension,
		CTuningCollection::s_FileExtension,
		CTuning::s_FileExtension,
		CTuningCollection::s_FileExtension);

	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.ExtensionFilter(sFilter)
		.WorkingDirectory(TrackerSettings::Instance().PathTunings.GetWorkingDir());
	if(!dlg.Show(this))
		return;

	TrackerSettings::Instance().PathTunings.SetWorkingDir(dlg.GetWorkingDirectory());

	mpt::ustring sLoadReport;

	const auto &files = dlg.GetFilenames();
	for(const auto &file : files)
	{
		mpt::PathString fileName;
		mpt::PathString fileExt;
		file.SplitPath(nullptr, nullptr, &fileName, &fileExt);
		const mpt::ustring fileNameExt = (fileName + fileExt).ToUnicode();

		const bool bIsTun = (mpt::PathString::CompareNoCase(fileExt, mpt::PathString::FromUTF8(CTuning::s_FileExtension)) == 0);
		const bool bIsScl = (mpt::PathString::CompareNoCase(fileExt, P_(".scl")) == 0);
		const bool bIsTc = (mpt::PathString::CompareNoCase(fileExt, mpt::PathString::FromUTF8(CTuningCollection::s_FileExtension)) == 0);

		mpt::ifstream fin(file, std::ios::binary);

		// "HSCT", 0x01, 0x00, 0x00, 0x00
		const uint8 magicTColdV1 [] = {  'H', 'S', 'C', 'T',0x01,0x00,0x00,0x00                          };
		// "HSCT", 0x02, 0x00, 0x00, 0x00
		const uint8 magicTColdV2 [] = {  'H', 'S', 'C', 'T',0x02,0x00,0x00,0x00                          };
		// "CTRTI_B.", 0x03, 0x00
		const uint8 magicTUNoldV2[] = {  'C', 'T', 'R', 'T', 'I', '_', 'B', '.',0x02,0x00                };
		// "CTRTI_B.", 0x03, 0x00
		const uint8 magicTUNoldV3[] = {  'C', 'T', 'R', 'T', 'I', '_', 'B', '.',0x03,0x00                };
		// "228", 0x02, "TC"
		const uint8 magicTC      [] = {  '2', '2', '8',0x02, 'T', 'C'                                    };
		// "228", 0x09, "CTB244RTI"
		const uint8 magicTUN     [] = {  '2', '2', '8',0x09, 'C', 'T', 'B', '2', '4', '4', 'R', 'T', 'I' };

		CTuningCollection *pTC = nullptr;
		CString tcName;
		mpt::PathString tcFilename;
		std::unique_ptr<CTuning> pT;

		if(bIsTun && CheckMagic(fin, 0, magicTC))
		{
			// OpenMPT since r3115 wrongly wrote .tc files instead of .tun files when exporting.
			// If such a file is detected and only contains a single Tuning, we can work-around that.
			// For .tc files containing multiple Tunings, we sadly cannot decide which one the user wanted.
			// In that case, we import as a Collection (an alternative might be to display a dialog in this case).
			pTC = new CTuningCollection();
			mpt::ustring name;
			if(pTC->Deserialize(fin, name, TuningCharsetFallback) == Tuning::SerializationResult::Success)
			{ // success
				if(pTC->GetNumTunings() == 1)
				{
					Reporting::Message(LogInformation, U_("- Tuning Collection with a Tuning file extension (.tun) detected. It only contains a single Tuning, importing the file as a Tuning.\n"), this);
					pT = std::unique_ptr<CTuning>(new CTuning(pTC->GetTuning(0)));
					delete pTC;
					pTC = nullptr;
					// ok
				} else
				{
					Reporting::Message(LogNotification, U_("- Tuning Collection with a Tuning file extension (.tun) detected. It only contains multiple Tunings, importing the file as a Tuning Collection.\n"), this);
					// ok
				}
			} else
			{
				delete pTC;
				pTC = nullptr;
				// fail
			}

		} else if(CheckMagic(fin, 0, magicTC) || CheckMagic(fin, 0, magicTColdV2) || CheckMagic(fin, 0, magicTColdV1))
		{

			pTC = new CTuningCollection();
			mpt::ustring name;
			if(pTC->Deserialize(fin, name, TuningCharsetFallback) != Tuning::SerializationResult::Success)
			{ // failure
				delete pTC;
				pTC = nullptr;
				// fail
			} else
			{
				tcName = mpt::ToCString(name);
				tcFilename = file;
				// ok
			}

		} else if(CheckMagic(fin, 0, magicTUNoldV3) || CheckMagic(fin, 0, magicTUNoldV2))
		{

			pT = CTuning::CreateDeserializeOLD(fin, TuningCharsetFallback);

		} else if(CheckMagic(fin, 0, magicTUN))
		{

			pT = CTuning::CreateDeserialize(fin, TuningCharsetFallback);

		} else if(bIsScl)
		{

			EnSclImport a = ImportScl(file, fileName.ToUnicode(), pT);
			if(a != enSclImportOk)
			{ // failure
				pT = nullptr;
			}

		}

		bool success = false;

		if(pT)
		{
			CTuningCollection &tc = *m_TuningCollections.front();
			CTuning *activeTuning = tc.AddTuning(std::move(pT));
			if(!activeTuning)
			{
				if(tc.GetNumTunings() >= CTuningCollection::s_nMaxTuningCount)
				{
					sLoadReport += MPT_UFORMAT("- Failed to load file \"{}\": maximum number({}) of temporary tunings is already open.\n")(fileNameExt, static_cast<std::size_t>(CTuningCollection::s_nMaxTuningCount));
				} else 
				{
					sLoadReport += MPT_UFORMAT("- Unable to import file \"{}\": unknown reason.\n")(fileNameExt);
				}
			} else
			{
				m_pActiveTuning = activeTuning;
				AddTreeItem(m_pActiveTuning, m_TreeItemTuningItemMap.GetMapping_21(TUNINGTREEITEM(&tc)), NULL);
				success = true;
			}
		}

		if(pTC)
		{
			m_TuningCollections.push_back(pTC);
			m_TuningCollectionsNames[pTC] = tcName;
			m_TuningCollectionsFilenames[pTC] = tcFilename;
			m_DeletableTuningCollections.push_back(pTC);
			AddTreeItem(pTC, NULL, NULL);
			success = true;
		}

		if(!success)
		{
			sLoadReport += MPT_UFORMAT("- Unable to load \"{}\": unrecognized file format.\n")(fileNameExt);
		}
	}

	if(sLoadReport.length() > 0)
		Reporting::Information(sLoadReport);
	UpdateView();
}


void CTuningDialog::OnEnChangeEditFinetunesteps()
{
}


void CTuningDialog::OnEnKillfocusEditFinetunesteps()
{
	if(m_pActiveTuning)
	{
		CString buffer;
		m_EditFineTuneSteps.GetWindowText(buffer);
		m_pActiveTuning->SetFineStepCount(ConvertStrTo<Tuning::USTEPINDEXTYPE>(buffer));
		m_EditFineTuneSteps.SetWindowText(mpt::cfmt::val(m_pActiveTuning->GetFineStepCount()));
		m_ModifiedTCs[GetpTuningCollection(m_pActiveTuning)] = true;
		m_EditFineTuneSteps.Invalidate();
	}
}


void CTuningDialog::OnEnKillfocusEditName()
{
	if(m_pActiveTuning != NULL)
	{
		CString buffer;
		m_EditName.GetWindowText(buffer);
		m_pActiveTuning->SetName(mpt::ToUnicode(buffer));
		m_ModifiedTCs[GetpTuningCollection(m_pActiveTuning)] = true;
		UpdateView(UM_TUNINGDATA);
		UpdateView(UM_TUNINGCOLLECTION);
	}
}


void CTuningDialog::OnEnKillfocusEditSteps()
{
	if(m_pActiveTuning)
	{
		CString buffer;
		m_EditSteps.GetWindowText(buffer);
		m_pActiveTuning->ChangeGroupsize(ConvertStrTo<UNOTEINDEXTYPE>(buffer));
		m_ModifiedTCs[GetpTuningCollection(m_pActiveTuning)] = true;
		UpdateView(UM_TUNINGDATA);
	}
}


void CTuningDialog::OnEnKillfocusEditRatioperiod()
{
	double ratio = 0.0;
	if(m_pActiveTuning && m_EditRatioPeriod.GetDecimalValue(ratio))
	{
		m_pActiveTuning->ChangeGroupRatio(static_cast<RATIOTYPE>(ratio));
		m_ModifiedTCs[GetpTuningCollection(m_pActiveTuning)] = true;
		UpdateView(UM_TUNINGDATA);
	}
}

void CTuningDialog::OnEnKillfocusEditRatiovalue()
{
	UpdateView(UM_TUNINGDATA);
}


void CTuningDialog::OnEnKillfocusEditNotename()
{
	UpdateView(UM_TUNINGDATA);
}

bool CTuningDialog::GetModifiedStatus(const CTuningCollection* const pTc) const
{
	auto iter = m_ModifiedTCs.find(pTc);
	if(iter != m_ModifiedTCs.end())
		return (*iter).second;
	else
		return false;

}

CTuningCollection* CTuningDialog::GetpTuningCollection(HTREEITEM ti) const
{
	//If treeitem is that of a tuningcollection, return address of
	//that tuning collection. If treeitem is that of a tuning, return
	//the owning tuningcollection
	TUNINGTREEITEM tunItem = m_TreeItemTuningItemMap.GetMapping_12(ti);
	CTuningCollection* pTC = tunItem.GetTC();
	if(pTC)
		return pTC;
	else
	{
		CTuning* pT = tunItem.GetT();
		return GetpTuningCollection(pT);
	}
}

CTuningCollection* CTuningDialog::GetpTuningCollection(const CTuning* const pT) const
{
	for(auto &tuningCol : m_TuningCollections)
	{
		for(const auto &tuning : *tuningCol)
		{
			if(pT == tuning.get())
			{
				return tuningCol;
			}
		}
	}
	return NULL;
}


void CTuningDialog::OnTvnSelchangedTreeTuning(NMHDR *pNMHDR, LRESULT *pResult)
{
	//This methods gets called when selected item in the treeview
	//changes.

	//TODO: This gets called before killfocus messages of edits, which
	//		can be a problem.

	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	TUNINGTREEITEM ti = m_TreeItemTuningItemMap.GetMapping_12(pNMTreeView->itemNew.hItem);

	if(ti)
	{
		int updateMask = UM_TUNINGDATA;
		CTuningCollection* pPrevTuningCollection = m_pActiveTuningCollection;
		CTuning* pT = ti.GetT();
		CTuningCollection* pTC = ti.GetTC();
		if(pTC)
		{
			m_pActiveTuningCollection = pTC;
			ASSERT(pT == NULL);
			m_pActiveTuning = NULL;
		}
		else
		{
			m_pActiveTuning = pT;
			m_pActiveTuningCollection = GetpTuningCollection(m_pActiveTuning);

		}
		if(m_pActiveTuningCollection != pPrevTuningCollection) updateMask |= UM_TUNINGCOLLECTION;
		UpdateView(updateMask);
	}
	else
	{
		m_DoErrorExit = true;
	}

	*pResult = 0;
}

void CTuningDialog::OnTvnDeleteitemTreeTuning(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;
	if(pNMTreeView->itemOld.mask & TVIF_HANDLE && pNMTreeView->itemOld.hItem)
	{
		m_TreeItemTuningItemMap.RemoveValue_1(pNMTreeView->itemOld.hItem);
	}
	else
		m_DoErrorExit = true;
}

void CTuningDialog::OnNMRclickTreeTuning(NMHDR *, LRESULT *pResult)
{
	*pResult = 0;

	HTREEITEM hItem;
	POINT point, ptClient;

	GetCursorPos(&point);
	ptClient = point;
	m_TreeCtrlTuning.ScreenToClient(&ptClient);
	hItem = m_TreeCtrlTuning.HitTest(ptClient, NULL);
	if(hItem == NULL)
		return;

	m_TreeCtrlTuning.Select(hItem, TVGN_CARET);

	TUNINGTREEITEM tunitem = m_TreeItemTuningItemMap.GetMapping_12(hItem);

	if(!tunitem)
	{
		m_DoErrorExit = true;
		return;
	}

	HMENU popUpMenu = CreatePopupMenu();
	if(popUpMenu == NULL) return;

	CTuning* pT = tunitem.GetT();
	CTuningCollection* pTC = tunitem.GetTC();

	if(pT) //Creating context menu for tuning-item
	{
		pTC = GetpTuningCollection(pT);
		if(pTC != NULL)
		{
			UINT mask = MF_STRING;
			if(!CanEdit(pT, pTC))
			{
				mask |= MF_GRAYED;
			}
			AppendMenu(popUpMenu, mask, ID_REMOVETUNING, _T("&Remove"));

			m_CommandItemDest.Set(pT);
		}
	}
	else //Creating context menu for tuning collection item.
	{
		if(pTC != NULL)
		{
			UINT mask = MF_STRING;

			mask = MF_STRING;
			if (!CanEdit(pTC))
				mask |= MF_GRAYED;
			AppendMenu(popUpMenu, mask, ID_ADDTUNINGGROUPGEOMETRIC, _T("Add &GroupGeometric tuning"));
			AppendMenu(popUpMenu, mask, ID_ADDTUNINGGEOMETRIC, _T("Add G&eometric tuning"));
			AppendMenu(popUpMenu, mask, ID_ADDTUNINGGENERAL, _T("Add Ge&neral tuning"));

			mask = MF_STRING;
			if(!IsDeletable(pTC))
				mask |= MF_GRAYED;
			AppendMenu(popUpMenu, mask, ID_REMOVETUNINGCOLLECTION, _T("&Unload tuning collection"));

			m_CommandItemDest.Set(pTC);
		}
	}

	GetCursorPos(&point);
	TrackPopupMenu(popUpMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, point.x, point.y, 0, m_hWnd, NULL);
	DestroyMenu(popUpMenu);
}

bool CTuningDialog::IsDeletable(const CTuningCollection* const pTC) const
{
	auto iter = find(m_DeletableTuningCollections.begin(), m_DeletableTuningCollections.end(), pTC);
	if(iter != m_DeletableTuningCollections.end())
		return true;
	else
		return false;
}


void CTuningDialog::OnTvnBegindragTreeTuning(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;

	m_CommandItemDest.Reset();
	m_CommandItemSrc.Reset();
	if(pNMTreeView == NULL || pNMTreeView->itemNew.hItem == NULL) return;
	TUNINGTREEITEM tunitem = m_TreeItemTuningItemMap.GetMapping_12(pNMTreeView->itemNew.hItem);

	if(tunitem.GetT() == NULL)
	{
		Reporting::Message(LogNotification, _T("For the time being Drag and Drop is only supported for tuning instances."), this);
		return;
	}

	SetCursor(CMainFrame::curDragging);

	m_TreeCtrlTuning.SetDragging();
	m_DragItem = m_TreeItemTuningItemMap.GetMapping_12(pNMTreeView->itemNew.hItem);

	m_TreeCtrlTuning.Select(pNMTreeView->itemNew.hItem, TVGN_CARET);
}


CTuningCollection *CTuningDialog::CanDrop(HTREEITEM dragDestItem)
{
	if(!m_DragItem)
		return nullptr;

	TUNINGTREEITEM destTunItem = m_TreeItemTuningItemMap.GetMapping_12(dragDestItem);
	if(!destTunItem)
		return nullptr;

	CTuningCollection* pTCdest = nullptr;
	CTuningCollection* pTCsrc = m_DragItem.GetTC();

	if(pTCsrc == nullptr)
		pTCsrc = GetpTuningCollection(m_DragItem.GetT());

	if(pTCsrc == NULL)
	{
		ASSERT(false);
		return nullptr;
	}

	if(destTunItem.GetT()) //Item dragged on tuning
		pTCdest = GetpTuningCollection(destTunItem.GetT());
	else //Item dragged on tuningcollecition
		pTCdest = destTunItem.GetTC();

	//For now, ignoring drags within a tuning collection.
	if(pTCdest == pTCsrc)
		return nullptr;

	return pTCdest;
}


void CTuningDialog::OnEndDrag(HTREEITEM dragDestItem)
{
	SetCursor(CMainFrame::curArrow);
	m_TreeCtrlTuning.SetDragging(false);
	if(!m_DragItem)
		return;

	CTuningCollection* pTCdest = CanDrop(dragDestItem);
	m_CommandItemSrc = m_DragItem;
	m_DragItem.Reset();

	if(!pTCdest)
		return;

	CTuningCollection* pTCsrc = m_CommandItemSrc.GetTC();
	if(pTCsrc == nullptr)
		pTCsrc = GetpTuningCollection(m_CommandItemSrc.GetT());

	if(pTCdest)
	{
		UINT mask = MF_STRING;
		HMENU popUpMenu = CreatePopupMenu();
		if(popUpMenu == NULL) return;

		POINT point;
		GetCursorPos(&point);

		if(!CanEdit(pTCdest))
		{
			mask |= MF_GRAYED;
		}
		AppendMenu(popUpMenu, mask, ID_COPYTUNING, _T("&Copy here"));

		GetCursorPos(&point);
		TrackPopupMenu(popUpMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, point.x, point.y, 0, m_hWnd, NULL);
		DestroyMenu(popUpMenu);

		m_CommandItemDest.Set(pTCdest);
	}
}

bool CTuningDialog::AddTuning(CTuningCollection* pTC, CTuning* pT)
{
	//Default: pT == NULL

	if(!pTC)
	{
		Reporting::Notification("No tuning collection chosen");
		return false;
	}

	std::unique_ptr<CTuning> pNewTuning;
	if(pT)
	{
		pNewTuning = std::unique_ptr<CTuning>(new CTuning(*pT));
	} else
	{
		Reporting::Notification("Add tuning failed");
		return false;
	}
	CTuning *pNewTuningTmp = pTC->AddTuning(std::move(pNewTuning));
	if(!pNewTuningTmp)
	{
		Reporting::Notification("Add tuning failed");
		return false;
	}
	AddTreeItem(pNewTuningTmp, m_TreeItemTuningItemMap.GetMapping_21(TUNINGTREEITEM(pTC)), NULL);
	m_pActiveTuning = pNewTuningTmp;
	m_ModifiedTCs[pTC] = true;
	UpdateView();

	return true;
}

void CTuningDialog::OnAddTuningGeneral()
{
	if(!m_CommandItemDest.GetTC())
	{
		m_CommandItemDest = s_notFoundItemTuning;
		return;
	}

	CTuningCollection* pTC = m_CommandItemDest.GetTC();
	m_CommandItemDest = s_notFoundItemTuning;
	m_ModifiedTCs[pTC];
	AddTuning(pTC, Tuning::Type::GENERAL);
}

void CTuningDialog::OnAddTuningGroupGeometric()
{
	if(!m_CommandItemDest.GetTC())
	{
		m_CommandItemDest = s_notFoundItemTuning;
		return;
	}

	CTuningCollection* pTC = m_CommandItemDest.GetTC();
	m_CommandItemDest = s_notFoundItemTuning;
	m_ModifiedTCs[pTC];
	AddTuning(pTC, Tuning::Type::GROUPGEOMETRIC);
}

void CTuningDialog::OnAddTuningGeometric()
{
	if(!m_CommandItemDest.GetTC())
	{
		m_CommandItemDest = s_notFoundItemTuning;
		return;
	}

	CTuningCollection* pTC = m_CommandItemDest.GetTC();
	m_CommandItemDest = s_notFoundItemTuning;
	m_ModifiedTCs[pTC];
	AddTuning(pTC, Tuning::Type::GEOMETRIC);
}

void CTuningDialog::OnRemoveTuning()
{
	CTuning* pT = m_CommandItemDest.GetT();
	if(m_CommandItemDest.GetT())
	{
		CTuningCollection* pTC = GetpTuningCollection(pT);
		if(pTC)
		{
			bool used = false;
			for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
			{
				if(m_sndFile.Instruments[i]->pTuning == pT)
				{
					used = true;
				}
			}
			if(used)
			{
				CString s = _T("Tuning '") + mpt::ToCString(pT->GetName()) + _T("' is used by instruments. Remove anyway?");
				if(Reporting::Confirm(s, false, true) == cnfYes)
				{
					CriticalSection cs;
					for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
					{
						if(m_sndFile.Instruments[i]->pTuning == pT)
						{
							m_sndFile.Instruments[i]->SetTuning(nullptr);
						}
					}
					pTC->Remove(pT);
					cs.Leave();
					m_ModifiedTCs[pTC] = true;
					DeleteTreeItem(pT);
					UpdateView();
				}
			} else
			{
				CString s = _T("Remove tuning '") + mpt::ToCString(pT->GetName()) + _T("'?");
				if(Reporting::Confirm(s) == cnfYes)
				{
					pTC->Remove(pT);
					m_ModifiedTCs[pTC] = true;
					DeleteTreeItem(pT);
					UpdateView();
				}
			}
		}
	}

	m_CommandItemDest = s_notFoundItemTuning;
}


void CTuningDialog::OnCopyTuning()
{
	CTuningCollection* pTC = m_CommandItemDest.GetTC();

	if(!pTC)
		return;

	m_CommandItemDest = s_notFoundItemTuning;

	CTuning* pT = m_CommandItemSrc.GetT();
	if(pT == nullptr)
	{
		return;
	}
	m_ModifiedTCs[pTC] = true;
	AddTuning(pTC, pT);
}

void CTuningDialog::OnRemoveTuningCollection()
{
	if(!m_pActiveTuningCollection)
		return;

	if(!IsDeletable(m_pActiveTuningCollection))
	{
		ASSERT(false);
		return;
	}

	auto iter = find(m_TuningCollections.begin(), m_TuningCollections.end(), m_pActiveTuningCollection);
	if(iter == m_TuningCollections.end())
	{
		ASSERT(false);
		return;
	}
	auto DTCiter = find(m_DeletableTuningCollections.begin(), m_DeletableTuningCollections.end(), *iter);
	CTuningCollection* deletableTC = m_pActiveTuningCollection;
	//Note: Order matters in the following lines.
	m_DeletableTuningCollections.erase(DTCiter);
	m_TuningCollections.erase(iter);
	DeleteTreeItem(m_pActiveTuningCollection);
	m_TuningCollectionsNames.erase(deletableTC);
	m_TuningCollectionsFilenames.erase(deletableTC);
	delete deletableTC; deletableTC = 0;

	UpdateView();
}


void CTuningDialog::OnOK()
{
	// Prevent return-key from closing the window.
	if(GetKeyState(VK_RETURN) <= -127 && GetFocus() != GetDlgItem(IDOK))
		return;
	else
		CDialog::OnOK();
}


////////////////////////////////////////////////////////
//***************
//CTuningTreeCtrl
//***************
////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CTuningTreeCtrl, CTreeCtrl)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


void CTuningTreeCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if(IsDragging())
	{
		HTREEITEM hItem = HitTest(point, nullptr);
		SetCursor((hItem == NULL || m_rParentDialog.CanDrop(hItem) == nullptr) ? CMainFrame::curNoDrop2 : CMainFrame::curDragging);
	}

	CTreeCtrl::OnMouseMove(nFlags, point);
}


void CTuningTreeCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	if(IsDragging())
	{
		HTREEITEM hItem = HitTest(point, nullptr);
		m_rParentDialog.OnEndDrag(hItem);

		CTreeCtrl::OnLButtonUp(nFlags, point);
	}
}


////////////////////////////////////////////////////////
//
// scl import
//
////////////////////////////////////////////////////////

using SclFloat = double;

CString CTuningDialog::GetSclImportFailureMsg(EnSclImport id)
{
	switch(id)
	{
		case enSclImportFailTooManyNotes:
			return MPT_CFORMAT("OpenMPT supports importing scl-files with at most {} notes")(mpt::cfmt::val(s_nSclImportMaxNoteCount));

		case enSclImportFailTooLargeNumDenomIntegers:
			return _T("Invalid numerator or denominator");

		case enSclImportFailZeroDenominator:
			return _T("Zero denominator");

		case enSclImportFailNegativeRatio:
			return _T("Negative ratio");

		case enSclImportFailUnableToOpenFile:
			return _T("Unable to open file");

		case enSclImportLineCountMismatch:
			return _T("Note count error");

		case enSclImportTuningCreationFailure:
			return _T("Unknown tuning creation error");

		case enSclImportAddTuningFailure:
			return _T("Can't add tuning to tuning collection");

		default:
			return _T("");
	}
}


static void SkipCommentLines(std::istream& iStrm, std::string& str)
{
	std::string whitespace(" \t");
	while(std::getline(iStrm, str))
	{
		auto start = str.find_first_not_of(whitespace);
		// Lines starting with a ! are comments
		if(start != std::string::npos && str[start] != '!')
			return;
	}
}


static inline SclFloat CentToRatio(const SclFloat& val)
{
	return pow(2.0, val / 1200.0);
}


CTuningDialog::EnSclImport CTuningDialog::ImportScl(const mpt::PathString &filename, const mpt::ustring &name, std::unique_ptr<CTuning> & result)
{
	MPT_ASSERT(result == nullptr);
	result = nullptr;
	mpt::ifstream iStrm(filename, std::ios::in | std::ios::binary);
	if(!iStrm)
	{
		return enSclImportFailUnableToOpenFile;
	}
	return ImportScl(iStrm, name, result);
}


CTuningDialog::EnSclImport CTuningDialog::ImportScl(std::istream& iStrm, const mpt::ustring &name, std::unique_ptr<CTuning> & result)
{
	MPT_ASSERT(result == nullptr);
	result = nullptr;
	std::string str;

	std::string filename;
	bool first = true;
	std::string whitespace(" \t");
	while(std::getline(iStrm, str))
	{
		auto start = str.find_first_not_of(whitespace);
		// Lines starting with a ! are comments
		if(start != std::string::npos && str[start] != '!')
			break;
		if(first)
		{
			filename = mpt::String::Trim(str.substr(start + 1), std::string(" \t\r\n"));
		}
		first = false;
	}
	std::string description = mpt::String::Trim(str, std::string(" \t\r\n"));

	SkipCommentLines(iStrm, str);
	// str should now contain number of notes.
	const size_t nNotes = 1 + ConvertStrTo<size_t>(str.c_str());
	if (nNotes - 1 > s_nSclImportMaxNoteCount)
		return enSclImportFailTooManyNotes;

	std::vector<mpt::ustring> names;
	std::vector<Tuning::RATIOTYPE> fRatios;
	fRatios.reserve(nNotes);
	fRatios.push_back(1);

	char buffer[128];
	MemsetZero(buffer);

	while (iStrm.getline(buffer, sizeof(buffer)))
	{
		LPSTR psz = buffer;
		LPSTR const pEnd = psz + strlen(buffer);

		// Skip tabs and spaces.
		while(psz != pEnd && (*psz == ' ' || *psz == '\t'))
			psz++;

		// Skip empty lines, comment lines and non-text.
		if (*psz == 0 || *psz == '!' || *psz < 32)
			continue;

		char* pNonDigit = pEnd;

		// Check type of first non digit. This tells whether to read cent, ratio or plain number.
		for (pNonDigit = psz; pNonDigit != pEnd; pNonDigit++)
		{
			if (isdigit(*pNonDigit) == 0)
				break;
		}

		if (*pNonDigit == '.') // Reading cents
		{
			SclFloat fCent = ConvertStrTo<SclFloat>(psz);
			fRatios.push_back(static_cast<Tuning::RATIOTYPE>(CentToRatio(fCent)));
		}
		else if (*pNonDigit == '/') // Reading ratios
		{
			*pNonDigit = 0; // Replace '/' with null.
			int64 nNum = ConvertStrTo<int64>(psz);
			psz = pNonDigit + 1;
			int64 nDenom = ConvertStrTo<int64>(psz);

			if (nNum > int32_max || nDenom > int32_max)
				return enSclImportFailTooLargeNumDenomIntegers;
			if (nDenom == 0)
				return enSclImportFailZeroDenominator;

			fRatios.push_back(static_cast<Tuning::RATIOTYPE>((SclFloat)nNum / (SclFloat)nDenom));
		}
		else // Plain numbers.
			fRatios.push_back(static_cast<Tuning::RATIOTYPE>(ConvertStrTo<int32>(psz)));

		std::string remainder = psz;
		remainder = mpt::String::Trim(remainder, std::string("\r\n"));
		if(remainder.find_first_of(" \t") != std::string::npos)
		{
			remainder = remainder.substr(remainder.find_first_of(" \t"));
		} else
		{
			remainder = std::string();
		}
		remainder = mpt::String::Trim(remainder, std::string(" \t"));
		if(!remainder.empty())
		{
			if(remainder[0] == '!')
			{
				remainder = remainder.substr(1);
				remainder = mpt::String::Trim(remainder, std::string(" \t"));
			}
		}
		if(mpt::ToLowerCaseAscii(remainder) == "cents" || mpt::ToLowerCaseAscii(remainder) == "cent")
		{
			remainder = std::string();
		}
		names.push_back(mpt::ToUnicode(mpt::Charset::ISO8859_1, remainder));
		
	}

	if (nNotes != fRatios.size())
		return enSclImportLineCountMismatch;

	for(size_t i = 0; i < fRatios.size(); i++)
	{
		if (fRatios[i] < 0)
			return enSclImportFailNegativeRatio;
	}

	Tuning::RATIOTYPE groupRatio = fRatios.back();
	fRatios.pop_back();

	mpt::ustring tuningName;
	if(!description.empty())
	{
		tuningName = mpt::ToUnicode(mpt::Charset::ISO8859_1, description);
	} else if(!filename.empty())
	{
		tuningName = mpt::ToUnicode(mpt::Charset::ISO8859_1, filename);
	} else if(!name.empty())
	{
		tuningName = name;
	} else
	{
		tuningName = MPT_UFORMAT("{} notes: {}:{}")(nNotes - 1, mpt::ufmt::fix(groupRatio), 1);
	}

	std::unique_ptr<CTuning> pT = CTuning::CreateGroupGeometric(tuningName, fRatios, groupRatio, 15);
	if(!pT)
	{
		return enSclImportTuningCreationFailure;
	}

	bool allNamesEmpty = true;
	bool allNamesValid = true;
	for(NOTEINDEXTYPE note = 0; note < mpt::saturate_cast<NOTEINDEXTYPE>(names.size()); ++note)
	{
		if(names[note].empty())
		{
			allNamesValid = false;
		} else
		{
			allNamesEmpty = false;
		}
	}

	if(nNotes - 1 == 12 && !allNamesValid)
	{
		for(NOTEINDEXTYPE note = 0; note < mpt::saturate_cast<NOTEINDEXTYPE>(names.size()); ++note)
		{
			pT->SetNoteName(note, mpt::ustring(CSoundFile::GetDefaultNoteNames()[note]));
		}
	} else
	{
		for(NOTEINDEXTYPE note = 0; note < mpt::saturate_cast<NOTEINDEXTYPE>(names.size()); ++note)
		{
			if(!names[note].empty())
			{
				pT->SetNoteName(note, names[(note - 1 + names.size()) % names.size()]);
			}
		}
	}

	result = std::move(pT);

	return enSclImportOk;
}


OPENMPT_NAMESPACE_END
