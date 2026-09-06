#pragma once

#include <thread>

#include "AccountManager.hpp"
#include "Common/Logger.hpp"
#include "Common/WSAManager.hpp"
#include "DBManager.hpp"
#include "IocpCore.hpp"
#include "Listener.hpp"
#include "PacketHandler.hpp"
#include "ServerUtils.hpp"
#include "SessionManager.hpp"


class ServerService {
   public:
	ServerService()
		: packetHandler_(&sessionManager_, &accountManager_),
		  listener_(iocpCore_, sessionManager_, Config::kPort),
		  accountManager_(&dbManager_) {}

	bool Start() {
		sessionManager_.Init(iocpCore_, listener_, accountManager_,
							 packetHandler_);

		size_t numThreads = std::thread::hardware_concurrency();
		if (!iocpCore_.Start(numThreads)) {
			LOG_FATAL("Failed to start IOCP worker threads");
		}

		if (!listener_.PostAccept()) {
			LOG_ERROR("Failed to post initial accept");
			return false;
		}

		return true;
	}

   private:
	WSAManager wsaManager_;
	ServerUtils::NetFuncs netFuncs_;
	SessionManager sessionManager_;
	PacketHandler packetHandler_;
	IocpCore iocpCore_;
	Listener listener_;
	DBManager dbManager_;
	AccountManager accountManager_;
};
