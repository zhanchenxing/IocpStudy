// IocpClient.cpp : Defines the entry point for the console application.
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
#include <assert.h>

#include "IocpBase.h"
#include "SendBuffer.h"
#include "IocpSocket.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "Mswsock.lib")

// 功能：
// 发起1000个连接，疯狂的往服务器发送和接收消息。

HANDLE g_hIoCompPort=  0;

BOOL finished = FALSE;
BOOL WINAPI HandlerRoutine( DWORD CtrlType )
{
	std::cerr<<"CtrlType="<<CtrlType<<std::endl;
	finished = TRUE;

	PostQueuedCompletionStatus( g_hIoCompPort, 0, 0, 0 );
	return TRUE;
}


class EchoClient : public IocpClientSocket
{
public:
	virtual bool OnConnect(){
		printf("%p connected!\n", this);
		char buff[8192]={0};
		bool bWrite = WriteData( buff, 8192 );
		assert( bWrite );
		return true;
	}

	virtual bool DataReceived( void * pvData, unsigned nLen ){
		//char buff[1024]={0};
		//memcpy( buff, pvData, nLen );
		//printf("[%s]\n", buff);
		//fwrite( pvData, 1, nLen, stdout );
		WriteData( pvData, nLen );
		return true;
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	if( !IocpBase::InitIocp() ){
		return 1;
	}

	 ::SetConsoleCtrlHandler( HandlerRoutine, TRUE );

	HANDLE hCompPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, (u_long)0, 0 );
	if( hCompPort == NULL ) {
		printf("CreateIoCompletionPort failed with error: %u\n", GetLastError() );
		return 1;
	}


	const int MAX_CONNECTED_CLIENT = 1;		// 最多同时连接10个客户端
	EchoClient * clients[MAX_CONNECTED_CLIENT]={NULL};
	for( int i=0; i<MAX_CONNECTED_CLIENT; ++i ){
		EchoClient * pClient = new EchoClient;
		pClient->SetCompPort( hCompPort );
		pClient->SetDestHostPort( "10.240.170.46", 27015 );
		pClient->Connect( );
		clients[i] = pClient;
	}

	while(!finished){

		DWORD dwBytesTransfered = 0;
		u_long CompKey = 0;
		OVERLAPPED * pOverLapped = 0;
		BOOL bOk = GetQueuedCompletionStatus( hCompPort, &dwBytesTransfered, &CompKey, &pOverLapped, INFINITE );
		IocpOverlapped * pIocpOverlapped = (IocpOverlapped*)pOverLapped;
		if( !bOk ){
			printf("GetQueuedCompletionStatus failed with error: %u\n", GetLastError() );
			if( !pIocpOverlapped->m_pIocpClient->OnCompletionStatusFailed(
				dwBytesTransfered, CompKey, pOverLapped, GetLastError() ) ){
				printf("Fatal error!\n");
				return 1;
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

