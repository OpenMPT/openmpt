#ifndef TUNINGDIALOG_H
#define TUNINGDIALOG_H

#include "tuningratiomapwnd.h"
#include "tuningcollection.h"
#include <vector>
#include <string>
#include "afxcmn.h"
#include "afxwin.h"

using std::vector;
using std::string;

//==========================
template<class T1, class T2>
class CBijectiveMap
//==========================
{
public:
	CBijectiveMap(const T1& a, const T2& b)
		:	m_NotFoundT1(a),
			m_NotFoundT2(b)
	{}

	void AddPair(const T1& a, const T2& b)
	{
		m_T1.push_back(a);
		m_T2.push_back(b);
	}

	void ClearMapping()
	{
		m_T1.clear();
		m_T2.clear();
	}

	size_t Size() const
	{
		ASSERT(m_T1.size() == m_T2.size());
		return m_T1.size();
	}

	void RemoveValue_1(const T1& a)
	{
		vector<T1>::iterator iter = find(m_T1.begin(), m_T1.end(), a);
		if(iter != m_T1.end())
		{
			m_T2.erase(m_T2.begin() + (iter-m_T1.begin()));
			m_T1.erase(iter);
		}
	}

	void RemoveValue_2(const T2& b)
	{
		vector<T2>::iterator iter = find(m_T2.begin(), m_T2.end(), b);
		if(iter != m_T2.end())
		{
			m_T1.erase(m_T1.begin() + (iter-m_T2.begin()));
			m_T2.erase(iter);
		}
	}

	T2 GetMapping_12(const T1& a) const
	{
		vector<T1>::const_iterator iter = find(m_T1.begin(), m_T1.end(), a);
		if(iter != m_T1.end())
		{
			return m_T2[iter-m_T1.begin()];
		}
		else
			return m_NotFoundT2;
	}

	T1 GetMapping_21(const T2& b) const
	{
		vector<T2>::const_iterator iter = find(m_T2.begin(), m_T2.end(), b);
		if(iter != m_T2.end())
		{
			return m_T1[iter-m_T2.begin()];
		}
		else
			return m_NotFoundT1;
	}

private:
	//Elements are collected to two arrays so that elements with the 
	//same index are mapped to each other.
	vector<T1> m_T1;
	vector<T2> m_T2;

	T1 m_NotFoundT1;
	T2 m_NotFoundT2;
};

class CTuningDialog;

//======================================
class CTuningTreeCtrl : public CTreeCtrl
//======================================
{
private:
	CTuningDialog& m_rParentDialog;
public:
	CTuningTreeCtrl(CTuningDialog* parent) : m_rParentDialog(*parent) {}
	//Note: Parent address may be given in initialiser list so 
	//do not use it.

	void SetDragging(bool state = true) {m_Dragging = state;}
	bool IsDragging() {return m_Dragging;}

private:
	bool m_Dragging;

	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()	
};

//===================
class CTuningTreeItem
//===================
{	
private:
	CTuning* m_pTuning;
	CTuningCollection* m_pTuningCollection;

public:
	CTuningTreeItem() : m_pTuning(NULL),
						m_pTuningCollection(NULL)
	{}

	CTuningTreeItem(CTuning* pT) : 
					m_pTuning(pT),
					m_pTuningCollection(NULL)
	{}

	CTuningTreeItem(CTuningCollection* pTC) : 
					m_pTuning(NULL),
					m_pTuningCollection(pTC)
	{}

	bool operator==(const CTuningTreeItem& ti) const
	{
		if(m_pTuning == ti.m_pTuning &&
			m_pTuningCollection == ti.m_pTuningCollection)
			return true;
		else
			return false;
	}

	void Reset() {m_pTuning = NULL; m_pTuningCollection = NULL;}
	

	void Set(CTuning* pT)
	{
		m_pTuning = pT;
		m_pTuningCollection = NULL;
	}

	void Set(CTuningCollection* pTC)
	{
		m_pTuning = NULL;
		m_pTuningCollection = pTC;
	}

	operator void*()
	{
		//Mimicing pointer behavior: if(CTuningTreeItemInstance) equals
		//if(CTuningTreeItemInstance.m_pTuning != NULL ||
		//	 CTuningTreeItemInstance.m_pTuningCollection != NULL)
		if(m_pTuning)
			return m_pTuning; 
		else 
			return m_pTuningCollection;
	}

	CTuningCollection* GetTC() {return m_pTuningCollection;}

	CTuning* GetT() {return m_pTuning;}
};

// CTuningDialog dialog

