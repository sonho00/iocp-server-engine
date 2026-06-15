#include "Session.hpp"

#include <WinSock2.h>

#include <cassert>
#include <cstring>
#include <mutex>

#include "Network/Common/Logger.hpp"
#include "Network/Common/Protocol.hpp"
#include "OverlappedEx.hpp"
#include "PacketHandler.hpp"
#include "ServerUtils.hpp"
#include "SessionManager.hpp"

Session::Session() {
	socket_ = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0,
						 WSA_FLAG_OVERLAPPED);
	if (socket_ == INVALID_SOCKET) {
		LOG_ERROR("Failed to create accept socket");
	}
}

Session::~Session() {
	if (socket_ != INVALID_SOCKET) {
		closesocket(socket_);
	}
}

void Session::Init() {
	recvOv_.Reset();
	sendOv_.Reset();
	disconnectOv_.Reset();
	listener_ = nullptr;
	isSending_ = false;
	handle_ = ISparsePool<Session>::kInvalidHandle;
}

bool Session::RegisterRecv() {
	recvOv_.wsaBuf_.len =
		recvOv_.buffer_.GetSize() - recvOv_.sendPos_ + recvOv_.recvPos_;

	if (recvOv_.wsaBuf_.len == 0) {
		LOG_WARN("[Session:{}] recv buffer overflow detected", handle_);
		return false;
	}

	recvOv_.ioType_ = IO_TYPE::kRecv;
	recvOv_.wsaBuf_.buf = recvOv_.buffer_.GetBuffer() + recvOv_.sendPos_;
	ZeroMemory(&recvOv_.overlapped_, sizeof(OVERLAPPED));
	recvOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	DWORD flags = 0;
	int result = WSARecv(socket_, &recvOv_.wsaBuf_, 1, nullptr, &flags,
						 &recvOv_.overlapped_, nullptr);

	if (result == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode == WSA_IO_PENDING) return true;
		recvOv_.sessionPtr_.Reset();
		switch (errorCode) {
			case WSAECONNRESET:
				LOG_INFO("[Session:{}] Connection closed by client", handle_);
				return false;

			default:
				LOG_ERROR("[Session:{}][Error:{}] Failed to post recv", handle_,
						  errorCode);
				return false;
		}
	}
	return true;
}

bool Session::OnRecv(DWORD bytesTransferred) {
	recvOv_.sendPos_ += bytesTransferred;

	while (true) {
		size_t availableData = recvOv_.sendPos_ - recvOv_.recvPos_;
		if (availableData < sizeof(PACKET_HEADER)) break;

		auto* header = reinterpret_cast<PACKET_HEADER*>(
			recvOv_.buffer_.GetBuffer() + recvOv_.recvPos_);

		if (header->size < sizeof(PACKET_HEADER) ||
			header->size >= recvOv_.buffer_.GetSize()) {
			LOG_ERROR("[Session:{}] Invalid packet size: {}", handle_,
					  header->size);
			return false;
		}

		if (availableData < header->size) break;

		SharedPoolPtr<PacketBlock> packet =
			packetHandler_->AcquirePacket(*header);
		packetHandler_->PostTask([this, packet = std::move(packet)]() mutable {
			auto& header = reinterpret_cast<PACKET_HEADER&>(*packet->data());
			packetHandler_->Execute(*this, header);
		});
		LOG_DEBUG("[Session:{}] Processed packet ID: {}, Size: {}", handle_,
				  static_cast<uint16_t>(header->id), header->size);

		recvOv_.recvPos_ += header->size;
	}

	if (recvOv_.recvPos_ >= recvOv_.buffer_.GetSize()) {
		recvOv_.recvPos_ -= recvOv_.buffer_.GetSize();
		recvOv_.sendPos_ -= recvOv_.buffer_.GetSize();
	}

	if (!RegisterRecv()) {
		LOG_ERROR("[Session:{}] Failed to post another recv", handle_);
		return false;
	}

	return true;
}

