#include "serialization.h"

namespace application {
    namespace serialization {
        DogSerialization DogSerialization::FromDog(const Dog& dog) {
            DogSerialization dog_ser;

            dog_ser.name_ = dog.GetName();
            dog_ser.id_ = dog.GetId();
            dog_ser.direction_ = dog.GetDirection();

            auto pos = dog.GetPosition();
            auto speed = dog.GetSpeed();

            dog_ser.position_ = { pos.x, pos.y };
            dog_ser.speed_ = { speed.x, speed.y };

            return dog_ser;
        }

        Dog DogSerialization::ToDog() const {
            Dog dog(name_, id_, Coordinates{ position_.first, position_.second });

            dog.SetDirection(direction_);
            dog.SetSpeed(Speed{ speed_.first, speed_.second });

            return dog;
        }

        LootSerialization LootSerialization::FromLoot(const Loot& loot) {
            LootSerialization loot_ser;

            loot_ser.coordinates_.first = loot.coordinates.x;
            loot_ser.coordinates_.second = loot.coordinates.y;
            loot_ser.type_index_ = loot.type_index;
            loot_ser.id_ = loot.id;
            loot_ser.is_collected_ = loot.is_collected;

            return loot_ser;
        }

        Loot LootSerialization::ToLoot() const {
            Loot loot(Coordinates{ coordinates_.first, coordinates_.second }, type_index_, false);

            loot.id = id_;
            loot.is_collected = is_collected_;

            return loot;
        }

        PlayerSerialization PlayerSerialization::FromPlayer(const Player& player) {
            PlayerSerialization player_ser;

            player_ser.dog_ = DogSerialization::FromDog(player.GetDog());
            player_ser.score_ = player.GetScore();

            for (const auto& loot : player.GetLoots()) {
                player_ser.loots_.emplace_back(LootSerialization::FromLoot(loot));
            }

            return player_ser;
        }

        Player PlayerSerialization::ToPlayer(GameSession& session) const {
            Player player(dog_.ToDog(), session);

            player.SetScore(score_);

            for (const auto& loot : loots_) {
                player.AddLoot(loot.ToLoot());
            }

            return player;
        }

        GameSessionSerialization GameSessionSerialization::FromGameSession(const GameSession& game_session) {
            GameSessionSerialization game_session_ser;

            game_session_ser.time_without_loot_ = game_session.GetLootGenerator().GetTimeWithoutLoot();

            for (const auto& [token, player] : game_session.GetPlayers()) {
                game_session_ser.players_.emplace(token.ToString(), PlayerSerialization::FromPlayer(player));
            }

            for (const auto& [loot_id, loot] : game_session.GetLoots()) {
                game_session_ser.loots_.emplace_back(LootSerialization::FromLoot(loot));
            }

            game_session_ser.map_id = game_session.GetMap()->GetId();

            return game_session_ser;
        }

        GameSession GameSessionSerialization::ToGameSession(const Map& map, bool is_random_spawn, loot_gen::LootGenerator loot_generator) {
            loot_generator.SetTimeWithoutLoot(time_without_loot_);
            GameSession game_session(map, is_random_spawn, loot_generator);

            /*for (const auto& [token_str, player_ser] : players_) {
                game_session.AddPlayer(PlayerToken::FromString(token_str), player_ser.ToPlayer(game_session));
            }*/

            for (const auto& loot : loots_) {
                game_session.AddLoot(loot.ToLoot());
            }

            return game_session;
        }

        GameSerialization GameSerialization::FromGame(const Game& game) {
            GameSerialization game_ser;

            for (const auto& [map_id, session] : game.GetSessions()) {
                game_ser.sessions_.push_back(GameSessionSerialization::FromGameSession(session));
            }

            return game_ser;
        }

        Game GameSerialization::ToGame(Game& game) {
            for (auto& session_ser : sessions_) {
                auto session = session_ser.ToGameSession(*game.GetMap(session_ser.map_id), game.IsSpawnRandom(), game.GetLootGenerator());
                game.AddSession(session);

                for (auto& [token_str, player_ser] : session_ser.players_) {
                    game.AddPlayer(session_ser.map_id, PlayerToken::FromString(token_str), player_ser.ToPlayer(game.GetSession(session_ser.map_id)));
                }
            }

            return game;
        }
    }
}