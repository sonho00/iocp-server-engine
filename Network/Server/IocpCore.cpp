// NOLINTBEGIN(performance-no-int-to-ptr)
#include "IocpCore.hpp"

#include <WinSock2.h>

#include <thread>

#include "Listener.hpp"
#include "Network/Common/Logger.hpp"
#include "Network/Common/Pool/SharedPoolPtr.hpp"
#include "OverlappedEx.hpp"
#include "ServerUtils.hpp"
#include "Session.hpp"

IocpCore::IocpCore() {
	hIocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (hIocp_ == nullptr) {
		LOG_FATAL("[Error:{}] Failed to create IOCP", GetLastError());
	}
}

IocpCore::~IocpCore() {
	isShuttingDown_ = true;
	for (auto& _ : threads_) {
		PostQueuedCompletionStatus(hIocp_, 0, 0, nullptr);
	}

	for (auto& thread : threads_) {
		if (thread.joinable()) {
			thread.join();
		}
	}

	CloseHandle(hIocp_);
}

bool IocpCore::Start(size_t threadCount) {
	threads_.reserve(threadCount);
	for (size_t i = 0; i < threadCount; ++i) {
		threads_.emplace_back(&IocpCore::WorkerThread, this);
		if (!threads_[i].joinable()) {
			LOG_FATAL("Failed to create worker thread");
			for (size_t j = 0; j < i; ++j) {
				PostQueuedCompletionStatus(hIocp_, 0, 0, nullptr);
			}
			return false;
		}
	}

	return true;
}

bool IocpCore::Register(SOCKET socket, ULONG_PTR completionKey) const {
	if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), hIocp_,
							   completionKey, 0) == nullptr) {
		LOG_ERROR("[Error:{}] Failed to register socket with IOCP",
				  GetLastError());
		return false;
	}
	return true;
}

void IocpCore::HandleError(OverlappedEx& overlappedEx, int errorCode) {
	SharedPoolPtr<Session> sessionPtr = overlappedEx.sessionPtr_;

	if (overlappedEx.ioType_ == IO_TYPE::kDisconnect) {
		LOG_ERROR("[Session:{}][Error:{}] Disconnect operation failed",
				  sessionPtr->GetHandle(), errorCode);
		sessionPtr->Clear();
	} else {
		overlappedEx.sessionPtr_.Reset();
		sessionPtr->Disconnect();
	}
	switch (errorCode) {
		case ERROR_NETNAME_DELETED:
			LOG_INFO("[Session:{}] Connection closed by client",
					 sessionPtr->GetHandle());
			break;

		case ERROR_OPERATION_ABORTED:
			if (isShuttingDown_) {
				LOG_INFO("Server is shutting down. Operation aborted.");
			} else {
				LOG_WARN("[Session:{}][Error:{}] Operation aborted",
						 sessionPtr->GetHandle(), errorCode);
			}
			break;

		default:
			LOG_ERROR("[Session:{}][Error:{}] I/O operation failed",
					  sessionPtr->GetHandle(), errorCode);
			break;
	}
}

void IocpCore::Dispatch(OverlappedEx& overlappedEx, DWORD bytesTransferred) {
	SharedPoolPtr<Session> sessionPtr = overlappedEx.sessionPtr_;
	overlappedEx.sessionPtr_.Reset();

	switch (overlappedEx.ioType_) {
		case IO_TYPE::kAccept: {
			if (sessionPtr->GetListener()->HandleAccept(sessionPtr)) {
				LOG_INFO("[Session:{}] Accept completed",
						 sessionPtr->GetHandle());
			} else {
				LOG_WARN("[Session:{}] Failed to handle accept",
						 sessionPtr->GetHandle());
				sessionPtr->Disconnect();
			}
			break;
		}
		case IO_TYPE::kDisconnect: {
			LOG_INFO("[Session:{}] Disconnect completed",
					 sessionPtr->GetHandle());
			sessionPtr->Clear();
			break;
		}
		case IO_TYPE::kRecv:
		case IO_TYPE::kSend: {
			LOG_DEBUG("[Session:{}] Dispatching I/O event - IOType: {}",
					  sessionPtr->GetHandle(),
					  static_cast<int>(overlappedEx.ioType_));

			if (bytesTransferred == 0) {
				LOG_INFO("[Session:{}] Connection closed by client",
						 sessionPtr->GetHandle());
				sessionPtr->Disconnect();
				return;
			}

			if (!sessionPtr->HandleIO(overlappedEx, bytesTransferred)) {
				LOG_ERROR("[Session:{}] Failed to handle I/O operation",
						  sessionPtr->GetHandle());
				sessionPtr->Disconnect();
			}
			break;
		}
		default:
			LOG_ERROR("[Session:{}] Unknown I/O type: {}",
					  sessionPtr->GetHandle(),
					  static_cast<int>(overlappedEx.ioType_));
			sessionPtr->Disconnect();
			return;
	}
}

void IocpCore::WorkerThread() {
	while (true) {
		DWORD bytesTransferred = 0;
		ULONG_PTR completionKey = 0;
		OVERLAPPED* overlapped = nullptr;

		BOOL result = GetQueuedCompletionStatus(
			hIocp_, &bytesTransferred, &completionKey, &overlapped, INFINITE);

		if (overlapped == nullptr) {
			if (isShuttingDown_) {
				LOG_INFO("Server is shutting down.");
				break;
			}
			LOG_FATAL("[Error:{}] GQCS failed", GetLastError());
		}

		OverlappedEx* overlappedEx =
			CONTAINING_RECORD(overlapped, OverlappedEx, overlapped_);
		SharedPoolPtr<Session> sessionPtr = overlappedEx->sessionPtr_;
		int errorCode = static_cast<int>(GetLastError());

		if (overlappedEx->ioType_ == IO_TYPE::kAccept) {
			sessionPtr->GetListener()->DecrementPendingAccepts();
		}

		if (result == FALSE) {
			HandleError(*overlappedEx, errorCode);
			continue;
		}

		LOG_DEBUG("[Session:{}] IOType: {} BytesTransferred: {}",
				  sessionPtr->GetHandle(),
				  static_cast<int>(overlappedEx->ioType_), bytesTransferred);

		Dispatch(*overlappedEx, bytesTransferred);
	}
}
// NOLINTEND(performance-no-int-to-ptr)
