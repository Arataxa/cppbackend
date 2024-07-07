#include "player.h"
#include "game.h"

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
        } // namespace player
    } // namespace game
} // namespace application