extern "C" {
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
}

#include <unordered_map>

#include "common.hpp"

const std::unordered_map<std::string, std::string> mime_types = {
    {"html", "text/html"},
    {"css", "text/css"},
    {"js", "application/javascript"},
    {"json", "application/json"},
    {"ttf", "application/font-sfnt"},
    {"pdf", "application/pdf"},
    {"svg", "image/svg+xml"},
    {"png", "image/png"},
    {"jpg", "image/jpeg"},
    {"gif", "image/gif"},
    {"ogg", "audio/ogg"},
    {"mp3", "audio/mpeg"},
    {"mkv", "video/x-matroska"},
    {"txt", "text/plain"},
    {"c", "text/x-csrc"},
    {"cpp", "text/x-c++src"},
    {"md", "text/x-markdown"}
};

std::string determine_mime(const std::string &file_name) {
    size_t length = file_name.size();
    size_t last_point = length - 1;
    while (last_point != 0 && file_name[last_point] != '.') {
        --last_point;
    }
    if (file_name[last_point] == '.') {
        ++last_point;
        std::string extension = std::string(
            &file_name[last_point],
            length - last_point);
        if (mime_types.find(extension) != mime_types.end()) {
            return mime_types.at(extension);
        }
    }
    return "application/octet-stream";
}

STAT file_type(const std::string &path) {
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) == -1) {
        return STAT::NOT_EXIST;
    }
    if (S_ISREG(path_stat.st_mode)) {
        return STAT::REGULAR;
    }
    if (S_ISDIR(path_stat.st_mode)) {
        return STAT::DIRECTORY;
    }
    return STAT::UNKNOWN;
}

std::string get_output(const std::string &program, const char *envp[]) {
    std::string output;
    int fd[2];
    pipe(fd);
    if (fork() == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        execle(program.c_str(), program.c_str(), nullptr, envp);
        exit(0);
    } else {
        close(fd[1]);
        char buffer[BUFFER_SIZE];
        for (;;) {
            ssize_t count = read(fd[0], buffer, sizeof(buffer));
            if (count == 0) {
                break;
            } else {
                output += std::string(buffer, count);
            }
        }
        close(fd[0]);
        wait(0);
    }
    return output;
}

std::string hostname() {
    struct utsname s;
    uname(&s);
    return s.nodename;
}

std::string pwd() {
    char buffer[PATH_MAX];
    getcwd(buffer, PATH_MAX);
    return buffer;
}
