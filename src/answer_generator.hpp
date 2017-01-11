#pragma once

#include "query_parser.hpp"

std::string header(
    int code,
    const std::string &record,
    const std::string &type,
    size_t size);

void generate_error(
    std::string &answer,
    int code,
    const std::string &record);

void from_file(
    std::string &answer,
    const std::string &file_name,
    uint16_t port,
    const std::string &query);

void generate_listing(
    std::string &answer,
    const std::string &directory);
