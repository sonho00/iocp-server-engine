#include "PacketHandler.hpp"

#include <cstring>
#include <string>
#include <vector>

#include "Account.hpp"
#include "AccountManager.hpp"
#include "Common/Config.hpp"
#include "Common/Logger.hpp"
#include "Common/Pool/SharedPoolPtr.hpp"
#include "Common/Protocol.hpp"
#include "Session.hpp"
#include "SessionManager.hpp"

PacketHandler::PacketHandler(SessionManager* sessionManager,
							 AccountManager* accountManager)
	: sessionManager_(sessionManager), accountManager_(accountManager) {
	handlers_[static_cast<size_t>(PACKET_ID::kMove)] =
		[this](Session& session, const PACKET_HEADER& header) {
			HandleMove(session, header);
		};
	handlers_[static_cast<size_t>(PACKET_ID::kChat)] =
		[this](Session& session, const PACKET_HEADER& header) {
			HandleChat(session, header);
		};
	handlers_[static_cast<size_t>(PACKET_ID::kRegister)] =
		[this](Session& session, const PACKET_HEADER& header) {
			HandleRegister(session, header);
		};
	handlers_[static_cast<size_t>(PACKET_ID::kLogin)] =
		[this](Session& session, const PACKET_HEADER& header) {
			HandleLogin(session, header);
		};
	handlers_[static_cast<size_t>(PACKET_ID::kLogout)] =
		[this](Session& session, const PACKET_HEADER& header) {
			HandleLogout(session, header);
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

	SharedPoolPtr<PacketBlock> packet = AcquirePacket();
	auto& packetHeader = reinterpret_cast<PACKET_HEADER&>(*packet->data());
	std::memcpy(&packetHeader, &header, header.size);

	handler(session, packetHeader);
}

SharedPoolPtr<PacketBlock> PacketHandler::AcquirePacket() {
	return packetPool_.Acquire();
}

SharedPoolPtr<PacketBlock> PacketHandler::AcquirePacket(
	const PACKET_HEADER& header) {
	auto packet = packetPool_.Acquire();
	std::memcpy(packet->data(), &header, header.size);
	return packet;
}

void PacketHandler::SendToSession(uint64_t sessionHandle,
								  const PACKET_HEADER& header) {
	auto session = sessionManager_->GetSession(sessionHandle);
	if (session.IsValid()) {
		session->SendPacket(header);
	}
}

void PacketHandler::Broadcast(uint64_t sessionHandle,
							  const PACKET_HEADER& header) {
	auto sessions =
		sessionManager_->GetSessionsInState(SessionState::kConnected);
	for (uint64_t handle : sessions) {
		if (handle != sessionHandle) {
			SendToSession(handle, header);
		}
	}
}

void PacketHandler::HandleMove(Session& session, const PACKET_HEADER& header) {
	const auto& moveData = reinterpret_cast<const C2S_MOVE&>(header);
	SharedPoolPtr<PacketBlock> packet = AcquirePacket();
	auto& movePacket = reinterpret_cast<S2C_MOVE&>(*packet->data());
	movePacket.header.id = static_cast<uint16_t>(PACKET_ID::kMove);
	movePacket.header.size = sizeof(S2C_MOVE);
	movePacket.sessionHandle = session.GetHandle();
	movePacket.x = moveData.x;
	movePacket.y = moveData.y;

	Broadcast(session.GetHandle(), movePacket.header);
}

void PacketHandler::HandleChat(Session& session, const PACKET_HEADER& header) {
	SharedPoolPtr<PacketBlock> packet = AcquirePacket();
	auto& chatPacket = reinterpret_cast<S2C_CHAT&>(*packet->data());
	chatPacket.header.id = static_cast<uint16_t>(PACKET_ID::kChat);
	chatPacket.header.size = header.size + sizeof(chatPacket.sessionHandle);
	chatPacket.sessionHandle = session.GetHandle();
	std::memcpy(chatPacket.message,
				reinterpret_cast<const char*>(&header) + sizeof(PACKET_HEADER),
				header.size - sizeof(PACKET_HEADER));

	Broadcast(session.GetHandle(), chatPacket.header);
}

void PacketHandler::HandleRegister(Session& session,
								   const PACKET_HEADER& header) {
	const auto& registerData = reinterpret_cast<const C2S_REGISTER&>(header);

	size_t idLength = strnlen(registerData.id, Config::kIdLength);
	size_t passwordLength =
		strnlen(registerData.password, Config::kPasswordLength);

	Account account(std::string(registerData.id, idLength),
					std::string(registerData.password, passwordLength));

	bool success = accountManager_->RegisterAccount(account);

	SharedPoolPtr<PacketBlock> packet = AcquirePacket();
	auto& response = reinterpret_cast<S2C_REGISTER&>(*packet->data());
	response.header.id = static_cast<uint16_t>(PACKET_ID::kRegister);
	response.success = success;
	const char* resultMessage =
		success ? "Registration successful" : "ID already exists";
	size_t messageLength = strlen(resultMessage) + 1;
	strncpy_s(response.message, messageLength, resultMessage,
			  messageLength - 1);
	response.header.size = sizeof(S2C_REGISTER) + messageLength;

	session.SendPacket(reinterpret_cast<const PACKET_HEADER&>(response));
}

void PacketHandler::HandleLogin(Session& session, const PACKET_HEADER& header) {
	const auto& loginData = reinterpret_cast<const C2S_LOGIN&>(header);

	size_t idLength = strnlen(loginData.id, Config::kIdLength);
	size_t passwordLength =
		strnlen(loginData.password, Config::kPasswordLength);

	Account account(std::string(loginData.id, idLength),
					std::string(loginData.password, passwordLength));

	SharedPoolPtr<PacketBlock> packet = AcquirePacket();
	auto& response = reinterpret_cast<S2C_LOGIN&>(*packet->data());
	response.header.id = static_cast<uint16_t>(PACKET_ID::kLogin);
	response.success =
		sessionManager_->LogInSession(session.GetHandle(), account);
	const char* resultMessage =
		response.success ? "Login successful" : "Invalid credentials";
	size_t messageLength = strlen(resultMessage) + 1;
	strncpy_s(response.message, messageLength, resultMessage,
			  messageLength - 1);
	response.header.size = sizeof(S2C_LOGIN) + messageLength;
	session.SendPacket(reinterpret_cast<const PACKET_HEADER&>(response));
}

void PacketHandler::HandleLogout(Session& session,
								 [[maybe_unused]] const PACKET_HEADER& header) {
	SharedPoolPtr<PacketBlock> packet = AcquirePacket();
	auto& response = reinterpret_cast<S2C_LOGOUT&>(*packet->data());
	response.header.id = static_cast<uint16_t>(PACKET_ID::kLogout);
	response.success = true;
	const char* resultMessage = "Logout successful";
	size_t messageLength = strlen(resultMessage) + 1;
	strncpy_s(response.message, messageLength, resultMessage,
			  messageLength - 1);
	response.header.size = sizeof(S2C_LOGOUT) + messageLength;

	session.SendPacket(reinterpret_cast<const PACKET_HEADER&>(response));

	sessionManager_->LogOutSession(session.GetHandle());
}

void PacketHandler::WorkerThreadFunc() {
	while (true) {
		auto task = taskQueue_.Pop();
		if (!task) break;
		task();
	}
}
