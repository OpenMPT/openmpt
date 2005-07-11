#include "stdafx.h"
#include ".\channelnode.h"

CChannelNode::CChannelNode(void)
{
	m_hBorderPen = ::CreatePen(PS_SOLID,0, RGB(0xFF, 0x00, 0x00));
}

CChannelNode::~CChannelNode(void)
{
}

HPEN CChannelNode::GetBorderPen()
{
	return m_hBorderPen;
}

CString CChannelNode::GetLabel()
{
	return "";
}