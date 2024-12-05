#include "json_loader.h"

#include <sstream>
#include <fstream>

#include <boost/json.hpp>

namespace json_loader {

    namespace json = boost::json;

    void ParseRoads(const boost::json::array& roads_array, model::Map& game_map) {
        for (const auto& road : roads_array) {
            json::object road_obj = road.as_object();

            int x0 = road_obj["x0"].as_int64();
            int y0 = road_obj["y0"].as_int64();

            if (road_obj.contains("x1")) {
                int x1 = road_obj["x1"].as_int64();
                game_map.AddRoad(model::Road(model::Road::HORIZONTAL, { x0, y0 }, x1));
            }
            else if (road_obj.contains("y1")) {
                int y1 = road_obj["y1"].as_int64();
                game_map.AddRoad(model::Road(model::Road::VERTICAL, { x0, y0 }, y1));
            }
        }
    }

    void ParseBuilding(const boost::json::array& buildings_array, model::Map& game_map) {
        for (const auto& building : buildings_array) {
            json::object building_obj = building.as_object();

            int x = building_obj["x"].as_int64();
            int y = building_obj["y"].as_int64();
            int w = building_obj["w"].as_int64();
            int h = building_obj["h"].as_int64();

            game_map.AddBuilding(model::Building({ {x, y}, {w, h} }));
        }
    }

    void ParseOffices(const boost::json::array& offices_array, model::Map& game_map) {
        for (const auto& office : offices_array) {
            json::object office_obj = office.as_object();

            model::Office::Id id(office_obj["id"].as_string().c_str());
            int x = office_obj["x"].as_int64();
            int y = office_obj["y"].as_int64();
            int offsetX = office_obj["offsetX"].as_int64();
            int offsetY = office_obj["offsetY"].as_int64();

            game_map.AddOffice(model::Office(id, { x, y }, { offsetX, offsetY }));
        }
    }

    model::Map ParseMap(const json::object& map_obj) {
        model::Map::Id id(map_obj.at("id").as_string().c_str());
        std::string name = map_obj.at("name").as_string().c_str();

        model::Map game_map(id, name);

        // Road parsing
        if (map_obj.contains("roads")) {
            ParseRoads(map_obj.at("roads").as_array(), game_map);
        }

        // Building parsing
        if (map_obj.contains("buildings")) {
            ParseBuilding(map_obj.at("buildings").as_array(), game_map);
        }

        // Office parsing
        if (map_obj.contains("offices")) {
            ParseOffices(map_obj.at("offices").as_array(), game_map);
        }

        return game_map;
    }

    model::Game LoadGame(const std::filesystem::path& json_path) {
        model::Game game;

        std::ifstream file(json_path);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + json_path.string());
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        std::string json_str = buffer.str();

        // JSON parsing
        json::value json_value = json::parse(json_str);
        json::object json_obj = json_value.as_object();

        if (!json_obj.contains("maps")) {
            throw std::runtime_error("Maps not found in json");
        }

        for (const auto& json_map : json_obj["maps"].as_array()) {
            game.AddMap(ParseMap(json_map.as_object()));
        }

        return game;
    }

}  // namespace json_loader
