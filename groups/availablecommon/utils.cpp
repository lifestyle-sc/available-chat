#include <string_view>
#include <utils.h>

#include <cstddef>
#include <iomanip>
#include <optional>
#include <sstream>

#include <arpa/inet.h>
#include <array>
#include <iostream>
#include <string>
#include <sys/random.h>

namespace available_common {

void printaddrinfo(addrinfo *const addinfo) {
    char ipstr[INET6_ADDRSTRLEN];
    addrinfo *curr = addinfo;
    while (curr) {
        void *addr;
        std::string ipver;
        struct sockaddr_in *ipv4;
        struct sockaddr_in6 *ipv6;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (curr->ai_family == AF_INET) { // IPv4
            ipv4 = (struct sockaddr_in *)curr->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            ipv6 = (struct sockaddr_in6 *)curr->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(curr->ai_family, addr, ipstr, sizeof(ipstr));
        std::cout << ipver << ": " << ipstr << std::endl;
        curr = curr->ai_next;
    }
}

std::optional<std::string> generate_token() {
    std::array<unsigned char, 16> bytes{};

    std::size_t offset = 0;
    while (offset < bytes.size()) {
        auto n = getrandom(bytes.data() + offset, bytes.size() - offset, 0);
        if (n < 0) {
            return {};
        }
        offset += static_cast<std::size_t>(n);
    }

    std::ostringstream oss;
    for (unsigned char byte : bytes) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }

    return oss.str();
}

std::optional<ParsedMsg> parse_msg_packet(const std::string &prefix, const std::string &packet,
                                          const ParseFlag &flag) {
    if (!packet.starts_with(prefix)) {
        return {};
    }

    std::string_view remaining_packet{packet.data() + prefix.size(), packet.size() - prefix.size()};

    if (flag == ParseFlag::ONLY_TOKEN) {
        return ParsedMsg{.d_token = std::string{remaining_packet}};
    }

    auto token_end = remaining_packet.find(' ');
    if (token_end == std::string_view::npos) {
        return {};
    }

    std::string_view token = remaining_packet.substr(0, token_end);
    std::string_view message = remaining_packet.substr(token_end + 1);
    return ParsedMsg{.d_token = std::string{token}, .d_message = std::string{message}};
}

} // namespace available_common