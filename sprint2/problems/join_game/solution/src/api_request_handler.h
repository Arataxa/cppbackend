#pragma once

#include "model.h"
#include "serialization.h"

#include <boost/json.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <optional>

#include <string>


namespace http_handler {
    namespace api_handler {
        namespace beast = boost::beast;
        namespace http = beast::http;
        using namespace std::literals;
        namespace fs = std::filesystem;

        class BadRequestBuilder {
        public:
            http::status status;
            unsigned int version;

            std::optional<bool> keep_alive;
            std::string content_type = "application/json";
            bool cache_control = false;
            std::optional<std::string> allow;

            beast::string_view code;
            beast::string_view message;

            template <typename Send>
            void HandleBadRequest(Send&& send) {
                http::response<http::string_body> response{status, version};

                response.set(http::field::content_type, content_type);

                if (keep_alive.has_value()) {
                    response.keep_alive(keep_alive.value());
                }

                if (allow.has_value()) {
                    response.set(http::field::allow, allow.value());
                }

                boost::json::object error_obj{
                    {"code", code},
                    {"message", message}
                };
                response.body() = boost::json::serialize(error_obj);
                response.content_length(response.body().size());

                if (cache_control) {
                    response.set(http::field::cache_control, "no-cache");
                }

                response.prepare_payload();
                return send(std::move(response));
            }
        };

        template <typename Body, typename Allocator, typename Send>
        void HandleJoinGame(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send, model::Game& game) {
            if (http::verb::post != request.method()) {
                BadRequestBuilder handler;
                handler.version = request.version();
                handler.status = http::status::method_not_allowed;
                handler.allow = "POST";
                handler.cache_control = true;
                handler.code = "invalidMethod";
                handler.message = "Only POST method is expected";

                handler.HandleBadRequest(std::move(send));
                return;
            }

            boost::json::value request_body;
            std::string map_id;
            std::string user_name;

            try {
                request_body = boost::json::parse(request.body());
                map_id = request_body.as_object()["mapId"].as_string();;
                user_name = request_body.as_object()["userName"].as_string();
            }
            catch (...) {
                BadRequestBuilder handler;
                handler.version = request.version();
                handler.status = http::status::bad_request;
                handler.cache_control = true;
                handler.code = "invalidArgument";
                handler.message = "Join game request parse error";

                handler.HandleBadRequest(std::move(send));
                return;
            }

            auto* map = game.FindMap(model::Map::Id(map_id));

            if (user_name.empty()) {
                BadRequestBuilder handler;
                handler.version = request.version();
                handler.status = http::status::bad_request;
                handler.cache_control = true;
                handler.code = "invalidArgument";
                handler.message = "Invalid name";

                handler.HandleBadRequest(std::move(send));
                return;
            }

            if (!map) {
                BadRequestBuilder handler;
                handler.version = request.version();
                handler.status = http::status::not_found;
                handler.cache_control = true;
                handler.code = "mapNotFound";
                handler.message = "Map not found";

                handler.HandleBadRequest(std::move(send));
                return;
            }

            http::response<http::string_body> response;

            response.result(http::status::ok);
            response.version(request.version());
            response.set(http::field::content_type, "application/json");

            auto data = game.AddPlayer(map, user_name);

            boost::json::object obj{
                {"authToken", data.first.ToString()},
                {"playerId", data.second}
            };

            response.body() = boost::json::serialize(obj);
            response.content_length(response.body().size());
            response.set(http::field::cache_control, "no-cache");

            response.prepare_payload();
            return send(std::move(response));
        }

        template <typename Body, typename Allocator, typename Send>
        void HandleGetMaps(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send, model::Game& game) {
            http::response<http::string_body> response;

            response.result(http::status::ok);
            response.version(request.version());
            response.set(http::field::content_type, "application/json");
            response.keep_alive(request.keep_alive());

            boost::json::array maps_json;
            for (const auto& map : game.GetMaps()) {
                maps_json.emplace_back(boost::json::object{ {"id", static_cast<std::string>(map.GetId())}, {"name", static_cast<std::string>(map.GetName())} });
            }

            response.body() = boost::json::serialize(maps_json);
            response.content_length(response.body().size());

            response.prepare_payload();
            return send(std::move(response));
        }

