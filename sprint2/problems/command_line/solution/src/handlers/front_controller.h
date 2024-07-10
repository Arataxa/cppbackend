#pragma once
#include "http_server.h"
#include "serialization.h"
#include "api_request_handler.h"
#include "static_request_handler.h"
#include "application.h"

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
#include <utility>

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    using namespace std::literals;
    namespace fs = std::filesystem;

    class FrontController {
    public:
        explicit FrontController(application::Application& application, 
            const std::string& static_root, 
            boost::asio::strand<boost::asio::io_context::executor_type>& strand, 
            bool is_tick_request_allowed)
            : application_(application), static_root_(static_root), strand_(strand), is_tick_request_allowed_(is_tick_request_allowed) {
        }

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            const std::string target = req.target().to_string();

            if (target.starts_with("/api/")) {
                boost::asio::dispatch(strand_, [this, req = std::move(req), send = std::move(send)]() mutable {
                    api_handler::ApiRequestHandler handlerr(application_, is_tick_request_allowed_);
                    handlerr.HandleRequest(std::move(req), std::move(send));
                    });
            }
            else {
                StaticRequestHandler handler(static_root_);
                handler.HandleRequest(std::move(req), std::forward<Send>(send));
            }
        }
    private:
        application::Application& application_;
        std::string static_root_;
        boost::asio::strand<boost::asio::io_context::executor_type>& strand_;
        bool is_tick_request_allowed_;
    };
}  // namespace http_handler
