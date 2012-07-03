#pragma once
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>


#include <WinSock2.h>
#include <MSWSock.h>


/// IOPC全局初始化相关
class IocpBase
{
public:
	IocpBase(void);
	~IocpBase(void);

	static bool InitIocp();
	static bool Finialize();
	static LPFN_ACCEPTEX AcceptEx;
	static LPFN_CONNECTEX ConnectEx;
	static LPFN_DISCONNECTEX DisconnectEx;
	static LPFN_GETACCEPTEXSOCKADDRS GetAcceptexSockAddrs;
	static LPFN_TRANSMITFILE TransmitFile;
	static LPFN_TRANSMITPACKETS TransmitPackets;
	static LPFN_WSARECVMSG WSARecvMsg;
	static LPFN_WSASENDMSG WSASendMsg;
};


