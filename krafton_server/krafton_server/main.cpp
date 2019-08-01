#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
using namespace boost::iostreams;

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>

#include "tst.h"
#include "playerInfo.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define BUFFER_SIZE 512
#define DEFAULT_PORT "27015"

#include "main.h"


int __cdecl main(void)
{
	struct addrinfo* result = NULL;
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP; // TCP
	hints.ai_flags = AI_PASSIVE; // 

	WSADATA wsaData;
	// Initialize Winsock
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}


	SOCKET ListenSocket = INVALID_SOCKET;
	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);


	printf("Start Listening\n");

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	SOCKET ClientSocket = INVALID_SOCKET;
	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	printf("Client Connected\n");
	// No longer need server socket
	closesocket(ListenSocket);

	int count = 0;

	// Receive until the peer shuts down the connection
	do {
		//iResult = receiveMessage(ClientSocket);
		
		if (sendMessage(ClientSocket) == -1)
			break;
		if (count++ > 2)
			break;

		iResult = 1;

		if (iResult > 0) {
			//sendMessage(ClientSocket);
			std::cout << "Message Sent" << std::endl;
		}

		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
	} while (iResult > 0);


	std::cout << "Shutting down client socket" << std::endl;
	// shutdown the connection since we're done

	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}

int receiveMessage(SOCKET ConnectSocket)
{
	int iResult;
	memset(recvBuffer, 0, sizeof(recvBuffer));
	iResult = recv(ConnectSocket, recvBuffer, BUFFER_SIZE, 0); // returns number of bytes received or error
	if (iResult > 0)
	{
		//FIND \n INDEX
		delimiterIndex.clear();
		for (int i = 0; i < strlen(recvBuffer); i++)
			if (recvBuffer[i] == '\n')
				delimiterIndex.push_back(i);

		//HANDLE EACH MESSAGE BY DELIMITER
		for (auto iter = delimiterIndex.begin(); iter != delimiterIndex.end(); iter++)
		{
			int messageLen = delimiterIndex[0];
			std::stringstream ss;
			ss.write(&(recvBuffer[*iter - messageLen]), messageLen);
			boost::archive::text_iarchive ia(ss);

			tst TT;
			ia >> TT;

			std::cout << TT.Name << std::endl;
			std::cout << TT.age << std::endl;
			std::cout << TT.pi << std::endl << std::endl;
		}
		//DESERIALIZATION FROM CHAR*
	}
	else if (iResult == 0)
		printf("Connection closed\n");
	else
		printf("recv failed with error: %d\n", WSAGetLastError());

	return iResult;
}

int sendMessage(SOCKET ClientSocket)
{
	int iSendResult;
	memset(sendBuffer, 0, sizeof(sendBuffer));

	array_sink sink{ sendBuffer };
	stream<array_sink> os{ sink };

	bool tempBool[] = { true, false, false };
	int tempInt[10];
	tempInt[0] = 0x11;
	playerInfo T(1,100,100,tempBool,tempInt);


	boost::archive::text_oarchive oa(os);
	oa << T;

	sendBuffer[strlen(sendBuffer)] = '\n';

	iSendResult = send(ClientSocket, sendBuffer, strlen(sendBuffer), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return -1;
	}

	std::cout << sendBuffer << std::endl;
	printf("Bytes sent: %d\n", iSendResult);

	return iSendResult;
}