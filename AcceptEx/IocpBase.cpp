
#include "IocpBase.h"

#include <stdio.h>


IocpBase::IocpBase(void)
{
}

IocpBase::~IocpBase(void)
{
}

template< class FuncPointer >
bool GetFunctionPointer( SOCKET sock, FuncPointer & pfn, GUID & guid )
{
	DWORD dwBytesReturned = 0;
	if( WSAIoctl( sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), 
		&pfn, sizeof(pfn),
		&dwBytesReturned, NULL, NULL ) == SOCKET_ERROR  ){
			printf("Get function failed with error: %u\n", GetLastError() );
			return false;
	}

	return true;
}

#define GET_FUNC_POINTER( GuidValue, pft )	{\
	GUID guid = GuidValue;\
	if( !GetFunctionPointer( ListenSocket, pft, guid ) )\
	{\
		return false;\
	}\
}

bool IocpBase::InitIocp()
{
	WSADATA wsaData;
	int iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
	if( iResult != NO_ERROR ) {
		printf("Error at WSAStartup: %u\n", GetLastError());
		return false;
	}

	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP );
	GET_FUNC_POINTER(WSAID_ACCEPTEX, AcceptEx);
	GET_FUNC_POINTER(WSAID_CONNECTEX, ConnectEx);
	GET_FUNC_POINTER(WSAID_DISCONNECTEX, DisconnectEx);
	GET_FUNC_POINTER(WSAID_GETACCEPTEXSOCKADDRS, GetAcceptexSockAddrs);
	GET_FUNC_POINTER(WSAID_TRANSMITFILE, TransmitFile);
	GET_FUNC_POINTER(WSAID_TRANSMITPACKETS, TransmitPackets);
	GET_FUNC_POINTER(WSAID_WSARECVMSG, WSARecvMsg);
	GET_FUNC_POINTER(WSAID_WSASENDMSG, WSASendMsg);

	return true;
}

bool IocpBase::Finialize(){

	WSACleanup();
	return true;
}


LPFN_ACCEPTEX IocpBase::AcceptEx = 0;
LPFN_CONNECTEX IocpBase::ConnectEx = 0;
LPFN_DISCONNECTEX IocpBase::DisconnectEx = 0;
LPFN_GETACCEPTEXSOCKADDRS IocpBase::GetAcceptexSockAddrs = 0;
LPFN_TRANSMITFILE IocpBase::TransmitFile = 0;
LPFN_TRANSMITPACKETS IocpBase::TransmitPackets = 0;
LPFN_WSARECVMSG IocpBase::WSARecvMsg = 0;
LPFN_WSASENDMSG IocpBase::WSASendMsg = 0;