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
#include <stdlib.h>
#include <stdio.h>



// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

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

	//---------------------------
	char buffer[3][500] = { 0, };
	array_sink sink1{ buffer[0] };
	stream<array_sink> os1{ sink1 };
	array_sink sink2{ buffer[1] };
	stream<array_sink> os2{ sink2 };
	array_sink sink3{ buffer[2] };
	stream<array_sink> os3{ sink3 };

	tst T1("Tab", 1, 3.1415);
	tst T2("Tab", 2, 3.1415);
	tst T3("Tab", 3, 3.1415);
	
	boost::archive::text_oarchive oa1(os1);
	oa1 << T1;
	boost::archive::text_oarchive oa2(os2);
	oa2 << T2;
	boost::archive::text_oarchive oa3(os3);
	oa3 << T3;

	//---------------------


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
	// No longer need server socket
	closesocket(ListenSocket);

	

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	int count=0;

	// Receive until the peer shuts down the connection
	do {
		//iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		iResult = 1;
		if (iResult > 0) {
			//printf("Bytes received: %d\n", iResult);

			// Echo the buffer back to the sender
			buffer[0][strlen(buffer[0])] = '\n';
			iSendResult = send(ClientSocket, buffer[0], strlen(buffer[0]), 0);
			std::cout << buffer[0] << std::endl;
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);

			buffer[1][strlen(buffer[1])] = '\n';
			iSendResult = send(ClientSocket, buffer[1], strlen(buffer[1]), 0);
			std::cout << buffer[1] << std::endl;
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);

			buffer[2][strlen(buffer[2])] = '\n';
			iSendResult = send(ClientSocket, buffer[2], strlen(buffer[2]), 0);
			std::cout << buffer[2] << std::endl;
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}

		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
		/*if (count++ > 2)
			break;*/
		iResult = 0;
	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	printf("%s\n", recvbuf);
	scanf_s("%d", &iResult);

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	
	/*
	tst* pT = new tst("Bat", 13, 2.5);
	std::ofstream pout("pout.txt", std::ios::binary);
	boost::archive::text_oarchive poa(pout);
	poa << pT;
	pout.close();

	std::ifstream pin("pout.txt", std::ios::binary);
	boost::archive::text_iarchive pia(pin);
	
	tst* pTT;
	pia >> pTT;

	std::cout << pTT->Name << std::endl;
	std::cout << pTT->age << std::endl;
	std::cout << pTT->pi << std::endl;
	*/
	return 0;
}