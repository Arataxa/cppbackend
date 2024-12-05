#include "dog.h"

namespace application {
    namespace game {
        Dog::Dog(std::string& name, size_t id, Coordinates coordinates) 
            : direction_(Direction::NORTH), coordinates_(std::move(coordinates)), id_(id), name_(name) {
        }

        std::string Dog::GetName() const {
            return name_;
        }

        size_t Dog::GetId() const {
            return id_;
        }

        Coordinates Dog::GetPosition() const {
            return coordinates_;
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

        void Dog::SetCoordinates(Coordinates& new_coordinates) {
            coordinates_.x = new_coordinates.x;
            coordinates_.y = new_coordinates.y;
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