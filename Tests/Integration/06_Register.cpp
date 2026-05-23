#include <gtest/gtest.h>

#include <cstring>

#include "Network/Common/Protocol.hpp"
#include "Tests/Base/Client.hpp"

void CreateC2SRegisterPacket(C2S_REGISTER& packet, const char* id,
							 const char* password) {
	packet.header.size = sizeof(C2S_REGISTER);
	packet.header.id = static_cast<uint8_t>(C2S_PACKET_ID::kRegister);
	strncpy(packet.id, id, sizeof(packet.id) - 1);
	strncpy(packet.password, password, sizeof(packet.password) - 1);
}

void CheckS2CRegisterPacket(S2C_REGISTER& packet, bool success,
							const char* message) {
	EXPECT_EQ(packet.header.id,
			  static_cast<uint16_t>(S2C_PACKET_ID::kRegister));
	EXPECT_EQ(packet.header.size, sizeof(S2C_REGISTER));
	EXPECT_EQ(packet.success, success);
	EXPECT_STREQ(packet.message, message);
}

TEST(RegisterTest, RegistrationFlow) {
	Client client1, client2;
	client1.Init();
	client2.Init();

	C2S_REGISTER sendRegisterPacket1{};
	CreateC2SRegisterPacket(sendRegisterPacket1, "test_user1", "test_pass1");
	EXPECT_TRUE(client1.SendPacket(
		reinterpret_cast<const PACKET_HEADER&>(sendRegisterPacket1)));

	S2C_REGISTER recvRegisterPacket1{};
	EXPECT_TRUE(
		client1.ReceivePacket(reinterpret_cast<char*>(&recvRegisterPacket1)));
	CheckS2CRegisterPacket(recvRegisterPacket1, true,
						   "Registration successful");

	LOG_INFO("Test 1 passed: Registration successful");

	C2S_REGISTER sendRegisterPacket2{};
	CreateC2SRegisterPacket(sendRegisterPacket2, "test_user1", "test_pass2");
	EXPECT_TRUE(client2.SendPacket(
		reinterpret_cast<const PACKET_HEADER&>(sendRegisterPacket2)));

	S2C_REGISTER recvRegisterPacket2{};
	EXPECT_TRUE(
		client2.ReceivePacket(reinterpret_cast<char*>(&recvRegisterPacket2)));
	CheckS2CRegisterPacket(recvRegisterPacket2, false, "ID already exists");

	LOG_INFO("Test 2 passed: Duplicate registration handled");
}
