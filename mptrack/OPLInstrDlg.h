/*
 * OPLInstrDlg.h
 * -------------
 * Purpose: Editor for OPL-based synth instruments
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "../soundlib/Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;

class OPLInstrDlg : public CDialog
{
	CButton m_additive, m_sustain[2], m_scaleEnv[2], m_vibrato[2], m_tremolo[2];
	CSliderCtrl m_feedback, m_attackRate[2], m_decayRate[2], m_sustainLevel[2], m_releaseRate[2], m_volume[2], m_levelScaling[2], m_freqMultiplier[2];
	CComboBox m_waveform[2];
	CSize m_windowSize;
	CWnd &m_parent;
	const CSoundFile &m_sndFile;
	OPLPatch *m_patch;

public:
	OPLInstrDlg(CWnd &parent, const CSoundFile &sndFile);
	~OPLInstrDlg();
	void SetPatch(OPLPatch &patch);
	CSize GetMinimumSize() const { return m_windowSize; }

protected:
	void ParamsChanged();

	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;
	BOOL PreTranslateMessage(MSG *pMsg) override;
	void OnOK() override { }
	void OnCancel() override { }
	void OnHScroll(UINT, UINT, CScrollBar *) { ParamsChanged(); }
	LRESULT OnDragonDropping(WPARAM wParam, LPARAM lParam);
	BOOL OnToolTip(UINT id, NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
