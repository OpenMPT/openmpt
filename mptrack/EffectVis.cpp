// EffectVis.cpp : implementation file
//

#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "view_pat.h"
#include "EffectVis.h"


#define NODESIZE 7
#define NODEHALF 3
#define BOTTOMBORDER 20
#define TOPBORDER 0
#define LEFTBORDER 0
#define RIGHTBORDER 0

#define INNERLEFTBORDER 4
#define INNERRIGHTBORDER 4

// EffectVis dialog

IMPLEMENT_DYNAMIC(CEffectVis, CDialog)
CEffectVis::CEffectVis(CViewPattern *pViewPattern, UINT startRow, UINT endRow, UINT nchn, CModDoc* pModDoc, UINT pat)
{
	m_pViewPattern = pViewPattern;
	m_dwStatus = 0x00;
	m_nDragItem = -1;
	m_brushBlack.CreateSolidBrush(RGB(0, 0, 0));
	m_boolForceRedraw = TRUE;
	m_pModDoc = pModDoc;

	m_nRowToErase = -1;
	m_nParamToErase = -1;
	m_nLastDrawnRow	= -1;
	m_nLastDrawnY = -1;
	m_nOldPlayPos = UINT_MAX;
	m_nFillEffect = m_pModDoc->GetIndexFromEffect(CMD_SMOOTHMIDI, 0);
	m_nAction=kAction_OverwriteFX;
	m_templatePCNote.Set(NOTE_PCS, 1, 0, 0);

	UpdateSelection(startRow, endRow, nchn, pModDoc, pat);
}

BEGIN_MESSAGE_MAP(CEffectVis, CDialog)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
//{{AFX_MSG_MAP(CEffectVis)
	ON_COMMAND(ID_EDIT_UNDO,		OnEditUndo)
//}}AFX_MSG_MAP
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
	if (m_nAction == (UINT)kAction_FillPC
		|| m_nAction == (UINT)kAction_OverwritePC
		|| m_nAction == (UINT)kAction_Preserve)
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

uint16 CEffectVis::GetParam(UINT row)
{	
	MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
	uint16 paramValue = 0;

	if (pcmd)
	{
		MODCOMMAND cmd = pcmd[row*m_pSndFile->m_nChannels + m_nChan];
		if (cmd.IsPcNote()) 
		{
			paramValue = cmd.GetValueEffectCol();
		}
		else
		{
			paramValue = cmd.param;
		}
	}

	return paramValue;
}

// Sets a row's param value based on the vertical cursor position.
// Sets either plain pattern effect parameter or PC note parameter
// as appropriate, depending on contents of row.
void CEffectVis::SetParamFromY(UINT row, long y)
{
	MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
	if (!pcmd) {
		return;
	}

	int offset = row*m_pSndFile->m_nChannels + m_nChan;
	if (IsPcNote(row))
	{
		uint16 param = ScreenYToPCParam(y);
		CriticalSection cs;
		pcmd[offset].SetValueEffectCol(param);
	}
	else
	{		
		int param = ScreenYToFXParam(y);
		// Cap the parameter value as appropriate, based on effect type (e.g. Zxx gets capped to [0x00,0x7F])
		m_pModDoc->GetEffectFromIndex(m_pModDoc->GetIndexFromEffect(pcmd[offset].command, param), param);
		CriticalSection cs;
		pcmd[offset].param = static_cast<BYTE>(param);
	}	
}


BYTE CEffectVis::GetCommand(UINT row)
{
	MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
	if (pcmd)
		return pcmd[row*m_pSndFile->m_nChannels + m_nChan].command;
	else
		return 0;
}

void CEffectVis::SetCommand(UINT row, BYTE command)
{
	MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
	if (pcmd)
	{
		CriticalSection cs;
		int offset = row*m_pSndFile->m_nChannels + m_nChan;
		if (pcmd[offset].IsPcNote()) {
			// Clear PC note
			pcmd[offset].note = 0;
			pcmd[offset].instr = 0;
			pcmd[offset].volcmd = 0;
			pcmd[offset].vol = 0;
		}
		pcmd[offset].command = command;
	}
}

int CEffectVis::RowToScreenX(UINT row)
{
	if ((row >= m_startRow) || (row <= m_endRow))
		return (int) (m_rcDraw.left + INNERLEFTBORDER + (row-m_startRow)*m_pixelsPerRow + 0.5);
	return -1;
}


