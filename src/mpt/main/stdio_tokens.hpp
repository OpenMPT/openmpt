/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_MAIN_STDIO_TOKENS_HPP
#define MPT_MAIN_STDIO_TOKENS_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace main {



enum class stdio {
	input = 0,
	output = 1,
	error = 2,
};

namespace detail {

class token_factory;

} // namespace detail

template <mpt::main::stdio fd>
struct token final {
private:
	friend class mpt::main::detail::token_factory;
	token() = default;
public:
	token(token && t) = default;
	token(const token & t) = delete;
	token & operator=(token && t) = default;
	token & operator=(const token & t) = delete;
	~token() = default;
};

using stdin_token = token<mpt::main::stdio::input>;
using stdout_token = token<mpt::main::stdio::output>;
using stderr_token = token<mpt::main::stdio::error>;

namespace detail {

class token_factory final {
public:
	token_factory() = default;
	~token_factory() = default;
public:
	template <mpt::main::stdio fd>
	inline mpt::main::token<fd> make() const {
		return mpt::main::token<fd>{};
	}
};

inline mpt::main::stdin_token make_stdin_token() {
	return mpt::main::detail::token_factory{}.template make<mpt::main::stdio::input>();
}

inline mpt::main::stdout_token make_stdout_token() {
	return mpt::main::detail::token_factory{}.template make<mpt::main::stdio::output>();
}

inline mpt::main::stderr_token make_stderr_token() {
	return mpt::main::detail::token_factory{}.template make<mpt::main::stdio::error>();
}

} // namespace detail



} // namespace main



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_MAIN_STDIO_TOKENS_HPP
