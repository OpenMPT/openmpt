#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "ctrl_graph.h"
#include "view_graph.h"

CCtrlGraph::CCtrlGraph(void)
{
}

CCtrlGraph::~CCtrlGraph(void)
{
}

CRuntimeClass *CCtrlGraph::GetAssociatedViewClass()
//----------------------------------------------------
{
	return RUNTIME_CLASS(CViewGraph);
}
