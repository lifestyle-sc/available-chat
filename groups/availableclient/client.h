#ifndef CLIENT_H
#define CLIENT_H

#include <netdb.h>
#include <string>

namespace available_client {
class Client {
    enum class CLIENT_ERROR_CODES : int { SUCCESS = 0, FAILURE = -1 };

    std::string d_username;
    std::string d_token;
    struct addrinfo *d_servinfo;
    struct addrinfo *d_connectedinfo;
    int d_sockfd;

    CLIENT_ERROR_CODES _setup(const std::string &hostname, const std::string &message);

    void _shutdown();

    void _send_msg(struct addrinfo *connectedinfo, const std::string &message);

  public:
    Client(const std::string &hostname, const std::string &message);

    ~Client();

    void run();
};
} // namespace available_client

#endif