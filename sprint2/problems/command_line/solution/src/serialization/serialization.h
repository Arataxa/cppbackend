#pragma once

#include <boost/json.hpp>

#include "map.h"

namespace game = application::game;
namespace map = game::map;

class JsonSerializer {
public:
	static boost::json::object SerializeMap(const map::Map& map) {
        return MapSerialazer::Serialize(map);
	}
private:
    class MapSerialazer {
    public:
        static boost::json::object Serialize(const map::Map& map);
    private:
        static boost::json::array SerializeRoads(const std::vector<map::Road>& roads);

        static boost::json::array SerializeBuildings(const std::vector<map::Building> buildings);

        static boost::json::array SerializeOffices(const std::vector<map::Office> offices);
    };
};