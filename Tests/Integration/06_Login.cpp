#include <gtest/gtest.h>
#include <string.h>

#include "Common/Protocol.hpp"
#include "Tests/Base/Client.hpp"

PacketBlock CreateRegisterPacket(const char* id, const char* password) {
	PacketBlock buffer{};
	auto& packet = reinterpret_cast<C2S_REGISTER&>(*buffer.data());
	packet.header.size = sizeof(C2S_REGISTER);
	packet.header.id = static_cast<uint8_t>(PACKET_ID::kRegister);
	strncpy_s(packet.id, sizeof(packet.id), id, _TRUNCATE);
	strncpy_s(packet.password, sizeof(packet.password), password, _TRUNCATE);
	return buffer;
}

PacketBlock CreateLoginPacket(const char* id, const char* password) {
	PacketBlock buffer{};
	auto& packet = reinterpret_cast<C2S_LOGIN&>(*buffer.data());
	packet.header.size = sizeof(C2S_LOGIN);
	packet.header.id = static_cast<uint8_t>(PACKET_ID::kLogin);
	strncpy_s(packet.id, sizeof(packet.id), id, _TRUNCATE);
	strncpy_s(packet.password, sizeof(packet.password), password, _TRUNCATE);
	return buffer;
}

PacketBlock CreateLogoutPacket() {
	PacketBlock buffer{};
	auto& packet = reinterpret_cast<C2S_LOGOUT&>(*buffer.data());
	packet.header.size = sizeof(C2S_LOGOUT);
	packet.header.id = static_cast<uint8_t>(PACKET_ID::kLogout);
	return buffer;
}

void CheckRegisterPacket(S2C_REGISTER& packet, bool success,
						 const char* message) {
	EXPECT_EQ(packet.header.id, static_cast<uint16_t>(PACKET_ID::kRegister));
	EXPECT_EQ(packet.header.size, sizeof(S2C_REGISTER) + strlen(message) + 1);
	EXPECT_EQ(packet.success, success);
	EXPECT_STREQ(packet.message, message);
}

void CheckLoginPacket(S2C_LOGIN& packet, bool success, const char* message) {
	EXPECT_EQ(packet.header.id, static_cast<uint16_t>(PACKET_ID::kLogin));
	EXPECT_EQ(packet.header.size, sizeof(S2C_LOGIN) + strlen(message) + 1);
	EXPECT_EQ(packet.success, success);
	EXPECT_STREQ(packet.message, message);
}

void CheckLogoutPacket(S2C_LOGOUT& packet, bool success, const char* message) {
	EXPECT_EQ(packet.header.id, static_cast<uint16_t>(PACKET_ID::kLogout));
	EXPECT_EQ(packet.header.size, sizeof(S2C_LOGOUT) + strlen(message) + 1);
	EXPECT_EQ(packet.success, success);
	EXPECT_STREQ(packet.message, message);
}

void Test1(Client& client1, Client& client2) {
	PacketBlock sendPacket = CreateRegisterPacket("test_user1", "test_pass1");
	EXPECT_TRUE(
		client1.SendPacket(reinterpret_cast<const PACKET_HEADER&>(sendPacket)));

	PacketBlock buffer{};
	auto& recvPacket = reinterpret_cast<S2C_REGISTER&>(*buffer.data());
	EXPECT_TRUE(client1.ReceivePacket(reinterpret_cast<char*>(&recvPacket)));
	CheckRegisterPacket(recvPacket, true, "Registration successful");

	LOG_INFO("Test 1 passed: Registration successful");
}

void Test2(Client& client1, Client& client2) {
	PacketBlock sendPacket = CreateRegisterPacket("test_user1", "test_pass2");
	EXPECT_TRUE(
		client2.SendPacket(reinterpret_cast<const PACKET_HEADER&>(sendPacket)));

	PacketBlock buffer{};
	auto& recvPacket = reinterpret_cast<S2C_REGISTER&>(*buffer.data());
	EXPECT_TRUE(client2.ReceivePacket(reinterpret_cast<char*>(&recvPacket)));
	CheckRegisterPacket(recvPacket, false, "ID already exists");

	LOG_INFO("Test 2 passed: Duplicate registration handled");
}

void Test3(Client& client1, Client& client2) {
	PacketBlock sendPacket = CreateLoginPacket("test_user1", "test_pass2");
	EXPECT_TRUE(
		client1.SendPacket(reinterpret_cast<const PACKET_HEADER&>(sendPacket)));

	PacketBlock buffer{};
	auto& recvPacket = reinterpret_cast<S2C_LOGIN&>(*buffer.data());
	EXPECT_TRUE(client1.ReceivePacket(reinterpret_cast<char*>(&recvPacket)));
	CheckLoginPacket(recvPacket, false, "Invalid credentials");

	LOG_INFO("Test 3 passed: Login with incorrect credentials handled");
}

void Test4(Client& client1, Client& client2) {
	PacketBlock sendPacket = CreateLoginPacket("test_user1", "test_pass1");
	EXPECT_TRUE(
		client1.SendPacket(reinterpret_cast<const PACKET_HEADER&>(sendPacket)));

	PacketBlock buffer{};
	auto& recvPacket = reinterpret_cast<S2C_LOGIN&>(*buffer.data());
	EXPECT_TRUE(client1.ReceivePacket(reinterpret_cast<char*>(&recvPacket)));
	CheckLoginPacket(recvPacket, true, "Login successful");

	LOG_INFO("Test 4 passed: Login with correct credentials handled");
}

void Test5(Client& client1, Client& client2) {
	PacketBlock sendPacket = CreateLogoutPacket();
	EXPECT_TRUE(
		client1.SendPacket(reinterpret_cast<const PACKET_HEADER&>(sendPacket)));

	PacketBlock buffer{};
	auto& recvPacket = reinterpret_cast<S2C_LOGOUT&>(*buffer.data());
	EXPECT_TRUE(client1.ReceivePacket(reinterpret_cast<char*>(&recvPacket)));
	CheckLogoutPacket(recvPacket, true, "Logout successful");

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
