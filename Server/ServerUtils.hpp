#pragma once

#include <WinSock2.h>
#include <mswsock.h>

class Session;

namespace ServerUtils {
struct NetFuncs {
	NetFuncs();
};
extern LPFN_ACCEPTEX AcceptEx;
extern LPFN_DISCONNECTEX DisconnectEx;
}  // namespace ServerUtils
