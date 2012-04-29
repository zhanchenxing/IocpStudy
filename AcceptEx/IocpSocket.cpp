#include "IocpSocket.h"

#include "IocpBase.h"

IocpSocket::IocpSocket(void)
{
	m_wsaBuf.buf = m_szBuff;
	m_wsaBuf.len = MAX_READ_BUFF_LEN;

	m_wsaWriteBuf.buf = 0;
	m_wsaWriteBuf.len = 0;

	m_oReadOverlapped.Zero( this );
	m_oWriteOverlapped.Zero( this );

	m_bWritting = false;
}


IocpSocket::~IocpSocket(void)
{
}

void IocpSocket::CloseClient()
{
	m_oReadOverlapped.Zero( this );
	m_oWriteOverlapped.Zero( this );

	closesocket( m_nSocket );
	m_nSocket = 0;

	memset(m_szBuff, 0, sizeof(m_szBuff) );

	m_oSendBuffer.ClearData();
	m_bWritting = false;


}

bool IocpSocket::ReadData()
{
	m_oReadOverlapped.m_eAction = READ_ACTION;
	DWORD dwReceived = 0;
	DWORD dwFlags = 0;
	memset( m_szBuff, 0, sizeof(m_szBuff) );
	int nRet = WSARecv( m_nSocket, &m_wsaBuf, 1, &dwReceived,  &dwFlags, &(m_oReadOverlapped.m_oOverlapped),
		0 );

	if( nRet == SOCKET_ERROR ){

		DWORD dwLastError = GetLastError();
		if( ERROR_IO_PENDING != dwLastError ){
			printf("WSARecv failed with error: %u\n", dwLastError );
			return false;
		}
	}
	else{
		printf( "WSARecv sucessed directly with: [%s]! \n", m_szBuff );
	}

	return true;
}

bool IocpSocket::DoWriteData()
{
	// 构造，发送
	unsigned nLen = 0;
	void * pvData = m_oSendBuffer.GetNextData(nLen);
	if( nLen == 0 ){
		return true;
	}

	m_wsaWriteBuf.buf = (CHAR*)pvData;
	m_wsaWriteBuf.len = nLen;

	DWORD dwFlags = 0;
	DWORD dwSent = 0;

	int nRet = WSASend( m_nSocket, &m_wsaWriteBuf, 1,
		&dwSent,
		dwFlags, &m_oWriteOverlapped.m_oOverlapped, 0 );

	if( nRet == SOCKET_ERROR ) {
		DWORD dwLastError = GetLastError();
		if( dwLastError != WSA_IO_PENDING ){
			printf("WSASend failed with error: %u\n", dwLastError);
			return false;
		}
	}

	m_bWritting = true;
	m_oWriteOverlapped.m_eAction = WRITE_ACTION;
	return TRUE;
}

bool IocpSocket::WriteData( void * pvData, unsigned nLen )
{
	m_oSendBuffer.AppendData( pvData, nLen );
	if( !m_bWritting )
	{
		if( !DoWriteData() ){
			return false;
		}
	}

	return true;
}

bool IocpSocket::OnErrorHappened( )
{
	CloseClient( );
	if( !Connect( ) ){
		return false;
	}

	return true;
}

