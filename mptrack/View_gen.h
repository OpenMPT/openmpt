/*
 * view_gen.h
 * ----------
 * Purpose: General tab, lower panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

//Note: Changing this won't increase the number of tabs in general view. Most
//of the code use plain number 4.
#define CHANNELS_IN_TAB	4

//==================================
class CViewGlobals: public CFormView
//==================================
{
protected:
	CRect m_rcClient;
	CTabCtrl m_TabCtrl;
	CComboBox m_CbnEffects[4];
	CComboBox m_CbnPlugin, m_CbnParam, m_CbnOutput;

	CSliderCtrl m_sbVolume[4], m_sbPan[4], m_sbValue, m_sbDryRatio;

	CComboBox m_CbnPreset;
	CSliderCtrl m_sbWetDry;
	CSpinButtonCtrl m_spinVolume[4], m_spinPan[4];
	CButton m_BtnSelect, m_BtnEdit;
	int m_nLockCount;
	PlugParamIndex m_nCurrentParam;
	CHANNELINDEX m_nActiveTab;
	PLUGINDEX m_nCurrentPlugin;

	CComboBox m_CbnSpecialMixProcessing;
	CSpinButtonCtrl m_SpinMixGain;			// update#02

	enum {AdjustPattern = true, NoPatternAdjust = false};

protected:
	CViewGlobals():CFormView(IDD_VIEW_GLOBALS) { m_nLockCount = 1; }
	DECLARE_SERIAL(CViewGlobals)

public:
	virtual ~CViewGlobals() {}

public:
	CModDoc* GetDocument() const { return (CModDoc *)m_pDocument; }
	void RecalcLayout();
	void LockControls() { m_nLockCount++; }
	void UnlockControls() { PostMessage(WM_MOD_UNLOCKCONTROLS); }
	BOOL IsLocked() const { return (m_nLockCount > 0); }
	int GetDlgItemIntEx(UINT nID);
	void PopulateChannelPlugins();
	void BuildEmptySlotList(std::vector<PLUGINDEX> &emptySlots);
	bool MovePlug(PLUGINDEX src, PLUGINDEX dest, bool bAdjustPat = AdjustPattern);

public:
	//{{AFX_VIRTUAL(CViewGlobals)
	virtual void OnInitialUpdate();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void UpdateView(UpdateHint hint, CObject *pObj = nullptr);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual LRESULT OnModViewMsg(WPARAM, LPARAM);
	LRESULT OnMidiMsg(WPARAM midiData, LPARAM);
	virtual HBRUSH OnCtlColor(CDC *pDC, CWnd* pWnd, UINT nCtlColor);

private:
	void OnMute(const CHANNELINDEX chnMod4, const UINT itemID);
	void OnSurround(const CHANNELINDEX chnMod4, const UINT itemID);
	void OnEditVol(const CHANNELINDEX chnMod4, const UINT itemID);
	void OnEditPan(const CHANNELINDEX chnMod4, const UINT itemID);
	void OnEditName(const CHANNELINDEX chnMod4, const UINT itemID);
	void OnFxChanged(const CHANNELINDEX chnMod4);

	IMixPlugin *GetCurrentPlugin() const;

	void FillPluginProgramBox(int32 firstProg, int32 lastProg);
	void SetPluginModified();

	void UpdateDryWetDisplay();

protected:
	//{{AFX_MSG(CViewGlobals)
	afx_msg void OnMute1();
	afx_msg void OnMute2();
	afx_msg void OnMute3();
	afx_msg void OnMute4();
	afx_msg void OnSurround1();
	afx_msg void OnSurround2();
	afx_msg void OnSurround3();
	afx_msg void OnSurround4();
	afx_msg void OnEditVol1();
	afx_msg void OnEditVol2();
	afx_msg void OnEditVol3();
	afx_msg void OnEditVol4();
	afx_msg void OnEditPan1();
	afx_msg void OnEditPan2();
	afx_msg void OnEditPan3();
	afx_msg void OnEditPan4();
	afx_msg void OnEditName1();
	afx_msg void OnEditName2();
	afx_msg void OnEditName3();
	afx_msg void OnEditName4();
	afx_msg void OnFx1Changed();
	afx_msg void OnFx2Changed();
	afx_msg void OnFx3Changed();
	afx_msg void OnFx4Changed();
	afx_msg void OnPluginChanged();
	afx_msg void OnPluginNameChanged();
	afx_msg void OnFillParamCombo();
	afx_msg void OnParamChanged();
	afx_msg void OnFocusParam();
	afx_msg void OnFillProgramCombo();
	afx_msg void OnProgramChanged();
	afx_msg void OnLoadParam();
	afx_msg void OnSaveParam();
	afx_msg void OnSelectPlugin();
	afx_msg void OnSetParameter();
	afx_msg void OnEditPlugin();
	afx_msg void OnMixModeChanged();
	afx_msg void OnBypassChanged();
	afx_msg void OnDryMixChanged();
	afx_msg void OnMovePlugToSlot();
	afx_msg void OnInsertSlot();
	afx_msg void OnClonePlug();
	LRESULT OnParamAutomated(WPARAM plugin, LPARAM param);

	afx_msg void OnWetDryExpandChanged();
	afx_msg void OnSpecialMixProcessingChanged();

	afx_msg void OnOutputRoutingChanged();
	afx_msg void OnPrevPlugin();
	afx_msg void OnNextPlugin();
	afx_msg void OnDestroy();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTabSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnUnlockControls(WPARAM, LPARAM) { if (m_nLockCount > 0) m_nLockCount--; return 0; }
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


OPENMPT_NAMESPACE_END
