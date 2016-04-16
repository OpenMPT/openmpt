/*
 * EffectVis.cpp
 * -------------
 * Purpose: Implementation of parameter visualisation dialog.
 * Notes  : (currenlty none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "Childfrm.h"
#include "Moddoc.h"
#include "Globals.h"
#include "View_pat.h"
#include "EffectVis.h"


OPENMPT_NAMESPACE_BEGIN


// EffectVis dialog

IMPLEMENT_DYNAMIC(CEffectVis, CDialog)
CEffectVis::CEffectVis(CViewPattern *pViewPattern, ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX nchn, CModDoc *pModDoc, PATTERNINDEX pat)
	: effectInfo(pModDoc->GetrSoundFile())
	, m_pViewPattern(pViewPattern)
	, m_dwStatus(0)
	, m_nDragItem(-1)
	, m_forceRedraw(true)
	, m_pModDoc(pModDoc)
	, m_nRowToErase(-1)
	, m_nParamToErase(-1)
	, m_nLastDrawnRow(ROWINDEX_INVALID)
	, m_nLastDrawnY(-1)
	, m_nOldPlayPos(ROWINDEX_INVALID)
	, m_nAction(kAction_OverwriteFX)
	, m_pixelsPerRow(1)
	, m_pixelsPerFXParam(1)
	, m_pixelsPerPCParam(1)
{
	m_nFillEffect = effectInfo.GetIndexFromEffect(CMD_SMOOTHMIDI, 0);
	m_templatePCNote.Set(NOTE_PCS, 1, 0, 0);
	UpdateSelection(startRow, endRow, nchn, pModDoc, pat);
}

BEGIN_MESSAGE_MAP(CEffectVis, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_CBN_SELCHANGE(IDC_VISACTION,		OnActionChanged)
	ON_CBN_SELCHANGE(IDC_VISEFFECTLIST,	OnEffectChanged)
END_MESSAGE_MAP()

void CEffectVis::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VISSTATUS, m_edVisStatus);
	DDX_Control(pDX, IDC_VISEFFECTLIST, m_cmbEffectList);
	DDX_Control(pDX, IDC_VISACTION, m_cmbActionList);
}

void CEffectVis::OnActionChanged()
{
	m_nAction = m_cmbActionList.GetItemData(m_cmbActionList.GetCurSel());
	if (m_nAction == kAction_FillPC
		|| m_nAction == kAction_OverwritePC
		|| m_nAction == kAction_Preserve)
		m_cmbEffectList.EnableWindow(FALSE);
	else
		m_cmbEffectList.EnableWindow(TRUE);

}

void CEffectVis::OnEffectChanged()
{
	m_nFillEffect = m_cmbEffectList.GetItemData(m_cmbEffectList.GetCurSel());
}

void CEffectVis::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	m_pModDoc = m_pViewPattern->GetDocument();

	if ((!m_pModDoc) || !(&dc)) return;

	ShowVis(&dc, m_rcDraw);

}

uint16 CEffectVis::GetParam(ROWINDEX row) const
{
	uint16 paramValue = 0;

	if(m_pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		const ModCommand &m = *m_pSndFile->Patterns[m_nPattern].GetpModCommand(row, m_nChan);
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
	if(!m_pSndFile->Patterns.IsValidPat(m_nPattern))
		return;

	ModCommand &m = *m_pSndFile->Patterns[m_nPattern].GetpModCommand(row, m_nChan);
	if (IsPcNote(row))
	{
		uint16 param = ScreenYToPCParam(y);
		m.SetValueEffectCol(param);
	} else
	{
		ModCommand::PARAM param = ScreenYToFXParam(y);
		// Cap the parameter value as appropriate, based on effect type (e.g. Zxx gets capped to [0x00,0x7F])
		effectInfo.GetEffectFromIndex(effectInfo.GetIndexFromEffect(m.command, param), param);
		m.param = param;
	}
}


ModCommand::COMMAND CEffectVis::GetCommand(ROWINDEX row) const
//------------------------------------------------------------
{
	if(m_pSndFile->Patterns.IsValidPat(m_nPattern))
		return m_pSndFile->Patterns[m_nPattern].GetpModCommand(row, m_nChan)->command;
	else
		return CMD_NONE;
}

void CEffectVis::SetCommand(ROWINDEX row, BYTE command)
{
	if(m_pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		ModCommand &m = *m_pSndFile->Patterns[m_nPattern].GetpModCommand(row, m_nChan);
		if(m.IsPcNote())
		{
			// Clear PC note
			m.note = 0;
			m.instr = 0;
			m.volcmd = 0;
			m.vol = 0;
		}
		m.command = command;
	}
}

int CEffectVis::RowToScreenX(ROWINDEX row) const
//----------------------------------------------
{
	if ((row >= m_startRow) || (row <= m_endRow))
		return Util::Round<int>(m_rcDraw.left + m_innerBorder + (row - m_startRow) * m_pixelsPerRow);
	return -1;
}


int CEffectVis::RowToScreenY(ROWINDEX row) const
//----------------------------------------------
{
	int screenY = -1;

	if(m_pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		const ModCommand &m = *m_pSndFile->Patterns[m_nPattern].GetpModCommand(row, m_nChan);
		if (m.IsPcNote())
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
//--------------------------------------------------
{
	if ((param >= 0x00) || (param <= 0xFF))
		return Util::Round<int>(m_rcDraw.bottom - param * m_pixelsPerFXParam);
	return -1;
}

int CEffectVis::PCParamToScreenY(uint16 param) const
//--------------------------------------------------
{
	if ((param >= 0x00) || (param <= ModCommand::maxColumnValue))
		return Util::Round<int>(m_rcDraw.bottom - param*m_pixelsPerPCParam);
	return -1;
}

ModCommand::PARAM CEffectVis::ScreenYToFXParam(int y) const
//---------------------------------------------------------
{
	if (y<=FXParamToScreenY(0xFF))
		return 0xFF;

	if (y>=FXParamToScreenY(0x00))
		return 0x00;

	return Util::Round<ModCommand::PARAM>((m_rcDraw.bottom - y) / m_pixelsPerFXParam);
}

uint16 CEffectVis::ScreenYToPCParam(int y) const
//----------------------------------------------
{
	if (y<=PCParamToScreenY(ModCommand::maxColumnValue))
		return ModCommand::maxColumnValue;

	if (y>=PCParamToScreenY(0x00))
		return 0x00;

	return Util::Round<uint16>((m_rcDraw.bottom - y) / m_pixelsPerPCParam);
}

ROWINDEX CEffectVis::ScreenXToRow(int x) const
//--------------------------------------------
{
	if (x <= RowToScreenX(m_startRow))
		return m_startRow;

	if (x >= RowToScreenX(m_endRow))
		return m_endRow;

	return Util::Round<ROWINDEX>(m_startRow + (x - m_innerBorder) / m_pixelsPerRow);
}

CEffectVis::~CEffectVis()
//-----------------------
{
	if (m_pViewPattern)
	{
		m_pViewPattern->m_pEffectVis = NULL;
	}

}

void CEffectVis::DrawGrid()
//-------------------------
{
	// Lots of room for optimisation here.
	// Draw vertical grid lines
	ROWINDEX nBeat = m_pSndFile->m_nDefaultRowsPerBeat, nMeasure = m_pSndFile->m_nDefaultRowsPerMeasure;
	if(m_pSndFile->Patterns[m_nPattern].GetOverrideSignature())
	{
		nBeat = m_pSndFile->Patterns[m_nPattern].GetRowsPerBeat();
		nMeasure = m_pSndFile->Patterns[m_nPattern].GetRowsPerMeasure();
	}

	m_dcGrid.FillSolidRect(&m_rcDraw, 0);
	for (ROWINDEX row = m_startRow; row <= m_endRow; row++)
	{
		if (row % nMeasure == 0)
			CMainFrame::penScratch = CMainFrame::penGrayff;
		else if (row % nBeat == 0)
			CMainFrame::penScratch = CMainFrame::penGray99;
		else
			CMainFrame::penScratch = CMainFrame::penGray55;
		m_dcGrid.SelectObject(CMainFrame::penScratch);
		int x1 = RowToScreenX(row);
		m_dcGrid.MoveTo(x1, m_rcDraw.top);
		m_dcGrid.LineTo(x1, m_rcDraw.bottom);
	}

	// Draw horizontal grid lines
	const UINT numHorizontalLines = 4;
	for (UINT i=0; i<numHorizontalLines; i++)
	{
		switch (i%4)
		{
			case 0: CMainFrame::penScratch = CMainFrame::penGray00; break;
			case 1: CMainFrame::penScratch = CMainFrame::penGray40; break;
			case 2: CMainFrame::penScratch = CMainFrame::penGray80; break;
			case 3: CMainFrame::penScratch = CMainFrame::penGraycc; break;
		}

		m_dcGrid.SelectObject(CMainFrame::penScratch);

		int y1 = m_rcDraw.bottom/numHorizontalLines * i;
		m_dcGrid.MoveTo(m_rcDraw.left + m_innerBorder, y1);
		m_dcGrid.LineTo(m_rcDraw.right - m_innerBorder, y1);
	}

}


void CEffectVis::SetPlayCursor(PATTERNINDEX nPat, ROWINDEX nRow)
//--------------------------------------------------------------
{
	int x1;
	//erase current playpos:
	if (m_nOldPlayPos>=m_startRow &&  m_nOldPlayPos<=m_endRow)
	{
		x1=RowToScreenX(m_nOldPlayPos);
		m_dcPlayPos.SelectObject(CMainFrame::penBlack);
		m_dcPlayPos.MoveTo(x1,m_rcDraw.top);
		m_dcPlayPos.LineTo(x1,m_rcDraw.bottom);
	}

	if ((nRow<m_startRow) || (nRow>m_endRow) || (nPat != m_nPattern))
		return;

	x1=RowToScreenX(nRow);
	m_dcPlayPos.SelectObject(CMainFrame::penSample);
	m_dcPlayPos.MoveTo(x1,m_rcDraw.top);
	m_dcPlayPos.LineTo(x1,m_rcDraw.bottom);

	m_nOldPlayPos = nRow;
	InvalidateRect(NULL, FALSE);
}


void CEffectVis::ShowVis(CDC * pDC, CRect rectBorder)
//---------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(rectBorder);
	if (m_forceRedraw)
	{
		m_forceRedraw = false;

		// if we already have a memory dc, destroy it (this occurs for a re-size)
		if (m_dcGrid.GetSafeHdc())
		{
			m_dcGrid.SelectObject(m_pbOldGrid) ;
			m_dcGrid.DeleteDC() ;

			m_dcNodes.SelectObject(m_pbOldNodes) ;
			m_dcNodes.DeleteDC() ;

			m_dcPlayPos.SelectObject(m_pbOldPlayPos) ;
			m_dcPlayPos.DeleteDC() ;

			m_bPlayPos.DeleteObject() ;
			m_bGrid.DeleteObject() ;
			m_bNodes.DeleteObject() ;
		}

		// create a memory based dc for drawing the grid
		m_dcGrid.CreateCompatibleDC(pDC);
		m_bGrid.CreateCompatibleBitmap(pDC, m_rcDraw.Width(), m_rcDraw.Height());
		m_pbOldGrid = m_dcGrid.SelectObject(&m_bGrid);

		// create a memory based dc for drawing the nodes
		m_dcNodes.CreateCompatibleDC(pDC);
		m_bNodes.CreateCompatibleBitmap(pDC, m_rcDraw.Width(), m_rcDraw.Height());
		m_pbOldNodes = m_dcNodes.SelectObject(&m_bNodes);

		// create a memory based dc for drawing the nodes
		m_dcPlayPos.CreateCompatibleDC(pDC);
		m_bPlayPos.CreateCompatibleBitmap(pDC, m_rcDraw.Width(), m_rcDraw.Height());
		m_pbOldPlayPos = m_dcPlayPos.SelectObject(&m_bPlayPos);

		SetPlayCursor(m_nPattern, m_nOldPlayPos);
		DrawGrid();
		DrawNodes();
	}

	// display the new image, combining the nodes with the grid
	ShowVisImage(pDC);

}


void CEffectVis::ShowVisImage(CDC *pDC)
//-------------------------------------
{
	CDC memDC;
	CBitmap memBitmap;
	CBitmap* oldBitmap; // bitmap originally found in CMemDC

	// to avoid flicker, establish a memory dc, draw to it
	// and then BitBlt it to the destination "pDC"
	memDC.CreateCompatibleDC(pDC);
	if (!memDC)
		return;
	memBitmap.CreateCompatibleBitmap(pDC, m_rcDraw.Width(), m_rcDraw.Height()) ;
	oldBitmap = (CBitmap *)memDC.SelectObject(&memBitmap) ;

	// make sure we have the bitmaps
	if (!m_dcGrid.GetSafeHdc())
		return ;
	if (!m_dcNodes.GetSafeHdc())
		return ;
	if (!m_dcPlayPos.GetSafeHdc())
		return ;

	if (memDC.GetSafeHdc() != NULL)
	{
		// draw the grid
		memDC.BitBlt(0, 0, m_rcDraw.Width(), m_rcDraw.Height(), &m_dcGrid, 0, 0, SRCCOPY) ;

		// merge the nodes image with the grid
		memDC.TransparentBlt(0, 0, m_rcDraw.Width(), m_rcDraw.Height(), &m_dcNodes, 0, 0, m_rcDraw.Width(), m_rcDraw.Height(), 0x00000000) ;
		// further merge the playpos
		memDC.TransparentBlt(0, 0, m_rcDraw.Width(), m_rcDraw.Height(), &m_dcPlayPos, 0, 0, m_rcDraw.Width(), m_rcDraw.Height(), 0x00000000) ;

		// copy the resulting bitmap to the destination
		pDC->BitBlt(0, 0, m_rcDraw.Width(), m_rcDraw.Height(), &memDC, 0, 0, SRCCOPY) ;
	}

	memDC.SelectObject(oldBitmap);

}


void CEffectVis::DrawNodes()
//--------------------------
{
	if(m_rcDraw.IsRectEmpty())
		return;

	//Draw
	const int lineWidth = Util::ScalePixels(1, m_hWnd);
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
		m_dcNodes.FillSolidRect(x - nodeSizeHalf, y - nodeSizeHalf, nodeSize, lineWidth, col);	// Top
		m_dcNodes.FillSolidRect(x + nodeSizeHalf2, y - nodeSizeHalf, lineWidth, nodeSize, col);	// Right
		m_dcNodes.FillSolidRect(x - nodeSizeHalf, y + nodeSizeHalf2, nodeSize, lineWidth, col);	// Bottom
		m_dcNodes.FillSolidRect(x - nodeSizeHalf, y - nodeSizeHalf, lineWidth, nodeSize, col);	// Left
	}

}

void CEffectVis::InvalidateRow(int row)
{
	if (((UINT)row < m_startRow) ||  ((UINT)row > m_endRow)) return;

//It seems this optimisation doesn't work properly yet.	Disable in Update()

	int x = RowToScreenX(row);
	invalidated.bottom = m_rcDraw.bottom;
	invalidated.top = m_rcDraw.top;
	invalidated.left = x - m_nodeSizeHalf;
	invalidated.right = x + m_nodeSizeHalf + 1;
	InvalidateRect(&invalidated, FALSE);
}


BOOL CEffectVis::OpenEditor(CWnd *parent)
//---------------------------------------
{
	Create(IDD_EFFECTVISUALIZER, parent);
	m_forceRedraw = true;

	if(TrackerSettings::Instance().effectVisWidth > 0 && TrackerSettings::Instance().effectVisHeight > 0)
	{
		SetWindowPos(nullptr,
			0, 0,
			MulDiv(TrackerSettings::Instance().effectVisWidth, Util::GetDPIx(m_hWnd), 96), MulDiv(TrackerSettings::Instance().effectVisHeight, Util::GetDPIy(m_hWnd), 96),
			SWP_NOZORDER | SWP_NOMOVE);
	}

	ShowWindow(SW_SHOW);
	return TRUE;
}


void CEffectVis::OnClose()
//------------------------
{
	DoClose();
}


void CEffectVis::OnOK()
//---------------------
{
	OnClose();
}


void CEffectVis::OnCancel()
//-------------------------
{
	OnClose();
}


void CEffectVis::DoClose()
//------------------------
{
	CRect rect;
	GetWindowRect(rect);
	TrackerSettings::Instance().effectVisWidth = MulDiv(rect.Width(), 96, Util::GetDPIx(m_hWnd));
	TrackerSettings::Instance().effectVisHeight = MulDiv(rect.Height(), 96, Util::GetDPIy(m_hWnd));

	m_dcGrid.SelectObject(m_pbOldGrid);
	m_dcGrid.DeleteDC();
	m_dcNodes.SelectObject(m_pbOldNodes);
	m_dcNodes.DeleteDC();
	m_dcPlayPos.SelectObject(m_pbOldPlayPos);
	m_dcPlayPos.DeleteDC();

	m_bGrid.DeleteObject();
	m_bNodes.DeleteObject();
	m_bPlayPos.DeleteObject();

	if (m_hWnd)
	{
		DestroyWindow();
		m_pViewPattern->m_pEffectVis = NULL;
	}
	m_hWnd = NULL;
	delete this;
}


void CEffectVis::OnSize(UINT nType, int cx, int cy)
{
	MPT_UNREFERENCED_PARAMETER(nType);
	MPT_UNREFERENCED_PARAMETER(cx);
	MPT_UNREFERENCED_PARAMETER(cy);
	GetClientRect(&m_rcFullWin);
	m_rcDraw.SetRect( m_rcFullWin.left,  m_rcFullWin.top,
				      m_rcFullWin.right, m_rcFullWin.bottom - m_marginBottom);

#define INFOWIDTH 200
#define ACTIONLISTWIDTH 150
#define COMMANDLISTWIDTH 150

	if (IsWindow(m_edVisStatus.m_hWnd))
		m_edVisStatus.SetWindowPos(this, m_rcFullWin.left, m_rcDraw.bottom, m_rcFullWin.right-COMMANDLISTWIDTH-ACTIONLISTWIDTH, m_rcFullWin.bottom-m_rcDraw.bottom, SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_SHOWWINDOW|SWP_NOZORDER);
	if (IsWindow(m_cmbActionList))
		m_cmbActionList.SetWindowPos(this,  m_rcFullWin.right-COMMANDLISTWIDTH-ACTIONLISTWIDTH, m_rcDraw.bottom, ACTIONLISTWIDTH, m_rcFullWin.bottom-m_rcDraw.bottom, SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_SHOWWINDOW|SWP_NOZORDER);
	if (IsWindow(m_cmbEffectList))
		m_cmbEffectList.SetWindowPos(this,  m_rcFullWin.right-COMMANDLISTWIDTH, m_rcDraw.bottom, COMMANDLISTWIDTH, m_rcFullWin.bottom-m_rcDraw.bottom, SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_SHOWWINDOW|SWP_NOZORDER);


	if(m_nRows)
		m_pixelsPerRow = (float)(m_rcDraw.Width() - m_innerBorder * 2) / (float)m_nRows;
	else
		m_pixelsPerRow = 1;
	m_pixelsPerFXParam = (float)(m_rcDraw.Height())/(float)0xFF;
	m_pixelsPerPCParam = (float)(m_rcDraw.Height())/(float)ModCommand::maxColumnValue;
	m_forceRedraw = true;
	InvalidateRect(NULL, FALSE);	 //redraw everything
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

void CEffectVis::UpdateSelection(ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX nchn, CModDoc *pModDoc, PATTERNINDEX pat)
{
	m_pModDoc = pModDoc;
	m_startRow = startRow;
	m_endRow = endRow;
	m_nRows = endRow - startRow;
	m_nChan = nchn;
	m_nPattern = pat;

	//Check document exists
	if (!m_pModDoc || (m_pSndFile = m_pModDoc->GetSoundFile()) == nullptr)
	{
		DoClose();
		return;
	}

	//Check pattern, start row and channel exist
	if(!m_pSndFile->Patterns.IsValidPat(m_nPattern) || !m_pSndFile->Patterns[m_nPattern].IsValidRow(m_startRow) || m_nChan >= m_pSndFile->GetNumChannels())
	{
		DoClose();
		return;
	}

	//Check end exists
	if (!m_pSndFile->Patterns[m_nPattern].IsValidRow(m_endRow))
	{
		m_endRow = m_pSndFile->Patterns[m_nPattern].GetNumRows() - 1;
	}

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
		m_pModDoc->GetPatternUndo().PrepareUndo(static_cast<PATTERNINDEX>(m_nPattern), m_nChan, m_nDragItem, 1, 1, "Parameter Editor entry");
		OnMouseMove(nFlags, point);
	}

	CDialog::OnRButtonDown(nFlags, point);
}

void CEffectVis::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_dwStatus = 0x00;
	m_nDragItem = -1;
	CDialog::OnRButtonUp(nFlags, point);
}

void CEffectVis::OnMouseMove(UINT nFlags, CPoint point)
{
	CDialog::OnMouseMove(nFlags, point);

	m_pModDoc = m_pViewPattern->GetDocument();
	if (!m_pModDoc)
		return;

	ROWINDEX row = ScreenXToRow(point.x);

	if ((m_dwStatus & FXVSTATUS_RDRAGGING) && (m_nDragItem>=0) )
	{
		m_nRowToErase = m_nDragItem;
		m_nParamToErase = GetParam(m_nDragItem);

		MakeChange(m_nDragItem, point.y);
	} else if ((m_dwStatus & FXVSTATUS_LDRAGGING))
	{
		// Interpolate if we detect that rows have been skipped but the left mouse button was not released.
		// This ensures we produce a smooth curve even when we are not notified of mouse movements at a high frequency (e.g. if CPU usage is high)
		const int steps = abs((int)row - (int)m_nLastDrawnRow);
		if (m_nLastDrawnRow != ROWINDEX_INVALID && m_nLastDrawnRow > m_startRow && steps > 1)
		{
			int direction = ((int)(row - m_nLastDrawnRow) > 0) ? 1 : -1;
			float factor = (float)(point.y - m_nLastDrawnY)/(float)steps + 0.5f;

			int currentRow;
			for (int i=1; i<=steps; i++)
			{
				currentRow = m_nLastDrawnRow+(direction*i);
				int interpolatedY = Util::Round<int>(m_nLastDrawnY + ((float)i * factor));
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
	CHAR status[256];
	CHAR effectName[128];
	uint16 paramValue;


	if (IsPcNote(row))
	{
		paramValue = ScreenYToPCParam(point.y);
		wsprintf(effectName, "%s", "Param Control"); // TODO - show smooth & plug+param
	}
	else
	{
		paramValue = ScreenYToFXParam(point.y);
		effectInfo.GetEffectInfo(effectInfo.GetIndexFromEffect(GetCommand(row), ModCommand::PARAM(GetParam(row))), effectName, true);
	}

	wsprintf(status, "Pat: %d\tChn: %d\tRow: %d\tVal: %02X (%03d) [%s]",
				m_nPattern, m_nChan+1, row, paramValue, paramValue, effectName);
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
		m_pModDoc->GetPatternUndo().PrepareUndo(static_cast<PATTERNINDEX>(m_nPattern), m_nChan, m_startRow, 1, m_endRow - m_startRow + 1, "Parameter Editor entry");
		OnMouseMove(nFlags, point);
	}

	CDialog::OnLButtonDown(nFlags, point);
}

void CEffectVis::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_dwStatus = 0x00;
	CDialog::OnLButtonUp(nFlags, point);
	m_nLastDrawnRow = ROWINDEX_INVALID;
}


BOOL CEffectVis::OnInitDialog()
//-----------------------------
{
	CDialog::OnInitDialog();

	int dpi = Util::GetDPIx(m_hWnd);
	m_nodeSizeHalf = MulDiv(3, dpi, 96);
	m_marginBottom = MulDiv(20, dpi, 96);
	m_innerBorder = MulDiv(4, dpi, 96);


	if (m_pModDoc->GetModType() == MOD_TYPE_MPT && IsPcNote(m_startRow))
	{
		// If first selected row is a PC Note, default to PC note overwrite mode
		// and use it as a template for new PC notes that will be created via the visualiser.
		m_nAction = kAction_OverwritePC;
		if(m_pSndFile->Patterns.IsValidPat(m_nPattern))
		{
			ModCommand &m = *m_pSndFile->Patterns[m_nPattern].GetpModCommand(m_startRow, m_nChan);
			m_templatePCNote.Set(m.note, m.instr, m.GetValueVolCol(), 0);
		}
		m_cmbEffectList.EnableWindow(FALSE);
	} else
	{
		// Otherwise, default to FX overwrite and
		// use effect of first selected row as default effect type
		m_nAction = kAction_OverwriteFX;
		m_nFillEffect = effectInfo.GetIndexFromEffect(GetCommand(m_startRow), ModCommand::PARAM(GetParam(m_startRow)));
		if (m_nFillEffect < 0 || m_nFillEffect >= MAX_EFFECTS)
			m_nFillEffect = effectInfo.GetIndexFromEffect(CMD_SMOOTHMIDI, 0);
	}


	CHAR s[128];
	UINT numfx = effectInfo.GetNumEffects();
	m_cmbEffectList.ResetContent();
	int k;
	for (UINT i=0; i<numfx; i++)
	{
		if (effectInfo.GetEffectInfo(i, s, true))
		{
			k =m_cmbEffectList.AddString(s);
			m_cmbEffectList.SetItemData(k, i);
			if ((int)i == m_nFillEffect)
				m_cmbEffectList.SetCurSel(k);
		}
	}

	m_cmbActionList.ResetContent();
	m_cmbActionList.SetItemData(m_cmbActionList.AddString("Overwrite with effect:"), kAction_OverwriteFX);
	m_cmbActionList.SetItemData(m_cmbActionList.AddString("Fill blanks with effect:"), kAction_FillFX);
	if (m_pModDoc->GetModType() == MOD_TYPE_MPT)
	{
		m_cmbActionList.SetItemData(m_cmbActionList.AddString("Overwrite with PC note"), kAction_OverwritePC);
		m_cmbActionList.SetItemData(m_cmbActionList.AddString("Fill blanks with PC note"), kAction_FillPC);
	}
	m_cmbActionList.SetItemData(m_cmbActionList.AddString("Never change effect type"), kAction_Preserve);

	m_cmbActionList.SetCurSel(m_nAction);
	return true;
}

void CEffectVis::MakeChange(ROWINDEX row, int y)
//-----------------------------------------------
{
	if(!m_pSndFile->Patterns.IsValidPat(m_nPattern))
		return;

	ModCommand &m = *m_pSndFile->Patterns[m_nPattern].GetpModCommand(row, m_nChan);

	switch (m_nAction)
	{
		case kAction_FillFX:
			// Only set command if there isn't a command already at this row and it's not a PC note
			if (!GetCommand(row) && !IsPcNote(row))
			{
				SetCommand(row, effectInfo.GetEffectFromIndex(m_nFillEffect));
			}
			// Always set param
			SetParamFromY(row, y);
			break;

		case kAction_OverwriteFX:
			// Always set command and param. Blows away any PC notes.
			SetCommand(row, effectInfo.GetEffectFromIndex(m_nFillEffect));
			SetParamFromY(row, y);
			break;

		case kAction_FillPC:
			// Fill only empty slots with PC notes - leave other slots alone.
			if (m.IsEmpty())
			{
				SetPcNote(row);
			}
			// Always set param
			SetParamFromY(row, y);
			break;

		case kAction_OverwritePC:
			// Always convert to PC Note and set param value
			SetPcNote(row);
			SetParamFromY(row, y);
			break;

		case kAction_Preserve:
			if (GetCommand(row) || IsPcNote(row))
			{
				// Only set param if we have an effect type or if this is a PC note.
				// Never change the effect type.
				SetParamFromY(row, y);
			}
			break;

	}

	m_pModDoc->SetModified();
	m_pModDoc->UpdateAllViews(nullptr, PatternHint(m_nPattern).Data());
}

void CEffectVis::SetPcNote(ROWINDEX row)
//--------------------------------------
{
	if(!m_pSndFile->Patterns.IsValidPat(m_nPattern))
		return;

	ModCommand &m = *m_pSndFile->Patterns[m_nPattern].GetpModCommand(row, m_nChan);
	m.Set(m_templatePCNote.note, m_templatePCNote.instr, m_templatePCNote.GetValueVolCol(), 0);
}

bool CEffectVis::IsPcNote(ROWINDEX row) const
//-------------------------------------------
{
	if(m_pSndFile->Patterns.IsValidPat(m_nPattern))
		return m_pSndFile->Patterns[m_nPattern].GetpModCommand(row, m_nChan)->IsPcNote();
	else
		return false;
}


OPENMPT_NAMESPACE_END
