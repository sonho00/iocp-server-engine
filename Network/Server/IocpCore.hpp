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

	void SetListener(Listener* listener) { listener_ = listener; }

   private:
	void WorkerThread();

	static void HandleError(OverlappedEx& overlappedEx, int errorCode);
	static void Dispatch(OverlappedEx& overlappedEx, DWORD bytesTransferred);

	std::vector<std::thread> threads_;
	HANDLE hIocp_;
	Listener* listener_ = nullptr;
	std::atomic<bool> isShuttingDown_ = false;
};