bool Session::RegisterSend() {
	assert(isSending_);

	sendOv_.ioType_ = IO_TYPE::kSend;
	sendOv_.wsaBuf_.buf = sendOv_.buffer_.GetBuffer() + sendOv_.recvPos_;
	sendOv_.wsaBuf_.len = sendOv_.sendPos_ - sendOv_.recvPos_;
	ZeroMemory(&sendOv_.overlapped_, sizeof(OVERLAPPED));
	sendOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	DWORD flags = 0;
	int result = WSASend(socket_, &sendOv_.wsaBuf_, 1, nullptr, flags,
						 &sendOv_.overlapped_, nullptr);

	if (result == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode == WSA_IO_PENDING) return true;
		sendOv_.sessionPtr_.Reset();
		switch (errorCode) {
			case WSAECONNRESET:
				LOG_INFO("[Session:{}] Connection closed by client", handle_);
				return false;

			default:
				LOG_ERROR("[Session:{}][Error:{}] Failed to post send", handle_,
						  errorCode);
				return false;
		}
	}
	return true;
}

bool Session::RegisterAccept(SOCKET listenSocket) {
	recvOv_.ioType_ = IO_TYPE::kAccept;
	recvOv_.wsaBuf_.buf = recvOv_.buffer_.GetBuffer();
	recvOv_.wsaBuf_.len = static_cast<ULONG>(recvOv_.buffer_.GetSize());
	ZeroMemory(&recvOv_.overlapped_, sizeof(OVERLAPPED));
	recvOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	DWORD bytesReceived = 0;
	BOOL result = ServerUtils::AcceptEx(
		listenSocket, socket_, recvOv_.buffer_.GetBuffer(), 0,
		Config::kAcceptAddrSize, Config::kAcceptAddrSize, &bytesReceived,
		&recvOv_.overlapped_);

	if (result == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode == WSA_IO_PENDING) return true;
		switch (errorCode) {
			default:
				LOG_ERROR("[Session:{}][Error:{}] AcceptEx failed", GetHandle(),
						  errorCode);
				break;
		}
		return false;
	}
	return true;
}

bool Session::RegisterDisconnect() {
	disconnectOv_.ioType_ = IO_TYPE::kDisconnect;
	ZeroMemory(&disconnectOv_.overlapped_, sizeof(OVERLAPPED));
	disconnectOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	int result = ServerUtils::DisconnectEx(socket_, &disconnectOv_.overlapped_,
										   TF_REUSE_SOCKET, 0);
	if (result == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode == WSA_IO_PENDING) return true;
		LOG_ERROR("[Session:{}][Error:{}] Failed to post disconnect", handle_,
				  errorCode);
		switch (errorCode) {
			default:
				LOG_ERROR("[Session:{}][Error:{}] Failed to post disconnect",
						  handle_, errorCode);
				break;
		}
		return false;
	}
	return true;
}

bool Session::OnSend(DWORD bytesTransferred) {
	std::lock_guard<std::mutex> lock(sendMtx_);
	assert(isSending_);

	sendOv_.recvPos_ += bytesTransferred;

	if (sendOv_.recvPos_ >= sendOv_.buffer_.GetSize()) {
		sendOv_.recvPos_ -= sendOv_.buffer_.GetSize();
		sendOv_.sendPos_ -= sendOv_.buffer_.GetSize();
	}

	if (sendOv_.recvPos_ == sendOv_.sendPos_) {
		isSending_ = false;
		return true;
	}

	if (!RegisterSend()) {
		LOG_ERROR("[Session:{}] Failed to post another send", handle_);
		return false;
	}

	return true;
}

