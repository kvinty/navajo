#include <algorithm>
#include <unordered_map>
#include <vector>

extern "C" {
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
}

#include "common.hpp"
#include "url_encoder.hpp"
#include "query_parser.hpp"
#include "answer_generator.hpp"

static const std::string NAME = PROJECT_NAME "/" PROJECT_VERSION;

static const std::string HTML_HEAD =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "    <head>\n"
    "        <title>" PROJECT_NAME "</title>\n"
    "        <meta charset=\"utf-8\">\n"
    "        <meta name=\"viewport\" content=\"width=device-width, initial-sca\
le=1.0\">\n"
    "    </head>\n";

static const std::unordered_map<int, std::string> error_codes = {
    {200, "OK"},
    {301, "Moved Permanently"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {431, "Request Header Fields Too Large"},
    {500, "Internal Server Error"}
};

std::string header(
    int code,
    const std::string &record,
    const std::string &type,
    size_t size
) {
    return
        "HTTP/1.1 " + std::to_string(code) + " " + error_codes.at(code) + "\n"
        "Server: " + NAME + "\n"
        "Content-Type: " + type + "\n"
        "Content-Length: " + std::to_string(size) + "\n"
        + record +
        "Connection: close\n\n";
}

std::string generate_error(int code, const std::string &record) {
    std::string page =
        HTML_HEAD +
        "    <body>\n"
        "        <h1>" + error_codes.at(code) + "</h1>\n"
        "        <h3>Error code: " + std::to_string(code) + "</h3>\n"
        "    </body>\n"
        "</html>\n";
    return header(code, record, "text/html", page.size()) + page;
}

std::string from_file(
    const std::string &file_name,
    uint16_t port,
    const std::string &query,
    const std::string &chroot
) {
    struct stat info;
    if (access(file_name.c_str(), R_OK)) {
        return generate_error(403, "");
    }
    stat(file_name.c_str(), &info);
    if (info.st_mode & S_IXUSR) {
        std::string s0, s1, s2, s3, s4, s5;
        const char *envp[] = {
            (s0 = ("SERVER_SOFTWARE=" + NAME)).c_str(),
            (s1 = ("SERVER_NAME=" + hostname())).c_str(),
            "GATEWAY_INTERFACE=CGI/1.1",
            "SERVER_PROTOCOL=HTTP/1.1",
            (s2 = ("SERVER_PORT=" + std::to_string(port))).c_str(),
            "REQUEST_METHOD=GET",
            "PATH_INFO=",
            (s3 = ("PATH_TRANSLATED=" + pwd())).c_str(),
            (s4 = (std::string("SCRIPT_NAME=/") + file_name)).c_str(),
            (s5 = ("QUERY_STRING=" + query)).c_str(),
            nullptr
        };
        std::string message = get_output(file_name, envp, chroot);
        size_t newline = message.find("\n\n");
        std::string content_type = message.substr(0, newline), a, b;
        if (parse_field(content_type, a, b)) {
            return header(
                200,
                "",
                b,
                message.size() - newline - 2
            ) + message.substr(newline + 2);
        } else {
            return generate_error(500, "");
        }
    } else {
        size_t length = info.st_size;
        int file = open(file_name.c_str(), O_RDONLY);
        std::string content;
        content.resize(length);
        if (read(file, &content[0], length) == -1) {
            return generate_error(500, "");
        }
        return header(200, "", determine_mime(file_name), length) + content;
    }
}

std::string generate_listing(const std::string &directory) {
    if (access(directory.c_str(), R_OK)) {
        return generate_error(403, "");
    }
    DIR *dir;
    struct dirent *entry;
    std::string page =
        HTML_HEAD +
        "    <body>\n"
        "        <h1>Directory listing</h1>\n"
        "        <ul>\n";
    std::vector<std::string> files;
    if ((dir = opendir(directory.c_str())) != nullptr) {
        while ((entry = readdir(dir)) != nullptr) {
            std::string file = entry->d_name;
            files.push_back(file);
        }
        std::sort(files.begin(), files.end());
        for (std::string &file : files) {
            if (file_type(directory + file) == STAT::DIRECTORY) {
                file += "/";
            }
            if (file != "./" && file != "../") {
                page +=
                    "            <li>\n"
                    "                <a href=" + url_encode(file) + ">" + file +
                    "</a>\n"
                    "            </li>\n";
            }
        }
        closedir(dir);
    } else {
        return generate_error(403, "");
    }
    page +=
        "        </ul>\n"
        "    </body>\n"
        "</html>\n";
    return header(200, "", "text/html", page.size()) + page;
}
