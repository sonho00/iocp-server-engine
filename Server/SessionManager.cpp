#include "SessionManager.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "Account.hpp"
#include "AccountManager.hpp"
#include "Common/Logger.hpp"
#include "Common/Pool/SharedPoolPtr.hpp"
#include "Common/Pool/SparsePool.hpp"
#include "IocpCore.hpp"
#include "Listener.hpp"
#include "PacketHandler.hpp"
#include "Session.hpp"


bool SessionManager::Init(IocpCore& iocpCore, Listener& listener,
						  AccountManager& accountManager,
						  PacketHandler& packetHandler) {
	iocpCore_ = &iocpCore;
	accountManager_ = &accountManager;
	packetHandler_ = &packetHandler;
	sessionPool_.SetPostReleaseFunc([&listener] { listener.PostAccept(); });

	std::vector<uint64_t> handles = GetSessionsInState(SessionState::kIdle);

	return std::ranges::all_of(handles, [this](uint64_t handle) {
		Session* sessionPtr = sessionPool_.GetObj(handle);
		if (!iocpCore_->Register(sessionPtr->socket_, handle)) {
			LOG_ERROR("Failed to register accept socket with IOCP");
			return false;
		}
		return true;
	});
}

bool SessionManager::RegisterSession(uint64_t handle) {
	Session* sessionPtr = sessionPool_.GetObj(handle);
	if (!iocpCore_->Register(sessionPtr->socket_, handle)) {
		LOG_ERROR("Failed to register accept socket with IOCP");
		return false;
	}
	return true;
}

SharedPoolPtr<Session> SessionManager::CreateSession() {
	SharedPoolPtr<Session> sessionPtr =
		sessionPool_.Acquire(static_cast<size_t>(SessionState::kPending));
	if (!sessionPtr.IsValid()) {
		LOG_WARN("Failed to acquire session from pool");
		return nullptr;
	}

	uint64_t handle = sessionPtr.GetHandle();
	sessionPtr->sessionManager_ = this;
	sessionPtr->packetHandler_ = packetHandler_;
	sessionPtr->handle_ = handle;
	auto idx = static_cast<uint32_t>(handle);
	sessionPtrs_[idx] = std::move(sessionPtr);
	return sessionPtrs_[idx];
}

void SessionManager::ConnectSession(uint64_t handle) {
	sessionPool_.MoveToState(handle,
							 static_cast<size_t>(SessionState::kConnected));
}

void SessionManager::DisconnectSession(uint64_t handle) {
	auto idx = static_cast<uint32_t>(handle);
	sessionPtrs_[idx].Reset();
}

bool SessionManager::LogInSession(uint64_t handle, const Account& account) {
	int64_t result = accountManager_->Authenticate(account);
	auto idx = static_cast<uint32_t>(handle);
	sessionPtrs_[idx]->accountId_ = account.GetDbId();

	return result != 0;
}

void SessionManager::LogOutSession(uint64_t handle) {
	auto idx = static_cast<uint32_t>(handle);
	sessionPtrs_[idx]->accountId_ = 0;
}

SharedPoolPtr<Session> SessionManager::GetSession(uint64_t handle) {
	auto idx = static_cast<uint32_t>(handle);
	return sessionPtrs_[idx];
}

std::vector<uint64_t> SessionManager::GetSessionsInState(SessionState state) {
	return sessionPool_.GetIndicesInState(static_cast<size_t>(state));
}

SessionState SessionManager::GetState(uint64_t handle) {
	return static_cast<SessionState>(sessionPool_.GetState(handle));
}

void SessionManager::SetState(uint64_t handle, SessionState newState) {
	sessionPool_.MoveToState(handle, static_cast<size_t>(newState));
}
