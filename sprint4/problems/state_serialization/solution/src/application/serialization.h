#pragma once

#include "game.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "iostream"

namespace application {
	using namespace game;
	using namespace map;

    namespace serialization {
        class DogSerialization {
        public:
            static DogSerialization FromDog(const Dog& dog);

            Dog ToDog() const;

            template <typename Archive>
            void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
                ar& name_;
                ar& id_;
                ar& position_;
                ar& direction_;
                ar& speed_;
            }
        private:
            std::string name_;
            size_t id_;
            std::pair<double, double> position_;
            Direction direction_;
            std::pair<double, double> speed_;
        };

        class LootSerialization {
        public:
            static LootSerialization FromLoot(const Loot& loot);

            Loot ToLoot() const;

            template <typename Archive>
            void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
                ar& coordinates_;
                ar& type_index_;
                ar& id_;
                ar& is_collected_;
            }
        private:
            std::pair<double, double> coordinates_;
            size_t type_index_;
            size_t id_;
            bool is_collected_;
        };

        class PlayerSerialization {
        public:
            static PlayerSerialization FromPlayer(const Player& player);

            Player ToPlayer(GameSession& session) const;

            template <typename Archive>
            void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
                ar& dog_;
                ar& score_;
                ar& loots_;
            }
        private:
            DogSerialization dog_;
            size_t score_;
            std::vector<LootSerialization> loots_;
        };

        class GameSessionSerialization {
        public:
            static GameSessionSerialization FromGameSession(const GameSession& game_session);

            GameSession ToGameSession(const Map& map, bool is_random_spawn, loot_gen::LootGenerator loot_generator);

            template <typename Archive>
            void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
                ar& players_;
                ar& loots_;
                ar& map_id;
            }

            std::string map_id;
            std::unordered_map<std::string, PlayerSerialization> players_;
        private:
            std::chrono::milliseconds time_without_loot_;
            std::vector<LootSerialization> loots_;

        };

        class GameSerialization {
        public:
            static GameSerialization FromGame(const Game& game);

            Game ToGame(Game& game);

            template <typename Archive>
            void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
                ar& sessions_;
            }

        private:
            std::vector<GameSessionSerialization> sessions_;
        };
    }
} // namespace application