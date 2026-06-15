#pragma once

#include <WinSock2.h>
#include <minwinbase.h>

#include "Network/Common/Config.hpp"
#include "Network/Common/MirroredRingBuffer.hpp"
#include "Network/Common/Pool/SharedPoolPtr.hpp"
#include "Network/Common/Protocol.hpp"

class Session;

struct OverlappedEx {
	OverlappedEx(size_t bufferSize = Config::kBufferSize)
		: buffer_(bufferSize) {}

	void Init() { Reset(); }

	void Reset() {
		overlapped_ = {};
		ioType_ = IO_TYPE::kNone;
		wsaBuf_ = {};
		buffer_.Clear();
		recvPos_ = 0;
		sendPos_ = 0;
		sessionPtr_.Reset();
	}

	OVERLAPPED overlapped_ = {};
	IO_TYPE ioType_ = IO_TYPE::kNone;

	WSABUF wsaBuf_ = {};
	MirroredRingBuffer buffer_;
	SharedPoolPtr<Session> sessionPtr_ = nullptr;

	size_t recvPos_ = 0;
	size_t sendPos_ = 0;
};
