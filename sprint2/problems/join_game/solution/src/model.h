#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>
#include <map>

#include "tagged.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
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

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

struct PlayerToken {
    uint64_t part_1;
    uint64_t part_2;

    std::string ToString() const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(16) << part_1
            << std::setw(16) << part_2;
        return oss.str();
    }

    bool operator==(const PlayerToken& other) const {
        return part_1 == other.part_1 && part_2 == other.part_2;
    }

    bool operator<(const PlayerToken& other) const {
        return std::tie(part_1, part_2) < std::tie(other.part_1, other.part_2);
    }

    static PlayerToken GenerateToken() {
        std::random_device rd;
        rng1.seed(rd());
        rng2.seed(rd());

        return {dist(rng1), dist(rng2)};
    }

    static PlayerToken FromString(const std::string& str) {
        if (str.size() != 32) {
            throw std::invalid_argument("Invalid token string length");
        }

        PlayerToken token;
        token.part_1 = std::stoull(str.substr(0, 16), nullptr, 16);
        token.part_2 = std::stoull(str.substr(16, 16), nullptr, 16);
        return token;
    }
    
private:
    static std::mt19937_64 rng1;
    static std::mt19937_64 rng2;
    static std::uniform_int_distribution<uint64_t> dist;
};

class Dog {
public:
    Dog(std::string& name, size_t id) : name_(name), id_(id) {
    }

    std::string GetName() const {
        return name_;
    }

    size_t GetId() const {
        return id_;
    }
private:
    size_t id_;
    std::string name_;
};

class GameSession;

class Player {
public:
    Player(PlayerToken token, Dog dog, GameSession& session)
        : token_(std::move(token)), dog_(dog), session_(session) {
    }

    size_t GetId() const {
        return dog_.GetId();
    }

    std::string GetName() const {
        return dog_.GetName();
    }

    const GameSession* GetSession() const {
        return &session_;
    }

private:
    PlayerToken token_;
    Dog dog_;
    GameSession& session_;
};

class GameSession {
public:
    GameSession(const Map& map) : map_(std::make_shared<Map>(map)) {
    }

    std::pair<PlayerToken, size_t> AddPlayer(std::string& name) {
        auto token = PlayerToken::GenerateToken();
        auto it = players_.emplace_back(token, Dog{ name, players_.size() }, *this);

        return { token, it.GetId() };
    }

    Player* GetPlayerById(size_t id) {
        return &players_[id];
    }

    const std::vector<Player>& GetPlayers() const {
        return players_;
    }
private:
    std::vector<Player> players_;
    std::shared_ptr<Map> map_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    std::pair<PlayerToken, size_t> AddPlayer(const Map* map, std::string& name) {
        auto it = sessions_.find(map->GetId());

        if (it == sessions_.end()) {
            GameSession new_session(*map);
            it = sessions_.emplace(map->GetId(), std::move(new_session)).first;
        }

        auto data = it->second.AddPlayer(name);

        players_.emplace(data.first, it->second.GetPlayerById(data.second));

        return data;
    }

    const GameSession* GetPlayerSession(const PlayerToken& token) const {
        return players_.at(token)->GetSession();
    }

    const std::vector<Player>& GetPlayersInSession(const GameSession& session) {
        return session.GetPlayers();
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::map<PlayerToken, Player*> players_;
    std::map<Map::Id, GameSession> sessions_;
    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
};

}  // namespace model
