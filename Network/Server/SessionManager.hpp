#pragma once

#include <array>
#include <vector>

#include "Network/Common/Pool/SharedPoolPtr.hpp"
#include "Network/Common/Pool/SparsePool.hpp"
#include "Session.hpp"

class Account;
class AccountManager;
class IocpCore;
class Listener;
class PacketHandler;

class SessionManager {
   public:
	bool Init(IocpCore& iocpCore, Listener& listener,
			  AccountManager& accountManager, PacketHandler& packetHandler);
	bool RegisterSession(uint64_t handle);

	SharedPoolPtr<Session> CreateSession();
	void ConnectSession(uint64_t handle);
	void DisconnectSession(uint64_t handle);

	bool LogInSession(uint64_t handle, const Account& account);
	void LogOutSession(uint64_t handle);

	SharedPoolPtr<Session> GetSession(uint64_t handle);
	std::vector<uint64_t> GetSessionsInState(SessionState state);

	SessionState GetState(uint64_t handle);
	void SetState(uint64_t handle, SessionState newState);

	[[nodiscard]] AccountManager* GetAccountManager() const {
		return accountManager_;
	}

   private:
	SparsePool<Session, Config::kMaxSession,
			   static_cast<size_t>(SessionState::kCnt)>
		sessionPool_;
	std::array<SharedPoolPtr<Session>, Config::kMaxSession> sessionPtrs_;

	AccountManager* accountManager_ = nullptr;
	IocpCore* iocpCore_ = nullptr;
	PacketHandler* packetHandler_ = nullptr;
};
