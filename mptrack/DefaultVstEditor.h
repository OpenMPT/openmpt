/*
 * DefaultVstEditor.h
 * ------------------
 * Purpose: Implementation of the default plugin editor that is used if a plugin does not provide an own editor GUI.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifndef NO_PLUGINS

#include "mptrack.h"
#include "AbstractVstEditor.h"

OPENMPT_NAMESPACE_BEGIN

enum
{
	PARAM_RESOLUTION = 1000,
	NUM_PLUGINEDITOR_PARAMETERS = 8,	// Parameters on screen
};

struct Measurements;

//===================
class ParamControlSet
//===================
{
protected:
	CSliderCtrl valueSlider;
	CEdit valueEdit;
	CStatic nameLabel;
	CStatic valueLabel;
	CStatic perMilLabel;

public:
	ParamControlSet(CWnd *parent, const CRect &rect, int setID, const Measurements &m);
	~ParamControlSet();

	void EnableControls(bool enable = true);
	void ResetContent();

	void SetParamName(const CString &name);
	void SetParamValue(int value, const CString &text);

	int GetParamValueFromSlider() const;
	int GetParamValueFromEdit() const;

	int GetSliderID() const { return valueSlider.GetDlgCtrlID(); };
};


//=================================================
class CDefaultVstEditor : public CAbstractVstEditor
//=================================================
{
protected:

	std::vector<ParamControlSet *> controls;

	CScrollBar paramScroller;
	PlugParamIndex paramOffset;

	int m_nControlLock;

public:

	CDefaultVstEditor(IMixPlugin &plugin);
	virtual ~CDefaultVstEditor();

	virtual void UpdateParamDisplays() { CAbstractVstEditor::UpdateParamDisplays(); UpdateControls(false); };

	virtual bool OpenEditor(CWnd *parent);

	// Plugins may not request to change the GUI size, since we use our own GUI.
	virtual bool IsResizable() const { return false; };
	virtual bool SetSize(int, int) { return false; };

protected:

	virtual void DoDataExchange(CDataExchange* pDX);
	
	DECLARE_MESSAGE_MAP()

	afx_msg void OnParamTextboxChanged(UINT id);
	afx_msg void OnParamSliderChanged(UINT id);

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

protected:

	void CreateControls();
	void UpdateControls(bool updateParamNames);
	void SetParam(PlugParamIndex param, int value);
	void UpdateParamDisplay(PlugParamIndex param);

};

OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
