#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "view_graph.h"
#include "graph.h"
#include ".\view_graph.h"

IMPLEMENT_SERIAL(CViewGraph, CModScrollView, 0)

CViewGraph::CViewGraph(void)
//--------------------------
{
	m_pGraph = new CGraph();
}

CViewGraph::~CViewGraph(void)
//---------------------------
{
	delete m_pGraph;
}
BEGIN_MESSAGE_MAP(CViewGraph, CModScrollView)
	ON_WM_ACTIVATE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CViewGraph::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
//------------------------------------------------------------------------
{
	CModScrollView::OnActivate(nState, pWndOther, bMinimized);
}

void CViewGraph::OnDestroy()
//--------------------------
{
	CModScrollView::OnDestroy();
	m_pGraph->EmptyNodePool();
}


void CViewGraph::OnDraw(CDC *cdc)
//----------------------------
{
	m_pGraph->Render(cdc);
}


void CViewGraph::OnInitialUpdate()
//--------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile = pModDoc->GetSoundFile();	
	m_pGraph->BuildNodePool(pSndFile);
}


void CViewGraph::OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint)
//---------------------------------------------------------------------
{
}


LRESULT CViewGraph::OnModViewMsg(WPARAM wParam, LPARAM lParam)
//------------------------------------------------------------
{
	switch(wParam)
	{
		default:
			return CModScrollView::OnModViewMsg(wParam, lParam);
	}
}