#pragma once

#include <boost/json.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>

#include <string>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <iostream>

namespace http_handler {
	namespace beast = boost::beast;
	namespace http = beast::http;
	using namespace std::literals;
	namespace fs = std::filesystem;

	std::string GetMimeType(const std::string& extension);

	std::string GetMimeTypeFromPath(const std::string& path);

	bool IsSubPath(const fs::path& path, const fs::path& base);

	std::string UrlDecode(const std::string& encoded);

	class StaticRequestHandler {
	public:
		StaticRequestHandler(std::string& static_root) : static_root_path_(static_root) {
		}

		template <typename Body, typename Allocator, typename Send>
		void HandleRequest(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send) {
			switch (request.method()) {
			case http::verb::get:
				HandleGetRequest(std::move(request), std::forward<Send>(send));
				break;
			default:
				HandleBadRequest(std::move(request), std::forward<Send>(send), "Unknown HTTP-method", http::status::method_not_allowed);
				break;
			}
		}
	private:
		template <typename Body, typename Allocator, typename Send>
		void HandleGetRequest(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send) {
			std::string target = static_root_path_.string() + UrlDecode(request.target().to_string());

			if (target == static_root_path_.string() + "/") {
				target += "/index.html";
			}

			fs::path full_path = fs::weakly_canonical(target);

			if (!IsSubPath(full_path, static_root_path_)) {
				HandleBadRequest(std::move(request), std::forward<Send>(send)
					, "Bad request: Path outside of root directory", http::status::bad_request);
				return;
			}

			if (fs::exists(full_path) && fs::is_regular_file(full_path)) {
				HandleGetFileRequest(std::move(request), std::forward<Send>(send), full_path);
			}
			else {
				HandleBadRequest(std::move(request), std::forward<Send>(send)
					, "File not found", http::status::not_found);
				return;
			}
		}

		template <typename Body, typename Allocator, typename Send>
		void HandleGetFileRequest(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send, fs::path& file_path) {
			std::ifstream file(file_path);

			std::string content_type = GetMimeTypeFromPath(file_path.string());

			http::response<http::string_body> response{ http::status::ok, request.version() };
			response.set(http::field::content_type, content_type);
			response.keep_alive(request.keep_alive());

			std::stringstream buffer;
			buffer << file.rdbuf();
			std::string content = buffer.str();
			response.body() = content;

			response.content_length(response.body().size());

			response.prepare_payload();
			return send(std::move(response));
		}

		template <typename Body, typename Allocator, typename Send>
		void HandleBadRequest(http::request<Body, http::basic_fields<Allocator>>&& request, Send&& send,
			beast::string_view message, http::status status) {

			http::response<http::string_body> response{ status, request.version() };
			response.set(http::field::content_type, "text/plain");
			response.keep_alive(request.keep_alive());

			response.body() = std::string(message);
			response.content_length(response.body().size());

			response.prepare_payload();
			return send(std::move(response));
		}

		fs::path static_root_path_;
	}; // class StaticRequestHandler
} // namespace http_handler
