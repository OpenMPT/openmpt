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
#define BOTTOMBORDER 15
#define TOPBORDER 0
#define LEFTBORDER 0
#define RIGHTBORDER 0

#define INNERLEFTBORDER 4
#define INNERRIGHTBORDER 4

// EffectVis dialog

IMPLEMENT_DYNAMIC(CEffectVis, CDialog)
CEffectVis::CEffectVis(CViewPattern *pViewPattern, UINT startRow, UINT endRow, UINT nchn, CModDoc* pModDoc, UINT pat)
	//: CDialog(CEffectVis::IDD, pParent)
{
	m_pViewPattern = pViewPattern;
	m_dwStatus = 0x00;
	m_nDragItem = -1;
	m_brushBlack.CreateSolidBrush(RGB(0, 0, 0));
	m_boolForceRedraw = TRUE;

	m_nRowToErase = -1;
	m_nParamToErase = -1;
	m_nLastDrawnRow	= -1;
	m_nOldPlayPos = -1;

	UpdateSelection(startRow, endRow, nchn, pModDoc, pat);
}


void CEffectVis::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VISSTATUS, m_edVisStatus);
}

void CEffectVis::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	m_pModDoc = m_pViewPattern->GetDocument();

	if ((!m_pModDoc) || !(&dc)) return;

	ShowVis(&dc, m_rcDraw);
	
}

BYTE CEffectVis::GetParam(UINT row)
{	MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
	if (pcmd)
		return pcmd[row*m_pSndFile->m_nChannels + m_nChan].param;
	else
		return 0;
}

void CEffectVis::SetParam(UINT row, BYTE param)
{
	MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
	BEGIN_CRITICAL();
	if (pcmd)
		pcmd[row*m_pSndFile->m_nChannels + m_nChan].param = param;
	END_CRITICAL();
}


BYTE CEffectVis::GetCommand(UINT row)
{
	MODCOMMAND *pcmd = m_pSndFile->Patterns[m_nPattern];
	if (pcmd)
		return pcmd[row*m_pSndFile->m_nChannels + m_nChan].command;
	else
		return 0;
}

int CEffectVis::RowToScreenX(UINT row)
{
	if ((row >= m_startRow) || (row <= m_endRow))
		return (int) (m_rcDraw.left + INNERLEFTBORDER + (row-m_startRow)*m_pixelsPerRow + 0.5);
	return -1;
}

int CEffectVis::ParamToScreenY(BYTE param)
{
	if ((param >= 0x00) || (param <= 0xFF))
		return (int) (m_rcDraw.bottom - param*m_pixelsPerParam + 0.5);
	return -1;
}

BYTE CEffectVis::ScreenYToParam(int y)
{
	if (y<=ParamToScreenY(0xFF))
		return 0xFF;

	if (y>=ParamToScreenY(0x00))
		return 0x00;

	return (BYTE)((m_rcDraw.bottom-y)/m_pixelsPerParam+0.5);
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
		//m_pViewPattern = NULL;
	}

}

void CEffectVis::DrawGrid() 
{
	//Lots of space for opti here.
	for (UINT row=m_startRow; row<=m_endRow; row++)
	{
		if (row % CMainFrame::m_nRowSpacing == 0)
			CMainFrame::penScratch = ::CreatePen(PS_SOLID,0, RGB(0xff, 0xff, 0xff));
		else if (row % CMainFrame::m_nRowSpacing2 == 0)
			CMainFrame::penScratch = ::CreatePen(PS_SOLID,0, RGB(0x99, 0x99, 0x99));
		else
			CMainFrame::penScratch = ::CreatePen(PS_SOLID,0, RGB(0x55, 0x55, 0x55));
		m_dcGrid.SelectObject(CMainFrame::penScratch);
		int x1 = RowToScreenX(row);
		m_dcGrid.MoveTo(x1, m_rcDraw.top);
		m_dcGrid.LineTo(x1, m_rcDraw.bottom);
		::DeletePen(CMainFrame::penScratch);
	}
	

	for (UINT i=0; i<256; i+=64)
	{
		UINT val = (BYTE) i;
		CMainFrame::penScratch = ::CreatePen(PS_SOLID,0, RGB(val, val, val));
		m_dcGrid.SelectObject(CMainFrame::penScratch);

		int y1 = ParamToScreenY(val);
		m_dcGrid.MoveTo(m_rcDraw.left+INNERLEFTBORDER, y1);
		m_dcGrid.LineTo(m_rcDraw.right-INNERRIGHTBORDER, y1);
		::DeletePen(CMainFrame::penScratch);
	}

} 

