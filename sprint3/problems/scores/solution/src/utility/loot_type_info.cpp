#include "loot_type_info.h"

namespace loot_type_info {
	const json::array& LootTypeInfo::GetInfo(const std::string& map_name) const {
		static const json::array empty_array;
		auto it = map_to_info_.find(map_name);
		return it != map_to_info_.end() ? it->second : empty_array;
	}
}