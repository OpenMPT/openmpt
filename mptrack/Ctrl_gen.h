/*
 * ctrl_gen.h
 * ----------
 * Purpose: General tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "CDecimalSupport.h"

OPENMPT_NAMESPACE_BEGIN

//=========================
class CVuMeter: public CWnd
//=========================
{
protected:
	LONG m_nDisplayedVu, m_nVuMeter;
	DWORD lastVuUpdateTime;

public:
	CVuMeter() { m_nDisplayedVu = -1; lastVuUpdateTime = timeGetTime(); m_nVuMeter = 0; }
	VOID SetVuMeter(LONG lVuMeter, bool force=false);

protected:
	VOID DrawVuMeter(CDC &dc, bool redraw=false);

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP();
};

enum
{
	MAX_SLIDER_GLOBAL_VOL=256,
	MAX_SLIDER_VSTI_VOL=255,
	MAX_SLIDER_SAMPLE_VOL=255
};


//=======================================
class CCtrlGeneral: public CModControlDlg
//=======================================
{
public:
	CCtrlGeneral(CModControlView &parent, CModDoc &document);
	Setting<LONG> &GetSplitPosRef() {return TrackerSettings::Instance().glGeneralWindowHeight;} 	//rewbs.varWindowSize

private:
	void setAsDecibels(LPSTR stringToSet, double value, double valueAtZeroDB);

	// Determine how the global volume slider should be scaled to actual global volume.
	// Display range for XM / S3M should be 0...64, for other formats it's 0...256.
	UINT GetGlobalVolumeFactor()
	{
		return (m_sndFile.GetType() & (MOD_TYPE_XM | MOD_TYPE_S3M)) ? UINT(MAX_SLIDER_GLOBAL_VOL / 64) : UINT(MAX_SLIDER_GLOBAL_VOL / 128);
	}

public:
	bool m_bEditsLocked;
	//{{AFX_DATA(CCtrlGeneral)
	CEdit m_EditTitle, m_EditArtist;
	CEdit m_EditSpeed, m_EditGlobalVol, m_EditRestartPos,
		  m_EditSamplePA, m_EditVSTiVol;
	CNumberEdit m_EditTempo;
	CButton m_BtnModType;
	CSpinButtonCtrl m_SpinTempo, m_SpinSpeed, m_SpinGlobalVol, m_SpinRestartPos, 
				    m_SpinSamplePA, m_SpinVSTiVol;
	CComboBox m_CbnResampling;

	CSliderCtrl m_SliderTempo, m_SliderSamplePreAmp, m_SliderGlobalVol, m_SliderVSTiVol;
	CVuMeter m_VuMeterLeft, m_VuMeterRight;

	TEMPO tempoMin, tempoMax;
	//}}AFX_DATA
	//{{AFX_VIRTUAL(CCtrlGeneral)
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void RecalcLayout();
	virtual void UpdateView(UpdateHint hint, CObject *pObj = nullptr);
	virtual CRuntimeClass *GetAssociatedViewClass();
	virtual void OnActivatePage(LPARAM);
	virtual void OnDeactivatePage();
	virtual BOOL GetToolTipText(UINT uId, LPSTR pszText);
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CCtrlGeneral)
	afx_msg LRESULT OnUpdatePosition(WPARAM, LPARAM);
	afx_msg void OnVScroll(UINT, UINT, CScrollBar *);
	afx_msg void OnTapTempo();
	afx_msg void OnTitleChanged();
	afx_msg void OnArtistChanged();
	afx_msg void OnTempoChanged();
	afx_msg void OnSpeedChanged();
	afx_msg void OnGlobalVolChanged();
	afx_msg void OnVSTiVolChanged();
	afx_msg void OnSamplePAChanged();
	afx_msg void OnRestartPosChanged();
	afx_msg void OnSongProperties();
	afx_msg void OnLoopSongChanged();
	afx_msg void OnEnSetfocusEditSongtitle();
	afx_msg void OnResamplingChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
