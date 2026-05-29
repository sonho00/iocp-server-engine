#pragma once

#include <array>
#include <cstdint>

#include "Network/Common/Config.hpp"
#include "Network/Common/Pool/SharedPoolPtr.hpp"
#include "Network/Common/Pool/SparsePool.hpp"
#include "Network/Common/Protocol.hpp"
#include "Session.hpp"

class IocpCore;
class Listener;
class AccountManager;
class Account;

class SessionManager {
   public:
	bool Init(IocpCore& iocpCore, Listener& listener,
			  AccountManager& accountManager);
	bool RegisterSession(uint64_t handle);

	SharedPoolPtr<Session> CreateSession();
	bool ConnectSession(uint64_t handle);
	void DisconnectSession(uint64_t handle);

	bool SendToSession(uint64_t handle, const PACKET_HEADER& header);
	bool Broadcast(const PACKET_HEADER& header, uint64_t sessionHandle);

	bool LogInSession(uint64_t handle, const Account& account);
	bool LogOutSession(uint64_t handle);

	SharedPoolPtr<Session> GetSession(uint64_t handle);

	SessionState GetState(uint64_t handle);
	bool SetState(uint64_t handle, SessionState newState);

	[[nodiscard]] AccountManager* GetAccountManager() const {
		return accountManager_;
	}

   private:
	IocpCore* iocpCore_ = nullptr;
	SparsePool<Session, Config::kMaxSession,
			   static_cast<size_t>(SessionState::kCnt)>
		sessionPool_;
	std::array<SharedPoolPtr<Session>, Config::kMaxSession> sessionPtrs_;
	AccountManager* accountManager_ = nullptr;
};
