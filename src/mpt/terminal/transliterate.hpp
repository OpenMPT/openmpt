/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_TERMINAL_TRANSLITERATE_HPP
#define MPT_TERMINAL_TRANSLITERATE_HPP

#include "mpt/base/alloc.hpp"
#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/base/utility.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string_transcode/transcode.hpp"

#include <string>
#include <vector>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace terminal {



enum class transliterate_c0 {
	none,
	remove,
	ascii_replacement,
	ascii,
	unicode_replacement,
	unicode,
};

enum class transliterate_del {
	none,
	remove,
	ascii_replacement,
	ascii,
	unicode_replacement,
	unicode,
};

enum class transliterate_c1 {
	none,
	remove,
	ascii_replacement,
	unicode_replacement,
};

struct transliterate_mode {
	transliterate_c0 c0;
	transliterate_del del;
	transliterate_c1 c1;
};

inline mpt::ustring transliterate_control_codes(mpt::ustring str, transliterate_mode mode) {
	mpt::ustring result;
	result.reserve(str.length());
	for (const auto c : str) {
		if (mpt::char_value(c) < 0x20u) {
			switch (mode.c0) {
				case transliterate_c0::none:
					result.push_back(mpt::char_value(c));
					break;
				case transliterate_c0::remove:
					// nothing;
					break;
				case transliterate_c0::ascii_replacement:
					result.push_back(MPT_UCHAR('?'));
					break;
				case transliterate_c0::ascii:
					result.push_back(MPT_UCHAR('^'));
					result.push_back(static_cast<mpt::uchar>(mpt::char_value(c) + 0x40u));
					break;
				case transliterate_c0::unicode_replacement:
					mpt::append(result, mpt::transcode<mpt::ustring>(std::u32string(1, 0xfffdu)));
					break;
				case transliterate_c0::unicode:
					mpt::append(result, mpt::transcode<mpt::ustring>(std::u32string(1, mpt::char_value(c) + 0x2400u)));
					break;
			}
		} else if (mpt::char_value(c) == 0x7fu) {
			switch (mode.del) {
				case transliterate_del::none:
					result.push_back(mpt::char_value(c));
					break;
				case transliterate_del::remove:
					// nothing;
					break;
				case transliterate_del::ascii_replacement:
					result.push_back(MPT_UCHAR('?'));
					break;
				case transliterate_del::ascii:
					result.push_back(MPT_UCHAR('^'));
					result.push_back(static_cast<mpt::uchar>(mpt::char_value(c) - 0x40u));
					break;
				case transliterate_del::unicode_replacement:
					mpt::append(result, mpt::transcode<mpt::ustring>(std::u32string(1, 0xfffdu)));
					break;
				case transliterate_del::unicode:
					mpt::append(result, mpt::transcode<mpt::ustring>(std::u32string(1, 0x2421u)));
					break;
			}
		} else if ((0x80u <= mpt::char_value(c)) && (mpt::char_value(c) <= 0x9f)) {
			switch (mode.c1) {
				case transliterate_c1::none:
					result.push_back(mpt::char_value(c));
					break;
				case transliterate_c1::remove:
					// nothing;
					break;
				case transliterate_c1::ascii_replacement:
					result.push_back(MPT_UCHAR('?'));
					break;
				case transliterate_c1::unicode_replacement:
					mpt::append(result, mpt::transcode<mpt::ustring>(std::u32string(1, 0xfffdu)));
					break;
			}
		} else {
			result.push_back(mpt::char_value(c));
		}
	}
	return result;
}



inline mpt::ustring transliterate_control_codes_multiline(mpt::ustring str, transliterate_mode mode) {
	if (!str.empty()) {
		std::vector<mpt::ustring> lines = mpt::split(str, MPT_USTRING("\n"));
		for (auto & line : lines) {
			line = transliterate_control_codes(line, mode);
		}
		str = mpt::join(lines, MPT_USTRING("\n"));
	}
	return str;
}



} // namespace terminal



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_TERMINAL_TRANSLITERATE_HPP
