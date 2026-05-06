#pragma once

#include <array>
#include <thread>
#include <vector>

#include "Network/Common/Logger.hpp"
#include "Tests/Client.hpp"

class ConnectionStress : public Client {
   public:
	void ThreadFunc() override {
		// 1000개의 클라이언트가 동시에 접속, 종료
		// 실패시 throw exception
		std::array<ConnectionStress, 1000> clients;
	}

	bool test() override {
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
		return true;
	}
};
