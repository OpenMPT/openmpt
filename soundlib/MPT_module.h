#ifndef MPT_MODULE_H
#define MPT_MODULE_H

#include "sndfile.h"
#include "tuning.h"
#include <string>
using std::string;

class CModDoc;


//==================================
class CMPTModule : public CSoundFile
//==================================
{
public:
	typedef int MODIDTYPE;
	string GetModuleFileExtension() {return ".mptm";}
	MODIDTYPE GetModuleID() {return m_ModID;}

	//Constructors
	CMPTModule(CModDoc* p);

	//Format convertors. 
	static bool ConvertToIT(CSoundFile*&);

	static const MODIDTYPE m_ModID;
protected:
private:

};

#endif
