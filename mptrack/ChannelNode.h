#pragma once
#include "node.h"

class CChannelNode :
	public CNode
{
public:
	CChannelNode(void);
	~CChannelNode(void);

	virtual CString GetLabel();
	virtual HPEN GetBorderPen();

private:
	HPEN m_hBorderPen;
};
