/*
 * test.cpp
 * --------
 * Purpose: Unit tests for OpenMPT.
 * Notes  : We need FAAAAAAAR more unit tests!
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "test.h"


#ifdef ENABLE_TESTS


#include "../common/version.h"
#include "../common/misc_util.h"
#include "../common/StringFixer.h"
#include "../common/serialization_utils.h"
#include "../soundlib/Sndfile.h"
#include "../soundlib/FileReader.h"
#include "../soundlib/MIDIEvents.h"
#include "../soundlib/MIDIMacros.h"
#include "../soundlib/SampleFormatConverters.h"
#include "../soundlib/ITCompression.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/mptrack.h"
#include "../mptrack/moddoc.h"
#include "../mptrack/MainFrm.h"
#endif // MODPLUG_TRACKER
#ifndef MODPLUG_TRACKER
#include "../common/mptFstream.h"
#endif // !MODPLUG_TRACKER
#include <limits>
#ifndef MODPLUG_TRACKER
#include <memory>
#include <iostream>
#endif // !MODPLUG_TRACKER
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>

#ifdef _DEBUG
#if MPT_COMPILER_MSVC && defined(_MFC_VER)
	#define new DEBUG_NEW
#endif
#endif

#ifndef _T
#define _T MPT_TEXT
#endif


namespace MptTest
{



#ifdef THIS_FILE
#undef THIS_FILE
#endif
#ifdef _MSC_VER
#define THIS_FILE "..\\test\\test.cpp" // __FILE__
#else
#define THIS_FILE "test/test.cpp" // __FILE__
#endif



#ifdef MODPLUG_TRACKER

static noinline void ReportError(const char * file, int line, const char * description, bool abort)
//-------------------------------------------------------------------------------------------------
{
	std::string pos = std::string() + file + "(" + Stringify(line) + "):\n";
	std::string message = pos + "Test failed: '" + description + "'";
	if(IsDebuggerPresent())
	{
		message += "\n";
		OutputDebugString(message.c_str());
	} else
	{
		if(abort)
		{
			throw std::runtime_error(message);
		} else
		{
			Reporting::Error(message.c_str(), "OpenMPT TestSuite");
		}
	}
}

static noinline void ReportExceptionError(const char * file, int line, const char * description, const char * exception_what)
//---------------------------------------------------------------------------------------------------------------------------
{
	std::string pos = std::string() + file + "(" + Stringify(line) + "):\n";
	std::string message;
	if(exception_what)
	{
		message = pos + "Test '" + description + "' threw an exception, message:\n" + exception_what;
	} else
	{
		message = pos + "Test '" + description + "' threw an unknown exception.";
	}
	if(IsDebuggerPresent())
	{
		message += "\n";
		OutputDebugString(message.c_str());
	} else
	{
		Reporting::Error(message.c_str(), "OpenMPT TestSuite");
	}
}

static noinline void ReportException(const char * const file, const int line, const char * const description, bool rethrow)
//-------------------------------------------------------------------------------------------------------------------------
{
	try
	{
		throw; // get the exception
	} catch(std::exception &e)
	{
		ReportExceptionError(file, line, description, e.what());
		if(rethrow)
		{
			throw;
		}
	} catch(...)
	{
		ReportExceptionError(file, line, description, nullptr);
		if(rethrow)
		{
			throw;
		}
	}
}

#if defined(_MSC_VER) && defined(_M_IX86)
// on x86, break directly using asm break interrupt instead of calling DebugBreak which breaks one stackframe deeper than we want
#define MyDebugBreak() do { __asm { int 3 }; } while(0)
#else
#define MyDebugBreak() DebugBreak()
#endif

#define ReportExceptionAndBreak(file, line, description, rethrow) do { ReportException(file, line, description, rethrow); if(IsDebuggerPresent()) MyDebugBreak(); } while(0)
#define ReportErrorAndBreak(file, line, description, progress)    do { ReportError(file, line, description, progress);    if(IsDebuggerPresent()) MyDebugBreak(); } while(0)

#define MULTI_TEST_TRY   try {
#define MULTI_TEST_START 
#define MULTI_TEST_END   
#define MULTI_TEST_CATCH } catch(...) { ReportExceptionAndBreak(THIS_FILE, __LINE__, description, false); }
#define TEST_TRY         try {
#define TEST_CATCH       } catch(...) { ReportExceptionAndBreak(THIS_FILE, __LINE__, description, true); }
#define TEST_START()     do { } while(0)
#define TEST_OK()        do { MPT_UNREFERENCED_PARAMETER(description); } while(0)
#define TEST_FAIL()      ReportErrorAndBreak(THIS_FILE, __LINE__, description, false)
#define TEST_FAIL_STOP() ReportErrorAndBreak(THIS_FILE, __LINE__, description, true)

#else // !MODPLUG_TRACKER

static std::string remove_newlines(std::string str)
{
	return mpt::String::Replace(mpt::String::Replace(str, "\n", " "), "\r", " ");
}

static noinline void show_start(const char * const file, const int line, const char * const description)
{
	std::cout << "Test: " << file << "(" << line << "): " << remove_newlines(description) << ": ";
}

static noinline void show_ok(const char * const file, const int line, const char * const description)
{
	MPT_UNREFERENCED_PARAMETER(file);
	MPT_UNREFERENCED_PARAMETER(line);
	MPT_UNREFERENCED_PARAMETER(description);
	std::cout << "PASS" << std::endl;
}

static noinline void show_fail(const char * const file, const int line, const char * const description, bool exception = false, const char * const exception_text = nullptr)
{
	std::cout << "FAIL" << std::endl;
	if(!exception)
	{
		std::cerr << "FAIL: " << file << "(" << line << "): " << remove_newlines(description) << std::endl;
	} else if(!exception_text || (exception_text && std::string(exception_text).empty()))
	{
		std::cerr << "FAIL: " << file << "(" << line << "): " << remove_newlines(description) << " EXCEPTION!" << std::endl;
	} else
	{
		std::cerr << "FAIL: " << file << "(" << line << "): " << remove_newlines(description) << " EXCEPTION: " << exception_text << std::endl;
	}
}

static int fail_count = 0;

static noinline void ReportException(const char * const file, const int line, const char * const description)
{
	try
	{
		throw; // get the exception
	} catch(std::exception & e)
	{
		show_fail(file, line, description, true, e.what());
		throw; // rethrow
	} catch(...)
	{
		show_fail(file, line, description, true);
		throw; // rethrow
	}
}

#define MULTI_TEST_TRY   try { \
                          fail_count = 0;
#define MULTI_TEST_START show_start(THIS_FILE, __LINE__, description); std::cout << "..." << std::endl;
#define MULTI_TEST_END   show_start(THIS_FILE, __LINE__, description);
#define MULTI_TEST_CATCH  if(fail_count > 0) { \
                           throw std::runtime_error("Test failed."); \
                          } \
                          show_ok(THIS_FILE, __LINE__, description); \
                         } catch(...) { \
                          ReportException(THIS_FILE, __LINE__, description); \
                         }
#define TEST_TRY         try {
#define TEST_CATCH       } catch(...) { \
                          fail_count++; \
                          ReportException(THIS_FILE, __LINE__, description); \
                          throw; \
                         }
#define TEST_START()     show_start(THIS_FILE, __LINE__, description)
#define TEST_OK()        show_ok(THIS_FILE, __LINE__, description)
#define TEST_FAIL()      do { show_fail(THIS_FILE, __LINE__, description); fail_count++; } while(0)
#define TEST_FAIL_STOP() do { show_fail(THIS_FILE, __LINE__, description); fail_count++; throw std::runtime_error(std::string("Test failed: ") + description); } while(0)

#endif // MODPLUG_TRACKER



//Verify that given parameters are 'equal'(show error message if not).
//The exact meaning of equality is not specified; for now using operator!=.
//The macro is active in both 'debug' and 'release' build.
#define VERIFY_EQUAL(x,y)	\
do{ \
	const char * const description = #x " == " #y ; \
	TEST_TRY \
	TEST_START(); \
	if((x) != (y))		\
	{					\
		TEST_FAIL(); \
	} else \
	{ \
		TEST_OK(); \
	} \
	TEST_CATCH \
}while(0)


// Like VERIFY_EQUAL, but throws exception if comparison fails.
#define VERIFY_EQUAL_NONCONT(x,y) \
do{ \
	const char * const description = #x " == " #y ; \
	TEST_TRY \
	TEST_START(); \
	if((x) != (y))		\
	{					\
		TEST_FAIL_STOP(); \
	} else \
	{ \
		TEST_OK(); \
	} \
	TEST_CATCH \
}while(0)


// Like VERIFY_EQUAL_NONCONT, but do not show message if test succeeds
#define VERIFY_EQUAL_QUIET_NONCONT(x,y) \
do{ \
	const char * const description = #x " == " #y ; \
	TEST_TRY \
	if((x) != (y))		\
	{					\
		TEST_FAIL_STOP(); \
	} \
	TEST_CATCH \
}while(0)


#define DO_TEST(func) \
do{ \
	const char * description = #func ; \
	MULTI_TEST_TRY \
	MULTI_TEST_START \
	func(); \
	MULTI_TEST_END \
	MULTI_TEST_CATCH \
}while(0)




void TestVersion();
void TestTypes();
void TestLoadSaveFile();
void TestPCnoteSerialization();
void TestMisc();
void TestMIDIEvents();
void TestStringIO();
void TestSampleConversion();
void TestITCompression();




void DoTests()
//------------
{
	DO_TEST(TestVersion);
	DO_TEST(TestTypes);
	DO_TEST(TestMisc);
	DO_TEST(TestStringIO);
	DO_TEST(TestMIDIEvents);
	DO_TEST(TestSampleConversion);
	DO_TEST(TestITCompression);

	// slower tests, require opening a CModDoc
	DO_TEST(TestPCnoteSerialization);
	DO_TEST(TestLoadSaveFile);

}


// Test if functions related to program version data work
void TestVersion()
//----------------
{
	//Verify that macros and functions work.
	{
		VERIFY_EQUAL( MptVersion::ToNum(MptVersion::ToStr(MptVersion::num)), MptVersion::num );
		VERIFY_EQUAL( MptVersion::ToStr(MptVersion::ToNum(MptVersion::str)), MptVersion::str );
		VERIFY_EQUAL( MptVersion::ToStr(18285096), "1.17.02.28" );
		VERIFY_EQUAL( MptVersion::ToNum("1.17.02.28"), 18285096 );
		VERIFY_EQUAL( MptVersion::ToNum(MptVersion::str), MptVersion::num );
		VERIFY_EQUAL( MptVersion::ToStr(MptVersion::num), MptVersion::str );
		VERIFY_EQUAL( MptVersion::RemoveBuildNumber(MAKE_VERSION_NUMERIC(1,19,02,00)), MAKE_VERSION_NUMERIC(1,19,02,00));
		VERIFY_EQUAL( MptVersion::RemoveBuildNumber(MAKE_VERSION_NUMERIC(1,18,03,20)), MAKE_VERSION_NUMERIC(1,18,03,00));
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,18,01,13)), true);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,19,01,00)), false);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,17,02,54)), false);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,18,00,00)), false);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,18,02,00)), false);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,18,02,01)), true);
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(1,17,2,28) == 18285096 );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(1,17,02,48) == 18285128 );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(01,17,02,52) == 18285138 );
		// Ensure that bit-shifting works (used in some mod loaders for example)
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(01,17,00,00) == 0x0117 << 16 );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(01,17,03,00) >> 8 == 0x011703 );
	}

#ifdef MODPLUG_TRACKER
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

		char *pVersionInfo;
		try
		{
			pVersionInfo = new char[dwVerInfoSize];
		} catch(MPTMemoryException)
		{
			throw std::runtime_error("Could not allocate memory for pVersionInfo");
		}

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

		std::string version = szVer;	
		delete[] pVersionInfo;

		//version string should be like: 1,17,2,38  Change ',' to '.' to get format 1.17.2.38
		version = mpt::String::Replace(version, ",", ".");

		VERIFY_EQUAL( version, MptVersion::str );
		VERIFY_EQUAL( MptVersion::ToNum(version), MptVersion::num );
	}
#endif
}


// Test if data types are interpreted correctly
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
}


static float AsFloat(uint32 x)
//----------------------------
{
	FloatInt32 conv;
	conv.i = x;
	return conv.f;
}

static uint32 AsInt(float x)
//--------------------------
{
	FloatInt32 conv;
	conv.f = x;
	return conv.i;
}

void TestMisc()
//-------------
{

	VERIFY_EQUAL(0x3f800000u, AsInt(1.0f));
	VERIFY_EQUAL(AsFloat(0x3f800000u),  1.0f);
	VERIFY_EQUAL(AsFloat(0x00000000u),  0.0f);
	VERIFY_EQUAL(AsFloat(0xbf800000u), -1.0f);
	VERIFY_EQUAL(DecodeFloatNE(0x3f800000u), 1.0f);
	VERIFY_EQUAL(DecodeFloatLE(uint8_4(0x00,0x00,0x80,0x3f)), 1.0f);
	VERIFY_EQUAL(DecodeFloatBE(uint8_4(0x3f,0x80,0x00,0x00)), 1.0f);
	VERIFY_EQUAL(EncodeFloatNE(1.0f), 0x3f800000u);
	VERIFY_EQUAL(EncodeFloatBE(1.0f).GetBE(), 0x3f800000u);
	VERIFY_EQUAL(EncodeFloatLE(1.0f).GetBE(), 0x0000803fu);
	VERIFY_EQUAL(EncodeFloatLE(1.0f).GetLE(), 0x3f800000u);
	VERIFY_EQUAL(EncodeFloatBE(1.0f).GetLE(), 0x0000803fu);

	VERIFY_EQUAL(ConvertStrTo<uint32>("586"), 586);
	VERIFY_EQUAL(ConvertStrTo<uint32>("2147483647"), (uint32)int32_max);
	VERIFY_EQUAL(ConvertStrTo<uint32>("4294967295"), uint32_max);

	VERIFY_EQUAL(ConvertStrTo<int64>("-9223372036854775808"), int64_min);
	VERIFY_EQUAL(ConvertStrTo<int64>("-159"), -159);
	VERIFY_EQUAL(ConvertStrTo<int64>("9223372036854775807"), int64_max);

	VERIFY_EQUAL(ConvertStrTo<uint64>("85059"), 85059);
	VERIFY_EQUAL(ConvertStrTo<uint64>("9223372036854775807"), (uint64)int64_max);
	VERIFY_EQUAL(ConvertStrTo<uint64>("18446744073709551615"), uint64_max);

	VERIFY_EQUAL(ConvertStrTo<float>("-87.0"), -87.0);
	VERIFY_EQUAL(ConvertStrTo<double>("-0.5e-6"), -0.5e-6);
	VERIFY_EQUAL(ConvertStrTo<double>("58.65403492763"), 58.65403492763);

	VERIFY_EQUAL(ModCommand::IsPcNote(NOTE_MAX), false);
	VERIFY_EQUAL(ModCommand::IsPcNote(NOTE_PC), true);
	VERIFY_EQUAL(ModCommand::IsPcNote(NOTE_PCS), true);

	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".mod"), MOD_TYPE_MOD);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("mod"), MOD_TYPE_MOD);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".s3m"), MOD_TYPE_S3M);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("s3m"), MOD_TYPE_S3M);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".xm"), MOD_TYPE_XM);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("xm"), MOD_TYPE_XM);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".it"), MOD_TYPE_IT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("it"), MOD_TYPE_IT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".itp"), MOD_TYPE_IT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("itp"), MOD_TYPE_IT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("mptm"), MOD_TYPE_MPT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("invalidExtension"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("ita"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("s2m"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(""), MOD_TYPE_NONE);

	VERIFY_EQUAL( Util::Round(1.99), 2.0 );
	VERIFY_EQUAL( Util::Round(1.5), 2.0 );
	VERIFY_EQUAL( Util::Round(1.1), 1.0 );
	VERIFY_EQUAL( Util::Round(-0.1), 0.0 );
	VERIFY_EQUAL( Util::Round(-0.5), -1.0 );
	VERIFY_EQUAL( Util::Round(-0.9), -1.0 );
	VERIFY_EQUAL( Util::Round(-1.4), -1.0 );
	VERIFY_EQUAL( Util::Round(-1.7), -2.0 );
	VERIFY_EQUAL( Util::Round<int32>(int32_max + 0.1), int32_max );
	VERIFY_EQUAL( Util::Round<int32>(int32_max - 0.4), int32_max );
	VERIFY_EQUAL( Util::Round<int32>(int32_min + 0.1), int32_min );
	VERIFY_EQUAL( Util::Round<int32>(int32_min - 0.1), int32_min );
	VERIFY_EQUAL( Util::Round<uint32>(uint32_max + 0.499), uint32_max );
	VERIFY_EQUAL( Util::Round<int8>(110.1), 110 );
	VERIFY_EQUAL( Util::Round<int8>(-110.1), -110 );

	// trivials
	VERIFY_EQUAL( mpt::saturate_cast<int>(-1), -1 );
	VERIFY_EQUAL( mpt::saturate_cast<int>(0), 0 );
	VERIFY_EQUAL( mpt::saturate_cast<int>(1), 1 );
	VERIFY_EQUAL( mpt::saturate_cast<int>(std::numeric_limits<int>::min()), std::numeric_limits<int>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<int>(std::numeric_limits<int>::max()), std::numeric_limits<int>::max() );

	// signed / unsigned
	VERIFY_EQUAL( mpt::saturate_cast<int16>(std::numeric_limits<uint16>::min()), std::numeric_limits<uint16>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<int16>(std::numeric_limits<uint16>::max()), std::numeric_limits<int16>::max() );
	VERIFY_EQUAL( mpt::saturate_cast<int32>(std::numeric_limits<uint32>::min()), (int32)std::numeric_limits<uint32>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<int32>(std::numeric_limits<uint32>::max()), std::numeric_limits<int32>::max() );
	VERIFY_EQUAL( mpt::saturate_cast<int64>(std::numeric_limits<uint64>::min()), (int64)std::numeric_limits<uint64>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<int64>(std::numeric_limits<uint64>::max()), std::numeric_limits<int64>::max() );
	VERIFY_EQUAL( mpt::saturate_cast<uint16>(std::numeric_limits<int16>::min()), std::numeric_limits<uint16>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<uint16>(std::numeric_limits<int16>::max()), std::numeric_limits<int16>::max() );
	VERIFY_EQUAL( mpt::saturate_cast<uint32>(std::numeric_limits<int32>::min()), std::numeric_limits<uint32>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<uint32>(std::numeric_limits<int32>::max()), (uint32)std::numeric_limits<int32>::max() );
	VERIFY_EQUAL( mpt::saturate_cast<uint64>(std::numeric_limits<int64>::min()), std::numeric_limits<uint64>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<uint64>(std::numeric_limits<int64>::max()), (uint64)std::numeric_limits<int64>::max() );
	
	// overflow
	VERIFY_EQUAL( mpt::saturate_cast<int16>(std::numeric_limits<int16>::min() - 1), std::numeric_limits<int16>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<int16>(std::numeric_limits<int16>::max() + 1), std::numeric_limits<int16>::max() );
	VERIFY_EQUAL( mpt::saturate_cast<int32>(std::numeric_limits<int32>::min() - int64(1)), std::numeric_limits<int32>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<int32>(std::numeric_limits<int32>::max() + int64(1)), std::numeric_limits<int32>::max() );

	VERIFY_EQUAL( mpt::saturate_cast<uint16>(std::numeric_limits<int16>::min() - 1), std::numeric_limits<uint16>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<uint16>(std::numeric_limits<int16>::max() + 1), (uint16)std::numeric_limits<int16>::max() + 1 );
	VERIFY_EQUAL( mpt::saturate_cast<uint32>(std::numeric_limits<int32>::min() - int64(1)), std::numeric_limits<uint32>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<uint32>(std::numeric_limits<int32>::max() + int64(1)), (uint32)std::numeric_limits<int32>::max() + 1 );

	VERIFY_EQUAL( mpt::saturate_cast<uint32>(static_cast<double>(std::numeric_limits<int64>::max())), std::numeric_limits<uint32>::max() );

	VERIFY_EQUAL( mpt::String::LTrim(" "), "" );
	VERIFY_EQUAL( mpt::String::RTrim(" "), "" );
	VERIFY_EQUAL( mpt::String::Trim(" "), "" );

	// weird things with std::string containing \0 in the middle and trimming \0
	VERIFY_EQUAL( std::string("\0\ta\0b ",6).length(), 6 );
	VERIFY_EQUAL( mpt::String::RTrim(std::string("\0\ta\0b ",6)), std::string("\0\ta\0b",5) );
	VERIFY_EQUAL( mpt::String::Trim(std::string("\0\ta\0b\0",6),std::string("\0",1)), std::string("\ta\0b",4) );

	// These should fail to compile
	//Util::Round<std::string>(1.0);
	//Util::Round<int64>(1.0);
	//Util::Round<uint64>(1.0);
	
	// This should trigger assert in Round.
	//VERIFY_EQUAL( Util::Round<int8>(-129), 0 );

	// Check for completeness of supported effect list in mod specifications
	for(size_t i = 0; i < CountOf(ModSpecs::Collection); i++)
	{
		VERIFY_EQUAL(strlen(ModSpecs::Collection[i]->commands), MAX_EFFECTS);
		VERIFY_EQUAL(strlen(ModSpecs::Collection[i]->volcommands), MAX_VOLCMDS);
	}

}


// Test MIDI Event generating / reading
void TestMIDIEvents()
//-------------------
{
	uint32 midiEvent;
	
	midiEvent = MIDIEvents::CC(MIDIEvents::MIDICC_Balance_Coarse, 13, 40);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evControllerChange);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 13);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), MIDIEvents::MIDICC_Balance_Coarse);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 40);

	midiEvent = MIDIEvents::NoteOn(10, 50, 120);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evNoteOn);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 10);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 50);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 120);

	midiEvent = MIDIEvents::NoteOff(15, 127, 42);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evNoteOff);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 15);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 127);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 42);

	midiEvent = MIDIEvents::ProgramChange(1, 127);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evProgramChange);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 1);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 127);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 0);

	midiEvent = MIDIEvents::PitchBend(2, MIDIEvents::pitchBendCentre);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evPitchBend);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 2);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 0x00);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 0x40);

	midiEvent = MIDIEvents::System(MIDIEvents::sysStart);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evSystem);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), MIDIEvents::sysStart);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 0);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 0);
}


// Check if our test file was loaded correctly.
void TestLoadXMFile(const CSoundFile &sndFile)
//--------------------------------------------
{
#ifdef MODPLUG_TRACKER
	const CModDoc *pModDoc = sndFile.GetpModDoc();
#endif // MODPLUG_TRACKER

	// Global Variables
	VERIFY_EQUAL_NONCONT(sndFile.GetTitle(), "Test Module");
	VERIFY_EQUAL_NONCONT(sndFile.songMessage.at(0), 'O');
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultTempo, 139);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultSpeed, 5);
	VERIFY_EQUAL_NONCONT(sndFile.m_nGlobalVolume, 128);
	VERIFY_EQUAL_NONCONT(sndFile.m_nVSTiVolume, 42);
	VERIFY_EQUAL_NONCONT(sndFile.m_nSamplePreAmp, 23);
	VERIFY_EQUAL_NONCONT((sndFile.m_SongFlags & SONG_FILE_FLAGS), SONG_EMBEDMIDICFG | SONG_LINEARSLIDES | SONG_EXFILTERRANGE);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_COMPATIBLE_PLAY), true);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_MIDICC_BUGEMULATION), false);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_OLDVOLSWING), false);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_OLD_MIDI_PITCHBENDS), false);
	VERIFY_EQUAL_NONCONT(sndFile.GetMixLevels(), mixLevels_compatible);
	VERIFY_EQUAL_NONCONT(sndFile.m_nTempoMode, tempo_mode_modern);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultRowsPerBeat, 6);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultRowsPerMeasure, 12);
	VERIFY_EQUAL_NONCONT(sndFile.m_dwCreatedWithVersion, MAKE_VERSION_NUMERIC(1, 19, 02, 05));
	VERIFY_EQUAL_NONCONT(sndFile.m_nRestartPos, 1);

	// Macros
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(0), sfx_reso);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(1), sfx_drywet);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetFixedMacroType(), zxx_resomode);

	// Channels
	VERIFY_EQUAL_NONCONT(sndFile.GetNumChannels(), 2);
	VERIFY_EQUAL_NONCONT(strcmp(sndFile.ChnSettings[0].szName, "First Channel"), 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nMixPlugin, 0);

	VERIFY_EQUAL_NONCONT(strcmp(sndFile.ChnSettings[1].szName, "Second Channel"), 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nMixPlugin, 1);

	// Samples
	VERIFY_EQUAL_NONCONT(sndFile.GetNumSamples(), 3);
	VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[1], "Pulse Sample"), 0);
	VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[2], "Empty Sample"), 0);
	VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[3], "Unassigned Sample"), 0);
#ifdef MODPLUG_TRACKER
	VERIFY_EQUAL_NONCONT(pModDoc->FindSampleParent(1), 1);
	VERIFY_EQUAL_NONCONT(pModDoc->FindSampleParent(2), 1);
	VERIFY_EQUAL_NONCONT(pModDoc->FindSampleParent(3), INSTRUMENTINDEX_INVALID);
#endif // MODPLUG_TRACKER
	const ModSample &sample = sndFile.GetSample(1);
	VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
	VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
	VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
	VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 16);
	VERIFY_EQUAL_NONCONT(sample.nFineTune, 35);
	VERIFY_EQUAL_NONCONT(sample.RelativeTone, 1);
	VERIFY_EQUAL_NONCONT(sample.nVolume, 32 * 4);
	VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 64);
	VERIFY_EQUAL_NONCONT(sample.nPan, 160);
	VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_PANNING | CHN_LOOP | CHN_PINGPONGLOOP);

	VERIFY_EQUAL_NONCONT(sample.nLoopStart, 1);
	VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 8);

	VERIFY_EQUAL_NONCONT(sample.nVibType, VIB_SQUARE);
	VERIFY_EQUAL_NONCONT(sample.nVibSweep, 3);
	VERIFY_EQUAL_NONCONT(sample.nVibRate, 4);
	VERIFY_EQUAL_NONCONT(sample.nVibDepth, 5);

	// Sample Data
	const int8 *p8 = static_cast<const int8 *>(sample.pSample);
	for(size_t i = 0; i < 6; i++)
	{
		VERIFY_EQUAL_NONCONT(p8[i], 18);
	}
	for(size_t i = 6; i < 16; i++)
	{
		VERIFY_EQUAL_NONCONT(p8[i], 0);
	}

	// Instruments
	VERIFY_EQUAL_NONCONT(sndFile.GetNumInstruments(), 1);
	const ModInstrument *pIns = sndFile.Instruments[1];
	VERIFY_EQUAL_NONCONT(pIns->nFadeOut, 1024);
	VERIFY_EQUAL_NONCONT(pIns->nPan, 128);
	VERIFY_EQUAL_NONCONT(pIns->dwFlags, InstrumentFlags(0));

	VERIFY_EQUAL_NONCONT(pIns->nPPS, 0);
	VERIFY_EQUAL_NONCONT(pIns->nPPC, NOTE_MIDDLEC - 1);

	VERIFY_EQUAL_NONCONT(pIns->nVolRampUp, 1200);
	VERIFY_EQUAL_NONCONT(pIns->nResampling, SRCMODE_POLYPHASE);

	VERIFY_EQUAL_NONCONT(pIns->IsCutoffEnabled(), false);
	VERIFY_EQUAL_NONCONT(pIns->GetCutoff(), 0);
	VERIFY_EQUAL_NONCONT(pIns->IsResonanceEnabled(), false);
	VERIFY_EQUAL_NONCONT(pIns->GetResonance(), 0);
	VERIFY_EQUAL_NONCONT(pIns->nFilterMode, FLTMODE_UNCHANGED);

	VERIFY_EQUAL_NONCONT(pIns->nVolSwing, 0);
	VERIFY_EQUAL_NONCONT(pIns->nPanSwing, 0);
	VERIFY_EQUAL_NONCONT(pIns->nCutSwing, 0);
	VERIFY_EQUAL_NONCONT(pIns->nResSwing, 0);

	VERIFY_EQUAL_NONCONT(pIns->nNNA, NNA_NOTECUT);
	VERIFY_EQUAL_NONCONT(pIns->nDCT, DCT_NONE);

	VERIFY_EQUAL_NONCONT(pIns->nMixPlug, 1);
	VERIFY_EQUAL_NONCONT(pIns->nMidiChannel, 16);
	VERIFY_EQUAL_NONCONT(pIns->nMidiProgram, 64);
	VERIFY_EQUAL_NONCONT(pIns->wMidiBank, 2);
	VERIFY_EQUAL_NONCONT(pIns->midiPWD, 8);

	VERIFY_EQUAL_NONCONT(pIns->pTuning, sndFile.GetDefaultTuning());

	VERIFY_EQUAL_NONCONT(pIns->wPitchToTempoLock, 0);

	VERIFY_EQUAL_NONCONT(pIns->nPluginVelocityHandling, PLUGIN_VELOCITYHANDLING_VOLUME);
	VERIFY_EQUAL_NONCONT(pIns->nPluginVolumeHandling, PLUGIN_VOLUMEHANDLING_MIDI);

	for(size_t i = sndFile.GetModSpecifications().noteMin; i < sndFile.GetModSpecifications().noteMax; i++)
	{
		VERIFY_EQUAL_NONCONT(pIns->Keyboard[i], (i == NOTE_MIDDLEC - 1) ? 2 : 1);
	}

	VERIFY_EQUAL_NONCONT(pIns->VolEnv.dwFlags, ENV_ENABLED | ENV_SUSTAIN);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nNodes, 3);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nReleaseNode, ENV_RELEASE_NODE_UNSET);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.Ticks[2], 96);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.Values[2], 0);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nSustainStart, 1);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nSustainEnd, 1);

	VERIFY_EQUAL_NONCONT(pIns->PanEnv.dwFlags, ENV_LOOP);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nNodes, 12);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopStart, 9);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopEnd, 11);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nReleaseNode, ENV_RELEASE_NODE_UNSET);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.Ticks[9], 46);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.Values[9], 23);

	VERIFY_EQUAL_NONCONT(pIns->PitchEnv.dwFlags, EnvelopeFlags(0));
	VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nNodes, 0);

	// Sequences
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetNumSequences(), 1);
	VERIFY_EQUAL_NONCONT(sndFile.Order[0], 0);
	VERIFY_EQUAL_NONCONT(sndFile.Order[1], 1);

	// Patterns
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.GetNumPatterns(), 2);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetName(), "First Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumChannels(), 2);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetRowsPerBeat(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetRowsPerMeasure(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(0), true);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetName(), "Second Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumRows(), 32);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumChannels(), 2);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerBeat(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerMeasure(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(1), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->IsPcNote(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->note, NOTE_NONE);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->instr, 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->volcmd, VOLCMD_VIBRATOSPEED);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->vol, 15);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 0)->IsEmpty(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->IsEmpty(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->IsPcNote(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->note, NOTE_MIDDLEC + 12);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->instr, 45);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->volcmd, VOLCMD_VOLSLIDEDOWN);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->vol, 5);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->command, CMD_PANNING8);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->param, 0xFF);

	// Test 4-Bit Panning conversion
	for(int i = 0; i < 16; i++)
	{
		VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(10 + i, 0)->vol, ((i * 64 + 8) / 15));
	}

	// Plugins
	const SNDMIXPLUGIN &plug = sndFile.m_MixPlugins[0];
	VERIFY_EQUAL_NONCONT(strcmp(plug.GetName(), "First Plugin"), 0);
	VERIFY_EQUAL_NONCONT(plug.fDryRatio, 0.26f);
	VERIFY_EQUAL_NONCONT(plug.IsMasterEffect(), true);
	VERIFY_EQUAL_NONCONT(plug.GetGain(), 11);
}


// Check if our test file was loaded correctly.
void TestLoadMPTMFile(const CSoundFile &sndFile)
//----------------------------------------------
{
#ifdef MODPLUG_TRACKER
	const CModDoc *pModDoc = sndFile.GetpModDoc();
#endif // MODPLUG_TRACKER

	// Global Variables
	VERIFY_EQUAL_NONCONT(sndFile.GetTitle(), "Test Module_____________X");
	VERIFY_EQUAL_NONCONT(sndFile.songMessage.at(0), 'O');
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultTempo, 139);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultSpeed, 5);
	VERIFY_EQUAL_NONCONT(sndFile.m_nGlobalVolume, 128);
	VERIFY_EQUAL_NONCONT(sndFile.m_nVSTiVolume, 42);
	VERIFY_EQUAL_NONCONT(sndFile.m_nSamplePreAmp, 23);
	VERIFY_EQUAL_NONCONT((sndFile.m_SongFlags & SONG_FILE_FLAGS), SONG_EMBEDMIDICFG | SONG_LINEARSLIDES | SONG_EXFILTERRANGE | SONG_ITCOMPATGXX | SONG_ITOLDEFFECTS);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_COMPATIBLE_PLAY), true);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_MIDICC_BUGEMULATION), false);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_OLDVOLSWING), false);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_OLD_MIDI_PITCHBENDS), false);
	VERIFY_EQUAL_NONCONT(sndFile.GetMixLevels(), mixLevels_compatible);
	VERIFY_EQUAL_NONCONT(sndFile.m_nTempoMode, tempo_mode_modern);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultRowsPerBeat, 6);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultRowsPerMeasure, 12);
	VERIFY_EQUAL_NONCONT(sndFile.m_dwCreatedWithVersion, MAKE_VERSION_NUMERIC(1, 19, 02, 05));
	VERIFY_EQUAL_NONCONT(sndFile.m_nRestartPos, 1);
	
#ifdef MODPLUG_TRACKER
	// Edit history
	VERIFY_EQUAL_NONCONT(pModDoc->GetFileHistory().size() > 0, true);
	const FileHistory &fh = pModDoc->GetFileHistory().at(0);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_year, 111);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_mon, 5);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_mday, 14);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_hour, 21);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_min, 8);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_sec, 32);
	VERIFY_EQUAL_NONCONT((uint32)((double)fh.openTime / HISTORY_TIMER_PRECISION), 31);
#endif // MODPLUG_TRACKER

	// Macros
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(0), sfx_reso);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(1), sfx_drywet);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(2), sfx_polyAT);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetFixedMacroType(), zxx_resomode);

	// Channels
	VERIFY_EQUAL_NONCONT(sndFile.GetNumChannels(), 70);
	VERIFY_EQUAL_NONCONT(strcmp(sndFile.ChnSettings[0].szName, "First Channel"), 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nPan, 32);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nVolume, 32);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].dwFlags, CHN_MUTE);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nMixPlugin, 0);

	VERIFY_EQUAL_NONCONT(strcmp(sndFile.ChnSettings[1].szName, "Second Channel"), 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nPan, 128);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nVolume, 16);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].dwFlags, CHN_SURROUND);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nMixPlugin, 1);

	VERIFY_EQUAL_NONCONT(strcmp(sndFile.ChnSettings[69].szName, "Last Channel______X"), 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[69].nPan, 256);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[69].nVolume, 7);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[69].dwFlags, 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[69].nMixPlugin, 1);
	// Samples
	VERIFY_EQUAL_NONCONT(sndFile.GetNumSamples(), 3);
	{
		const ModSample &sample = sndFile.GetSample(1);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 16);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_MPT), 9001);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 32 * 4);
		VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 16);
		VERIFY_EQUAL_NONCONT(sample.nPan, 160);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_PANNING | CHN_LOOP | CHN_SUSTAINLOOP | CHN_PINGPONGSUSTAIN);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 1);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 8);
		VERIFY_EQUAL_NONCONT(sample.nSustainStart, 1);
		VERIFY_EQUAL_NONCONT(sample.nSustainEnd, 7);

		VERIFY_EQUAL_NONCONT(sample.nVibType, VIB_SQUARE);
		VERIFY_EQUAL_NONCONT(sample.nVibSweep, 3);
		VERIFY_EQUAL_NONCONT(sample.nVibRate, 4);
		VERIFY_EQUAL_NONCONT(sample.nVibDepth, 5);

		// Sample Data
		const int8 *p8 = static_cast<const int8 *>(sample.pSample);
		for(size_t i = 0; i < 6; i++)
		{
			VERIFY_EQUAL_NONCONT(p8[i], 18);
		}
		for(size_t i = 6; i < 16; i++)
		{
			VERIFY_EQUAL_NONCONT(p8[i], 0);
		}
	}

	{
		const ModSample &sample = sndFile.GetSample(2);
		VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[2], "Stereo / 16-Bit"), 0);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 4);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 16 * 4);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_MPT), 16000);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_16BIT | CHN_STEREO | CHN_LOOP);
		
		// Sample Data (Stereo Interleaved)
		for(size_t i = 0; i < 7; i++)
		{
			VERIFY_EQUAL_NONCONT(static_cast<int16 *>(sample.pSample)[4 + i], int16(-32768));
		}
	}

	// Instruments
	VERIFY_EQUAL_NONCONT(sndFile.GetNumInstruments(), 2);
	for(INSTRUMENTINDEX ins = 1; ins <= 2; ins++)
	{
		const ModInstrument *pIns = sndFile.Instruments[ins];
		VERIFY_EQUAL_NONCONT(pIns->nGlobalVol, 32);
		VERIFY_EQUAL_NONCONT(pIns->nFadeOut, 1024);
		VERIFY_EQUAL_NONCONT(pIns->nPan, 64);
		VERIFY_EQUAL_NONCONT(pIns->dwFlags, INS_SETPANNING);

		VERIFY_EQUAL_NONCONT(pIns->nPPS, 16);
		VERIFY_EQUAL_NONCONT(pIns->nPPC, (NOTE_MIDDLEC - NOTE_MIN) + 6);	// F#5

		VERIFY_EQUAL_NONCONT(pIns->nVolRampUp, 1200);
		VERIFY_EQUAL_NONCONT(pIns->nResampling, SRCMODE_POLYPHASE);

		VERIFY_EQUAL_NONCONT(pIns->IsCutoffEnabled(), true);
		VERIFY_EQUAL_NONCONT(pIns->GetCutoff(), 0x32);
		VERIFY_EQUAL_NONCONT(pIns->IsResonanceEnabled(), true);
		VERIFY_EQUAL_NONCONT(pIns->GetResonance(), 0x64);
		VERIFY_EQUAL_NONCONT(pIns->nFilterMode, FLTMODE_HIGHPASS);

		VERIFY_EQUAL_NONCONT(pIns->nVolSwing, 0x30);
		VERIFY_EQUAL_NONCONT(pIns->nPanSwing, 0x18);
		VERIFY_EQUAL_NONCONT(pIns->nCutSwing, 0x0C);
		VERIFY_EQUAL_NONCONT(pIns->nResSwing, 0x3C);

		VERIFY_EQUAL_NONCONT(pIns->nNNA, NNA_CONTINUE);
		VERIFY_EQUAL_NONCONT(pIns->nDCT, DCT_NOTE);
		VERIFY_EQUAL_NONCONT(pIns->nDNA, DNA_NOTEFADE);

		VERIFY_EQUAL_NONCONT(pIns->nMixPlug, 1);
		VERIFY_EQUAL_NONCONT(pIns->nMidiChannel, 16);
		VERIFY_EQUAL_NONCONT(pIns->nMidiProgram, 64);
		VERIFY_EQUAL_NONCONT(pIns->wMidiBank, 2);
		VERIFY_EQUAL_NONCONT(pIns->midiPWD, ins);

		VERIFY_EQUAL_NONCONT(pIns->pTuning, sndFile.GetDefaultTuning());

		VERIFY_EQUAL_NONCONT(pIns->wPitchToTempoLock, 130);

		VERIFY_EQUAL_NONCONT(pIns->nPluginVelocityHandling, PLUGIN_VELOCITYHANDLING_VOLUME);
		VERIFY_EQUAL_NONCONT(pIns->nPluginVolumeHandling, PLUGIN_VOLUMEHANDLING_MIDI);

		for(size_t i = 0; i < NOTE_MAX; i++)
		{
			VERIFY_EQUAL_NONCONT(pIns->Keyboard[i], (i == NOTE_MIDDLEC - 1) ? 99 : 1);
			VERIFY_EQUAL_NONCONT(pIns->NoteMap[i], (i == NOTE_MIDDLEC - 1) ? (i + 13) : (i + 1));
		}

		VERIFY_EQUAL_NONCONT(pIns->VolEnv.dwFlags, ENV_ENABLED | ENV_CARRY);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.nNodes, 3);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.nReleaseNode, 1);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.Ticks[2], 96);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.Values[2], 0);

		VERIFY_EQUAL_NONCONT(pIns->PanEnv.dwFlags, ENV_LOOP);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nNodes, 76);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopStart, 22);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopEnd, 29);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nReleaseNode, ENV_RELEASE_NODE_UNSET);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.Ticks[75], 427);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.Values[75], 27);

		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.dwFlags, ENV_ENABLED | ENV_CARRY | ENV_SUSTAIN | ENV_FILTER);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nNodes, 3);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nSustainStart, 1);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nSustainEnd, 2);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.Ticks[1], 96);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.Values[1], 64);
	}
	// Sequences
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetNumSequences(), 2);

	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(0).GetLengthTailTrimmed(), 3);
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(0).m_sName, "First Sequence");
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(0)[0], sndFile.Order.GetIgnoreIndex());
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(0)[1], 0);
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(0)[2], sndFile.Order.GetIgnoreIndex());

	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(1).GetLengthTailTrimmed(), 2);
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(1).m_sName, "Second Sequence");
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(1)[0], 1);
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(1)[1], 2);

	// Patterns
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.GetNumPatterns(), 2);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetName(), "First Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumRows(), 70);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumChannels(), 70);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetOverrideSignature(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetRowsPerBeat(), 5);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetRowsPerMeasure(), 10);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(0), true);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetName(), "Second Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumRows(), 32);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumChannels(), 70);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerBeat(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerMeasure(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(1), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->IsPcNote(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->note, NOTE_PC);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->instr, 99);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->GetValueVolCol(), 1);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->GetValueEffectCol(), 200);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 0)->IsEmpty(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->IsEmpty(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->IsPcNote(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->note, NOTE_MIDDLEC + 12);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->instr, 45);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->volcmd, VOLCMD_VOLSLIDEDOWN);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->vol, 5);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->command, CMD_PANNING8);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->param, 0xFF);

	// Plugins
	const SNDMIXPLUGIN &plug = sndFile.m_MixPlugins[0];
	VERIFY_EQUAL_NONCONT(strcmp(plug.GetName(), "First Plugin"), 0);
	VERIFY_EQUAL_NONCONT(plug.fDryRatio, 0.26f);
	VERIFY_EQUAL_NONCONT(plug.IsMasterEffect(), true);
	VERIFY_EQUAL_NONCONT(plug.GetGain(), 11);

#ifdef MODPLUG_TRACKER
	// MIDI Mapping
	VERIFY_EQUAL_NONCONT(sndFile.GetMIDIMapper().GetCount(), 1);
	const CMIDIMappingDirective &mapping = sndFile.GetMIDIMapper().GetDirective(0);
	VERIFY_EQUAL_NONCONT(mapping.GetAllowPatternEdit(), true);
	VERIFY_EQUAL_NONCONT(mapping.GetCaptureMIDI(), false);
	VERIFY_EQUAL_NONCONT(mapping.IsActive(), true);
	VERIFY_EQUAL_NONCONT(mapping.GetAnyChannel(), false);
	VERIFY_EQUAL_NONCONT(mapping.GetChannel(), 5);
	VERIFY_EQUAL_NONCONT(mapping.GetPlugIndex(), 1);
	VERIFY_EQUAL_NONCONT(mapping.GetParamIndex(), 0);
	VERIFY_EQUAL_NONCONT(mapping.GetEvent(), MIDIEvents::evControllerChange);
	VERIFY_EQUAL_NONCONT(mapping.GetController(), MIDIEvents::MIDICC_ModulationWheel_Coarse);
#endif

}


// Check if our test file was loaded correctly.
void TestLoadS3MFile(const CSoundFile &sndFile, bool resaved)
//-----------------------------------------------------------
{

	// Global Variables
	VERIFY_EQUAL_NONCONT(sndFile.GetTitle(), "S3M_Test__________________X");
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultTempo, 33);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultSpeed, 254);
	VERIFY_EQUAL_NONCONT(sndFile.m_nGlobalVolume, 32 * 4);
	VERIFY_EQUAL_NONCONT(sndFile.m_nVSTiVolume, 48);
	VERIFY_EQUAL_NONCONT(sndFile.m_nSamplePreAmp, 16);
	VERIFY_EQUAL_NONCONT((sndFile.m_SongFlags & SONG_FILE_FLAGS), SONG_FASTVOLSLIDES);
	VERIFY_EQUAL_NONCONT(sndFile.GetMixLevels(), mixLevels_compatible);
	VERIFY_EQUAL_NONCONT(sndFile.m_nTempoMode, tempo_mode_classic);
	VERIFY_EQUAL_NONCONT(sndFile.m_dwLastSavedWithVersion, resaved ? (MptVersion::num & 0xFFFF0000) : MAKE_VERSION_NUMERIC(1, 20, 00, 00));
	VERIFY_EQUAL_NONCONT(sndFile.m_nRestartPos, 0);

	// Channels
	VERIFY_EQUAL_NONCONT(sndFile.GetNumChannels(), 4);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nPan, 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].dwFlags, ChannelFlags(0));

	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nPan, 256);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].dwFlags, CHN_MUTE);

	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[2].nPan, 85);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[2].dwFlags, ChannelFlags(0));

	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[3].nPan, 171);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[3].dwFlags, CHN_MUTE);

	// Samples
	VERIFY_EQUAL_NONCONT(sndFile.GetNumSamples(), 3);
	{
		const ModSample &sample = sndFile.GetSample(1);
		VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[1], "Sample_1__________________X"), 0);
		VERIFY_EQUAL_NONCONT(strcmp(sample.filename, "Filename_1_X"), 0);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 60);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 9001);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 32 * 4);
		VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 64);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_LOOP);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 16);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 60);

		// Sample Data
		const int8 *p8 = static_cast<const int8 *>(sample.pSample);
		for(size_t i = 0; i < 30; i++)
		{
			VERIFY_EQUAL_NONCONT(p8[i], 127);
		}
		for(size_t i = 31; i < 60; i++)
		{
			VERIFY_EQUAL_NONCONT(p8[i], -128);
		}
	}

	{
		const ModSample &sample = sndFile.GetSample(2);
		VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[2], "Empty"), 0);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 16384);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 2 * 4);
	}

	{
		const ModSample &sample = sndFile.GetSample(3);
		VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[3], "Stereo / 16-Bit"), 0);
		VERIFY_EQUAL_NONCONT(strcmp(sample.filename, "Filename_3_X"), 0);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 4);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 64);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 16000);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 0);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_LOOP | CHN_16BIT | CHN_STEREO);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 0);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 16);

		// Sample Data (Stereo Interleaved)
		for(size_t i = 0; i < 7; i++)
		{
			VERIFY_EQUAL_NONCONT(static_cast<int16 *>(sample.pSample)[4 + i], int16(-32768));
		}
	}

	// Orders
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetLengthTailTrimmed(), 5);
	VERIFY_EQUAL_NONCONT(sndFile.Order[0], 0);
	VERIFY_EQUAL_NONCONT(sndFile.Order[1], sndFile.Order.GetIgnoreIndex());
	VERIFY_EQUAL_NONCONT(sndFile.Order[2], sndFile.Order.GetInvalidPatIndex());
	VERIFY_EQUAL_NONCONT(sndFile.Order[3], 1);
	VERIFY_EQUAL_NONCONT(sndFile.Order[4], 0);

	// Patterns
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.GetNumPatterns(), 2);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumChannels(), 4);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 0)->note, NOTE_MIN + 12);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(1, 0)->note, NOTE_MIN + 107);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 1)->volcmd, VOLCMD_VOLUME);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 1)->vol, 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(1, 1)->volcmd, VOLCMD_VOLUME);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(1, 1)->vol, 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(2, 1)->volcmd, VOLCMD_PANNING);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(2, 1)->vol, 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(3, 1)->volcmd, VOLCMD_PANNING);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(3, 1)->vol, 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 3)->command, CMD_SPEED);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 3)->param, 0x11);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(1), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(63, 3)->param, 0x04);
}



#ifdef MODPLUG_TRACKER

static bool ShouldRunTests()
{
	CString theFile = theApp.GetAppDirPath();
	// Only run the tests when we're in the project directory structure.
	return theFile.Mid(theFile.GetLength() - 6, 5) == "Debug";
}

static std::string GetTestFilenameBase()
{
	CString theFile = theApp.GetAppDirPath();
	theFile.Delete(theFile.GetLength() - 6, 6);
	theFile.Append("../test/test.");
	return theFile.GetString();
}

typedef CModDoc *TSoundFileContainer;

static CSoundFile &GetrSoundFile(TSoundFileContainer &sndFile)
{
	return sndFile->GetrSoundFile();
}

static TSoundFileContainer CreateSoundFileContainer(const std::string filename)
{
	CModDoc *pModDoc = (CModDoc *)theApp.OpenDocumentFile(filename.c_str(), FALSE);
	return pModDoc;
}

static void DestroySoundFileContainer(TSoundFileContainer &sndFile)
{
	sndFile->OnCloseDocument();
}

static void SaveIT(const TSoundFileContainer &sndFile, const std::string &filename)
{
	sndFile->DoSave(filename.c_str());
	// Saving the file puts it in the MRU list...
	theApp.RemoveMruItem(0);
}

static void SaveXM(const TSoundFileContainer &sndFile, const std::string &filename)
{
	sndFile->DoSave(filename.c_str());
	// Saving the file puts it in the MRU list...
	theApp.RemoveMruItem(0);
}

static void SaveS3M(const TSoundFileContainer &sndFile, const std::string &filename)
{
	sndFile->DoSave(filename.c_str());
	// Saving the file puts it in the MRU list...
	theApp.RemoveMruItem(0);
}

#else

static bool ShouldRunTests()
{
	return true;
}

static std::string GetTestFilenameBase()
{
	return "../test/test.";
}

typedef std::shared_ptr<CSoundFile> TSoundFileContainer;

static CSoundFile &GetrSoundFile(TSoundFileContainer &sndFile)
{
	return *sndFile.get();
}

static TSoundFileContainer CreateSoundFileContainer(const std::string &filename)
{
	mpt::ifstream stream(filename, std::ios::binary);
	FileReader file(&stream);
	std::shared_ptr<CSoundFile> pSndFile(new CSoundFile());
	pSndFile->Create(file, CSoundFile::loadCompleteModule);
	return pSndFile;
}

static void DestroySoundFileContainer(TSoundFileContainer & /* sndFile */ )
{
	return;
}