void CEffectVis::SetPlayCursor(UINT nPat, UINT nRow)
{
	//TEMP:
	::FillRect(m_dcPlayPos.m_hDC, &m_rcDraw, m_brushBlack);

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
	if (m_nRowToErase<m_startRow ||  m_nParamToErase<0)
		::FillRect(m_dcNodes.m_hDC, &m_rcDraw, m_brushBlack);
	else
	{
		int x = RowToScreenX(m_nRowToErase);
		int y = ParamToScreenY(m_nParamToErase);
		CRect r( x-NODEHALF-1, m_rcDraw.top, x-NODEHALF+NODESIZE+1, m_rcDraw.bottom);
		::FillRect(m_dcNodes.m_hDC, r, m_brushBlack /*CMainFrame::brushHighLight*/);
	}

	//Draw
	for (UINT row=m_startRow; row<=m_endRow; row++)
	{
		int x = RowToScreenX(row);
		int y = ParamToScreenY(GetParam(row));
		DibBlt(m_dcNodes.m_hDC, x-NODEHALF, y-NODEHALF, NODESIZE, NODESIZE, 0, 0, CMainFrame::bmpVisNode);
	}

}

void CEffectVis::InvalidateRow(int row)
{
	if ((row < m_startRow) ||  (row > m_endRow)) return;

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
	//	delete fastDC;
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
//	_CrtDumpMemoryLeaks();
}


BEGIN_MESSAGE_MAP(CEffectVis, CDialog)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
//	ON_WM_CHAR()
//	ON_WM_KEYDOWN()
//	ON_WM_KEYUP()
//	ON_WM_MBUTTONDOWN()
//{{AFX_MSG_MAP(CEffectVis)
	ON_COMMAND(ID_EDIT_UNDO,		OnEditUndo)
//}}AFX_MSG_MAP
//ON_STN_CLICKED(IDC_VISSTATUS, OnStnClickedVisstatus)
//ON_EN_CHANGE(IDC_VISSTATUS, OnEnChangeVisstatus)
END_MESSAGE_MAP()

// EffectVis message handlers



void CEffectVis::OnSize(UINT nType, int cx, int cy)
{
	//CDialog::OnSize(nType, cx, cy);
	// TODO: Add your message handler code here
	GetClientRect(&m_rcFullWin);
	m_rcDraw.SetRect( m_rcFullWin.left  + LEFTBORDER,  m_rcFullWin.top    + TOPBORDER,
				      m_rcFullWin.right - RIGHTBORDER, m_rcFullWin.bottom - BOTTOMBORDER);

	if (IsWindow(m_edVisStatus.m_hWnd))
		m_edVisStatus.SetWindowPos(this, m_rcFullWin.left, m_rcDraw.bottom, m_rcFullWin.right, m_rcFullWin.bottom, SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_SHOWWINDOW|SWP_NOZORDER);	
	m_pixelsPerRow   = (float)(m_rcDraw.Width()-INNERLEFTBORDER-INNERRIGHTBORDER)/(float)m_nRows;
	m_pixelsPerParam = (float)(m_rcDraw.Height())/(float)0xFF;
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
	m_nChan = nchn;
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
	if (!pcmd ||  (m_startRow >= m_pSndFile->PatternSize[m_nPattern]) || (m_nChan >= m_pSndFile->m_nChannels))
	{
		DoClose();
		return;
	}

	//Check end exists
	if ( (m_endRow >= m_pSndFile->PatternSize[m_nPattern]) )
	{
		m_endRow =  m_pSndFile->PatternSize[m_nPattern]-1;
		//ensure we still have some rows to process
		if (m_endRow <= m_startRow)
		{
			DoClose();
			return;
		}
	}

	m_pixelsPerRow = (float)(m_rcDraw.right - m_rcDraw.left)/(float)m_nRows;
	m_pixelsPerParam = (float)(m_rcDraw.bottom - m_rcDraw.top)/(float)0xFF;

	m_boolForceRedraw = TRUE;
	Update();

}

void CEffectVis::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
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
			int y = ParamToScreenY(GetParam(row));
			rect.SetRect(x-NODEHALF, y-NODEHALF, x+NODEHALF+1, y+NODEHALF+1);
			if (rect.PtInRect(point))
			{
				m_pModDoc->PrepareUndo(m_nPattern, m_nChan, row, m_nChan+1, row+1);
				m_nDragItem = row;
			}
		}		
		m_dwStatus |= FXVSTATUS_LDRAGGING;
	}

	CDialog::OnLButtonDown(nFlags, point);
}

void CEffectVis::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	ReleaseCapture();
	m_dwStatus = 0x00;
	m_nDragItem = -1;
	CDialog::OnLButtonUp(nFlags, point);

}

