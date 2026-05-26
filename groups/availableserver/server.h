#ifndef SERVER_H
#define SERVER_H

#include <chrono>
#include <string>
#include <sys/socket.h>
#include <unordered_map>

namespace available_server {

class Server {
    enum class SERVER_ERROR_CODES : int { SUCCESS = 0, FAILURE = -1 };

    struct ClientInfo {
        sockaddr_storage d_client_addr;
        socklen_t d_client_addr_len;
        std::string d_username;
        std::chrono::steady_clock::time_point d_last_seen;
    };

    std::unordered_map<std::string, ClientInfo> d_clients;
    int d_sockfd;

    SERVER_ERROR_CODES _setup();
    void _shutdown();

    void _send_to_client(const ClientInfo &client, const std::string &message);
    void _broadcast(const std::string &sender_key, const std::string &message);

  public:
    Server();

    Server(const Server &) = delete;

    Server &operator=(const Server &) = delete;

    ~Server();

    void run();
};

} // namespace available_server
#endif