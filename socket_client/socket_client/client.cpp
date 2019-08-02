#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
using namespace boost::iostreams;

#include <windows.h> // #define win32_lean_and_mean required to include because winsock2 already includes core elements
#include <winsock2.h> //includes core elements from Windows.h
#include <ws2tcpip.h> //contains definitions/protocols for Winsocket
//#include <iphlpapi.h> // IP Helper API
#include <stdlib.h>
#include <stdio.h>
#include <vector>
using namespace std;

#include "playerInfo.h"

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define BUFFER_SIZE 512
#define DEFAULT_PORT "27015"

int Initialize();
int receiveMessage(SOCKET);
int sendMessage(SOCKET);

char sendBuffer[BUFFER_SIZE];
char recvBuffer[BUFFER_SIZE];
std::vector<int> delimiterIndex;
SOCKET ConnectSocket;
int counter;

class tst {
public:
	friend class boost::serialization::access;

	tst(std::string sName, int sage, float spi)
		:Name(sName), age(sage), pi(spi)
	{}

	tst() {
	}

	~tst() {

	}

	std::string Name;
	int age;
	float pi;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar& Name;
		ar& age;
		ar& pi;
	}


};


int __cdecl main(int argc, char** argv)
{
	int iResult;

	Initialize();
	counter = 0;
	while (1)
	{
		iResult = receiveMessage(ConnectSocket);

		if (iResult > 0) {
			std::cout << "Message Received: " + std::to_string(iResult) + " Bytes" << std::endl;
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return false;
		}
	}


	return 0;
}


int Initialize()
{
	WSADATA wsaData; //Contains information aout the Windows Sockets implementation.
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // initiate the use of WS2_32.dll // = Winsock version 2.2 used
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	//addrinfo contains sockaddr structure
	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC; //either IPv6 or IPv4 address can be returned
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	ConnectSocket = INVALID_SOCKET;

	// Resolve the server address and port
	iResult = getaddrinfo("", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		// Validate Socket
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	//Validate Socket
	freeaddrinfo(result);
	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	return 0;
}
int receiveMessage(SOCKET ConnectSocket)
{
	int iResult;
	//First receive size of data that needs to be read
	memset(recvBuffer, 0, sizeof(recvBuffer));
	iResult = recv(ConnectSocket, recvBuffer, sizeof(int), 0);
	if (iResult > 0)
	{
		cout << "count: " + to_string(counter++) << endl;
		//read to know how many bytes to receive
		int msgLen = std::stoi(recvBuffer);
		memset(recvBuffer, 0, sizeof(recvBuffer));
		//read real messages
		iResult = recv(ConnectSocket, recvBuffer, msgLen, 0); // returns number of bytes received or error
		if (iResult > 0)
		{
			//FIND \n INDEX
			delimiterIndex.clear();
			for (int i = 0; i < strlen(recvBuffer); i++)
				if (recvBuffer[i] == '\n')
					delimiterIndex.push_back(i);

			//HANDLE EACH MESSAGE BY DELIMITER
			int prevEnd = -1;
			for (int i = 0; i < delimiterIndex.size(); i++)
			{
				int messageLen = delimiterIndex[i];
				std::stringstream ss;
				ss.write(&(recvBuffer[prevEnd + 1]), delimiterIndex[i] - (prevEnd + 1));
				boost::archive::text_iarchive ia(ss);
				prevEnd = delimiterIndex[i] + 1;
				playerInfo pInfo;
				ia >> pInfo;

				std::cout << "ID: " << pInfo.playerID << std::endl;
				std::cout << "mouseX: " << pInfo.mouseX << std::endl;
				std::cout << "mouseY: " << pInfo.mouseY << std::endl << std::endl;
			}
			//DESERIALIZATION FROM CHAR*
		}
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
	playerInfo T(1, 100, 100, tempBool, tempInt);


	boost::archive::text_oarchive oa(os);
	oa << T;
	sendBuffer[strlen(sendBuffer)] = '\n';

	std::string msgLen = std::to_string(strlen(sendBuffer));
	const char* msgLenChar = msgLen.c_str();
	iSendResult = send(ClientSocket, msgLenChar, sizeof(int), 0);

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