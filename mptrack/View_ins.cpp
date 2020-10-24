/*
 * view_ins.cpp
 * ------------
 * Purpose: Instrument tab, lower panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "ImageLists.h"
#include "Childfrm.h"
#include "Moddoc.h"
#include "Globals.h"
#include "Ctrl_ins.h"
#include "View_ins.h"
#include "Dlsbank.h"
#include "ChannelManagerDlg.h"
#include "ScaleEnvPointsDlg.h"
#include "../soundlib/MIDIEvents.h"
#include "../soundlib/mod_specifications.h"
#include "../common/mptStringBuffer.h"
#include "FileDialog.h"


OPENMPT_NAMESPACE_BEGIN

namespace
{
	const int ENV_POINT_SIZE = 4;
	const float ENV_MIN_ZOOM = 2.0f;
	const float ENV_MAX_ZOOM = 256.0f;
}


// Non-client toolbar
#define ENV_LEFTBAR_CY			Util::ScalePixels(29, m_hWnd)
#define ENV_LEFTBAR_CXSEP		Util::ScalePixels(14, m_hWnd)
#define ENV_LEFTBAR_CXSPC		Util::ScalePixels(3, m_hWnd)
#define ENV_LEFTBAR_CXBTN		Util::ScalePixels(24, m_hWnd)
#define ENV_LEFTBAR_CYBTN		Util::ScalePixels(22, m_hWnd)


static constexpr UINT cLeftBarButtons[ENV_LEFTBAR_BUTTONS] =
{
	ID_ENVSEL_VOLUME,
	ID_ENVSEL_PANNING,
	ID_ENVSEL_PITCH,
		ID_SEPARATOR,
	ID_ENVELOPE_VOLUME,
	ID_ENVELOPE_PANNING,
	ID_ENVELOPE_PITCH,
	ID_ENVELOPE_FILTER,
		ID_SEPARATOR,
	ID_ENVELOPE_SETLOOP,
	ID_ENVELOPE_SUSTAIN,
	ID_ENVELOPE_CARRY,
		ID_SEPARATOR,
	ID_INSTRUMENT_SAMPLEMAP,
		ID_SEPARATOR,
	ID_ENVELOPE_VIEWGRID,
		ID_SEPARATOR,
	ID_ENVELOPE_ZOOM_IN,
	ID_ENVELOPE_ZOOM_OUT,
		ID_SEPARATOR,
	ID_ENVELOPE_LOAD,
	ID_ENVELOPE_SAVE,
};


IMPLEMENT_SERIAL(CViewInstrument, CModScrollView, 0)

BEGIN_MESSAGE_MAP(CViewInstrument, CModScrollView)
	//{{AFX_MSG_MAP(CViewInstrument)
	ON_MESSAGE(WM_DPICHANGED, &CViewInstrument::OnDPIChanged)
	ON_WM_ERASEBKGND()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_NCHITTEST()
	ON_WM_MOUSEMOVE()
	ON_WM_NCMOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_XBUTTONUP()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_NCLBUTTONUP()
	ON_WM_NCLBUTTONDBLCLK()
	ON_WM_DROPFILES()
	ON_COMMAND(ID_PREVINSTRUMENT,			&CViewInstrument::OnPrevInstrument)
	ON_COMMAND(ID_NEXTINSTRUMENT,			&CViewInstrument::OnNextInstrument)
	ON_COMMAND(ID_ENVELOPE_SETLOOP,			&CViewInstrument::OnEnvLoopChanged)
	ON_COMMAND(ID_ENVELOPE_SUSTAIN,			&CViewInstrument::OnEnvSustainChanged)
	ON_COMMAND(ID_ENVELOPE_CARRY,			&CViewInstrument::OnEnvCarryChanged)
	ON_COMMAND(ID_ENVELOPE_INSERTPOINT,		&CViewInstrument::OnEnvInsertPoint)
	ON_COMMAND(ID_ENVELOPE_REMOVEPOINT,		&CViewInstrument::OnEnvRemovePoint)
	ON_COMMAND(ID_ENVELOPE_VOLUME,			&CViewInstrument::OnEnvVolChanged)
	ON_COMMAND(ID_ENVELOPE_PANNING,			&CViewInstrument::OnEnvPanChanged)
	ON_COMMAND(ID_ENVELOPE_PITCH,			&CViewInstrument::OnEnvPitchChanged)
	ON_COMMAND(ID_ENVELOPE_FILTER,			&CViewInstrument::OnEnvFilterChanged)
	ON_COMMAND(ID_ENVELOPE_VIEWGRID,		&CViewInstrument::OnEnvToggleGrid)
	ON_COMMAND(ID_ENVELOPE_ZOOM_IN,			&CViewInstrument::OnEnvZoomIn)
	ON_COMMAND(ID_ENVELOPE_ZOOM_OUT,		&CViewInstrument::OnEnvZoomOut)
	ON_COMMAND(ID_ENVELOPE_LOAD,			&CViewInstrument::OnEnvLoad)
	ON_COMMAND(ID_ENVELOPE_SAVE,			&CViewInstrument::OnEnvSave)
	ON_COMMAND(ID_ENVSEL_VOLUME,			&CViewInstrument::OnSelectVolumeEnv)
	ON_COMMAND(ID_ENVSEL_PANNING,			&CViewInstrument::OnSelectPanningEnv)
	ON_COMMAND(ID_ENVSEL_PITCH,				&CViewInstrument::OnSelectPitchEnv)
	ON_COMMAND(ID_EDIT_COPY,				&CViewInstrument::OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE,				&CViewInstrument::OnEditPaste)
	ON_COMMAND(ID_EDIT_UNDO,				&CViewInstrument::OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO,				&CViewInstrument::OnEditRedo)
	ON_COMMAND(ID_INSTRUMENT_SAMPLEMAP,		&CViewInstrument::OnEditSampleMap)
	ON_COMMAND(ID_ENVELOPE_TOGGLERELEASENODE, &CViewInstrument::OnEnvToggleReleasNode)
	ON_COMMAND(ID_ENVELOPE_SCALEPOINTS, &CViewInstrument::OnEnvelopeScalePoints)

	ON_MESSAGE(WM_MOD_MIDIMSG,				&CViewInstrument::OnMidiMsg)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,			&CViewInstrument::OnCustomKeyMsg)

	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO,		&CViewInstrument::OnUpdateUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO,		&CViewInstrument::OnUpdateRedo)

	//}}AFX_MSG_MAP
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


///////////////////////////////////////////////////////////////
// CViewInstrument operations

CViewInstrument::CViewInstrument()
{
	EnableActiveAccessibility();
	m_rcClient.bottom = 2;
	m_dwNotifyPos.fill(uint32(Notification::PosInvalid));
	MemsetZero(m_NcButtonState);

	m_bmpEnvBar.Create(&CMainFrame::GetMainFrame()->m_EnvelopeIcons);

	m_baPlayingNote.reset();
}


void CViewInstrument::OnInitialUpdate()
{
	CModScrollView::OnInitialUpdate();
	ModifyStyleEx(0, WS_EX_ACCEPTFILES);
	m_zoom = (ENV_POINT_SIZE * m_nDPIx) / 96.0f;
	m_envPointSize = Util::ScalePixels(ENV_POINT_SIZE, m_hWnd);
	UpdateScrollSize();
	UpdateNcButtonState();
	EnableToolTips();
}


void CViewInstrument::UpdateScrollSize()
{
	CModDoc *pModDoc = GetDocument();
	GetClientRect(&m_rcClient);
	if(m_rcClient.bottom < 2)
		m_rcClient.bottom = 2;
	if(pModDoc)
	{
		SIZE sizeTotal, sizePage, sizeLine;
		uint32 maxTick = EnvGetTick(EnvGetLastPoint());

		sizeTotal.cx = mpt::saturate_round<int>((maxTick + 2) * m_zoom);
		sizeTotal.cy = 1;
		sizeLine.cx = mpt::saturate_round<int>(m_zoom);
		sizeLine.cy = 2;
		sizePage.cx = sizeLine.cx * 4;
		sizePage.cy = sizeLine.cy;
		SetScrollSizes(MM_TEXT, sizeTotal, sizePage, sizeLine);
	}
}


LRESULT CViewInstrument::OnDPIChanged(WPARAM wParam, LPARAM lParam)
{
	LRESULT res = CModScrollView::OnDPIChanged(wParam, lParam);
	m_envPointSize = Util::ScalePixels(4, m_hWnd);
	return res;
}


void CViewInstrument::PrepareUndo(const char *description)
{
	GetDocument()->GetInstrumentUndo().PrepareUndo(m_nInstrument, description, m_nEnv);
}


// Set instrument (and moddoc) as modified.
// updateAll: Update all views including this one. Otherwise, only update update other views.
void CViewInstrument::SetModified(InstrumentHint hint, bool updateAll)
{
	CModDoc *pModDoc = GetDocument();
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(nullptr, hint.SetData(m_nInstrument), updateAll ? nullptr : this);
	CMainFrame::GetMainFrame()->NotifyAccessibilityUpdate(*this);
}


BOOL CViewInstrument::SetCurrentInstrument(INSTRUMENTINDEX nIns, EnvelopeType nEnv)
{
	CModDoc *pModDoc = GetDocument();
	Notification::Type type;

	if((!pModDoc) || (nIns < 1) || (nIns >= MAX_INSTRUMENTS))
		return FALSE;
	m_nEnv = nEnv;
	m_nInstrument = nIns;
	switch(m_nEnv)
	{
	case ENV_PANNING:	type = Notification::PanEnv; break;
	case ENV_PITCH:		type = Notification::PitchEnv; break;
	default:			m_nEnv = ENV_VOLUME; type = Notification::VolEnv; break;
	}
	pModDoc->SetNotifications(type, m_nInstrument);
	pModDoc->SetFollowWnd(m_hWnd);
	UpdateScrollSize();
	UpdateNcButtonState();
	InvalidateRect(NULL, FALSE);
	return TRUE;
}


void CViewInstrument::OnSetFocus(CWnd *pOldWnd)
{
	CScrollView::OnSetFocus(pOldWnd);
	SetCurrentInstrument(m_nInstrument, m_nEnv);
}


LRESULT CViewInstrument::OnModViewMsg(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
	case VIEWMSG_SETCURRENTINSTRUMENT:
		SetCurrentInstrument(lParam & 0xFFFF, m_nEnv);
		break;

	case VIEWMSG_LOADSTATE:
		if(lParam)
		{
			INSTRUMENTVIEWSTATE *pState = (INSTRUMENTVIEWSTATE *)lParam;
			if(pState->initialized)
			{
				SetCurrentInstrument(m_nInstrument, pState->nEnv);
				m_bGrid = pState->bGrid;
			}
		}
		break;

	case VIEWMSG_SAVESTATE:
		if(lParam)
		{
			INSTRUMENTVIEWSTATE *pState = (INSTRUMENTVIEWSTATE *)lParam;
			pState->initialized = true;
			pState->nEnv = m_nEnv;
			pState->bGrid = m_bGrid;
		}
		break;

	default:
		return CModScrollView::OnModViewMsg(wParam, lParam);
	}
	return 0;
}


uint32 CViewInstrument::EnvGetTick(int nPoint) const
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return 0;
	if((nPoint >= 0) && (nPoint < (int)envelope->size()))
		return envelope->at(nPoint).tick;
	else
		return 0;
}


uint32 CViewInstrument::EnvGetValue(int nPoint) const
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return 0;
	if(nPoint >= 0 && nPoint < (int)envelope->size())
		return envelope->at(nPoint).value;
	else
		return 0;
}


bool CViewInstrument::EnvSetValue(int nPoint, int32 nTick, int32 nValue, bool moveTail)
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr || nPoint < 0)
		return false;

	if(nPoint == 0)
	{
		nTick = 0;
		moveTail = false;
	}
	int tickDiff = 0;

	bool ok = false;
	if(nPoint < (int)envelope->size())
	{
		if(nTick != int32_min)
		{
			nTick = std::max(0, nTick);
			tickDiff = envelope->at(nPoint).tick;
			int mintick = (nPoint > 0) ? envelope->at(nPoint - 1).tick : 0;
			int maxtick;
			if(nPoint + 1 >= (int)envelope->size() || moveTail)
				maxtick = Util::MaxValueOfType(maxtick);
			else
				maxtick = envelope->at(nPoint + 1).tick;

			// Can't have multiple points on same tick
			if(nPoint > 0 && mintick < maxtick - 1)
			{
				mintick++;
				if(nPoint + 1 < (int)envelope->size())
					maxtick--;
			}
			if(nTick < mintick)
				nTick = mintick;
			if(nTick > maxtick)
				nTick = maxtick;
			if(nTick != envelope->at(nPoint).tick)
			{
				envelope->at(nPoint).tick = static_cast<EnvelopeNode::tick_t>(nTick);
				ok = true;
			}
		}
		const int maxVal = (GetDocument()->GetModType() != MOD_TYPE_XM || m_nEnv != ENV_PANNING) ? 64 : 63;
		if(nValue != int32_min)
		{
			Limit(nValue, 0, maxVal);
			if(nValue != envelope->at(nPoint).value)
			{
				envelope->at(nPoint).value = static_cast<EnvelopeNode::value_t>(nValue);
				ok = true;
			}
		}
	}

	if(ok && moveTail)
	{
		// Move all points after modified point as well.
		tickDiff = envelope->at(nPoint).tick - tickDiff;
		for(auto it = envelope->begin() + nPoint + 1; it != envelope->end(); it++)
		{
			it->tick = static_cast<EnvelopeNode::tick_t>(std::max(0, (int)it->tick + tickDiff));
		}
	}

	return ok;
}


uint32 CViewInstrument::EnvGetNumPoints() const
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return 0;
	return envelope->size();
}


uint32 CViewInstrument::EnvGetLastPoint() const
{
	uint32 nPoints = EnvGetNumPoints();
	if(nPoints > 0)
		return nPoints - 1;
	return 0;
}


// Return if an envelope flag is set.
bool CViewInstrument::EnvGetFlag(const EnvelopeFlags dwFlag) const
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv != nullptr)
		return pEnv->dwFlags[dwFlag];
	return false;
}


uint32 CViewInstrument::EnvGetLoopStart() const
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return 0;
	return envelope->nLoopStart;
}


uint32 CViewInstrument::EnvGetLoopEnd() const
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return 0;
	return envelope->nLoopEnd;
}


uint32 CViewInstrument::EnvGetSustainStart() const
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return 0;
	return envelope->nSustainStart;
}


uint32 CViewInstrument::EnvGetSustainEnd() const
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return 0;
	return envelope->nSustainEnd;
}


bool CViewInstrument::EnvGetVolEnv() const
{
	ModInstrument *pIns = GetInstrumentPtr();
	if(pIns)
		return pIns->VolEnv.dwFlags[ENV_ENABLED] != 0;
	return false;
}


bool CViewInstrument::EnvGetPanEnv() const
{
	ModInstrument *pIns = GetInstrumentPtr();
	if(pIns)
		return pIns->PanEnv.dwFlags[ENV_ENABLED] != 0;
	return false;
}


bool CViewInstrument::EnvGetPitchEnv() const
{
	ModInstrument *pIns = GetInstrumentPtr();
	if(pIns)
		return ((pIns->PitchEnv.dwFlags & (ENV_ENABLED | ENV_FILTER)) == ENV_ENABLED);
	return false;
}


bool CViewInstrument::EnvGetFilterEnv() const
{
	ModInstrument *pIns = GetInstrumentPtr();
	if(pIns)
		return ((pIns->PitchEnv.dwFlags & (ENV_ENABLED | ENV_FILTER)) == (ENV_ENABLED | ENV_FILTER));
	return false;
}


bool CViewInstrument::EnvSetLoopStart(int nPoint)
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return false;
	if(nPoint < 0 || nPoint > (int)EnvGetLastPoint())
		return false;

	if(nPoint != envelope->nLoopStart)
	{
		envelope->nLoopStart = static_cast<decltype(envelope->nLoopStart)>(nPoint);
		if(envelope->nLoopEnd < nPoint)
			envelope->nLoopEnd = static_cast<decltype(envelope->nLoopEnd)>(nPoint);
		return true;
	} else
	{
		return false;
	}
}


bool CViewInstrument::EnvSetLoopEnd(int nPoint)
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return false;
	if(nPoint < 0 || nPoint > (int)EnvGetLastPoint())
		return false;

	if(nPoint != envelope->nLoopEnd)
	{
		envelope->nLoopEnd = static_cast<decltype(envelope->nLoopEnd)>(nPoint);
		if(envelope->nLoopStart > nPoint)
			envelope->nLoopStart = static_cast<decltype(envelope->nLoopStart)>(nPoint);
		return true;
	} else
	{
		return false;
	}
}


bool CViewInstrument::EnvSetSustainStart(int nPoint)
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return false;
	if(nPoint < 0 || nPoint > (int)EnvGetLastPoint())
		return false;

	// We won't do any security checks here as GetEnvelopePtr() does that for us.
	CSoundFile &sndFile = GetDocument()->GetSoundFile();

	if(nPoint != envelope->nSustainStart)
	{
		envelope->nSustainStart = static_cast<decltype(envelope->nSustainStart)>(nPoint);
		if((envelope->nSustainEnd < nPoint) || (sndFile.GetType() & MOD_TYPE_XM))
			envelope->nSustainEnd = static_cast<decltype(envelope->nSustainEnd)>(nPoint);
		return true;
	} else
	{
		return false;
	}
}


bool CViewInstrument::EnvSetSustainEnd(int nPoint)
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return false;
	if(nPoint < 0 || nPoint > (int)EnvGetLastPoint())
		return false;

	// We won't do any security checks here as GetEnvelopePtr() does that for us.
	CSoundFile &sndFile = GetDocument()->GetSoundFile();

	if(nPoint != envelope->nSustainEnd)
	{
		envelope->nSustainEnd = static_cast<decltype(envelope->nSustainEnd)>(nPoint);
		if((envelope->nSustainStart > nPoint) || (sndFile.GetType() & MOD_TYPE_XM))
			envelope->nSustainStart = static_cast<decltype(envelope->nSustainStart)>(nPoint);
		return true;
	} else
	{
		return false;
	}
}


bool CViewInstrument::EnvToggleReleaseNode(int nPoint)
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return false;
	if(nPoint < 0 || nPoint >= (int)EnvGetNumPoints())
		return false;

	// Don't allow release nodes in IT/XM. GetDocument()/... nullptr check is done in GetEnvelopePtr, so no need to check twice.
	if(!GetDocument()->GetSoundFile().GetModSpecifications().hasReleaseNode)
	{
		if(envelope->nReleaseNode != ENV_RELEASE_NODE_UNSET)
		{
			envelope->nReleaseNode = ENV_RELEASE_NODE_UNSET;
			return true;
		}
		return false;
	}

	if(envelope->nReleaseNode == nPoint)
	{
		envelope->nReleaseNode = ENV_RELEASE_NODE_UNSET;
	} else
	{
		envelope->nReleaseNode = static_cast<decltype(envelope->nReleaseNode)>(nPoint);
	}
	return true;
}

// Enable or disable a flag of the current envelope
bool CViewInstrument::EnvSetFlag(EnvelopeFlags flag, bool enable)
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr || envelope->empty())
		return false;

	bool modified = envelope->dwFlags[flag] != enable;
	PrepareUndo("Toggle Envelope Flag");
	envelope->dwFlags.set(flag, enable);
	return modified;
}


bool CViewInstrument::EnvToggleEnv(EnvelopeType envelope, CSoundFile &sndFile, ModInstrument &ins, bool enable, EnvelopeNode::value_t defaultValue, EnvelopeFlags extraFlags)
{
	InstrumentEnvelope &env = ins.GetEnvelope(envelope);

	const FlagSet<EnvelopeFlags> flags = (ENV_ENABLED | extraFlags);

	env.dwFlags.set(flags, enable);
	if(enable && env.empty())
	{
		env.reserve(2);
		env.push_back(EnvelopeNode(0, defaultValue));
		env.push_back(EnvelopeNode(10, defaultValue));
		InvalidateRect(NULL, FALSE);
	}

	CriticalSection cs;

	// Update mixing flags...
	for(auto &chn : sndFile.m_PlayState.Chn)
	{
		if(chn.pModInstrument == &ins)
		{
			chn.GetEnvelope(envelope).flags.set(flags, enable);
		}
	}

	return true;
}


bool CViewInstrument::EnvSetVolEnv(bool enable)
{
	ModInstrument *pIns = GetInstrumentPtr();
	if(pIns == nullptr)
		return false;
	return EnvToggleEnv(ENV_VOLUME, GetDocument()->GetSoundFile(), *pIns, enable, 64);
}


bool CViewInstrument::EnvSetPanEnv(bool enable)
{
	ModInstrument *pIns = GetInstrumentPtr();
	if(pIns == nullptr)
		return false;
	return EnvToggleEnv(ENV_PANNING, GetDocument()->GetSoundFile(), *pIns, enable, 32);
}


bool CViewInstrument::EnvSetPitchEnv(bool enable)
{
	ModInstrument *pIns = GetInstrumentPtr();
	if(pIns == nullptr)
		return false;

	pIns->PitchEnv.dwFlags.reset(ENV_FILTER);
	return EnvToggleEnv(ENV_PITCH, GetDocument()->GetSoundFile(), *pIns, enable, 32);
}


bool CViewInstrument::EnvSetFilterEnv(bool enable)
{
	ModInstrument *pIns = GetInstrumentPtr();
	if(pIns == nullptr)
		return false;

	return EnvToggleEnv(ENV_PITCH, GetDocument()->GetSoundFile(), *pIns, enable, 64, ENV_FILTER);
}


uint32 CViewInstrument::DragItemToEnvPoint() const
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr || !m_nDragItem)
		return 0;

	switch(m_nDragItem)
	{
	case ENV_DRAGLOOPSTART: return pEnv->nLoopStart;
	case ENV_DRAGLOOPEND: return pEnv->nLoopEnd;
	case ENV_DRAGSUSTAINSTART: return pEnv->nSustainStart;
	case ENV_DRAGSUSTAINEND: return pEnv->nSustainEnd;
	default: return m_nDragItem - 1;
	}
}


int CViewInstrument::TickToScreen(int tick) const
{
	return static_cast<int>((tick * m_zoom) - m_nScrollPosX + m_envPointSize);
}

int CViewInstrument::PointToScreen(int nPoint) const
{
	return TickToScreen(EnvGetTick(nPoint));
}


int CViewInstrument::ScreenToTick(int x) const
{
	int offset = m_nScrollPosX + x;
	if(offset < m_envPointSize)
		return 0;
	return mpt::saturate_round<int>((offset - m_envPointSize) / m_zoom);
}


int CViewInstrument::ScreenToValue(int y) const
{
	if(m_rcClient.bottom < 2)
		return ENVELOPE_MIN;
	int n = ENVELOPE_MAX - Util::muldivr(y, ENVELOPE_MAX, m_rcClient.bottom - 1);
	if(n < ENVELOPE_MIN)
		return ENVELOPE_MIN;
	if(n > ENVELOPE_MAX)
		return ENVELOPE_MAX;
	return n;
}


int CViewInstrument::ScreenToPoint(int x0, int y0) const
{
	int nPoint = -1;
	int64 ydist = int64_max, xdist = int64_max;
	int numPoints = EnvGetNumPoints();
	for(int i = 0; i < numPoints; i++)
	{
		int dx = x0 - PointToScreen(i);
		int64 dx2 = Util::mul32to64(dx, dx);
		if(dx2 <= xdist)
		{
			int dy = y0 - ValueToScreen(EnvGetValue(i));
			int64 dy2 = Util::mul32to64(dy, dy);
			if(dx2 < xdist || (dx2 == xdist && dy2 < ydist))
			{
				nPoint = i;
				xdist = dx2;
				ydist = dy2;
			}
		}
	}
	return nPoint;
}


bool CViewInstrument::GetNcButtonRect(UINT button, CRect &rect) const
{
	rect.left = 4;
	rect.top = 3;
	rect.bottom = rect.top + ENV_LEFTBAR_CYBTN;
	if(button >= ENV_LEFTBAR_BUTTONS)
		return false;
	for(UINT i = 0; i < button; i++)
	{
		if(cLeftBarButtons[i] == ID_SEPARATOR)
			rect.left += ENV_LEFTBAR_CXSEP;
		else
			rect.left += ENV_LEFTBAR_CXBTN + ENV_LEFTBAR_CXSPC;
	}
	if(cLeftBarButtons[button] == ID_SEPARATOR)
	{
		rect.left += ENV_LEFTBAR_CXSEP / 2 - 2;
		rect.right = rect.left + 2;
		return false;
	} else
	{
		rect.right = rect.left + ENV_LEFTBAR_CXBTN;
	}
	return true;
}


UINT CViewInstrument::GetNcButtonAtPoint(CPoint point, CRect *outRect) const
{
	CRect rect, rcWnd;
	UINT button = uint32_max;
	GetWindowRect(&rcWnd);
	for(UINT i = 0; i < ENV_LEFTBAR_BUTTONS; i++)
	{
		if(!(m_NcButtonState[i] & NCBTNS_DISABLED) && GetNcButtonRect(i, rect))
		{
			rect.OffsetRect(rcWnd.left, rcWnd.top);
			if(rect.PtInRect(point))
			{
				button = i;
				break;
			}
		}
	}
	if(outRect)
		*outRect = rect;
	return button;
}


void CViewInstrument::UpdateNcButtonState()
{
	CModDoc *pModDoc = GetDocument();
	if(!pModDoc)
		return;
	CSoundFile &sndFile = pModDoc->GetSoundFile();

	CDC *pDC = NULL;
	for (UINT i=0; i<ENV_LEFTBAR_BUTTONS; i++) if (cLeftBarButtons[i] != ID_SEPARATOR)
	{
		DWORD dwStyle = 0;

		switch(cLeftBarButtons[i])
		{
		case ID_ENVSEL_VOLUME:		if (m_nEnv == ENV_VOLUME) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVSEL_PANNING:		if (m_nEnv == ENV_PANNING) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVSEL_PITCH:		if (!(sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))) dwStyle |= NCBTNS_DISABLED;
									else if (m_nEnv == ENV_PITCH) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_SETLOOP:	if (EnvGetLoop()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_SUSTAIN:	if (EnvGetSustain()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_CARRY:		if (!(sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))) dwStyle |= NCBTNS_DISABLED;
									else if (EnvGetCarry()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_VOLUME:	if (EnvGetVolEnv()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_PANNING:	if (EnvGetPanEnv()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_PITCH:		if (!(sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))) dwStyle |= NCBTNS_DISABLED; else
									if (EnvGetPitchEnv()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_FILTER:	if (!(sndFile.GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))) dwStyle |= NCBTNS_DISABLED; else
									if (EnvGetFilterEnv()) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_VIEWGRID:	if (m_bGrid) dwStyle |= NCBTNS_CHECKED; break;
		case ID_ENVELOPE_ZOOM_IN:	if (m_zoom >= ENV_MAX_ZOOM) dwStyle |= NCBTNS_DISABLED; break;
		case ID_ENVELOPE_ZOOM_OUT:	if (m_zoom <= ENV_MIN_ZOOM) dwStyle |= NCBTNS_DISABLED; break;
		case ID_ENVELOPE_LOAD:
		case ID_ENVELOPE_SAVE:		if (GetInstrumentPtr() == nullptr) dwStyle |= NCBTNS_DISABLED; break;
		}
		if (m_nBtnMouseOver == i)
		{
			dwStyle |= NCBTNS_MOUSEOVER;
			if (m_dwStatus & INSSTATUS_NCLBTNDOWN) dwStyle |= NCBTNS_PUSHED;
		}
		if (dwStyle != m_NcButtonState[i])
		{
			m_NcButtonState[i] = dwStyle;
			if (!pDC) pDC = GetWindowDC();
			DrawNcButton(pDC, i);
		}
	}
	if (pDC) ReleaseDC(pDC);
}


////////////////////////////////////////////////////////////////////
// CViewInstrument drawing

void CViewInstrument::UpdateView(UpdateHint hint, CObject *pObj)
{
	if(pObj == this)
	{
		return;
	}
	const InstrumentHint instrHint = hint.ToType<InstrumentHint>();
	FlagSet<HintType> hintType = instrHint.GetType();
	const INSTRUMENTINDEX updateIns = instrHint.GetInstrument();
	if(hintType[HINT_MPTOPTIONS | HINT_MODTYPE]
		|| (hintType[HINT_ENVELOPE] && (m_nInstrument == updateIns || updateIns == 0)))
	{
		UpdateScrollSize();
		UpdateNcButtonState();
		InvalidateRect(NULL, FALSE);
	}
}


void CViewInstrument::DrawGrid(CDC *pDC, uint32 speed)
{
	bool windowResized = false;

	if(m_dcGrid.m_hDC)
	{
		m_dcGrid.SelectObject(m_pbmpOldGrid);
		m_dcGrid.DeleteDC();
		m_bmpGrid.DeleteObject();
		windowResized = true;
	}

	if(windowResized || m_bGridForceRedraw || (m_nScrollPosX != m_GridScrollPos) || (speed != (UINT)m_GridSpeed) && speed > 0)
	{
		m_GridSpeed = speed;
		m_GridScrollPos = m_nScrollPosX;
		m_bGridForceRedraw = false;

		// create a memory based dc for drawing the grid
		m_dcGrid.CreateCompatibleDC(pDC);
		m_bmpGrid.CreateCompatibleBitmap(pDC, m_rcClient.Width(), m_rcClient.Height());
		m_pbmpOldGrid = *m_dcGrid.SelectObject(&m_bmpGrid);

		// Do draw
		const int width = m_rcClient.Width();
		int rowsPerBeat = 1, rowsPerMeasure = 1;
		const CModDoc *modDoc = GetDocument();
		if(modDoc != nullptr)
		{
			rowsPerBeat = modDoc->GetSoundFile().m_nDefaultRowsPerBeat;
			rowsPerMeasure = modDoc->GetSoundFile().m_nDefaultRowsPerMeasure;
		}

		// Paint it black!
		m_dcGrid.FillSolidRect(&m_rcClient, TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BACKENV]);

		const uint32 startTick = (ScreenToTick(0) / speed) * speed;
		const uint32 endTick = (ScreenToTick(width) / speed) * speed;

		auto oldPen = m_dcGrid.SelectStockObject(DC_PEN);
		for(uint32 tick = startTick, row = startTick / speed; tick <= endTick; tick += speed, row++)
		{
			if(rowsPerMeasure > 0 && row % rowsPerMeasure == 0)
				m_dcGrid.SetDCPenColor(RGB(0x80, 0x80, 0x80));
			else if(rowsPerBeat > 0 && row % rowsPerBeat == 0)
				m_dcGrid.SetDCPenColor(RGB(0x55, 0x55, 0x55));
			else
				m_dcGrid.SetDCPenColor(RGB(0x33, 0x33, 0x33));

			int x = TickToScreen(tick);
			m_dcGrid.MoveTo(x, 0);
			m_dcGrid.LineTo(x, m_rcClient.bottom);
		}
		if(oldPen)
			m_dcGrid.SelectObject(oldPen);
	}

	pDC->BitBlt(m_rcClient.left, m_rcClient.top, m_rcClient.Width(), m_rcClient.Height(), &m_dcGrid, 0, 0, SRCCOPY);
}


void CViewInstrument::OnDraw(CDC *pDC)
{
	CModDoc *pModDoc = GetDocument();
	if((!pModDoc) || (!pDC))
		return;

	// to avoid flicker, establish a memory dc, draw to it
	// and then BitBlt it to the destination "pDC"

	//check for window resize
	if(m_dcMemMain.GetSafeHdc() && m_rcOldClient != m_rcClient)
	{
		m_dcMemMain.SelectObject(oldBitmap);
		m_dcMemMain.DeleteDC();
		m_bmpMemMain.DeleteObject();
	}

	if(!m_dcMemMain.m_hDC)
	{
		m_dcMemMain.CreateCompatibleDC(pDC);
		if(!m_dcMemMain.m_hDC)
			return;
	}
	if(!m_bmpMemMain.m_hObject)
	{
		m_bmpMemMain.CreateCompatibleBitmap(pDC, m_rcClient.Width(), m_rcClient.Height());
	}
	m_rcOldClient = m_rcClient;
	oldBitmap = *m_dcMemMain.SelectObject(&m_bmpMemMain);

	auto stockBrush = CBrush::FromHandle(GetStockBrush(DC_BRUSH));
	if(m_bGrid)
	{
		DrawGrid(&m_dcMemMain, pModDoc->GetSoundFile().m_PlayState.m_nMusicSpeed);
	} else
	{
		// Paint it black!
		m_dcMemMain.FillSolidRect(&m_rcClient, TrackerSettings::Instance().rgbCustomColors[MODCOLOR_BACKENV]);
	}

	auto oldPen = m_dcMemMain.SelectObject(CMainFrame::penDarkGray);

	// Middle line (half volume or pitch / panning center)
	const int ymed = (m_rcClient.bottom - 1) / 2;
	m_dcMemMain.MoveTo(0, ymed);
	m_dcMemMain.LineTo(m_rcClient.right, ymed);

	// Drawing Loop Start/End
	if(EnvGetLoop())
	{
		m_dcMemMain.SelectObject(m_nDragItem == ENV_DRAGLOOPSTART ? CMainFrame::penGray99 : CMainFrame::penDarkGray);
		int x1 = PointToScreen(EnvGetLoopStart()) - m_envPointSize / 2;
		m_dcMemMain.MoveTo(x1, 0);
		m_dcMemMain.LineTo(x1, m_rcClient.bottom);
		m_dcMemMain.SelectObject(m_nDragItem == ENV_DRAGLOOPEND ? CMainFrame::penGray99 : CMainFrame::penDarkGray);
		int x2 = PointToScreen(EnvGetLoopEnd()) + m_envPointSize / 2;
		m_dcMemMain.MoveTo(x2, 0);
		m_dcMemMain.LineTo(x2, m_rcClient.bottom);
	}
	// Drawing Sustain Start/End
	if(EnvGetSustain())
	{
		m_dcMemMain.SelectObject(CMainFrame::penHalfDarkGray);
		int nspace = m_rcClient.bottom / 4;
		int n1 = EnvGetSustainStart();
		int x1 = PointToScreen(n1) - m_envPointSize / 2;
		int y1 = ValueToScreen(EnvGetValue(n1));
		m_dcMemMain.MoveTo(x1, y1 - nspace);
		m_dcMemMain.LineTo(x1, y1 + nspace);
		int n2 = EnvGetSustainEnd();
		int x2 = PointToScreen(n2) + m_envPointSize / 2;
		int y2 = ValueToScreen(EnvGetValue(n2));
		m_dcMemMain.MoveTo(x2, y2 - nspace);
		m_dcMemMain.LineTo(x2, y2 + nspace);
	}
	uint32 maxpoint = EnvGetNumPoints();
	// Drawing Envelope
	if(maxpoint)
	{
		maxpoint--;
		m_dcMemMain.SelectObject(GetStockObject(DC_PEN));
		m_dcMemMain.SetDCPenColor(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_ENVELOPES]);
		uint32 releaseNode = EnvGetReleaseNode();
		RECT rect;
		for(uint32 i = 0; i <= maxpoint; i++)
		{
			int x = PointToScreen(i);
			int y = ValueToScreen(EnvGetValue(i));
			rect.left = x - m_envPointSize + 1;
			rect.top = y - m_envPointSize + 1;
			rect.right = x + m_envPointSize;
			rect.bottom = y + m_envPointSize;
			if(i)
				m_dcMemMain.LineTo(x, y);
			else
				m_dcMemMain.MoveTo(x, y);

			if(i == releaseNode)
			{
				m_dcMemMain.SetDCBrushColor(RGB(0xFF, 0x00, 0x00));
				m_dcMemMain.FrameRect(&rect, stockBrush);
				m_dcMemMain.SetDCPenColor(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_ENVELOPE_RELEASE]);
			} else if(i == m_nDragItem - 1)
			{
				// currently selected env point
				m_dcMemMain.SetDCBrushColor(RGB(0xFF, 0xFF, 0x00));
				m_dcMemMain.FrameRect(&rect, stockBrush);
			} else
			{
				m_dcMemMain.SetDCBrushColor(RGB(0xFF, 0xFF, 0xFF));
				m_dcMemMain.FrameRect(&rect, stockBrush);
			}
		}
	}
	DrawPositionMarks();
	if(oldPen)
		m_dcMemMain.SelectObject(oldPen);

	pDC->BitBlt(m_rcClient.left, m_rcClient.top, m_rcClient.Width(), m_rcClient.Height(), &m_dcMemMain, 0, 0, SRCCOPY);
}


uint8 CViewInstrument::EnvGetReleaseNode()
{
	InstrumentEnvelope *envelope = GetEnvelopePtr();
	if(envelope == nullptr)
		return ENV_RELEASE_NODE_UNSET;
	return envelope->nReleaseNode;
}


bool CViewInstrument::EnvRemovePoint(uint32 nPoint)
{
	CModDoc *pModDoc = GetDocument();
	if((pModDoc) && (nPoint <= EnvGetLastPoint()))
	{
		ModInstrument *pIns = pModDoc->GetSoundFile().Instruments[m_nInstrument];
		if(pIns)
		{
			InstrumentEnvelope *envelope = GetEnvelopePtr();
			if(envelope == nullptr || envelope->empty())
				return false;

			PrepareUndo("Remove Envelope Point");
			envelope->erase(envelope->begin() + nPoint);
			if (nPoint >= envelope->size()) nPoint = envelope->size() - 1;
			if (envelope->nLoopStart > nPoint) envelope->nLoopStart--;
			if (envelope->nLoopEnd > nPoint) envelope->nLoopEnd--;
			if (envelope->nSustainStart > nPoint) envelope->nSustainStart--;
			if (envelope->nSustainEnd > nPoint) envelope->nSustainEnd--;
			if (envelope->nReleaseNode>nPoint && envelope->nReleaseNode != ENV_RELEASE_NODE_UNSET) envelope->nReleaseNode--;
			envelope->at(0).tick = 0;

			if(envelope->size() <= 1)
			{
				// if only one node is left, just disable the envelope completely
				*envelope = InstrumentEnvelope();
			}

			SetModified(InstrumentHint().Envelope(), true);
			return true;
		}
	}
	return false;
}


// Insert point. Returns 0 if error occurred, else point ID + 1.
uint32 CViewInstrument::EnvInsertPoint(int nTick, int nValue)
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc && nTick >= 0)
	{
		InstrumentEnvelope *envelope = GetEnvelopePtr();
		if(envelope != nullptr && envelope->size() < pModDoc->GetSoundFile().GetModSpecifications().envelopePointsMax)
		{
			nValue = Clamp(nValue, ENVELOPE_MIN, ENVELOPE_MAX);

			if(std::binary_search(envelope->cbegin(), envelope->cend(), EnvelopeNode(static_cast<EnvelopeNode::tick_t>(nTick), 0),
				[] (const EnvelopeNode &l, const EnvelopeNode &r) { return l.tick < r.tick; }))
			{
				// Don't want to insert a node at the same position as another node.
				return 0;
			}

			uint8 defaultValue;
			switch(m_nEnv)
			{
			case ENV_VOLUME:
				defaultValue = ENVELOPE_MAX;
				break;
			case ENV_PANNING:
				defaultValue = ENVELOPE_MID;
				break;
			case ENV_PITCH:
				defaultValue = envelope->dwFlags[ENV_FILTER] ? ENVELOPE_MAX : ENVELOPE_MID;
				break;
			default:
				return 0;
			}

			PrepareUndo("Insert Envelope Point");
			if(envelope->empty())
			{
				envelope->reserve(2);
				envelope->push_back(EnvelopeNode(0, defaultValue));
				envelope->dwFlags.set(ENV_ENABLED);
				if(nTick == 0)
				{
					// Can't insert two points on the same tick!
					nTick = 16;
				}
			}
			uint32 i = 0;
			for(i = 0; i < envelope->size(); i++) if(nTick <= envelope->at(i).tick) break;
			envelope->insert(envelope->begin() + i, EnvelopeNode(mpt::saturate_cast<EnvelopeNode::tick_t>(nTick), static_cast<EnvelopeNode::value_t>(nValue)));
			if(envelope->nLoopStart >= i) envelope->nLoopStart++;
			if(envelope->nLoopEnd >= i) envelope->nLoopEnd++;
			if(envelope->nSustainStart >= i) envelope->nSustainStart++;
			if(envelope->nSustainEnd >= i) envelope->nSustainEnd++;
			if(envelope->nReleaseNode >= i && envelope->nReleaseNode != ENV_RELEASE_NODE_UNSET) envelope->nReleaseNode++;

			SetModified(InstrumentHint().Envelope(), true);
			return i + 1;
		}
	}
	return 0;
}



void CViewInstrument::DrawPositionMarks()
{
	CRect rect;
	for(auto pos : m_dwNotifyPos) if (pos != Notification::PosInvalid)
	{
		rect.top = -2;
		rect.left = TickToScreen(pos);
		rect.right = rect.left + 1;
		rect.bottom = m_rcClient.bottom + 1;
		InvertRect(m_dcMemMain.m_hDC, &rect);
	}
}


LRESULT CViewInstrument::OnPlayerNotify(Notification *pnotify)
{
	Notification::Type type;
	CModDoc *pModDoc = GetDocument();
	if((!pnotify) || (!pModDoc))
		return 0;
	switch(m_nEnv)
	{
	case ENV_PANNING:	type = Notification::PanEnv; break;
	case ENV_PITCH:		type = Notification::PitchEnv; break;
	default:			type = Notification::VolEnv; break;
	}
	if(pnotify->type[Notification::Stop])
	{
		bool invalidate = false;
		for(auto &pos : m_dwNotifyPos)
		{
			if(pos != (uint32)Notification::PosInvalid)
			{
				pos = (uint32)Notification::PosInvalid;
				invalidate = true;
			}
		}
		if(invalidate)
		{
			InvalidateEnvelope();
		}
		m_baPlayingNote.reset();
	} else if(pnotify->type[type] && pnotify->item == m_nInstrument)
	{
		bool update = false;
		for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
		{
			uint32 newpos = (uint32)pnotify->pos[i];
			if(m_dwNotifyPos[i] != newpos)
			{
				update = true;
				break;
			}
		}
		if(update)
		{
			HDC hdc = ::GetDC(m_hWnd);
			DrawPositionMarks();
			for(CHANNELINDEX j = 0; j < MAX_CHANNELS; j++)
			{
				uint32 newpos = (uint32)pnotify->pos[j];
				m_dwNotifyPos[j] = newpos;
			}
			DrawPositionMarks();
			BitBlt(hdc, m_rcClient.left, m_rcClient.top, m_rcClient.Width(), m_rcClient.Height(), m_dcMemMain.GetSafeHdc(), 0, 0, SRCCOPY);
			::ReleaseDC(m_hWnd, hdc);
		}
	}
	return 0;
}


void CViewInstrument::DrawNcButton(CDC *pDC, UINT nBtn)
{
	CRect rect;
	COLORREF crHi = GetSysColor(COLOR_3DHILIGHT);
	COLORREF crDk = GetSysColor(COLOR_3DSHADOW);
	COLORREF crFc = GetSysColor(COLOR_3DFACE);
	COLORREF c1, c2;

	if(GetNcButtonRect(nBtn, rect))
	{
		DWORD dwStyle = m_NcButtonState[nBtn];
		COLORREF c3, c4;
		int xofs = 0, yofs = 0, nImage = 0;

		c1 = c2 = c3 = c4 = crFc;
		if(!(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FLATBUTTONS))
		{
			c1 = c3 = crHi;
			c2 = crDk;
			c4 = RGB(0, 0, 0);
		}
		if(dwStyle & (NCBTNS_PUSHED | NCBTNS_CHECKED))
		{
			c1 = crDk;
			c2 = crHi;
			if(!(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FLATBUTTONS))
			{
				c4 = crHi;
				c3 = (dwStyle & NCBTNS_PUSHED) ? RGB(0, 0, 0) : crDk;
			}
			xofs = yofs = 1;
		} else if((dwStyle & NCBTNS_MOUSEOVER) && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FLATBUTTONS))
		{
			c1 = crHi;
			c2 = crDk;
		}
		switch(cLeftBarButtons[nBtn])
		{
		case ID_ENVSEL_VOLUME:		nImage = IIMAGE_VOLENV; break;
		case ID_ENVSEL_PANNING:		nImage = IIMAGE_PANENV; break;
		case ID_ENVSEL_PITCH:		nImage = (dwStyle & NCBTNS_DISABLED) ? IIMAGE_NOPITCHENV : IIMAGE_PITCHENV; break;
		case ID_ENVELOPE_SETLOOP:	nImage = IIMAGE_LOOP; break;
		case ID_ENVELOPE_SUSTAIN:	nImage = IIMAGE_SUSTAIN; break;
		case ID_ENVELOPE_CARRY:		nImage = (dwStyle & NCBTNS_DISABLED) ? IIMAGE_NOCARRY : IIMAGE_CARRY; break;
		case ID_ENVELOPE_VOLUME:	nImage = IIMAGE_VOLSWITCH; break;
		case ID_ENVELOPE_PANNING:	nImage = IIMAGE_PANSWITCH; break;
		case ID_ENVELOPE_PITCH:		nImage = (dwStyle & NCBTNS_DISABLED) ? IIMAGE_NOPITCHSWITCH : IIMAGE_PITCHSWITCH; break;
		case ID_ENVELOPE_FILTER:	nImage = (dwStyle & NCBTNS_DISABLED) ? IIMAGE_NOFILTERSWITCH : IIMAGE_FILTERSWITCH; break;
		case ID_INSTRUMENT_SAMPLEMAP: nImage = IIMAGE_SAMPLEMAP; break;
		case ID_ENVELOPE_VIEWGRID:	nImage = IIMAGE_GRID; break;
		case ID_ENVELOPE_ZOOM_IN:	nImage = (dwStyle & NCBTNS_DISABLED) ? IIMAGE_NOZOOMIN : IIMAGE_ZOOMIN; break;
		case ID_ENVELOPE_ZOOM_OUT:	nImage = (dwStyle & NCBTNS_DISABLED) ? IIMAGE_NOZOOMOUT : IIMAGE_ZOOMOUT; break;
		case ID_ENVELOPE_LOAD:		nImage = IIMAGE_LOAD; break;
		case ID_ENVELOPE_SAVE:		nImage = IIMAGE_SAVE; break;
		}
		pDC->Draw3dRect(rect.left - 1, rect.top - 1, ENV_LEFTBAR_CXBTN + 2, ENV_LEFTBAR_CYBTN + 2, c3, c4);
		pDC->Draw3dRect(rect.left, rect.top, ENV_LEFTBAR_CXBTN, ENV_LEFTBAR_CYBTN, c1, c2);
		rect.DeflateRect(1, 1);
		pDC->FillSolidRect(&rect, crFc);
		rect.left += xofs;
		rect.top += yofs;
		if(dwStyle & NCBTNS_CHECKED)
			m_bmpEnvBar.Draw(pDC, IIMAGE_CHECKED, rect.TopLeft(), ILD_NORMAL);
		m_bmpEnvBar.Draw(pDC, nImage, rect.TopLeft(), ILD_NORMAL);
	} else
	{
		c1 = c2 = crFc;
		if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FLATBUTTONS)
		{
			c1 = crDk;
			c2 = crHi;
		}
		pDC->Draw3dRect(rect.left, rect.top, 2, ENV_LEFTBAR_CYBTN, c1, c2);
	}
}


void CViewInstrument::OnNcPaint()
{
	RECT rect;

	CModScrollView::OnNcPaint();
	GetWindowRect(&rect);
	// Assumes there is no other non-client items
	rect.bottom = ENV_LEFTBAR_CY;
	rect.right -= rect.left;
	rect.left = 0;
	rect.top = 0;
	if((rect.left < rect.right) && (rect.top < rect.bottom))
	{
		CDC *pDC = GetWindowDC();
		{
			// Shadow
			auto shadowRect = rect;
			shadowRect.top = shadowRect.bottom - 1;
			pDC->FillSolidRect(&shadowRect, GetSysColor(COLOR_BTNSHADOW));
		}
		rect.bottom--;
		if(rect.top < rect.bottom)
			pDC->FillSolidRect(&rect, GetSysColor(COLOR_BTNFACE));
		if(rect.top + 2 < rect.bottom)
		{
			for(UINT i = 0; i < ENV_LEFTBAR_BUTTONS; i++)
			{
				DrawNcButton(pDC, i);
			}
		}
		ReleaseDC(pDC);
	}
}


////////////////////////////////////////////////////////////////////
// CViewInstrument messages


void CViewInstrument::OnSize(UINT nType, int cx, int cy)
{
	CModScrollView::OnSize(nType, cx, cy);
	if(((nType == SIZE_RESTORED) || (nType == SIZE_MAXIMIZED)) && (cx > 0) && (cy > 0))
	{
		UpdateScrollSize();
	}
}


void CViewInstrument::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS *lpncsp)
{
	CModScrollView::OnNcCalcSize(bCalcValidRects, lpncsp);
	if(lpncsp)
	{
		lpncsp->rgrc[0].top += ENV_LEFTBAR_CY;
		if(lpncsp->rgrc[0].bottom < lpncsp->rgrc[0].top)
			lpncsp->rgrc[0].top = lpncsp->rgrc[0].bottom;
	}
}


void CViewInstrument::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	const auto button = GetNcButtonAtPoint(point);
	if(button != m_nBtnMouseOver)
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if(pMainFrm)
		{
			CString strText;
			if(button < ENV_LEFTBAR_BUTTONS && cLeftBarButtons[button] != ID_SEPARATOR)
			{
				strText = LoadResourceString(cLeftBarButtons[button]);
			}
			pMainFrm->SetHelpText(strText);
		}
		m_nBtnMouseOver = button;
		UpdateNcButtonState();
	}
	CModScrollView::OnNcMouseMove(nHitTest, point);
}


void CViewInstrument::OnNcLButtonDown(UINT uFlags, CPoint point)
{
	if(m_nBtnMouseOver < ENV_LEFTBAR_BUTTONS)
	{
		m_dwStatus |= INSSTATUS_NCLBTNDOWN;
		if(cLeftBarButtons[m_nBtnMouseOver] != ID_SEPARATOR)
		{
			PostMessage(WM_COMMAND, cLeftBarButtons[m_nBtnMouseOver]);
			UpdateNcButtonState();
		}
	}
	CModScrollView::OnNcLButtonDown(uFlags, point);
}


void CViewInstrument::OnNcLButtonUp(UINT uFlags, CPoint point)
{
	if(m_dwStatus & INSSTATUS_NCLBTNDOWN)
	{
		m_dwStatus &= ~INSSTATUS_NCLBTNDOWN;
		UpdateNcButtonState();
	}
	CModScrollView::OnNcLButtonUp(uFlags, point);
}


void CViewInstrument::OnNcLButtonDblClk(UINT uFlags, CPoint point)
{
	OnNcLButtonDown(uFlags, point);
}


LRESULT CViewInstrument::OnNcHitTest(CPoint point)
{
	CRect rect;
	GetWindowRect(&rect);
	rect.bottom = rect.top + ENV_LEFTBAR_CY;
	if(rect.PtInRect(point))
	{
		return HTBORDER;
	}
	return CModScrollView::OnNcHitTest(point);
}


void CViewInstrument::OnMouseMove(UINT, CPoint pt)
{
	ModInstrument *pIns = GetInstrumentPtr();
	if(pIns == nullptr)
		return;

	bool splitCursor = false;

	if((m_nBtnMouseOver < ENV_LEFTBAR_BUTTONS) || (m_dwStatus & INSSTATUS_NCLBTNDOWN))
	{
		m_dwStatus &= ~INSSTATUS_NCLBTNDOWN;
		m_nBtnMouseOver = 0xFFFF;
		UpdateNcButtonState();
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if(pMainFrm)
			pMainFrm->SetHelpText(_T(""));
	}
	int nTick = ScreenToTick(pt.x);
	int nVal = Clamp(ScreenToValue(pt.y), ENVELOPE_MIN, ENVELOPE_MAX);
	if(nTick < 0)
		nTick = 0;
	UpdateIndicator(nTick, nVal);

	if((m_dwStatus & INSSTATUS_DRAGGING) && (m_nDragItem))
	{
		if(!m_mouseMoveModified)
		{
			PrepareUndo("Move Envelope Point");
			m_mouseMoveModified = true;
		}
		bool changed = false;
		if(pt.x >= m_rcClient.right - 2)
			nTick++;
		if(IsDragItemEnvPoint())
		{
			// Ctrl pressed -> move tail of envelope
			changed = EnvSetValue(m_nDragItem - 1, nTick, nVal, CMainFrame::GetInputHandler()->CtrlPressed());
		} else
		{
			int nPoint = ScreenToPoint(pt.x, pt.y);
			if (nPoint >= 0) switch(m_nDragItem)
			{
			case ENV_DRAGLOOPSTART:
				changed = EnvSetLoopStart(nPoint);
				splitCursor = true;
				break;
			case ENV_DRAGLOOPEND:
				changed = EnvSetLoopEnd(nPoint);
				splitCursor = true;
				break;
			case ENV_DRAGSUSTAINSTART:
				changed = EnvSetSustainStart(nPoint);
				splitCursor = true;
				break;
			case ENV_DRAGSUSTAINEND:
				changed = EnvSetSustainEnd(nPoint);
				splitCursor = true;
				break;
			}
		}
		if(changed)
		{
			if(pt.x <= 0)
			{
				UpdateScrollSize();
				OnScrollBy(CSize(pt.x - (int)m_zoom, 0), TRUE);
			}
			if(pt.x >= m_rcClient.right - 1)
			{
				UpdateScrollSize();
				OnScrollBy(CSize((int)m_zoom + pt.x - m_rcClient.right, 0), TRUE);
			}
			SetModified(InstrumentHint().Envelope(), true);
			UpdateWindow();  //rewbs: TODO - optimisation here so we don't redraw whole view.
		}
	} else
	{
		CRect rect;
		if(EnvGetSustain())
		{
			int nspace = m_rcClient.bottom / 4;
			rect.top = ValueToScreen(EnvGetValue(EnvGetSustainStart())) - nspace;
			rect.bottom = rect.top + nspace * 2;
			rect.right = PointToScreen(EnvGetSustainStart()) + 1;
			rect.left = rect.right - m_envPointSize * 2;
			if(rect.PtInRect(pt))
			{
				splitCursor = true;  // ENV_DRAGSUSTAINSTART;
			} else
			{
				rect.top = ValueToScreen(EnvGetValue(EnvGetSustainEnd())) - nspace;
				rect.bottom = rect.top + nspace * 2;
				rect.left = PointToScreen(EnvGetSustainEnd()) - 1;
				rect.right = rect.left + m_envPointSize * 2;
				if(rect.PtInRect(pt))
					splitCursor = true;  // ENV_DRAGSUSTAINEND;
			}
		}
		if(EnvGetLoop())
		{
			rect.top = m_rcClient.top;
			rect.bottom = m_rcClient.bottom;
			rect.right = PointToScreen(EnvGetLoopStart()) + 1;
			rect.left = rect.right - m_envPointSize * 2;
			if(rect.PtInRect(pt))
			{
				splitCursor = true;  // ENV_DRAGLOOPSTART;
			} else
			{
				rect.left = PointToScreen(EnvGetLoopEnd()) - 1;
				rect.right = rect.left + m_envPointSize * 2;
				if(rect.PtInRect(pt))
					splitCursor = true;  // ENV_DRAGLOOPEND;
			}
		}
	}
	// Update the mouse cursor
	if(splitCursor)
	{
		if(!(m_dwStatus & INSSTATUS_SPLITCURSOR))
		{
			m_dwStatus |= INSSTATUS_SPLITCURSOR;
			if(!(m_dwStatus & INSSTATUS_DRAGGING))
				SetCapture();
			SetCursor(CMainFrame::curVSplit);
		}
	} else
	{
		if(m_dwStatus & INSSTATUS_SPLITCURSOR)
		{
			m_dwStatus &= ~INSSTATUS_SPLITCURSOR;
			SetCursor(CMainFrame::curArrow);
			if(!(m_dwStatus & INSSTATUS_DRAGGING))
				ReleaseCapture();
		}
	}
}



void CViewInstrument::UpdateIndicator()
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr || !m_nDragItem)
		return;

	uint32 point = DragItemToEnvPoint();
	if(point < pEnv->size())
	{
		UpdateIndicator(pEnv->at(point).tick, pEnv->at(point).value);
	}
}


void CViewInstrument::UpdateIndicator(int tick, int val)
{
	ModInstrument *pIns = GetInstrumentPtr();
	if(pIns == nullptr)
		return;

	CString s;
	s.Format(TrackerSettings::Instance().cursorPositionInHex ? _T("Tick %X, [%s]") : _T("Tick %d, [%s]"), tick, EnvValueToString(tick, val).GetString());
	CModScrollView::UpdateIndicator(s);
	CMainFrame::GetMainFrame()->NotifyAccessibilityUpdate(*this);
}


CString CViewInstrument::EnvValueToString(int tick, int val) const
{
	const InstrumentEnvelope *env = GetEnvelopePtr();
	const bool hasReleaseNode = env->nReleaseNode != ENV_RELEASE_NODE_UNSET;
	EnvelopeNode releaseNode;
	if(hasReleaseNode)
	{
		releaseNode = env->at(env->nReleaseNode);
	}

	CString s;
	if(!hasReleaseNode || tick <= releaseNode.tick + 1)
	{
		// ticks before release node (or no release node)
		const int displayVal = (m_nEnv != ENV_VOLUME && !(m_nEnv == ENV_PITCH && env->dwFlags[ENV_FILTER])) ? val - 32 : val;
		if(m_nEnv != ENV_PANNING)
			s.Format(_T("%d"), displayVal);
		else  // panning envelope: display right/center/left chars
			s.Format(_T("%d %c"), std::abs(displayVal), displayVal > 0 ? _T('R') : (displayVal < 0 ? _T('L') : _T('C')));
	} else
	{
		// ticks after release node
		int displayVal = (val - releaseNode.value) * 2;
		displayVal = (m_nEnv != ENV_VOLUME) ? displayVal - 32 : displayVal;
		s.Format(_T("Rel%c%d"), displayVal > 0 ? _T('+') : _T('-'), std::abs(displayVal));
	}
	return s;
}


void CViewInstrument::OnLButtonDown(UINT, CPoint pt)
{
	m_mouseMoveModified = false;
	if(!(m_dwStatus & INSSTATUS_DRAGGING))
	{
		CRect rect;
		// Look if dragging a point
		uint32 maxpoint = EnvGetLastPoint();
		uint32 oldDragItem = m_nDragItem;
		m_nDragItem = 0;
		const int hitboxSize = static_cast<int>((6 * m_nDPIx) / 96.0f);
		for(uint32 i = 0; i <= maxpoint; i++)
		{
			int x = PointToScreen(i);
			int y = ValueToScreen(EnvGetValue(i));
			rect.SetRect(x - hitboxSize, y - hitboxSize, x + hitboxSize + 1, y + hitboxSize + 1);
			if(rect.PtInRect(pt))
			{
				m_nDragItem = i + 1;
				break;
			}
		}
		if((!m_nDragItem) && (EnvGetSustain()))
		{
			int nspace = m_rcClient.bottom / 4;
			rect.top = ValueToScreen(EnvGetValue(EnvGetSustainStart())) - nspace;
			rect.bottom = rect.top + nspace * 2;
			rect.right = PointToScreen(EnvGetSustainStart()) + 1;
			rect.left = rect.right - m_envPointSize * 2;
			if(rect.PtInRect(pt))
			{
				m_nDragItem = ENV_DRAGSUSTAINSTART;
			} else
			{
				rect.top = ValueToScreen(EnvGetValue(EnvGetSustainEnd())) - nspace;
				rect.bottom = rect.top + nspace * 2;
				rect.left = PointToScreen(EnvGetSustainEnd()) - 1;
				rect.right = rect.left + m_envPointSize * 2;
				if(rect.PtInRect(pt))
					m_nDragItem = ENV_DRAGSUSTAINEND;
			}
		}
		if((!m_nDragItem) && (EnvGetLoop()))
		{
			rect.top = m_rcClient.top;
			rect.bottom = m_rcClient.bottom;
			rect.right = PointToScreen(EnvGetLoopStart()) + 1;
			rect.left = rect.right - m_envPointSize * 2;
			if(rect.PtInRect(pt))
			{
				m_nDragItem = ENV_DRAGLOOPSTART;
			} else
			{
				rect.left = PointToScreen(EnvGetLoopEnd()) - 1;
				rect.right = rect.left + m_envPointSize * 2;
				if(rect.PtInRect(pt))
					m_nDragItem = ENV_DRAGLOOPEND;
			}
		}

		if(m_nDragItem)
		{
			SetCapture();
			m_dwStatus |= INSSTATUS_DRAGGING;
			// refresh active node colour
			InvalidateRect(NULL, FALSE);
		} else
		{
			// Shift-Click: Insert envelope point here
			if(CMainFrame::GetInputHandler()->ShiftPressed())
			{
				if(InsertAtPoint(pt) == 0 && oldDragItem != 0)
				{
					InvalidateRect(NULL, FALSE);
				}
			} else if(oldDragItem)
			{
				InvalidateRect(NULL, FALSE);
			}
		}
	}
}


void CViewInstrument::OnLButtonUp(UINT, CPoint)
{
	m_mouseMoveModified = false;
	if(m_dwStatus & INSSTATUS_SPLITCURSOR)
	{
		m_dwStatus &= ~INSSTATUS_SPLITCURSOR;
		SetCursor(CMainFrame::curArrow);
	}
	if(m_dwStatus & INSSTATUS_DRAGGING)
	{
		m_dwStatus &= ~INSSTATUS_DRAGGING;
		ReleaseCapture();
	}
}


void CViewInstrument::OnRButtonDown(UINT flags, CPoint pt)
{
	const CModDoc *pModDoc = GetDocument();
	if(!pModDoc)
		return;
	const CSoundFile &sndFile = GetDocument()->GetSoundFile();

	if(m_dwStatus & INSSTATUS_DRAGGING)
		return;

	// Ctrl + Right-Click = Delete point
	if(flags & MK_CONTROL)
	{
		OnMButtonDown(flags, pt);
		return;
	}

	CMenu Menu;
	if((pModDoc) && (Menu.LoadMenu(IDR_ENVELOPES)))
	{
		CMenu *pSubMenu = Menu.GetSubMenu(0);
		if(pSubMenu != nullptr)
		{
			m_nDragItem = ScreenToPoint(pt.x, pt.y) + 1;
			const uint32 maxPoint = (sndFile.GetType() == MOD_TYPE_XM) ? 11 : 24;
			const uint32 lastpoint = EnvGetLastPoint();
			const bool forceRelease = !sndFile.GetModSpecifications().hasReleaseNode && (EnvGetReleaseNode() != ENV_RELEASE_NODE_UNSET);
			pSubMenu->EnableMenuItem(ID_ENVELOPE_INSERTPOINT, (lastpoint < maxPoint) ? MF_ENABLED : MF_GRAYED);
			pSubMenu->EnableMenuItem(ID_ENVELOPE_REMOVEPOINT, ((m_nDragItem) && (lastpoint > 0)) ? MF_ENABLED : MF_GRAYED);
			pSubMenu->EnableMenuItem(ID_ENVELOPE_CARRY, (sndFile.GetType() & (MOD_TYPE_IT | MOD_TYPE_MPT)) ? MF_ENABLED : MF_GRAYED);
			pSubMenu->EnableMenuItem(ID_ENVELOPE_TOGGLERELEASENODE, ((sndFile.GetModSpecifications().hasReleaseNode && m_nEnv == ENV_VOLUME) || forceRelease) ? MF_ENABLED : MF_GRAYED);
			pSubMenu->CheckMenuItem(ID_ENVELOPE_SETLOOP, (EnvGetLoop()) ? MF_CHECKED : MF_UNCHECKED);
			pSubMenu->CheckMenuItem(ID_ENVELOPE_SUSTAIN, (EnvGetSustain()) ? MF_CHECKED : MF_UNCHECKED);
			pSubMenu->CheckMenuItem(ID_ENVELOPE_CARRY, (EnvGetCarry()) ? MF_CHECKED : MF_UNCHECKED);
			pSubMenu->CheckMenuItem(ID_ENVELOPE_TOGGLERELEASENODE, (EnvGetReleaseNode() == m_nDragItem - 1) ? MF_CHECKED : MF_UNCHECKED);
			m_ptMenu = pt;
			ClientToScreen(&pt);
			pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
		}
	}
}

void CViewInstrument::OnMButtonDown(UINT, CPoint pt)
{
	// Middle mouse button: Remove envelope point
	int point = ScreenToPoint(pt.x, pt.y);
	if(point >= 0)
	{
		EnvRemovePoint(point);
		m_nDragItem = point + 1;
	}
}


void CViewInstrument::OnPrevInstrument()
{
	SendCtrlMessage(CTRLMSG_INS_PREVINSTRUMENT);
}


void CViewInstrument::OnNextInstrument()
{
	SendCtrlMessage(CTRLMSG_INS_NEXTINSTRUMENT);
}


void CViewInstrument::OnEditSampleMap()
{
	SendCtrlMessage(CTRLMSG_INS_SAMPLEMAP);
}


void CViewInstrument::OnSelectVolumeEnv()
{
	if(m_nEnv != ENV_VOLUME)
		SetCurrentInstrument(m_nInstrument, ENV_VOLUME);
}


void CViewInstrument::OnSelectPanningEnv()
{
	if(m_nEnv != ENV_PANNING)
		SetCurrentInstrument(m_nInstrument, ENV_PANNING);
}


void CViewInstrument::OnSelectPitchEnv()
{
	if(m_nEnv != ENV_PITCH)
		SetCurrentInstrument(m_nInstrument, ENV_PITCH);
}


void CViewInstrument::OnEnvLoopChanged()
{
	CModDoc *pModDoc = GetDocument();
	PrepareUndo("Toggle Envelope Loop");
	if((pModDoc) && (EnvSetLoop(!EnvGetLoop())))
	{
		InstrumentEnvelope *pEnv = GetEnvelopePtr();
		if(EnvGetLoop() && pEnv != nullptr && pEnv->nLoopEnd == 0)
		{
			// Enabled loop => set loop points if no loop has been specified yet.
			pEnv->nLoopStart = 0;
			pEnv->nLoopEnd = mpt::saturate_cast<decltype(pEnv->nLoopEnd)>(pEnv->size() - 1);
		}
		SetModified(InstrumentHint().Envelope(), true);
	}
}


void CViewInstrument::OnEnvSustainChanged()
{
	CModDoc *pModDoc = GetDocument();
	PrepareUndo("Toggle Envelope Sustain");
	if((pModDoc) && (EnvSetSustain(!EnvGetSustain())))
	{
		InstrumentEnvelope *pEnv = GetEnvelopePtr();
		if(EnvGetSustain() && pEnv != nullptr && pEnv->nSustainStart == pEnv->nSustainEnd && IsDragItemEnvPoint())
		{
			// Enabled sustain loop => set sustain loop points if no sustain loop has been specified yet.
			pEnv->nSustainStart = pEnv->nSustainEnd = mpt::saturate_cast<decltype(pEnv->nSustainEnd)>(m_nDragItem - 1);
		}
		SetModified(InstrumentHint().Envelope(), true);
	}
}


void CViewInstrument::OnEnvCarryChanged()
{
	CModDoc *pModDoc = GetDocument();
	PrepareUndo("Toggle Envelope Carry");
	if((pModDoc) && (EnvSetCarry(!EnvGetCarry())))
	{
		SetModified(InstrumentHint().Envelope(), false);
		UpdateNcButtonState();
	}
}

void CViewInstrument::OnEnvToggleReleasNode()
{
	if(IsDragItemEnvPoint())
	{
		PrepareUndo("Toggle Envelope Release Node");
		if(EnvToggleReleaseNode(m_nDragItem - 1))
		{
			SetModified(InstrumentHint().Envelope(), true);
		}
	}
}


void CViewInstrument::OnEnvVolChanged()
{
	GetDocument()->GetInstrumentUndo().PrepareUndo(m_nInstrument, "Toggle Volume Envelope", ENV_VOLUME);
	if(EnvSetVolEnv(!EnvGetVolEnv()))
	{
		SetModified(InstrumentHint().Envelope(), false);
		UpdateNcButtonState();
	}
}


void CViewInstrument::OnEnvPanChanged()
{
	GetDocument()->GetInstrumentUndo().PrepareUndo(m_nInstrument, "Toggle Panning Envelope", ENV_PANNING);
	if(EnvSetPanEnv(!EnvGetPanEnv()))
	{
		SetModified(InstrumentHint().Envelope(), false);
		UpdateNcButtonState();
	}
}


void CViewInstrument::OnEnvPitchChanged()
{
	GetDocument()->GetInstrumentUndo().PrepareUndo(m_nInstrument, "Toggle Pitch Envelope", ENV_PITCH);
	if(EnvSetPitchEnv(!EnvGetPitchEnv()))
	{
		SetModified(InstrumentHint().Envelope(), false);
		UpdateNcButtonState();
	}
}


void CViewInstrument::OnEnvFilterChanged()
{
	GetDocument()->GetInstrumentUndo().PrepareUndo(m_nInstrument, "Toggle Filter Envelope", ENV_PITCH);
	if(EnvSetFilterEnv(!EnvGetFilterEnv()))
	{
		SetModified(InstrumentHint().Envelope(), false);
		UpdateNcButtonState();
	}
}


void CViewInstrument::OnEnvToggleGrid()
{
	m_bGrid = !m_bGrid;
	if(m_bGrid)
		m_bGridForceRedraw = true;
	CModDoc *pModDoc = GetDocument();
	if(pModDoc)
		pModDoc->UpdateAllViews(nullptr, InstrumentHint(m_nInstrument).Envelope());
}


void CViewInstrument::OnEnvRemovePoint()
{
	if(m_nDragItem > 0)
	{
		EnvRemovePoint(m_nDragItem - 1);
	}
}


void CViewInstrument::OnEnvInsertPoint()
{
	const int tick = ScreenToTick(m_ptMenu.x), value = ScreenToValue(m_ptMenu.y);
	if(!EnvInsertPoint(tick, value))
	{
		// Couldn't insert point, maybe because there's already a point at this tick
		// => Try next tick
		EnvInsertPoint(tick + 1, value);
	}
}


bool CViewInstrument::InsertAtPoint(CPoint pt)
{
	auto item = EnvInsertPoint(ScreenToTick(pt.x), ScreenToValue(pt.y));  // returns point ID + 1 if successful, else 0.
	if(item > 0)
	{
		// Drag point if successful
		SetCapture();
		m_dwStatus |= INSSTATUS_DRAGGING;
		m_nDragItem = item;
	}
	return item > 0;
}


void CViewInstrument::OnEditCopy()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc)
		pModDoc->CopyEnvelope(m_nInstrument, m_nEnv);
}


void CViewInstrument::OnEditPaste()
{
	CModDoc *pModDoc = GetDocument();
	PrepareUndo("Paste Envelope");
	if(pModDoc->PasteEnvelope(m_nInstrument, m_nEnv))
	{
		SetModified(InstrumentHint().Envelope(), true);
	} else
	{
		pModDoc->GetInstrumentUndo().RemoveLastUndoStep(m_nInstrument);
	}
}

static DWORD nLastNotePlayed = 0;
static DWORD nLastScanCode = 0;


void CViewInstrument::PlayNote(ModCommand::NOTE note)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr || pMainFrm == nullptr)
	{
		return;
	}
	if(note > 0 && note < 128)
	{
		if(m_nInstrument && !m_baPlayingNote[note])
		{
			CSoundFile &sndFile = pModDoc->GetSoundFile();
			ModInstrument *pIns = sndFile.Instruments[m_nInstrument];
			if((!pIns) || (!pIns->Keyboard[note - NOTE_MIN] && !pIns->nMixPlug))
				return;
			{
				if(pMainFrm->GetModPlaying() != pModDoc)
				{
					sndFile.m_SongFlags.set(SONG_PAUSED);
					sndFile.ResetChannels();
					if(!pMainFrm->PlayMod(pModDoc))
						return;
				}
				CriticalSection cs;
				pModDoc->CheckNNA(note, m_nInstrument, m_baPlayingNote);
				pModDoc->PlayNote(PlayNoteParam(note).Instrument(m_nInstrument), &m_noteChannel);
			}
			CString noteName;
			if(ModCommand::IsNote(note))
			{
				noteName = mpt::ToCString(sndFile.GetNoteName(note, m_nInstrument));
			}
			pMainFrm->SetInfoText(noteName);
		}
	} else
	{
		pModDoc->PlayNote(PlayNoteParam(note).Instrument(m_nInstrument));
	}
}


// Drop files from Windows
void CViewInstrument::OnDropFiles(HDROP hDropInfo)
{
	const UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
	CMainFrame::GetMainFrame()->SetForegroundWindow();
	for(UINT f = 0; f < nFiles; f++)
	{
		UINT size = ::DragQueryFile(hDropInfo, f, nullptr, 0) + 1;
		std::vector<TCHAR> fileName(size, _T('\0'));
		if(::DragQueryFile(hDropInfo, f, fileName.data(), size))
		{
			const mpt::PathString file = mpt::PathString::FromNative(fileName.data());
			PrepareUndo("Replace Envelope");
			if(GetDocument()->LoadEnvelope(m_nInstrument, m_nEnv, file))
			{
				SetModified(InstrumentHint(m_nInstrument).Envelope(), true);
			} else
			{
				GetDocument()->GetInstrumentUndo().RemoveLastUndoStep(m_nInstrument);
				if(SendCtrlMessage(CTRLMSG_INS_OPENFILE, (LPARAM)&file) && f < nFiles - 1)
				{
					// Insert more instrument slots
					SendCtrlMessage(IDC_INSTRUMENT_NEW);
				}
			}
		}
	}
	::DragFinish(hDropInfo);
}


BOOL CViewInstrument::OnDragonDrop(BOOL doDrop, const DRAGONDROP *dropInfo)
{
	CModDoc *modDoc = GetDocument();
	bool canDrop = false;

	if((!dropInfo) || (!modDoc))
		return FALSE;
	CSoundFile &sndFile = modDoc->GetSoundFile();
	switch(dropInfo->dropType)
	{
	case DRAGONDROP_INSTRUMENT:
		if(dropInfo->sndFile == &sndFile)
		{
			canDrop = ((dropInfo->dropItem)
			           && (dropInfo->dropItem <= sndFile.m_nInstruments)
			           && (dropInfo->sndFile == &sndFile));
		} else
		{
			canDrop = ((dropInfo->dropItem)
			           && ((dropInfo->dropParam) || (dropInfo->sndFile)));
		}
		break;

	case DRAGONDROP_DLS:
		canDrop = ((dropInfo->dropItem < CTrackApp::gpDLSBanks.size())
		            && (CTrackApp::gpDLSBanks[dropInfo->dropItem]));
		break;

	case DRAGONDROP_SOUNDFILE:
	case DRAGONDROP_MIDIINSTR:
		canDrop = !dropInfo->GetPath().empty();
		break;
	}
	if((!canDrop) || (!doDrop))
		return canDrop;
	if((!sndFile.GetNumInstruments()) && sndFile.GetModSpecifications().instrumentsMax > 0)
	{
		SendCtrlMessage(CTRLMSG_INS_NEWINSTRUMENT);
	}
	if((!m_nInstrument) || (m_nInstrument > sndFile.GetNumInstruments()))
		return FALSE;
	// Do the drop
	bool update = false;
	BeginWaitCursor();
	switch(dropInfo->dropType)
	{
	case DRAGONDROP_INSTRUMENT:
		if(dropInfo->sndFile == &sndFile)
			SendCtrlMessage(CTRLMSG_SETCURRENTINSTRUMENT, dropInfo->dropItem);
		else
			SendCtrlMessage(CTRLMSG_INS_SONGDROP, (LPARAM)dropInfo);
		break;

	case DRAGONDROP_MIDIINSTR:
		if(CDLSBank::IsDLSBank(dropInfo->GetPath()))
		{
			CDLSBank dlsbank;
			if(dlsbank.Open(dropInfo->GetPath()))
			{
				const DLSINSTRUMENT *pDlsIns;
				UINT nIns = 0, nRgn = 0xFF;
				// Drums
				if(dropInfo->dropItem & 0x80)
				{
					UINT key = dropInfo->dropItem & 0x7F;
					pDlsIns = dlsbank.FindInstrument(TRUE, 0xFFFF, 0xFF, key, &nIns);
					if(pDlsIns)
						nRgn = dlsbank.GetRegionFromKey(nIns, key);
				} else
				// Melodic
				{
					pDlsIns = dlsbank.FindInstrument(FALSE, 0xFFFF, dropInfo->dropItem, 60, &nIns);
					if(pDlsIns)
						nRgn = dlsbank.GetRegionFromKey(nIns, 60);
				}
				canDrop = false;
				if(pDlsIns)
				{
					CriticalSection cs;
					modDoc->GetInstrumentUndo().PrepareUndo(m_nInstrument, "Replace Instrument");
					canDrop = dlsbank.ExtractInstrument(sndFile, m_nInstrument, nIns, nRgn);
				}
				update = true;
				break;
			}
		}
		// Instrument file -> fall through
		[[fallthrough]];
	case DRAGONDROP_SOUNDFILE:
		SendCtrlMessage(CTRLMSG_INS_OPENFILE, dropInfo->dropParam);
		break;

	case DRAGONDROP_DLS:
		{
			const CDLSBank *pDLSBank = CTrackApp::gpDLSBanks[dropInfo->dropItem];
			UINT nIns = dropInfo->dropParam & 0x7FFF;
			UINT nRgn;
			// Drums: (0x80000000) | (Region << 16) | (Instrument)
			if (dropInfo->dropParam & 0x80000000)
			{
				nRgn = (dropInfo->dropParam & 0xFF0000) >> 16;
			} else
			// Melodic: (Instrument)
			{
				nRgn = pDLSBank->GetRegionFromKey(nIns, 60);
			}

			CriticalSection cs;

			modDoc->GetInstrumentUndo().PrepareUndo(m_nInstrument, "Replace Instrument");
			canDrop = pDLSBank->ExtractInstrument(sndFile, m_nInstrument, nIns, nRgn);
			update = true;
		}
		break;
	}
	if(update)
	{
		SetModified(InstrumentHint().Info().Envelope().Names(), true);
		GetDocument()->UpdateAllViews(nullptr, SampleHint().Info().Names().Data(), this);
	}
	CMDIChildWnd *pMDIFrame = (CMDIChildWnd *)GetParentFrame();
	if(pMDIFrame)
	{
		pMDIFrame->MDIActivate();
		pMDIFrame->SetActiveView(this);
		SetFocus();
	}
	EndWaitCursor();
	return canDrop;
}


LRESULT CViewInstrument::OnMidiMsg(WPARAM midiDataParam, LPARAM)
{
	const uint32 midiData = static_cast<uint32>(midiDataParam);
	CModDoc *modDoc = GetDocument();
	if(modDoc != nullptr)
	{
		modDoc->ProcessMIDI(midiData, m_nInstrument, modDoc->GetSoundFile().GetInstrumentPlugin(m_nInstrument), kCtxViewInstruments);

		MIDIEvents::EventType event = MIDIEvents::GetTypeFromEvent(midiData);
		uint8 midiByte1 = MIDIEvents::GetDataByte1FromEvent(midiData);
		if(event == MIDIEvents::evNoteOn)
		{
			CMainFrame::GetMainFrame()->SetInfoText(mpt::ToCString(modDoc->GetSoundFile().GetNoteName(midiByte1 + NOTE_MIN, m_nInstrument)));
		}

		return 1;
	}
	return 0;
}


BOOL CViewInstrument::PreTranslateMessage(MSG *pMsg)
{
	if(pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) ||
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler *ih = CMainFrame::GetInputHandler();

			//Translate message manually
			UINT nChar = static_cast<UINT>(pMsg->wParam);
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);
			InputTargetContext ctx = (InputTargetContext)(kCtxViewInstruments);

			if(ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT) != kcNull)
				return true;  // Mapped to a command, no need to pass message on.

			// Handle Application (menu) key
			if(pMsg->message == WM_KEYDOWN && nChar == VK_APPS)
			{
				CPoint pt(0, 0);
				if(m_nDragItem > 0)
				{
					uint32 point = DragItemToEnvPoint();
					pt.SetPoint(PointToScreen(point), ValueToScreen(EnvGetValue(point)));
				}
				OnRButtonDown(0, pt);
			}
		}
	}

	return CModScrollView::PreTranslateMessage(pMsg);
}


LRESULT CViewInstrument::OnCustomKeyMsg(WPARAM wParam, LPARAM)
{
	CModDoc *pModDoc = GetDocument();
	if(!pModDoc)
		return kcNull;
	CSoundFile &sndFile = pModDoc->GetSoundFile();

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	switch(wParam)
	{
		case kcPrevInstrument:	OnPrevInstrument(); return wParam;
		case kcNextInstrument:	OnNextInstrument(); return wParam;
		case kcEditCopy:		OnEditCopy(); return wParam;
		case kcEditPaste:		OnEditPaste(); return wParam;
		case kcEditUndo:		OnEditUndo(); return wParam;
		case kcEditRedo:		OnEditRedo(); return wParam;
		case kcNoteOff:			PlayNote(NOTE_KEYOFF); return wParam;
		case kcNoteCut:			PlayNote(NOTE_NOTECUT); return wParam;
		case kcInstrumentLoad:	SendCtrlMessage(IDC_INSTRUMENT_OPEN); return wParam;
		case kcInstrumentSave:	SendCtrlMessage(IDC_INSTRUMENT_SAVEAS); return wParam;
		case kcInstrumentNew:	SendCtrlMessage(IDC_INSTRUMENT_NEW); return wParam;

		// envelope editor
		case kcInstrumentEnvelopeLoad:					OnEnvLoad(); return wParam;
		case kcInstrumentEnvelopeSave:					OnEnvSave(); return wParam;
		case kcInstrumentEnvelopeZoomIn:				OnEnvZoomIn(); return wParam;
		case kcInstrumentEnvelopeZoomOut:				OnEnvZoomOut(); return wParam;
		case kcInstrumentEnvelopeScale:					OnEnvelopeScalePoints(); return wParam;
		case kcInstrumentEnvelopeSwitchToVolume:		OnSelectVolumeEnv(); return wParam;
		case kcInstrumentEnvelopeSwitchToPanning:		OnSelectPanningEnv(); return wParam;
		case kcInstrumentEnvelopeSwitchToPitch:			OnSelectPitchEnv(); return wParam;
		case kcInstrumentEnvelopeToggleVolume:			OnEnvVolChanged(); return wParam;
		case kcInstrumentEnvelopeTogglePanning:			OnEnvPanChanged(); return wParam;
		case kcInstrumentEnvelopeTogglePitch:			OnEnvPitchChanged(); return wParam;
		case kcInstrumentEnvelopeToggleFilter:			OnEnvFilterChanged(); return wParam;
		case kcInstrumentEnvelopeToggleLoop:			OnEnvLoopChanged(); return wParam;
		case kcInstrumentEnvelopeSelectLoopStart:		EnvKbdSelectPoint(ENV_DRAGLOOPSTART); return wParam;
		case kcInstrumentEnvelopeSelectLoopEnd:			EnvKbdSelectPoint(ENV_DRAGLOOPEND); return wParam;
		case kcInstrumentEnvelopeToggleSustain:			OnEnvSustainChanged(); return wParam;
		case kcInstrumentEnvelopeSelectSustainStart:	EnvKbdSelectPoint(ENV_DRAGSUSTAINSTART); return wParam;
		case kcInstrumentEnvelopeSelectSustainEnd:		EnvKbdSelectPoint(ENV_DRAGSUSTAINEND); return wParam;
		case kcInstrumentEnvelopeToggleCarry:			OnEnvCarryChanged(); return wParam;
		case kcInstrumentEnvelopePointPrev:				EnvKbdSelectPoint(ENV_DRAGPREVIOUS); return wParam;
		case kcInstrumentEnvelopePointNext:				EnvKbdSelectPoint(ENV_DRAGNEXT); return wParam;
		case kcInstrumentEnvelopePointMoveLeft:			EnvKbdMovePointLeft(1); return wParam;
		case kcInstrumentEnvelopePointMoveRight:		EnvKbdMovePointRight(1); return wParam;
		case kcInstrumentEnvelopePointMoveLeftCoarse:	EnvKbdMovePointLeft(sndFile.m_PlayState.m_nCurrentRowsPerBeat * sndFile.m_PlayState.m_nMusicSpeed); return wParam;
		case kcInstrumentEnvelopePointMoveRightCoarse:	EnvKbdMovePointRight(sndFile.m_PlayState.m_nCurrentRowsPerBeat * sndFile.m_PlayState.m_nMusicSpeed); return wParam;
		case kcInstrumentEnvelopePointMoveUp:			EnvKbdMovePointVertical(1); return wParam;
		case kcInstrumentEnvelopePointMoveDown:			EnvKbdMovePointVertical(-1); return wParam;
		case kcInstrumentEnvelopePointMoveUp8:			EnvKbdMovePointVertical(8); return wParam;
		case kcInstrumentEnvelopePointMoveDown8:		EnvKbdMovePointVertical(-8); return wParam;
		case kcInstrumentEnvelopePointInsert:			EnvKbdInsertPoint(); return wParam;
		case kcInstrumentEnvelopePointRemove:			EnvKbdRemovePoint(); return wParam;
		case kcInstrumentEnvelopeSetLoopStart:			EnvKbdSetLoopStart(); return wParam;
		case kcInstrumentEnvelopeSetLoopEnd:			EnvKbdSetLoopEnd(); return wParam;
		case kcInstrumentEnvelopeSetSustainLoopStart:	EnvKbdSetSustainStart(); return wParam;
		case kcInstrumentEnvelopeSetSustainLoopEnd:		EnvKbdSetSustainEnd(); return wParam;
		case kcInstrumentEnvelopeToggleReleaseNode:		EnvKbdToggleReleaseNode(); return wParam;
	}
	if(wParam >= kcInstrumentStartNotes && wParam <= kcInstrumentEndNotes)
	{
		PlayNote(static_cast<ModCommand::NOTE>(wParam - kcInstrumentStartNotes + 1 + pMainFrm->GetBaseOctave() * 12));
		return wParam;
	}
	if(wParam >= kcInstrumentStartNoteStops && wParam <= kcInstrumentEndNoteStops)
	{
		ModCommand::NOTE note = static_cast<ModCommand::NOTE>(wParam - kcInstrumentStartNoteStops + 1 + pMainFrm->GetBaseOctave() * 12);
		if(ModCommand::IsNote(note))
			m_baPlayingNote[note] = false;
		pModDoc->NoteOff(note, false, m_nInstrument, m_noteChannel[note - NOTE_MIN]);
		return wParam;
	}

	return kcNull;
}


void CViewInstrument::OnEnvelopeScalePoints()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return;
	const CSoundFile &sndFile = pModDoc->GetSoundFile();

	if(m_nInstrument >= 1
	   && m_nInstrument <= sndFile.GetNumInstruments()
	   && sndFile.Instruments[m_nInstrument])
	{
		// "Center" y value of the envelope. For panning and pitch, this is 32, for volume and filter it is 0 (minimum).
		int nOffset = ((m_nEnv != ENV_VOLUME) && !GetEnvelopePtr()->dwFlags[ENV_FILTER]) ? 32 : 0;

		CScaleEnvPointsDlg dlg(this, *GetEnvelopePtr(), nOffset);
		if(dlg.DoModal() == IDOK)
		{
			PrepareUndo("Scale Envelope");
			dlg.Apply();
			SetModified(InstrumentHint().Envelope(), true);
		}
	}
}


void CViewInstrument::EnvSetZoom(float newZoom)
{
	m_zoom = Clamp(newZoom, ENV_MIN_ZOOM, ENV_MAX_ZOOM);
	InvalidateRect(NULL, FALSE);
	UpdateScrollSize();
	UpdateNcButtonState();
}


////////////////////////////////////////
//  Envelope Editor - Keyboard actions

void CViewInstrument::EnvKbdSelectPoint(DragPoints point)
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr)
		return;

	switch(point)
	{
	case ENV_DRAGLOOPSTART:
	case ENV_DRAGLOOPEND:
		if(!pEnv->dwFlags[ENV_LOOP])
			return;
		m_nDragItem = point;
		break;
	case ENV_DRAGSUSTAINSTART:
	case ENV_DRAGSUSTAINEND:
		if(!pEnv->dwFlags[ENV_SUSTAIN])
			return;
		m_nDragItem = point;
		break;
	case ENV_DRAGPREVIOUS:
		if(m_nDragItem <= 1 || m_nDragItem > pEnv->size())
			m_nDragItem = pEnv->size();
		else
			m_nDragItem--;
		break;
	case ENV_DRAGNEXT:
		if(m_nDragItem >= pEnv->size())
			m_nDragItem = 1;
		else
			m_nDragItem++;
		break;
	}
	UpdateIndicator();
	InvalidateRect(NULL, FALSE);
}


void CViewInstrument::EnvKbdMovePointLeft(int stepsize)
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr)
		return;
	const MODTYPE modType = GetDocument()->GetModType();

	// Move loop points?
	PrepareUndo("Move Envelope Point");
	if(m_nDragItem == ENV_DRAGSUSTAINSTART)
	{
		if(pEnv->nSustainStart <= 0)
			return;
		pEnv->nSustainStart--;
		if(modType == MOD_TYPE_XM)
			pEnv->nSustainEnd = pEnv->nSustainStart;
	} else if(m_nDragItem == ENV_DRAGSUSTAINEND)
	{
		if(pEnv->nSustainEnd <= 0)
			return;
		if(pEnv->nSustainEnd <= pEnv->nSustainStart)
			pEnv->nSustainStart--;
		pEnv->nSustainEnd--;
	} else if(m_nDragItem == ENV_DRAGLOOPSTART)
	{
		if(pEnv->nLoopStart <= 0)
			return;
		pEnv->nLoopStart--;
	} else if(m_nDragItem == ENV_DRAGLOOPEND)
	{
		if(pEnv->nLoopEnd <= 0)
			return;
		if(pEnv->nLoopEnd <= pEnv->nLoopStart)
			pEnv->nLoopStart--;
		pEnv->nLoopEnd--;
	} else
	{
		// Move envelope node
		if(!IsDragItemEnvPoint() || m_nDragItem <= 1)
		{
			GetDocument()->GetInstrumentUndo().RemoveLastUndoStep(m_nInstrument);
			return;
		}
		if(!EnvSetValue(m_nDragItem - 1, pEnv->at(m_nDragItem - 1).tick - stepsize))
		{
			GetDocument()->GetInstrumentUndo().RemoveLastUndoStep(m_nInstrument);
			return;
		}
	}
	UpdateIndicator();
	SetModified(InstrumentHint().Envelope(), true);
}


void CViewInstrument::EnvKbdMovePointRight(int stepsize)
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr)
		return;
	const MODTYPE modType = GetDocument()->GetModType();

	// Move loop points?
	PrepareUndo("Move Envelope Point");
	if(m_nDragItem == ENV_DRAGSUSTAINSTART)
	{
		if(pEnv->nSustainStart >= pEnv->size() - 1)
			return;
		if(pEnv->nSustainStart >= pEnv->nSustainEnd)
			pEnv->nSustainEnd++;
		pEnv->nSustainStart++;
	} else if(m_nDragItem == ENV_DRAGSUSTAINEND)
	{
		if(pEnv->nSustainEnd >= pEnv->size() - 1)
			return;
		pEnv->nSustainEnd++;
		if(modType == MOD_TYPE_XM)
			pEnv->nSustainStart = pEnv->nSustainEnd;
	} else if(m_nDragItem == ENV_DRAGLOOPSTART)
	{
		if(pEnv->nLoopStart >= pEnv->size() - 1)
			return;
		if(pEnv->nLoopStart >= pEnv->nLoopEnd)
			pEnv->nLoopEnd++;
		pEnv->nLoopStart++;
	} else if(m_nDragItem == ENV_DRAGLOOPEND)
	{
		if(pEnv->nLoopEnd >= pEnv->size() - 1)
			return;
		pEnv->nLoopEnd++;
	} else
	{
		// Move envelope node
		if(!IsDragItemEnvPoint() || m_nDragItem <= 1)
		{
			GetDocument()->GetInstrumentUndo().RemoveLastUndoStep(m_nInstrument);
			return;
		}
		if(!EnvSetValue(m_nDragItem - 1, pEnv->at(m_nDragItem - 1).tick + stepsize))
		{
			GetDocument()->GetInstrumentUndo().RemoveLastUndoStep(m_nInstrument);
			return;
		}
	}
	UpdateIndicator();
	SetModified(InstrumentHint().Envelope(), true);
}


void CViewInstrument::EnvKbdMovePointVertical(int stepsize)
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr || !IsDragItemEnvPoint())
		return;
	int val = pEnv->at(m_nDragItem - 1).value + stepsize;
	PrepareUndo("Move Envelope Point");
	if(EnvSetValue(m_nDragItem - 1, int32_min, val, false))
	{
		UpdateIndicator();
		SetModified(InstrumentHint().Envelope(), true);
	} else
	{
		GetDocument()->GetInstrumentUndo().RemoveLastUndoStep(m_nInstrument);
	}
}


void CViewInstrument::EnvKbdInsertPoint()
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr)
		return;
	if(!IsDragItemEnvPoint())
		m_nDragItem = pEnv->size();
	EnvelopeNode::tick_t newTick = 10;
	EnvelopeNode::value_t newVal = m_nEnv == ENV_VOLUME ? ENVELOPE_MAX : ENVELOPE_MID;
	if(m_nDragItem < pEnv->size() && (pEnv->at(m_nDragItem).tick - pEnv->at(m_nDragItem - 1).tick > 1))
	{
		// If some other point than the last is selected: interpolate between this and next point (if there's room between them)
		newTick = (pEnv->at(m_nDragItem - 1).tick + pEnv->at(m_nDragItem).tick) / 2;
		newVal = (pEnv->at(m_nDragItem - 1).value + pEnv->at(m_nDragItem).value) / 2;
	} else if(!pEnv->empty())
	{
		// Last point is selected: add point after last point
		newTick = pEnv->back().tick + 4;
		newVal = pEnv->back().value;
	}

	auto newPoint = EnvInsertPoint(newTick, newVal);
	if(newPoint > 0)
		m_nDragItem = newPoint;
	UpdateIndicator();
}


void CViewInstrument::EnvKbdRemovePoint()
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr || !IsDragItemEnvPoint() || pEnv->empty())
		return;
	if(m_nDragItem > pEnv->size())
		m_nDragItem = pEnv->size();
	EnvRemovePoint(m_nDragItem - 1);
	UpdateIndicator();
}


void CViewInstrument::EnvKbdSetLoopStart()
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr || !IsDragItemEnvPoint())
		return;
	PrepareUndo("Set Envelope Loop Start");
	if(!EnvGetLoop())
		EnvSetLoopStart(0);
	EnvSetLoopStart(m_nDragItem - 1);
	SetModified(InstrumentHint(m_nInstrument).Envelope(), true);
}


void CViewInstrument::EnvKbdSetLoopEnd()
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr || !IsDragItemEnvPoint())
		return;
	PrepareUndo("Set Envelope Loop End");
	if(!EnvGetLoop())
	{
		EnvSetLoop(true);
		EnvSetLoopStart(0);
	}
	EnvSetLoopEnd(m_nDragItem - 1);
	SetModified(InstrumentHint(m_nInstrument).Envelope(), true);
}


void CViewInstrument::EnvKbdSetSustainStart()
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr || !IsDragItemEnvPoint())
		return;
	PrepareUndo("Set Envelope Sustain Start");
	if(!EnvGetSustain())
		EnvSetSustain(true);
	EnvSetSustainStart(m_nDragItem - 1);
	SetModified(InstrumentHint(m_nInstrument).Envelope(), true);
}


void CViewInstrument::EnvKbdSetSustainEnd()
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr || !IsDragItemEnvPoint())
		return;
	PrepareUndo("Set Envelope Sustain End");
	if(!EnvGetSustain())
	{
		EnvSetSustain(true);
		EnvSetSustainStart(0);
	}
	EnvSetSustainEnd(m_nDragItem - 1);
	SetModified(InstrumentHint(m_nInstrument).Envelope(), true);
}


void CViewInstrument::EnvKbdToggleReleaseNode()
{
	InstrumentEnvelope *pEnv = GetEnvelopePtr();
	if(pEnv == nullptr || !IsDragItemEnvPoint())
		return;
	PrepareUndo("Toggle Release Node");
	if(EnvToggleReleaseNode(m_nDragItem - 1))
	{
		UpdateIndicator();
		SetModified(InstrumentHint().Envelope(), true);
	} else
	{
		GetDocument()->GetInstrumentUndo().RemoveLastUndoStep(m_nInstrument);
	}
}


// Get a pointer to the currently active instrument.
ModInstrument *CViewInstrument::GetInstrumentPtr() const
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return nullptr;
	return pModDoc->GetSoundFile().Instruments[m_nInstrument];
}


// Get a pointer to the currently selected envelope.
// This function also implicitely validates the moddoc and soundfile pointers.
InstrumentEnvelope *CViewInstrument::GetEnvelopePtr() const
{
	// First do some standard checks...
	ModInstrument *pIns = GetInstrumentPtr();
	if(pIns == nullptr)
		return nullptr;

	return &pIns->GetEnvelope(m_nEnv);
}


bool CViewInstrument::CanMovePoint(uint32 envPoint, int step)
{
	const InstrumentEnvelope *env = GetEnvelopePtr();
	if(env == nullptr)
		return false;

	// Can't move first point
	if(envPoint == 0)
	{
		return false;
	}
	// Can't move left of previous point
	if((step < 0) && (env->at(envPoint).tick - env->at(envPoint - 1).tick <= -step))
	{
		return false;
	}
	// Can't move right of next point
	if((step > 0) && (envPoint < env->size() - 1) && (env->at(envPoint + 1).tick - env->at(envPoint).tick <= step))
	{
		return false;
	}
	return true;
}


BOOL CViewInstrument::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// Ctrl + mouse wheel: envelope zoom.
	if(nFlags == MK_CONTROL)
	{
		// Speed up zoom scrolling by some factor (might need some tuning).
		const float speedUpFactor = std::max(1.0f, m_zoom * 7.0f / ENV_MAX_ZOOM);
		EnvSetZoom(m_zoom + speedUpFactor * (zDelta / WHEEL_DELTA));
	}

	return CModScrollView::OnMouseWheel(nFlags, zDelta, pt);
}


void CViewInstrument::OnXButtonUp(UINT nFlags, UINT nButton, CPoint point)
{
	if(nButton == XBUTTON1)
		OnPrevInstrument();
	else if(nButton == XBUTTON2)
		OnNextInstrument();
	CModScrollView::OnXButtonUp(nFlags, nButton, point);
}


void CViewInstrument::OnEnvLoad()
{
	if(GetInstrumentPtr() == nullptr)
		return;

	FileDialog dlg = OpenFileDialog()
		.DefaultExtension("envelope")
		.ExtensionFilter("Instrument Envelopes (*.envelope)|*.envelope||")
		.WorkingDirectory(TrackerSettings::Instance().PathInstruments.GetWorkingDir());
	if(!dlg.Show(this)) return;
	TrackerSettings::Instance().PathInstruments.SetWorkingDir(dlg.GetWorkingDirectory());

	PrepareUndo("Replace Envelope");
	if(GetDocument()->LoadEnvelope(m_nInstrument, m_nEnv, dlg.GetFirstFile()))
	{
		SetModified(InstrumentHint(m_nInstrument).Envelope(), true);
	} else
	{
		GetDocument()->GetInstrumentUndo().RemoveLastUndoStep(m_nInstrument);
	}
}


void CViewInstrument::OnEnvSave()
{
	const InstrumentEnvelope *env = GetEnvelopePtr();
	if(env == nullptr || env->empty())
	{
		MessageBeep(MB_ICONWARNING);
		return;
	}

	FileDialog dlg = SaveFileDialog()
		.DefaultExtension("envelope")
		.ExtensionFilter("Instrument Envelopes (*.envelope)|*.envelope||")
		.WorkingDirectory(TrackerSettings::Instance().PathInstruments.GetWorkingDir());
	if(!dlg.Show(this)) return;
	TrackerSettings::Instance().PathInstruments.SetWorkingDir(dlg.GetWorkingDirectory());

	if(!GetDocument()->SaveEnvelope(m_nInstrument, m_nEnv, dlg.GetFirstFile()))
	{
		Reporting::Error(MPT_CFORMAT("Unable to save file {}")(dlg.GetFirstFile()), _T("OpenMPT"), this);
	}
}


void CViewInstrument::OnUpdateUndo(CCmdUI *pCmdUI)
{
	CModDoc *pModDoc = GetDocument();
	if((pCmdUI) && (pModDoc))
	{
		pCmdUI->Enable(pModDoc->GetInstrumentUndo().CanUndo(m_nInstrument));
		pCmdUI->SetText(CMainFrame::GetInputHandler()->GetKeyTextFromCommand(kcEditUndo, _T("Undo ") + mpt::ToCString(pModDoc->GetSoundFile().GetCharsetInternal(), pModDoc->GetInstrumentUndo().GetUndoName(m_nInstrument))));
	}
}


void CViewInstrument::OnUpdateRedo(CCmdUI *pCmdUI)
{
	CModDoc *pModDoc = GetDocument();
	if((pCmdUI) && (pModDoc))
	{
		pCmdUI->Enable(pModDoc->GetInstrumentUndo().CanRedo(m_nInstrument));
		pCmdUI->SetText(CMainFrame::GetInputHandler()->GetKeyTextFromCommand(kcEditRedo, _T("Redo ") + mpt::ToCString(pModDoc->GetSoundFile().GetCharsetInternal(), pModDoc->GetInstrumentUndo().GetRedoName(m_nInstrument))));
	}
}


void CViewInstrument::OnEditUndo()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return;
	if(pModDoc->GetInstrumentUndo().Undo(m_nInstrument))
	{
		SetModified(InstrumentHint().Info().Envelope().Names(), true);
	}
}


void CViewInstrument::OnEditRedo()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return;
	if(pModDoc->GetInstrumentUndo().Redo(m_nInstrument))
	{
		SetModified(InstrumentHint().Info().Envelope().Names(), true);
	}
}


INT_PTR CViewInstrument::OnToolHitTest(CPoint point, TOOLINFO *pTI) const
{
	CRect ncRect;
	ClientToScreen(&point);
	const auto ncButton = GetNcButtonAtPoint(point, &ncRect);
	if(ncButton == uint32_max)
		return CModScrollView::OnToolHitTest(point, pTI);
	
	auto buttonID = cLeftBarButtons[ncButton];
	ScreenToClient(&ncRect);

	pTI->hwnd = m_hWnd;
	pTI->uId = buttonID;
	pTI->rect = ncRect;
	CString text = LoadResourceString(buttonID);

	CommandID cmd = kcNull;
	switch(buttonID)
	{
	case ID_ENVSEL_VOLUME: cmd = kcInstrumentEnvelopeSwitchToVolume; break;
	case ID_ENVSEL_PANNING: cmd = kcInstrumentEnvelopeSwitchToPanning; break;
	case ID_ENVSEL_PITCH: cmd = kcInstrumentEnvelopeSwitchToPitch; break;
	case ID_ENVELOPE_VOLUME: cmd = kcInstrumentEnvelopeToggleVolume; break;
	case ID_ENVELOPE_PANNING: cmd = kcInstrumentEnvelopeTogglePanning; break;
	case ID_ENVELOPE_PITCH: cmd = kcInstrumentEnvelopeTogglePitch; break;
	case ID_ENVELOPE_FILTER: cmd = kcInstrumentEnvelopeToggleFilter; break;
	case ID_ENVELOPE_SETLOOP: cmd = kcInstrumentEnvelopeToggleLoop; break;
	case ID_ENVELOPE_SUSTAIN: cmd = kcInstrumentEnvelopeToggleSustain; break;
	case ID_ENVELOPE_CARRY: cmd = kcInstrumentEnvelopeToggleCarry; break;
	case ID_INSTRUMENT_SAMPLEMAP: cmd = kcInsNoteMapEditSampleMap; break;
	case ID_ENVELOPE_ZOOM_IN: cmd = kcInstrumentEnvelopeZoomIn; break;
	case ID_ENVELOPE_ZOOM_OUT: cmd = kcInstrumentEnvelopeZoomOut; break;
	case ID_ENVELOPE_LOAD: cmd = kcInstrumentEnvelopeLoad; break;
	case ID_ENVELOPE_SAVE: cmd = kcInstrumentEnvelopeSave; break;
	}
	if(cmd != kcNull)
	{
		auto keyText = CMainFrame::GetInputHandler()->m_activeCommandSet->GetKeyTextFromCommand(cmd, 0);
		if(!keyText.IsEmpty())
			text += MPT_CFORMAT(" ({})")(keyText);
	}

	// MFC will free() the text
	auto size = text.GetLength() + 1;
	TCHAR *textP = static_cast<TCHAR *>(calloc(size, sizeof(TCHAR)));
	std::copy(text.GetString(), text.GetString() + size, textP);
	pTI->lpszText = textP;

	return buttonID;
}


// Accessible description for screen readers
HRESULT CViewInstrument::get_accName(VARIANT varChild, BSTR *pszName)
{
	const InstrumentEnvelope *env = GetEnvelopePtr();
	if(env == nullptr)
		return CModScrollView::get_accName(varChild, pszName);

	const TCHAR *typeStr = _T("");
	switch(m_nEnv)
	{
	case ENV_VOLUME: typeStr = _T("Volume"); break;
	case ENV_PANNING: typeStr = _T("Panning"); break;
	case ENV_PITCH: typeStr = env->dwFlags[ENV_FILTER] ? _T("Filter") : _T("Pitch"); break;
	}

	CString str;
	if(env->empty() || m_nDragItem == 0)
	{
		str = typeStr;
		if(env->empty())
			str += _T(" envelope has no points");
		else
			str += MPT_CFORMAT(" envelope, {} point{}")(env->size(), env->size() == 1 ? CString(_T("")) : CString(_T("s")));
	} else
	{
		bool isEnvPoint = false;
		auto point = DragItemToEnvPoint();
		auto tick = EnvGetTick(point);
		switch(m_nDragItem)
		{
		case ENV_DRAGLOOPSTART: str = _T("Loop start"); break;
		case ENV_DRAGLOOPEND: str = _T("Loop end"); break;
		case ENV_DRAGSUSTAINSTART: str = _T("Sustain loop start"); break;
		case ENV_DRAGSUSTAINEND: str = _T("Sustain loop end"); break;
		default: isEnvPoint = true;
		}
		if(!isEnvPoint)
		{
			str += MPT_CFORMAT(" at point {}, tick {}")(point + 1, tick);
		} else
		{
			str = MPT_CFORMAT("Point {}, tick {}, {} {}")(point + 1, tick, CString(typeStr), EnvValueToString(EnvGetTick(point), EnvGetValue(point)));
			if(env->dwFlags[ENV_LOOP])
			{
				if(point == env->nLoopStart)
					str += _T(", loop start");
				if(point == env->nLoopEnd)
					str += _T(", loop end");
			}
			if(env->dwFlags[ENV_SUSTAIN])
			{
				if(point == env->nLoopStart)
					str += _T(", sustain loop start");
				if(point == env->nLoopEnd)
					str += _T(", sustain loop end");
			}
			if(env->nReleaseNode == point)
				str += _T(", release node");
		}
	}

	*pszName = str.AllocSysString();
	return S_OK;
}

OPENMPT_NAMESPACE_END
