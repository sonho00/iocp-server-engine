#pragma once

#include <WinSock2.h>
#include <mswsock.h>

#include "Network/Common/Pool/SharedPoolPtr.hpp"

class Session;

namespace ServerUtils {
struct NetFuncs {
	NetFuncs();
};
void HandleError(SharedPoolPtr<Session>& session, int errorCode);
extern LPFN_ACCEPTEX AcceptEx;
extern LPFN_DISCONNECTEX DisconnectEx;
}  // namespace ServerUtils
