#include "stdafx.h"
#include ".\finalnode.h"

CFinalNode::CFinalNode(void)
{
	m_hBorderPen = ::CreatePen(PS_SOLID,0, RGB(0x00, 0x00, 0x00));
}

CFinalNode::~CFinalNode(void)
{
}

HPEN CFinalNode::GetBorderPen()
{
	return m_hBorderPen;
}

CString CFinalNode::GetLabel()
{
	return "";
}