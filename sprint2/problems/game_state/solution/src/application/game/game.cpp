#include "game.h"

using namespace std::literals;

namespace application {
    namespace game {
        using namespace player;

        GameSession::GameSession(const Map& map) : map_(std::make_shared<Map>(map)) {
        }

        std::pair<PlayerToken, size_t> GameSession::AddPlayer(std::string& name) {

            auto token = PlayerToken::GenerateToken();

            Coordinates coordinates = map_->GetRandomPosition();

            Player player(Dog{ name, players_.size(), coordinates }, *this);
            auto it = players_.emplace(token, std::move(player));

            return { token, it.first->second.GetId() };
        }

        const Player* GameSession::GetPlayer(PlayerToken token) const {
            return &players_.at(token);
        }

        std::vector<const Player*> GameSession::GetPlayersVector() const {
            std::vector<const Player*> result;

            for (const auto& [key, player] : players_) {
                result.push_back(&player);
            }

            return result;
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

        const Player* Game::GetPlayer(const PlayerToken& token) const {
            auto it = players_.find(token);
            if (it != players_.end()) {
                return it->second;
            }
            return nullptr;
        }

        std::vector<const Player*> Game::GetPlayersInSession(const GameSession& session) const {
            return session.GetPlayersVector();
        }

    } // namespace game
} // namespace application