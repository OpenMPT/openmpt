/*
 * mptBufferIO.h
 * -------------
 * Purpose: Stale, can be removed
 * Notes  : You should only ever use these wrappers instead of plain std::stringstream classes.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#include <ios>
#include <istream>
#include <ostream>
#include <sstream>
#include <streambuf>

OPENMPT_NAMESPACE_BEGIN


namespace mpt
{

typedef std::stringbuf stringbuf;
typedef std::istringstream istringstream;
typedef std::ostringstream ostringstream;
typedef std::stringstream stringstream;

} // namespace mpt


OPENMPT_NAMESPACE_END

