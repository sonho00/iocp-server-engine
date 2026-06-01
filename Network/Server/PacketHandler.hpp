#pragma once

#include <array>
#include <functional>

#include "Network/Common/Protocol.hpp"

class SessionManager;
class AccountManager;

class Session;

class PacketHandler {
   public:
	PacketHandler(SessionManager* sessionManager,
				  AccountManager* accountManager);

	void Execute(Session& session, const PACKET_HEADER& header);

   private:
	bool HandleMove(Session& session, const PACKET_HEADER& header);
	bool HandleChat(Session& session, const PACKET_HEADER& header);
	bool HandleRegister(Session& session, const PACKET_HEADER& header);
	bool HandleLogin(Session& session, const PACKET_HEADER& header);
	bool HandleLogout(Session& session, const PACKET_HEADER& header);

	void WorkerThreadFunc();

	std::array<std::function<bool(Session&, const PACKET_HEADER&)>,
			   static_cast<size_t>(PACKET_ID::kCnt)>
		handlers_;

	SessionManager* sessionManager_ = nullptr;
	AccountManager* accountManager_ = nullptr;
};
