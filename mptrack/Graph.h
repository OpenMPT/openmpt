#pragma once
class CSoundFile;
class CNode;
class CGraph
{
public:
	CGraph(void);
	~CGraph(void);
	void BuildNodePool(CSoundFile* pSndFile);
	void EmptyNodePool();
	
	int GetMapKeyFromChannel(int chan);
	int GetMapKeyFromInstrument(int instr);
	int GetMapKeyFromPlugSlot(int slot);
	int GetMapKeyForFinalNode();
	void Render(CDC *cdc); 

protected:
	CMap<int, int, CNode*, CNode*> m_nodePool;

};
