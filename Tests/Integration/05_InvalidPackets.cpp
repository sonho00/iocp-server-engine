#include <gtest/gtest.h>

#include <cstdint>

#include "Common/Protocol.hpp"
#include "Tests/Base/Client.hpp"

TEST(InvalidPackets, InvalidHeader) {
	Client client;
	client.Init();

	PACKET_HEADER invalidHeader{};
	invalidHeader.size = sizeof(PACKET_HEADER) - 1;	 // Invalid size
	EXPECT_TRUE(client.SendByte((char*)&invalidHeader, sizeof(invalidHeader)));

	char buffer[1024];

	LOG_INFO(
		"Checking that server detects invalid packet size and closes "
		"connection...");
	EXPECT_FALSE(client.ReceiveByte(buffer, sizeof(PACKET_HEADER)));

	invalidHeader.size = sizeof(PACKET_HEADER);
	invalidHeader.id = static_cast<uint16_t>(-1);  // Invalid packet ID
	EXPECT_TRUE(client.SendByte((char*)&invalidHeader, sizeof(invalidHeader)));

	LOG_INFO(
		"Checking that server detects invalid packet ID and closes "
		"connection...");
	EXPECT_FALSE(client.ReceiveByte(buffer, sizeof(PACKET_HEADER)));
}
