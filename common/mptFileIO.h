/*
 * mptFileIO.h
 * -----------
 * Purpose: A wrapper around std::fstream, enforcing usage of mpt::PathString.
 * Notes  : You should only ever use these wrappers instead of plain std::fstream classes.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#if defined(MPT_ENABLE_FILEIO)

#include "mpt/base/detect_libcxx.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/io_file/fstream.hpp"
#include "mpt/io_file_read/inputfile_filecursor.hpp"

#include "../common/mptString.h"
#include "../common/mptPathString.h"
#include "../common/FileReaderFwd.h"

#include <utility>

#endif // MPT_ENABLE_FILEIO


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ENABLE_FILEIO)


namespace mpt
{

using fstream = mpt::IO::fstream;
using ifstream = mpt::IO::ifstream;
using ofstream = mpt::IO::ofstream;

} // namespace mpt


template <typename Targ1>
inline FileCursor GetFileReader(Targ1 &&arg1)
{
	return mpt::IO::make_FileCursor<mpt::PathString>(std::forward<Targ1>(arg1));
}


template <typename Targ1, typename Targ2>
inline FileCursor GetFileReader(Targ1 &&arg1, Targ2 &&arg2)
{
	return mpt::IO::make_FileCursor<mpt::PathString>(std::forward<Targ1>(arg1), std::forward<Targ2>(arg2));
}


#endif // MPT_ENABLE_FILEIO


OPENMPT_NAMESPACE_END