bool Session::SendPacket(const PACKET_HEADER& header) {
	std::lock_guard<std::mutex> lock(sendMtx_);
	if (sendOv_.sendPos_ - sendOv_.recvPos_ + header.size >
		sendOv_.buffer_.GetSize()) {
		LOG_WARN("[Session:{}] Send buffer overflow detected", handle_);
		return false;
	}

	memcpy(sendOv_.buffer_.GetBuffer() + sendOv_.sendPos_, &header,
		   header.size);
	sendOv_.sendPos_ += header.size;

	if (!isSending_) {
		isSending_ = true;

		if (!RegisterSend()) {
			LOG_ERROR("[Session:{}] Failed to post another send", handle_);
			return false;
		}
	}

	return true;
}

bool Session::HandleIO(OverlappedEx& ovEx, DWORD bytesTransferred) {
	switch (ovEx.ioType_) {
		case IO_TYPE::kRecv:
			return OnRecv(bytesTransferred);
		case IO_TYPE::kSend:
			return OnSend(bytesTransferred);
		default:
			LOG_ERROR("[Session:{}] Unknown IO type", handle_);
			return false;
	}
}

bool Session::Connect() {
	{
		std::lock_guard<std::mutex> lock(connectMtx_);
		if (sessionManager_->GetState(handle_) != SessionState::kPending) {
			LOG_ERROR("[Session:{}] Invalid state for Connect: {}", handle_,
					  static_cast<uint8_t>(sessionManager_->GetState(handle_)));
			return false;
		}

		if (!sessionManager_->ConnectSession(handle_)) {
			LOG_ERROR("[Session:{}] Failed to transition to Connected state",
					  handle_);
			return false;
		}
	}

	SharedPoolPtr<PacketBlock> packet = packetHandler_->AcquirePacket();
	auto& welcomePacket = reinterpret_cast<S2C_CHAT&>(*packet->data());
	welcomePacket.header.id = static_cast<uint16_t>(PACKET_ID::kChat);
	sprintf(welcomePacket.message, "%lld", handle_);
	welcomePacket.header.size =
		sizeof(welcomePacket.header) + strlen(welcomePacket.message) + 1;

	if (!SendPacket(welcomePacket.header)) {
		LOG_ERROR("[Session:{}] Failed to send welcome packet", handle_);
		return false;
	}

	if (!RegisterRecv()) {
		LOG_WARN("[Session:{}] Failed to post initial recv", handle_);
		return false;
	}

	return true;
}

bool Session::Disconnect() {
	{
		std::lock_guard<std::mutex> lock(connectMtx_);
		switch (sessionManager_->GetState(handle_)) {
			case SessionState::kPending:
			case SessionState::kConnected:
				if (!sessionManager_->SetState(handle_,
											   SessionState::kDisconnecting)) {
					LOG_ERROR(
						"[Session:{}] Failed to transition to Disconnecting "
						"state",
						handle_);
					return false;
				}
				break;

			case SessionState::kDisconnecting:
				LOG_WARN("[Session:{}] Already in Disconnecting state",
						 handle_);
				return true;

			default:
				LOG_ERROR(
					"[Session:{}] Invalid state for Disconnect: {}", handle_,
					static_cast<uint8_t>(sessionManager_->GetState(handle_)));
				return false;
		}
	}

	disconnectOv_.ioType_ = IO_TYPE::kDisconnect;
	ZeroMemory(&disconnectOv_.overlapped_, sizeof(OVERLAPPED));
	disconnectOv_.sessionPtr_ = sessionManager_->GetSession(handle_);

	int result = ServerUtils::DisconnectEx(socket_, &disconnectOv_.overlapped_,
										   TF_REUSE_SOCKET, 0);
	if (result == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode == WSA_IO_PENDING) return true;
		LOG_ERROR("[Session:{}][Error:{}] Failed to post disconnect", handle_,
				  errorCode);
		switch (errorCode) {
			default:
				LOG_ERROR("[Session:{}][Error:{}] Failed to post disconnect",
						  handle_, errorCode);
				break;
		}
		return false;
	}
	return true;
}

bool Session::Clear() {
	sessionManager_->DisconnectSession(handle_);
	return true;
}
