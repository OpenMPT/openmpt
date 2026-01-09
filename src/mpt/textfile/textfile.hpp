/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_TEXTFILE_TEXTFILE_HPP
#define MPT_TEXTFILE_TEXTFILE_HPP



#include "mpt/base/alloc.hpp"
#include "mpt/base/bit.hpp"
#include "mpt/base/detect.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/base/memory.hpp"
#include "mpt/base/saturate_cast.hpp"
#include "mpt/base/span.hpp"
#include "mpt/endian/integer.hpp"
#include "mpt/io_read/filecursor.hpp"
#include "mpt/io_read/filecursor_filename_traits.hpp"
#include "mpt/io_read/filecursor_traits_filedata.hpp"
#include "mpt/io_read/filereader.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string_transcode/transcode.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include <cassert>
#include <cstddef>
#include <cstring>

#if MPT_OS_WINDOWS
#include <windows.h>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace textfile {



struct encoding {
	static inline constexpr const std::array<std::byte, 4> bom_utf32be = {std::byte{0x00}, std::byte{0x00}, std::byte{0xfe}, std::byte{0xff}};
	static inline constexpr const std::array<std::byte, 4> bom_utf32le = {std::byte{0xff}, std::byte{0xfe}, std::byte{0x00}, std::byte{0x00}};
	static inline constexpr const std::array<std::byte, 2> bom_utf16be = {std::byte{0xfe}, std::byte{0xff}};
	static inline constexpr const std::array<std::byte, 2> bom_utf16le = {std::byte{0xff}, std::byte{0xfe}};
	static inline constexpr const std::array<std::byte, 3> bom_utf8 = {std::byte{0xef}, std::byte{0xbb}, std::byte{0xbf}};
	enum class type {
		utf32be,
		utf32le,
		utf16be,
		utf16le,
		utf8,
		locale,
	};
	enum class header {
		none,
		bom,
	};
	type type;
	header header;
	friend inline constexpr bool operator==(encoding a, encoding b) {
		return (a.type == b.type) && (a.header == b.header);
	}
	friend inline constexpr bool operator!=(encoding a, encoding b) {
		return (a.type != b.type) || (a.header != b.header);
	}
	inline mpt::const_byte_span get_header() const {
		mpt::const_byte_span result{};
		if (header == header::bom) {
			switch (type) {
				case type::utf32be:
					result = bom_utf32be;
					break;
				case type::utf32le:
					result = bom_utf32le;
					break;
				case type::utf16be:
					result = bom_utf16be;
					break;
				case type::utf16le:
					result = bom_utf16le;
					break;
				case type::utf8:
					result = bom_utf8;
					break;
				case type::locale:
					result = mpt::const_byte_span{};
					break;
			}
		}
		return result;
	}
	inline std::size_t get_text_offset() const {
		return get_header().size();
	}
};



inline encoding get_preferred_encoding() {
	encoding result = encoding{encoding::type::utf8, encoding::header::none};
#if MPT_OS_WINDOWS
#if defined(UNICODE)
	result = encoding{encoding::type::utf16le, encoding::header::bom};
#else
	if (mpt::osinfo::windows::Version::Current().IsAtLeast(mpt::osinfo::windows::Version::Win2000)) {
		result = encoding{encoding::type::utf16le, encoding::header::bom};
	} else {
		result = encoding{encoding::type::locale, encoding::header::none};
	}
#endif
	if ((result.type == encoding::type::utf16le) && mpt::osinfo::windows::current_is_wine()) {
		result = encoding{encoding::type::utf8, encoding::header::bom};
	}
#else
	result = encoding::locale;
#endif
	return result;
}



inline encoding probe_encoding(mpt::const_byte_span filedata) {
	encoding result = {encoding::type::locale, encoding::header::none};
	if ((filedata.size() >= encoding::bom_utf32be.size()) && (std::memcmp(filedata.data(), encoding::bom_utf32be.data(), encoding::bom_utf32be.size()) == 0)) {
		result = {encoding::type::utf32be, encoding::header::bom};
	} else if ((filedata.size() >= encoding::bom_utf32le.size()) && (std::memcmp(filedata.data(), encoding::bom_utf32le.data(), encoding::bom_utf32le.size()) == 0)) {
		result = {encoding::type::utf32le, encoding::header::bom};
	} else if ((filedata.size() >= encoding::bom_utf16be.size()) && (std::memcmp(filedata.data(), encoding::bom_utf16be.data(), encoding::bom_utf16be.size()) == 0)) {
		result = {encoding::type::utf16be, encoding::header::bom};
	} else if ((filedata.size() >= encoding::bom_utf16le.size()) && (std::memcmp(filedata.data(), encoding::bom_utf16le.data(), encoding::bom_utf16le.size()) == 0)) {
		result = {encoding::type::utf16le, encoding::header::bom};
	} else if ((filedata.size() >= encoding::bom_utf8.size()) && (std::memcmp(filedata.data(), encoding::bom_utf8.data(), encoding::bom_utf8.size()) == 0)) {
		result = {encoding::type::utf8, encoding::header::bom};
#if MPT_OS_WINDOWS
	} else if (!filedata.empty() && IsTextUnicode(filedata.data(), mpt::saturate_cast<int>(filedata.size()), NULL)) {
		result = {encoding::type::utf16le, encoding::header::none};
#endif
	} else if (mpt::is_utf8(mpt::buffer_cast<std::string>(filedata))) {
		result = {encoding::type::utf8, encoding::header::none};
	} else {
		result = {encoding::type::locale, encoding::header::none};
	}
	return result;
}



