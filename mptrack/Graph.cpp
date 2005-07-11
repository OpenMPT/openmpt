#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "node.h"
#include "channelnode.h"
#include "instrumentnode.h"
#include "effectnode.h"
#include "finalnode.h"
#include "sndfile.h"
#include ".\graph.h"

CGraph::CGraph()
{
}

CGraph::~CGraph(void)
{
	EmptyNodePool();
}


int CGraph::GetMapKeyFromChannel(int chan) 
{
	return chan;
}

int CGraph::GetMapKeyFromInstrument(int instr) 
{
	return MAX_CHANNELS + instr;
}

int CGraph::GetMapKeyFromPlugSlot(int slot)
{
	return MAX_CHANNELS + MAX_INSTRUMENTS + slot;
}

int CGraph::GetMapKeyForFinalNode() {
	return MAX_CHANNELS + MAX_INSTRUMENTS + MAX_MIXPLUGINS;
}

void CGraph::BuildNodePool(CSoundFile* pSndFile)
//----------------------------------------------
{
	EmptyNodePool();
	
	CNode* newNode;
	
	//Add final (master out) node
	newNode = new CFinalNode();
	m_nodePool.SetAt(GetMapKeyForFinalNode(), newNode);

	//Add nodes for all channels
	for (int chan=0; chan<pSndFile->m_nChannels; chan++) {
		newNode = new CChannelNode();
		m_nodePool.SetAt(GetMapKeyFromChannel(chan), newNode);
	}

	//Add nodes for all instruments
	for (int inst=0; inst<pSndFile->m_nInstruments; inst++) {
		newNode = new CInstrumentNode();
		m_nodePool.SetAt(GetMapKeyFromInstrument(inst), newNode);
	}

	//Add nodes for all plugins
	for (int plug=0; plug<MAX_MIXPLUGINS; plug++) {
		if (pSndFile->m_MixPlugins[plug].pMixPlugin != NULL) {
			newNode = new CPluginNode();
			m_nodePool.SetAt(GetMapKeyFromPlugSlot(plug), newNode);
		}
	}
}

void CGraph::EmptyNodePool() 
{
	POSITION pos = m_nodePool.GetStartPosition();
	while( pos != NULL ) {
		CNode* pNode;
		int key;
		m_nodePool.GetNextAssoc(pos, key, pNode);
		delete pNode;
	}
	
	m_nodePool.RemoveAll();
}


void CGraph::Render(CDC *cdc)
{
	POSITION pos = m_nodePool.GetStartPosition();
	while( pos != NULL ) {
		CNode* pNode;
		int key;
		m_nodePool.GetNextAssoc(pos, key, pNode);
		pNode->Render(cdc);
	}
}