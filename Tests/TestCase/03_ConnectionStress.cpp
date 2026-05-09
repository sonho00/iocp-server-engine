#include <gtest/gtest.h>

#include <array>
#include <thread>
#include <vector>

#include "Network/Common/Logger.hpp"
#include "Tests/Client.hpp"

class ConnectionStress : public Client {
   public:
	void ThreadFunc() override {
		// 1000개의 클라이언트가 동시에 접속, 종료
		std::array<ConnectionStress, 1000> clients;
	}

	bool test() override {
		try {
			std::vector<std::thread> clientThreads;
			clientThreads.reserve(1000);
			for (int i = 0; i < 1000; ++i) {
				clientThreads.emplace_back(&ConnectionStress::ThreadFunc, this);
			}
			int cnt = 0;
			for (auto& thread : clientThreads) {
				thread.join();
				++cnt;
				if (cnt % 100 == 0) LOG_INFO("Joined {} threads", cnt);
			}
		} catch (const std::exception& e) {
			LOG_ERROR("Exception in ConnectionStress test: {}", e.what());
			return false;
		}
		return true;
	}
};

TEST(NetworkTest, ConnectionStress) {
	ConnectionStress client;
	EXPECT_TRUE(client.test());
}
