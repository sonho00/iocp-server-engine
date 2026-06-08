#pragma once

#include <WinSock2.h>

#include <cstdint>

namespace Config {
constexpr uint16_t kPort = 12345;
constexpr uint32_t kAcceptAddrSize = sizeof(sockaddr_in) + 16;

constexpr uint32_t kBufferSize = 65536;
constexpr uint32_t kMaxPacketSize = 1024;

constexpr uint32_t kMaxAccept = 1000;
constexpr uint32_t kMaxSession = 4096;

constexpr uint32_t kIdLength = 32;
constexpr uint32_t kPasswordLength = 32;

constexpr uint32_t kWorkerThreadCount = 1;
}  // namespace Config
