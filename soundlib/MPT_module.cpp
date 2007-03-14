#include "stdafx.h"
#include "MPT_module.h"

const CMPTModule::MODIDTYPE CMPTModule::m_ModID = 0x2e6D7074; //.mpt ASCII in hex(. <-> 2e, m <-> 6D etc.)
//Used in serialization.


CMPTModule::CMPTModule(CModDoc* pModDoc)
//--------------------------------------
{
	Create(0, pModDoc, 0);
}

bool CMPTModule::ConvertToIT(CSoundFile*&)
//------------------------------------------------
{
	return false;
}

