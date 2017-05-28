#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>

#include "config_reader.hpp"

static bool isnotspace(char c) {
    return !isspace(c);
}

static std::string trim(const std::string &s) {
    std::string tmp(s);
    tmp.erase(
        tmp.begin(),
        std::find_if(tmp.begin(), tmp.end(), isnotspace)
    );
    tmp.erase(
        std::find_if(tmp.rbegin(), tmp.rend(), isnotspace).base(),
        tmp.end()
    );
    return tmp;
}

std::vector<std::pair<std::string, std::string>> read_config(
    const std::string &path
) {
    std::vector<std::pair<std::string, std::string>> options;
    std::ifstream config(path);
    std::string line;
    while (std::getline(config, line)) {
        size_t delimiter = line.find('=');
        std::string part0 = line, part1 = line;
        if (delimiter != std::string::npos) {
            part0.erase(delimiter, line.size() - delimiter);
            part1.erase(0, delimiter + 1);
            options.push_back({trim(part0), trim(part1)});
        }
    }
    return options;
}
