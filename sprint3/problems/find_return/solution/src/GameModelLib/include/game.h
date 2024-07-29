#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <memory>
#include <thread>
#include <vector>
#include <list>
#include <thread>
#include <future>
#include <algorithm>
#include <functional>
#include <variant>

#include "loot_generator.h"
#include "map.h"
#include "player.h"
#include "loot.h"

namespace application {
    namespace game {
        using namespace map;
        using namespace player;
        using namespace loot;



        struct GatheringEvent {
            size_t loot_id;
        };

        struct BaseEvent {

        };

        struct InteractionEvent {
            std::variant<GatheringEvent, BaseEvent> event;
            Player* player;
            double time;
        };

        class GameSession {
        public:
            GameSession(const Map& map, bool is_random_spawn, loot_gen::LootGenerator loot_generator);

            std::pair<PlayerToken, size_t> AddPlayer(std::string& name);

            Player* GetPlayer(PlayerToken token);

            std::vector<Player*> GetPlayersVector();

            double GetSpeed() const;

            const std::shared_ptr<Map> GetMap() const;

            const Road* GetHorizontalRoad(int y) const;

            const Road* GetVerticalRoad(int x) const;

            void ProcessTick(int time);

            const std::unordered_map<size_t, Loot>& GetLoots() const;
        private:
            void ProcessTimeMovement(int time);

            void GenerateLoot(int time);

            void ProcessEvents(std::vector<InteractionEvent>& events);

            std::vector<InteractionEvent> CollectEvents(double time);

            std::shared_ptr<Map> map_;
            bool is_random_spawn_;
            loot_gen::LootGenerator loot_generator_;
            std::unordered_map<int, const Road*> horizontal_roads_;
            std::unordered_map<int, const Road*> vertical_roads_;
            std::unordered_map<PlayerToken, Player, PlayerTokenHash> players_;

            std::unordered_map<size_t, Loot> loots_;
        };

        class Game {
        public:
            using Maps = std::vector<Map>;

            explicit Game(bool is_random_spawn, loot_gen::LootGenerator loot_generator);

            void AddMap(Map map);

            std::pair<PlayerToken, size_t> AddPlayer(const Map* map, std::string& name);

            const Maps& GetMaps() const noexcept;

            const Map* GetMap(const std::string& id) const noexcept;

            Player* GetPlayer(const PlayerToken& token);

            std::vector<Player*> GetPlayersInSession(GameSession& session);

            void ProcessTimeMovement(int time);

        private:
            using MapIdToIndex = std::unordered_map<std::string, size_t>;

            std::unordered_map<PlayerToken, Player*, PlayerTokenHash> players_;
            std::map<std::string, GameSession> sessions_;
            std::vector<Map> maps_;
            MapIdToIndex map_id_to_index_;
            loot_gen::LootGenerator loot_generator_;

            bool is_random_spawn_;
        };
    } // namespace game
} // namespace application
