#pragma once

#include <string>

bool parse_method(
    const std::string &input,
    std::string &method,
    std::string &version,
    std::string &resource,
    std::string &query
);

bool parse_field(const std::string &input, std::string &a, std::string &b);
