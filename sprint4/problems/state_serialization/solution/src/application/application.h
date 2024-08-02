#pragma once

#include "game.h"
#include "map.h"
#include "loot_type_info.h"
#include "serialization.h"

#include <boost/archive/text_oarchive.hpp>

#include <iostream>
#include <fstream>

namespace application {
	using namespace game;
	using namespace map;

	class Application {
	public:
		explicit Application(Game&& game, loot_type_info::LootTypeInfo&& type_info, const std::string& state_file, int save_state_period);

		std::pair<PlayerToken, size_t> JoinGame(const Map* map, std::string& name);

		const Map* GetMap(const std::string& id) const noexcept;

		const std::vector<Map>& GetMaps() const noexcept;

		Player* GetPlayer(const PlayerToken& token);

		void ProcessTime(int time);

		void SaveGame();

		const loot_type_info::LootTypeInfo& GetLootTypeInfo() const;

	private:
		Game game_;
		loot_type_info::LootTypeInfo loot_type_info_;
		std::string state_file_;
		int save_state_period_;
		int accumulated_time_;
	};
} // namespace application