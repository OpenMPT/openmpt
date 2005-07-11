#pragma once
#include "node.h"

class CFinalNode :
	public CNode
{
public:
	CFinalNode(void);
	~CFinalNode(void);

	virtual CString GetLabel();
	virtual HPEN GetBorderPen();

private:
	HPEN m_hBorderPen;
};
