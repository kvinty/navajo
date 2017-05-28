#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

std::string header(
    int code,
    const std::string &record,
    const std::string &type,
    size_t size
);

std::string generate_error(int code, const std::string &record);

std::string from_file(
    const std::string &file_name,
    uint16_t port,
    const std::string &query,
    const std::string &chroot
);

std::string generate_listing(const std::string &directory);
