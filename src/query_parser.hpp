#pragma once

#include <vector>

#include "common.hpp"

void split(const std::string &str, std::vector<std::string> &parts);

bool parse_method(
    const std::string &input,
    std::string &method,
    std::string &version,
    std::string &resource,
    std::string &query);

bool parse_field(
    const std::string &input,
    std::string &a,
    std::string &b);
