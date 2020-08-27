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

#include "BuildSettings.h"
#include "ColorPickerButton.h"

OPENMPT_NAMESPACE_BEGIN

//Note: Changing this won't increase the number of tabs in general view. Most
//of the code use plain number 4.
#define CHANNELS_IN_TAB	4

class CViewGlobals: public CFormView
{
protected:
	CRect m_rcClient;
	CTabCtrl m_TabCtrl;
	CComboBox m_CbnEffects[CHANNELS_IN_TAB];
	CComboBox m_CbnPlugin, m_CbnParam, m_CbnOutput;

	CSliderCtrl m_sbVolume[CHANNELS_IN_TAB], m_sbPan[CHANNELS_IN_TAB], m_sbValue, m_sbDryRatio;
	ColorPickerButton m_channelColor[CHANNELS_IN_TAB];

	CComboBox m_CbnPreset;
	CSliderCtrl m_sbWetDry;
	CSpinButtonCtrl m_spinVolume[CHANNELS_IN_TAB], m_spinPan[CHANNELS_IN_TAB];
	CButton m_BtnSelect, m_BtnEdit;
	int m_nLockCount = 1;
	PlugParamIndex m_nCurrentParam = 0;
	CHANNELINDEX m_nActiveTab = 0;
	CHANNELINDEX m_lastEdit = CHANNELINDEX_INVALID;
	PLUGINDEX m_nCurrentPlugin = 0;

	CComboBox m_CbnSpecialMixProcessing;
	CSpinButtonCtrl m_SpinMixGain;

	enum {AdjustPattern = true, NoPatternAdjust = false};

protected:
	CViewGlobals() : CFormView(IDD_VIEW_GLOBALS) { }
	DECLARE_SERIAL(CViewGlobals)

public:
	CModDoc* GetDocument() const { return static_cast<CModDoc *>(m_pDocument); }
	void RecalcLayout();
	void LockControls() { m_nLockCount++; }
	void UnlockControls() { PostMessage(WM_MOD_UNLOCKCONTROLS); }
	bool IsLocked() const noexcept { return (m_nLockCount > 0); }
	int GetDlgItemIntEx(UINT nID);
	void PopulateChannelPlugins();
	void BuildEmptySlotList(std::vector<PLUGINDEX> &emptySlots);
	bool MovePlug(PLUGINDEX src, PLUGINDEX dest, bool bAdjustPat = AdjustPattern);

public:
	//{{AFX_VIRTUAL(CViewGlobals)
	void OnInitialUpdate() override;
	void DoDataExchange(CDataExchange *pDX) override;
	void OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint) override;

	void UpdateView(UpdateHint hint, CObject *pObj = nullptr);
	LRESULT OnModViewMsg(WPARAM, LPARAM);
	LRESULT OnMidiMsg(WPARAM midiData, LPARAM);

private:
	void PrepareUndo(CHANNELINDEX chnMod4);
	void UndoRedo(bool undo);

	void OnEditColor(const CHANNELINDEX chnMod4);
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
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateUndo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateRedo(CCmdUI *pCmdUI);

	afx_msg void OnEditColor1();
	afx_msg void OnEditColor2();
	afx_msg void OnEditColor3();
	afx_msg void OnEditColor4();
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
	afx_msg LRESULT OnMDIDeactivate(WPARAM, LPARAM);
	afx_msg LRESULT OnUnlockControls(WPARAM, LPARAM) { if (m_nLockCount > 0) m_nLockCount--; return 0; }
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


OPENMPT_NAMESPACE_END
