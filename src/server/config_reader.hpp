#pragma once

#include <string>
#include <utility>
#include <vector>

std::vector<std::pair<std::string, std::string>> read_config(
    const std::string &path
);
