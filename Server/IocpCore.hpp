#pragma once

#include <WinSock2.h>

#include <thread>
#include <vector>

#include "OverlappedEx.hpp"

class Listener;

class IocpCore {
   public:
	IocpCore();
	~IocpCore();

	bool Start(size_t threadCount);
	[[nodiscard]] bool Register(SOCKET socket, ULONG_PTR completionKey) const;

   private:
	void WorkerThread();

	void HandleError(OverlappedEx& overlappedEx, int errorCode);
	static void DispatchAccept(SharedPoolPtr<Session>& sessionPtr);
	static void DispatchDisconnect(SharedPoolPtr<Session>& sessionPtr);
	static void DispatchRecvSend(SharedPoolPtr<Session>& sessionPtr,
								 OverlappedEx& overlappedEx,
								 DWORD bytesTransferred);
	static void Dispatch(OverlappedEx& overlappedEx, DWORD bytesTransferred);

	std::vector<std::thread> threads_;
	HANDLE hIocp_;
	std::atomic<bool> isShuttingDown_ = false;
};
