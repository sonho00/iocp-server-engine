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
	bool HandleC2S_MOVE(Session& session, const PACKET_HEADER& header);
	bool HandleC2S_CHAT(Session& session, const PACKET_HEADER& header);
	bool HandleC2S_REGISTER(Session& session, const PACKET_HEADER& header);
	bool HandleC2S_LOGIN(Session& session, const PACKET_HEADER& header);
	bool HandleC2S_LOGOUT(Session& session, const PACKET_HEADER& header);

	void WorkerThreadFunc();

	std::array<std::function<bool(Session&, const PACKET_HEADER&)>,
			   static_cast<size_t>(C2S_PACKET_ID::kCnt)>
		handlers_;

	SessionManager* sessionManager_ = nullptr;
	AccountManager* accountManager_ = nullptr;
};
