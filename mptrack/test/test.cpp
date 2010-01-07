/*
 * Miscellaneous (unit) tests.
 * 
 */

#include "stdafx.h"
#include "test.h"
#include "../mptrack.h"
#include "../moddoc.h"
#include "../MainFrm.h"
#include "../version.h"
#include "../misc_util.h"
#include "../serialization_utils.h"
#include <limits>
#include <fstream>
#include <strstream>


#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

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

// Like VERIFY_EQUAL, but throws exception if comparison fails.
#define VERIFY_EQUAL_NONCONT(x,y) \
	if((x) != (y))		\
	{					\
		CString str;	\
		str.Format("File: " STRINGIZE(__FILE__) "\nLine: " STRINGIZE(__LINE__) "\n\nVERIFY_EQUAL failed when comparing\n" #x "\nand\n" #y); \
		std::string stdstr = str; \
		throw std::runtime_error(stdstr); \
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
void TestTypes();
void TestPCnoteSerialization();
void TestMisc();




void DoTests()
//------------
{
	DO_TEST(TestVersion);
	DO_TEST(TestTypes);
	//DO_TEST(TestPCnoteSerialization);
	DO_TEST(TestMisc);

	Log(TEXT("Tests were run\n"));
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
		// Ensure that bit-shifting works (used in some mod loaders for example)
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(01,17,00,00) == 0x0117 << 16 );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(01,17,03,00) >> 8 == 0x011703 );
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


void TestTypes()
//--------------
{
	VERIFY_EQUAL(int8_min, (std::numeric_limits<int8>::min)());
	VERIFY_EQUAL(int8_max, (std::numeric_limits<int8>::max)());
	VERIFY_EQUAL(uint8_max, (std::numeric_limits<uint8>::max)());

	VERIFY_EQUAL(int16_min, (std::numeric_limits<int16>::min)());
	VERIFY_EQUAL(int16_max, (std::numeric_limits<int16>::max)());
	VERIFY_EQUAL(uint16_max, (std::numeric_limits<uint16>::max)());

	VERIFY_EQUAL(int32_min, (std::numeric_limits<int32>::min)());
	VERIFY_EQUAL(int32_max, (std::numeric_limits<int32>::max)());
	VERIFY_EQUAL(uint32_max, (std::numeric_limits<uint32>::max)());

	VERIFY_EQUAL(int64_min, (std::numeric_limits<int64>::min)());
	VERIFY_EQUAL(int64_max, (std::numeric_limits<int64>::max)());
	VERIFY_EQUAL(uint64_max, (std::numeric_limits<uint64>::max)());

	VERIFY_EQUAL(ROWINDEX_MAX, (std::numeric_limits<ROWINDEX>::max)());
	VERIFY_EQUAL(ORDERINDEX_MAX, (std::numeric_limits<ORDERINDEX>::max)());
	VERIFY_EQUAL(PATTERNINDEX_MAX, (std::numeric_limits<PATTERNINDEX>::max)());
	VERIFY_EQUAL(SAMPLEINDEX_MAX, (std::numeric_limits<SAMPLEINDEX>::max)());
	VERIFY_EQUAL(INSTRUMENTINDEX_MAX, (std::numeric_limits<INSTRUMENTINDEX>::max)());
}


void TestMisc()
//-------------
{
	VERIFY_EQUAL(ConvertStrTo<uint32>("586"), 586);
	VERIFY_EQUAL(ConvertStrTo<uint32>("2147483647"), int32_max);
	VERIFY_EQUAL(ConvertStrTo<uint32>("4294967295"), uint32_max);

	VERIFY_EQUAL(ConvertStrTo<int64>("-9223372036854775808"), int64_min);
	VERIFY_EQUAL(ConvertStrTo<int64>("-159"), -159);
	VERIFY_EQUAL(ConvertStrTo<int64>("9223372036854775807"), int64_max);

	VERIFY_EQUAL(ConvertStrTo<uint64>("85059"), 85059);
	VERIFY_EQUAL(ConvertStrTo<uint64>("9223372036854775807"), int64_max);
	VERIFY_EQUAL(ConvertStrTo<uint64>("18446744073709551615"), uint64_max);

	VERIFY_EQUAL(ConvertStrTo<float>("-87.0"), -87.0);
	VERIFY_EQUAL(ConvertStrTo<double>("-0.5e-6"), -0.5e-6);
	VERIFY_EQUAL(ConvertStrTo<double>("58.65403492763"), 58.65403492763);
}


template<class T>
T Round(double a) {return static_cast<T>(floor(a + 0.5));}

double Rand01() {return rand() / double(RAND_MAX);}

template <class T>
T Rand(const T& min, const T& max) {return Round<T>(min + Rand01() * (max - min));}



void GenerateCommands(CPattern& pat, const double dProbPcs, const double dProbPc)
//-------------------------------------------------------------------------------
{
    const double dPcxProb = dProbPcs + dProbPc;
    const CPattern::const_iterator end = pat.End();
    for(CPattern::iterator i = pat.Begin(); i != end; i++)
    {
        const double rand = Rand01();
        if(rand < dPcxProb)
        {
            if(rand < dProbPcs)
                i->note = NOTE_PCS;
            else
                i->note = NOTE_PC;

            i->instr = Rand<BYTE>(0, MAX_MIXPLUGINS);
			i->SetValueVolCol(Rand<uint16>(0, MODCOMMAND::maxColumnValue));
			i->SetValueEffectCol(Rand<uint16>(0, MODCOMMAND::maxColumnValue));
        }
        else
            i->Clear();
    }
}



void TestPCnoteSerialization()
//----------------------------
{
	theApp.OnFileNewMPT();
	CMainFrame* pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm == nullptr)
		throw(std::runtime_error("pMainFrm is nullptr"));
	CModDoc* pModdoc = pMainFrm->GetActiveDoc();
	if(pModdoc == nullptr)
		throw(std::runtime_error("pModdoc is nullptr"));

	CSoundFile* pSndFile = pModdoc->GetSoundFile();
	if(pSndFile == nullptr)
		throw(std::runtime_error("pSndFile is nullptr"));

	// Set maximum number of channels.
	pSndFile->ReArrangeChannels(std::vector<CHANNELINDEX>(ModSpecs::mptm.channelsMax , 0));

	pSndFile->Patterns.Remove(0);
	pSndFile->Patterns.Insert(0, ModSpecs::mptm.patternRowsMin);
	pSndFile->Patterns.Insert(1, 64);
	GenerateCommands(pSndFile->Patterns[1], 0.3, 0.3);
	pSndFile->Patterns.Insert(2, ModSpecs::mptm.patternRowsMax);
	GenerateCommands(pSndFile->Patterns[2], 0.5, 0.5);

	//
	vector<MODCOMMAND> pat[3];
	const size_t numCommands[] = {	pSndFile->GetNumChannels() * pSndFile->Patterns[0].GetNumRows(),
									pSndFile->GetNumChannels() * pSndFile->Patterns[1].GetNumRows(),
									pSndFile->GetNumChannels() * pSndFile->Patterns[2].GetNumRows()
								 };
	pat[0].resize(numCommands[0]);
	pat[1].resize(numCommands[1]);
	pat[2].resize(numCommands[2]);

	for(size_t i = 0; i<3; i++) // Copy pattern data for comparison.
	{
		CPattern::const_iterator iter = pSndFile->Patterns[i].Begin();
		for(size_t j = 0; j < numCommands[i]; j++, iter++) pat[i][j] = *iter;
	}

	std::strstream mem;
	WriteModPatterns(mem, pSndFile->Patterns);

	VERIFY_EQUAL_NONCONT( mem.good(), true );

	// Clear patterns.
	pSndFile->Patterns[0].ClearCommands();
	pSndFile->Patterns[1].ClearCommands();
	pSndFile->Patterns[2].ClearCommands();

	// Read data back.
	ReadModPatterns(mem, pSndFile->Patterns);

	// Compare.
	VERIFY_EQUAL_NONCONT( pSndFile->Patterns[0].GetNumRows(), ModSpecs::mptm.patternRowsMin);
	VERIFY_EQUAL_NONCONT( pSndFile->Patterns[1].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT( pSndFile->Patterns[2].GetNumRows(), ModSpecs::mptm.patternRowsMax);
	for(size_t i = 0; i < 3; i++)
	{
		bool bPatternDataMatch = true;
		CPattern::const_iterator iter = pSndFile->Patterns[i].Begin();
		for(size_t j = 0; j < numCommands[i]; j++, iter++)
		{
			if(pat[i][j] != *iter)
			{
				bPatternDataMatch = false;
				break;
			}
		}
		VERIFY_EQUAL( bPatternDataMatch, true);
	}
		
	pModdoc->OnCloseDocument();
}


}; //Namespace MptTest

#else //Case: ENABLE_TESTS is not defined.

namespace MptTest
{
	void DoTests() {}
};

#endif

