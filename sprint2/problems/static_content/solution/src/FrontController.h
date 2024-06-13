#pragma once
#include "http_server.h"
#include "model.h"
#include "serialization.h"
#include "ApiRequestHandler.h"
#include "StaticRequestHandler.h"

#include <boost/json.hpp>
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
#include <filesystem>

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    using namespace std::literals;
    namespace fs = std::filesystem;

    class FrontController {
    public:
        explicit FrontController(model::Game& game, const std::string& static_root) 
            : game_(game), static_root_(static_root) {
        }

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            const std::string target = req.target().to_string();

            if (target.starts_with("/api/")) {
                ApiRequestHandler handler(game_);
                handler.HandleRequest(std::move(req), std::forward<Send>(send));
            }
            else {
                StaticRequestHandler handler(static_root_);
                handler.HandleRequest(std::move(req), std::forward<Send>(send));
            }
        }
    private:
        model::Game& game_;
        std::string static_root_;
    };
}  // namespace http_handler
