/*
 * zlib_helper.h
 * -------------
 * Purpose: Simple wrapper for zlib'S error handling and RAII
 * Notes  : none
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#if defined(MPT_WITH_ZLIB) || defined(MPT_WITH_MINIZ)

#include "mpt/out_of_memory/out_of_memory.hpp"

#include <stdexcept>

#if defined(MPT_WITH_ZLIB)
#include <zlib.h>
#elif defined(MPT_WITH_MINIZ)
#include <miniz/miniz.h>
#endif

OPENMPT_NAMESPACE_BEGIN

namespace zlib
{

	inline int expect_Z_OK(int zlib_errc, const char *msg)
	{
		if(zlib_errc == Z_MEM_ERROR)
		{
			mpt::throw_out_of_memory();
		} else if(zlib_errc < Z_OK)
		{
			throw std::runtime_error{msg};
		}
		return zlib_errc;
	}

	inline int expect_Z_OK_or_Z_BUF_ERROR(int zlib_errc, const char *msg)
	{
		if(zlib_errc == Z_BUF_ERROR)
		{
			// expected
		} else if(zlib_errc == Z_MEM_ERROR)
		{
			mpt::throw_out_of_memory();
		} else if(zlib_errc < Z_OK)
		{
			throw std::runtime_error{msg};
		}
		return zlib_errc;
	}

	inline bool is_Z_OK(int zlib_errc)
	{
		if(zlib_errc == Z_MEM_ERROR)
		{
			mpt::throw_out_of_memory();
		} else if(zlib_errc < Z_OK)
		{
			return false;
		}
		return true;
	}

	inline bool is_Z_OK_or_Z_BUF_ERROR(int zlib_errc)
	{
		if(zlib_errc == Z_BUF_ERROR)
		{
			return true;
		} else if(zlib_errc == Z_MEM_ERROR)
		{
			mpt::throw_out_of_memory();
		} else if(zlib_errc < Z_OK)
		{
			return false;
		}
		return true;
	}

	class z_inflate_stream
	{
	private:
		z_stream stream{};
	public:
		z_inflate_stream() = default;
		inline z_stream & operator*()
		{
			return stream;
		}
		inline z_stream * operator->()
		{
			return &stream;
		}
		inline ~z_inflate_stream()
		{
			try
			{
				expect_Z_OK(inflateEnd(&**this), "inflateEnd() failed");
			} catch(mpt::out_of_memory e)
			{
				mpt::delete_out_of_memory(e);
			} catch(...)
			{
				// ignore
			}
		}
	};

	class z_deflate_stream
	{
	private:
		z_stream stream{};
	public:
		z_deflate_stream() = default;
		inline z_stream & operator*()
		{
			return stream;
		}
		inline z_stream * operator->()
		{
			return &stream;
		}
		inline ~z_deflate_stream()
		{
			try
			{
				expect_Z_OK(deflateEnd(&**this), "deflateEnd() failed");
			} catch(mpt::out_of_memory e)
			{
				mpt::delete_out_of_memory(e);
			} catch(...)
			{
				// ignore
			}
		}
	};

} // namespace zlib

OPENMPT_NAMESPACE_END

#endif
