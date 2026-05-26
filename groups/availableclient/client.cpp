#include <client.h>

#include <constant.h>
#include <protocol.h>
#include <utils.h>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <poll.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace ac = available_common;

namespace available_client {
auto Client::_setup(const std::string &hostname, const std::string &message) -> CLIENT_ERROR_CODES {
    static const std::string k_LOG_PREFIX = "CLIENT:_setup():";

    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;

    if (auto rv = getaddrinfo(hostname.c_str(), ac::k_PORT.c_str(), &hints, &d_servinfo);
        rv == -1) {
        std::cerr << k_LOG_PREFIX << "31 Error occurred " << gai_strerror(rv) << std::endl;
        return CLIENT_ERROR_CODES::FAILURE;
    }

    d_connectedinfo = d_servinfo;
    for (; d_connectedinfo != nullptr; d_connectedinfo = d_connectedinfo->ai_next) {
        if (d_sockfd = socket(d_connectedinfo->ai_family, d_connectedinfo->ai_socktype,
                              d_connectedinfo->ai_protocol);
            d_sockfd == ac::k_INVALID_SOCKET) {
            static const std::string error_msg = k_LOG_PREFIX + "41 socket";
            perror(error_msg.c_str());
            continue;
        }
        break;
    }

    if (d_connectedinfo == nullptr) {
        _shutdown();
        std::cerr << k_LOG_PREFIX << "49 Unable to bind to a socket" << std::endl;
        return CLIENT_ERROR_CODES::FAILURE;
    }

    return CLIENT_ERROR_CODES::SUCCESS;
}

void Client::_shutdown() {
    if (d_servinfo != nullptr) {
        freeaddrinfo(d_servinfo);
        d_servinfo = nullptr;
        d_connectedinfo = nullptr;
    }

    if (d_sockfd != ac::k_INVALID_SOCKET) {
        close(d_sockfd);
        d_sockfd = ac::k_INVALID_SOCKET;
    }
}

void Client::_send_msg(struct addrinfo *connectedinfo, const std::string &message) {
    static const std::string k_LOG_PREFIX = "CLIENT:run():";

    auto send_bytes = sendto(d_sockfd, message.data(), message.size(), 0, connectedinfo->ai_addr,
                             connectedinfo->ai_addrlen);

    if (send_bytes < 0) {
        static const std::string error_msg = k_LOG_PREFIX + " sendto";
        perror(error_msg.c_str());
    }
}

Client::Client(const std::string &hostname, const std::string &message)
    : d_username(message), d_connectedinfo(nullptr), d_servinfo(nullptr),
      d_sockfd(ac::k_INVALID_SOCKET) {
    if (_setup(hostname, message) != CLIENT_ERROR_CODES::SUCCESS) {
        _shutdown();
    }
}

Client::~Client() { _shutdown(); }

void Client::run() {
    if (d_sockfd == ac::k_INVALID_SOCKET || d_servinfo == nullptr) {
        return;
    }

    static const std::string k_LOG_PREFIX = "CLIENT:run():";
    const std::string login_msg = ac::ChatProtocol::k_JOIN + d_username;
    _send_msg(d_connectedinfo, login_msg);

    struct pollfd pfds[2];

    // listening on standard input
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;

    // listening on the socket
    pfds[1].fd = d_sockfd;
    pfds[1].events = POLLIN;

    char buffer[ac::k_MAX_DATA_SIZE];
    while (true) {
        int rc = poll(pfds, 2, -1);

        if (rc < 0) {
            static const std::string error_msg = k_LOG_PREFIX + "109 polling";
            perror(error_msg.c_str());
            break;
        }

        if (pfds[0].revents & POLLIN) {
            if (d_token.empty()) {
                break;
            }

            std::string line;
            if (!std::getline(std::cin, line)) {
                _send_msg(d_connectedinfo, ac::ChatProtocol::k_LEAVE + d_token);
                break;
            }

            if (line == "/quit") {
                _send_msg(d_connectedinfo, ac::ChatProtocol::k_LEAVE + d_token);
                break;
            }

            if (!line.empty()) {
                _send_msg(d_connectedinfo, ac::ChatProtocol::k_MSG + d_token + " " + line);
            }
        }

        if (pfds[1].revents & POLLIN) {
            sockaddr_storage from_addr{};
            socklen_t from_addr_len = sizeof(from_addr);

            size_t num_bytes =
                recvfrom(d_sockfd, buffer, ac::k_MAX_DATA_SIZE - 1, 0,
                         reinterpret_cast<struct sockaddr *>(&from_addr), &from_addr_len);
            if (num_bytes < 0) {
                static const std::string error_msg = k_LOG_PREFIX + "139 recvfrom";
                perror(error_msg.c_str());
                continue;
            }

            buffer[num_bytes] = '\0';

            std::string message{buffer, static_cast<size_t>(num_bytes)};
            if (message.starts_with(ac::ChatProtocol::k_TOKEN)) {
                d_token = message.substr(ac::ChatProtocol::k_TOKEN.size());
            } else {
                std::cout << "\n <" << message << "> \n" << std::flush;
            }
        }
        std::cout << "> " << std::flush;
    }

    _shutdown();
}
} // namespace available_client