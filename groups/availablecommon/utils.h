#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <netdb.h>
#include <optional>
#include <string>

namespace available_common {

struct ParsedMsg {
    std::string d_token;
    std::string d_message;
};

enum class ParseFlag : std::uint8_t { ONLY_TOKEN = 0, TOKEN_AND_MSG = 1 };

void printaddrinfo(addrinfo *const addinfo);

std::optional<std::string> generate_token();

std::optional<ParsedMsg> parse_msg_packet(const std::string &prefix, const std::string &packet,
                                          const ParseFlag &flag = ParseFlag::TOKEN_AND_MSG);

} // namespace available_common

#endif