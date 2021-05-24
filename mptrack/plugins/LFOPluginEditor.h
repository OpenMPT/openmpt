/*
 * LFOPluginEditor.h
 * -----------------
 * Purpose: Editor interface for the LFO plugin.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "../AbstractVstEditor.h"
#include "../../soundlib/plugins/LFOPlugin.h"

OPENMPT_NAMESPACE_BEGIN

struct UpdateHint;

class LFOPluginEditor : public CAbstractVstEditor
{
protected:
	CComboBox m_plugParam, m_outPlug, m_midiCC;
	CSliderCtrl m_amplitudeSlider, m_offsetSlider, m_frequencySlider;
	CEdit m_midiChnEdit;
	CSpinButtonCtrl m_midiChnSpin;
	LFOPlugin &m_lfoPlugin;
	bool m_locked : 1;
	static constexpr int SLIDER_GRANULARITY = 1000;

public:

	LFOPluginEditor(LFOPlugin &plugin);

	bool OpenEditor(CWnd *parent) override;
	bool IsResizable() const override { return false; }
	bool SetSize(int, int) override { return false; }

	void UpdateParamDisplays() override;
	void UpdateParam(int32 param) override;
	void UpdateView(UpdateHint hint) override;

protected:
	void DoDataExchange(CDataExchange* pDX) override;
	void OnHScroll(UINT nCode, UINT nPos, CScrollBar *pSB);

	void InitSlider(CSliderCtrl &slider, LFOPlugin::Parameters param);
	void SetSliderText(LFOPlugin::Parameters param);
	void SetSliderValue(CSliderCtrl &slider, float value);
	float GetSliderValue(CSliderCtrl &slider);

	void OnPolarityChanged();
	void OnTempoSyncChanged();
	void OnBypassChanged();
	void OnLoopModeChanged();
	void OnWaveformChanged(UINT nID);
	void OnPlugParameterChanged();
	void OnMidiCCChanged();
	void OnParameterChanged();
	void OnOutputPlugChanged();
	void OnPluginEditor();
	LRESULT OnUpdateParam(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
