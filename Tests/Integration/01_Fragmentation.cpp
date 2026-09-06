#include <gtest/gtest.h>

#include "Common/Protocol.hpp"
#include "Tests/Base/Client.hpp"

TEST(FragmentationTest, HandleFragmentedPacket) {
	Client client1, client2;
	client1.Init();
	client2.Init();

	PacketBlock sendBuffer{};
	auto& sendPacket = reinterpret_cast<C2S_CHAT&>(*sendBuffer.data());
	sendPacket.header.id = static_cast<uint16_t>(PACKET_ID::kChat);
	sendPacket.header.size = 805;
	for (int i = 0; i < 200; ++i) {
		sprintf_s(sendPacket.message + i * 4, 5, "%03d ", i);
	}

	for (int i = 0; i < sendPacket.header.size; ++i) {
		EXPECT_TRUE(
			client1.SendByte(reinterpret_cast<char*>(&sendPacket) + i, 1));
	}

	PacketBlock recvBuffer{};
	auto& recvPacket = reinterpret_cast<S2C_CHAT&>(*recvBuffer.data());
	EXPECT_TRUE(client2.ReceivePacket(reinterpret_cast<char*>(&recvPacket)));
	EXPECT_EQ(recvPacket.header.id, static_cast<uint16_t>(PACKET_ID::kChat));
	EXPECT_EQ(recvPacket.header.size,
			  sendPacket.header.size + sizeof(recvPacket.sessionHandle));
	EXPECT_STREQ(recvPacket.message, sendPacket.message);
}
