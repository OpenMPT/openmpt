#pragma once
#include "afxtempl.h"

class CNode;

struct Link
{
	CNode* node;
	int vertexID;
};

enum {		//vertex types
	INPUT_VERTEX = 0,
	OUTPUT_VERTEX = 1
};

class CVertex
{
public:
	CVertex(void);
	~CVertex(void);

protected:
	int m_nType;
	CArray<Link, Link> m_linkArray;
};
