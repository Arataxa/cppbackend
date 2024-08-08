#pragma once

#include <algorithm>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "loot.h"
#include "loot_generator.h"
#include "map.h"
#include "player.h"

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
            GameSession(const Map& map, bool is_random_spawn, loot_gen::LootGenerator loot_generator, double retirement_time);

            std::pair<PlayerToken, size_t> AddPlayer(std::string& name);

            void AddPlayer(PlayerToken token, const Player& player);

            Player* GetPlayer(PlayerToken token);

            std::vector<Player*> GetPlayersVector();

            const std::unordered_map<PlayerToken, Player, PlayerTokenHash>& GetPlayers() const;

            double GetSpeed() const;

            const std::shared_ptr<Map> GetMap() const;

            const Road* GetHorizontalRoad(int y) const;

            const Road* GetVerticalRoad(int x) const;

            void ProcessTick(int time);

            const std::unordered_map<size_t, Loot>& GetLoots() const;

            loot_gen::LootGenerator GetLootGenerator() const;

            void AddLoot(Loot loot);

            std::vector<std::pair<PlayerToken, Player>>& GetInactivePlayers();
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
            const size_t max_bag_capacity_;
            const double retirement_time_;
            std::unordered_map<size_t, Loot> loots_;
            std::vector<std::pair<PlayerToken,Player>> inactive_players_;
        };

        class Game {
        public:
            using Maps = std::vector<Map>;

            explicit Game(bool is_random_spawn, loot_gen::LootGenerator loot_generator, double retirement_time);

            void AddMap(Map map);

            void AddSession(GameSession& session);

            std::pair<PlayerToken, size_t> AddPlayer(const Map* map, std::string& name);

            void AddPlayer(const std::string& map_id, PlayerToken token, const Player& player);

            const Maps& GetMaps() const noexcept;

            const Map* GetMap(const std::string& id) const noexcept;

            Player* GetPlayer(const PlayerToken& token);

            std::vector<Player*> GetPlayersInSession(GameSession& session);

            void ProcessTimeMovement(int time);

            const std::map<std::string, GameSession>& GetSessions() const;

            std::map<std::string, GameSession>& GetSessions();

            bool IsSpawnRandom() const;

            loot_gen::LootGenerator GetLootGenerator() const;

            GameSession& GetSession(const std::string& map_id);

            std::vector<Player>& GetLeavedPlayers();

            double GetRetirementTime() const;

            Game(const Game&) = delete;
            Game& operator=(const Game&) = delete;

            Game(Game&&) noexcept = default;
            Game& operator=(Game&&) noexcept = default;
        private:
            using MapIdToIndex = std::unordered_map<std::string, size_t>;

            std::unordered_map<PlayerToken, Player*, PlayerTokenHash> players_;
            std::map<std::string, GameSession> sessions_;
            std::vector<Map> maps_;
            MapIdToIndex map_id_to_index_;
            loot_gen::LootGenerator loot_generator_;

            const bool is_random_spawn_;
            const double retirement_time_;
            std::vector<Player> inactive_players_;
            std::mutex mutex_;
        };
    } // namespace game
} // namespace application
