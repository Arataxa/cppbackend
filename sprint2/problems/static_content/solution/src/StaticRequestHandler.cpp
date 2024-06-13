#include "StaticRequestHandler.h"

namespace http_handler {
	std::string StaticRequestHandler::GetMimeType(const std::string& extension) {
		static const std::unordered_map<std::string, std::string> mime_types = {
			{".html", "text/html"},
			{".htm", "text/html"},
			{".css", "text/css"},
			{".txt", "text/plain"},
			{".js", "text/javascript"},
			{".json", "application/json"},
			{".xml", "application/xml"},
			{".png", "image/png"},
			{".jpg", "image/jpeg"},
			{".jpe", "image/jpeg"},
			{".jpeg", "image/jpeg"},
			{".gif", "image/gif"},
			{".bmp", "image/bmp"},
			{".ico", "image/vnd.microsoft.icon"},
			{".tiff", "image/tiff"},
			{".tif", "image/tiff"},
			{".svg", "image/svg+xml"},
			{".svgz", "image/svg+xml"},
			{".mp3", "audio/mpeg"}
		};

		auto it = mime_types.find(extension);
		if (it != mime_types.end()) {
			return it->second;
		}
		return "application/octet-stream";
	}

	std::string StaticRequestHandler::GetMimeTypeFromPath(const std::string& path) {
		auto dot_pos = path.find_last_of('.');
		if (dot_pos != std::string::npos) {
			return GetMimeType(path.substr(dot_pos));
		}
		return "application/octet-stream";
	}

	bool StaticRequestHandler::IsSubPath(const fs::path& path, const fs::path& base) {
		auto canonical_path = fs::weakly_canonical(path);
		auto canonical_base = fs::weakly_canonical(base);
		return std::mismatch(canonical_base.begin(), canonical_base.end(), canonical_path.begin()).first == canonical_base.end();
	}

	std::string StaticRequestHandler::UrlDecode(const std::string& encoded) {
		std::ostringstream decoded;
		for (size_t i = 0; i < encoded.length(); ++i) {
			if (encoded[i] == '%' && i + 2 < encoded.length()) {
				std::istringstream hex_stream(encoded.substr(i + 1, 2));
				int hex;
				hex_stream >> std::hex >> hex;
				decoded << static_cast<char>(hex);
				i += 2;
			}
			else if (encoded[i] == '+') {
				decoded << ' ';
			}
			else {
				decoded << encoded[i];
			}
		}
		return decoded.str();
	}
} // namespace http_handler