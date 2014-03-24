/*
 * TestToolsLib.cpp
 * ----------------
 * Purpose: Unit test framework for libopenmpt.
 * Notes  : Currently somewhat unreadable :/
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "TestToolsLib.h"


#ifdef ENABLE_TESTS
#ifndef MODPLUG_TRACKER


#include <exception>
#include <iostream>


namespace mpt { namespace Test {


int fail_count = 0;


static std::string remove_newlines(std::string str)
{
	return mpt::String::Replace(mpt::String::Replace(str, "\n", " "), "\r", " ");
}


Context::Context(const char * file, int line)
	: file(file)
	, line(line)
{
	return;
}


Context::Context(const Context &c)
	: file(c.file)
	, line(c.line)
{
	return;
}


Test::Test(Fatality fatality, Verbosity verbosity, const char * const desc, const Context &context)
	: fatality(fatality)
	, verbosity(verbosity)
	, desc(desc)
	, context(context)
{
	return;
}


std::string Test::AsString() const
{
	return mpt::String::Print("Test: %1(%2): %3", context.file, context.line, remove_newlines(desc));
}


void Test::ShowStart() const
{
	switch(verbosity)
	{
		case VerbosityQuiet:
			break;
		case VerbosityNormal:
		case VerbosityVerbose:
			std::cout << AsString() << ": " << std::endl;
			break;
	}
}


void Test::ShowProgress(const char * text) const
{
	switch(verbosity)
	{
		case VerbosityQuiet:
			break;
		case VerbosityNormal:
		case VerbosityVerbose:
			std::cout << AsString() << ": " << text << std::endl;
			break;
	}
}


void Test::ShowPass() const
{
	switch(verbosity)
	{
		case VerbosityQuiet:
			break;
		case VerbosityNormal:
		case VerbosityVerbose:
			std::cout << AsString() << ": PASS" << std::endl;
			break;
	}
}


void Test::ShowFail(bool exception, const char * const text) const
{
	std::cout << AsString() << ": FAIL" << std::endl;
	std::cout.flush();
	if(!exception)
	{
		if(!text || (text && std::string(text).empty()))
		{
			std::cerr << "FAIL: " << AsString() << std::endl;
		} else
		{
			std::cerr << "FAIL: " << AsString() << " : " << text << std::endl;
		}
	} else
	{
		if(!text || (text && std::string(text).empty()))
		{
			std::cerr << "FAIL: " << AsString() << " EXCEPTION!" << std::endl;
		} else
		{
			std::cerr << "FAIL: " << AsString() << " EXCEPTION: " << text << std::endl;
		}
	}
	std::cerr.flush();
}


void Test::ReportPassed()
{
	ShowPass();
}


void Test::ReportFailed()
{
	fail_count++;
	ReportException();
}


void Test::ReportException()
{
	try
	{
		throw; // get the exception
	} catch(TestFailed & e)
	{
		ShowFail(false, e.values.c_str());
		if(fatality == FatalityStop)
		{
			throw; // rethrow
		}
	} catch(std::exception & e)
	{
		ShowFail(true, e.what());
		throw; // rethrow
	} catch(...)
	{
		ShowFail(true);
		throw; // rethrow
	}
}


} } // namespace mpt::Test


#endif // !MODPLUG_TRACKER
#endif // ENABLE_TESTS
