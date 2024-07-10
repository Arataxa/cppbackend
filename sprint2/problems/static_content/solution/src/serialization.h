#pragma once

#include <boost/json.hpp>

#include "model.h"

class JsonSerializer {
public:
	static boost::json::object SerializeMap(const model::Map& map) {
        return MapSerialazer::Serialize(map);
	}
private:
    class MapSerialazer {
    public:
        static boost::json::object Serialize(const model::Map& map);
    private:
        static boost::json::array SerializeRoads(const std::vector<model::Road>& roads);

        static boost::json::array SerializeBuildings(const std::vector<model::Building> buildings);

        static boost::json::array SerializeOffices(const std::vector<model::Office> offices);
    };
};