bool IocpSocket::OnCompletionStatusFailed( DWORD dwBytesTransfered, u_long CompKey, OVERLAPPED * pOverLapped, DWORD dwLastError )
{
	IocpOverlapped * pIocpOverlapped = (IocpOverlapped *)pOverLapped;
	if( pIocpOverlapped->m_eAction == CONNECT_ACTION ){
		if( pIocpOverlapped != &m_oReadOverlapped ){
			printf("fatal error: connect action with different IocpSocket\n");
			return false;
		}

		printf("OnCompletionStatusFailed: Connect failed with error: %u\n", dwLastError );

		// 连接失败
		return OnErrorHappened();
	}

	else if( pIocpOverlapped->m_eAction == READ_ACTION ){
		if( pIocpOverlapped != &m_oReadOverlapped ){
			printf("fatal error: read action with different IocpSocket\n");
			return false;
		}

		printf("OnCompletionStatusFailed: read failed with error: %u\n", dwLastError );

		return OnErrorHappened();
	}
	else if( pIocpOverlapped->m_eAction == WRITE_ACTION ) {
		// should not be here
		if( pIocpOverlapped != &m_oWriteOverlapped ){
			printf("fatal error: write action with different IocpSocket\n");
			return false;
		}

		printf("OnCompletionStatusFailed: write failed with error: %u\n", dwLastError );

		return OnErrorHappened();
	}

	else if( pIocpOverlapped->m_eAction == ACCEPT_ACTION  ) {
		// should not be here
		if( pIocpOverlapped != &m_oReadOverlapped ){
			printf("fatal error: connect action with different IocpSocket\n");
			return false;
		}

		printf("OnCompletionStatusFailed: accept failed with error: %u\n", dwLastError );

		return OnErrorHappened();
	}

	return true;
}



bool IocpSocket::OnCompletionStatusOK( DWORD dwBytesTransfered, u_long CompKey, OVERLAPPED * pOverLapped )
{
	if( dwBytesTransfered == 0 && CompKey == 0 && pOverLapped == 0 ){
		return true;
	}

	IocpOverlapped * pIocpOverlapped = (IocpOverlapped *)pOverLapped;

	if( pIocpOverlapped->m_eAction == CONNECT_ACTION ){
		if( pIocpOverlapped != &m_oReadOverlapped ){
			printf("fatal error: connect action with different IocpSocket\n");
			return false;
		}

		// 发送读请求
		if( !ReadData( ) ){
			return OnErrorHappened( );

		}
		else
		{
			if( !OnConnect() ){
				return !OnErrorHappened( );
			}
		}
	}

	else if( pIocpOverlapped->m_eAction == READ_ACTION ){

		if( pIocpOverlapped != &m_oReadOverlapped ){
			printf("fatal error: read action with different IocpSocket\n");
			return false;
		}

		printf("received %u bytes: [%s]\n", m_oReadOverlapped.m_oOverlapped.InternalHigh, m_szBuff );

		if( m_oReadOverlapped.m_oOverlapped.InternalHigh == 0 ){
			printf("Client closed!\n");
			return OnErrorHappened();
		}
		else
		{
			if( !DataReceived( m_szBuff, m_oReadOverlapped.m_oOverlapped.InternalHigh ) ){
				return OnErrorHappened( );

			}

			if( !ReadData( ) ){		// 如果对方没有关闭连接，继续接收
				return OnErrorHappened();
			}
		}

	}
	else if( pIocpOverlapped->m_eAction == WRITE_ACTION ) {

		if( pIocpOverlapped != &m_oWriteOverlapped ){
			printf("fatal error: write action with different IocpSocket\n");
			return false;
		}

		if( !m_bWritting ){
			printf("Error: m_bWritting==FALSE when pIocpOverlapped->m_eAction == WRITE_ACTION\n" );
			return false;
		}
		else{
			m_bWritting = false;

			if( !DataWritted() ){
				return OnErrorHappened();
			}

			if( !DoWriteData( ) ){
				return OnErrorHappened();
			}
		}
	}

	else if( pIocpOverlapped->m_eAction == ACCEPT_ACTION  ) {

		sockaddr_in * localAddr = 0;
		int nLocalAddrLen = 0;

		sockaddr_in * remoteAddr = 0;
		int nRemoteAddrLen = 0;

		IocpBase::GetAcceptexSockAddrs( m_szBuff, 0, sizeof(sockaddr_in)+16, sizeof(sockaddr_in)+16, 
			(sockaddr**)(&localAddr), &nLocalAddrLen, (sockaddr**)(&remoteAddr), &nRemoteAddrLen );

		// 输入日志
		printf("Connected from [%s:%u] to [%s:%u]\n", inet_ntoa( remoteAddr->sin_addr) , remoteAddr->sin_port, 
			inet_ntoa( localAddr->sin_addr ), localAddr->sin_port );

		if( !OnAccept() )
		{
			return OnErrorHappened();
		}

		HANDLE hCompPort2 = CreateIoCompletionPort( (HANDLE)m_nSocket, m_hCompPort, (u_long)0, 0 );
		if( hCompPort2 == 0 ){
			printf("CreateIoCompletionPort associate failed with error: %u\n", GetLastError() );
			return false;
		}
		else{
			if( !ReadData( ) ){
				return OnErrorHappened( );
			}
		}
	}

	return true;
}

