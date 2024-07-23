#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"

#include <sstream>
#include "catch2/catch_test_macros.hpp"
#include "catch2/catch_approx.hpp"
#include <cmath>
#include <vector>

using Catch::Approx;

namespace Catch {
    template<>
    struct StringMaker<collision_detector::GatheringEvent> {
        static std::string convert(collision_detector::GatheringEvent const& value) {
            std::ostringstream tmp;
            tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";
            return tmp.str();
        }
    };
}

class TestItemGathererProvider : public collision_detector::ItemGathererProvider {
public:
    std::vector<collision_detector::Item> items;
    std::vector<collision_detector::Gatherer> gatherers;

    size_t ItemsCount() const override {
        return items.size();
    }

    collision_detector::Item GetItem(size_t idx) const override {
        return items[idx];
    }

    size_t GatherersCount() const override {
        return gatherers.size();
    }

    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return gatherers[idx];
    }
};


TEST_CASE("No items or gatherers", "[FindGatherEvents]") {
    TestItemGathererProvider provider;
    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("Single item and gatherer, no collision", "[FindGatherEvents]") {
    TestItemGathererProvider provider;
    provider.items.push_back({ {0, 0}, 1 });
    provider.gatherers.push_back({ {10, 10}, {20, 20}, 1 });

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("Single item and gatherer, collision occurs", "[FindGatherEvents]") {
    TestItemGathererProvider provider;
    provider.items.push_back({ {5, 5}, 1 });
    provider.gatherers.push_back({ {0, 0}, {10, 10}, 1 });

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].item_id == 0);
    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE(events[0].sq_distance == Approx(0.0).epsilon(1e-10));
    REQUIRE(events[0].time == Approx(0.5).epsilon(1e-10));
}

TEST_CASE("Multiple items and gatherers", "[FindGatherEvents]") {
    TestItemGathererProvider provider;
    provider.items.push_back({ {5, 5}, 1 });
    provider.items.push_back({ {15, 15}, 1 });
    provider.gatherers.push_back({ {0, 0}, {10, 10}, 1 });
    provider.gatherers.push_back({ {10, 10}, {20, 20}, 1 });

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 2);

    REQUIRE(events[0].item_id == 0);
    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE(events[0].sq_distance == Approx(0.0).epsilon(1e-10));
    REQUIRE(events[0].time == Approx(0.5).epsilon(1e-10));

    REQUIRE(events[1].item_id == 1);
    REQUIRE(events[1].gatherer_id == 1);
    REQUIRE(events[1].sq_distance == Approx(0.0).epsilon(1e-10));
    REQUIRE(events[1].time == Approx(0.5).epsilon(1e-10));
}

TEST_CASE("Collision detection with different item sizes", "[FindGatherEvents]") {
    TestItemGathererProvider provider;
    provider.items.push_back({ {5, 5}, 2 });
    provider.items.push_back({ {15, 15}, 0.5 });
    provider.gatherers.push_back({ {0, 0}, {10, 10}, 1 });
    provider.gatherers.push_back({ {10, 10}, {20, 20}, 1 });

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 2);

    REQUIRE(events[0].item_id == 0);
    REQUIRE(events[0].gatherer_id == 0);
    REQUIRE(events[0].sq_distance == Approx(0.0).epsilon(1e-10));
    REQUIRE(events[0].time == Approx(0.5).epsilon(1e-10));

    REQUIRE(events[1].item_id == 1);
    REQUIRE(events[1].gatherer_id == 1);
    REQUIRE(events[1].sq_distance == Approx(0.0).epsilon(1e-10));
    REQUIRE(events[1].time == Approx(0.5).epsilon(1e-10));
}

TEST_CASE("No collision when gatherer does not move", "[FindGatherEvents]") {
    TestItemGathererProvider provider;
    provider.items.push_back({ {5, 5}, 1 });
    provider.gatherers.push_back({ {10, 10}, {10, 10}, 1 });

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.empty());
}

TEST_CASE("Events are in chronological order", "[FindGatherEvents]") {
    TestItemGathererProvider provider;
    provider.items.push_back({ {2, 2}, 1 });
    provider.items.push_back({ {5, 5}, 1 });
    provider.gatherers.push_back({ {0, 0}, {10, 10}, 1 });

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 2);

    REQUIRE(events[0].time < events[1].time);
}