        template <typename Body, typename Allocator, typename Send>
        void HandleGetMap(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send, model::Game& game) {
            auto target = request.target();
            std::string target_str = std::string(target);

            std::string expected_prefix = "/api/v1/map/";

            std::string map_id_str = target_str.substr(expected_prefix.size() + 1);
            model::Map::Id map_id = model::Map::Id(map_id_str);

            const auto map = game.FindMap(map_id);
            if (!map) {
                BadRequestBuilder handler;
                handler.version = request.version();
                handler.status = http::status::not_found;
                handler.code = "mapNotFound";
                handler.message = "Map not found";

                handler.HandleBadRequest(std::move(send));
                return;
            }

            http::response<http::string_body> response;

            response.result(http::status::ok);
            response.version(request.version());
            response.set(http::field::content_type, "application/json");
            response.keep_alive(request.keep_alive());

            response.body() = boost::json::serialize(JsonSerializer::SerializeMap(*map));
            response.content_length(response.body().size());

            response.prepare_payload();
            return send(std::move(response));
        }

        template <typename Body, typename Allocator, typename Send>
        void HandleGetPlayers(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send, model::Game& game) {
            if (request.method() != http::verb::head && request.method() != http::verb::get) {
                BadRequestBuilder handler;
                handler.version = request.version();
                handler.status = http::status::method_not_allowed;
                handler.allow = "GET, HEAD";
                handler.cache_control = true;
                handler.code = "invalidMethod";
                handler.message = "Invalid method";

                handler.HandleBadRequest(std::move(send));
                return;
            }

            auto auth_field = request[http::field::authorization];
            if (auth_field.empty() || !auth_field.starts_with("Bearer ")) {
                BadRequestBuilder handler;
                handler.version = request.version();
                handler.status = http::status::unauthorized;
                handler.cache_control = true;
                handler.code = "invalidToken";
                handler.message = "Authorization header is missing";

                handler.HandleBadRequest(std::move(send));
                return;
            }

            std::string auth_token = std::string(auth_field.substr(7));
            std::vector<const model::Player*> players;

            auto token = model::PlayerToken::FromString(auth_token);

            const auto* player = game.GetPlayer(token);

            if (!player) {
                BadRequestBuilder handler;
                handler.version = request.version();
                handler.status = http::status::unauthorized;
                handler.cache_control = true;
                handler.code = "unknownToken";
                handler.message = "Player token has not been found";

                handler.HandleBadRequest(std::move(send));
                return;
            }

            players = player->GetSession()->GetPlayersVector();

            http::response<http::string_body> response;
            response.result(http::status::ok);
            response.version(request.version());
            response.set(http::field::content_type, "application/json");
            response.set(http::field::cache_control, "no-cache");
            response.keep_alive(request.keep_alive());

            if (request.method() != http::verb::head) {
                boost::json::object json_body;

                for (const auto& player : players) {
                    json_body[std::to_string(player->GetId())] = boost::json::object({ {"name", player->GetName()} });
                }

                response.body() = boost::json::serialize(json_body);
                response.content_length(response.body().size());
            }
            else {
                response.content_length(0);
            }

            response.prepare_payload();

            return send(std::move(response));

        }

        class ApiRequestHandler {
        public:
            ApiRequestHandler(model::Game& game) : game_(game) {
            }

            template <typename Body, typename Allocator, typename Send>
            void HandleRequest(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send) {
                ProcessRequest(std::move(request), std::forward<Send>(send));
            }
        private:
            template <typename Body, typename Allocator, typename Send>
            void ProcessRequest(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send) {
                auto target = request.target();
                std::string target_str = std::string(target);

                std::string base_target = "/api/v1/";

                if (target_str == base_target + "game/join") {
                    HandleJoinGame(std::move(request), std::move(send), game_);
                }
                else if (target_str == base_target + "game/players") {
                    HandleGetPlayers(std::move(request), std::move(send), game_);
                }
                else if (target_str == base_target + "maps") {
                    HandleGetMaps(std::move(request), std::move(send), game_);
                }
                else if (target_str.starts_with(base_target + "maps/")) {
                    HandleGetMap(std::move(request), std::move(send), game_);
                }
                else {
                    BadRequestBuilder handler;
                    handler.version = request.version();
                    handler.status = http::status::bad_request;
                    handler.code = "badRequest";
                    handler.message = "Bad request";

                    handler.HandleBadRequest(std::move(send));
                }
            }

            
            model::Game& game_;
        };      
    } // namespace api_handler
} // namespace http_handler
