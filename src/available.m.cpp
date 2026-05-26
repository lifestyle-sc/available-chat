#include <iostream>

#include <client.h>
#include <constant.h>
#include <server.h>

namespace ac = available_common;
namespace as = available_server;
namespace acl = available_client;

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    std::cout << "Welcome to my setup" << std::endl;

    if (argc < 2) {
        std::cerr << "Incorrect usage: ./net.tsk <appType: server | "
                     "client> <hostname | required "
                     "for client> <message | required for client>"
                  << std::endl;
        return 1;
    }

    if (argv[1] == ac::k_SERVER) {
        auto server = as::Server{};
        server.run();
        std::cout << "Main():27 Server has stopped running!" << std::endl;
    } else if (argv[1] == ac::k_CLIENT) {
        if (argc < 4) {
            std::cerr << "Incorrect usage: ./net.tsk <appType: server | "
                         "client> <hostname | required "
                         "for client> <message | required for client>"
                      << std::endl;
            return 1;
        }

        auto client = acl::Client{argv[2], argv[3]};
        client.run();
        std::cout << "Main():39 Client has stopped running!" << std::endl;
    } else {
        std::cerr << "Incorrect usage: ./net.tsk <appType: server | "
                     "client> <hostname | required "
                     "for client> <message | required for client>"
                  << std::endl;
        return 1;
    }

    return 0;
}