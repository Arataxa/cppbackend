#include "json_loader.h"

#include <sstream>
#include <fstream>

namespace json_loader {
    namespace json = boost::json;
    namespace game = application::game;
    namespace map = game::map;
    
    MapParser::MapParser(double default_dog_speed, size_t default_bag_capacity) 
        : default_dog_speed_(default_dog_speed), default_bag_capacity_(default_bag_capacity) {
    }

    map::Map MapParser::Parse(const json::object& map_obj) {
        std::string id(map_obj.at("id").as_string().c_str());
        std::string name = map_obj.at("name").as_string().c_str();

        double dog_speed;
        size_t bag_capacity; 

        {
            auto it = map_obj.find("dogSpeed");
            if (it != map_obj.end()) {
                dog_speed = it->value().as_double();
            }
            else {
                dog_speed = default_dog_speed_;
            }

            it = map_obj.find("bagCapacity");
            if (it != map_obj.end()) {
                bag_capacity = it->value().as_int64();
            }
            else {
                bag_capacity = default_bag_capacity_;
            }
        }

        loot_type_info_ = map_obj.at("lootTypes").as_array();

        unsigned types_count = loot_type_info_.size();

        std::vector<size_t> index_to_score_;

        for (const auto& loot_info : loot_type_info_) {
            index_to_score_.push_back(loot_info.at("value").as_int64());
        }

        map::Map game_map = map::MapBuilder()
            .SetId(id).SetName(name)
            .SetDogSpeed(dog_speed)
            .SetTypesValue(index_to_score_)
            .SetBagCapacity(bag_capacity).Build();

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

        return ParseGameFromJson(buffer.str());
    }
    
    game::Game GameLoader::ParseGameFromJson(const std::string& json_str) {
        json::value json_value = json::parse(json_str);
        json::object json_obj = json_value.as_object();

        if (!json_obj.contains("maps")) {
            throw std::runtime_error("Maps not found in json");
        }

        double default_dog_speed = 1;
        size_t default_bag_capacity = 3;

        {
            auto it = json_obj.find("defaultDogSpeed");
            if (it != json_obj.end()) {
                default_dog_speed = it->value().as_double();
            }

            it = json_obj.find("defaultBagCapacity");
            if (it != json_obj.end()) {
                default_bag_capacity = it->value().as_int64();
            }
        }

        game::Game game(is_random_spawn_, ParseLootGenerator(json_obj));

        MapParser parser(default_dog_speed, default_bag_capacity);

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