#ifndef MODPLUG_NO_FILESAVE

static void SaveIT(const TSoundFileContainer &sndFile, const std::string &filename)
{
	sndFile->SaveIT(filename.c_str(), false);
}

static void SaveXM(const TSoundFileContainer &sndFile, const std::string &filename)
{
	sndFile->SaveXM(filename.c_str(), false);
}

static void SaveS3M(const TSoundFileContainer &sndFile, const std::string &filename)
{
	sndFile->SaveS3M(filename.c_str());
}

#endif

#endif



// Test file loading and saving
void TestLoadSaveFile()
//---------------------
{
	if(!ShouldRunTests())
	{
		return;
	}
	std::string filenameBase = GetTestFilenameBase();

	// Test MPTM file loading
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + "mptm");

		TestLoadMPTMFile(GetrSoundFile(sndFileContainer));

		#ifndef MODPLUG_NO_FILESAVE
			// Test file saving
			SaveIT(sndFileContainer, filenameBase + "saved.mptm");
		#endif

		DestroySoundFileContainer(sndFileContainer);
	}

	// Reload the saved file and test if everything is still working correctly.
	#ifndef MODPLUG_NO_FILESAVE
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + "saved.mptm");

		TestLoadMPTMFile(GetrSoundFile(sndFileContainer));
		
		DestroySoundFileContainer(sndFileContainer);
	}
	#endif

	// Test XM file loading
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + "xm");

		TestLoadXMFile(GetrSoundFile(sndFileContainer));

		// In OpenMPT 1.20 (up to revision 1459), there was a bug in the XM saver
		// that would create broken XMs if the sample map contained samples that
		// were only referenced below C-1 or above B-8 (such samples should not
		// be written). Let's insert a sample there and check if re-loading the
		// file still works.
		GetrSoundFile(sndFileContainer).m_nSamples++;
		GetrSoundFile(sndFileContainer).Instruments[1]->Keyboard[110] = GetrSoundFile(sndFileContainer).GetNumSamples();

		#ifndef MODPLUG_NO_FILESAVE
			// Test file saving
			SaveXM(sndFileContainer, filenameBase + "saved.xm");
		#endif

		DestroySoundFileContainer(sndFileContainer);
	}

	// Reload the saved file and test if everything is still working correctly.
	#ifndef MODPLUG_NO_FILESAVE
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + "saved.xm");

		TestLoadXMFile(GetrSoundFile(sndFileContainer));

		DestroySoundFileContainer(sndFileContainer);
	}
	#endif

	// Test S3M file loading
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + "s3m");
		
		TestLoadS3MFile(GetrSoundFile(sndFileContainer), false);

		#ifndef MODPLUG_NO_FILESAVE
			// Test file saving
			SaveS3M(sndFileContainer, filenameBase + "saved.s3m");
		#endif

		DestroySoundFileContainer(sndFileContainer);
	}

	// Reload the saved file and test if everything is still working correctly.
	#ifndef MODPLUG_NO_FILESAVE
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + "saved.s3m");

		TestLoadS3MFile(GetrSoundFile(sndFileContainer), true);

		DestroySoundFileContainer(sndFileContainer);
	}
	#endif
}


