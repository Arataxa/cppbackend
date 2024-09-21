#include "map.h"

#include <stdexcept>

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

            Map::Map(std::string id, std::string name, double default_speed, std::vector<size_t> index_to_score_value, size_t bag_capacity) noexcept
                : id_(std::move(id))
                , name_(std::move(name))
                , default_speed_(default_speed)
                , index_to_score_value_(index_to_score_value)
                , bag_capacity_(bag_capacity) {
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

                size_t road_index = GetRandomInteger(roads_.size() - 1);
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

                double random_position = GetRandomReal(static_cast<double>(road_length));

                if (road.IsHorizontal()) {
                    result.x = std::min(road.GetStart().x, road.GetEnd().x) + static_cast<int>(random_position);
                }
                else {
                    result.y = std::min(road.GetStart().y, road.GetEnd().y) + static_cast<int>(random_position);
                }

                return result;
            }

            utils::Coordinates Map::GetStartPosition() const {
                utils::Coordinates result;
                
                const Road& road = roads_.front();

                Point start = road.GetStart();
                result.x = start.x;
                result.y = start.y;

                return result;
            }

            double Map::GetSpeed() const {
                return default_speed_;
            }

            size_t Map::GetLootTypesCount() const {
                return index_to_score_value_.size();
            }

            size_t Map::GetBagCapacity() const {
                return bag_capacity_;
            }

            const std::vector<size_t>& Map::GetIndexToScoreList() const {
                return index_to_score_value_;
            }

            MapBuilder& MapBuilder::SetId(std::string& id) {
                id_ = id;
                return *this;
            }

            MapBuilder& MapBuilder::SetName(const std::string& name) {
                name_ = name;
                return *this;
            }

            MapBuilder& MapBuilder::SetDogSpeed(double dog_speed) {
                dog_speed_ = dog_speed;
                return *this;
            }

            MapBuilder& MapBuilder::SetTypesValue(std::vector<size_t>& index_to_score_value) {
                index_to_score_value_ = index_to_score_value;
                return *this;
            }

            MapBuilder& MapBuilder::SetBagCapacity(size_t bag_capacity) {
                bag_capacity_ = bag_capacity;
                return *this;
            }

            Map MapBuilder::Build() {
                return Map(id_, name_, dog_speed_, index_to_score_value_, bag_capacity_);
            }
        }
    }
}