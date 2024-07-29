#pragma once

#include <filesystem>
#include <boost/json.hpp>
#include <loot_type_info.h>

#include "game.h"

namespace json_loader {
	namespace json = boost::json;
	namespace game = application::game;
	namespace map = game::map;

	class MapParser {
	public:
		MapParser(double default_dog_speed);

		map::Map Parse(const json::object& map_obj);

		json::array GetLootTypeInfo() const;
	private:
		void ParseRoads(const json::array& roads_array, map::Map& game_map) const;

		void ParseBuilding(const json::array& buildings_array, map::Map& game_map)const ;

		void ParseOffices(const json::array& offices_array, map::Map& game_map)const ;

		json::array loot_type_info_;
		double default_dog_speed_;
	};

	class GameLoader {
	public:
		GameLoader(bool is_random_spawn);

		game::Game Load(const std::filesystem::path& json_path);

		loot_type_info::LootTypeInfo GetLootTypeInfo() const;
	private:
		loot_gen::LootGenerator ParseLootGenerator(const json::object& json_obj) const;
		bool is_random_spawn_;
		loot_type_info::LootTypeInfo loot_type_info_;
	};
}  // namespace json_loader
