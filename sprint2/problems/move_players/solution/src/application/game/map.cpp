#include "map.h"

#include <stdexcept>
#include <iostream>

namespace application {
    namespace game {
        namespace map {
            using namespace utils;

            Road::Road(HorizontalTag, Point start, int end_x) noexcept
                : start_{ start }
                , end_{ end_x, start.y } {
            }

            Road::Road(VerticalTag, Point start, int end_y) noexcept
                : start_{ start }
                , end_{ start.x, end_y } {
            }

            bool Road::IsHorizontal() const noexcept {
                return start_.y == end_.y;
            }

            bool Road::IsVertical() const noexcept {
                return start_.x == end_.x;
            }

            Point Road::GetStart() const noexcept {
                return start_;
            }

            Point Road::GetEnd() const noexcept {
                return end_;
            }

            Building::Building(Rectangle bounds) noexcept
                : bounds_{ bounds } {
            }

            const Rectangle& Building::GetBounds() const noexcept {
                return bounds_;
            }

            Office::Office(std::string id, Point position, Offset offset) noexcept
                : id_{ std::move(id) }
                , position_{ position }
                , offset_{ offset } {
            }

            const std::string& Office::GetId() const noexcept {
                return id_;
            }

            Point Office::GetPosition() const noexcept {
                return position_;
            }

            Offset Office::GetOffset() const noexcept {
                return offset_;
            }

            Map::Map(std::string id, std::string name, double default_speed) noexcept
                : id_(std::move(id))
                , name_(std::move(name))
                , default_speed_(default_speed) {
            }

            const std::string& Map::GetId() const noexcept {
                return id_;
            }

            const std::string& Map::GetName() const noexcept {
                return name_;
            }

            const Map::Buildings& Map::GetBuildings() const noexcept {
                return buildings_;
            }

            const Map::Roads& Map::GetRoads() const noexcept {
                return roads_;
            }

            const Map::Offices& Map::GetOffices() const noexcept {
                return offices_;
            }

            void Map::AddRoad(const Road& road) {
                roads_.emplace_back(road);
            }

            void Map::AddBuilding(const Building& building) {
                buildings_.emplace_back(building);
            }

            void Map::AddOffice(Office office) {
                if (warehouse_id_to_index_.contains(office.GetId())) {
                    throw std::invalid_argument("Duplicate warehouse");
                }

                const size_t index = offices_.size();
                Office& o = offices_.emplace_back(std::move(office));
                try {
                    warehouse_id_to_index_.emplace(o.GetId(), index);
                }
                catch (...) {
                    // Удаляем офис из вектора, если не удалось вставить в unordered_map
                    offices_.pop_back();
                    throw;
                }
            }

            utils::Coordinates Map::GetRandomPosition() const {
                utils::Coordinates result;

                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> distrib(0, roads_.size() - 1);

                size_t road_index = distrib(gen);
                int road_length = 0;

                const Road& road = roads_[road_index];

                if (road.IsHorizontal()) {
                    Point start = road.GetStart();
                    road_length = abs(road.GetEnd().x - start.x);
                    result.y = start.y;
                }
                else {
                    Point start = road.GetStart();
                    road_length = abs(road.GetEnd().y - start.y);
                    result.x = start.x;
                }

                std::uniform_real_distribution<> real_distrib(0.0, static_cast<double>(road_length));
                double random_position = real_distrib(gen);

                if (road.IsHorizontal()) {
                    result.x = std::min(road.GetStart().x, road.GetEnd().x) + static_cast<int>(random_position);
                }
                else {
                    result.y = std::min(road.GetStart().y, road.GetEnd().y) + static_cast<int>(random_position);
                }

                std::cout << "x: " << result.x << " y: " << result.y << std::endl;

                return result;
            }

            double Map::GetSpeed() const {
                return default_speed_;
            }
        }
    }
}