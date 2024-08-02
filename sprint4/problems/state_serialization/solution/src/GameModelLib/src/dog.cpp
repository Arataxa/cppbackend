#include "dog.h"

namespace application {
    namespace game {
        Dog::Dog(std::string name, size_t id, Coordinates coordinates) 
            : name_(std::move(name)), id_(id), position_(std::move(coordinates)), direction_(Direction::NORTH) {
        }

        std::string Dog::GetName() const {
            return name_;
        }

        size_t Dog::GetId() const {
            return id_;
        }

        Coordinates Dog::GetPosition() const {
            return position_;
        }

        Speed Dog::GetSpeed() const{
            return speed_;
        }

        Direction Dog::GetDirection() const {
            return direction_;
        }

        void Dog::SetDirection(Direction direction) {
            direction_ = direction;
        }

        void Dog::SetSpeed(const Speed& speed) {
            speed_ = speed;
        }

        void Dog::SetPosition(const Coordinates& new_coordinates) {
            position_.x = new_coordinates.x;
            position_.y = new_coordinates.y;
        }

        void Dog::ChangeSpeed(double speed) {
            switch (direction_) {
            case Direction::NORTH:
                speed_.x = 0;
                speed_.y = (speed * -1);
                break;
            case Direction::SOUTH:
                speed_.x = 0;
                speed_.y = speed;
                break;
            case Direction::WEST:
                speed_.x = (speed * -1);
                speed_.y = 0;
                break;
            case Direction::EAST:
                speed_.x = speed;
                speed_.y = 0;
                break;
            }
        }
    }
}