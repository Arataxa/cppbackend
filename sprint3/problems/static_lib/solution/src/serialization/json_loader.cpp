#include "json_loader.h"

#include <sstream>
#include <fstream>

namespace json_loader {
    namespace json = boost::json;
    namespace game = application::game;
    namespace map = game::map;
    
    MapParser::MapParser(double default_dog_speed) : default_dog_speed_(default_dog_speed) {
    }

    map::Map MapParser::Parse(const json::object& map_obj) {
        std::string id(map_obj.at("id").as_string().c_str());
        std::string name = map_obj.at("name").as_string().c_str();
        double dog_speed = default_dog_speed_;

        if (map_obj.find("dogSpeed") != map_obj.end()) {
            dog_speed = map_obj.at("dogSpeed").as_double();
        }

        loot_type_info_ = map_obj.at("lootTypes").as_array();

        unsigned types_count = loot_type_info_.size();

        map::Map game_map(id, name, dog_speed, types_count);

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

    json::array MapParser::GetLootTypeInfo() const {
        return loot_type_info_;
    }

    void MapParser::ParseRoads(const boost::json::array& roads_array, map::Map& game_map) const {
        for (const auto& road : roads_array) {
            json::object road_obj = road.as_object();

            int x0 = road_obj["x0"].as_int64();
            int y0 = road_obj["y0"].as_int64();

            if (road_obj.contains("x1")) {
                int x1 = road_obj["x1"].as_int64();
                game_map.AddRoad(map::Road(map::Road::HORIZONTAL, { x0, y0 }, x1));
            }
            else if (road_obj.contains("y1")) {
                int y1 = road_obj["y1"].as_int64();
                game_map.AddRoad(map::Road(map::Road::VERTICAL, { x0, y0 }, y1));
            }
        }
    }

    void MapParser::ParseBuilding(const boost::json::array& buildings_array, map::Map& game_map) const {
        for (const auto& building : buildings_array) {
            json::object building_obj = building.as_object();

            int x = building_obj["x"].as_int64();
            int y = building_obj["y"].as_int64();
            int w = building_obj["w"].as_int64();
            int h = building_obj["h"].as_int64();

            game_map.AddBuilding(map::Building({ {x, y}, {w, h} }));
        }
    }

    void MapParser::ParseOffices(const boost::json::array& offices_array, map::Map& game_map) const {
        for (const auto& office : offices_array) {
            json::object office_obj = office.as_object();

            std::string id(office_obj["id"].as_string().c_str());
            int x = office_obj["x"].as_int64();
            int y = office_obj["y"].as_int64();
            int offsetX = office_obj["offsetX"].as_int64();
            int offsetY = office_obj["offsetY"].as_int64();

            game_map.AddOffice(map::Office(id, { x, y }, { offsetX, offsetY }));
        }
    }

    GameLoader::GameLoader(bool is_random_spawn) : is_random_spawn_(is_random_spawn) {
    }

    game::Game GameLoader::Load(const std::filesystem::path& json_path) {
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
        double default_dog_speed = 1;

        if (json_obj.find("defaultDogSpeed") != json_obj.end()) {
            default_dog_speed = json_obj.at("defaultDogSpeed").as_double();
        }

        if (!json_obj.contains("maps")) {
            throw std::runtime_error("Maps not found in json");
        }

        game::Game game(is_random_spawn_, ParseLootGenerator(json_obj));

        MapParser parser(default_dog_speed);

        for (const auto& json_map : json_obj["maps"].as_array()) {
            auto new_map = parser.Parse(json_map.as_object());
            game.AddMap(new_map);

            loot_type_info_.AddInfo(new_map.GetName(), std::move(parser.GetLootTypeInfo()));
        }

        return game;
    }

    loot_gen::LootGenerator GameLoader::ParseLootGenerator(const json::object& json_obj) const {
        json::object json_settings = json_obj.at("lootGeneratorConfig").as_object();

        double period = json_settings.at("period").as_double();
        double probability = json_settings.at("probability").as_double();

        loot_gen::LootGenerator::TimeInterval period_ms = std::chrono::milliseconds(static_cast<int>(period * 1000));

        return loot_gen::LootGenerator(period_ms, probability);
    }

    loot_type_info::LootTypeInfo GameLoader::GetLootTypeInfo() const {
        return loot_type_info_;
    }
}  // namespace json_loader
