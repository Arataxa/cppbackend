#pragma once
#include "database_manager.h"
#include "game.h"
#include "loot_type_info.h"
#include "map.h"
#include "serialization.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <fstream>
#include <iostream>

namespace application {
	using namespace game;
	using namespace map;
	using namespace database;

	class Application {
	public:
		explicit Application(std::shared_ptr<game::Game> game, DatabaseManager&& db_manager, loot_type_info::LootTypeInfo&& type_info, const std::string& state_file, int save_state_period);

		std::pair<PlayerToken, size_t> JoinGame(const Map* map, std::string& name);

		const Map* GetMap(const std::string& id) const noexcept;

		const std::vector<Map>& GetMaps() const noexcept;

		Player* GetPlayer(const PlayerToken& token);

		void ProcessTime(int time);

		void SaveGame();

		void LoadGame();

		const loot_type_info::LootTypeInfo& GetLootTypeInfo() const;

		std::vector<Record> GetRecords(int start, int max_items);

		void SaveLeavedPlayerScore(Player&& player);
	private:
		std::shared_ptr<game::Game> game_;
		DatabaseManager db_manager_;
		loot_type_info::LootTypeInfo loot_type_info_;
		std::string state_file_;
		int save_state_period_;
		int accumulated_time_ = 0;
	};
} // namespace application