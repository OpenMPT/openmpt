#pragma once
#include "node.h"

class CPluginNode :
	public CNode
{
public:
	CPluginNode(void);
	~CPluginNode(void);

	virtual CString GetLabel();
	virtual HPEN GetBorderPen();

private:
	HPEN m_hBorderPen;
};