//==================================
class CTuningDialog : public CDialog
//==================================
{
	DECLARE_DYNAMIC(CTuningDialog)

	friend class CTuningTreeCtrl;

public:
	typedef vector<CTuningCollection*> TUNINGVECTOR;

public:
	CTuningDialog(CWnd* pParent = NULL, const TUNINGVECTOR& = TUNINGVECTOR(), CTuning* pTun = NULL);   // standard constructor
	virtual ~CTuningDialog();

	BOOL OnInitDialog();

	void AddTuningCollection(CTuningCollection* pTC) {if(pTC) m_TuningCollections.push_back(pTC);}
	void UpdateRatioMapEdits(const CTuning::STEPTYPE&);

	bool GetModifiedStatus(const CTuningCollection* const pTc) const;

// Dialog Data
	enum { IDD = IDD_TUNING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

private:
	CTuning::CTUNINGTYPE GetTuningTypeFromStr(const string& str) const;

	void UpdateView(const int UpdateMask = 0);
	void UpdateTuningType();

	HTREEITEM AddTreeItem(CTuningCollection* pTC, HTREEITEM parent, HTREEITEM insertAfter);
	HTREEITEM AddTreeItem(CTuning* pT, HTREEITEM parent, HTREEITEM insertAfter);

	void DeleteTreeItem(CTuning* pT);
	void DeleteTreeItem(CTuningCollection* pTC);

	void OnEndDrag(HTREEITEM dragDestItem);

	CTuningCollection* GetpTuningCollection(const CTuning* const) const;
	//Returns pointer to the tuning collection where tuning given as argument
	//belongs to.

	CTuningCollection* GetpTuningCollection(HTREEITEM ti) const;
	//Returns the address of corresponding tuningcollection; if it points
	//to tuning-entry, returning the owning tuningcollection

	bool IsDeletable(const CTuningCollection* const pTC) const;
	//Checks whether tuning collection can be deleted.

private:
	CTuningRatioMapWnd m_RatioMapWnd;
	TUNINGVECTOR m_TuningCollections;
	vector<CTuningCollection*> m_DeletableTuningCollections;

	CTuning* m_pActiveTuning;
	CTuningCollection* m_pActiveTuningCollection;

	CTuningCollection m_TempTunings;

	CComboBox m_CombobTuningType;

	//Tuning Edits-->
	CEdit m_EditTableSize;
	CEdit m_EditBeginNote;
	CEdit m_EditSteps;
	CEdit m_EditRatioPeriod;
	CEdit m_EditRatio;
	CEdit m_EditNotename;
	CEdit m_EditMiscActions;
	CEdit m_EditFineTuneSteps;
	CEdit m_EditName;
	//<--Tuning Edits

	//-->Tuning collection edits
	CEdit m_EditTuningCollectionName;
	CEdit m_EditTuningCollectionVersion;
	CEdit m_EditTuningCollectionEditMask;
	CEdit m_EditTuningCollectionItemNum;
	CEdit m_EditTuningCollectionPath;
	//<--Tuningcollection edits
	
	CButton m_ButtonSet;
	CButton m_ButtonExport;
	CButton m_ButtonImport;
	CButton m_ButtonReadOnly;

	CTuningTreeCtrl m_TreeCtrlTuning;

private:
	static const string s_stringTypeGEN;
	static const string s_stringTypeRP;
	static const string s_stringTypeTET;

	typedef CTuningTreeItem TUNINGTREEITEM;
	typedef CBijectiveMap<HTREEITEM, TUNINGTREEITEM> TREETUNING_MAP;
	TREETUNING_MAP m_TreeItemTuningItemMap;

	TUNINGTREEITEM m_DragItem;
	TUNINGTREEITEM m_CommandItemSrc;
	TUNINGTREEITEM m_CommandItemDest;
	//Commanditem is used when receiving context menu-commands,
	//m_CommandItemDest is used when the command really need only 
	//one argument.

	typedef map<const CTuningCollection* const, bool> MODIFIED_MAP;
	MODIFIED_MAP m_ModifiedTCs;
	//If tuning collection seems to have been modified, its address
	//is added to this map.

	enum
	{
		TT_TUNINGCOLLECTION = 1,
		TT_TUNING
	};

	bool m_NoteEditApply;
	bool m_RatioEditApply;
	//To indicate whether to apply changes made to 
	//to those edit boxes(they are modified by non-user 
	//activies and in these cases the value should be applied
	//to the tuning data.

	enum
	{
		UM_TUNINGDATA = 1, //UM <-> Update Mask
		UM_TUNINGCOLLECTION = 2,
	};

	static const TUNINGTREEITEM s_notFoundItemTuning;
	static const HTREEITEM s_notFoundItemTree;

	bool AddTuning(CTuningCollection*, CTuning* pT = NULL);

	bool m_DoErrorExit;
	//Flag to prevent multiple exit error-messages.

	void DoErrorExit();


//Treectrl context menu functions.
public:
	afx_msg void OnRemoveTuning();
	afx_msg void OnAddTuning();
	afx_msg void OnMoveTuning();
	afx_msg void OnCopyTuning();
	afx_msg void OnRemoveTuningCollection();
	
//Event-functions
public:
	afx_msg void OnCbnSelchangeComboTtype();
	afx_msg void OnEnChangeEditSteps();
	afx_msg void OnEnChangeEditRatioperiod();
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
	afx_msg void OnBnClickedButtonTuningcollectionSave();

	//Treeview events
	afx_msg void OnTvnSelchangedTreeTuning(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnDeleteitemTreeTuning(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickTreeTuning(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnBegindragTreeTuning(NMHDR *pNMHDR, LRESULT *pResult);	

	DECLARE_MESSAGE_MAP()
};

#endif
