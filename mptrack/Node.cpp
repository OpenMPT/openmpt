#include "stdafx.h"
#include ".\node.h"

CNode::CNode(void)
{
	x = rand()%500;
	y = rand()%500;
}

CNode::~CNode(void)
{
}

void CNode::Render(CDC* cdc) 
{
	cdc->SelectObject(GetBorderPen());
	cdc->Rectangle(x-nodeWidth/2, y-nodeHeight/2, x+nodeWidth/2, y+nodeHeight/2);
}