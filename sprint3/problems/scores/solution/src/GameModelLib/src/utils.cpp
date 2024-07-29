#include "utils.h"

namespace application {
    namespace game {
        namespace utils {
            double Coordinates::ComputeDistance(const Coordinates& start, const Coordinates& end) {
                return std::sqrt(std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2));
            }

            size_t GetRandomInteger(size_t max_value) {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<size_t> dist(0, max_value);
                return dist(gen);
            }

            double GetRandomReal(double max_value) {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_real_distribution<> dist(0.0, max_value);
                return dist(gen);
            }
        }
    }
}