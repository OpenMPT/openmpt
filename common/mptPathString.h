/*
 * mptPathString.h
 * ---------------
 * Purpose: Wrapper class around the platform-native representation of path names. Should be the only type that is used to store path names.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/path/os_path.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string_transcode/transcode.hpp"

#include "mptString.h"

#include <stdexcept>
#include <string_view>
#include <vector>



OPENMPT_NAMESPACE_BEGIN



namespace mpt
{



#if defined(MPT_ENABLE_CHARSET_LOCALE)
using RawPathString = mpt::os_path;
#define MPT_PATHSTRING_LITERAL(x) MPT_OSPATH_LITERAL( x )
#define MPT_PATHSTRING(x) mpt::PathString::FromNative(MPT_OSPATH_LITERAL( x ))
#else // !MPT_ENABLE_CHARSET_LOCALE
using RawPathString = mpt::utf8string;
#define MPT_PATHSTRING_LITERAL(x) ( x )
#define MPT_PATHSTRING(x) mpt::PathString::FromNative( x )
#endif // MPT_ENABLE_CHARSET_LOCALE

#define PC_(x) MPT_PATHSTRING_LITERAL(x)
#define PL_(x) MPT_PATHSTRING_LITERAL(x)
#define P_(x) MPT_PATHSTRING(x)


namespace path_literals
{

template <typename Tchar>
struct literals;

template <typename Tchar>
MPT_CONSTEVAL Tchar L(char x) {
	return path_literals::literals<Tchar>::L(x);
}

template <typename Tchar, std::size_t N>
MPT_CONSTEVAL const Tchar * L(const char (&x)[N]) {
	return path_literals::literals<Tchar>::L(x);
}

template <>
struct literals<char> {
	using char_type = char;
	static MPT_CONSTEVAL char_type L(char c) {
		if (c == '\0') return '\0';
		if (c == '\\') return '\\';
		if (c == '/') return '/';
		if (c == '.') return '.';
		if (c == ':') return ':';
#if defined(MPT_COMPILER_QUIRK_NO_CONSTEXPR_THROW)
		else return false ? 0 : throw std::domain_error("invalid path literal");
#else
		throw std::domain_error("invalid path literal");
#endif
	}
	template <std::size_t N>
	static MPT_CONSTEVAL const char_type * L(const char (&s)[N]) {
		if (std::string_view(s) == std::string_view("")) return "";
		if (std::string_view(s) == std::string_view("/")) return "/";
		if (std::string_view(s) == std::string_view(".")) return ".";
		if (std::string_view(s) == std::string_view("\\")) return "\\";
		if (std::string_view(s) == std::string_view("..")) return "..";
		if (std::string_view(s) == std::string_view("//")) return "//";
		if (std::string_view(s) == std::string_view("./")) return "./";
		if (std::string_view(s) == std::string_view(".\\")) return ".\\";
		if (std::string_view(s) == std::string_view("\\/")) return "\\/";
		if (std::string_view(s) == std::string_view("/\\")) return "/\\";
		if (std::string_view(s) == std::string_view("\\\\")) return "\\\\";
		if (std::string_view(s) == std::string_view("\\\\?\\")) return "\\\\?\\";
		if (std::string_view(s) == std::string_view("\\\\?\\UNC")) return "\\\\?\\UNC";
		if (std::string_view(s) == std::string_view("\\\\?\\UNC\\")) return "\\\\?\\UNC\\";
#if defined(MPT_COMPILER_QUIRK_NO_CONSTEXPR_THROW)
		else return false ? nullptr : throw std::domain_error("invalid path literal");
#else
		throw std::domain_error("invalid path literal");
#endif
	}
};

#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)
template <>
struct literals<wchar_t> {
	using char_type = wchar_t;
	static MPT_CONSTEVAL char_type L(char c) {
		if (c == '\0') return L'\0';
		if (c == '\\') return L'\\';
		if (c == '/') return L'/';
		if (c == '.') return L'.';
		if (c == ':') return L':';
#if defined(MPT_COMPILER_QUIRK_NO_CONSTEXPR_THROW)
		else return false ? 0 : throw std::domain_error("invalid path literal");
#else
		throw std::domain_error("invalid path literal");
#endif
	}
	template <std::size_t N>
	static MPT_CONSTEVAL const char_type * L(const char (&s)[N]) {
		if (std::string_view(s) == std::string_view("")) return L"";
		if (std::string_view(s) == std::string_view("/")) return L"/";
		if (std::string_view(s) == std::string_view(".")) return L".";
		if (std::string_view(s) == std::string_view("\\")) return L"\\";
		if (std::string_view(s) == std::string_view("..")) return L"..";
		if (std::string_view(s) == std::string_view("//")) return L"//";
		if (std::string_view(s) == std::string_view("./")) return L"./";
		if (std::string_view(s) == std::string_view(".\\")) return L".\\";
		if (std::string_view(s) == std::string_view("\\/")) return L"\\/";
		if (std::string_view(s) == std::string_view("/\\")) return L"/\\";
		if (std::string_view(s) == std::string_view("\\\\")) return L"\\\\";
		if (std::string_view(s) == std::string_view("\\\\?\\")) return L"\\\\?\\";
		if (std::string_view(s) == std::string_view("\\\\?\\UNC")) return L"\\\\?\\UNC";
		if (std::string_view(s) == std::string_view("\\\\?\\UNC\\")) return L"\\\\?\\UNC\\";
#if defined(MPT_COMPILER_QUIRK_NO_CONSTEXPR_THROW)
		else return false ? nullptr : throw std::domain_error("invalid path literal");
#else
		throw std::domain_error("invalid path literal");
#endif
	}
};
#endif // !MPT_COMPILER_QUIRK_NO_WCHAR

#if MPT_CXX_AT_LEAST(20)
template <>
struct literals<char8_t> {
	using char_type = char8_t;
	static MPT_CONSTEVAL char_type L(char c) {
		if (c == '\0') return u8'\0';
		if (c == '\\') return u8'\\';
		if (c == '/') return u8'/';
		if (c == '.') return u8'.';
		if (c == ':') return u8':';
#if defined(MPT_COMPILER_QUIRK_NO_CONSTEXPR_THROW)
		else return false ? 0 : throw std::domain_error("invalid path literal");
#else
		throw std::domain_error("invalid path literal");
#endif
	}
	template <std::size_t N>
	static MPT_CONSTEVAL const char_type * L(const char (&s)[N]) {
		if (std::string_view(s) == std::string_view("")) return u8"";
		if (std::string_view(s) == std::string_view("/")) return u8"/";
		if (std::string_view(s) == std::string_view(".")) return u8".";
		if (std::string_view(s) == std::string_view("\\")) return u8"\\";
		if (std::string_view(s) == std::string_view("..")) return u8"..";
		if (std::string_view(s) == std::string_view("//")) return u8"//";
		if (std::string_view(s) == std::string_view("./")) return u8"./";
		if (std::string_view(s) == std::string_view(".\\")) return u8".\\";
		if (std::string_view(s) == std::string_view("\\/")) return u8"\\/";
		if (std::string_view(s) == std::string_view("/\\")) return u8"/\\";
		if (std::string_view(s) == std::string_view("\\\\")) return u8"\\\\";
		if (std::string_view(s) == std::string_view("\\\\?\\")) return u8"\\\\?\\";
		if (std::string_view(s) == std::string_view("\\\\?\\UNC")) return u8"\\\\?\\UNC";
		if (std::string_view(s) == std::string_view("\\\\?\\UNC\\")) return u8"\\\\?\\UNC\\";
#if defined(MPT_COMPILER_QUIRK_NO_CONSTEXPR_THROW)
		else return false ? nullptr : throw std::domain_error("invalid path literal");
#else
		throw std::domain_error("invalid path literal");
#endif
	}
};
#endif // C++20

template <>
struct literals<char16_t> {
	using char_type = char16_t;
	static MPT_CONSTEVAL char_type L(char c) {
		if (c == '\0') return u'\0';
		if (c == '\\') return u'\\';
		if (c == '/') return u'/';
		if (c == '.') return u'.';
		if (c == ':') return u':';
#if defined(MPT_COMPILER_QUIRK_NO_CONSTEXPR_THROW)
		else return false ? 0 : throw std::domain_error("invalid path literal");
#else
		throw std::domain_error("invalid path literal");
#endif
	}
	template <std::size_t N>
	static MPT_CONSTEVAL const char_type * L(const char (&s)[N]) {
		if (std::string_view(s) == std::string_view("")) return u"";
		if (std::string_view(s) == std::string_view("/")) return u"/";
		if (std::string_view(s) == std::string_view(".")) return u".";
		if (std::string_view(s) == std::string_view("\\")) return u"\\";
		if (std::string_view(s) == std::string_view("..")) return u"..";
		if (std::string_view(s) == std::string_view("//")) return u"//";
		if (std::string_view(s) == std::string_view("./")) return u"./";
		if (std::string_view(s) == std::string_view(".\\")) return u".\\";
		if (std::string_view(s) == std::string_view("\\/")) return u"\\/";
		if (std::string_view(s) == std::string_view("/\\")) return u"/\\";
		if (std::string_view(s) == std::string_view("\\\\")) return u"\\\\";
		if (std::string_view(s) == std::string_view("\\\\?\\")) return u"\\\\?\\";
		if (std::string_view(s) == std::string_view("\\\\?\\UNC")) return u"\\\\?\\UNC";
		if (std::string_view(s) == std::string_view("\\\\?\\UNC\\")) return u"\\\\?\\UNC\\";
#if defined(MPT_COMPILER_QUIRK_NO_CONSTEXPR_THROW)
		else return false ? nullptr : throw std::domain_error("invalid path literal");
#else
		throw std::domain_error("invalid path literal");
#endif
	}
};

template <>
struct literals<char32_t> {
	using char_type = char32_t;
	static MPT_CONSTEVAL char_type L(char c) {
		if (c == '\0') return U'\0';
		if (c == '\\') return U'\\';
		if (c == '/') return U'/';
		if (c == '.') return U'.';
		if (c == ':') return U':';
#if defined(MPT_COMPILER_QUIRK_NO_CONSTEXPR_THROW)
		else return false ? 0 : throw std::domain_error("invalid path literal");
#else
		throw std::domain_error("invalid path literal");
#endif
	}
	template <std::size_t N>
	static MPT_CONSTEVAL const char_type * L(const char (&s)[N]) {
		if (std::string_view(s) == std::string_view("")) return U"";
		if (std::string_view(s) == std::string_view("/")) return U"/";
		if (std::string_view(s) == std::string_view(".")) return U".";
		if (std::string_view(s) == std::string_view("\\")) return U"\\";
		if (std::string_view(s) == std::string_view("..")) return U"..";
		if (std::string_view(s) == std::string_view("//")) return U"//";
		if (std::string_view(s) == std::string_view("./")) return U"./";
		if (std::string_view(s) == std::string_view(".\\")) return U".\\";
		if (std::string_view(s) == std::string_view("\\/")) return U"\\/";
		if (std::string_view(s) == std::string_view("/\\")) return U"/\\";
		if (std::string_view(s) == std::string_view("\\\\")) return U"\\\\";
		if (std::string_view(s) == std::string_view("\\\\?\\")) return U"\\\\?\\";
		if (std::string_view(s) == std::string_view("\\\\?\\UNC")) return U"\\\\?\\UNC";
		if (std::string_view(s) == std::string_view("\\\\?\\UNC\\")) return U"\\\\?\\UNC\\";
#if defined(MPT_COMPILER_QUIRK_NO_CONSTEXPR_THROW)
		else return false ? nullptr : throw std::domain_error("invalid path literal");
#else
		throw std::domain_error("invalid path literal");
#endif
	}
};

} // namespace path_literals



enum class PathStyle
{
	Posix,
	WindowsNT,
	Windows9x,
	DJGPP,
};

#if MPT_OS_WINDOWS
inline constexpr PathStyle NativePathStyle = PathStyle::WindowsNT;
#else
inline constexpr PathStyle NativePathStyle = PathStyle::Posix;
#endif



template <typename TRawPath, PathStyle EPathStyle>
struct PathTraits
{



	static inline constexpr PathStyle path_style = EPathStyle;
	using raw_path_type = TRawPath;

	static_assert((path_style == PathStyle::WindowsNT) || (path_style == PathStyle::Posix));



	static bool IsPathSeparator(raw_path_type::value_type c)
	{
		using namespace path_literals;
		using char_type = raw_path_type::value_type;
		bool result{};
		if constexpr((path_style == PathStyle::WindowsNT) || (path_style == PathStyle::Windows9x) || (path_style == PathStyle::DJGPP))
		{
			result = (c == L<char_type>('\\')) || (c == L<char_type>('/'));
		} else if constexpr(path_style == PathStyle::Posix)
		{
			result = (c == L<char_type>('/'));
		} else
		{
			//static_assert(false);
		}
		return result;
	}



	static raw_path_type::value_type GetDefaultPathSeparator()
	{
		using namespace path_literals;
		using char_type = raw_path_type::value_type;
		typename raw_path_type::value_type result{};
		if constexpr((path_style == PathStyle::WindowsNT) || (path_style == PathStyle::Windows9x) || (path_style == PathStyle::DJGPP))
		{
			result = L<char_type>('\\');
		} else if constexpr(path_style == PathStyle::Posix)
		{
			result = L<char_type>('/');
		} else
		{
			//static_assert(false);
		}
		return result;
	}



	static void SplitPath(raw_path_type p, raw_path_type *prefix, raw_path_type *drive, raw_path_type *dir, raw_path_type *fbase, raw_path_type *fext)
	{

		using namespace path_literals;
		using char_type = raw_path_type::value_type;

		if(prefix) *prefix = raw_path_type();
		if(drive) *drive = raw_path_type();
		if(dir) *dir = raw_path_type();
		if(fbase) *fbase = raw_path_type();
		if(fext) *fext = raw_path_type();

		if constexpr(path_style == PathStyle::WindowsNT)
		{

			// We cannot use CRT splitpath here, because:
			//  * limited to _MAX_PATH or similar
			//  * no support for UNC paths
			//  * no support for \\?\ prefixed paths

			// remove \\?\\ prefix
			if(p.substr(0, 8) == L<char_type>("\\\\?\\UNC\\"))
			{
				if(prefix) *prefix = L<char_type>("\\\\?\\UNC");
				p = L<char_type>("\\\\") + p.substr(8);
			} else if(p.substr(0, 4) == L<char_type>("\\\\?\\"))
			{
				if (prefix) *prefix = L<char_type>("\\\\?\\");
				p = p.substr(4);
			}

			if(p.length() >= 2 && (
				p.substr(0, 2) == L<char_type>("\\\\")
				|| p.substr(0, 2) == L<char_type>("\\/")
				|| p.substr(0, 2) == L<char_type>("/\\")
				|| p.substr(0, 2) == L<char_type>("//")
				))
			{ // UNC
				typename raw_path_type::size_type first_slash = p.substr(2).find_first_of(L<char_type>("\\/"));
				if(first_slash != raw_path_type::npos)
				{
					typename raw_path_type::size_type second_slash = p.substr(2 + first_slash + 1).find_first_of(L<char_type>("\\/"));
					if(second_slash != raw_path_type::npos)
					{
						if(drive) *drive = p.substr(0, 2 + first_slash + 1 + second_slash);
						p = p.substr(2 + first_slash + 1 + second_slash);
					} else
					{
						if(drive) *drive = p;
						p = raw_path_type();
					}
				} else
				{
					if(drive) *drive = p;
					p = raw_path_type();
				}
			} else
			{ // local
				if(p.length() >= 2 && (p[1] == L<char_type>(':')))
				{
					if(drive) *drive = p.substr(0, 2);
					p = p.substr(2);
				} else
				{
					if(drive) *drive = raw_path_type();
				}
			}
			typename raw_path_type::size_type last_slash = p.find_last_of(L<char_type>("\\/"));
			if(last_slash != raw_path_type::npos)
			{
				if(dir) *dir = p.substr(0, last_slash + 1);
				p = p.substr(last_slash + 1);
			} else
			{
				if(dir) *dir = raw_path_type();
			}
			typename raw_path_type::size_type last_dot = p.find_last_of(L<char_type>("."));
			if(last_dot == raw_path_type::npos)
			{
				if(fbase) *fbase = p;
				if(fext) *fext = raw_path_type();
			} else if(last_dot == 0)
			{
				if(fbase) *fbase = p;
				if(fext) *fext = raw_path_type();
			} else if(p == L<char_type>(".") || p == L<char_type>(".."))
			{
				if(fbase) *fbase = p;
				if(fext) *fext = raw_path_type();
			} else
			{
				if(fbase) *fbase = p.substr(0, last_dot);
				if(fext) *fext = p.substr(last_dot);
			}

		} else if constexpr(path_style == PathStyle::Posix)
		{

			typename raw_path_type::size_type last_slash = p.find_last_of(L<char_type>("/"));
			if(last_slash != raw_path_type::npos)
			{
				if(dir) *dir = p.substr(0, last_slash + 1);
				p = p.substr(last_slash + 1);
			} else
			{
				if(dir) *dir = raw_path_type();
			}
			typename raw_path_type::size_type last_dot = p.find_last_of(L<char_type>("."));
			if(last_dot == raw_path_type::npos)
			{
				if(fbase) *fbase = p;
				if(fext) *fext = raw_path_type();
			} else if(last_dot == 0)
			{
				if(fbase) *fbase = p;
				if(fext) *fext = raw_path_type();
			} else if(p == L<char_type>(".") || p == L<char_type>(".."))
			{
				if(fbase) *fbase = p;
				if(fext) *fext = raw_path_type();
			} else
			{
				if(fbase) *fbase = p.substr(0, last_dot);
				if(fext) *fext = p.substr(last_dot);
			}

		} else
		{

			//static_assert(false);

		}

	}



	// Convert a path to its simplified form, i.e. remove ".\" and "..\" entries
	// Note: We use our own implementation as PathCanonicalize is limited to MAX_PATH
	// and unlimited versions are only available on Windows 8 and later.
	// Furthermore, we also convert forward-slashes to backslashes and always remove trailing slashes.
	static raw_path_type Simplify(const raw_path_type &path)
	{

		using namespace path_literals;
		using char_type = raw_path_type::value_type;

		raw_path_type result{};

		if(path.empty())
		{
			return result;
		}

		std::vector<raw_path_type> components;

		if constexpr(path_style == PathStyle::WindowsNT)
		{

			raw_path_type root;
			typename raw_path_type::size_type startPos = 0;
			if(path.size() >= 2 && path[1] == L<char_type>(':'))
			{
				// Drive letter
				root = path.substr(0, 2) + L<char_type>('\\');
				startPos = 2;
			} else if(path.substr(0, 2) == L<char_type>("\\\\"))
			{
				// Network share
				root = L<char_type>("\\\\");
				startPos = 2;
			} else if(path.substr(0, 2) == L<char_type>(".\\") || path.substr(0, 2) == L<char_type>("./"))
			{
				// Special case for relative paths
				root = L<char_type>(".\\");
				startPos = 2;
			} else if(path.size() >= 1 && (path[0] == L<char_type>('\\') || path[0] == L<char_type>('/')))
			{
				// Special case for relative paths
				root = L<char_type>("\\");
				startPos = 1;
			}

			while(startPos < path.size())
			{
				auto pos = path.find_first_of(L<char_type>("\\/"), startPos);
				if(pos == raw_path_type::npos)
				{
					pos = path.size();
				}
				raw_path_type dir = path.substr(startPos, pos - startPos);
				if(dir == L<char_type>(".."))
				{
					// Go back one directory
					if(!components.empty())
					{
						components.pop_back();
					}
				} else if(dir == L<char_type>("."))
				{
					// nop
				} else if(!dir.empty())
				{
					components.push_back(std::move(dir));
				}
				startPos = pos + 1;
			}

			result = root;
			result.reserve(path.size());
			for(const auto &component : components)
			{
				result += component + L<char_type>("\\");
			}
			if(!components.empty())
			{
				result.pop_back();
			}

		} else if constexpr(path_style == PathStyle::Posix)
		{

			raw_path_type root;
			typename raw_path_type::size_type startPos = 0;
			if(path.substr(0, 2) == L<char_type>("./"))
			{
				// Special case for relative paths
				root = L<char_type>("./");
				startPos = 2;
			} else if(path.size() >= 1 && (path[0] == L<char_type>('/')))
			{
				// Special case for relative paths
				root = L<char_type>("/");
				startPos = 1;
			}

			while(startPos < path.size())
			{
				auto pos = path.find_first_of(L<char_type>("/"), startPos);
				if(pos == raw_path_type::npos)
				{
					pos = path.size();
				}
				raw_path_type dir = path.substr(startPos, pos - startPos);
				if(dir == L<char_type>(".."))
				{
					// Go back one directory
					if(!components.empty())
					{
						components.pop_back();
					}
				} else if(dir == L<char_type>("."))
				{
					// nop
				} else if(!dir.empty())
				{
					components.push_back(std::move(dir));
				}
				startPos = pos + 1;
			}

			result = root;
			result.reserve(path.size());
			for(const auto &component : components)
			{
				result += component + L<char_type>("/");
			}
			if(!components.empty())
			{
				result.pop_back();
			}

		} else
		{

			//static_assert(false);

		}

		return result;

	}



	static bool IsAbsolute(const raw_path_type &path)
	{
		bool result{};
		if constexpr(path_style == PathStyle::WindowsNT)
		{
			using namespace path_literals;
			using char_type = raw_path_type::value_type;
			if(path.substr(0, 8) == L<char_type>("\\\\?\\UNC\\"))
			{
				return true;
			}
			if(path.substr(0, 4) == L<char_type>("\\\\?\\"))
			{
				return true;
			}
			if(path.substr(0, 2) == L<char_type>("\\\\"))
			{
				return true; // UNC
			}
			if(path.substr(0, 2) == L<char_type>("//"))
			{
				return true; // UNC
			}
			result = (path.length()) >= 3 && (path[1] == L<char_type>(':')) && IsPathSeparator(path[2]);
		} else if constexpr(path_style == PathStyle::Posix)
		{
			result = (path.length() >= 1) && IsPathSeparator(path[0]);
		} else
		{
			//static_assert(false);
		}
		return result;
	}



};



class PathString
{

private:

	using path_traits = PathTraits<RawPathString, NativePathStyle>;
	using raw_path_type = path_traits::raw_path_type;

	raw_path_type path;
	
private:

	explicit PathString(const raw_path_type &path_)
		: path(path_)
	{
		return;
	}

public:

	PathString() = default;
	PathString(const PathString &) = default;
	PathString(PathString &&) noexcept = default;

	PathString & assign(const PathString & other)
	{
		path = other.path;
		return *this;
	}

	PathString & assign(PathString && other) noexcept
	{
		path = std::move(other.path);
		return *this;
	}
	
	PathString & operator = (const PathString & other)
	{
		return assign(other);
	}
	
	PathString &operator = (PathString && other) noexcept
	{
		return assign(std::move(other));
	}
	
	PathString & append(const PathString & other)
	{
		path.append(other.path);
		return *this;
	}
	
	PathString & operator += (const PathString & other)
	{
		return append(other);
	}

	friend PathString operator + (const PathString & a, const PathString & b)
	{
		return PathString(a).append(b);
	}

	friend bool operator < (const PathString & a, const PathString & b)
	{
		return a.AsNative() < b.AsNative();
	}

	friend bool operator == (const PathString & a, const PathString & b)
	{
		return a.AsNative() == b.AsNative();
	}
	
	friend bool operator != (const PathString & a, const PathString & b)
	{
		return a.AsNative() != b.AsNative();
	}

	bool empty() const
	{
		return path.empty();
	}

	std::size_t length() const
	{
		return path.size();
	}

	std::size_t Length() const
	{
		return path.size();
	}

public:

	raw_path_type AsNative() const
	{
		return path;
	}
	
	static PathString FromNative(const raw_path_type &path)
	{
		return PathString(path);
	}

	mpt::ustring ToUnicode() const
	{
		return mpt::transcode<mpt::ustring>(path);
	}

	static PathString FromUnicode(const mpt::ustring &path)
	{
		return PathString(mpt::transcode<raw_path_type>(path));
	}

	std::string ToUTF8() const
	{
		return mpt::transcode<std::string>(mpt::common_encoding::utf8, path);
	}

	static PathString FromUTF8(const std::string &path)
	{
		return PathString(mpt::transcode<raw_path_type>(mpt::common_encoding::utf8, path));
	}

#if MPT_WSTRING_CONVERT

	std::wstring ToWide() const
	{
		return mpt::transcode<std::wstring>(path);
	}

	static PathString FromWide(const std::wstring &path)
	{
		return PathString(mpt::transcode<raw_path_type>(path));
	}

#endif // MPT_WSTRING_CONVERT

#if defined(MPT_ENABLE_CHARSET_LOCALE)

	std::string ToLocale() const
	{
		return mpt::transcode<std::string>(mpt::logical_encoding::locale, path);
	}

	static PathString FromLocale(const std::string &path)
	{
		return PathString(mpt::transcode<raw_path_type>(mpt::logical_encoding::locale, path));
	}

#endif // MPT_ENABLE_CHARSET_LOCALE

#if defined(MPT_WITH_MFC)

	CString ToCString() const
	{
		return mpt::transcode<CString>(path);
	}

	static PathString FromCString(const CString &path)
	{
		return PathString(mpt::transcode<raw_path_type>(path));
	}

#endif // MPT_WITH_MFC

public:

	static bool IsPathSeparator(raw_path_type::value_type c)
	{
		return path_traits::IsPathSeparator(c);
	}

	static raw_path_type::value_type GetDefaultPathSeparator()
	{
		return path_traits::GetDefaultPathSeparator();
	}

	bool HasTrailingSlash() const
	{
		if(path.empty())
		{
			return false;
		}
		raw_path_type::value_type c = path[path.length() - 1];
		return IsPathSeparator(c);
	}

	PathString WithoutTrailingSlash() const
	{
		PathString result = *this;
		while(result.HasTrailingSlash())
		{
			if(result.Length() == 1)
			{
				return result;
			}
			result = PathString(result.AsNative().substr(0, result.AsNative().length() - 1));
		}
		return result;
	}

	PathString WithTrailingSlash() const
	{
		PathString result = *this;
		if(!result.empty() && !result.HasTrailingSlash())
		{
			result.path += GetDefaultPathSeparator();
		}
		return result;
	}

	void SplitPath(PathString *prefix, PathString *drive, PathString *dir, PathString *fbase, PathString *fext) const
	{
		path_traits::SplitPath(path, prefix ? &prefix->path : nullptr, drive ? &drive->path : nullptr, dir ? &dir->path : nullptr, fbase ? &fbase->path : nullptr, fext ? &fext->path : nullptr);
	}

	// \\?\ or \\?\\UNC or empty
	PathString GetPrefix() const
	{
		PathString prefix;
		SplitPath(&prefix, nullptr, nullptr, nullptr, nullptr);
		return prefix;
	}
	
	// Drive letter + colon, e.g. "C:" or \\server\share
	PathString GetDrive() const
	{
		PathString drive;
		SplitPath(nullptr, &drive, nullptr, nullptr, nullptr);
		return drive;
	}
	
	// Directory, e.g. "\OpenMPT\"
	PathString GetDirectory() const
	{
		PathString dir;
		SplitPath(nullptr, nullptr, &dir, nullptr, nullptr);
		return dir;
	}
	
	// Drive + Dir, e.g. "C:\OpenMPT\"
	PathString GetDirectoryWithDrive() const
	{
		PathString drive, dir;
		SplitPath(nullptr, &drive, &dir, nullptr, nullptr);
		return drive + dir;
	}
	
	// File name without extension, e.g. "OpenMPT"
	PathString GetFilenameBase() const
	{
		PathString fname;
		SplitPath(nullptr, nullptr, nullptr, &fname, nullptr);
		return fname;
	}
	
	// Extension including dot, e.g. ".exe"
	PathString GetFilenameExtension() const
	{
		PathString ext;
		SplitPath(nullptr, nullptr, nullptr, nullptr, &ext);
		return ext;
	}
	
	// File name + extension, e.g. "OpenMPT.exe"
	PathString GetFilename() const
	{
		PathString name, ext;
		SplitPath(nullptr, nullptr, nullptr, &name, &ext);
		return name + ext;
	}

	// Return the same path string with a different (or appended) extension (including "."), e.g. "foo.bar",".txt" -> "foo.txt" or "C:\OpenMPT\foo",".txt" -> "C:\OpenMPT\foo.txt"
	PathString ReplaceExtension(const PathString &newExt) const
	{
		return GetDirectoryWithDrive() + GetFilenameBase() + newExt;
	}

	// Convert a path to its simplified form, i.e. remove ".\" and "..\" entries, similar to std::fs::path::lexically_normal
	PathString Simplify() const
	{
		return PathString::FromNative(path_traits::Simplify(path));
	}

	bool IsAbsolute() const
	{
		return path_traits::IsAbsolute(path);
	}

};



#if defined(MPT_ENABLE_CHARSET_LOCALE)
inline std::string ToAString(const mpt::PathString &x)
{
	return x.ToLocale();
}
#else
inline std::string ToAString(const mpt::PathString &x)
{
	return x.ToUTF8();
}
#endif
inline mpt::ustring ToUString(const mpt::PathString &x)
{
	return x.ToUnicode();
}
#if MPT_WSTRING_FORMAT
inline std::wstring ToWString(const mpt::PathString &x)
{
	return x.ToWide();
}
#endif



// Return native string, with possible \\?\ prefix if it exceeds MAX_PATH characters.
mpt::RawPathString SupportLongPath(const mpt::RawPathString &path);

#if MPT_OS_WINDOWS
#if !(MPT_OS_WINDOWS_WINRT && (_WIN32_WINNT < 0x0a00))
// Returns the absolute path for a potentially relative path and removes ".." or "." components. (same as GetFullPathNameW)
mpt::PathString GetAbsolutePath(const mpt::PathString &path);
#endif
#endif // MPT_OS_WINDOWS

#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS

// Relative / absolute paths conversion

mpt::PathString AbsolutePathToRelative(const mpt::PathString &p, const mpt::PathString &relativeTo); // similar to std::fs::path::lexically_approximate
	
mpt::PathString RelativePathToAbsolute(const mpt::PathString &p, const mpt::PathString &relativeTo);

#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS



#if MPT_OS_WINDOWS
#if !MPT_OS_WINDOWS_WINRT
int PathCompareNoCase(const PathString &a, const PathString &b);
#endif // !MPT_OS_WINDOWS_WINRT
#endif



} // namespace mpt



#if defined(MODPLUG_TRACKER)

// Sanitize a filename (remove special chars)

mpt::PathString SanitizePathComponent(const mpt::PathString &filename);

std::string SanitizePathComponent(std::string str);
std::wstring SanitizePathComponent(std::wstring str);
#if MPT_USTRING_MODE_UTF8
mpt::u8string SanitizePathComponent(mpt::u8string str);
#endif // MPT_USTRING_MODE_UTF8
#if defined(MPT_WITH_MFC)
CString SanitizePathComponent(CString str);
#endif // MPT_WITH_MFC

#endif // MODPLUG_TRACKER



OPENMPT_NAMESPACE_END
