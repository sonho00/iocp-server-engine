#pragma once

#include <WinSock2.h>

#include <cstdint>

namespace Config {
constexpr uint16_t kPort = 12345;
constexpr uint32_t kAcceptAddrSize = sizeof(sockaddr_in) + 16;

constexpr uint32_t kMagicBufferSize = 65536;
constexpr uint32_t kClientBufferSize = 65536;
constexpr uint32_t kChatPacketSize = 256;

constexpr uint32_t kMaxAccept = 1000;
constexpr uint32_t kMaxSession = 4096;
}  // namespace Config
