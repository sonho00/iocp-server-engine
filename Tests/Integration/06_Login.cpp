#include <gtest/gtest.h>
#include <string.h>

#include "Network/Common/Protocol.hpp"
#include "Tests/Base/Client.hpp"

void CreateC2SRegisterPacket(C2S_REGISTER& packet, const char* id,
							 const char* password) {
	packet.header.size = sizeof(C2S_REGISTER);
	packet.header.id = static_cast<uint8_t>(PACKET_ID::kRegister);
	strncpy_s(packet.id, sizeof(packet.id), id, _TRUNCATE);
	strncpy_s(packet.password, sizeof(packet.password), password, _TRUNCATE);
}

void CreateC2SLoginPacket(C2S_LOGIN& packet, const char* id,
						  const char* password) {
	packet.header.size = sizeof(C2S_LOGIN);
	packet.header.id = static_cast<uint8_t>(PACKET_ID::kLogin);
	strncpy_s(packet.id, sizeof(packet.id), id, _TRUNCATE);
	strncpy_s(packet.password, sizeof(packet.password), password, _TRUNCATE);
}

void CreateC2SLogoutPacket(C2S_LOGOUT& packet) {
	packet.header.size = sizeof(C2S_LOGOUT);
	packet.header.id = static_cast<uint8_t>(PACKET_ID::kLogout);
}

void CheckS2CRegisterPacket(S2C_REGISTER& packet, bool success,
							const char* message) {
	EXPECT_EQ(packet.header.id, static_cast<uint16_t>(PACKET_ID::kRegister));
	EXPECT_EQ(packet.header.size, sizeof(S2C_REGISTER));
	EXPECT_EQ(packet.success, success);
	EXPECT_STREQ(packet.message, message);
}

void CheckS2CLoginPacket(S2C_LOGIN& packet, bool success, const char* message) {
	EXPECT_EQ(packet.header.id, static_cast<uint16_t>(PACKET_ID::kLogin));
	EXPECT_EQ(packet.header.size, sizeof(S2C_LOGIN));
	EXPECT_EQ(packet.success, success);
	EXPECT_STREQ(packet.message, message);
}

void CheckS2CLogoutPacket(S2C_LOGOUT& packet, bool success,
						  const char* message) {
	EXPECT_EQ(packet.header.id, static_cast<uint16_t>(PACKET_ID::kLogout));
	EXPECT_EQ(packet.header.size, sizeof(S2C_LOGOUT));
	EXPECT_EQ(packet.success, success);
	EXPECT_STREQ(packet.message, message);
}

void Test1(Client& client1, Client& client2) {
	C2S_REGISTER sendRegisterPacket{};
	CreateC2SRegisterPacket(sendRegisterPacket, "test_user1", "test_pass1");
	EXPECT_TRUE(client1.SendPacket(
		reinterpret_cast<const PACKET_HEADER&>(sendRegisterPacket)));

	S2C_REGISTER recvRegisterPacket1{};
	EXPECT_TRUE(
		client1.ReceivePacket(reinterpret_cast<char*>(&recvRegisterPacket1)));
	CheckS2CRegisterPacket(recvRegisterPacket1, true,
						   "Registration successful");

	LOG_INFO("Test 1 passed: Registration successful");
}

void Test2(Client& client1, Client& client2) {
	C2S_REGISTER sendRegisterPacket{};
	CreateC2SRegisterPacket(sendRegisterPacket, "test_user1", "test_pass2");
	EXPECT_TRUE(client2.SendPacket(
		reinterpret_cast<const PACKET_HEADER&>(sendRegisterPacket)));

	S2C_REGISTER recvRegisterPacket{};
	EXPECT_TRUE(
		client2.ReceivePacket(reinterpret_cast<char*>(&recvRegisterPacket)));
	CheckS2CRegisterPacket(recvRegisterPacket, false, "ID already exists");

	LOG_INFO("Test 2 passed: Duplicate registration handled");
}

void Test3(Client& client1, Client& client2) {
	C2S_LOGIN sendLoginPacket{};
	CreateC2SLoginPacket(sendLoginPacket, "test_user1", "test_pass2");
	EXPECT_TRUE(client1.SendPacket(
		reinterpret_cast<const PACKET_HEADER&>(sendLoginPacket)));

	S2C_LOGIN recvLoginPacket{};
	EXPECT_TRUE(
		client1.ReceivePacket(reinterpret_cast<char*>(&recvLoginPacket)));
	CheckS2CLoginPacket(recvLoginPacket, false, "Invalid credentials");

	LOG_INFO("Test 3 passed: Login with incorrect credentials handled");
}

void Test4(Client& client1, Client& client2) {
	C2S_LOGIN sendLoginPacket{};
	CreateC2SLoginPacket(sendLoginPacket, "test_user1", "test_pass1");
	EXPECT_TRUE(client1.SendPacket(
		reinterpret_cast<const PACKET_HEADER&>(sendLoginPacket)));

	S2C_LOGIN recvLoginPacket{};
	EXPECT_TRUE(
		client1.ReceivePacket(reinterpret_cast<char*>(&recvLoginPacket)));
	CheckS2CLoginPacket(recvLoginPacket, true, "Login successful");

	LOG_INFO("Test 4 passed: Login with correct credentials handled");
}

void Test5(Client& client1, Client& client2) {
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

TEST(LoginTest, RegisterAndLogin) {
	Client client1, client2;
	client1.Init();
	client2.Init();

	Test1(client1, client2);
	Test2(client1, client2);
	Test3(client1, client2);
	Test4(client1, client2);
	Test5(client1, client2);
}
