/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_JSON_JSON_HPP
#define MPT_JSON_JSON_HPP

#include "mpt/base/detect.hpp"
#include "mpt/detect/nlohmann_json.hpp"

#if MPT_DETECTED_NLOHMANN_JSON
#include "mpt/string/types.hpp"
#include "mpt/string_convert/convert.hpp"
#endif // MPT_DETECTED_NLOHMANN_JSON

#if MPT_DETECTED_NLOHMANN_JSON
#include <optional>
#endif // MPT_DETECTED_NLOHMANN_JSON

#if MPT_DETECTED_NLOHMANN_JSON
#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif // MPT_COMPILER_CLANG
#include <nlohmann/json.hpp>
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif // MPT_COMPILER_CLANG
#endif // MPT_DETECTED_NLOHMANN_JSON



namespace nlohmann {
template <>
struct adl_serializer<mpt::ustring> {
	static void to_json(json & j, const mpt::ustring & val) {
		j = mpt::convert<std::string>(mpt::common_encoding::utf8, val);
	}
	static void from_json(const json & j, mpt::ustring & val) {
		val = mpt::convert<mpt::ustring>(mpt::common_encoding::utf8, j.get<std::string>());
	}
};
template <typename Tvalue>
struct adl_serializer<std::map<mpt::ustring, Tvalue>> {
	static void to_json(json & j, const std::map<mpt::ustring, Tvalue> & val) {
		std::map<std::string, Tvalue> utf8map;
		for (const auto & value : val)
		{
			utf8map[mpt::convert<std::string>(mpt::common_encoding::utf8, value.first)] = value.second;
		}
		j = std::move(utf8map);
	}
	static void from_json(const json & j, std::map<mpt::ustring, Tvalue> & val) {
		std::map<std::string, Tvalue> utf8map = j.get<std::map<std::string, Tvalue>>();
		std::map<mpt::ustring, Tvalue> result;
		for (const auto & value : utf8map)
		{
			result[mpt::convert<mpt::ustring>(mpt::common_encoding::utf8, value.first)] = value.second;
		}
		val = std::move(result);
	}
};
template <typename Tvalue>
struct adl_serializer<std::optional<Tvalue>> {
	static void to_json(json & j, const std::optional<Tvalue> & val) {
		j = (val ? json{*val} : json{nullptr});
	}
	static void from_json(const json & j, std::optional<Tvalue> & val) {
		if (!j.is_null())
		{
			val = j.get<Tvalue>();
		} else
		{
			val = std::nullopt;
		}
	}
};
} // namespace nlohmann



#endif // MPT_JSON_JSON_HPP