/// act as echo server
bool IocpServerSocket::DataReceived( void * pvData, unsigned nLen )
{
	return WriteData( pvData, nLen );
}

void IocpServerSocket::SetListenSocket( SOCKET nListenSocket )
{
	m_nListenSocket = nListenSocket;
}


bool IocpServerSocket::Connect()
{
	m_nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( INVALID_SOCKET == m_nSocket ) {
		printf("create AcceptSocket failed with error: %u\n", GetLastError() );
		return false;
	}

	m_oReadOverlapped.m_eAction = ACCEPT_ACTION;
	DWORD dwBytesReceived = 0;
	BOOL bAccept = IocpBase::AcceptEx( m_nListenSocket, m_nSocket, m_szBuff, 0, 
		sizeof(sockaddr_in)+16, sizeof(sockaddr_in)+16, &dwBytesReceived, &m_oReadOverlapped.m_oOverlapped );
	if( !bAccept ){
		DWORD dwLastError = GetLastError();
		if( ERROR_IO_PENDING != dwLastError ) {
			printf("AcceptEx failed with error: %u\n", GetLastError() );
			return false;
		}
	}
	else{
		printf("AcceptEx successed directly!\n");
	}

	return true;
}

 bool IocpServerSocket::OnAccept()
 {
	 static int nConnected = 0;

	 ++nConnected;
	 printf("connected = %d with: %p\n", nConnected, this);

	 return true;
 }

bool IocpClientSocket::Connect()
{
	m_nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( INVALID_SOCKET == m_nSocket ) {
		printf("create AcceptSocket failed with error: %u\n", GetLastError() );
		return false;
	}

	m_oReadOverlapped.m_eAction = CONNECT_ACTION;

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_port = htons(m_nPort);
	service.sin_addr.s_addr = inet_addr(m_szHost);	// INADDR_ANY

	sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_port = 0;
	client.sin_addr.s_addr = INADDR_ANY;
	bind( m_nSocket, (const sockaddr*)&client, sizeof(client) );

	CreateIoCompletionPort( (HANDLE)m_nSocket, m_hCompPort, (u_long)0, 0 );

	BOOL b = IocpBase::ConnectEx( m_nSocket, (const sockaddr*) &service, sizeof(service),
		0, 0, 0, &m_oReadOverlapped.m_oOverlapped );

	if( !b ){
		DWORD dwLastError = GetLastError();
		if( dwLastError != ERROR_IO_PENDING ){
			printf("ConnectEx failed with error: %u\n", dwLastError);
			return false;
		}
	}

	return true;

}

static char buffer[]="123456789012345678901234567890123456789012345678901234567890";

bool IocpClientSocket::OnConnectFailed()
{
	/// 重新尝试连接
	printf("Connect failed. Trying to connect again...\n");
	return OnErrorHappened();
}


void IocpClientSocket::SetDestHostPort( const char * szHost, unsigned short nPort )
{
	m_nPort = nPort;
	strcpy( m_szHost, szHost );
}

bool IocpClientSocket::DataReceived( void * pvData, unsigned nLen )
{
	printf("DataReceived: %s\n", pvData );
	return true;
}

bool IocpClientSocket::OnConnect()
{
	return true;
	//return WriteData( buffer, sizeof(buffer)-1 );
};

bool IocpClientSocket::DataWritted()
{
	// return WriteData( buffer, sizeof(buffer)-1 );
	m_oSendBuffer.AppendData( buffer, sizeof(buffer)-1 );
	return true;
};

bool IocpClientSocket::OnErrorHappened( )
{
	printf("error happened!\n");
	return false;
}
