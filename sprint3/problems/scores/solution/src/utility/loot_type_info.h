#pragma once

#include <unordered_map>
#include <string>

#include <boost/json.hpp>

namespace json = boost::json;

namespace loot_type_info {
	class LootTypeInfo {
	public:
		LootTypeInfo() {

		}

		template<typename T>
		void AddInfo(const std::string& map_name, T&& info) {
			map_to_info_[map_name] = std::forward<T>(info);
		}

		const json::array& GetInfo(const std::string& map_name) const;

	private:
		std::unordered_map<std::string, json::array> map_to_info_;
	};
}


