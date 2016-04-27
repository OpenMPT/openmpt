/*
 * DefaultVstEditor.cpp
 * --------------------
 * Purpose: Implementation of the default plugin editor that is used if a plugin does not provide an own editor GUI.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "DefaultVstEditor.h"
#include "../soundlib/Sndfile.h"
#include "../soundlib/plugins/PlugInterface.h"


OPENMPT_NAMESPACE_BEGIN


#ifndef NO_PLUGINS


// Window proportions
struct Measurements
{
	enum
	{
		edSpacing = 5,		// Spacing between elements
		edLineHeight = 20,	// Line of a single parameter line
		edEditWidth = 45,	// Width of the parameter edit box
		edPerMilWidth = 30,	// Width of the per mil label
		edRightWidth = edEditWidth + edSpacing + edPerMilWidth,	// Width of the right part of a parameter control set (edit box, param value)
		edTotalHeight = 2 * edLineHeight + edSpacing,			// Height of one set of controls
	};

	const int spacing;
	const int lineHeight;
	const int editWidth;
	const int perMilWidth;
	const int rightWidth;
	const int totalHeight;

	Measurements(HWND hWnd)
		: spacing(Util::ScalePixels(edSpacing, hWnd))
		, lineHeight(Util::ScalePixels(edLineHeight, hWnd))
		, editWidth(Util::ScalePixels(edEditWidth, hWnd))
		, perMilWidth(Util::ScalePixels(edPerMilWidth, hWnd))
		, rightWidth(Util::ScalePixels(edRightWidth, hWnd))
		, totalHeight(Util::ScalePixels(edTotalHeight, hWnd))
	{ }
};


// Create a set of parameter controls
ParamControlSet::ParamControlSet(CWnd *parent, const CRect &rect, int setID, const Measurements &m)
//-------------------------------------------------------------------------------------------------
{
	// Offset of components on the right side
	const int horizSplit = rect.left + rect.Width() - m.rightWidth;
	
	// Parameter name
	nameLabel.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, CRect(rect.left, rect.top, horizSplit - m.spacing, rect.top + m.lineHeight), parent);
	nameLabel.SetFont(parent->GetFont());

	// Parameter value as reported by the plugin
	valueLabel.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, CRect(horizSplit, rect.top, rect.right, rect.top + m.lineHeight), parent);
	valueLabel.SetFont(parent->GetFont());

	// Parameter value slider
	valueSlider.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP /* | TBS_NOTICKS | TBS_BOTH */ | TBS_AUTOTICKS, CRect(rect.left, rect.bottom - m.lineHeight, horizSplit - m.spacing, rect.bottom), parent, ID_PLUGINEDITOR_SLIDERS_BASE + setID);
	valueSlider.SetFont(parent->GetFont());
	valueSlider.SetRange(0, PARAM_RESOLUTION);
	valueSlider.SetTicFreq(PARAM_RESOLUTION / 10);

	// Parameter value edit box
	valueEdit.CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER, CRect(horizSplit, rect.bottom - m.lineHeight, horizSplit + m.editWidth, rect.bottom), parent, ID_PLUGINEDITOR_EDIT_BASE + setID);
	valueEdit.SetFont(parent->GetFont());

	// "Per mil" label
	perMilLabel.Create("‰", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, CRect(horizSplit + m.editWidth + m.spacing, rect.bottom - m.lineHeight, rect.right, rect.bottom), parent);
	perMilLabel.SetFont(parent->GetFont());
}


ParamControlSet::~ParamControlSet()
//---------------------------------
{
	nameLabel.DestroyWindow();
	valueLabel.DestroyWindow();
	valueSlider.DestroyWindow();
	valueEdit.DestroyWindow();
	perMilLabel.DestroyWindow();
}


// Enable a set of parameter controls
void ParamControlSet::EnableControls(bool enable)
//-----------------------------------------------
{
	const BOOL b = enable ? TRUE : FALSE;
	nameLabel.EnableWindow(b);
	valueLabel.EnableWindow(b);
	valueSlider.EnableWindow(b);
	valueEdit.EnableWindow(b);
	perMilLabel.EnableWindow(b);
}


