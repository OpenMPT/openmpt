
#pragma once

#include "BuildSettings.h"


#ifdef MPT_WITH_NLOHMANNJSON
#include <optional>
#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif // MPT_COMPILER_CLANG
#include "nlohmann-json/include/nlohmann/json.hpp"
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif // MPT_COMPILER_CLANG
#endif // MPT_WITH_NLOHMANNJSON

#ifdef MPT_WITH_NLOHMANNJSON

namespace nlohmann
{
	template <>
	struct adl_serializer<OPENMPT_NAMESPACE::mpt::ustring>
	{
		static void to_json(json& j, const OPENMPT_NAMESPACE::mpt::ustring& val)
		{
			j = OPENMPT_NAMESPACE::mpt::ToCharset(OPENMPT_NAMESPACE::mpt::Charset::UTF8, val);
		}
		static void from_json(const json& j, OPENMPT_NAMESPACE::mpt::ustring& val)
		{
			val = OPENMPT_NAMESPACE::mpt::ToUnicode(OPENMPT_NAMESPACE::mpt::Charset::UTF8, j.get<std::string>());
		}
	};
	template <typename Tvalue>
	struct adl_serializer<std::map<OPENMPT_NAMESPACE::mpt::ustring, Tvalue>>
	{
		static void to_json(json& j, const std::map<OPENMPT_NAMESPACE::mpt::ustring, Tvalue>& val)
		{
			std::map<std::string, Tvalue> utf8map;
			for(const auto &value : val)
			{
				utf8map[OPENMPT_NAMESPACE::mpt::ToCharset(OPENMPT_NAMESPACE::mpt::Charset::UTF8, value.first)] = value.second;
			}
			j = std::move(utf8map);
		}
		static void from_json(const json& j, std::map<OPENMPT_NAMESPACE::mpt::ustring, Tvalue>& val)
		{
			std::map<std::string, Tvalue> utf8map = j.get<std::map<std::string, Tvalue>>();
			std::map<OPENMPT_NAMESPACE::mpt::ustring, Tvalue> result;
			for(const auto &value : utf8map)
			{
				result[OPENMPT_NAMESPACE::mpt::ToUnicode(OPENMPT_NAMESPACE::mpt::Charset::UTF8, value.first)] = value.second;
			}
			val = std::move(result);
		}
	};
	template <typename Tvalue>
	struct adl_serializer<std::optional<Tvalue>>
	{
		static void to_json(json& j, const std::optional<Tvalue>& val)
		{
			j = (val ? json{*val} : json{nullptr});
		}
		static void from_json(const json& j, std::optional<Tvalue>& val)
		{
			if(!j.is_null())
			{
				val = j.get<Tvalue>();
			} else
			{
				val = std::nullopt;
			}
		}
	};
} // namespace nlohmann

#endif // MPT_WITH_NLOHMANNJSON


OPENMPT_NAMESPACE_BEGIN


#ifdef MPT_WITH_NLOHMANNJSON

namespace JSON {

	using value = nlohmann::json;

	inline std::string serialize(const nlohmann::json &src)
	{
		return src.dump(4);
	}
	inline nlohmann::json deserialize(const std::string &str)
	{
		return nlohmann::json::parse(str);
	}

	template <typename T>
	inline void enc(JSON::value &j, const T &val)
	{
		j = val;
	}
	template <typename T>
	inline void dec(T &val, const JSON::value &j)
	{
		val = j.get<T>();
	}

} // namespace JSON

#endif // MPT_WITH_NLOHMANNJSON


OPENMPT_NAMESPACE_END
