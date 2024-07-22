#pragma once

#include <boost/json.hpp>

#include "map.h"

namespace game = application::game;
namespace map = game::map;
namespace json = boost::json;


class JsonSerializer {
public:
	static json::object SerializeMap(const map::Map& map, json::array loot_type_info) {
        return MapSerialazer::Serialize(map, std::move(loot_type_info));
	}
private:
    class MapSerialazer {
    public:
        static json::object Serialize(const map::Map& map, json::array&& loot_type_info);
    private:
        static json::array SerializeRoads(const std::vector<map::Road>& roads);

        static json::array SerializeBuildings(const std::vector<map::Building>& buildings);

        static json::array SerializeOffices(const std::vector<map::Office>& offices);
    };
};