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

class FileCursorTraitsFileData;

class FileCursorFilenameTraitsNone;

template <typename Tpath>
class FileCursorFilenameTraits;

namespace mpt {

template <typename Ttraits, typename Tfilenametraits>
class FileCursor;

} // namespace mpt

namespace detail {

template <typename Ttraits, typename Tfilenametraits>
using FileCursor = mpt::FileCursor<Ttraits, Tfilenametraits>;

template <typename Ttraits, typename Tfilenametraits>
class FileReader;

} // namespace detail

namespace mpt {

class PathString;

} // namespace mpt

using FileCursor = detail::FileCursor<FileCursorTraitsFileData, FileCursorFilenameTraits<mpt::PathString>>;
using FileReader = detail::FileReader<FileCursorTraitsFileData, FileCursorFilenameTraits<mpt::PathString>>;

using MemoryFileCursor = detail::FileCursor<FileCursorTraitsMemory, FileCursorFilenameTraitsNone>;
using MemoryFileReader = detail::FileReader<FileCursorTraitsMemory, FileCursorFilenameTraitsNone>;

OPENMPT_NAMESPACE_END

