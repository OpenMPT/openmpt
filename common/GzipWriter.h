/*
 * GzipWriter.h
 * ------------
 * Purpose: Simple wrapper around zlib's Gzip writer
 * Notes  : miniz doesn't implement Gzip writing, so this is only compatible with zlib.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#ifdef MPT_WITH_ZLIB

#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"

#include "mptString.h"
#include "zlib_helper.h"

#include <ctime>

#include <zlib.h>

OPENMPT_NAMESPACE_BEGIN

inline void WriteGzip(std::ostream &output, std::string &outData, const mpt::ustring &fileName)
{
	zlib::z_deflate_stream strm{};
	strm->avail_in = static_cast<uInt>(outData.size());
	strm->next_in = mpt::byte_cast<Bytef *>(outData.data());
	zlib::expect_Z_OK(deflateInit2(&*strm, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 9, Z_DEFAULT_STRATEGY), "deflateInit2() failed");
	gz_header gzHeader{};
	gzHeader.time = static_cast<uLong>(std::time(nullptr));
	std::string filenameISO = mpt::ToCharset(mpt::Charset::ISO8859_1, fileName);
	gzHeader.name = mpt::byte_cast<Bytef *>(filenameISO.data());
	zlib::expect_Z_OK(deflateSetHeader(&*strm, &gzHeader), "deflateSetHeader() failed");
	do
	{
		std::array<Bytef, mpt::IO::BUFFERSIZE_TINY> buffer;
		strm->avail_out = static_cast<uInt>(buffer.size());
		strm->next_out = buffer.data();
		zlib::expect_Z_OK_or_Z_BUF_ERROR(deflate(&*strm, Z_FINISH), "deflate() failed");
		mpt::IO::WritePartial(output, buffer, buffer.size() - strm->avail_out);
	} while(strm->avail_out == 0);
}

OPENMPT_NAMESPACE_END

#endif
