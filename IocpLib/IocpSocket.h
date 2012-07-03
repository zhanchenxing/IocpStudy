#pragma once

#include "SendBuffer.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>


enum e_ActionType{
	NO_ACTION = 0,
	ACCEPT_ACTION = 1, 
	READ_ACTION = 2,
	WRITE_ACTION = 3,
	CONNECT_ACTION = 4,
};

enum {
	MAX_READ_BUFF_LEN = 2048,	///<	每次WSARecv最大的数量
};

class IocpSocket;

struct IocpOverlapped{
	OVERLAPPED m_oOverlapped;
	e_ActionType m_eAction;
	IocpSocket *m_pIocpClient;

	IocpOverlapped(){
		Zero(0);
	}

	void Zero( IocpSocket * pIocpClient ){
		memset( this, 0, sizeof(*this));
		m_pIocpClient = pIocpClient;
	}
};

class IocpSocket
{
public:
	IocpSocket(void);
	~IocpSocket(void);

	void SetCompPort(HANDLE hCompPort){ m_hCompPort = hCompPort; };

	virtual bool Connect() = 0;

	/// 读取到数据的时候调用这个函数
	virtual bool DataReceived( void * pvData, unsigned nLen ){ return true; };

	/// 上一次写数据的请求发送成功的时候调用这个函数
	virtual bool DataWritted( ){ return true; }

	/// 连接成功的时候调用这个函数
	virtual bool OnConnect(){ return true; };

	/// accept 成功的时候调用这个函数
	virtual bool OnAccept(){ return true; };

	void CloseClient();
	bool ReadData();
	bool DoWriteData();
	bool WriteData( void * pvData, unsigned nLen );

	bool OnCompletionStatusOK( DWORD dwBytesTransfered, u_long CompKey, OVERLAPPED * pOverLapped );
	bool OnCompletionStatusFailed( DWORD dwBytesTransfered, u_long CompKey, OVERLAPPED * pOverLapped, DWORD dwLastError );

	virtual bool OnErrorHappened( );

protected:
	IocpOverlapped m_oReadOverlapped;
	IocpOverlapped m_oWriteOverlapped;
	IocpOverlapped m_oAcceptOverlapped;

	SOCKET m_nSocket;

	// Read buffer
	char m_szBuff[ MAX_READ_BUFF_LEN +1 ];
	WSABUF m_wsaBuf;

	//char m_szAcceptBuff[1024];

	// Write buffer
	CSendBuffer m_oSendBuffer;
	WSABUF m_wsaWriteBuf;
	bool m_bWritting;


	HANDLE m_hCompPort;
};

class IocpServerSocket : public IocpSocket
{
public:
	void SetListenSocket( SOCKET nListenSocket );

	virtual bool Connect();
	virtual bool DataReceived( void * pvData, unsigned nLen );
	virtual bool OnAccept();


private:
	SOCKET m_nListenSocket;
};

class IocpClientSocket : public IocpSocket
{
public:
	void SetDestHostPort( const char * szHost, unsigned short nPort );
	virtual bool Connect();
	virtual bool DataReceived( void * pvData, unsigned nLen );
	virtual bool OnConnect();

	virtual bool DataWritted( );

	virtual bool OnErrorHappened( );

private:
	char m_szHost[32];
	unsigned short m_nPort;

};