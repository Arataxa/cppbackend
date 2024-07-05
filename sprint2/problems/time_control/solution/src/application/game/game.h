#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <memory>
#include <thread>
#include <vector>
#include <future>

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

            Player* GetPlayer(PlayerToken token);

            std::vector<Player*> GetPlayersVector();

            double GetSpeed() const;

            const Map* GetMap() const {
                return map_.get();
            }

            const Road* GetHorizontalRoad(int y) const {
                auto it = horizontal_roads_.find(y);

                if (it == horizontal_roads_.end()) {
                    return nullptr;
                }

                return it->second;
            }

            const Road* GetVerticalRoad(int x) const {
                auto it = vertical_roads_.find(x);

                if (it == vertical_roads_.end()) {
                    return nullptr;
                }

                return it->second;
            }

            void ProcessTimeMovement(int time) {
                std::vector<std::future<void>> futures;

                for (auto& player : players_) {
                    // Запускаем Move асинхронно для каждого игрока
                    futures.push_back(std::async(std::launch::async, &Player::Move, &player.second, time));
                }

                // Ждем завершения всех асинхронных вызовов
                for (auto& future : futures) {
                    future.get();
                }
            }

        private:
            std::shared_ptr<Map> map_;
            std::unordered_map<int, const Road*> horizontal_roads_;
            std::unordered_map<int, const Road*> vertical_roads_;
            std::unordered_map<PlayerToken, Player, PlayerTokenHash> players_;
        };

        class Game {
        public:
            using Maps = std::vector<Map>;

            void AddMap(Map map);

            std::pair<PlayerToken, size_t> AddPlayer(const Map* map, std::string& name);

            const Maps& GetMaps() const noexcept;

            const Map* GetMap(const std::string& id) const noexcept;

            Player* GetPlayer(const PlayerToken& token);

            std::vector<Player*> GetPlayersInSession(GameSession& session);

            void ProcessTimeMovement(int time) {
                std::vector<std::future<void>> futures;

                for (auto& session : sessions_) {
                    futures.push_back(std::async(std::launch::async, &GameSession::ProcessTimeMovement, session.second, time));
                }

                for (auto& future : futures) {
                    future.get();
                }
            }

        private:
            using MapIdToIndex = std::unordered_map<std::string, size_t>;

            std::unordered_map<PlayerToken, Player*, PlayerTokenHash> players_;
            std::map<std::string, GameSession> sessions_;
            std::vector<Map> maps_;
            MapIdToIndex map_id_to_index_;
        };
    } // namespace game
} // namespace application
