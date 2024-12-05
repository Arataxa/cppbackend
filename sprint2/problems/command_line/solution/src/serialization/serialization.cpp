#include "serialization.h"

boost::json::object JsonSerializer::MapSerialazer::Serialize(const map::Map& map) {
    boost::json::array roads = SerializeRoads(map.GetRoads());

    boost::json::array buildings = SerializeBuildings(map.GetBuildings());

    boost::json::array offices = SerializeOffices(map.GetOffices());

    return boost::json::object{
        {"id", static_cast<std::string>(map.GetId())},
        {"name", map.GetName()},
        {"roads", roads},
        {"buildings", buildings},
        {"offices", offices}
    };
}

boost::json::array JsonSerializer::MapSerialazer::SerializeRoads(const std::vector<map::Road>& roads) {
    boost::json::array roads_array;
    for (const auto& road : roads) {
        boost::json::object road_obj;

        auto start = road.GetStart();
        road_obj["x0"] = start.x;
        road_obj["y0"] = start.y;

        if (road.IsHorizontal()) {
            road_obj["x1"] = road.GetEnd().x;
        }
        else {
            road_obj["y1"] = road.GetEnd().y;
        }

        roads_array.push_back(road_obj);
    }

    return roads_array;
}

boost::json::array JsonSerializer::MapSerialazer::SerializeBuildings(const std::vector<map::Building> buildings) {
    boost::json::array buildings_array;
    for (const auto& building : buildings) {
        boost::json::object building_obj;

        auto bounds = building.GetBounds();
        building_obj["x"] = bounds.position.x;
        building_obj["y"] = bounds.position.y;
        building_obj["w"] = bounds.size.width;
        building_obj["h"] = bounds.size.height;

        buildings_array.push_back(building_obj);
    }

    return buildings_array;
}

boost::json::array JsonSerializer::MapSerialazer::SerializeOffices(const std::vector<map::Office> offices) {
    boost::json::array offices_array;
    for (const auto& office : offices) {
        boost::json::object office_obj;

        auto position = office.GetPosition();
        auto offset = office.GetOffset();

        office_obj["id"] = boost::json::string(static_cast<std::string>(office.GetId()));
        office_obj["x"] = position.x;
        office_obj["y"] = position.y;
        office_obj["offsetX"] = offset.dx;
        office_obj["offsetY"] = offset.dy;

        offices_array.push_back(office_obj);
    }

    return offices_array;
}