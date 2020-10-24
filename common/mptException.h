/*
 * mptException.h
 * --------------
 * Purpose: Exception abstraction, in particular for bad_alloc.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"



#include "mptBaseMacros.h"

#include <exception>
#if !defined(MPT_WITH_MFC)
#include <new>
#endif // !MPT_WITH_MFC

#if defined(MPT_WITH_MFC)
// cppcheck-suppress missingInclude
#include <afx.h>
#endif // MPT_WITH_MFC



OPENMPT_NAMESPACE_BEGIN



// Exception handling helpers, because MFC requires explicit deletion of the exception object,
// Thus, always call exactly one of mpt::rethrow_out_of_memory(e) or mpt::delete_out_of_memory(e).

namespace mpt
{

#if defined(MPT_WITH_MFC)

using out_of_memory = CMemoryException *;

[[noreturn]] inline void throw_out_of_memory()
{
	AfxThrowMemoryException();
}

[[noreturn]] inline void rethrow_out_of_memory(out_of_memory e)
{
	MPT_UNREFERENCED_PARAMETER(e);
	throw;
}

inline void delete_out_of_memory(out_of_memory & e)
{
	if(e)
	{
		e->Delete();
		e = nullptr;
	}
}

#else // !MPT_WITH_MFC

using out_of_memory = const std::bad_alloc &;

[[noreturn]] inline void throw_out_of_memory()
{
	throw std::bad_alloc();
}

[[noreturn]] inline void rethrow_out_of_memory(out_of_memory e)
{
	MPT_UNREFERENCED_PARAMETER(e);
	throw;
}

inline void delete_out_of_memory(out_of_memory e)
{
	MPT_UNREFERENCED_PARAMETER(e);
}

#endif // MPT_WITH_MFC

} // namespace mpt



OPENMPT_NAMESPACE_END
