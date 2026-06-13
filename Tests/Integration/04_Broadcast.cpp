#include <gtest/gtest.h>
#include <string.h>

#include "Network/Common/Protocol.hpp"
#include "Tests/Base/Client.hpp"

TEST(BroadcastTest, BroadcastMessageToAllClients) {
	Client client1, client2, client3;
	client1.Init();
	client2.Init();
	client3.Init();

	PacketBlock sendBuffer{};
	auto& sendPacket = reinterpret_cast<C2S_CHAT&>(*sendBuffer.data());
	const char* testMessage = "Hello, everyone!";
	size_t messageLength = strlen(testMessage) + 1;
	strcpy_s(sendPacket.message, messageLength, testMessage);
	sendPacket.header.id = static_cast<uint16_t>(PACKET_ID::kChat);
	sendPacket.header.size =
		sizeof(sendPacket.header) + strlen(sendPacket.message) + 1;
	EXPECT_TRUE(client3.SendByte(reinterpret_cast<char*>(&sendPacket),
								 sendPacket.header.size));

	PacketBlock recvBuffer1{}, recvBuffer2{};
	auto& recvPacket1 = reinterpret_cast<S2C_CHAT&>(*recvBuffer1.data());
	auto& recvPacket2 = reinterpret_cast<S2C_CHAT&>(*recvBuffer2.data());
	EXPECT_TRUE(client1.ReceivePacket(reinterpret_cast<char*>(&recvPacket1)));
	EXPECT_TRUE(client2.ReceivePacket(reinterpret_cast<char*>(&recvPacket2)));

	size_t expectedSize =
		sendPacket.header.size + sizeof(recvPacket1.sessionHandle);
	EXPECT_EQ(recvPacket1.header.id, static_cast<uint16_t>(PACKET_ID::kChat));
	EXPECT_EQ(recvPacket2.header.id, static_cast<uint16_t>(PACKET_ID::kChat));
	EXPECT_EQ(recvPacket1.header.size, expectedSize);
	EXPECT_EQ(recvPacket2.header.size, expectedSize);
	EXPECT_STREQ(recvPacket1.message, "Hello, everyone!");
	EXPECT_STREQ(recvPacket2.message, "Hello, everyone!");

	// 클라이언트 3을 논블로킹 모드로 설정
	unsigned long mode = 1;
	ioctlsocket(client3.socket_, FIONBIO, &mode);

	// 클라이언트 3이 브로드캐스트 메시지를 받지 못하는지 확인
	char test;
	LOG_INFO("Checking that client3 does not receive the broadcast message...");
	EXPECT_FALSE(client3.ReceiveByte(&test, 1));
}