void RunITCompressionTest(const std::vector<int8> &sampleData, ChannelFlags smpFormat, bool it215, int testcount)
//---------------------------------------------------------------------------------------------------------------
{
	std::string filename = GetTestFilenameBase() + "itcomp" + Stringify(testcount) + ".raw";

	ModSample smp;
	smp.uFlags = smpFormat;
	smp.pSample = const_cast<int8 *>(&sampleData[0]);
	smp.nLength = sampleData.size() / smp.GetBytesPerSample();

	{
		FILE *f = fopen(filename.c_str(), "wb");
		ITCompression compression(smp, it215, f);
		fclose(f);
	}

	{
		FILE *f = fopen(filename.c_str(), "rb");
		fseek(f, 0, SEEK_END);
		std::vector<int8> fileData(ftell(f), 0);
		fseek(f, 0, SEEK_SET);
		VERIFY_EQUAL(fread(&fileData[0], fileData.size(), 1, f), 1);
		FileReader file(&fileData[0], fileData.size());

		std::vector<int8> sampleDataNew(sampleData.size(), 0);
		smp.pSample = &sampleDataNew[0];

		ITDecompression decompression(file, smp, it215);
		VERIFY_EQUAL_NONCONT(memcmp(&sampleData[0], &sampleDataNew[0], sampleData.size()), 0);
		fclose(f);
	}
	while(remove(filename.c_str()) == EACCES)
	{
		// wait for windows virus scanners
		#ifdef WIN32
			Sleep(1);
		#endif
	}
}


