#pragma once
#ifndef _SOCKETMANAGER_H_
#define _SOCKETMANAGER_H_

#define BUFFER_SIZE 512
#define DEFAULT_PORT "27015"

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <string>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

class playerInfo;

class socketManager
{
public:
	socketManager();
	~socketManager();
	int Initialize();
	void Frame();
	void Shutdown();

	bool CheckNewConnection();

	void PushToClients();

	int receiveMessage(SOCKET ConnectSocket);
	int sendMessage(SOCKET ConnectSocket);
private:
	void CloseClientSockets(std::vector<int>);


	char sendBuffer[BUFFER_SIZE];
	char recvBuffer[BUFFER_SIZE];
	std::vector<int> delimiterIndex;
	//Last is NULL waiting for new connection
	std::vector<SOCKET> ClientSocket;
	SOCKET ListenSocket;
	//playerInfo pInfo;
	int count;
};

#endif