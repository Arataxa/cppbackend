#include "htmldecode.h"

std::string HtmlDecode(std::string_view str) {
    static const std::unordered_map<std::string, char> html_entities = {
       {"&lt", '<'},
       {"&gt", '>'},
       {"&amp", '&'},
       {"&apos", '\''},
       {"&quot", '"'}
    };

    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '&') {
            size_t j = i + 1;
            while (j < str.size() && (std::isalnum(static_cast<unsigned char>(str[j])) || str[j] == '#')) {
                ++j;
            }
            if (j < str.size() && (str[j] == ';' || !std::isalnum(static_cast<unsigned char>(str[j])))) {
                std::string entity(str.substr(i, j - i));
                if (str[j] == ';') {
                    entity += ';';
                    ++j;
                }
                auto it = html_entities.find(entity);
                if (it != html_entities.end()) {
                    result += it->second;
                    i = j - 1;
                    continue;
                }
            }
        }
        result += str[i];
    }
    return result;
}