void TestITCompression()
//----------------------
{
	// Test loading / saving of IT-compressed samples
	const int sampleDataSize = 65536;
	std::vector<int8> sampleData(sampleDataSize, 0);
	std::srand(0);
	for(int i = 0; i < sampleDataSize; i++)
	{
		sampleData[i] = (int8)std::rand();
	}

	int testcount = 0;

	// Run each compression test with IT215 compression and without.
	for(int i = 0; i < 2; i++)
	{
		RunITCompressionTest(sampleData, ChannelFlags(0), i == 0, testcount++);
		RunITCompressionTest(sampleData, CHN_16BIT, i == 0, testcount++);
		RunITCompressionTest(sampleData, CHN_STEREO, i == 0, testcount++);
		RunITCompressionTest(sampleData, CHN_16BIT | CHN_STEREO, i == 0, testcount++);
	}
}


double Rand01() {return rand() / double(RAND_MAX);}

template <class T>
T Rand(const T& min, const T& max) {return Util::Round<T>(min + Rand01() * (max - min));}



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
			i->SetValueVolCol(Rand<uint16>(0, ModCommand::maxColumnValue));
			i->SetValueEffectCol(Rand<uint16>(0, ModCommand::maxColumnValue));
		}
		else
			i->Clear();
	}
}


