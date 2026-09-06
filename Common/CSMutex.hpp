#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <WinSock2.h>
#include <windows.h>

class CSMutex {
   public:
	CSMutex() : cs_{} { InitializeCriticalSection(&cs_); }
	~CSMutex() { DeleteCriticalSection(&cs_); }

	CSMutex(const CSMutex&) = delete;
	CSMutex& operator=(const CSMutex&) = delete;

	void lock() { EnterCriticalSection(&cs_); }
	void unlock() { LeaveCriticalSection(&cs_); }

   private:
	CRITICAL_SECTION cs_;
};
