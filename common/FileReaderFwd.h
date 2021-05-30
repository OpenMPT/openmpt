/*
 * FileReaderFwd.h
 * ---------------
 * Purpose: Forward declaration for class FileReader.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"


OPENMPT_NAMESPACE_BEGIN

class FileCursorTraitsMemory;

class FileCursorTraitsStdStream;

namespace mpt {

template <typename Ttraits>
class FileCursor;

} // namespace mpt

namespace detail {

template <typename Ttraits>
using FileCursor = mpt::FileCursor<Ttraits>;

template <typename Ttraits>
class FileReader;

} // namespace detail

using FileCursor = detail::FileCursor<FileCursorTraitsStdStream>;
using FileReader = detail::FileReader<FileCursorTraitsStdStream>;

using MemoryFileCursor = detail::FileCursor<FileCursorTraitsMemory>;
using MemoryFileReader = detail::FileReader<FileCursorTraitsMemory>;

OPENMPT_NAMESPACE_END

