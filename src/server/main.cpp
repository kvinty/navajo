#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

extern "C" {
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
}

#include "common.hpp"
#include "query_parser.hpp"
#include "answer_generator.hpp"
#include "config_reader.hpp"

static int listen_socket;

static void termination_handler(int signal) {
    fprintf(stderr, "\nServer stopped (signal %d)\n", signal);
    close(listen_socket);
    exit(0);
}

static void process_connection(
    int connection_socket,
    struct sockaddr_in *client_address,
    uint16_t port,
    FILE *log,
    const std::string &chroot
) {
    if (fork() == 0) {
        std::string buffer;
        buffer.resize(BUFFER_SIZE);
        std::string message;
        if (connection_socket == -1) {
            fprintf(stderr, "Error: accept() failed: %d\n", errno);
        } else {
            ssize_t bytes = read(connection_socket, &buffer[0], BUFFER_SIZE);
            if (bytes == -1) {
                message = generate_error(404, "");
            } else if (bytes == BUFFER_SIZE) {
                message = generate_error(431, "");
            } else {
                buffer.resize(bytes);
                std::string method, version, resource, query;
                std::string first = buffer.substr(0, buffer.find('\n'));
                time_t now = time(nullptr);
                fprintf(
                    log,
                    "%s%s\n%s\n",
                    ctime(&now),
                    inet_ntoa(client_address->sin_addr),
                    first.c_str()
                );
                fflush(log);
                if (!parse_method(first, method, version, resource, query)) {
                    message = generate_error(400, "");
                } else {
                    if (resource == "") {
                        message = generate_listing("./");
                    } else if (file_type(resource) == STAT::REGULAR) {
                        message = from_file(resource, port, query, chroot);
                    } else if (file_type(resource) == STAT::DIRECTORY) {
                        if (resource[resource.size() - 1] != '/') {
                            message = generate_error(
                                301,
                                "Location: /" + resource + "/\n"
                            );
                        } else {
                            message = generate_listing(resource);
                        }
                    } else {
                        message = generate_error( 404, "");
                    }
                }
            }
            if (write(
                connection_socket,
                message.c_str(),
                message.size()) == -1
            ) {
                fprintf(stderr, "Error: write() failed: %d\n", errno);
            }
        }
        close(listen_socket);
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    uint16_t port;
    std::string chroot;
    FILE *log = nullptr;
    bool port_set = false, home_set = false, log_set = false;
    struct sockaddr_in server_address;
    if (argc != 2) {
        fprintf(stderr, "Error: usage: " PROJECT_NAME " config_file\n");
        return 1;
    }
    std::vector<std::pair<std::string, std::string>> config =
    read_config(argv[1]);
    for (const std::pair<std::string, std::string> &p : config) {
        if (p.first == "port") {
            if (from_string(p.second, &port)) {
                port_set = true;
            }
        } else if (p.first == "home") {
            if (chdir(p.second.c_str()) == 0) {
                home_set = true;
            }
        } else if (p.first == "log") {
            log = fopen(p.second.c_str(), "a");
            if (log != nullptr) {
                log_set = true;
            }
        } else if (p.first == "chroot") {
            chroot = p.second;
        }
    }
    if (!port_set || !home_set || !log_set) {
        fprintf(stderr, "Error: config file invalid\n");
        return 1;
    }
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    int value = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
    if (listen_socket == -1) {
        fprintf(stderr, "Error: socket() failed: %d\n", errno);
        return 1;
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(
            listen_socket,
            (struct sockaddr *) &server_address,
            sizeof(server_address)
        ) == -1
    ) {
        fprintf(stderr, "Error: bind() failed: %d\n", errno);
        return 1;
    }
    listen(listen_socket, 1);
    fprintf(stderr, "Server started on port %d\n\n", port);
    signal(SIGINT, termination_handler);
    signal(SIGTERM, termination_handler);
    signal(SIGCHLD, SIG_IGN);
    for (;;) {
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);
        int connection_socket = accept(
            listen_socket,
            (struct sockaddr *) &client_address,
            &client_address_length
        );
        process_connection(
            connection_socket,
            &client_address,
            port,
            log,
            chroot
        );
    }
}
