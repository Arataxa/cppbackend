#pragma once

#include <string>
#include <iostream>

#include "utils.h"

namespace application{
	namespace game {
        using namespace utils;

        class Dog {
        public:
            Dog(std::string& name, size_t id, Coordinates coordinates);

            std::string GetName() const;

            size_t GetId() const;

            Coordinates GetPosition() const;

            Speed GetSpeed() const;

            void SetSpeed(const Speed& speed);

            Direction GetDirection() const;

            void SetDirection(Direction direction);

            void ChangeSpeed(double speed);

            void SetCoordinates(const Coordinates& new_coordinates);
        private:
            Direction direction_;
            Coordinates coordinates_;
            Speed speed_;
            size_t id_;
            std::string name_;
        };
	}
}
