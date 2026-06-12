#pragma once

#include <array>
#include <functional>
#include <thread>
#include <vector>

#include "Network/Common/Config.hpp"
#include "Network/Common/Pool/SparsePool.hpp"
#include "Network/Common/Protocol.hpp"
#include "TaskQueue.hpp"

class SessionManager;
class AccountManager;

class Session;

class PacketHandler {
   public:
	PacketHandler(SessionManager* sessionManager,
				  AccountManager* accountManager);
	~PacketHandler();

	void PostTask(std::function<void()> task);
	void Execute(Session& session, const PACKET_HEADER& header);

	SharedPoolPtr<PacketBlock> AcquirePacket(const PACKET_HEADER& header);
	SharedPoolPtr<PacketBlock> AcquirePacket();

   private:
	void HandleMove(Session& session, const PACKET_HEADER& header);
	void HandleChat(Session& session, const PACKET_HEADER& header);
	void HandleRegister(Session& session, const PACKET_HEADER& header);
	void HandleLogin(Session& session, const PACKET_HEADER& header);
	void HandleLogout(Session& session, const PACKET_HEADER& header);

	void WorkerThreadFunc();

	std::array<std::function<void(Session&, const PACKET_HEADER&)>,
			   static_cast<size_t>(PACKET_ID::kCnt)>
		handlers_;
	std::vector<std::thread> workerThreads_;
	TaskQueue taskQueue_;

	SparsePool<PacketBlock, Config::kMaxPacketCount> packetPool_;

	SessionManager* sessionManager_ = nullptr;
	AccountManager* accountManager_ = nullptr;
};
