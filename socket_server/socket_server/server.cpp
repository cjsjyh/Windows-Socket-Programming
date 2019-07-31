#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include<iostream>
#include<fstream>
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
	char buffer[500] = { 0, };
	array_sink sink{ buffer };
	stream<array_sink> os{ sink };

	tst T("Tab", 31, 3.1415);
	boost::archive::text_oarchive oa(os);
	oa << T;
	std::cout << buffer << std::endl;
	std::cout << std::to_string(std::strlen(buffer)) << std::endl;
	//---------------------
	/*
	char buffer2[500] = { 0, };
	array_sink sink2{ buffer2 };
	stream<array_sink> os2{ sink2 };
	boost::archive::text_iarchive ia(os2);


	std::ifstream input("output.txt", std::ios::binary);
	boost::archive::text_iarchive ia(input);

	tst TT;
	ia >> TT;

	std::cout << TT.Name << std::endl;
	std::cout << TT.age << std::endl;
	std::cout << TT.pi << std::endl;
	*/
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

	// Receive until the peer shuts down the connection
	do {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);

			// Echo the buffer back to the sender
			iSendResult = send(ClientSocket, buffer, strlen(buffer), 0);
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