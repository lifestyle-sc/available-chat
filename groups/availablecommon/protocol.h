#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>

namespace available_common {

// TODO: Separate protocol into client and server
struct ChatProtocol {
    static constexpr std::string k_MSG = "MSG ";
    static constexpr std::string k_JOIN = "JOIN ";
    static constexpr std::string k_TOKEN = "TOKEN ";
    static constexpr std::string k_LEAVE = "LEAVE ";
    static constexpr std::string k_SYS = "SYS ";
    static constexpr std::string k_CHAT = "CHAT ";
};

} // namespace available_common

#endif