inline mpt::ustring decode(encoding encoding, mpt::const_byte_span filedata) {
	mpt::const_byte_span textdata = filedata.subspan(encoding.get_text_offset());
	mpt::ustring filetext;
	switch (encoding.type) {
		case encoding::type::utf32be:
			MPT_MAYBE_CONSTANT_IF (mpt::endian_is_big()) {
				std::u32string utf32data;
				std::size_t count = textdata.size() / sizeof(char32_t);
				utf32data.resize(count);
				std::memcpy(utf32data.data(), textdata.data(), count * sizeof(char32_t));
				filetext = mpt::transcode<mpt::ustring>(utf32data);
			} else {
				auto fc = mpt::IO::FileCursor<mpt::IO::FileCursorTraitsFileData, mpt::IO::FileCursorFilenameTraitsNone>{textdata};
				std::u32string utf32data;
				utf32data.reserve(fc.GetLength() / sizeof(char32_t));
				while (!fc.EndOfFile()) {
					utf32data.push_back(static_cast<char32_t>(mpt::IO::FileReader::ReadUint32BE(fc)));
				}
				filetext = mpt::transcode<mpt::ustring>(utf32data);
			}
			break;
		case encoding::type::utf32le:
			MPT_MAYBE_CONSTANT_IF (mpt::endian_is_little()) {
				std::u32string utf32data;
				std::size_t count = textdata.size() / sizeof(char32_t);
				utf32data.resize(count);
				std::memcpy(utf32data.data(), textdata.data(), count * sizeof(char32_t));
				filetext = mpt::transcode<mpt::ustring>(utf32data);
			} else {
				auto fc = mpt::IO::FileCursor<mpt::IO::FileCursorTraitsFileData, mpt::IO::FileCursorFilenameTraitsNone>{textdata};
				std::u32string utf32data;
				utf32data.reserve(fc.GetLength() / sizeof(char32_t));
				while (!fc.EndOfFile()) {
					utf32data.push_back(static_cast<char32_t>(mpt::IO::FileReader::ReadUint32LE(fc)));
				}
				filetext = mpt::transcode<mpt::ustring>(utf32data);
			}
			break;
		case encoding::type::utf16be:
			MPT_MAYBE_CONSTANT_IF (mpt::endian_is_big()) {
				std::u16string utf16data;
				std::size_t count = textdata.size() / sizeof(char16_t);
				utf16data.resize(count);
				std::memcpy(utf16data.data(), textdata.data(), count * sizeof(char16_t));
				filetext = mpt::transcode<mpt::ustring>(utf16data);
			} else {
				auto fc = mpt::IO::FileCursor<mpt::IO::FileCursorTraitsFileData, mpt::IO::FileCursorFilenameTraitsNone>{textdata};
				std::u16string utf16data;
				utf16data.reserve(fc.GetLength() / sizeof(char16_t));
				while (!fc.EndOfFile()) {
					utf16data.push_back(static_cast<char16_t>(mpt::IO::FileReader::ReadUint16BE(fc)));
				}
				filetext = mpt::transcode<mpt::ustring>(utf16data);
			}
			break;
		case encoding::type::utf16le:
			MPT_MAYBE_CONSTANT_IF (MPT_OS_WINDOWS) {
				std::wstring utf16data;
				std::size_t count = textdata.size() / sizeof(wchar_t);
				utf16data.resize(count);
				std::memcpy(utf16data.data(), textdata.data(), count * sizeof(wchar_t));
				filetext = mpt::transcode<mpt::ustring>(utf16data);
			} else MPT_MAYBE_CONSTANT_IF (mpt::endian_is_little()) {
				std::u16string utf16data;
				std::size_t count = textdata.size() / sizeof(char16_t);
				utf16data.resize(count);
				std::memcpy(utf16data.data(), textdata.data(), count * sizeof(char16_t));
				filetext = mpt::transcode<mpt::ustring>(utf16data);
			} else {
				auto fc = mpt::IO::FileCursor<mpt::IO::FileCursorTraitsFileData, mpt::IO::FileCursorFilenameTraitsNone>{textdata};
				std::u16string utf16data;
				utf16data.reserve(fc.GetLength() / sizeof(char16_t));
				while (!fc.EndOfFile()) {
					utf16data.push_back(static_cast<char16_t>(mpt::IO::FileReader::ReadUint16LE(fc)));
				}
				filetext = mpt::transcode<mpt::ustring>(utf16data);
			}
			break;
		case encoding::type::utf8:
			filetext = mpt::transcode<mpt::ustring>(mpt::common_encoding::utf8, mpt::buffer_cast<std::string>(textdata));
			break;
		case encoding::type::locale:
			filetext = mpt::transcode<mpt::ustring>(mpt::logical_encoding::locale, mpt::buffer_cast<std::string>(textdata));
			break;
	}
	return filetext;
}



