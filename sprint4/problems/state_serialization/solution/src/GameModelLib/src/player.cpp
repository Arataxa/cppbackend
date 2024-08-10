#include "player.h"
#include "game.h"

#include <iostream>

namespace application {
    namespace game {
        namespace player {
            std::string PlayerToken::ToString() const {
                std::ostringstream oss;
                oss << std::hex << std::setfill('0') << std::setw(16) << part_1
                    << std::setw(16) << part_2;
                return oss.str();
            }

            bool PlayerToken::operator==(const PlayerToken& other) const {
                return part_1 == other.part_1 && part_2 == other.part_2;
            }

            bool PlayerToken::operator<(const PlayerToken& other) const {
                return std::tie(part_1, part_2) < std::tie(other.part_1, other.part_2);
            }

            PlayerToken PlayerToken::GenerateToken() {
                std::random_device rd;
                rng1.seed(rd());
                rng2.seed(rd());

                return { dist(rng1), dist(rng2) };
            }

            PlayerToken PlayerToken::FromString(const std::string& str) {
                if (str.size() != 32) {
                    throw std::invalid_argument("Invalid token string length");
                }

                PlayerToken token;
                token.part_1 = std::stoull(str.substr(0, 16), nullptr, 16);
                token.part_2 = std::stoull(str.substr(16, 16), nullptr, 16);
                return token;
            }

            std::mt19937_64 PlayerToken::rng1;
            std::mt19937_64 PlayerToken::rng2;
            std::uniform_int_distribution<uint64_t> PlayerToken::dist;

            Player::Player(Dog dog, GameSession& session)
                : dog_(dog), session_(session) {
            }

            size_t Player::GetId() const {
                return dog_.GetId();
            }

            std::string Player::GetName() const {
                return dog_.GetName();
            }

            GameSession* Player::GetSession() {
                return &session_;
            }

            const GameSession* Player::GetSession() const {
                return &session_;
            }

            Coordinates Player::GetPosition() const {
                return dog_.GetPosition();
            }

            Speed Player::GetSpeed() const {
                return dog_.GetSpeed();
            }

            Direction Player::GetDirection() const {
                return dog_.GetDirection();
            }

            void Player::SetDirection(Direction direction) {
                dog_.SetDirection(direction);
                ChangeSpeed();
            }

            void Player::ChangeSpeed() {
                dog_.ChangeSpeed(session_.GetSpeed());
            }

            Coordinates Player::ProccessVerticalMovement(double time) {
                Coordinates new_coordinates;

                Coordinates dog_coordinates = dog_.GetPosition();
                Speed speed = dog_.GetSpeed();
                Direction direction = dog_.GetDirection();

                const auto* horizontal_road = session_.GetHorizontalRoad(std::round(dog_coordinates.y));
                const auto* vertical_road = session_.GetVerticalRoad(std::round(dog_coordinates.x));

                double moving_distance = speed.y * time;
                double limit_y;

                if (vertical_road) {
                    auto start = vertical_road->GetStart();
                    auto end = vertical_road->GetEnd();

                    new_coordinates.x = dog_coordinates.x;

                    if (direction == Direction::NORTH) {
                        limit_y = std::min(start.y, end.y) - 0.4;
                    }
                    else {
                        limit_y = std::max(start.y, end.y) + 0.4;
                    }
                }
                else {
                    auto start = horizontal_road->GetStart();

                    new_coordinates.x = dog_coordinates.x;

                    if (direction == Direction::NORTH) {
                        limit_y = start.y - 0.4;
                    }
                    else {
                        limit_y = start.y + 0.4;
                    }
                }

                double new_position = dog_coordinates.y + moving_distance;

                if (direction == Direction::SOUTH) {
                    new_coordinates.y = (new_position > limit_y) ? limit_y : new_position;
                }
                else {
                    new_coordinates.y = (new_position < limit_y) ? limit_y : new_position;
                }

                if (new_coordinates.y == limit_y) {
                    dog_.SetSpeed({ 0.0, 0.0 });
                }

                return new_coordinates;
            }

            Coordinates Player::ProccessHorizontalMovement(double time) {
                Coordinates new_coordinates;

                Coordinates dog_coordinates = dog_.GetPosition();
                Speed speed = dog_.GetSpeed();
                Direction direction = dog_.GetDirection();

                const auto* horizontal_road = session_.GetHorizontalRoad(std::round(dog_coordinates.y));
                const auto* vertical_road = session_.GetVerticalRoad(std::round(dog_coordinates.x));

                double moving_distance = speed.x * time;
                double limit_x;

                if (horizontal_road) {
                    auto start = horizontal_road->GetStart();
                    auto end = horizontal_road->GetEnd();

                    new_coordinates.y = dog_coordinates.y;

                    if (direction == Direction::EAST) {
                        limit_x = std::max(start.x, end.x) + 0.4;
                    }
                    else {
                        limit_x = std::min(start.x, end.x) - 0.4;
                    }
                }
                else {
                    auto start = vertical_road->GetStart();

                    new_coordinates.y = dog_coordinates.y;

                    if (direction == Direction::EAST) {
                        limit_x = start.x + 0.4;
                    }
                    else {
                        limit_x = start.x - 0.4;
                    }
                }

                double new_position = dog_coordinates.x + moving_distance;

                if (direction == Direction::EAST) {
                    new_coordinates.x = (new_position > limit_x) ? limit_x : new_position;
                }
                else {
                    new_coordinates.x = (new_position < limit_x) ? limit_x : new_position;
                }

                if (new_coordinates.x == limit_x) {
                    dog_.SetSpeed({ 0.0, 0.0 });
                }

                return new_coordinates;
            }

            Coordinates Player::Move(double time) {
                Coordinates new_coordinates;

                Direction direction = dog_.GetDirection();

                switch (direction) {
                case application::game::utils::NORTH:
                case application::game::utils::SOUTH:
                    new_coordinates = ProccessVerticalMovement(time);
                    break;
                case application::game::utils::WEST:
                case application::game::utils::EAST:
                    new_coordinates = ProccessHorizontalMovement(time);
                    break;
                }

                dog_.SetPosition(new_coordinates);

                return new_coordinates;
            }

            size_t Player::GetBagCapacity() const {
                return loots_.size();
            }

            void Player::AddLoot(Loot loot) {
                loots_.push_back(std::move(loot));
            }

            const std::vector<Loot>& Player::GetLoots() const {
                return loots_;
            }

            void Player::CountItemValue(const std::vector<size_t>& index_to_value_) {
                for (const auto& loot : loots_) {
                    score_ += index_to_value_[loot.type_index];
                }

                loots_.clear();
            }

            size_t Player::GetScore() const {
                return score_;
            }

            void Player::SetScore(size_t score) {
                score_ = score;
            }

            const Dog& Player::GetDog() const {
                return dog_;
            }
            
        } // namespace player
    } // namespace game
} // namespace application