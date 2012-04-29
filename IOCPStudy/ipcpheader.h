#pragma once

#include <WinSock2.h>
#include <Windows.h>

const static int BUFFER_SIZE = 1024;

typedef struct _PER_HANDLE_DATA {
	SOCKET s;
	sockaddr_in addr;
}PER_HANDLE_DATA, *PPER_HANDLE_DATA;

typedef struct _PER_IO_DATA{
	OVERLAPPED ol;
	char buf[BUFFER_SIZE+1];
	int nOperationType;

	_PER_IO_DATA(){
		memset( this, 0, sizeof(*this) );
	}

}PER_IO_DATA, *PPER_IO_DATA;

enum {
	OP_READ = 1,
	OP_WRITE = 2,
	OP_ACCEPT = 3,
};