int CEffectVis::RowToScreenY(UINT row)
{
	MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
	int screenY = -1;

	if (pcmd)
	{
		MODCOMMAND cmd = pcmd[row*m_pSndFile->m_nChannels + m_nChan];
		if (cmd.IsPcNote()) 
		{
			uint16 paramValue = cmd.GetValueEffectCol();
			screenY = PCParamToScreenY(paramValue);
		}
		else
		{
			uint16 paramValue = cmd.param;
			screenY = FXParamToScreenY(paramValue);
		}
	}

	return screenY;
}

int CEffectVis::FXParamToScreenY(uint16 param)
{
	if ((param >= 0x00) || (param <= 0xFF))
		return (int) (m_rcDraw.bottom - param*m_pixelsPerFXParam + 0.5);
	return -1;
}

int CEffectVis::PCParamToScreenY(uint16 param)
{
	if ((param >= 0x00) || (param <= MODCOMMAND::maxColumnValue))
		return (int) (m_rcDraw.bottom - param*m_pixelsPerPCParam + 0.5);
	return -1;
}

BYTE CEffectVis::ScreenYToFXParam(int y)
{
	if (y<=FXParamToScreenY(0xFF))
		return 0xFF;

	if (y>=FXParamToScreenY(0x00))
		return 0x00;

	return (BYTE)((m_rcDraw.bottom-y)/m_pixelsPerFXParam+0.5);
}

uint16 CEffectVis::ScreenYToPCParam(int y)
{
	if (y<=PCParamToScreenY(MODCOMMAND::maxColumnValue))
		return MODCOMMAND::maxColumnValue;

	if (y>=PCParamToScreenY(0x00))
		return 0x00;

	return (uint16)((m_rcDraw.bottom-y)/m_pixelsPerPCParam+0.5);
}

UINT CEffectVis::ScreenXToRow(int x)
{
	if (x<=RowToScreenX(m_startRow))
		return m_startRow;

	if (x>=RowToScreenX(m_endRow))
		return m_endRow;

	return (UINT)(m_startRow +  (x-INNERLEFTBORDER)/m_pixelsPerRow + 0.5);
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
{
	// Lots of room for optimisation here.
	// Draw vertical grid lines
	ROWINDEX nBeat = m_pSndFile->m_nDefaultRowsPerBeat, nMeasure = m_pSndFile->m_nDefaultRowsPerMeasure;
	if(m_pSndFile->Patterns[m_nPattern].GetOverrideSignature())
	{
		nBeat = m_pSndFile->Patterns[m_nPattern].GetRowsPerBeat();
		nMeasure = m_pSndFile->Patterns[m_nPattern].GetRowsPerMeasure();
	}
	for (UINT row=m_startRow; row<=m_endRow; row++)
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
		m_dcGrid.MoveTo(m_rcDraw.left+INNERLEFTBORDER, y1);
		m_dcGrid.LineTo(m_rcDraw.right-INNERRIGHTBORDER, y1);
	}

} 

void CEffectVis::SetPlayCursor(UINT nPat, UINT nRow)
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

