// AcceptEx.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#define WIN32_LEAN_AND_MEAN

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <stdio.h>

#include <iostream>
#include <string>

#include "IocpBase.h"
#include "SendBuffer.h"
#include "IocpSocket.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "Mswsock.lib")

HANDLE g_hIoCompPort=  0;

BOOL finished = FALSE;
BOOL WINAPI HandlerRoutine( DWORD CtrlType )
{
	std::cerr<<"CtrlType="<<CtrlType<<std::endl;
	finished = TRUE;

	PostQueuedCompletionStatus( g_hIoCompPort, 0, 0, 0 );
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if( !IocpBase::InitIocp() ){
		return 1;
	}

	 // _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF);

	 ::SetConsoleCtrlHandler( HandlerRoutine, TRUE );

	HANDLE hCompPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, (u_long)0, 0 );
	if( hCompPort == NULL ) {
		printf("CreateIoCompletionPort failed with error: %u\n", GetLastError() );
		return 1;
	}

	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( INVALID_SOCKET == ListenSocket ) {
		printf("Create of ListenSocket failed with error: %u\n", GetLastError() );
		return 1;
	}
	CreateIoCompletionPort( (HANDLE)ListenSocket, hCompPort, (u_long)0, 0 );

	g_hIoCompPort = hCompPort;

	unsigned short port = 27015;
	hostent *thisHost = gethostbyname("");
	const char *ip = inet_ntoa( *(struct in_addr*)*thisHost->h_addr_list );
	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_port = htons(port);
	service.sin_addr.s_addr = inet_addr(ip);	// INADDR_ANY
	if( bind(ListenSocket, (sockaddr*) &service, sizeof(service) ) == SOCKET_ERROR ) {
		printf("bind failed with error: %u\n", GetLastError() );
		return 1;
	}
	if( listen( ListenSocket, 100 ) == SOCKET_ERROR ) {
		printf("listen failed with error: %u\n", GetLastError() );
		return 1;
	}

	printf("Listening on address: %s:%d\n", ip, port );

	const int MAX_CONNECTED_CLIENT = 10;		// 最多同时连接10个客户端
	const DWORD dwReceivedDataLength = 0;

	IocpSocket * clients[MAX_CONNECTED_CLIENT]={NULL};
	for( int i=0; i<MAX_CONNECTED_CLIENT; ++i ){
		IocpServerSocket * pClient = new IocpServerSocket;
		pClient->SetListenSocket( ListenSocket );
		pClient->SetCompPort( hCompPort );
		pClient->Connect( );

		printf("%d Connecting with: %p\n", i, pClient);
		clients[i] = pClient;
	}

	while(!finished){

		DWORD dwBytesTransfered = 0;
		u_long CompKey = 0;
		OVERLAPPED * pOverLapped = 0;
		BOOL bOk = GetQueuedCompletionStatus( hCompPort, &dwBytesTransfered, &CompKey, &pOverLapped, INFINITE );
		IocpOverlapped * pIocpOverlapped = (IocpOverlapped*)pOverLapped;
		if( !bOk ){
			DWORD dwLastError = GetLastError();
			printf("%p GetQueuedCompletionStatus failed with error: %u\n", pIocpOverlapped->m_pIocpClient, dwLastError );
			if( !pIocpOverlapped->m_pIocpClient->OnCompletionStatusFailed(
				dwBytesTransfered, CompKey, pOverLapped, dwLastError ) ){
				printf("Fatal error!\n");
			}
		}
		else{
			if( dwBytesTransfered == 0 && CompKey == 0 && pOverLapped == 0 ){
				break;
			}

			if( !pIocpOverlapped->m_pIocpClient->OnCompletionStatusOK(
				dwBytesTransfered, CompKey, pOverLapped ) ){
				printf("Fatal error!\n");
			}
		}

	}	// end while(true);

	// 清理
	for( int i=0; i<MAX_CONNECTED_CLIENT; ++i ){
		delete clients[i];
	}

	IocpBase::Finialize();
	int nRet = _CrtDumpMemoryLeaks();
	return 0;
}

