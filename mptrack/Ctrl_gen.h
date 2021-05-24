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

#include "openmpt/all/BuildSettings.hpp"

#include "CDecimalSupport.h"

OPENMPT_NAMESPACE_BEGIN

namespace Util { class MultimediaClock; }

class CVuMeter final : public CWnd
{
protected:
	int m_lastDisplayedLevel = -1, m_lastLevel = 0;
	DWORD m_lastVuUpdateTime;

public:
	CVuMeter() { m_lastVuUpdateTime = timeGetTime(); }
	void SetVuMeter(int level, bool force = false);

protected:
	void DrawVuMeter(CDC &dc, bool redraw = false);

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP();
};


class CCtrlGeneral final : public CModControlDlg
{
public:
	CCtrlGeneral(CModControlView &parent, CModDoc &document);
	Setting<LONG> &GetSplitPosRef() override { return TrackerSettings::Instance().glGeneralWindowHeight; }

private:

	// Determine how the global volume slider should be scaled to actual global volume.
	// Display range for XM / S3M should be 0...64, for other formats it's 0...256.
	uint32 GetGlobalVolumeFactor() const
	{
		return (m_sndFile.GetType() & (MOD_TYPE_XM | MOD_TYPE_S3M)) ? uint32(MAX_SLIDER_GLOBAL_VOL / 64) : uint32(MAX_SLIDER_GLOBAL_VOL / 128);
	}

public:
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
	std::unique_ptr<Util::MultimediaClock> m_tapTimer;
	bool m_editsLocked = false;

	TEMPO m_tempoMin, m_tempoMax;

	//{{AFX_VIRTUAL(CCtrlGeneral)
	BOOL OnInitDialog() override;
	void DoDataExchange(CDataExchange *pDX) override;  // DDX/DDV support
	void RecalcLayout() override;
	void UpdateView(UpdateHint hint, CObject *pObj = nullptr) override;
	CRuntimeClass *GetAssociatedViewClass() override;
	void OnActivatePage(LPARAM) override;
	void OnDeactivatePage() override;
	BOOL GetToolTipText(UINT uId, LPTSTR pszText) override;
	//}}AFX_VIRTUAL

protected:
	static constexpr int MAX_SLIDER_GLOBAL_VOL = 256;
	static constexpr int MAX_SLIDER_VSTI_VOL = 255;
	static constexpr int MAX_SLIDER_SAMPLE_VOL = 255;

	// At this point, the tempo slider moves in more coarse steps to provide detailed values in the regions where it matters
	static constexpr auto TEMPO_SPLIT_THRESHOLD = TEMPO(256, 0);
	static constexpr int TEMPO_SPLIT_PRECISION = 3;

	TEMPO TempoSliderRange() const;
	TEMPO SliderToTempo(int value) const;
	int TempoToSlider(TEMPO tempo) const;

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
	afx_msg void OnRestartPosDone();
	afx_msg void OnSongProperties();
	afx_msg void OnLoopSongChanged();
	afx_msg void OnEnSetfocusEditSongtitle();
	afx_msg void OnResamplingChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
