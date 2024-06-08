#pragma once
#include "http_server.h"
#include "model.h"

#include <boost\json.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
using namespace std::literals;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        switch (req.method()) {
        case http::verb::get:
            HandleGetRequest(std::move(req), std::forward<Send>(send));
            break;
        default:
            HandleBadRequest(std::move(req), std::forward<Send>(send), "Unknown HTTP-method", http::status::method_not_allowed);
            break;
        }
    }

private:
    template <typename Body, typename Allocator, typename Send>
    void HandleGetRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto target = req.target();
        std::string target_str = std::string(target);

        std::string expected_prefix = "/api/v1/maps";

        if (expected_prefix != target_str.substr(0, expected_prefix.size())) {
            HandleBadRequest(std::move(req), std::forward<Send>(send), "Invalid request-target", http::status::bad_request);
            return;
        }

        if (target_str == expected_prefix) {
            HandleGetMapsListRequest(std::move(req), std::forward<Send>(send));
            return;
        }

        std::string map_id_str = target_str.substr(expected_prefix.size());
        model::Map::Id map_id = model::Map::Id(map_id_str);

        if (const auto map = game_.FindMap(map_id)) {
            HandleGetMapRequest(std::move(req), std::forward<Send>(send), *map);
        }
        else {
            HandleBadRequest(std::move(req), std::forward<Send>(send), "Map not found", http::status::not_found, "mapNotFound");
        }
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleGetMapsListRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        http::response<http::string_body> response;

        response.result(http::status::ok);
        response.version(req.version());
        response.set(http::field::content_type, "application/json");
        response.keep_alive(req.keep_alive());

        boost::json::array maps_json;
        for (const auto& map : game_.GetMaps()) {
            maps_json.emplace_back(boost::json::array{ {"id", static_cast<std::string>(map.GetId())}, {"name", static_cast<std::string>(map.GetName())} });
        }

        response.body() = boost::json::serialize(maps_json);
        response.content_length(response.body().size());

        response.prepare_payload();
        return send(std::move(response));
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleGetMapRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, const model::Map& map) const {
        http::response<http::string_body> response;

        response.result(http::status::ok);
        response.version(req.version());
        response.set(http::field::content_type, "application/json");
        response.keep_alive(req.keep_alive());

        boost::json::array roads;
        for (const auto& road : map.GetRoads()) {
            boost::json::object road_obj;

            auto start = road.GetStart();
            road_obj["x0"] = start.x;
            road_obj["y0"] = start.y;

            if (road.IsHorizontal()) {
                road_obj["x1"] = road.GetEnd().x;
            }
            else {
                road_obj["y1"] = road.GetEnd().y;
            }

            roads.push_back(road_obj);
        }

        boost::json::array buildings;
        for (const auto& building : map.GetBuildings()) {
            boost::json::object building_obj;

            auto bounds = building.GetBounds();
            building_obj["x"] = bounds.position.x;
            building_obj["y"] = bounds.position.y;
            building_obj["w"] = bounds.size.width;
            building_obj["h"] = bounds.size.height;

            buildings.push_back(building_obj);
        }

        boost::json::array offices;
        for (const auto& office : map.GetOffices()) {
            boost::json::object office_obj;

            auto position = office.GetPosition();
            auto offset = office.GetOffset();

            office_obj["id"] = boost::json::string(static_cast<std::string>(office.GetId()));
            office_obj["x"] = position.x;
            office_obj["y"] = position.y;
            office_obj["offsetX"] = offset.dx;
            office_obj["offsetY"] = offset.dy;

            offices.push_back(office_obj);
        }

        boost::json::object map_obj{
            {"id", static_cast<std::string>(map.GetId())},
            {"name", map.GetName()},
            {"roads", roads},
            {"buildings", buildings},
            {"offices", offices}
        };

        response.body() = boost::json::serialize(map_obj);
        response.content_length(response.body().size());

        response.prepare_payload();
        return send(std::move(response));
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleBadRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send,
        beast::string_view message, http::status status, beast::string_view code = "badRequest") const {

        http::response<http::string_body> res{ status, req.version() };
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());

        boost::json::object error_obj{
            {"code", code},
            {"message", message}
        };

        res.body() = boost::json::serialize(error_obj);
        res.content_length(res.body().size());

        res.prepare_payload();
        return send(std::move(res));
    }

    

    model::Game& game_;
};

}  // namespace http_handler
