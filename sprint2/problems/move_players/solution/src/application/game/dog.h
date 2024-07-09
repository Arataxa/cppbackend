#pragma once

#include <string>

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

            Direction GetDirection() const;

            void SetDirection(Direction direction);

            void ChangeSpeed(double speed);
        private:
            Direction direction_;
            Coordinates coordinates_;
            Speed speed_;
            size_t id_;
            std::string name_;
        };
	}
}