// Test PC note serialization
void TestPCnoteSerialization()
//----------------------------
{
	FileReader file;
	MPT_SHARED_PTR<CSoundFile> pSndFile(new CSoundFile());
	CSoundFile &sndFile = *pSndFile.get();
	sndFile.ChangeModTypeTo(MOD_TYPE_MPT);
	sndFile.Patterns.DestroyPatterns();
	sndFile.m_nChannels = ModSpecs::mptm.channelsMax;

	sndFile.Patterns.Insert(0, ModSpecs::mptm.patternRowsMin);
	sndFile.Patterns.Insert(1, 64);
	GenerateCommands(sndFile.Patterns[1], 0.3, 0.3);
	sndFile.Patterns.Insert(2, ModSpecs::mptm.patternRowsMax);
	GenerateCommands(sndFile.Patterns[2], 0.5, 0.5);

	//
	std::vector<ModCommand> pat[3];
	const size_t numCommands[] = {	sndFile.GetNumChannels() * sndFile.Patterns[0].GetNumRows(),
									sndFile.GetNumChannels() * sndFile.Patterns[1].GetNumRows(),
									sndFile.GetNumChannels() * sndFile.Patterns[2].GetNumRows()
								 };
	pat[0].resize(numCommands[0]);
	pat[1].resize(numCommands[1]);
	pat[2].resize(numCommands[2]);

	for(size_t i = 0; i < 3; i++) // Copy pattern data for comparison.
	{
		CPattern::const_iterator iter = sndFile.Patterns[i].Begin();
		for(size_t j = 0; j < numCommands[i]; j++, iter++) pat[i][j] = *iter;
	}

	std::stringstream mem;
	WriteModPatterns(mem, sndFile.Patterns);

	VERIFY_EQUAL_NONCONT( mem.good(), true );

	// Clear patterns.
	sndFile.Patterns[0].ClearCommands();
	sndFile.Patterns[1].ClearCommands();
	sndFile.Patterns[2].ClearCommands();

	// Read data back.
	ReadModPatterns(mem, sndFile.Patterns);

	// Compare.
	VERIFY_EQUAL_NONCONT( sndFile.Patterns[0].GetNumRows(), ModSpecs::mptm.patternRowsMin);
	VERIFY_EQUAL_NONCONT( sndFile.Patterns[1].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT( sndFile.Patterns[2].GetNumRows(), ModSpecs::mptm.patternRowsMax);
	for(size_t i = 0; i < 3; i++)
	{
		bool bPatternDataMatch = true;
		CPattern::const_iterator iter = sndFile.Patterns[i].Begin();
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
}


// Test String I/O functionality
void TestStringIO()
//-----------------
{
	char src0[4] = { '\0', 'X', ' ', 'X' };		// Weird empty buffer
	char src1[4] = { 'X', ' ', '\0', 'X' };		// Weird buffer (hello Impulse Tracker)
	char src2[4] = { 'X', 'Y', 'Z', ' ' };		// Full buffer, last character space
	char src3[4] = { 'X', 'Y', 'Z', '!' };		// Full buffer, last character non-space
	char dst1[6];	// Destination buffer, larger than source buffer
	char dst2[3];	// Destination buffer, smaller than source buffer

#define ReadTest(mode, dst, src, expectedResult) \
	mpt::String::Read<mpt::String:: mode >(dst, src); \
	VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, CountOf(dst)), 0); /* Ensure that the strings are identical */ \
	for(size_t i = strlen(dst); i < CountOf(dst); i++) \
		VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */

#define WriteTest(mode, dst, src, expectedResult) \
	mpt::String::Write<mpt::String:: mode >(dst, src); \
	VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, CountOf(dst)), 0);  /* Ensure that the strings are identical */ \
	for(size_t i = mpt::strnlen(dst, CountOf(dst)); i < CountOf(dst); i++) \
		VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */

	// Check reading of null-terminated string into larger buffer
	ReadTest(nullTerminated, dst1, src0, "");
	ReadTest(nullTerminated, dst1, src1, "X ");
	ReadTest(nullTerminated, dst1, src2, "XYZ");
	ReadTest(nullTerminated, dst1, src3, "XYZ");

	// Check reading of string that should be null-terminated, but is maybe too long to still hold the null character.
	ReadTest(maybeNullTerminated, dst1, src0, "");
	ReadTest(maybeNullTerminated, dst1, src1, "X ");
	ReadTest(maybeNullTerminated, dst1, src2, "XYZ ");
	ReadTest(maybeNullTerminated, dst1, src3, "XYZ!");

	// Check reading of space-padded strings with ignored last character
	ReadTest(spacePaddedNull, dst1, src0, " X");
	ReadTest(spacePaddedNull, dst1, src1, "X");
	ReadTest(spacePaddedNull, dst1, src2, "XYZ");
	ReadTest(spacePaddedNull, dst1, src3, "XYZ");

	// Check reading of space-padded strings
	ReadTest(spacePadded, dst1, src0, " X X");
	ReadTest(spacePadded, dst1, src1, "X  X");
	ReadTest(spacePadded, dst1, src2, "XYZ");
	ReadTest(spacePadded, dst1, src3, "XYZ!");

	///////////////////////////////

	// Check reading of null-terminated string into smaller buffer
	ReadTest(nullTerminated, dst2, src0, "");
	ReadTest(nullTerminated, dst2, src1, "X ");
	ReadTest(nullTerminated, dst2, src2, "XY");
	ReadTest(nullTerminated, dst2, src3, "XY");

	// Check reading of string that should be null-terminated, but is maybe too long to still hold the null character.
	ReadTest(maybeNullTerminated, dst2, src0, "");
	ReadTest(maybeNullTerminated, dst2, src1, "X ");
	ReadTest(maybeNullTerminated, dst2, src2, "XY");
	ReadTest(maybeNullTerminated, dst2, src3, "XY");

	// Check reading of space-padded strings with ignored last character
	ReadTest(spacePaddedNull, dst2, src0, " X");
	ReadTest(spacePaddedNull, dst2, src1, "X");
	ReadTest(spacePaddedNull, dst2, src2, "XY");
	ReadTest(spacePaddedNull, dst2, src3, "XY");

	// Check reading of space-padded strings
	ReadTest(spacePadded, dst2, src0, " X");
	ReadTest(spacePadded, dst2, src1, "X");
	ReadTest(spacePadded, dst2, src2, "XY");
	ReadTest(spacePadded, dst2, src3, "XY");

	///////////////////////////////

	// Check writing of null-terminated string into larger buffer
	WriteTest(nullTerminated, dst1, src0, "");
	WriteTest(nullTerminated, dst1, src1, "X ");
	WriteTest(nullTerminated, dst1, src2, "XYZ ");
	WriteTest(nullTerminated, dst1, src3, "XYZ!");

	// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
	WriteTest(maybeNullTerminated, dst1, src0, "");
	WriteTest(maybeNullTerminated, dst1, src1, "X ");
	WriteTest(maybeNullTerminated, dst1, src2, "XYZ ");
	WriteTest(maybeNullTerminated, dst1, src3, "XYZ!");

	// Check writing of space-padded strings with last character set to null
	WriteTest(spacePaddedNull, dst1, src0, "     ");
	WriteTest(spacePaddedNull, dst1, src1, "X    ");
	WriteTest(spacePaddedNull, dst1, src2, "XYZ  ");
	WriteTest(spacePaddedNull, dst1, src3, "XYZ! ");

	// Check writing of space-padded strings
	WriteTest(spacePadded, dst1, src0, "      ");
	WriteTest(spacePadded, dst1, src1, "X     ");
	WriteTest(spacePadded, dst1, src2, "XYZ   ");
	WriteTest(spacePadded, dst1, src3, "XYZ!  ");

	///////////////////////////////

	// Check writing of null-terminated string into smaller buffer
	WriteTest(nullTerminated, dst2, src0, "");
	WriteTest(nullTerminated, dst2, src1, "X ");
	WriteTest(nullTerminated, dst2, src2, "XY");
	WriteTest(nullTerminated, dst2, src3, "XY");

	// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
	WriteTest(maybeNullTerminated, dst2, src0, "");
	WriteTest(maybeNullTerminated, dst2, src1, "X ");
	WriteTest(maybeNullTerminated, dst2, src2, "XYZ");
	WriteTest(maybeNullTerminated, dst2, src3, "XYZ");

	// Check writing of space-padded strings with last character set to null
	WriteTest(spacePaddedNull, dst2, src0, "  ");
	WriteTest(spacePaddedNull, dst2, src1, "X ");
	WriteTest(spacePaddedNull, dst2, src2, "XY");
	WriteTest(spacePaddedNull, dst2, src3, "XY");

	// Check writing of space-padded strings
	WriteTest(spacePadded, dst2, src0, "   ");
	WriteTest(spacePadded, dst2, src1, "X  ");
	WriteTest(spacePadded, dst2, src2, "XYZ");
	WriteTest(spacePadded, dst2, src3, "XYZ");

