#include "game.h"

using namespace std::literals;

namespace application {
    namespace game {
        using namespace player;
        using namespace map;

        GameSession::GameSession(const Map& map) : map_(std::make_shared<Map>(map)) {
            const std::vector<Road>& roads = map_->GetRoads();

            for (const auto& road : roads) {
                if (road.IsHorizontal()) {
                    horizontal_roads_.emplace(road.GetStart().y, &road );
                }
                else {
                    vertical_roads_.emplace(road.GetStart().x, &road);
                }
            }
        }

        std::pair<PlayerToken, size_t> GameSession::AddPlayer(std::string& name) {

            auto token = PlayerToken::GenerateToken();

            Coordinates coordinates = map_->GetRandomPosition();

            Player player(Dog{ name, players_.size(), coordinates }, *this);
            auto it = players_.emplace(token, std::move(player));

            return { token, it.first->second.GetId() };
        }

        Player* GameSession::GetPlayer(PlayerToken token) {
            return &players_.at(token);
        }

        std::vector<Player*> GameSession::GetPlayersVector() {
            std::vector<Player*> result;

            for (auto& [key, player] : players_) {
                result.push_back(&player);
            }

            return result;
        }

        double GameSession::GetSpeed() const {
            return map_->GetSpeed();
        }

        const Map* GameSession::GetMap() const {
            return map_.get();
        }

        void GameSession::ProcessTimeMovement(double time) {
            boost::asio::thread_pool pool(std::thread::hardware_concurrency());
            for (auto& player : players_) {
                boost::asio::post(pool, [&player, time]() {
                    player.second.Move(time);
                    });
            }

            pool.join();
        }

        const Road* GameSession::GetHorizontalRoad(int y) const {
            auto it = horizontal_roads_.find(y);

            if (it == horizontal_roads_.end()) {
                return nullptr;
            }

            return it->second;
        }

        const Road* GameSession::GetVerticalRoad(int x) const {
            auto it = vertical_roads_.find(x);

            if (it == vertical_roads_.end()) {
                return nullptr;
            }

            return it->second;
        }

        void Game::AddMap(Map map) {
            const size_t index = maps_.size();
            if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
                throw std::invalid_argument("Map with id "s + map.GetId() + " already exists"s);
            }
            else {
                try {
                    maps_.emplace_back(std::move(map));
                }
                catch (...) {
                    map_id_to_index_.erase(it);
                    throw;
                }
            }
        }

        const Game::Maps& Game::GetMaps() const noexcept {
            return maps_;
        }

        const Map* Game::GetMap(const std::string& id) const noexcept {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
                return &maps_.at(it->second);
            }
            return nullptr;
        }

        std::pair<PlayerToken, size_t> Game::AddPlayer(const Map* map, std::string& name) {
            auto it = sessions_.find(map->GetId());

            if (it == sessions_.end()) {
                GameSession new_session(*map);
                it = sessions_.emplace(map->GetId(), std::move(new_session)).first;
            }

            auto data = it->second.AddPlayer(name);

            players_.emplace(data.first, it->second.GetPlayer(data.first));

            return data;
        }

        Player* Game::GetPlayer(const PlayerToken& token) {
            auto it = players_.find(token);
            if (it != players_.end()) {
                return it->second;
            }
            return nullptr;
        }

        std::vector<Player*> Game::GetPlayersInSession(GameSession& session) {
            return session.GetPlayersVector();
        }

        void Game::ProcessTimeMovement(double time) {
            boost::asio::thread_pool pool(std::thread::hardware_concurrency());
            for (auto& session : sessions_) {
                boost::asio::post(pool, [&session, time]() {
                    session.second.ProcessTimeMovement(time);
                    });
            }

            pool.join();
        }

    } // namespace game
} // namespace application