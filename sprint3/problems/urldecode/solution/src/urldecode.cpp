#include "urldecode.h"

#include <charconv>
#include <stdexcept>
#include <sstream>

std::string UrlDecode(std::string encoded) {
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
