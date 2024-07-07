#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <random>

#include "utils.h"

namespace application {
    namespace game {
        namespace map {
            struct Size {
                int width, height;
            };

            struct Point {
                int x, y;
            };

            struct Rectangle {
                Point position;
                Size size;
            };

            struct Offset {
                int dx, dy;
            };

            class Road {
                struct HorizontalTag {
                    HorizontalTag() = default;
                };

                struct VerticalTag {
                    VerticalTag() = default;
                };

            public:
                constexpr static HorizontalTag HORIZONTAL{};
                constexpr static VerticalTag VERTICAL{};

                Road(HorizontalTag, Point start, int end_x) noexcept;

                Road(VerticalTag, Point start, int end_y) noexcept;

                bool IsHorizontal() const noexcept;

                bool IsVertical() const noexcept;

                Point GetStart() const noexcept;

                Point GetEnd() const noexcept;

            private:
                Point start_;
                Point end_;
            };

            class Building {
            public:
                explicit Building(Rectangle bounds) noexcept;

                const Rectangle& GetBounds() const noexcept;

            private:
                Rectangle bounds_;
            };

            class Office {
            public:
                Office(std::string id, Point position, Offset offset) noexcept;

                const std::string& GetId() const noexcept;

                Point GetPosition() const noexcept;

                Offset GetOffset() const noexcept;

            private:
                std::string id_;
                Point position_;
                Offset offset_;
            };

            class Map {
            public:
                using Roads = std::vector<Road>;
                using Buildings = std::vector<Building>;
                using Offices = std::vector<Office>;

                Map(std::string id, std::string name, double default_speed) noexcept;

                const std::string& GetId() const noexcept;

                const std::string& GetName() const noexcept;

                const Buildings& GetBuildings() const noexcept;

                const Roads& GetRoads() const noexcept;

                const Offices& GetOffices() const noexcept;

                void AddRoad(const Road& road);

                void AddBuilding(const Building& building);

                void AddOffice(Office office);

                utils::Coordinates GetRandomPosition() const;

                double GetSpeed() const;
            private:
                using OfficeIdToIndex = std::unordered_map<std::string, size_t>;

                std::string id_;
                std::string name_;
                double default_speed_;

                Roads roads_;
                Buildings buildings_;

                OfficeIdToIndex warehouse_id_to_index_;
                Offices offices_;
            };
        } // namespace map
    }  // namespace game
} // namespace application