//-----------------------------------------------------------------------------------------
void CEffectVis::ShowVis(CDC * pDC, CRect rectBorder)
{
	UNREFERENCED_PARAMETER(rectBorder);
	if (m_boolForceRedraw)
	{
		m_boolForceRedraw = FALSE ;

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
//-----------------------------------------------------------------------------------------
void CEffectVis::ShowVisImage(CDC *pDC)
{
	CDC memDC ;
	CBitmap memBitmap ;
	CBitmap* oldBitmap ; // bitmap originally found in CMemDC

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
#ifndef HAVE_DOT_NET
		
		// VC6 TransparentBlit...
		// WINGDIAPI BOOL  WINAPI TransparentBlt(HDC,int,int,int,int,HDC,int,int,int,int,UINT);
        // hDestDC, xDest, yDest, nDestWidth,nDestHeight,xSrc,ySrc,nSrcWidth,nSrcHeight, crTransparent
        // 0,     , 0    ,
		unsigned int height = m_rcDraw.Height();
		unsigned int width  = m_rcDraw.Width();

		// FIX ME!
		memDC.BitBlt(0,0,width, height, &m_dcNodes,width,height,PATCOPY);
        memDC.BitBlt(0,0,width, height, &m_dcPlayPos,width,height,PATCOPY);
#endif

#ifdef HAVE_DOT_NET
		// merge the nodes image with the grid
		memDC.TransparentBlt(0, 0, m_rcDraw.Width(), m_rcDraw.Height(), &m_dcNodes, 0, 0, m_rcDraw.Width(), m_rcDraw.Height(), 0x00000000) ;
		// further merge the playpos 
		memDC.TransparentBlt(0, 0, m_rcDraw.Width(), m_rcDraw.Height(), &m_dcPlayPos, 0, 0, m_rcDraw.Width(), m_rcDraw.Height(), 0x00000000) ;
#endif
		// copy the resulting bitmap to the destination
		pDC->BitBlt(LEFTBORDER, TOPBORDER, m_rcDraw.Width()+LEFTBORDER, m_rcDraw.Height()+TOPBORDER, &memDC, 0, 0, SRCCOPY) ;
	}

	memDC.SelectObject(oldBitmap) ;

} // end ShowMeterImage

void CEffectVis::DrawNodes()
//---------------------------------------
{
	//erase
	if ((UINT)m_nRowToErase<m_startRow ||  m_nParamToErase < 0)
	{
		::FillRect(m_dcNodes.m_hDC, &m_rcDraw, m_brushBlack);
	}
	else
	{
		int x = RowToScreenX(m_nRowToErase);
		CRect r( x-NODEHALF-1, m_rcDraw.top, x-NODEHALF+NODESIZE+1, m_rcDraw.bottom);
		::FillRect(m_dcNodes.m_hDC, r, m_brushBlack);
	}

	//Draw
	for (UINT row=m_startRow; row<=m_endRow; row++)
	{
		int x = RowToScreenX(row);
		int y = RowToScreenY(row);
		DibBlt(m_dcNodes.m_hDC, x-NODEHALF, y-NODEHALF, NODESIZE, NODESIZE, 0, 0, IsPcNote(row)? CMainFrame::bmpVisPcNode : CMainFrame::bmpVisNode);
	}

}

void CEffectVis::InvalidateRow(int row)
{
	if (((UINT)row < m_startRow) ||  ((UINT)row > m_endRow)) return;

//It seems this optimisation doesn't work properly yet.	Disable in Update()

	int x = RowToScreenX(row);
	invalidated.bottom = m_rcDraw.bottom;
	invalidated.top = m_rcDraw.top;
	invalidated.left = x - NODEHALF*2;
	invalidated.right = x + NODEHALF*2;
	InvalidateRect(&invalidated, FALSE);

	InvalidateRect(NULL, FALSE);
}


BOOL CEffectVis::OpenEditor(CWnd *parent)
//---------------------------------------
{
	Create(IDD_EFFECTVISUALIZER, parent);
	m_boolForceRedraw = TRUE;
	ShowWindow(SW_SHOW);
	return TRUE;
}


VOID CEffectVis::OnClose()
//------------------------
{
	DoClose();
}


VOID CEffectVis::OnOK()
//---------------------
{
	OnClose();
}


VOID CEffectVis::OnCancel()
//-------------------------
{
	OnClose();
}


VOID CEffectVis::DoClose()
//------------------------
{
	m_dcGrid.SelectObject(m_pbOldGrid);
	m_dcGrid.DeleteDC();
	m_dcNodes.SelectObject(m_pbOldNodes);
	m_dcNodes.DeleteDC();
	m_dcPlayPos.SelectObject(m_pbOldPlayPos) ;
	m_dcPlayPos.DeleteDC() ;

	m_bGrid.DeleteObject();
	m_bNodes.DeleteObject();
	m_bPlayPos.DeleteObject() ;
	m_brushBlack.DeleteObject();

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
	UNREFERENCED_PARAMETER(nType);
	UNREFERENCED_PARAMETER(cx);
	UNREFERENCED_PARAMETER(cy);
	GetClientRect(&m_rcFullWin);
	m_rcDraw.SetRect( m_rcFullWin.left  + LEFTBORDER,  m_rcFullWin.top    + TOPBORDER,
				      m_rcFullWin.right - RIGHTBORDER, m_rcFullWin.bottom - BOTTOMBORDER);

#define INFOWIDTH 200
#define ACTIONLISTWIDTH 150
#define COMMANDLISTWIDTH 130

	if (IsWindow(m_edVisStatus.m_hWnd))
		m_edVisStatus.SetWindowPos(this, m_rcFullWin.left, m_rcDraw.bottom, m_rcFullWin.right-COMMANDLISTWIDTH-ACTIONLISTWIDTH, m_rcFullWin.bottom-m_rcDraw.bottom, SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_SHOWWINDOW|SWP_NOZORDER);	
	if (IsWindow(m_cmbActionList))
		m_cmbActionList.SetWindowPos(this,  m_rcFullWin.right-COMMANDLISTWIDTH-ACTIONLISTWIDTH, m_rcDraw.bottom, ACTIONLISTWIDTH, m_rcFullWin.bottom-m_rcDraw.bottom, SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_SHOWWINDOW|SWP_NOZORDER);
	if (IsWindow(m_cmbEffectList))
		m_cmbEffectList.SetWindowPos(this,  m_rcFullWin.right-COMMANDLISTWIDTH, m_rcDraw.bottom, COMMANDLISTWIDTH, m_rcFullWin.bottom-m_rcDraw.bottom, SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_SHOWWINDOW|SWP_NOZORDER);


	m_pixelsPerRow   = (float)(m_rcDraw.Width()-INNERLEFTBORDER-INNERRIGHTBORDER)/(float)m_nRows;
	m_pixelsPerFXParam = (float)(m_rcDraw.Height())/(float)0xFF;
	m_pixelsPerPCParam = (float)(m_rcDraw.Height())/(float)MODCOMMAND::maxColumnValue;
	m_boolForceRedraw = TRUE;
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

void CEffectVis::UpdateSelection(UINT startRow, UINT endRow, UINT nchn, CModDoc* pModDoc, UINT pat)
{
	m_pModDoc = pModDoc;
	m_startRow = startRow;
    m_endRow = endRow;
	m_nRows = endRow - startRow;
	m_nChan = static_cast<CHANNELINDEX>(nchn);
	m_nPattern = pat;

	//Check document exists
	if (!m_pModDoc)
	{
		DoClose();
		return;
	}

	//Check SoundFile exists
	m_pSndFile = m_pModDoc->GetSoundFile();
	if (!m_pSndFile)
	{
		DoClose();
		return;
	}

	//Check pattern, start row and channel exist
	MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
	if (!pcmd ||  (m_startRow >= m_pSndFile->Patterns[m_nPattern].GetNumRows()) || (m_nChan >= m_pSndFile->m_nChannels))
	{
		DoClose();
		return;
	}

	//Check end exists
	if ( (m_endRow >= m_pSndFile->Patterns[m_nPattern].GetNumRows()) )
	{
		m_endRow =  m_pSndFile->Patterns[m_nPattern].GetNumRows()-1;
		//ensure we still have some rows to process
		if (m_endRow <= m_startRow)
		{
			DoClose();
			return;
		}
	}

	m_pixelsPerRow = (float)(m_rcDraw.right - m_rcDraw.left)/(float)m_nRows;
	m_pixelsPerFXParam = (float)(m_rcDraw.bottom - m_rcDraw.top)/(float)0xFF;

	m_boolForceRedraw = TRUE;
	Update();

}

void CEffectVis::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (!(m_dwStatus & FXVSTATUS_LDRAGGING))
	{
		SetFocus();
		SetCapture();

		CRect rect;
		// Look if dragging a point
		m_nDragItem = -1;
		for (UINT row=m_startRow; row<=m_endRow; row++)
		{
			int x = RowToScreenX(row);
			int y = RowToScreenY(row);
			rect.SetRect(x-NODEHALF, y-NODEHALF, x+NODEHALF+1, y+NODEHALF+1);
			if (rect.PtInRect(point))
			{
				m_pModDoc->GetPatternUndo()->PrepareUndo(static_cast<PATTERNINDEX>(m_nPattern), m_nChan, row, m_nChan+1, row+1);
				m_nDragItem = row;
			}
		}		
		m_dwStatus |= FXVSTATUS_RDRAGGING;
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

	UINT row = ScreenXToRow(point.x);

	if ((m_dwStatus & FXVSTATUS_RDRAGGING) && (m_nDragItem>=0) )
	{
		m_nRowToErase = m_nDragItem;
		m_nParamToErase = GetParam(m_nDragItem);

		MakeChange(m_nDragItem, point.y);
	}
	else if ((m_dwStatus & FXVSTATUS_LDRAGGING))
	{
		// Interpolate if we detect that rows have been skipped but the left mouse button was not released.
		// This ensures we produce a smooth curve even when we are not notified of mouse movements at a high frequency (e.g. if CPU usage is high)
		if ((m_nLastDrawnRow>(int)m_startRow) && ((int)row != m_nLastDrawnRow) && ((int)row != m_nLastDrawnRow+1) && ((int)row != m_nLastDrawnRow-1))
		{
			int steps = abs((long)row-(long)m_nLastDrawnRow);
			ASSERT(steps!=0);
			int direction = ((int)(row-m_nLastDrawnRow) > 0) ? 1 : -1;
			float factor = (float)(point.y - m_nLastDrawnY)/(float)steps + 0.5f;

			int currentRow;
			for (int i=1; i<=steps; i++)
			{
				currentRow = m_nLastDrawnRow+(direction*i);
				int interpolatedY = static_cast<int>(m_nLastDrawnY+((float)i*factor+0.5f));
				MakeChange(currentRow, interpolatedY);
			}
			
			//Don't use single value update 
			m_nRowToErase = -1;
			m_nParamToErase = -1;
		}
		else 
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
		m_pModDoc->GetEffectInfo(m_pModDoc->GetIndexFromEffect(GetCommand(row), GetParam(row)), effectName, true);
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

		m_pModDoc->GetPatternUndo()->PrepareUndo(static_cast<PATTERNINDEX>(m_nPattern), m_nChan, m_startRow, m_nChan+1, m_endRow);
		m_dwStatus |= FXVSTATUS_LDRAGGING;
	}

	CModControlDlg::OnLButtonDown(nFlags, point);
}

void CEffectVis::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_dwStatus = 0x00;
	CModControlDlg::OnLButtonUp(nFlags, point);
	m_nLastDrawnRow = -1;
}


void CEffectVis::OnEditUndo()
//---------------------------
{
	Reporting::Notification("Undo Through!");
}


BOOL CEffectVis::OnInitDialog()
//-----------------------------
{
	CModControlDlg::OnInitDialog();
	if (m_pModDoc->GetModType() == MOD_TYPE_MPT && IsPcNote(m_startRow))
	{
		// If first selected row is a PC Note, default to PC note overwrite mode
		// and use it as a template for new PC notes that will be created via the visualiser.
		m_nAction = kAction_OverwritePC;
		MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
		if (pcmd) {
			int offset = m_startRow*m_pSndFile->m_nChannels + m_nChan;
			m_templatePCNote.Set(pcmd[offset].note, pcmd[offset].instr, pcmd[offset].GetValueVolCol(), 0);
		}
		m_cmbEffectList.EnableWindow(FALSE);
	}
	else
	{
		// Otherwise, default to FX overwrite and
		// use effect of first selected row as default effect type
		m_nAction = kAction_OverwriteFX;
		m_nFillEffect = m_pModDoc->GetIndexFromEffect(GetCommand(m_startRow), GetParam(m_startRow));
		if (m_nFillEffect<0 || m_nFillEffect>=MAX_EFFECTS)
			m_nFillEffect = m_pModDoc->GetIndexFromEffect(CMD_SMOOTHMIDI, 0);
	}


	CHAR s[128];
	UINT numfx = m_pModDoc->GetNumEffects();
	m_cmbEffectList.ResetContent();
	int k;
	for (UINT i=0; i<numfx; i++)
	{
		if (m_pModDoc->GetEffectInfo(i, s, true))
		{
			k =m_cmbEffectList.AddString(s);
			m_cmbEffectList.SetItemData(k, i);
			if ((long)i == m_nFillEffect)
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

void CEffectVis::MakeChange(int row, long y)
//------------------------------------------
{
	MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
	if (!pcmd)
	{
		return;
	}

	int offset = row * m_pSndFile->GetNumChannels() + m_nChan;
	
	switch (m_nAction)
	{
		case kAction_FillFX:
			// Only set command if there isn't a command already at this row and it's not a PC note
			if (!GetCommand(row) && !IsPcNote(row))
			{ 
				SetCommand(row, m_pModDoc->GetEffectFromIndex(m_nFillEffect));
			}
			// Always set param
			SetParamFromY(row, y);
			break;
	
		case kAction_OverwriteFX: 
			// Always set command and param. Blows away any PC notes.
			SetCommand(row, m_pModDoc->GetEffectFromIndex(m_nFillEffect));
			SetParamFromY(row, y);
			break;

		case kAction_FillPC:
			// Fill only empty slots with PC notes - leave other slots alone.
			if (pcmd[offset].IsEmpty())
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
	m_pModDoc->UpdateAllViews(NULL, HINT_PATTERNDATA | (m_nPattern << HINT_SHIFT_PAT), NULL);
}

void CEffectVis::SetPcNote(int row)
{
	MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
	if (!pcmd) {
		return;
	}

	int offset = row * m_pSndFile->GetNumChannels() + m_nChan;
	CriticalSection cs;
	pcmd[offset].Set(m_templatePCNote.note, m_templatePCNote.instr, m_templatePCNote.GetValueVolCol(), 0);
}

bool CEffectVis::IsPcNote(int row)
{
	MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
	if (pcmd)
		return pcmd[row * m_pSndFile->GetNumChannels() + m_nChan].IsPcNote();
	else
		return false;
}