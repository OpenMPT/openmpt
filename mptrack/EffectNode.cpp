#include "stdafx.h"
#include ".\effectnode.h"

CPluginNode::CPluginNode(void)
{
	m_hBorderPen = ::CreatePen(PS_SOLID,0, RGB(0x00, 0xFF, 0x00));
}

CPluginNode::~CPluginNode(void)
{
}

HPEN CPluginNode::GetBorderPen()
{
	return m_hBorderPen;
}

CString CPluginNode::GetLabel()
{
	return "";
}