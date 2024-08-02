#pragma once

#include <optional>
#include <cassert>
#include <list>

#include "utils.h"

namespace application {
    namespace game {
        using namespace utils;

        namespace loot {
            struct CollectionResult {
                bool IsCollected(double collect_radius) const {
                    return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
                }

                // квадрат расстояния до точки
                double sq_distance;

                // доля пройденного отрезка
                double proj_ratio;
            };

            CollectionResult TryCollectPoint(const Coordinates& from, const Coordinates& to, const Coordinates& collect);

            struct Loot {
                Coordinates coordinates;
                size_t type_index;
                size_t id;
                bool is_collected = false;

                Loot(Coordinates coords, size_t type, bool generate_id = true)
                    : coordinates(coords), type_index(type) {

                    if (generate_id) {
                        id = GenerateId();
                    }
                }

            private:
                static size_t GenerateId() {
                    static size_t current_id = 0;
                    return current_id++;
                }
            };
        } // namespace loot
    } // namespace game
} // namespace application
