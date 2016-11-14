/*
 * TempoSwingDialog.h
 * ------------------
 * Purpose: Implementation of the tempo swing configuration dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

class CSoundFile;
#include "../soundlib/Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

//==================================
class CTempoSwingDlg: public CDialog
//==================================
{
protected:
	// Scrollable container for the sliders
	class SliderContainer : public CStatic
	{
	public:
		CTempoSwingDlg &m_parent;

		SliderContainer(CTempoSwingDlg &parent) : m_parent(parent) { }

		afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
		afx_msg BOOL OnToolTipNotify(UINT, NMHDR *pNMHDR, LRESULT *);
		DECLARE_MESSAGE_MAP()
	};

	enum { SliderResolution = 1000, SliderUnity = SliderResolution / 2 };
	struct RowCtls
	{
		CStatic rowLabel, valueLabel;
		CSliderCtrl valueSlider;

		void SetValue(TempoSwing::value_type v);
		TempoSwing::value_type GetValue() const;
	};
	std::vector<std::shared_ptr<RowCtls>> m_controls;

	CButton m_checkGroup;
	CScrollBar m_scrollBar;
	SliderContainer m_container;

	int m_scrollPos;
	static int m_groupSize;

public:
	TempoSwing m_tempoSwing;
	const TempoSwing m_origTempoSwing;
	CSoundFile &m_sndFile;
	PATTERNINDEX m_pattern;

public:
	CTempoSwingDlg(CWnd *parent, const TempoSwing &currentTempoSwing, CSoundFile &sndFile, PATTERNINDEX pattern = PATTERNINDEX_INVALID);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	void OnClose();
	afx_msg void OnReset();
	afx_msg void OnUseGlobal();
	afx_msg void OnToggleGroup();
	afx_msg void OnGroupChanged();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
