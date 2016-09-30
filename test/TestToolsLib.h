/*
 * TestToolsLib.h
 * --------------
 * Purpose: Unit test framework for libopenmpt.
 * Notes  : This is more complex than the OpenMPT version because we cannot
 *          rely on a debugger and have to deal with exceptions ourselves.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#ifdef ENABLE_TESTS
#ifndef MODPLUG_TRACKER


#include "../common/FlagSet.h"


OPENMPT_NAMESPACE_BEGIN


namespace Test {


extern int fail_count;


enum Verbosity
{
	VerbosityQuiet,
	VerbosityNormal,
	VerbosityVerbose,
};

enum Fatality
{
	FatalityContinue,
	FatalityStop
};


struct Context
{
public:
	const char * const file;
	const int line;
public:
	Context(const char * file, int line);
	Context(const Context &c);
};

#define MPT_TEST_CONTEXT_CURRENT() (Test::Context( __FILE__ , __LINE__ ))


struct TestFailed
{
	std::string values;
	TestFailed(const std::string &values) : values(values) { }
	TestFailed() { }
};

} // namespace Test

template<typename T>
struct ToStringHelper
{
	std::string operator () (const T &x)
	{
		return mpt::ToString(x);
	}
};

template<typename enum_t, typename store_t>
struct ToStringHelper<FlagSet<enum_t, store_t> >
{
	std::string operator () (const FlagSet<enum_t, store_t> &x)
	{
		return mpt::ToString(x.GetRaw());
	}
};

namespace Test {

// We do not generally have type_traits from C++03-TR1
// and std::numeric_limits does not provide a is_integer which is useable as template argument.
template <typename T> struct is_integer : public std::false_type { };
template <> struct is_integer<signed short>     : public std::true_type { };
template <> struct is_integer<signed int>       : public std::true_type { };
template <> struct is_integer<signed long>      : public std::true_type { };
template <> struct is_integer<signed long long> : public std::true_type { };
template <> struct is_integer<unsigned short>     : public std::true_type { };
template <> struct is_integer<unsigned int>       : public std::true_type { };
template <> struct is_integer<unsigned long>      : public std::true_type { };
template <> struct is_integer<unsigned long long> : public std::true_type { };

class Testcase
{

private:

	Fatality const fatality;
	Verbosity const verbosity;
	const char * const desc;
	Context const context;

public:

	Testcase(Fatality fatality, Verbosity verbosity, const char * const desc, const Context &context);

public:

	std::string AsString() const;

	void ShowStart() const;
	void ShowProgress(const char * text) const;
	void ShowPass() const;
	void ShowFail(bool exception = false, const char * const text = nullptr) const;

	void ReportPassed();
	void ReportFailed();

	void ReportException();

private:

	template <typename Tx, typename Ty>
	inline bool IsEqual(const Tx &x, const Ty &y, std::false_type, std::false_type)
	{
		return (x == y);
	}

	template <typename Tx, typename Ty>
	inline bool IsEqual(const Tx &x, const Ty &y, std::false_type, std::true_type)
	{
		return (x == y);
	}

	template <typename Tx, typename Ty>
	inline bool IsEqual(const Tx &x, const Ty &y, std::true_type, std::false_type)
	{
		return (x == y);
	}

	template <typename Tx, typename Ty>
	inline bool IsEqual(const Tx &x, const Ty &y, std::true_type /* is_integer */, std::true_type /* is_integer */ )
	{
		// Avoid signed-unsigned-comparison warnings and test equivalence in case of either type conversion direction.
		return ((x == static_cast<Tx>(y)) && (static_cast<Ty>(x) == y));
	}

	template <typename Tx, typename Ty, typename Teps>
	inline bool IsEqualEpsilon(const Tx &x, const Ty &y, const Teps &eps)
	{
		return mpt::abs(x - y) <= eps;
	}

public:

#if 0 // C++11 version

private:

	template <typename Tx, typename Ty>
	MPT_NOINLINE void TypeCompareHelper(const Tx &x, const Ty &y)
	{
		if(!IsEqual(x, y, is_integer<Tx>(), is_integer<Ty>()))
		{
			throw TestFailed(mpt::String::Print("%1 != %2", ToStringHelper<Tx>()(x), ToStringHelper<Ty>()(y)));
			//throw TestFailed();
		}
	}

	template <typename Tx, typename Ty>
	MPT_NOINLINE void TypeCompareHelper(const Tx &x, const Ty &y, const Tesp &eps)
	{
		if(!IsEqualEpsilon(x, y, eps))
		{
			throw TestFailed(mpt::String::Print("%1 != %2", ToStringHelper<Tx>()(x), ToStringHelper<Ty>()(y)));
			//throw TestFailed();
		}
	}

