#pragma once

#include <cstddef>

#include <string>

constexpr ssize_t BUFFER_SIZE = 4096;

enum class STAT {
    REGULAR,
    DIRECTORY,
    NOT_EXIST,
    UNKNOWN
};

std::string determine_mime(const std::string &file_name);

STAT file_type(const std::string &path);

std::string get_output(const std::string &program, const char *envp[]);

std::string hostname();

std::string pwd();
