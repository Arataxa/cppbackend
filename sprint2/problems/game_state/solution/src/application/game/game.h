#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <memory>

#include "map.h"
#include "player.h"

namespace application {
    namespace game {
        using namespace map;
        using namespace player;

        class GameSession {
        public:
            GameSession(const Map& map);

            std::pair<PlayerToken, size_t> AddPlayer(std::string& name);

            const Player* GetPlayer(PlayerToken token) const;

            std::vector<const Player*> GetPlayersVector() const;
        private:
            std::unordered_map<PlayerToken, Player, PlayerTokenHash> players_;
            std::shared_ptr<Map> map_;
        };

        class Game {
        public:
            using Maps = std::vector<Map>;

            void AddMap(Map map);

            std::pair<PlayerToken, size_t> AddPlayer(const Map* map, std::string& name);

            const Maps& GetMaps() const noexcept;

            const Map* GetMap(const std::string& id) const noexcept;

            const Player* GetPlayer(const PlayerToken& token) const;

            std::vector<const Player*> GetPlayersInSession(const GameSession& session) const;

        private:
            using MapIdToIndex = std::unordered_map<std::string, size_t>;

            std::unordered_map<PlayerToken, const Player*, PlayerTokenHash> players_;
            std::map<std::string, GameSession> sessions_;
            std::vector<Map> maps_;
            MapIdToIndex map_id_to_index_;
        };
    } // namespace game
} // namespace application
