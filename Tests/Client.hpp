#pragma once

#include <WinSock2.h>

#include <array>

#include "Network/Common/Protocol.hpp"
#include "Network/Common/WSAManager.hpp"

class Client {
   public:
	Client();
	~Client();

	[[nodiscard]] bool SendPacket(const PACKET_HEADER& header) const;
	[[nodiscard]] PACKET_HEADER* ReceivePacket(char* buffer);

	[[nodiscard]] bool SendByte(char* buffer, int len) const;
	[[nodiscard]] bool ReceiveByte(char* buffer, int len) const;

	static bool HandlePacket(const PACKET_HEADER& header);

	virtual void ThreadFunc() = 0;
	virtual bool test() = 0;

	bool success_ = true;

	inline static std::array<char, 1 << 20> sendBuf_;
	inline static std::array<char, 1 << 20> recvBuf_;

	static constexpr const char* kIpAddr_ = "127.0.0.1";
	static constexpr uint16_t kPort_ = 12345;

   private:
	WSAManager wsaManager_;

   protected:
	SOCKET socket_;
};