inline mpt::ustring decode(mpt::const_byte_span filedata) {
	return decode(probe_encoding(filedata), filedata);
}



inline std::vector<std::byte> encode(encoding encoding, const mpt::ustring & text) {
	std::vector<std::byte> result;
	if (encoding.get_header().size() > 0) {
		mpt::append(result, encoding.get_header());
	}
	switch (encoding.type) {
		case encoding::type::utf32be:
			MPT_MAYBE_CONSTANT_IF (mpt::endian_is_big()) {
				std::u32string utf32text = mpt::transcode<std::u32string>(text);
				mpt::append(result, mpt::as_span(reinterpret_cast<const std::byte *>(utf32text.data()), utf32text.size() * sizeof(char32_t)));
			} else {
				std::u32string utf32text = mpt::transcode<std::u32string>(text);
				std::vector<mpt::uint32be> utf32betext;
				utf32betext.resize(utf32text.size());
				std::copy(utf32text.begin(), utf32text.end(), utf32betext.begin());
				mpt::append(result, mpt::as_span(reinterpret_cast<const std::byte *>(utf32betext.data()), utf32betext.size() * sizeof(mpt::uint32be)));
			}
			break;
		case encoding::type::utf32le:
			MPT_MAYBE_CONSTANT_IF (mpt::endian_is_little()) {
				std::u32string utf32text = mpt::transcode<std::u32string>(text);
				mpt::append(result, mpt::as_span(reinterpret_cast<const std::byte *>(utf32text.data()), utf32text.size() * sizeof(char32_t)));
			} else {
				std::u32string utf32text = mpt::transcode<std::u32string>(text);
				std::vector<mpt::uint32le> utf32letext;
				utf32letext.resize(utf32text.size());
				std::copy(utf32text.begin(), utf32text.end(), utf32letext.begin());
				mpt::append(result, mpt::as_span(reinterpret_cast<const std::byte *>(utf32letext.data()), utf32letext.size() * sizeof(mpt::uint32le)));
			}
			break;
		case encoding::type::utf16be:
			MPT_MAYBE_CONSTANT_IF (mpt::endian_is_big()) {
				std::u16string utf16text = mpt::transcode<std::u16string>(text);
				mpt::append(result, mpt::as_span(reinterpret_cast<const std::byte *>(utf16text.data()), utf16text.size() * sizeof(char16_t)));
			} else {
				std::u16string utf16text = mpt::transcode<std::u16string>(text);
				std::vector<mpt::uint16be> utf16betext;
				utf16betext.resize(utf16text.size());
				std::copy(utf16text.begin(), utf16text.end(), utf16betext.begin());
				mpt::append(result, mpt::as_span(reinterpret_cast<const std::byte *>(utf16betext.data()), utf16betext.size() * sizeof(mpt::uint16be)));
			}
			break;
		case encoding::type::utf16le:
			MPT_MAYBE_CONSTANT_IF (MPT_OS_WINDOWS) {
				std::wstring wtext = mpt::transcode<std::wstring>(text);
				mpt::append(result, mpt::as_span(reinterpret_cast<const std::byte *>(wtext.data()), wtext.size() * sizeof(wchar_t)));
			} else MPT_MAYBE_CONSTANT_IF (mpt::endian_is_little()) {
				std::u16string utf16text = mpt::transcode<std::u16string>(text);
				mpt::append(result, mpt::as_span(reinterpret_cast<const std::byte *>(utf16text.data()), utf16text.size() * sizeof(char16_t)));
			} else {
				std::u16string utf16text = mpt::transcode<std::u16string>(text);
				std::vector<mpt::uint16le> utf16letext;
				utf16letext.resize(utf16text.size());
				std::copy(utf16text.begin(), utf16text.end(), utf16letext.begin());
				mpt::append(result, mpt::as_span(reinterpret_cast<const std::byte *>(utf16letext.data()), utf16letext.size() * sizeof(mpt::uint16le)));
			}
			break;
		case encoding::type::utf8:
			mpt::append(result, mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(mpt::transcode<std::string>(mpt::common_encoding::utf8, text))));
			break;
		case encoding::type::locale:
			assert(encoding.header != TextFileEncoding::Header::BOM);
			mpt::append(result, mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(mpt::transcode<std::string>(mpt::logical_encoding::locale, text))));
			break;
	}
	return result;
}



} // namespace textfile



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_TEXTFILE_TEXTFILE_HPP
