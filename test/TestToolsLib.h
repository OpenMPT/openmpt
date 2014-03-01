
#pragma once


#ifdef ENABLE_TESTS
#ifndef MODPLUG_TRACKER


#include <memory>
#include <iostream>


namespace MptTest
{


static std::string remove_newlines(std::string str)
{
	return mpt::String::Replace(mpt::String::Replace(str, "\n", " "), "\r", " ");
}

static noinline void show_start(const char * const file, const int line, const char * const description)
{
	std::cout << "Test: " << file << "(" << line << "): " << remove_newlines(description) << ": ";
}

static noinline void MultiTestStart(const char * const file, const int line, const char * const description)
{
	show_start(file, line, description);
	std::cout << "..." << std::endl;
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
	std::cout.flush();
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
	std::cerr.flush();
}

static int fail_count = 0;

static noinline void CheckFailCountOrThrow()
{
	if(fail_count > 0)
	{
		throw std::runtime_error("Test failed.");
	}
}

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

static noinline void TestFail(const char * const file, const int line, const char * const description)
{
	show_fail(file, line, description);
	fail_count++;
}

static noinline void TestFailStop(const char * const file, const int line, const char * const description)
{
	show_fail(file, line, description);
	fail_count++;
	throw std::runtime_error(std::string("Test failed: ") + description);
}

#define MULTI_TEST_TRY   try { \
                          fail_count = 0;
#define MULTI_TEST_START MultiTestStart(THIS_FILE, __LINE__, description);
#define MULTI_TEST_END   show_start(THIS_FILE, __LINE__, description);
#define MULTI_TEST_CATCH  CheckFailCountOrThrow(); \
                          show_ok(THIS_FILE, __LINE__, description); \
                         } catch(...) { \
                          ReportException(THIS_FILE, __LINE__, description); \
                         }
#define TEST_TRY         try {
#define TEST_CATCH       } catch(...) { \
                          fail_count++; \
                          ReportException(file, line, description); \
                          throw; \
                         }
#define TEST_START()     show_start(file, line, description)
#define TEST_OK()        show_ok(file, line, description)
#define TEST_FAIL()      TestFail(file, line, description)
#define TEST_FAIL_STOP() TestFailStop(file, line, description)


template <typename Tx, typename Ty>
noinline void VerifyEqualImpl(const Tx &x, const Ty &y, const char *const description, const char *const file, const int line)
{
	TEST_TRY
	TEST_START();
	if(x != y)
	{
		TEST_FAIL();
	} else
	{
		TEST_OK();
	}
	TEST_CATCH
}


template <typename Tx, typename Ty>
noinline void VerifyEqualNonContImpl(const Tx &x, const Ty &y, const char *const description, const char *const file, const int line)
{
	TEST_TRY
	TEST_START();
	if(x != y)
	{
		TEST_FAIL_STOP();
	} else
	{
		TEST_OK();
	}
	TEST_CATCH
}


template <typename Tx, typename Ty>
noinline void VerifyEqualQuietNonContImpl(const Tx &x, const Ty &y, const char *const description, const char *const file, const int line)
{
	TEST_TRY
	if(x != y)
	{
		TEST_FAIL_STOP();
	}
	TEST_CATCH
}


//Verify that given parameters are 'equal'(show error message if not).
//The exact meaning of equality is not specified; for now using operator!=.
//The macro is active in both 'debug' and 'release' build.
#define VERIFY_EQUAL(x,y)	VerifyEqualImpl( (x) , (y) , #x " == " #y , THIS_FILE, __LINE__)


// Like VERIFY_EQUAL, but throws exception if comparison fails.
#define VERIFY_EQUAL_NONCONT(x,y)	VerifyEqualNonContImpl( (x) , (y) , #x " == " #y , THIS_FILE, __LINE__)


// Like VERIFY_EQUAL_NONCONT, but do not show message if test succeeds
#define VERIFY_EQUAL_QUIET_NONCONT(x,y)	VerifyEqualQuietNonContImpl( (x) , (y) , #x " == " #y , THIS_FILE, __LINE__)


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


#endif // !MODPLUG_TRACKER
#endif // ENABLE_TESTS
