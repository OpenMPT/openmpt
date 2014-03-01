
#pragma once


#ifdef ENABLE_TESTS
#ifdef MODPLUG_TRACKER


namespace MptTest
{


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


} // namespace MptTest


#endif // MODPLUG_TRACKER
#endif // ENABLE_TESTS

