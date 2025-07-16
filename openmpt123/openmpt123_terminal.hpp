/*
 * openmpt123_terminal.hpp
 * -----------------------
 * Purpose: libopenmpt command line player
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef OPENMPT123_TERMINAL_HPP
#define OPENMPT123_TERMINAL_HPP

#include "openmpt123_config.hpp"

#include "mpt/terminal/base.hpp"
#include "mpt/terminal/input.hpp"
#include "mpt/terminal/is_terminal.hpp"
#include "mpt/terminal/output.hpp"
#include "mpt/terminal/size.hpp"
#include "mpt/terminal/stdio_manager.hpp"

namespace openmpt123 {

using mpt::terminal::concat_stream;

using mpt::terminal::string_concat_stream;

using mpt::terminal::textout;

template <typename Tstring>
inline concat_stream<Tstring> & lf(concat_stream<Tstring> & s) {
	return s.append(Tstring(1, mpt::char_constants<typename Tstring::value_type>::lf));
}

} // namespace openmpt123

#endif // OPENMPT123_TERMINAL_HPP
