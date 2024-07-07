#pragma once

#include <sstream>
#include <string>
#include <iomanip>
#include <random>

#include "dog.h"

namespace application {
    namespace game {
        class GameSession;

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
                Player(Dog dog, GameSession& session);

                size_t GetId() const;

                std::string GetName() const;

                const GameSession* GetSession() const;

                Coordinates GetPosition() const;

                Speed GetSpeed() const;

                Direction GetDirection() const;
            private:
                Dog dog_;
                GameSession& session_;
            };
        } // namespace player
    } // namespace game
} // namespace application