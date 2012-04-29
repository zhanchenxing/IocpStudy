// IOCPStudy.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ipcpheader.h"

#include <iostream>

using namespace std;

DWORD WINAPI ServerThread( LPVOID lpParam);

static const int LISTEN_PORT = 20055;

int _tmain(int argc, _TCHAR* argv[])
{

	WSADATA wsaData;
	if( 0 != WSAStartup( MAKEWORD(2,2), &wsaData ) ){
		cout<<"WSAStartup failed!"<<endl;
		return 1;
	}

	HANDLE hIocp = ::CreateIoCompletionPort( INVALID_HANDLE_VALUE, 0, 0, 0 );
	HANDLE hThread = ::CreateThread( 0, 0, ServerThread, (LPVOID) hIocp, 0, 0 );
	if( NULL == hThread ){
		cout<<"CreateThread failed!"<<endl;
		return 1;
	}

	SOCKET sListen = ::socket( AF_INET, SOCK_STREAM, 0 );
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( LISTEN_PORT );
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind( sListen, (const sockaddr*)&addr, sizeof(addr ) );
	listen( sListen, 5 );


	cout<<"IOCP demo start listening..."<<endl;
	while(true ){
		SOCKADDR_IN saRemote;
		int nLen = sizeof(saRemote);

		SOCKET sRemote = accept( sListen, (sockaddr*)&saRemote, &nLen );
		if( INVALID_SOCKET  == sRemote ){
			cerr<<"accept failed!"<<endl;
			continue;
		}

		PPER_HANDLE_DATA pPerHandle = new PER_HANDLE_DATA;
		pPerHandle->s = sRemote;
		memcpy( &pPerHandle->addr, &saRemote, nLen );
		::CreateIoCompletionPort( (HANDLE)sRemote, hIocp, (DWORD)pPerHandle, 0 );

		PPER_IO_DATA pIoData = new PER_IO_DATA;
		pIoData->nOperationType = OP_READ;
		WSABUF buf;
		buf.buf = pIoData->buf;
		buf.len = BUFFER_SIZE;

		// 刚刚接收进来的要发一个读请求
		DWORD dwRecv = 0;
		DWORD dwFlags = 0;
		::WSARecv( sRemote, &buf, 1, &dwRecv, &dwFlags,  &pIoData->ol, 0 );
	}


	cout<<"ended"<<endl;


	return 0;
}


DWORD WINAPI ServerThread( LPVOID lpParam ){
	HANDLE hIocp = (HANDLE) lpParam;

	while (true ){
		DWORD dwTrans = 0;
		PPER_HANDLE_DATA pPerHandle = 0;
		PPER_IO_DATA pPerIo = 0;
		BOOL bRet = ::GetQueuedCompletionStatus( hIocp, &dwTrans, (LPDWORD)&pPerHandle, (LPOVERLAPPED*)&pPerIo, WSA_INFINITE );
		if( !bRet ){
			cerr<<"wrong!"<<endl;
			closesocket( pPerHandle->s );

			delete pPerHandle;
			delete pPerIo;
			continue;
		}

		if( dwTrans == 0 && ( pPerIo->nOperationType == OP_READ || pPerIo->nOperationType == OP_WRITE ) ){
			closesocket( pPerHandle->s );

			delete pPerHandle;
			delete pPerIo;

			cerr<<"wrong!"<<endl;

			continue;
		}

		switch( pPerIo->nOperationType ) {
		case OP_READ:	// 完成了一个接收请求
			{
				cerr<<"OP_READ finished with:"<<pPerIo->buf<<endl;

				// 继续接收
				memset(pPerIo, 0, sizeof(PER_IO_DATA) );
				pPerIo->nOperationType = OP_READ;
				DWORD dwRecv = 0;
				DWORD dwFlags = 0;

				WSABUF buf;
				buf.buf = pPerIo->buf;
				buf.len = BUFFER_SIZE;
				::WSARecv( pPerHandle->s, &buf, 1, &dwRecv, &dwFlags,  &pPerIo->ol, 0 );

			}
			break;

		case OP_WRITE:
			{
				cerr<<"OP_WRITE finished!"<<endl;

				// 看看有没有数据要发送。
				// 如果有，就发送。
				// 如果没有，就不再发送。

			}
			break;

		case OP_ACCEPT:
			{
				cerr<<"OP_ACCEPT finished!"<<endl;
			}
			break;
		default:
			{
				cerr<<"Unknown op:"<<pPerIo->nOperationType<<endl;
			}
			break;
		}
	}

	return 0;
}