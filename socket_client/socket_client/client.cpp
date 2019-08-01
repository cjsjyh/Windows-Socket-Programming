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

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


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


int __cdecl main(int argc, char** argv)
{
	//--------------------
	//Setup Socket
	//--------------------
	/*
	// Validate the parameters
	if (argc != 2) {
		printf("usage: %s server-name\n", argv[0]);
		return 1;
	}
	*/
	// Initialize Winsock
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

	SOCKET ConnectSocket = INVALID_SOCKET;

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

	//-------------------------
	// SEND AND RECEIVE DATA
	//-------------------------
	int recvbuflen = DEFAULT_BUFLEN;
	char* sendbuf = (char*)"this is a test";
	char recvbuf[DEFAULT_BUFLEN];
	vector<int> delimiterIndex;


	// Send an initial buffer
	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0); // returns number of bytes sent or error
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	printf("Bytes Sent: %ld\n", iResult);

	// shutdown the sending side of the socket. this allows server to release some resources.
	// the client can still use the ConnectSocket for receiving data
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	// Receive until the peer closes the connection
	do {
		memset(recvbuf, 0, DEFAULT_BUFLEN);
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0); // returns number of bytes received or error
		if (iResult > 0)
		{
			delimiterIndex.clear();
			for (int i = 0; i < strlen(recvbuf); i++)
				if (recvbuf[i] == '\n')
					delimiterIndex.push_back(i);
			cout << "delimiter find done" << endl;
			for (auto iter = delimiterIndex.begin(); iter != delimiterIndex.end(); iter++)
			{
				int messageLen = delimiterIndex[0];
				std::stringstream ss;
				ss.write(&(recvbuf[*iter - messageLen]), messageLen);
				boost::archive::text_iarchive ia(ss);

				tst TT;
				ia >> TT;


				std::cout << TT.Name << std::endl;
				std::cout << TT.age << std::endl;
				std::cout << TT.pi << std::endl << endl;
			}
			//DESERIALIZATION FROM CHAR*
		}
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed with error: %d\n", WSAGetLastError());

	} while (iResult > 0);
	
	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}