void CEffectVis::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CDialog::OnMouseMove(nFlags, point);

	m_pModDoc = m_pViewPattern->GetDocument();
	if (!m_pModDoc)
		return;

	UINT row= ScreenXToRow(point.x);
	BYTE paramValue=ScreenYToParam(point.y);

	if ((m_dwStatus & FXVSTATUS_LDRAGGING) && (m_nDragItem>=0) )
	{
		m_nRowToErase = m_nDragItem;
		m_nParamToErase = GetParam(m_nDragItem);

		SetParam(m_nDragItem, paramValue);
		m_pModDoc->SetModified();
		m_pModDoc->UpdateAllViews(NULL, HINT_PATTERNDATA | (m_nPattern << 24), NULL);
	}
	else if ((m_dwStatus & FXVSTATUS_RDRAGGING))
	{		
		//Interpolate if gap in rows, but with no right mouse release.		
		if ((m_nLastDrawnRow>(int)m_startRow) && (row != m_nLastDrawnRow) && (row != m_nLastDrawnRow+1) &&   (row != m_nLastDrawnRow-1))
		{
			int diff = abs(row-m_nLastDrawnRow);
			ASSERT(diff!=0);
			int sign = ((int)(row-m_nLastDrawnRow) > 0) ? 1 : -1;
			float factor = (float)(paramValue-GetParam(m_nLastDrawnRow))/(float)diff + 0.5f;

			int currentRow;

			for (int i=1; i<=diff; i++)
			{
				currentRow = m_nLastDrawnRow+(sign*i);
				m_nRowToErase = currentRow;
				m_nParamToErase = GetParam(currentRow);
				int newParam = GetParam(m_nLastDrawnRow)+((float)i*factor+0.5f);
				if (newParam>0xFF) newParam = 0xFF; if (newParam<0x00) newParam = 0x00;
				SetParam(currentRow, newParam);
			}
		
			m_nRowToErase = -1;		//Don't use single value update 
			m_nParamToErase = -1;

			m_nLastDrawnRow = row;
			m_pModDoc->SetModified();
			m_pModDoc->UpdateAllViews(NULL, HINT_PATTERNDATA | (m_nPattern << 24), NULL);
		}
		else 
		{
			m_nLastDrawnRow = row;
			m_nRowToErase = row;
			m_nParamToErase = GetParam(row);
			SetParam(row, paramValue);
			m_pModDoc->SetModified();
			m_pModDoc->UpdateAllViews(NULL, HINT_PATTERNDATA | (m_nPattern << 24), NULL);
		}
	}
	//update status bar
	CHAR s[256];
	wsprintf(s, "Pat: %d\tChn: %d\tRow: %d\tVal: %02X", m_nPattern, m_nChan+1, row, paramValue);
	m_edVisStatus.SetWindowText(s);
}

void CEffectVis::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (!(m_dwStatus & FXVSTATUS_RDRAGGING))
	{
		SetFocus();
		SetCapture();

		m_pModDoc->PrepareUndo(m_nPattern, m_nChan, m_startRow, m_nChan+1, m_endRow);
		m_dwStatus |= FXVSTATUS_RDRAGGING;
	}

	CModControlDlg::OnRButtonDown(nFlags, point);
}

void CEffectVis::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	ReleaseCapture();
	m_dwStatus = 0x00;
	CModControlDlg::OnRButtonUp(nFlags, point);
	m_nLastDrawnRow = -1;
}

//void CEffectVis::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
//{
//	// TODO: Add your message handler code here and/or call default
//	CModControlDlg::OnChar(nChar, nRepCnt, nFlags);
//	m_pViewPattern->OnChar(nChar, nRepCnt, nFlags);
//}

//void CEffectVis::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
//{
//	// TODO: Add your message handler code here and/or call default
//
//	CModControlDlg::OnKeyDown(nChar, nRepCnt, nFlags);
//	m_pViewPattern->OnKeyDown(nChar, nRepCnt, nFlags);
//}

//void CEffectVis::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
//{
//	// TODO: Add your message handler code here and/or call default
//
//	CModControlDlg::OnKeyUp(nChar, nRepCnt, nFlags);
//	m_pViewPattern->OnKeyUp(nChar, nRepCnt, nFlags);
//}

//void CEffectVis::OnMButtonDown(UINT nFlags, CPoint point)
//{
//	// TODO: Add your message handler code here and/or call default
//
//	CModControlDlg::OnMButtonDown(nFlags, point);
//}

void CEffectVis::OnEditUndo()
{
		CHAR s[64];
		wsprintf(s, "Undo Through!");
		::MessageBox(NULL, s, NULL, MB_OK|MB_ICONEXCLAMATION);
}
//void CEffectVis::OnStnClickedVisstatus()
//{
//	// TODO: Add your control notification handler code here
//}

//void CEffectVis::OnEnChangeVisstatus()
//{
//	// TODO:  If this is a RICHEDIT control, the control will not
//	// send this notification unless you override the CModControlDlg::OnInitDialog()
//	// function and call CRichEditCtrl().SetEventMask()
//	// with the ENM_CHANGE flag ORed into the mask.
//
//	// TODO:  Add your control notification handler code here
//}