#undef ReadTest
#undef WriteTest

	{

		std::string dststring;
		std::string src0string = std::string(src0, CountOf(src0));
		std::string src1string = std::string(src1, CountOf(src1));
		std::string src2string = std::string(src2, CountOf(src2));
		std::string src3string = std::string(src3, CountOf(src3));

#define ReadTest(mode, dst, src, expectedResult) \
	mpt::String::Read<mpt::String:: mode >(dst, src); \
	VERIFY_EQUAL_NONCONT(dst, expectedResult); /* Ensure that the strings are identical */ \

#define WriteTest(mode, dst, src, expectedResult) \
	mpt::String::Write<mpt::String:: mode >(dst, src); \
	VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, CountOf(dst)), 0);  /* Ensure that the strings are identical */ \
	for(size_t i = mpt::strnlen(dst, CountOf(dst)); i < CountOf(dst); i++) \
		VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */

		// Check reading of null-terminated string into std::string
		ReadTest(nullTerminated, dststring, src0, "");
		ReadTest(nullTerminated, dststring, src1, "X ");
		ReadTest(nullTerminated, dststring, src2, "XYZ");
		ReadTest(nullTerminated, dststring, src3, "XYZ");

		// Check reading of string that should be null-terminated, but is maybe too long to still hold the null character.
		ReadTest(maybeNullTerminated, dststring, src0, "");
		ReadTest(maybeNullTerminated, dststring, src1, "X ");
		ReadTest(maybeNullTerminated, dststring, src2, "XYZ ");
		ReadTest(maybeNullTerminated, dststring, src3, "XYZ!");

		// Check reading of space-padded strings with ignored last character
		ReadTest(spacePaddedNull, dststring, src0, " X");
		ReadTest(spacePaddedNull, dststring, src1, "X");
		ReadTest(spacePaddedNull, dststring, src2, "XYZ");
		ReadTest(spacePaddedNull, dststring, src3, "XYZ");

		// Check reading of space-padded strings
		ReadTest(spacePadded, dststring, src0, " X X");
		ReadTest(spacePadded, dststring, src1, "X  X");
		ReadTest(spacePadded, dststring, src2, "XYZ");
		ReadTest(spacePadded, dststring, src3, "XYZ!");

		///////////////////////////////

		// Check writing of null-terminated string into larger buffer
		WriteTest(nullTerminated, dst1, src0string, "");
		WriteTest(nullTerminated, dst1, src1string, "X ");
		WriteTest(nullTerminated, dst1, src2string, "XYZ ");
		WriteTest(nullTerminated, dst1, src3string, "XYZ!");

		// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
		WriteTest(maybeNullTerminated, dst1, src0string, "");
		WriteTest(maybeNullTerminated, dst1, src1string, "X ");
		WriteTest(maybeNullTerminated, dst1, src2string, "XYZ ");
		WriteTest(maybeNullTerminated, dst1, src3string, "XYZ!");

		// Check writing of space-padded strings with last character set to null
		WriteTest(spacePaddedNull, dst1, src0string, "     ");
		WriteTest(spacePaddedNull, dst1, src1string, "X    ");
		WriteTest(spacePaddedNull, dst1, src2string, "XYZ  ");
		WriteTest(spacePaddedNull, dst1, src3string, "XYZ! ");

		// Check writing of space-padded strings
		WriteTest(spacePadded, dst1, src0string, "      ");
		WriteTest(spacePadded, dst1, src1string, "X     ");
		WriteTest(spacePadded, dst1, src2string, "XYZ   ");
		WriteTest(spacePadded, dst1, src3string, "XYZ!  ");

		///////////////////////////////

		// Check writing of null-terminated string into smaller buffer
		WriteTest(nullTerminated, dst2, src0string, "");
		WriteTest(nullTerminated, dst2, src1string, "X ");
		WriteTest(nullTerminated, dst2, src2string, "XY");
		WriteTest(nullTerminated, dst2, src3string, "XY");

		// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
		WriteTest(maybeNullTerminated, dst2, src0string, "");
		WriteTest(maybeNullTerminated, dst2, src1string, "X ");
		WriteTest(maybeNullTerminated, dst2, src2string, "XYZ");
		WriteTest(maybeNullTerminated, dst2, src3string, "XYZ");

		// Check writing of space-padded strings with last character set to null
		WriteTest(spacePaddedNull, dst2, src0string, "  ");
		WriteTest(spacePaddedNull, dst2, src1string, "X ");
		WriteTest(spacePaddedNull, dst2, src2string, "XY");
		WriteTest(spacePaddedNull, dst2, src3string, "XY");

		// Check writing of space-padded strings
		WriteTest(spacePadded, dst2, src0string, "   ");
		WriteTest(spacePadded, dst2, src1string, "X  ");
		WriteTest(spacePadded, dst2, src2string, "XYZ");
		WriteTest(spacePadded, dst2, src3string, "XYZ");

		///////////////////////////////

