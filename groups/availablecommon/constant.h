#ifndef CONSTANT_H
#define CONSTANT_H

#include <string>

namespace available_common {

static constexpr std::string k_PORT = "3040";
static constexpr std::string k_CLIENT = "client";
static constexpr std::string k_SERVER = "server";
static constexpr int k_MAX_DATA_SIZE = 1024;
static constexpr int k_INVALID_SOCKET = -1;

} // namespace available_common

#endif