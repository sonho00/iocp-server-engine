#include <gtest/gtest.h>

#include <cstring>

#include "Network/Common/Protocol.hpp"
#include "Tests/Base/Client.hpp"

TEST(StickyPacketsTest, VerifyDataIntegrity) {
	Client client1, client2;
	client1.Init();
	client2.Init();

	C2S_CHAT sendPacket;
	sendPacket.header.id = static_cast<uint16_t>(PACKET_ID::kChat);
	for (int i = 0; i < 200; ++i) {
		sprintf_s(sendPacket.message + i * 4, 5, "%03d ", i);
	}
	sendPacket.header.size = 805;

	std::array<char, 805 * 200> sendBuf{};
	for (int i = 0; i < 200; ++i) {
		memcpy(sendBuf.data() + i * 805, &sendPacket, 805);
	}
	EXPECT_TRUE(client1.SendByte(sendBuf.data(), 805 * 200));

	S2C_CHAT recvPacket;
	for (int i = 0; i < 200; ++i) {
		EXPECT_TRUE(
			client2.ReceivePacket(reinterpret_cast<char*>(&recvPacket)));
		EXPECT_EQ(recvPacket.header.id,
				  static_cast<uint16_t>(PACKET_ID::kChat));
		EXPECT_EQ(recvPacket.header.size,
				  805 + sizeof(recvPacket.sessionHandle));
		EXPECT_EQ(memcmp(recvPacket.message, sendPacket.message, 800), 0);
	}
}
