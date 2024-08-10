#pragma once

#include <sstream>
#include <string>
#include <iomanip>
#include <random>
#include <map>
#include <cmath>
#include <mutex>
#include <thread>
#include <list>

#include "dog.h"
#include "loot.h"

namespace application {
    namespace game {
        class GameSession;
        using namespace loot;

        namespace player {
            struct PlayerToken {
                uint64_t part_1;
                uint64_t part_2;

                std::string ToString() const;

                bool operator==(const PlayerToken& other) const;

                bool operator<(const PlayerToken& other) const;

                static PlayerToken GenerateToken();

                static PlayerToken FromString(const std::string& str);

            private:
                static std::mt19937_64 rng1;
                static std::mt19937_64 rng2;
                static std::uniform_int_distribution<uint64_t> dist;
            };

            struct PlayerTokenHash {
                std::size_t operator()(const PlayerToken& token) const {
                    return std::hash<uint64_t>{}(token.part_1) ^ std::hash<uint64_t>{}(token.part_2);
                }
            };

            class Player {
            public:
                static constexpr double width = 0.6;

                Player(Dog dog, GameSession& session);

                size_t GetId() const;

                std::string GetName() const;

                GameSession* GetSession();

                const GameSession* GetSession() const;

                Coordinates GetPosition() const;

                Speed GetSpeed() const;

                Direction GetDirection() const;

                void SetDirection(Direction direction);

                void ChangeSpeed();

                Coordinates Move(double time);

                size_t GetBagCapacity() const;

                void AddLoot(Loot loot);

                const std::vector<Loot>& GetLoots() const;

                void CountItemValue(const std::vector<size_t>& index_to_value_);

                size_t GetScore() const;

                void SetScore(size_t score);

                const Dog& GetDog() const;
            private:

                Coordinates ProccessVerticalMovement(double time);

                Coordinates ProccessHorizontalMovement(double time);

                Dog dog_;
                GameSession& session_;
                size_t score_ = 0;

                std::vector<Loot> loots_;
            };
        } // namespace player
    } // namespace game
} // namespace application