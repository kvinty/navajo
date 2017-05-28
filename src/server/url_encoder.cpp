#include <cctype>

#include "url_encoder.hpp"

static constexpr char hex[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F'
};

static char to_hex(char c) {
    return hex[c & 15];
}

static char from_hex(char c) {
    if (isdigit(c)) {
        return c - '0';
    }
    return c - 'A' + 10;
}

std::string url_encode(const std::string &s) {
    std::string buffer;
    buffer.reserve(3 * s.size());
    for (char c : s) {
        if (isalnum(c) || c == '-' || c == '_' ||
            c == '.' || c == '~' || c == '/') {
            buffer += c;
        } else {
            buffer += '%';
            buffer += to_hex(c >> 4);
            buffer += to_hex(c & 15);
        }
    }
    buffer.shrink_to_fit();
    return buffer;
}

std::string url_decode(const std::string &s) {
    std::string buffer;
    buffer.reserve(s.size());
    for (size_t i = 0; i != s.size(); ++i) {
        if (s[i] == '%') {
            buffer += from_hex(s[i + 1]) << 4 | from_hex(s[i + 2]);
            i += 2;
        } else {
            buffer += s[i];
        }
    }
    buffer.shrink_to_fit();
    return buffer;
}
