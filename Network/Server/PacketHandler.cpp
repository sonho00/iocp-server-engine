#include "PacketHandler.hpp"

#include <cstring>
#include <string>
#include <vector>

#include "Account.hpp"
#include "AccountManager.hpp"
#include "Network/Common/Config.hpp"
#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"
#include "Session.hpp"
#include "SessionManager.hpp"

PacketHandler::PacketHandler(SessionManager* sessionManager,
							 AccountManager* accountManager)
	: sessionManager_(sessionManager), accountManager_(accountManager) {
	handlers_[static_cast<size_t>(PACKET_ID::kMove)] =
		[this](Session& session, const PACKET_HEADER& header) {
			return HandleMove(session, header);
		};
	handlers_[static_cast<size_t>(PACKET_ID::kChat)] =
		[this](Session& session, const PACKET_HEADER& header) {
			return HandleChat(session, header);
		};
	handlers_[static_cast<size_t>(PACKET_ID::kRegister)] =
		[this](Session& session, const PACKET_HEADER& header) {
			return HandleRegister(session, header);
		};
	handlers_[static_cast<size_t>(PACKET_ID::kLogin)] =
		[this](Session& session, const PACKET_HEADER& header) {
			return HandleLogin(session, header);
		};
	handlers_[static_cast<size_t>(PACKET_ID::kLogout)] =
		[this](Session& session, const PACKET_HEADER& header) {
			return HandleLogout(session, header);
		};

	workerThreads_.reserve(Config::kWorkerThreadCount);
	for (size_t i = 0; i < Config::kWorkerThreadCount; ++i) {
		workerThreads_.emplace_back(&PacketHandler::WorkerThreadFunc, this);
	}
}

PacketHandler::~PacketHandler() {
	taskQueue_.Shutdown();
	for (auto& thread : workerThreads_) {
		if (thread.joinable()) {
			thread.join();
		}
	}
}

void PacketHandler::PostTask(std::function<void()> task) {
	taskQueue_.Push(std::move(task));
}

void PacketHandler::Execute(Session& session, const PACKET_HEADER& header) {
	if (header.id >= static_cast<uint16_t>(PACKET_ID::kCnt)) {
		LOG_ERROR("[Session:{}] Invalid packet ID: {}", session.GetHandle(),
				  header.id);
		return;
	}

	auto& handler = handlers_[header.id];
	if (!handler) {
		LOG_ERROR("[Session:{}] No handler for packet ID: {}",
				  session.GetHandle(), header.id);
		return;
	}

	if (!handler(session, header)) {
		LOG_ERROR("[Session:{}] Failed to handle packet ID: {}",
				  session.GetHandle(), header.id);
	}
}

SharedPoolPtr<PacketBlock> PacketHandler::AcquirePacket() {
	return packetPool_.Acquire();
}

bool PacketHandler::HandleMove(Session& session, const PACKET_HEADER& header) {
	const auto* moveData = reinterpret_cast<const C2S_MOVE*>(&header);
	S2C_MOVE movePacket{};
	movePacket.header.id = static_cast<uint16_t>(PACKET_ID::kMove);
	movePacket.header.size = sizeof(S2C_MOVE);
	movePacket.sessionHandle = session.GetHandle();
	movePacket.x = moveData->x;
	movePacket.y = moveData->y;

	if (!sessionManager_->Broadcast(
			reinterpret_cast<const PACKET_HEADER&>(movePacket),
			session.GetHandle())) {
		LOG_ERROR("Failed to broadcast MOVE packet");
		return false;
	}
	return true;
}

bool PacketHandler::HandleChat(Session& session, const PACKET_HEADER& header) {
	std::vector<char> data(header.size + sizeof(session.GetHandle()));
	auto* chatPacket = reinterpret_cast<S2C_CHAT*>(data.data());
	chatPacket->header.id = static_cast<uint16_t>(PACKET_ID::kChat);
	chatPacket->header.size = header.size + sizeof(chatPacket->sessionHandle);
	chatPacket->sessionHandle = session.GetHandle();
	std::memcpy(chatPacket->message,
				reinterpret_cast<const char*>(&header) + sizeof(PACKET_HEADER),
				header.size - sizeof(PACKET_HEADER));

	if (!sessionManager_->Broadcast(
			reinterpret_cast<const PACKET_HEADER&>(*chatPacket),
			session.GetHandle())) {
		LOG_ERROR("Failed to broadcast CHAT packet");
		return false;
	}

	return true;
}
bool PacketHandler::HandleRegister(Session& session,
								   const PACKET_HEADER& header) {
	const auto* registerData = reinterpret_cast<const C2S_REGISTER*>(&header);

	size_t idLength = strnlen(registerData->id, Config::kIdLength);
	size_t passwordLength =
		strnlen(registerData->password, Config::kPasswordLength);

	Account account(std::string(registerData->id, idLength),
					std::string(registerData->password, passwordLength));

	bool success = accountManager_->RegisterAccount(account);

	S2C_REGISTER response{};
	response.header.id = static_cast<uint16_t>(PACKET_ID::kRegister);
	response.header.size = sizeof(S2C_REGISTER);
	response.success = success;
	const char* resultMessage =
		success ? "Registration successful" : "ID already exists";
	std::strncpy(response.message, resultMessage, sizeof(response.message) - 1);
	response.message[sizeof(response.message) - 1] = '\0';

	if (!session.SendPacket(reinterpret_cast<const PACKET_HEADER&>(response))) {
		LOG_ERROR("Failed to send REGISTER response");
		return false;
	}

	return true;
}

bool PacketHandler::HandleLogin(Session& session, const PACKET_HEADER& header) {
	const auto* loginData = reinterpret_cast<const C2S_LOGIN*>(&header);

	size_t idLength = strnlen(loginData->id, Config::kIdLength);
	size_t passwordLength =
		strnlen(loginData->password, Config::kPasswordLength);

	Account account(std::string(loginData->id, idLength),
					std::string(loginData->password, passwordLength));

	S2C_LOGIN response{};
	response.header.id = static_cast<uint16_t>(PACKET_ID::kLogin);
	response.header.size = sizeof(S2C_LOGIN);
	response.success =
		sessionManager_->LogInSession(session.GetHandle(), account);
	const char* resultMessage =
		response.success ? "Login successful" : "Invalid credentials";
	std::strncpy(response.message, resultMessage, sizeof(response.message) - 1);
	response.message[sizeof(response.message) - 1] = '\0';

	if (!session.SendPacket(reinterpret_cast<const PACKET_HEADER&>(response))) {
		LOG_ERROR("Failed to send LOGIN response");
		return false;
	}

	return true;
}

bool PacketHandler::HandleLogout(Session& session,
								 [[maybe_unused]] const PACKET_HEADER& header) {
	S2C_LOGOUT response{};
	response.header.id = static_cast<uint16_t>(PACKET_ID::kLogout);
	response.header.size = sizeof(S2C_LOGOUT);
	response.success = true;
	const char* resultMessage = "Logout successful";
	std::strncpy(response.message, resultMessage, sizeof(response.message) - 1);
	response.message[sizeof(response.message) - 1] = '\0';

	if (!session.SendPacket(reinterpret_cast<const PACKET_HEADER&>(response))) {
		LOG_ERROR("Failed to send LOGOUT response");
		return false;
	}

	sessionManager_->LogOutSession(session.GetHandle());

	return true;
}

void PacketHandler::WorkerThreadFunc() {
	while (true) {
		auto task = taskQueue_.Pop();
		if (!task) break;
		task();
	}
}
