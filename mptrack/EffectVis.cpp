/*
 * EffectVis.cpp
 * -------------
 * Purpose: Implementation of parameter visualisation dialog.
 * Notes  : (currenlty none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "EffectVis.h"
#include "Childfrm.h"
#include "Globals.h"
#include "HighDPISupport.h"
#include "InputHandler.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Mptrack.h"
#include "resource.h"
#include "View_pat.h"


OPENMPT_NAMESPACE_BEGIN

CEffectVis::Action CEffectVis::m_nAction = CEffectVis::Action::OverwriteFX;

BEGIN_MESSAGE_MAP(CEffectVis, DialogBase)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_CBN_SELCHANGE(IDC_VISACTION,     &CEffectVis::OnActionChanged)
	ON_CBN_SELCHANGE(IDC_VISEFFECTLIST, &CEffectVis::OnEffectChanged)
END_MESSAGE_MAP()


CEffectVis::CEffectVis(CViewPattern *pViewPattern, ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX nchn, CModDoc &modDoc, PATTERNINDEX pat)
	: m_ModDoc(modDoc)
	, m_SndFile(modDoc.GetSoundFile())
	, m_pViewPattern(pViewPattern)
	, m_effectInfo(modDoc.GetSoundFile())
{
	m_nFillEffect = m_effectInfo.GetIndexFromEffect(CMD_SMOOTHMIDI, 0);
	m_templatePCNote.Set(NOTE_PCS, 1, 0, 0);
	UpdateSelection(startRow, endRow, nchn, pat);
}


void CEffectVis::DoDataExchange(CDataExchange* pDX)
{
	DialogBase::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VISSTATUS, m_edVisStatus);
	DDX_Control(pDX, IDC_VISACTION, m_cmbActionList);
}


void CEffectVis::OnActionChanged()
{
	const auto oldActionWasPC = m_nAction == Action::FillPC || m_nAction == Action::OverwritePC;
	m_nAction = static_cast<Action>(m_cmbActionList.GetItemData(m_cmbActionList.GetCurSel()));
	const auto newActionIsPC = m_nAction == Action::FillPC || m_nAction == Action::OverwritePC;

	m_cmbEffectList.EnableWindow((m_nAction == Action::Preserve) ? FALSE : TRUE);

	if(oldActionWasPC != newActionIsPC)
		UpdateEffectList();
}


void CEffectVis::OnEffectChanged()
{
	if(m_nAction == Action::FillPC || m_nAction == Action::OverwritePC)
		m_templatePCNote.SetValueVolCol(static_cast<uint16>(m_cmbEffectList.GetCurSel()));
	else
		m_nFillEffect = static_cast<UINT>(m_cmbEffectList.GetItemData(m_cmbEffectList.GetCurSel()));
}


void CEffectVis::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	ShowVis(&dc);
}


uint16 CEffectVis::GetParam(ROWINDEX row) const
{
	uint16 paramValue = 0;

	if(m_SndFile.Patterns.IsValidPat(m_nPattern))
	{
		const ModCommand &m = *m_SndFile.Patterns[m_nPattern].GetpModCommand(row, m_nChan);
		if (m.IsPcNote())
		{
			paramValue = m.GetValueEffectCol();
		} else
		{
			paramValue = m.param;
		}
	}

	return paramValue;
}


// Sets a row's param value based on the vertical cursor position.
// Sets either plain pattern effect parameter or PC note parameter
// as appropriate, depending on contents of row.
void CEffectVis::SetParamFromY(ROWINDEX row, int y)
{
	if(!m_SndFile.Patterns.IsValidPat(m_nPattern))
		return;

	ModCommand &m = *m_SndFile.Patterns[m_nPattern].GetpModCommand(row, m_nChan);
	if(IsPcNote(row))
	{
		uint16 param = ScreenYToPCParam(y);
		m.SetValueEffectCol(param);
	} else
	{
		ModCommand::PARAM param = ScreenYToFXParam(y);
		// Cap the parameter value as appropriate, based on effect type (e.g. Zxx gets capped to [0x00,0x7F])
		m_effectInfo.GetEffectFromIndex(m_effectInfo.GetIndexFromEffect(m.command, param), param);
		m.param = param;
	}
}



EffectCommand CEffectVis::GetCommand(ROWINDEX row) const
{
	if(m_SndFile.Patterns.IsValidPat(m_nPattern))
		return static_cast<EffectCommand>(m_SndFile.Patterns[m_nPattern].GetpModCommand(row, m_nChan)->command);
	else
		return CMD_NONE;
}


void CEffectVis::SetCommand(ROWINDEX row, EffectCommand command)
{
	if(m_SndFile.Patterns.IsValidPat(m_nPattern))
	{
		ModCommand &m = *m_SndFile.Patterns[m_nPattern].GetpModCommand(row, m_nChan);
		if(m.IsPcNote())
		{
			// Clear PC note
			m.note = 0;
			m.instr = 0;
			m.volcmd = VOLCMD_NONE;
			m.vol = 0;
		}
		m.command = command;
	}
}


std::pair<ROWINDEX, ROWINDEX> CEffectVis::GetTimeSignature() const
{
	ROWINDEX beat = m_SndFile.m_nDefaultRowsPerBeat, measure = m_SndFile.m_nDefaultRowsPerMeasure;
	if(m_SndFile.Patterns.IsValidIndex(m_nPattern) && m_SndFile.Patterns[m_nPattern].GetOverrideSignature())
	{
		beat = m_SndFile.Patterns[m_nPattern].GetRowsPerBeat();
		measure = m_SndFile.Patterns[m_nPattern].GetRowsPerMeasure();
	}
	return std::make_pair(beat, measure);
}


int CEffectVis::RowToScreenX(ROWINDEX row) const
{
	if ((row >= m_startRow) || (row <= m_endRow))
		return mpt::saturate_round<int>(m_rcDraw.left + m_innerBorder + (row - m_startRow) * m_pixelsPerRow);
	return -1;
}


int CEffectVis::RowToScreenY(ROWINDEX row) const
{
	int screenY = -1;

	if(m_SndFile.Patterns.IsValidPat(m_nPattern))
	{
		const ModCommand &m = *m_SndFile.Patterns[m_nPattern].GetpModCommand(row, m_nChan);
		if(m.IsPcNote())
		{
			uint16 paramValue = m.GetValueEffectCol();
			screenY = PCParamToScreenY(paramValue);
		} else
		{
			uint16 paramValue = m.param;
			screenY = FXParamToScreenY(paramValue);
		}
	}

	return screenY;
}


int CEffectVis::FXParamToScreenY(uint16 param) const
{
	if(param >= 0x00 && param <= 0xFF)
		return mpt::saturate_round<int>(m_rcDraw.bottom - param * m_pixelsPerFXParam);
	return -1;
}


int CEffectVis::PCParamToScreenY(uint16 param) const
{
	if(param >= 0x00 && param <= ModCommand::maxColumnValue)
		return mpt::saturate_round<int>(m_rcDraw.bottom - param*m_pixelsPerPCParam);
	return -1;
}


ModCommand::PARAM CEffectVis::ScreenYToFXParam(int y) const
{
	if(y <= FXParamToScreenY(0xFF))
		return 0xFF;

	if(y >= FXParamToScreenY(0x00))
		return 0x00;

	return mpt::saturate_round<ModCommand::PARAM>((m_rcDraw.bottom - y) / m_pixelsPerFXParam);
}


uint16 CEffectVis::ScreenYToPCParam(int y) const
{
	if(y <= PCParamToScreenY(ModCommand::maxColumnValue))
		return ModCommand::maxColumnValue;

	if(y >= PCParamToScreenY(0x00))
		return 0x00;

	return mpt::saturate_round<uint16>((m_rcDraw.bottom - y) / m_pixelsPerPCParam);
}


ROWINDEX CEffectVis::ScreenXToRow(int x) const
{
	if(x <= RowToScreenX(m_startRow))
		return m_startRow;

	if(x >= RowToScreenX(m_endRow))
		return m_endRow;

	return mpt::saturate_round<ROWINDEX>(m_startRow + (x - m_innerBorder) / m_pixelsPerRow);
}


void CEffectVis::DrawGrid()
{
	// Lots of room for optimisation here.
	// Draw vertical grid lines
	const auto [beat, measure] = GetTimeSignature();

	m_dcGrid.FillSolidRect(&m_rcDraw, 0);
	auto oldPen = m_dcGrid.SelectStockObject(DC_PEN);
	for(ROWINDEX row = m_startRow; row <= m_endRow; row++)
	{
		if(row % measure == 0)
			m_dcGrid.SetDCPenColor(RGB(0xFF, 0xFF, 0xFF));
		else if(row % beat == 0)
			m_dcGrid.SetDCPenColor(RGB(0x99, 0x99, 0x99));
		else
			m_dcGrid.SetDCPenColor(RGB(0x55, 0x55, 0x55));
		int x1 = RowToScreenX(row);
		m_dcGrid.MoveTo(x1, m_rcDraw.top);
		m_dcGrid.LineTo(x1, m_rcDraw.bottom);
	}

	// Draw horizontal grid lines
	constexpr UINT numHorizontalLines = 4;
	for(UINT i = 0; i < numHorizontalLines; i++)
	{
		COLORREF c = 0;
		switch(i % 4)
		{
		case 0: c = RGB(0x00, 0x00, 0x00); break;
		case 1: c = RGB(0x40, 0x40, 0x40); break;
		case 2: c = RGB(0x80, 0x80, 0x80); break;
		case 3: c = RGB(0xCC, 0xCC, 0xCC); break;
		}
		m_dcGrid.SetDCPenColor(c);
		int y1 = m_rcDraw.bottom / numHorizontalLines * i;
		m_dcGrid.MoveTo(m_rcDraw.left + m_innerBorder, y1);
		m_dcGrid.LineTo(m_rcDraw.right - m_innerBorder, y1);
	}
	m_dcGrid.SelectObject(oldPen);
}


void CEffectVis::SetPlayCursor(PATTERNINDEX nPat, ROWINDEX nRow)
{
	if(nPat == m_nPattern && nRow == m_nOldPlayPos)
		return;

	if(m_nOldPlayPos >= m_startRow && m_nOldPlayPos <= m_endRow)
	{
		// erase current playpos
		int x1 = RowToScreenX(m_nOldPlayPos);
		m_dcPlayPos.SelectStockObject(BLACK_PEN);
		m_dcPlayPos.MoveTo(x1,m_rcDraw.top);
		m_dcPlayPos.LineTo(x1,m_rcDraw.bottom);
	}

	if((nRow < m_startRow) || (nRow > m_endRow) || (nPat != m_nPattern))
		return;

	int x1 = RowToScreenX(nRow);
	m_dcPlayPos.SelectStockObject(DC_PEN);
	m_dcPlayPos.SetDCPenColor(TrackerSettings::Instance().rgbCustomColors[MODCOLOR_SAMPLE]);
	m_dcPlayPos.MoveTo(x1,m_rcDraw.top);
	m_dcPlayPos.LineTo(x1,m_rcDraw.bottom);

	m_nOldPlayPos = nRow;
	InvalidateRect(NULL, FALSE);
}


void CEffectVis::ShowVis(CDC *pDC)
{
	if (m_forceRedraw)
	{
		m_forceRedraw = false;

		// if we already have a memory dc, destroy it (this occurs for a re-size)
		if (m_dcGrid.m_hDC)
		{
			m_dcGrid.SelectObject(m_pbOldGrid);
			m_dcGrid.DeleteDC();

			m_dcNodes.SelectObject(m_pbOldNodes);
			m_dcNodes.DeleteDC();

			m_dcPlayPos.SelectObject(m_pbOldPlayPos);
			m_dcPlayPos.DeleteDC();

			m_bPlayPos.DeleteObject();
			m_bGrid.DeleteObject();
			m_bNodes.DeleteObject();
		}

		// create a memory based dc for drawing the grid
		m_dcGrid.CreateCompatibleDC(pDC);
		m_bGrid.CreateCompatibleBitmap(pDC, m_rcDraw.Width(), m_rcDraw.Height());
		m_pbOldGrid = *m_dcGrid.SelectObject(&m_bGrid);

		// create a memory based dc for drawing the nodes
		m_dcNodes.CreateCompatibleDC(pDC);
		m_bNodes.CreateCompatibleBitmap(pDC, m_rcDraw.Width(), m_rcDraw.Height());
		m_pbOldNodes = *m_dcNodes.SelectObject(&m_bNodes);

		// create a memory based dc for drawing the nodes
		m_dcPlayPos.CreateCompatibleDC(pDC);
		m_bPlayPos.CreateCompatibleBitmap(pDC, m_rcDraw.Width(), m_rcDraw.Height());
		m_pbOldPlayPos = *m_dcPlayPos.SelectObject(&m_bPlayPos);

		SetPlayCursor(m_nPattern, m_nOldPlayPos);
		DrawGrid();
		DrawNodes();
	}

	// display the new image, combining the nodes with the grid
	ShowVisImage(pDC);

}


void CEffectVis::ShowVisImage(CDC *pDC)
{
	// to avoid flicker, establish a memory dc, draw to it
	// and then BitBlt it to the destination "pDC"
	CDC memDC;
	memDC.CreateCompatibleDC(pDC);
	if (!memDC)
		return;

	CBitmap memBitmap;
	memBitmap.CreateCompatibleBitmap(pDC, m_rcDraw.Width(), m_rcDraw.Height());
	CBitmap *oldBitmap = memDC.SelectObject(&memBitmap);

	// make sure we have the bitmaps
	if (!m_dcGrid.m_hDC)
		return;
	if (!m_dcNodes.m_hDC)
		return;
	if (!m_dcPlayPos.m_hDC)
		return;

	if (memDC.m_hDC != nullptr)
	{
		// draw the grid
		memDC.BitBlt(0, 0, m_rcDraw.Width(), m_rcDraw.Height(), &m_dcGrid, 0, 0, SRCCOPY);

		// merge the nodes image with the grid
		memDC.TransparentBlt(0, 0, m_rcDraw.Width(), m_rcDraw.Height(), &m_dcNodes, 0, 0, m_rcDraw.Width(), m_rcDraw.Height(), 0x00000000);
		// further merge the playpos
		memDC.TransparentBlt(0, 0, m_rcDraw.Width(), m_rcDraw.Height(), &m_dcPlayPos, 0, 0, m_rcDraw.Width(), m_rcDraw.Height(), 0x00000000);

		// copy the resulting bitmap to the destination
		pDC->BitBlt(0, 0, m_rcDraw.Width(), m_rcDraw.Height(), &memDC, 0, 0, SRCCOPY);
	}

	memDC.SelectObject(oldBitmap);

}


void CEffectVis::DrawNodes()
{
	if(m_rcDraw.IsRectEmpty())
		return;

	if(m_nDragItem < m_startRow || m_nDragItem > m_endRow)
		m_nDragItem = m_startRow;

	//Draw
	const int lineWidth = HighDPISupport::ScalePixels(1, m_hWnd);
	const int nodeSizeHalf = m_nodeSizeHalf;
	const int nodeSizeHalf2 = nodeSizeHalf - lineWidth + 1;
	const int nodeSize = 2 * nodeSizeHalf + 1;

	//erase
	if ((ROWINDEX)m_nRowToErase < m_startRow || m_nParamToErase < 0)
	{
		m_dcNodes.FillSolidRect(&m_rcDraw, 0);
	} else
	{
		int x = RowToScreenX(m_nRowToErase);
		CRect r(x - nodeSizeHalf, m_rcDraw.top, x + nodeSizeHalf + 1, m_rcDraw.bottom);
		m_dcNodes.FillSolidRect(&r, 0);
	}

	for (ROWINDEX row = m_startRow; row <= m_endRow; row++)
	{
		COLORREF col = IsPcNote(row) ? RGB(0xFF, 0xFF, 0x00) : RGB(0xD0, 0xFF, 0xFF);
		int x = RowToScreenX(row);
		int y = RowToScreenY(row);
		m_dcNodes.FillSolidRect(x - nodeSizeHalf, y - nodeSizeHalf, nodeSize, lineWidth, col);  // Top
		m_dcNodes.FillSolidRect(x + nodeSizeHalf2, y - nodeSizeHalf, lineWidth, nodeSize, col); // Right
		m_dcNodes.FillSolidRect(x - nodeSizeHalf, y + nodeSizeHalf2, nodeSize, lineWidth, col); // Bottom
		m_dcNodes.FillSolidRect(x - nodeSizeHalf, y - nodeSizeHalf, lineWidth, nodeSize, col);  // Left
	}

}

void CEffectVis::InvalidateRow(int row)
{
	if (((UINT)row < m_startRow) ||  ((UINT)row > m_endRow)) return;

//It seems this optimisation doesn't work properly yet.	Disable in Update()

	int x = RowToScreenX(row);
	RECT invalidated;
	invalidated.bottom = m_rcDraw.bottom;
	invalidated.top = m_rcDraw.top;
	invalidated.left = x - m_nodeSizeHalf;
	invalidated.right = x + m_nodeSizeHalf + 1;
	InvalidateRect(&invalidated, FALSE);
}


void CEffectVis::OpenEditor(CWnd *parent)
{
	Create(IDD_EFFECTVISUALIZER, parent);
	m_forceRedraw = true;

	if(TrackerSettings::Instance().effectVisWidth > 0 && TrackerSettings::Instance().effectVisHeight > 0)
	{
		WINDOWPLACEMENT wnd;
		wnd.length = sizeof(wnd);
		GetWindowPlacement(&wnd);
		wnd.showCmd = SW_SHOWNOACTIVATE;
		CRect rect = wnd.rcNormalPosition;
		const auto dpi = HighDPISupport::GetDpiForWindow(m_hWnd);
		if(TrackerSettings::Instance().effectVisX > int32_min && TrackerSettings::Instance().effectVisY > int32_min)
		{
			CRect mainRect;
			CMainFrame::GetMainFrame()->GetWindowRect(mainRect);
			rect.left = mainRect.left + MulDiv(TrackerSettings::Instance().effectVisX, dpi, 96);
			rect.top = mainRect.top + MulDiv(TrackerSettings::Instance().effectVisY, dpi, 96);
		}
		rect.right = rect.left + MulDiv(TrackerSettings::Instance().effectVisWidth, dpi, 96);
		rect.bottom = rect.top + MulDiv(TrackerSettings::Instance().effectVisHeight, dpi, 96);
		wnd.rcNormalPosition = rect;
		SetWindowPlacement(&wnd);
	}

	ShowWindow(SW_SHOW);
}


void CEffectVis::OnClose()
{
	DoClose();
}


void CEffectVis::OnOK()
{
	OnClose();
}


void CEffectVis::OnCancel()
{
	OnClose();
}


void CEffectVis::DoClose()
{
	WINDOWPLACEMENT wnd;
	wnd.length = sizeof(wnd);
	GetWindowPlacement(&wnd);
	CRect mainRect;
	CMainFrame::GetMainFrame()->GetWindowRect(mainRect);

	CRect rect = wnd.rcNormalPosition;
	rect.MoveToXY(rect.left - mainRect.left, rect.top - mainRect.top);
	const auto dpi = HighDPISupport::GetDpiForWindow(m_hWnd);
	TrackerSettings::Instance().effectVisWidth = MulDiv(rect.Width(), 96, dpi);
	TrackerSettings::Instance().effectVisHeight = MulDiv(rect.Height(), 96, dpi);
	TrackerSettings::Instance().effectVisX = MulDiv(rect.left, 96, dpi);
	TrackerSettings::Instance().effectVisY = MulDiv(rect.top, 96, dpi);

	m_dcGrid.SelectObject(m_pbOldGrid);
	m_dcGrid.DeleteDC();
	m_dcNodes.SelectObject(m_pbOldNodes);
	m_dcNodes.DeleteDC();
	m_dcPlayPos.SelectObject(m_pbOldPlayPos);
	m_dcPlayPos.DeleteDC();

	m_bGrid.DeleteObject();
	m_bNodes.DeleteObject();
	m_bPlayPos.DeleteObject();

	DestroyWindow();
}


void CEffectVis::PostNcDestroy()
{
	m_pViewPattern->m_pEffectVis = nullptr;
}


void CEffectVis::OnSize(UINT nType, int cx, int cy)
{
	MPT_UNREFERENCED_PARAMETER(nType);
	MPT_UNREFERENCED_PARAMETER(cx);
	MPT_UNREFERENCED_PARAMETER(cy);

	m_nodeSizeHalf = HighDPISupport::ScalePixels(3, m_hWnd);
	m_marginBottom = HighDPISupport::ScalePixels(20, m_hWnd);
	m_innerBorder = HighDPISupport::ScalePixels(4, m_hWnd);

	GetClientRect(&m_rcFullWin);
	m_rcDraw.SetRect(m_rcFullWin.left, m_rcFullWin.top, m_rcFullWin.right, m_rcFullWin.bottom - m_marginBottom);

	const int actionListWidth = HighDPISupport::ScalePixels(170, m_hWnd);
	const int commandListWidth = HighDPISupport::ScalePixels(160, m_hWnd);

	auto dwp = ::BeginDeferWindowPos(3);
	if (IsWindow(m_edVisStatus.m_hWnd))
		::DeferWindowPos(dwp, m_edVisStatus, nullptr, m_rcFullWin.left, m_rcDraw.bottom, m_rcFullWin.right - commandListWidth - actionListWidth, m_rcFullWin.bottom - m_rcDraw.bottom, SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);
	if (IsWindow(m_cmbActionList))
		::DeferWindowPos(dwp, m_cmbActionList, nullptr, m_rcFullWin.right - commandListWidth - actionListWidth, m_rcDraw.bottom, actionListWidth, m_rcFullWin.bottom - m_rcDraw.bottom, SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);
	if (IsWindow(m_cmbEffectList))
		::DeferWindowPos(dwp, m_cmbEffectList, nullptr, m_rcFullWin.right - commandListWidth, m_rcDraw.bottom, commandListWidth, m_rcFullWin.bottom - m_rcDraw.bottom, SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);
	::EndDeferWindowPos(dwp);

	if(m_nRows)
		m_pixelsPerRow = (float)(m_rcDraw.Width() - m_innerBorder * 2) / (float)m_nRows;
	else
		m_pixelsPerRow = 1;
	m_pixelsPerFXParam = (float)(m_rcDraw.Height())/(float)0xFF;
	m_pixelsPerPCParam = (float)(m_rcDraw.Height())/(float)ModCommand::maxColumnValue;
	m_forceRedraw = true;
	InvalidateRect(NULL, FALSE);	//redraw everything
}

void CEffectVis::Update()
{
	DrawNodes();
	if (::IsWindow(m_hWnd))
	{
		OnPaint();
		if (m_nRowToErase<0)
			InvalidateRect(NULL, FALSE);	// redraw everything
		else
		{
			InvalidateRow(m_nRowToErase);
			m_nParamToErase=-1;
			m_nRowToErase=-1;
		}

	}
}

void CEffectVis::UpdateSelection(ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX nchn, PATTERNINDEX pat)
{
	m_startRow = startRow;
	m_endRow = endRow;
	m_nRows = endRow - startRow;
	m_nChan = nchn;
	m_nPattern = pat;

	//Check pattern, start row and channel exist
	if(!m_SndFile.Patterns.IsValidPat(m_nPattern) || !m_SndFile.Patterns[m_nPattern].IsValidRow(m_startRow) || m_nChan >= m_SndFile.GetNumChannels())
	{
		DoClose();
		return;
	}

	//Check end exists
	if(!m_SndFile.Patterns[m_nPattern].IsValidRow(m_endRow))
	{
		m_endRow = m_SndFile.Patterns[m_nPattern].GetNumRows() - 1;
		m_nRows = m_endRow - startRow;
	}

	if(m_nDragItem < m_startRow || m_nDragItem > m_endRow)
		m_nDragItem = m_startRow;

	if(m_nRows)
		m_pixelsPerRow = (float)(m_rcDraw.Width() - m_innerBorder * 2) / (float)m_nRows;
	else
		m_pixelsPerRow = 1;
	m_pixelsPerFXParam = (float)(m_rcDraw.Height())/(float)0xFF;
	m_pixelsPerPCParam = (float)(m_rcDraw.Height())/(float)ModCommand::maxColumnValue;

	m_forceRedraw = true;
	Update();

}

void CEffectVis::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (!(m_dwStatus & FXVSTATUS_LDRAGGING))
	{
		SetFocus();
		SetCapture();

		m_nDragItem = ScreenXToRow(point.x);
		m_dwStatus |= FXVSTATUS_RDRAGGING;
		m_ModDoc.GetPatternUndo().PrepareUndo(static_cast<PATTERNINDEX>(m_nPattern), m_nChan, m_nDragItem, 1, 1, "Parameter Editor entry");
		OnMouseMove(nFlags, point);
	}

	DialogBase::OnRButtonDown(nFlags, point);
}

void CEffectVis::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_dwStatus = 0x00;
	DialogBase::OnRButtonUp(nFlags, point);
}

void CEffectVis::OnMouseMove(UINT nFlags, CPoint point)
{
	DialogBase::OnMouseMove(nFlags, point);

	ROWINDEX row = ScreenXToRow(point.x);

	if(m_dwStatus & FXVSTATUS_RDRAGGING)
	{
		m_nRowToErase = static_cast<int>(m_nDragItem);
		m_nParamToErase = GetParam(m_nDragItem);

		MakeChange(m_nDragItem, point.y);
	} else if ((m_dwStatus & FXVSTATUS_LDRAGGING))
	{
		// Interpolate if we detect that rows have been skipped but the left mouse button was not released.
		// This ensures we produce a smooth curve even when we are not notified of mouse movements at a high frequency (e.g. if CPU usage is high)
		const int steps = std::abs((int)row - (int)m_nLastDrawnRow);
		if (m_nLastDrawnRow != ROWINDEX_INVALID && m_nLastDrawnRow > m_startRow && steps > 1)
		{
			int direction = ((int)(row - m_nLastDrawnRow) > 0) ? 1 : -1;
			float factor = (float)(point.y - m_nLastDrawnY)/(float)steps + 0.5f;

			int currentRow;
			for (int i=1; i<=steps; i++)
			{
				currentRow = m_nLastDrawnRow+(direction*i);
				int interpolatedY = mpt::saturate_round<int>(m_nLastDrawnY + ((float)i * factor));
				MakeChange(currentRow, interpolatedY);
			}

			//Don't use single value update
			m_nRowToErase = -1;
			m_nParamToErase = -1;
		} else
		{
			m_nRowToErase = -1;
			m_nParamToErase = -1;
			MakeChange(row, point.y);
		}

		// Remember last modified point in case we need to interpolate
		m_nLastDrawnRow = row;
		m_nLastDrawnY = point.y;
	}
	//update status bar
	CString status;
	CString effectName;
	uint16 paramValue;


	if (IsPcNote(row))
	{
		paramValue = ScreenYToPCParam(point.y);
		effectName.Format(_T("%s"), _T("Param Control")); // TODO - show smooth & plug+param
	} else
	{
		paramValue = ScreenYToFXParam(point.y);
		m_effectInfo.GetEffectInfo(m_effectInfo.GetIndexFromEffect(GetCommand(row), ModCommand::PARAM(GetParam(row))), &effectName, true);
	}

	status.Format(_T("Pat: %d\tChn: %d\tRow: %d\tVal: %02X (%03d) [%s]"),
				m_nPattern, m_nChan+1, static_cast<signed int>(row), paramValue, paramValue, effectName.GetString());
	m_edVisStatus.SetWindowText(status);
}

void CEffectVis::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (!(m_dwStatus & FXVSTATUS_RDRAGGING))
	{
		SetFocus();
		SetCapture();

		m_nDragItem = ScreenXToRow(point.x);
		m_dwStatus |= FXVSTATUS_LDRAGGING;
		m_ModDoc.GetPatternUndo().PrepareUndo(static_cast<PATTERNINDEX>(m_nPattern), m_nChan, m_startRow, 1, m_endRow - m_startRow + 1, "Parameter Editor entry");
		OnMouseMove(nFlags, point);
	}

	DialogBase::OnLButtonDown(nFlags, point);
}

void CEffectVis::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_dwStatus = 0x00;
	DialogBase::OnLButtonUp(nFlags, point);
	m_nLastDrawnRow = ROWINDEX_INVALID;
}


BOOL CEffectVis::OnInitDialog()
{
	DialogBase::OnInitDialog();

	// If first selected row is a PC event (or some other row but there aren't any other effects), default to PC note overwrite mode
	// and use it as a template for new PC notes that will be created via the visualiser.
	bool isPCevent = IsPcNote(m_startRow);
	ROWINDEX templatePCRow = m_startRow;
	if(!isPCevent)
	{
		for(ROWINDEX row = m_startRow; row <= m_endRow; row++)
		{
			if(IsPcNote(row))
			{
				isPCevent = true;
				templatePCRow = row;
			} else if(GetCommand(row) != CMD_NONE)
			{
				isPCevent = false;
				break;
			}
		}
	}

	if(isPCevent)
	{
		m_nAction = Action::OverwritePC;
		if(m_SndFile.Patterns.IsValidPat(m_nPattern))
		{
			ModCommand &m = *m_SndFile.Patterns[m_nPattern].GetpModCommand(templatePCRow, m_nChan);
			m_templatePCNote.Set(m.note, m.instr, m.GetValueVolCol(), 0);
		}
	} else
	{
		// Otherwise, default to FX overwrite and
		// use effect of first selected row as default effect type
		m_nAction = Action::OverwriteFX;
		m_nFillEffect = m_effectInfo.GetIndexFromEffect(GetCommand(m_startRow), ModCommand::PARAM(GetParam(m_startRow)));
		if(m_nFillEffect < 0 || m_nFillEffect >= MAX_EFFECTS)
			m_nFillEffect = m_effectInfo.GetIndexFromEffect(CMD_SMOOTHMIDI, 0);
	}


	UpdateEffectList();

	m_cmbActionList.ResetContent();
	m_cmbActionList.SetItemData(m_cmbActionList.AddString(_T("Overwrite with effect:")), static_cast<DWORD_PTR>(Action::OverwriteFX));
	m_cmbActionList.SetItemData(m_cmbActionList.AddString(_T("Overwrite effect next to note:")), static_cast<DWORD_PTR>(Action::OverwriteFXWithNote));
	m_cmbActionList.SetItemData(m_cmbActionList.AddString(_T("Fill blanks with effect:")), static_cast<DWORD_PTR>(Action::FillFX));
	if(m_ModDoc.GetModType() == MOD_TYPE_MPT || isPCevent)
	{
		m_cmbActionList.SetItemData(m_cmbActionList.AddString(_T("Overwrite with PC note")), static_cast<DWORD_PTR>(Action::OverwritePC));
		m_cmbActionList.SetItemData(m_cmbActionList.AddString(_T("Fill blanks with PC note")), static_cast<DWORD_PTR>(Action::FillPC));
	}
	m_cmbActionList.SetItemData(m_cmbActionList.AddString(_T("Never change effect type")), static_cast<DWORD_PTR>(Action::Preserve));

	m_cmbActionList.SetCurSel(static_cast<int>(m_nAction));
	return true;
}


void CEffectVis::UpdateEffectList()
{
	const bool fillPlugParams = m_nAction == Action::FillPC || m_nAction == Action::OverwritePC;
	CRect rect{0, 0, 16, HighDPISupport::ScalePixels(16, m_hWnd)};
	if(m_cmbEffectList)
	{
		m_cmbEffectList.GetWindowRect(rect);
		ScreenToClient(rect);
		m_cmbEffectList.DestroyWindow();
	}
	// Can't change CBS_SORT style after creation...
	m_cmbEffectList.Create(CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP | WS_VISIBLE | (fillPlugParams ? 0 : CBS_SORT), rect, this, IDC_VISEFFECTLIST);
	m_cmbEffectList.SetRedraw(FALSE);
	m_cmbEffectList.SetFont(GetFont(), FALSE);
	CString s;
	if(fillPlugParams)
	{
		IMixPlugin *plugin = nullptr;
		if(m_templatePCNote.instr > 0 && m_templatePCNote.instr <= MAX_MIXPLUGINS)
			plugin = m_SndFile.m_MixPlugins[m_templatePCNote.instr - 1].pMixPlugin;

		if(plugin)
		{
			AddPluginParameternamesToCombobox(m_cmbEffectList, *plugin);
		} else
		{
			for(PlugParamIndex i = 0; i < ModCommand::maxColumnValue; i++)
			{
				s.Format(_T("Parameter %u"), i);
				m_cmbEffectList.SetItemData(m_cmbEffectList.AddString(s), i);
			}
		}
		m_cmbEffectList.SetCurSel(m_templatePCNote.GetValueVolCol());
	} else
	{
		const UINT numfx = m_effectInfo.GetNumEffects();
		for(UINT i = 0; i < numfx; i++)
		{
			if(m_effectInfo.GetEffectInfo(i, &s, true))
			{
				int k = m_cmbEffectList.AddString(s);
				m_cmbEffectList.SetItemData(k, i);
				if(static_cast<int>(i) == m_nFillEffect)
					m_cmbEffectList.SetCurSel(k);
			}
		}
	}
	m_cmbEffectList.SetRedraw(TRUE);
	m_cmbEffectList.Invalidate(FALSE);
}

void CEffectVis::MakeChange(ROWINDEX row, int y)
{
	if(!m_SndFile.Patterns.IsValidPat(m_nPattern))
		return;

	ModCommand &m = *m_SndFile.Patterns[m_nPattern].GetpModCommand(row, m_nChan);

	switch(m_nAction)
	{
		case Action::FillFX:
			// Only set command if there isn't a command already at this row and it's not a PC note
			if (GetCommand(row) == CMD_NONE && !IsPcNote(row))
			{
				SetCommand(row, m_effectInfo.GetEffectFromIndex(m_nFillEffect));
			}
			// Always set param
			SetParamFromY(row, y);
			break;

		case Action::OverwriteFXWithNote:
			if(!m_SndFile.Patterns.IsValidPat(m_nPattern))
				break;
			if(!m_SndFile.Patterns[m_nPattern].GetpModCommand(row, m_nChan)->IsNote())
				break;
			[[fallthrough]];
		case Action::OverwriteFX:
			// Always set command and param. Blows away any PC notes.
			SetCommand(row, m_effectInfo.GetEffectFromIndex(m_nFillEffect));
			SetParamFromY(row, y);
			break;

		case Action::FillPC:
			// Fill only empty slots with PC notes - leave other slots alone.
			if (m.IsEmpty())
			{
				SetPcNote(row);
			}
			// Always set param
			SetParamFromY(row, y);
			break;

		case Action::OverwritePC:
			// Always convert to PC Note and set param value
			SetPcNote(row);
			SetParamFromY(row, y);
			break;

		case Action::Preserve:
			if (GetCommand(row) != CMD_NONE || IsPcNote(row))
			{
				// Only set param if we have an effect type or if this is a PC note.
				// Never change the effect type.
				SetParamFromY(row, y);
			}
			break;

	}

	m_ModDoc.SetModified();
	m_ModDoc.UpdateAllViews(nullptr, RowHint(row), this);
}

void CEffectVis::SetPcNote(ROWINDEX row)
{
	if(!m_SndFile.Patterns.IsValidPat(m_nPattern))
		return;

	ModCommand &m = *m_SndFile.Patterns[m_nPattern].GetpModCommand(row, m_nChan);
	m.Set(m_templatePCNote.note, m_templatePCNote.instr, m_templatePCNote.GetValueVolCol(), 0);
}

bool CEffectVis::IsPcNote(ROWINDEX row) const
{
	if(m_SndFile.Patterns.IsValidPat(m_nPattern))
		return m_SndFile.Patterns[m_nPattern].GetpModCommand(row, m_nChan)->IsPcNote();
	else
		return false;
}


OPENMPT_NAMESPACE_END
