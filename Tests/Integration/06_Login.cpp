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

void CreateC2SLoginPacket(C2S_LOGIN& packet, const char* id,
						  const char* password) {
	packet.header.size = sizeof(C2S_LOGIN);
	packet.header.id = static_cast<uint8_t>(C2S_PACKET_ID::kLogin);
	strncpy(packet.id, id, sizeof(packet.id) - 1);
	strncpy(packet.password, password, sizeof(packet.password) - 1);
}

void CreateC2SLogoutPacket(C2S_LOGOUT& packet) {
	packet.header.size = sizeof(C2S_LOGOUT);
	packet.header.id = static_cast<uint8_t>(C2S_PACKET_ID::kLogout);
}

void CheckS2CRegisterPacket(S2C_REGISTER& packet, bool success,
							const char* message) {
	EXPECT_EQ(packet.header.id,
			  static_cast<uint16_t>(S2C_PACKET_ID::kRegister));
	EXPECT_EQ(packet.header.size, sizeof(S2C_REGISTER));
	EXPECT_EQ(packet.success, success);
	EXPECT_STREQ(packet.message, message);
}

void CheckS2CLoginPacket(S2C_LOGIN& packet, bool success, const char* message) {
	EXPECT_EQ(packet.header.id, static_cast<uint16_t>(S2C_PACKET_ID::kLogin));
	EXPECT_EQ(packet.header.size, sizeof(S2C_LOGIN));
	EXPECT_EQ(packet.success, success);
	EXPECT_STREQ(packet.message, message);
}

void CheckS2CLogoutPacket(S2C_LOGOUT& packet, bool success,
						  const char* message) {
	EXPECT_EQ(packet.header.id, static_cast<uint16_t>(S2C_PACKET_ID::kLogout));
	EXPECT_EQ(packet.header.size, sizeof(S2C_LOGOUT));
	EXPECT_EQ(packet.success, success);
	EXPECT_STREQ(packet.message, message);
}

TEST(LoginTest, RegisterAndLogin) {
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

	C2S_LOGIN sendLoginPacket1{};
	CreateC2SLoginPacket(sendLoginPacket1, "test_user1", "test_pass2");
	EXPECT_TRUE(client1.SendPacket(
		reinterpret_cast<const PACKET_HEADER&>(sendLoginPacket1)));

	S2C_LOGIN recvLoginPacket1{};
	EXPECT_TRUE(
		client1.ReceivePacket(reinterpret_cast<char*>(&recvLoginPacket1)));
	CheckS2CLoginPacket(recvLoginPacket1, false, "Invalid credentials");

	LOG_INFO("Test 3 passed: Login with incorrect credentials handled");

	C2S_LOGIN sendLoginPacket2{};
	CreateC2SLoginPacket(sendLoginPacket2, "test_user1", "test_pass1");
	EXPECT_TRUE(client1.SendPacket(
		reinterpret_cast<const PACKET_HEADER&>(sendLoginPacket2)));

	S2C_LOGIN recvLoginPacket2{};
	EXPECT_TRUE(
		client1.ReceivePacket(reinterpret_cast<char*>(&recvLoginPacket2)));
	CheckS2CLoginPacket(recvLoginPacket2, true, "Login successful");

	LOG_INFO("Test 4 passed: Login with correct credentials handled");

	C2S_LOGOUT sendLogoutPacket{};
	CreateC2SLogoutPacket(sendLogoutPacket);
	EXPECT_TRUE(client1.SendPacket(
		reinterpret_cast<const PACKET_HEADER&>(sendLogoutPacket)));

	S2C_LOGOUT recvLogoutPacket{};
	EXPECT_TRUE(
		client1.ReceivePacket(reinterpret_cast<char*>(&recvLogoutPacket)));
	CheckS2CLogoutPacket(recvLogoutPacket, true, "Logout successful");

	LOG_INFO("Test 5 passed: Logout handled");
}
