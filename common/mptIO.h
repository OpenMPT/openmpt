/*
 * mptIO.h
 * -------
 * Purpose: Basic functions for reading/writing binary and endian safe data to/from files/streams.
 * Notes  : Some useful functions for reading and writing are still missing.
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/utility.hpp"
#include "mpt/io/base.hpp"
#include "mpt/io/io.hpp"
#include "mpt/io/io_span.hpp"
#include "mpt/io/io_virtual_wrapper.hpp"
#include "openmpt/base/Endian.hpp"

#include "mptAssert.h"
#include <algorithm>
#include <array>
#include <iosfwd>
#include <limits>
#include <cstring>


OPENMPT_NAMESPACE_BEGIN



OPENMPT_NAMESPACE_END