// Reset the content of a set of parameter controls
void ParamControlSet::ResetContent()
//----------------------------------
{
	nameLabel.SetWindowText("");
	valueLabel.SetWindowText("");
	valueSlider.SetPos(0);
	valueEdit.SetWindowText("");
}


void ParamControlSet::SetParamName(const CString &name)
//-----------------------------------------------------
{
	nameLabel.SetWindowText(name);
}


void ParamControlSet::SetParamValue(int value, const CString &text)
//-----------------------------------------------------------------
{
	valueSlider.SetPos(value);
	if (&valueEdit != valueEdit.GetFocus())
	{
		// Don't update textbox when it has focus, else this will prevent user from changing the content.
		CString paramValue;
		paramValue.Format("%0000d", value);
		valueEdit.SetWindowText(paramValue);
	}
	valueLabel.SetWindowText(text);
}


int ParamControlSet::GetParamValueFromSlider() const
//--------------------------------------------------
{
	return valueSlider.GetPos();
}


int ParamControlSet::GetParamValueFromEdit() const
//------------------------------------------------
{
	char s[16];
	valueEdit.GetWindowText(s, 16);
	int val = atoi(s);
	Limit(val, int(0), int(PARAM_RESOLUTION));
	return val;
}


BEGIN_MESSAGE_MAP(CDefaultVstEditor, CAbstractVstEditor)
	//{{AFX_MSG_MAP(CDefaultVstEditor)
	ON_CONTROL_RANGE(EN_CHANGE, ID_PLUGINEDITOR_EDIT_BASE, ID_PLUGINEDITOR_EDIT_BASE + NUM_PLUGINEDITOR_PARAMETERS - 1, OnParamTextboxChanged)
	//}}AFX_MSG_MAP
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


void CDefaultVstEditor::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDefaultVstEditor)
	DDX_Control(pDX, IDC_SCROLLBAR1,	paramScroller);
	//}}AFX_DATA_MAP
}


CDefaultVstEditor::CDefaultVstEditor(IMixPlugin &plugin) : CAbstractVstEditor(plugin)
//-----------------------------------------------------------------------------------
{
	m_nControlLock = 0;
	paramOffset = 0;

	controls.clear();
}

CDefaultVstEditor::~CDefaultVstEditor()
//-------------------------------------
{
	for(size_t i = 0; i < controls.size(); i++)
	{
		delete controls[i];
	}
}


void CDefaultVstEditor::CreateControls()
//--------------------------------------
{
	// Already initialized.
	if(!controls.empty())
	{
		return;
	}
	Measurements m(m_hWnd);

	CRect window;
	GetWindowRect(&window);

	CRect rect;
	GetClientRect(&rect);
	int origHeight = rect.bottom;

	// Get parameter scroll bar dimensions and move it to the right side of the window
	CRect scrollRect;
	paramScroller.GetClientRect(&scrollRect);
	scrollRect.bottom = rect.bottom;
	scrollRect.MoveToX(rect.right - scrollRect.right);

	// Ignore this space in our calculation from now on.
	rect.right -= scrollRect.Width();

	controls.clear();

	// Create a bit of border space
	rect.DeflateRect(m.spacing, m.spacing);
	rect.bottom = m.totalHeight;

	for(int i = 0; i < NUM_PLUGINEDITOR_PARAMETERS; i++)
	{
		try
		{
			controls.push_back(new ParamControlSet(this, rect, i, m));
			rect.OffsetRect(0, m.totalHeight + m.spacing);
		} catch(MPTMemoryException)
		{
		}
	}

	// Calculate new ideal window height.
	const int heightChange = (rect.bottom - m.totalHeight + m.spacing) - origHeight;

	// Update parameter scroll bar height
	scrollRect.bottom += heightChange;
	paramScroller.MoveWindow(scrollRect, FALSE);

	// Resize window height
	window.bottom += heightChange;
	MoveWindow(&window);

	// Set scrollbar page size.
	SCROLLINFO sbInfo;
	paramScroller.GetScrollInfo(&sbInfo);
	sbInfo.nPage = NUM_PLUGINEDITOR_PARAMETERS;
	paramScroller.SetScrollInfo(&sbInfo);

	UpdateControls(true);
}


