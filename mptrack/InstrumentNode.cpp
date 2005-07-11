#include "stdafx.h"
#include ".\instrumentnode.h"

CInstrumentNode::CInstrumentNode(void)
{
	m_hBorderPen = ::CreatePen(PS_SOLID,0, RGB(0x00, 0x00, 0xFF));
}

CInstrumentNode::~CInstrumentNode(void)
{
}

HPEN CInstrumentNode::GetBorderPen()
{
	return m_hBorderPen;
}

CString CInstrumentNode::GetLabel()
{
	return "";
}