public:

	template <typename Tfx, typename Tfy>
	MPT_NOINLINE void operator () (const Tfx &fx, const Tfy &fy)
	{
		ShowStart();
		try
		{
			ShowProgress("Calculate x ...");
			const auto x = fx();
			ShowProgress("Calculate y ...");
			const auto y = fy();
			ShowProgress("Compare ...");
			TypeCompareHelper(x, y);
			ReportPassed();
		} catch(...)
		{
			ReportFailed();
		}
	}

	template <typename Tfx, typename Tfy, typename Teps>
	MPT_NOINLINE void operator () (const Tfx &fx, const Tfy &fy, const Teps &eps)
	{
		ShowStart();
		try
		{
			ShowProgress("Calculate x ...");
			const auto x = fx();
			ShowProgress("Calculate y ...");
			const auto y = fy();
			ShowProgress("Compare ...");
			TypeCompareHelper(x, y, eps);
			ReportPassed();
		} catch(...)
		{
			ReportFailed();
		}
	}

	#define VERIFY_EQUAL(x,y)               Test::Testcase(Test::FatalityContinue, Test::VerbosityNormal, #x " == " #y , MPT_TEST_CONTEXT_CURRENT() )( [&](){return (x) ;}, [&](){return (y) ;} )
	#define VERIFY_EQUAL_NONCONT(x,y)       Test::Testcase(Test::FatalityStop    , Test::VerbosityNormal, #x " == " #y , MPT_TEST_CONTEXT_CURRENT() )( [&](){return (x) ;}, [&](){return (y) ;} )
	#define VERIFY_EQUAL_QUIET_NONCONT(x,y) Test::Testcase(Test::FatalityStop    , Test::VerbosityQuiet , #x " == " #y , MPT_TEST_CONTEXT_CURRENT() )( [&](){return (x) ;}, [&](){return (y) ;} )

	#define VERIFY_EQUAL_EPS(x,y)               Test::Testcase(Test::FatalityContinue, Test::VerbosityNormal, #x " == " #y , MPT_TEST_CONTEXT_CURRENT() )( [&](){return (x) ;}, [&](){return (y) ;}, (eps) )

#else

public:

	template <typename Tx, typename Ty>
	MPT_NOINLINE void operator () (const Tx &x, const Ty &y)
	{
		ShowStart();
		try
		{
			if(!IsEqual(x, y, is_integer<Tx>(), is_integer<Ty>()))
			{
				//throw TestFailed(mpt::String::Print("%1 != %2", x, y));
				throw TestFailed();
			}
			ReportPassed();
		} catch(...)
		{
			ReportFailed();
		}
	}

	template <typename Tx, typename Ty, typename Teps>
	MPT_NOINLINE void operator () (const Tx &x, const Ty &y, const Teps &eps)
	{
		ShowStart();
		try
		{
			if(!IsEqualEpsilon(x, y, eps))
			{
				//throw TestFailed(mpt::String::Print("%1 != %2", x, y));
				throw TestFailed();
			}
			ReportPassed();
		} catch(...)
		{
			ReportFailed();
		}
	}

	#define VERIFY_EQUAL(x,y)               Test::Testcase(Test::FatalityContinue, Test::VerbosityNormal, #x " == " #y , MPT_TEST_CONTEXT_CURRENT() )( (x) , (y) )
	#define VERIFY_EQUAL_NONCONT(x,y)       Test::Testcase(Test::FatalityStop    , Test::VerbosityNormal, #x " == " #y , MPT_TEST_CONTEXT_CURRENT() )( (x) , (y) )
	#define VERIFY_EQUAL_QUIET_NONCONT(x,y) Test::Testcase(Test::FatalityStop    , Test::VerbosityQuiet , #x " == " #y , MPT_TEST_CONTEXT_CURRENT() )( (x) , (y) )

	#define VERIFY_EQUAL_EPS(x,y,eps)       Test::Testcase(Test::FatalityContinue, Test::VerbosityNormal, #x " == " #y , MPT_TEST_CONTEXT_CURRENT() )( (x) , (y), (eps) )

#endif

};


#define DO_TEST(func) \
MPT_DO { \
	Test::Testcase test(Test::FatalityStop, Test::VerbosityVerbose, #func , MPT_TEST_CONTEXT_CURRENT() ); \
	try { \
		test.ShowStart(); \
		fail_count = 0; \
		func(); \
		if(fail_count > 0) { \
			throw Test::TestFailed(); \
		} \
		test.ReportPassed(); \
	} catch(...) { \
		test.ReportException(); \
	} \
} MPT_WHILE_0


} // namespace Test


OPENMPT_NAMESPACE_END


#endif // !MODPLUG_TRACKER
#endif // ENABLE_TESTS
