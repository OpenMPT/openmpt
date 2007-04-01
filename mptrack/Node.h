#pragma once
#include ".\vertex.h"
class CVertex;

typedef CArray<CVertex, CVertex> VertexArray;

enum {
	nodeWidth = 50,
	nodeHeight = 20,
};


class CNode
{
public:
	CNode(void);
	~CNode(void);

	void Render(CDC* cdc);
	virtual CString GetLabel() = 0;
	virtual HPEN GetBorderPen() = 0;

	int ConnectInput(int input, CNode sourceNode, int sourceNodeOutput);

protected:
	int x,y;
	int m_nOutputs;
	int m_nInputs;
	VertexArray m_inputs;
//	VertexArray m_outputs;
};
