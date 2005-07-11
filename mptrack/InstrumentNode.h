#pragma once
#include "node.h"

class CInstrumentNode :
	public CNode
{
public:
	CInstrumentNode(void);
	~CInstrumentNode(void);

	virtual CString GetLabel();
	virtual HPEN GetBorderPen();

private:
	HPEN m_hBorderPen;
};
