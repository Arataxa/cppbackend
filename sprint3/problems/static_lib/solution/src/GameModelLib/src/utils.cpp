#include "utils.h"

namespace application {
    namespace game {
        namespace utils {
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