/*
 * test.h
 * ------
 * Purpose: Unit tests for OpenMPT.
 * Notes  : We need FAAAAAAAR more unit tests!
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifdef _DEBUG
	#define ENABLE_TESTS
#endif

namespace MptTest
{
	void DoTests();
};
