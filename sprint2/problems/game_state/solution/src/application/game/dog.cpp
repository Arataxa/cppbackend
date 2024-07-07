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
    }
}