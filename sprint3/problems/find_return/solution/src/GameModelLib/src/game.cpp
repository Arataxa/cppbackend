#include "game.h"

using namespace std::literals;

namespace application {
    namespace game {
        using namespace player;
        using namespace map;

        GameSession::GameSession(const Map& map, bool is_random_spawn, loot_gen::LootGenerator loot_generator) 
            : map_(std::make_shared<Map>(map)), is_random_spawn_(is_random_spawn), loot_generator_(std::move(loot_generator)) {
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

            Coordinates coordinates;

            if (is_random_spawn_) {
                coordinates = std::move(map_->GetRandomPosition());
            }
            else {
                coordinates = std::move(map_->GetStartPosition());
            }

            Player player(Dog{ name, players_.size(), coordinates }, *this, map_->GetBagCapacity());
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

        const std::shared_ptr<Map> GameSession::GetMap() const {
            return map_;
        }

        void GameSession::ProcessTick(int time) {
            GenerateLoot(time);
            ProcessTimeMovement(time);
        }

        void GameSession::GenerateLoot(int time) {
            auto time_delta = std::chrono::milliseconds(time);
            unsigned loot_count = loots_.size();
            unsigned looter_count = players_.size();

            unsigned new_loot_count = loot_generator_.Generate(time_delta, loot_count, looter_count);

            for (unsigned i = 0; i < new_loot_count; i++) {
                Loot new_loot(map_->GetRandomPosition(), GetRandomInteger(map_->GetLootTypesCount() - 1));
                loots_.emplace(new_loot.id, new_loot);
            }
        }

        void GameSession::ProcessTimeMovement(int time) {
            double time_in_second = time / 1000.0;
            
            auto collected_events = CollectEvents(time_in_second);
            ProcessEvents(collected_events);
        }

        std::vector<InteractionEvent> GameSession::CollectEvents(double time) {
            std::vector<InteractionEvent> events;

            for (auto& [token, player] : players_) {
                auto old_coordinates = player.GetPosition();
                auto new_coordinates = player.Move(time);

                if (old_coordinates == new_coordinates) {
                    continue;
                }
                
                for (auto& [id, loot] : loots_) {
                    auto collect_result = TryCollectPoint(old_coordinates, new_coordinates, loot.coordinates);

                    if (collect_result.IsCollected(Player::width / 2)) {
                        GatheringEvent gathering_event{ .loot_id = loot.id };

                        InteractionEvent interaction_event{.event = gathering_event,
                            .player = &player,
                            .time = collect_result.proj_ratio};

                        events.push_back(interaction_event);
                    }
                }              

                for (const auto& office : map_->GetOffices()) {
                    auto pos = office.GetPosition();
                    Coordinates office_coordinates{.x = (double)pos.x, .y = (double)pos.y};
                    auto collect_result = TryCollectPoint(old_coordinates, new_coordinates, office_coordinates);

                    if (collect_result.IsCollected((Player::width / 2) + (Office::width / 2))) {
                        BaseEvent base_event;

                        InteractionEvent interaction_event{ .event = base_event,
                            .player = &player,
                            .time = collect_result.proj_ratio };
                        
                        events.push_back(interaction_event);
                    }
                }
            }

            std::sort(events.begin(), events.end(),
                [](const InteractionEvent& e_l, const InteractionEvent& e_r) {
                    return e_l.time < e_r.time;
                });

            return events;
        }

        void GameSession::ProcessEvents(std::vector<InteractionEvent>& interaction_events) {
            auto max_bag_capacity = map_->GetBagCapacity();

            for (const auto& interaction_event : interaction_events) {
                auto& player = interaction_event.player;

                if (std::holds_alternative<GatheringEvent>(interaction_event.event)) {
                    if (interaction_event.player->GetBagCapacity() == max_bag_capacity) {
                        continue;
                    }

                    auto gathering_event = std::get<GatheringEvent>(interaction_event.event);

                    auto it = loots_.find(gathering_event.loot_id);
                    if (it != loots_.end()) {
                        interaction_event.player->AddLoot(it->second);
                        loots_.erase(it);
                    }
                }
                else if (std::holds_alternative<BaseEvent>(interaction_event.event)) {
                    interaction_event.player->ClearBag();
                }
            }
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

        const std::unordered_map<size_t, Loot>& GameSession::GetLoots() const {
            return loots_;
        }

        Game::Game(bool is_random_spawn, loot_gen::LootGenerator loot_generator) 
            : is_random_spawn_(is_random_spawn), loot_generator_(loot_generator) {

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
                GameSession new_session(*map, is_random_spawn_, loot_generator_);
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

        void Game::ProcessTimeMovement(int time) {
            std::vector<std::future<void>> futures;
            std::size_t num_threads = std::thread::hardware_concurrency();
            futures.reserve(sessions_.size());

            for (auto& session : sessions_) {
                futures.emplace_back(std::async(std::launch::async, [&session, time]() {
                    session.second.ProcessTick(time);
                    }));
            }

            // Ожидание завершения всех задач
            for (auto& future : futures) {
                future.get();
            }
        }

    } // namespace game
} // namespace application