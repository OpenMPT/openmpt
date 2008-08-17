/*
 * Miscellaneous (unit) tests.
 * 
 */

#include "stdafx.h"
#include "test.h"
#include "../../soundlib/Sndfile.h"
#include "../version.h"
#include "../misc_util.h"

#ifdef ENABLE_TESTS

namespace MptTest
{

//Verify that given parameters are 'equal'(show error message if not).
//The exact meaning of equality is not specified; for now using operator!=.
//The macro is active in both 'debug' and 'release' build.
#define VERIFY_EQUAL(x,y)	\
	if((x) != (y))		\
	{					\
		CString str;	\
		str.Format("File: " STRINGIZE(__FILE__) "\nLine: " STRINGIZE(__LINE__) "\n\nVERIFY_EQUAL failed when comparing\n" #x "\nand\n" #y); \
		MessageBox(0, str, "VERIFY_EQUAL failed", MB_ICONERROR);	\
	}


#define DO_TEST(func)			\
try								\
{								\
	func();						\
}								\
catch(const std::exception& e)	\
{								\
	MessageBox(0, CString("Test \"" STRINGIZE(func) "\" threw an exception, message: ") + e.what(), "Test \"" STRINGIZE(func) "\": Exception was thrown", MB_ICONERROR); \
}	\
catch(...)  \
{			\
	MessageBox(0, CString("Test \"" STRINGIZE(func) "\" threw an unknown exception."), "Test \"" STRINGIZE(func) "\": Exception was thrown", MB_ICONERROR); \
}
	

void TestVersion();
void TestMisc();




void DoTests()
//------------
{
	DO_TEST(TestVersion);
	DO_TEST(TestMisc);
	MessageBox(0, "Tests were run", "Testing", MB_ICONINFORMATION);
}


void TestMisc()
{
	STATIC_ASSERT(SMP_16BIT == CHN_16BIT);
	STATIC_ASSERT(SMP_STEREO == CHN_STEREO);
	STATIC_ASSERT( sizeof(SNDMIXPLUGININFO) == 128 ); //Comment right after the struct said: "Size should be 128"
	VERIFY_EQUAL( (MAX_BASECHANNELS >= MPTM_SPECS.channelsMax), true );
}

void TestVersion()
//----------------
{
	//Verify that macros and functions work.
	{
		VERIFY_EQUAL( MPT_VERSION_NUMERIC, MptVersion::num );
		VERIFY_EQUAL( CString(MPT_VERSION_STR), CString(MptVersion::str) );
		VERIFY_EQUAL( MptVersion::ToNum(MptVersion::ToStr(MptVersion::num)), MptVersion::num );
		VERIFY_EQUAL( MptVersion::ToStr(MptVersion::ToNum(MptVersion::str)), MptVersion::str );
		VERIFY_EQUAL( MptVersion::ToStr(18285096), "1.17.02.28" );
		VERIFY_EQUAL( MptVersion::ToNum("1.17.02.28"), 18285096 );
		VERIFY_EQUAL( MptVersion::ToNum(MptVersion::str), MptVersion::num );
		VERIFY_EQUAL( MptVersion::ToStr(MptVersion::num), MptVersion::str );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(1,17,2,28) == 18285096 );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(1,17,02,48) == 18285128 );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(01,17,02,52) == 18285138 );
	}

	//Verify that the version obtained from the executable file is the same as 
	//defined in MptVersion.
	{
		char  szFullPath[MAX_PATH];
		DWORD dwVerHnd;
		DWORD dwVerInfoSize;

		// Get version information from the application
		::GetModuleFileName(NULL, szFullPath, sizeof(szFullPath));
		dwVerInfoSize = ::GetFileVersionInfoSize(szFullPath, &dwVerHnd);
		if (!dwVerInfoSize)
			throw std::runtime_error("!dwVerInfoSize is true");

		char* pVersionInfo = new char[dwVerInfoSize];
		if (!pVersionInfo)
			throw std::runtime_error("!pVersionInfo is true");

		char* szVer = NULL;
		UINT uVerLength;
		if (!(::GetFileVersionInfo((LPTSTR)szFullPath, (DWORD)dwVerHnd, 
								   (DWORD)dwVerInfoSize, (LPVOID)pVersionInfo))) {
			delete[] pVersionInfo;
			throw std::runtime_error("GetFileVersionInfo() returned false");
		}   
		if (!(::VerQueryValue(pVersionInfo, TEXT("\\StringFileInfo\\040904b0\\FileVersion"), 
							  (LPVOID*)&szVer, &uVerLength))) {
			delete[] pVersionInfo;
			throw std::runtime_error("VerQueryValue() returned false");
		}

		CString version = szVer;	
		delete[] pVersionInfo;

		//version string should be like: 1,17,2,38  Change ',' to '.' to get format 1.17.2.38
		version.Replace(',', '.');

		VERIFY_EQUAL( version, MptVersion::str );
		VERIFY_EQUAL( MptVersion::ToNum(version), MptVersion::num );
	}
}

}; //Namespace MptTest

#else //Case: ENABLE_TESTS is not defined.

namespace MptTest
{
	void DoTests() {}
};

#endif