void CDefaultVstEditor::UpdateControls(bool updateParamNames)
//-----------------------------------------------------------
{
	const PlugParamIndex numParams = m_VstPlugin.GetNumParameters();
	const PlugParamIndex scrollMax = numParams - MIN(numParams, NUM_PLUGINEDITOR_PARAMETERS);
	LimitMax(paramOffset, scrollMax);

	int curScrollMin, curScrollMax;
	paramScroller.GetScrollRange(&curScrollMin, &curScrollMax);
	if(curScrollMax != scrollMax)
	{
		// Number of parameters changed - update scrollbar limits
		paramScroller.SetScrollRange(0, scrollMax);
		paramScroller.SetScrollPos(paramOffset);
		paramScroller.EnableWindow(scrollMax > 0 ? TRUE : FALSE);
		updateParamNames = true;
	}

	m_VstPlugin.CacheParameterNames(paramOffset, std::min(paramOffset + NUM_PLUGINEDITOR_PARAMETERS, numParams));
	for(PlugParamIndex i = 0; i < NUM_PLUGINEDITOR_PARAMETERS; i++)
	{
		const PlugParamIndex param = paramOffset + i;

		if(param >= numParams)
		{
			// This param doesn't exist.
			controls[i]->EnableControls(false);
			controls[i]->ResetContent();
			continue;
		}

		controls[i]->EnableControls();

		if(updateParamNames)
		{
			// Update param name
			controls[i]->SetParamName(m_VstPlugin.GetFormattedParamName(param));
		}

		UpdateParamDisplay(param);
	}
}


void CDefaultVstEditor::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//--------------------------------------------------------------------------------
{
	CSliderCtrl* pScrolledSlider = reinterpret_cast<CSliderCtrl*>(pScrollBar);
	// Check if any of the value sliders were affected.
	for(size_t i = 0; i < controls.size(); i++)
	{
		if ((pScrolledSlider->GetDlgCtrlID() == controls[i]->GetSliderID()) && (nSBCode != SB_ENDSCROLL))
		{
			OnParamSliderChanged(controls[i]->GetSliderID());
			break;
		}
	}

	CAbstractVstEditor::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CDefaultVstEditor::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//--------------------------------------------------------------------------------
{
	if (pScrollBar == &paramScroller)
	{
		// Get the minimum and maximum scrollbar positions.
		int minpos;
		int maxpos;
		pScrollBar->GetScrollRange(&minpos, &maxpos); 
		//maxpos = pScrollBar->GetScrollLimit();

		SCROLLINFO sbInfo;
		paramScroller.GetScrollInfo(&sbInfo);

		// Get the current position of scroll box.
		int curpos = pScrollBar->GetScrollPos();

		// Determine the new position of scroll box.
		switch(nSBCode)
		{
		case SB_LEFT:			// Scroll to far left.
			curpos = minpos;
			break;

		case SB_RIGHT:			// Scroll to far right.
			curpos = maxpos;
			break;

		case SB_ENDSCROLL:		// End scroll.
			break;

		case SB_LINELEFT:		// Scroll left.
			if(curpos > minpos)
				curpos--;
			break;

		case SB_LINERIGHT:		// Scroll right.
			if(curpos < maxpos)
				curpos++;
			break;

		case SB_PAGELEFT:		// Scroll one page left.
			if(curpos > minpos)
			{
				curpos = MAX(minpos, curpos - (int)sbInfo.nPage);
			}
			break;

		case SB_PAGERIGHT:		// Scroll one page right.
			if(curpos < maxpos)
			{
				curpos = MIN(maxpos, curpos + (int)sbInfo.nPage);
			}
			break;

		case SB_THUMBPOSITION:	// Scroll to absolute position. nPos is the position
			curpos = nPos;		// of the scroll box at the end of the drag operation.
			break;

		case SB_THUMBTRACK:		// Drag scroll box to specified position. nPos is the
			curpos = nPos;		// position that the scroll box has been dragged to.
			break;
		}

		// Set the new position of the thumb (scroll box).
		pScrollBar->SetScrollPos(curpos);

		paramOffset = curpos;
		UpdateControls(true);
	}

	CAbstractVstEditor::OnVScroll(nSBCode, nPos, pScrollBar);
}


BOOL CDefaultVstEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
//------------------------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(nFlags);
	MPT_UNREFERENCED_PARAMETER(pt);

	// Mouse wheel - scroll parameter list
	int minpos, maxpos;
	paramScroller.GetScrollRange(&minpos, &maxpos);
	if(minpos != maxpos)
	{
		paramOffset -= sgn(zDelta);
		Limit(paramOffset, PlugParamIndex(minpos), PlugParamIndex(maxpos));
		paramScroller.SetScrollPos(paramOffset);

		UpdateControls(true);
	}

	return CAbstractVstEditor::OnMouseWheel(nFlags, zDelta, pt);
}