#undef ReadTest
#undef WriteTest

	}

	// Test FixNullString()
	mpt::String::FixNullString(src1);
	mpt::String::FixNullString(src2);
	mpt::String::FixNullString(src3);
	VERIFY_EQUAL_NONCONT(strncmp(src1, "X ", CountOf(src1)), 0);
	VERIFY_EQUAL_NONCONT(strncmp(src2, "XYZ", CountOf(src2)), 0);
	VERIFY_EQUAL_NONCONT(strncmp(src3, "XYZ", CountOf(src3)), 0);

}


void TestSampleConversion()
//-------------------------
{
	uint8 *sourceBuf = new uint8[65536 * 4];
	void *targetBuf = new uint8[65536 * 6];


	// Signed 8-Bit Integer PCM
	// Unsigned 8-Bit Integer PCM
	// Delta 8-Bit Integer PCM
	{
		uint8 *source8 = sourceBuf;
		for(size_t i = 0; i < 256; i++)
		{
			source8[i] = static_cast<uint8>(i);
		}

		int8 *signed8 = static_cast<int8 *>(targetBuf);
		uint8 *unsigned8 = static_cast<uint8 *>(targetBuf) + 256;
		int8 *delta8 = static_cast<int8 *>(targetBuf) + 512;
		int8 delta = 0;
		CopySample<SC::DecodeInt8>(signed8, 256, 1, reinterpret_cast<const char *>(source8), 256, 1);
		CopySample<SC::DecodeUint8>(reinterpret_cast<int8 *>(unsigned8), 256, 1, reinterpret_cast<const char *>(source8), 256, 1);
		CopySample<SC::DecodeInt8Delta>(delta8, 256, 1, reinterpret_cast<const char *>(source8), 256, 1);

		for(size_t i = 0; i < 256; i++)
		{
			delta += static_cast<int8>(i);
			VERIFY_EQUAL_QUIET_NONCONT(signed8[i], static_cast<int8>(i));
			VERIFY_EQUAL_QUIET_NONCONT(unsigned8[i], static_cast<uint8>(i + 0x80u));
			VERIFY_EQUAL_QUIET_NONCONT(delta8[i], static_cast<int8>(delta));
		}
	}

	// Signed 16-Bit Integer PCM
	// Unsigned 16-Bit Integer PCM
	// Delta 16-Bit Integer PCM
	{
		// Little Endian

		uint8 *source16 = sourceBuf;
		for(size_t i = 0; i < 65536; i++)
		{
			source16[i * 2 + 0] = static_cast<uint8>(i & 0xFF);
			source16[i * 2 + 1] = static_cast<uint8>(i >> 8);
		}

		int16 *signed16 = static_cast<int16 *>(targetBuf);
		uint16 *unsigned16 = static_cast<uint16 *>(targetBuf) + 65536;
		int16 *delta16 = static_cast<int16 *>(targetBuf) + 65536 * 2;
		int16 delta = 0;
		CopySample<SC::DecodeInt16<0, littleEndian16> >(signed16, 65536, 1, reinterpret_cast<const char *>(source16), 65536 * 2, 1);
		CopySample<SC::DecodeInt16<0x8000u, littleEndian16> >(reinterpret_cast<int16*>(unsigned16), 65536, 1, reinterpret_cast<const char *>(source16), 65536 * 2, 1);
		CopySample<SC::DecodeInt16Delta<littleEndian16> >(delta16, 65536, 1, reinterpret_cast<const char *>(source16), 65536 * 2, 1);

		for(size_t i = 0; i < 65536; i++)
		{
			delta += static_cast<int16>(i);
			VERIFY_EQUAL_QUIET_NONCONT(signed16[i], static_cast<int16>(i));
			VERIFY_EQUAL_QUIET_NONCONT(unsigned16[i], static_cast<uint16>(i + 0x8000u));
			VERIFY_EQUAL_QUIET_NONCONT(delta16[i], static_cast<int16>(delta));
		}

		// Big Endian

		for(size_t i = 0; i < 65536; i++)
		{
			source16[i * 2 + 0] = static_cast<uint8>(i >> 8);
			source16[i * 2 + 1] = static_cast<uint8>(i & 0xFF);
		}

		CopySample<SC::DecodeInt16<0, bigEndian16> >(signed16, 65536, 1, reinterpret_cast<const char *>(source16), 65536 * 2, 1);
		CopySample<SC::DecodeInt16<0x8000u, bigEndian16> >(reinterpret_cast<int16*>(unsigned16), 65536, 1, reinterpret_cast<const char *>(source16), 65536 * 2, 1);
		CopySample<SC::DecodeInt16Delta<bigEndian16> >(delta16, 65536, 1, reinterpret_cast<const char *>(source16), 65536 * 2, 1);

		delta = 0;
		for(size_t i = 0; i < 65536; i++)
		{
			delta += static_cast<int16>(i);
			VERIFY_EQUAL_QUIET_NONCONT(signed16[i], static_cast<int16>(i));
			VERIFY_EQUAL_QUIET_NONCONT(unsigned16[i], static_cast<uint16>(i + 0x8000u));
			VERIFY_EQUAL_QUIET_NONCONT(delta16[i], static_cast<int16>(delta));
		}

	}

	// Signed 24-Bit Integer PCM
	{
		uint8 *source24 = sourceBuf;
		for(size_t i = 0; i < 65536; i++)
		{
			source24[i * 3 + 0] = 0;
			source24[i * 3 + 1] = static_cast<uint8>(i & 0xFF);
			source24[i * 3 + 2] = static_cast<uint8>(i >> 8);
		}

		int16 *truncated16 = static_cast<int16 *>(targetBuf);
		ModSample sample;
		sample.Initialize();
		sample.nLength = 65536;
		sample.uFlags.set(CHN_16BIT);
		sample.pSample = (static_cast<int16 *>(targetBuf) + 65536);
		CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(sample, reinterpret_cast<const char *>(source24), 3*65536);
		CopySample<SC::ConversionChain<SC::ConvertShift<int16, int32, 16>, SC::DecodeInt24<0, littleEndian24> > >(truncated16, 65536, 1, reinterpret_cast<const char *>(source24), 65536 * 3, 1);

		for(size_t i = 0; i < 65536; i++)
		{
			VERIFY_EQUAL_QUIET_NONCONT(static_cast<const int16 *>(sample.pSample)[i], static_cast<int16>(i));
			VERIFY_EQUAL_QUIET_NONCONT(truncated16[i], static_cast<int16>(i));
		}
	}

	// Float 32-Bit
	{
		uint8 *source32 = sourceBuf;
		for(size_t i = 0; i < 65536; i++)
		{
			uint8_4 floatbits = EncodeFloatBE((static_cast<float>(i) / 65536.0f) - 0.5f);
			source32[i * 4 + 0] = floatbits.x[0];
			source32[i * 4 + 1] = floatbits.x[1];
			source32[i * 4 + 2] = floatbits.x[2];
			source32[i * 4 + 3] = floatbits.x[3];
		}

		int16 *truncated16 = static_cast<int16 *>(targetBuf);
		ModSample sample;
		sample.Initialize();
		sample.nLength = 65536;
		sample.uFlags.set(CHN_16BIT);
		sample.pSample = static_cast<int16 *>(targetBuf) + 65536;
		CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, float32>, SC::DecodeFloat32<bigEndian32> > >(sample, reinterpret_cast<const char *>(source32), 4*65536);
		CopySample<SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeFloat32<bigEndian32> > >(truncated16, 65536, 1, reinterpret_cast<const char *>(source32), 65536 * 4, 1);

		for(size_t i = 0; i < 65536; i++)
		{
			VERIFY_EQUAL_QUIET_NONCONT(static_cast<const int16 *>(sample.pSample)[i], static_cast<int16>(i - 0x8000u));
			if(abs(truncated16[i] - static_cast<int16>((i - 0x8000u) / 2)) > 1)
			{
				VERIFY_EQUAL_QUIET_NONCONT(true, false);
			}
		}
	}

	// Range checks
	{
		int8 oneSample = 1;
		char *signed8 = reinterpret_cast<char *>(targetBuf);
		memset(signed8, 0, 4);
		CopySample<SC::DecodeInt8>(reinterpret_cast<int8*>(targetBuf), 4, 1, reinterpret_cast<const char*>(&oneSample), sizeof(oneSample), 1);
		VERIFY_EQUAL_NONCONT(signed8[0], 1);
		VERIFY_EQUAL_NONCONT(signed8[1], 0);
		VERIFY_EQUAL_NONCONT(signed8[2], 0);
		VERIFY_EQUAL_NONCONT(signed8[3], 0);
	}

	delete[] sourceBuf;
	delete[] static_cast<uint8*>(targetBuf);
}


} // namespace MptTest

#else //Case: ENABLE_TESTS is not defined.

namespace MptTest
{
	void DoTests() {}
}

#endif

