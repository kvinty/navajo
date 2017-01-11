#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>

extern "C" {
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
}

#include "answer_generator.hpp"

constexpr uint16_t DEFAULT_PORT = 1200;

int listen_socket;

void termination_handler(int signal) {
    printf("\nServer stopped (signal %d)\n", signal);
    close(listen_socket);
    exit(0);
}

void process_connection(int connection_socket, uint16_t port) {
    if (fork() == 0) {
        std::string buffer;
        buffer.resize(BUFFER_SIZE);
        std::string message;
        if (connection_socket == -1) {
            printf("accept() failed: %d\n", errno);
        } else {
            ssize_t readed = read(connection_socket, &buffer[0], BUFFER_SIZE);
            if (readed == -1) {
                generate_error(message, 404, "");
            } else if (readed == BUFFER_SIZE) {
                generate_error(message, 431, "");
            } else {
                buffer.resize(readed);
                printf("%s\n", buffer.c_str());
                std::string method, version, resource, query;
                std::string first = buffer.substr(0, buffer.find('\n'));
                if (!parse_method(first, method, version, resource, query)) {
                    generate_error(message, 400, "");
                } else {
                    if (resource == "") {
                        generate_listing(message, "./");
                    } else if (file_type(resource) == STAT::REGULAR) {
                        from_file(message, resource, port, query);
                    } else if (file_type(resource) == STAT::DIRECTORY) {
                        if (resource[resource.size() - 1] != '/') {
                            generate_error(
                                message,
                                301,
                                "Location: /" + resource + "/\n");
                        } else {
                            generate_listing(message, resource);
                        }
                    } else {
                        generate_error(message, 404, "");
                    }
                }
            }
            if (write(
                connection_socket,
                message.c_str(),
                message.size()) == -1) {
                printf("write() failed: %d\n", errno);
            }
        }
        close(listen_socket);
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    uint16_t port;
    struct sockaddr_in server_address;
    if (argc == 1) {
        port = DEFAULT_PORT;
    } else if (argc == 2) {
        long port_input;
        port_input = strtol(argv[1], nullptr, 10);
        port = port_input;
        if (port != port_input || port == 0) {
            printf("Port number should be an integer from 1 to 65535\n)");
            return 1;
        }
    } else {
        printf("Usage: %s [port] (default port: %d)\n", argv[0], DEFAULT_PORT);
        return 1;
    }
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1) {
        printf("socket() failed: %d\n", errno);
        return 1;
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(
            listen_socket,
            (struct sockaddr *) &server_address,
            sizeof(server_address)) == -1) {
        printf("bind() failed: %d\n", errno);
        return 1;
    }
    listen(listen_socket, 1);
    printf("Server started on port %d\n\n", port);
    signal(SIGINT, termination_handler);
    signal(SIGTERM, termination_handler);
    signal(SIGCHLD, SIG_IGN);
    for (;;) {
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);
        int connection_socket = accept(
            listen_socket,
            (struct sockaddr *) &client_address,
            &client_address_length);
        process_connection(connection_socket, port);
    }
}