bool CDefaultVstEditor::OpenEditor(CWnd *parent)
//----------------------------------------------
{
	Create(IDD_DEFAULTPLUGINEDITOR, parent);
	CreateControls();
	return CAbstractVstEditor::OpenEditor(parent);
}


// Called when a change occurs to the parameter textbox
// If the change is triggered by the user, we'll need to notify the plugin and update 
// the other GUI controls 
void CDefaultVstEditor::OnParamTextboxChanged(UINT id)
//----------------------------------------------------
{
	if (m_nControlLock)
	{
		// Lock will be set if the GUI change was triggered internally (in UpdateParamDisplays).
		// We're only interested in handling changes triggered by the user.
		return;
	}

	const PlugParamIndex param = paramOffset + id - ID_PLUGINEDITOR_EDIT_BASE;

	// Extract value and update
	SetParam(param, controls[param - paramOffset]->GetParamValueFromEdit());
}


// Called when a change occurs to the parameter slider
// If the change is triggered by the user, we'll need to notify the plugin and update 
// the other GUI controls 
void CDefaultVstEditor::OnParamSliderChanged(UINT id)
//---------------------------------------------------
{
	if (m_nControlLock)
	{
		// Lock will be set if the GUI change was triggered internally (in UpdateParamDisplays).
		// We're only interested in handling changes triggered by the user.
		return;
	}

	const PlugParamIndex param = paramOffset + id - ID_PLUGINEDITOR_SLIDERS_BASE;

	// Extract value and update
	SetParam(param, controls[param - paramOffset]->GetParamValueFromSlider());

}


// Update a given parameter to a given value and notify plugin
void CDefaultVstEditor::SetParam(PlugParamIndex param, int value)
//---------------------------------------------------------------
{
	if(param >= m_VstPlugin.GetNumParameters())
	{
		return;
	}

	m_VstPlugin.SetParameter(param, static_cast<PlugParamValue>(value) / static_cast<PlugParamValue>(PARAM_RESOLUTION));

	// Update other GUI controls
	UpdateParamDisplay(param);

	// Act as if an automation message has been sent by the plugin (record param changes, set document modified, etc...)
	m_VstPlugin.AutomateParameter(param);

}


//Update all GUI controls with the new param value
void CDefaultVstEditor::UpdateParamDisplay(PlugParamIndex param)
//--------------------------------------------------------------
{
	if(m_nControlLock || param < paramOffset || param >= paramOffset + NUM_PLUGINEDITOR_PARAMETERS)
	{
		//Just to make sure we're not here as a consequence of an internal GUI change, and avoid modifying a parameter that doesn't exist on the GUI.
		return;
	}

	// Get the actual parameter value from the plugin
	const int val = static_cast<int>(m_VstPlugin.GetParameter(param) * static_cast<float>(PARAM_RESOLUTION) + 0.5f);

	// Update the GUI controls

	// Set lock to indicate that the changes to the GUI are internal - no need to notify the plug and re-update GUI.
	m_nControlLock++;

	controls[param - paramOffset]->SetParamValue(val, m_VstPlugin.GetFormattedParamValue(param));

	// Unset lock - done with internal GUI updates.
	m_nControlLock--;
	
}

#endif // NO_PLUGINS


OPENMPT_NAMESPACE_END
