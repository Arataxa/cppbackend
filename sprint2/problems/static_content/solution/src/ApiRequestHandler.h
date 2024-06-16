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

#include <string>


namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    using namespace std::literals;
    namespace fs = std::filesystem;

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
			switch (request.method()) {
			case http::verb::get:
				HandleGetRequest(std::move(request), std::forward<Send>(send));
				break;
			default:
				HandleBadRequest(std::move(request), std::forward<Send>(send), "Unknown HTTP-method", http::status::method_not_allowed);
				break;
			}
		}

        template <typename Body, typename Allocator, typename Send>
        void HandleGetRequest(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send) {
            auto target = request.target();
            std::string target_str = std::string(target);

            std::string expected_prefix = "/api/v1/maps";

            if (expected_prefix != target_str.substr(0, expected_prefix.size())) {
                HandleBadRequest(std::move(request), std::forward<Send>(send), "Bad request", http::status::bad_request);
                return;
            }

            if (target_str == expected_prefix) {
                HandleGetMapsListRequest(std::move(request), std::forward<Send>(send));
                return;
            }

            std::string map_id_str = target_str.substr(expected_prefix.size() + 1);
            model::Map::Id map_id = model::Map::Id(map_id_str);

            if (const auto map = game_.FindMap(map_id)) {
                HandleGetMapRequest(std::move(request), std::forward<Send>(send), *map);
            }
            else {
                HandleBadRequest(std::move(request), std::forward<Send>(send), "Map not found", http::status::not_found, "mapNotFound");
            }
        }

        template <typename Body, typename Allocator, typename Send>
        void HandleGetMapsListRequest(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send) {
            http::response<http::string_body> response;

            response.result(http::status::ok);
            response.version(request.version());
            response.set(http::field::content_type, "application/json");
            response.keep_alive(request.keep_alive());

            boost::json::array maps_json;
            for (const auto& map : game_.GetMaps()) {
                maps_json.emplace_back(boost::json::object{ {"id", static_cast<std::string>(map.GetId())}, {"name", static_cast<std::string>(map.GetName())} });
            }

            response.body() = boost::json::serialize(maps_json);
            response.content_length(response.body().size());

            response.prepare_payload();
            return send(std::move(response));
        }

        template <typename Body, typename Allocator, typename Send>
        void HandleGetMapRequest(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send, const model::Map& map) const {
            http::response<http::string_body> response;

            response.result(http::status::ok);
            response.version(request.version());
            response.set(http::field::content_type, "application/json");
            response.keep_alive(request.keep_alive());

            response.body() = boost::json::serialize(JsonSerializer::SerializeMap(map));
            response.content_length(response.body().size());

            response.prepare_payload();
            return send(std::move(response));
        }

        template <typename Body, typename Allocator, typename Send>
        void HandleBadRequest(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send,
            beast::string_view message, http::status status, beast::string_view code = "badRequest") const {

            http::response<http::string_body> response{ status, request.version() };
            response.set(http::field::content_type, "application/json");
            response.keep_alive(request.keep_alive());

            boost::json::object error_obj{
                {"code", code},
                {"message", message}
            };

            response.body() = boost::json::serialize(error_obj);
            response.content_length(response.body().size());

            response.prepare_payload();
            return send(std::move(response));
        }

        model::Game& game_;
	}; // class ApiRequestHandler
} // namespace http_handler
