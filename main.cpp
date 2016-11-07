// libc headers
#include <errno.h>  // errno
#include <signal.h> // signal
#include <stddef.h> // size_t
#include <stdint.h> // uint16_t
#include <stdlib.h> // strtol, exit

// libstdc++ headers
#include <iostream>
#include <string>
#include <unordered_map>

// posix headers
#include <arpa/inet.h>  // htonl, htons
#include <dirent.h>     // closedir, opendir, readdir
#include <fcntl.h>      // open
#include <sys/stat.h>   // open, stat
#include <sys/socket.h> // bind, socket
#include <sys/types.h>  // bind, open, socket, stat
#include <unistd.h>     // close, read, stat, write

static constexpr size_t BUFFER_SIZE = 10240;
static constexpr uint16_t DEFAULT_PORT = 1200;

static int listen_socket;

static std::string determine_mime(const std::string &file_name) {
    static const std::unordered_map<std::string, std::string> mime_types = {
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
        {"mkv", "video/x-matroska"}
    };
    size_t last_point = file_name.size() - 1;
    while (last_point != 0 && file_name[last_point] != '.') {
        --last_point;
    }
    if (last_point != 0) {
        ++last_point;
        std::string extension = std::string(&file_name[last_point],
                                            file_name.size() - last_point);
        if (mime_types.find(extension) != mime_types.end()) {
            return mime_types.at(extension);
        }
    }
    return "application/octet-stream";
}

enum class STAT {
    REGULAR,
    DIRECTORY,
    NOT_EXIST,
    UNKNOWN
};

static STAT file_type(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) == -1) {
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

static std::string response(int code, const std::string &description,
                            const std::string &type, size_t size) {
	return 
	    "HTTP/1.1 " + std::to_string(code) + " " + description + "\n"
        "Server: navajo/0.1.0\n"
        "Content-Type: " + type + "\n"
        "Content-Length: " + std::to_string(size) + "\n"
        "Connection: close\n\n";
}

static const std::string HTML_HEAD = 
    "<!DOCTYPE html>\n"
	"<html>\n"
	"    <head>\n"
	"        <title>navajo</title>\n"
	"        <meta charset=\"utf-8\">\n"
	"        <meta name=\"viewport\" content=\"width=device-width, initial-sca\
le=1.0\">\n"
	"    </head>\n";
        
static void generate_error(std::string &answer, int number,
                           const std::string &description) {
    std::string page =
        HTML_HEAD +
        "    <body>\n"
        "        <h1>" + description + "</h1>\n"
        "        <h3>Error code: " + std::to_string(number) + "</h3>\n"
        "    </body>\n"
        "</html>\n";
    answer = response(number, description, "text/html", page.size()) + page;
}

static void from_file(std::string &answer, const char *file_name) {
    struct stat info;
    stat(file_name, &info);
    size_t length = info.st_size;
    int file = open(file_name, O_RDONLY);
    std::string content;
    content.resize(length);
    if (read(file, &content[0], length) == -1) {
		std::cout << "read() failed: " << errno << "\n";
	}
    answer = response(200, "OK", determine_mime(file_name), length) + content;
}

static void generate_listing(std::string &answer, const char *directory) {
    DIR *dir;
    struct dirent *entry;
    std::string page =
        HTML_HEAD +
        "    <body>\n"
        "        <h1>Directory listing</h1>\n"
        "        <ul>\n";

    if ((dir = opendir(directory)) != nullptr) {
        while ((entry = readdir(dir)) != nullptr) {
            std::string file = entry->d_name;
            std::string is_dir;
            if (file_type(&(std::string(directory) + file)[0]) == 
                          STAT::DIRECTORY) {
                is_dir = "/";
            }
            if (file != "." && file != "..") {
                page +=
                    "            <li>\n"
                    "                <a href=" + file + is_dir + ">" + file + 
                    "</a>\n"
                    "            </li>\n";
            }
        }
        closedir(dir);
    } else {
        generate_error(page, 403, "Forbidden");
        return;
    }
    page += 
        "        </ul>\n"
        "    </body>\n"
        "</html>\n";
    answer = response(200, "OK", "text/html", page.size()) + page;
}

static void termination_handler(int signal) {
    std::cout << "\nServer stopped (signal " << signal << ")\n";
    close(listen_socket);
    exit(0);
}

/* stub, should rewrite it completely */
static std::string parse_request(const std::string &request) {
    size_t end = 5;
    while (end != request.size() && request[end] != ' ') {
        ++end;
    }
    return std::string(&request[0] + 5, end - 5);
}

int main(int argc, char *argv[]) {
    uint16_t port;
    std::string buffer;
    buffer.resize(BUFFER_SIZE);
    struct sockaddr_in server_address;
    if (argc == 1) {
        port = DEFAULT_PORT;
    } else if (argc == 2) {
        long port_input;
        port_input = strtol(argv[1], nullptr, 10);
        port = port_input;
        if (port != port_input || port == 0) {
            std::cout << "Port number should be an integer from 1 to 65535\n";
            return 1;
        }
    } else {
        std::cout << "Usage: navajo [port] (default port: "
                  << DEFAULT_PORT << "\n";
        return 1;
    }
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1) {
        std::cout << "socket() failed: " << errno << "\n";
        return 1;
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listen_socket, (struct sockaddr *)&server_address,
             sizeof(server_address)) == -1) {
        std::cout << "bind() failed: " << errno << "\n";
        return 1;
    }
    listen(listen_socket, 1);
    std::cout << "Server started on port " << port << "\n\n";
    signal(SIGINT, termination_handler);
    for (;;) {
        int connection_socket;
        socklen_t client_address_length;
        struct sockaddr_in client_address;
        std::string message;
        client_address_length = sizeof(client_address);
        connection_socket = accept(listen_socket,
                                   (struct sockaddr *)&client_address,
                                   &client_address_length);
        if (connection_socket == -1) {
            std::cout << "accept() failed: " << errno << "\n";
        } else {
            buffer.resize(read(connection_socket, &buffer[0],
                               BUFFER_SIZE - 1));
            std::cout << buffer;
            std::string resource = parse_request(buffer) + "\0";
            if (resource == "\0") {
                generate_listing(message, "./");
            } else if (file_type(&resource[0]) == STAT::REGULAR) {
                from_file(message, &resource[0]);
            } else if (file_type(&resource[0]) == STAT::DIRECTORY) {
                generate_listing(message, &resource[0]);
            } else {
                generate_error(message, 404, "Not Found");
            }
            if (write(connection_socket, &message[0], message.size()) == -1) {
				std::cout << "write() failed: " << errno << "\n";
			}
            close(connection_socket);
        }
    }
}
