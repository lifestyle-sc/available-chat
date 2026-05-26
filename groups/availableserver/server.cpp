#include "utils.h"
#include <server.h>

#include <constant.h>
#include <protocol.h>

#include <arpa/inet.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace ac = available_common;

namespace available_server {

namespace {
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(reinterpret_cast<struct sockaddr_in *>(sa)->sin_addr);
    }
    return &(reinterpret_cast<struct sockaddr_in6 *>(sa)->sin6_addr);
}

std::uint16_t get_in_port(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return reinterpret_cast<struct sockaddr_in *>(sa)->sin_port;
    }
    return reinterpret_cast<struct sockaddr_in6 *>(sa)->sin6_port;
}
} // namespace

auto Server::_setup() -> SERVER_ERROR_CODES {
    static const std::string k_LOG_PREFIX = "SERVER:_setup():";
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (auto rv = getaddrinfo(nullptr, ac::k_PORT.c_str(), &hints, &servinfo); rv == -1) {
        std::cerr << k_LOG_PREFIX << "21 Error occurred " << gai_strerror(rv) << std::endl;
        return SERVER_ERROR_CODES::FAILURE;
    }

    struct addrinfo *connectedinfo = servinfo;
    for (; connectedinfo != nullptr; connectedinfo = connectedinfo->ai_next) {
        if (d_sockfd = socket(connectedinfo->ai_family, connectedinfo->ai_socktype,
                              connectedinfo->ai_protocol);
            d_sockfd == -1) {
            static const std::string error_msg = k_LOG_PREFIX + "31 socket";
            perror(error_msg.c_str());
            continue;
        }

        if (bind(d_sockfd, connectedinfo->ai_addr, connectedinfo->ai_addrlen) == -1) {
            static const std::string error_msg = k_LOG_PREFIX + "37 bind";
            _shutdown();
            perror(error_msg.c_str());
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (connectedinfo == nullptr) {
        std::cerr << k_LOG_PREFIX << "48 Unable to bind to a socket" << std::endl;
        return SERVER_ERROR_CODES::FAILURE;
    }

    std::cout << k_LOG_PREFIX << "52 waiting to listen from...." << std::endl;
    return SERVER_ERROR_CODES::SUCCESS;
}

void Server::_shutdown() {
    if (d_sockfd != ac::k_INVALID_SOCKET) {
        close(d_sockfd);
        d_sockfd = ac::k_INVALID_SOCKET;
    }
}

void Server::_send_to_client(const ClientInfo &client, const std::string &message) {
    static const std::string k_LOG_PREFIX = "CLIENT:run():";

    auto send_bytes = sendto(d_sockfd, message.data(), message.size(), 0,
                             reinterpret_cast<const struct sockaddr *>(&client.d_client_addr),
                             client.d_client_addr_len);

    if (send_bytes < 0) {
        static const std::string error_msg = k_LOG_PREFIX + " sendto";
        perror(error_msg.c_str());
    }
}

void Server::_broadcast(const std::string &sender_key, const std::string &message) {
    for (auto &[client_key, client_info] : d_clients) {
        if (sender_key != client_key) {
            _send_to_client(client_info, message);
        }
    }
}

Server::Server() : d_sockfd(ac::k_INVALID_SOCKET) {
    if (_setup() == SERVER_ERROR_CODES::FAILURE) {
        _shutdown();
    }
}

Server::~Server() { _shutdown(); }

void Server::run() {
    if (d_sockfd == ac::k_INVALID_SOCKET) {
        return;
    }

    static const std::string k_LOG_PREFIX = "SERVER:run():";

    char buf[ac::k_MAX_DATA_SIZE];
    while (true) {
        struct sockaddr_storage client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        auto num_bytes =
            recvfrom(d_sockfd, buf, ac::k_MAX_DATA_SIZE - 1, 0,
                     reinterpret_cast<struct sockaddr *>(&client_addr), &client_addr_len);
        if (num_bytes < 0) {
            static const std::string error_msg = k_LOG_PREFIX + "130 recvfrom";
            perror(error_msg.c_str());
            continue;
        }

        auto now = std::chrono::steady_clock::now();

        buf[num_bytes] = '\0';
        auto packet = std::string{buf, static_cast<std::size_t>(num_bytes)};

        // Handle different message
        if (packet.starts_with(ac::ChatProtocol::k_JOIN)) {
            std::string username = packet.substr(5);
            if (username.empty()) {
                // send invalid username msg
                ClientInfo tmp_client = {.d_client_addr = client_addr,
                                         .d_client_addr_len = client_addr_len,
                                         .d_last_seen = now};
                static const std::string msg =
                    ac::ChatProtocol::k_SYS + "username length should be greater than one";
                _send_to_client(tmp_client, msg);
                continue;
            }

            auto token = ac::generate_token();
            if (!token) {
                // send error occurred
                ClientInfo tmp_client = {.d_client_addr = client_addr,
                                         .d_client_addr_len = client_addr_len,
                                         .d_last_seen = now};
                static const std::string msg =
                    ac::ChatProtocol::k_SYS + "system error occured, try again later!";
                _send_to_client(tmp_client, msg);
                continue;
            }

            ClientInfo tmp_client = {.d_client_addr = client_addr,
                                     .d_client_addr_len = client_addr_len,
                                     .d_username = username,
                                     .d_last_seen = now};
            d_clients[*token] = tmp_client;

            std::cout << k_LOG_PREFIX << "162 user " << username << " has joined the chat"
                      << std::endl;

            _send_to_client(tmp_client, ac::ChatProtocol::k_TOKEN + *token);
            _send_to_client(tmp_client, ac::ChatProtocol::k_SYS + "welcome!");
            _broadcast(*token,
                       ac::ChatProtocol::k_SYS + "user " + username + " has joined the chat");
        } else if (packet.starts_with(ac::ChatProtocol::k_MSG)) {
            auto parsed_msg = ac::parse_msg_packet(ac::ChatProtocol::k_MSG, packet);
            if (!parsed_msg || parsed_msg->d_token.empty()) {
                ClientInfo tmp_client = {.d_client_addr = client_addr,
                                         .d_client_addr_len = client_addr_len,
                                         .d_last_seen = now};
                _send_to_client(tmp_client, ac::ChatProtocol::k_SYS +
                                                "Invalid packet, attach token to packet!");
                continue;
            }

            const std::string &message = parsed_msg->d_message;
            const std::string &token = parsed_msg->d_token;
            if (!d_clients.contains(token)) {
                ClientInfo tmp_client = {.d_client_addr = client_addr,
                                         .d_client_addr_len = client_addr_len,
                                         .d_last_seen = now};
                _send_to_client(tmp_client, ac::ChatProtocol::k_SYS +
                                                "you need to join, before sending a message");
                continue;
            }
            // if (message.size() == 0) {
            //     _send_to_client(*(reinterpret_cast<struct sockaddr *>(&client_addr)),
            //                     ac::ChatProtocol::k_SYS +
            //                         "message length should be greater than one");
            //     continue;
            // }

            std::cout << k_LOG_PREFIX << "162 user " << d_clients[token].d_username
                      << " sent msg: " << message << std::endl;
            d_clients[token].d_client_addr = client_addr;
            d_clients[token].d_client_addr_len = client_addr_len;
            d_clients[token].d_last_seen = now;
            _broadcast(token, ac::ChatProtocol::k_CHAT + d_clients[token].d_username +
                                  " sent: " + message);
        } else if (packet.starts_with(ac::ChatProtocol::k_LEAVE)) {
            auto parsed_msg =
                ac::parse_msg_packet(ac::ChatProtocol::k_LEAVE, packet, ac::ParseFlag::ONLY_TOKEN);
            if (!parsed_msg || parsed_msg->d_token.empty()) {
                std::cout << "Invalid packet, attach token to packet: " << packet << std::endl;
                ClientInfo tmp_client = {.d_client_addr = client_addr,
                                         .d_client_addr_len = client_addr_len,
                                         .d_last_seen = now};
                _send_to_client(tmp_client, ac::ChatProtocol::k_SYS +
                                                "Invalid packet, attach token to packet!");
                continue;
            }

            const std::string &message = parsed_msg->d_message;
            const std::string &token = parsed_msg->d_token;
            if (d_clients.contains(token)) {
                std::cout << k_LOG_PREFIX << "177 User " << d_clients[token].d_username
                          << " has left the chat" << std::endl;
                _broadcast(token, ac::ChatProtocol::k_SYS + "user " + d_clients[token].d_username +
                                      " has left the chat");
                _send_to_client(d_clients[token], ac::ChatProtocol::k_SYS + "you've left the chat");
                d_clients.erase(token);
            }
        } else {
            std::cerr << k_LOG_PREFIX << "186 Invalid packet received" << std::endl;
            // Send a message to client
            static const std::string msg =
                ac::ChatProtocol::k_SYS +
                " received invalid packet, send supported packet type (JOIN | MSG | LEAVE)";
            ClientInfo tmp_client = {.d_client_addr = client_addr,
                                     .d_client_addr_len = client_addr_len,
                                     .d_last_seen = now};
            _send_to_client(tmp_client, msg);
        }
    }

    _shutdown();
}

} // namespace available_server