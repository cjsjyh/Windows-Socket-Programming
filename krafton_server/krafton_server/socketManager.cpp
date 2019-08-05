#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/vector.hpp>
using namespace boost::iostreams;



#include "socketManager.h"

socketManager::socketManager()
{
	count = 0;
}

socketManager::~socketManager()
{

}

int socketManager::Initialize()
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
		return 0;
	}

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 0;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 0;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 0;
	}

	freeaddrinfo(result);


	printf("Preparing to Listen\n");

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 0;
	}

	printf("Listening\n");

	SOCKET tempSocket = INVALID_SOCKET;
	ClientSocket.push_back(tempSocket);
	//--------------------------------------

	CheckNewConnection();
	//CheckNewConnection();

	
	return 1;
}

bool socketManager::CheckNewConnection()
{
	// Accept a client socket
	std::cout << "starting accept" << std::endl;
	ClientSocket[ClientSocket.size()-1] = accept(ListenSocket, NULL, NULL);
	if (ClientSocket[ClientSocket.size() - 1] == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		//closesocket(ListenSocket);
		WSACleanup();
		return false;
	}
	else {
		std::string clientId = std::to_string(ClientSocket.size() - 1);
		const char* clientIdChar = clientId.c_str();
		int iSendResult = send(ClientSocket[ClientSocket.size() - 1], clientIdChar, sizeof(int), 0);
		printf("Client ID Sent\n");

		SOCKET tempSocket = INVALID_SOCKET;
		ClientSocket.push_back(tempSocket);
		printf("Client Connected\n");

		if(iSendResult > 0)
			return true;
		return false;
	}
}

void socketManager::Shutdown()
{
	closesocket(ListenSocket);
	for(auto iter = ClientSocket.begin(); iter != ClientSocket.end(); iter++)
		closesocket(*iter);
	WSACleanup();
}


void socketManager::Frame()
{
	for (auto iter = ClientSocket.begin(); iter != ClientSocket.end(); iter++)
	{
		if(receiveMessage(*iter) != -1)
			PushToClients();
	}	
}

void socketManager::CloseClientSockets(std::vector<int> index)
{
	for (auto iter = index.end(); iter != index.begin(); iter--)
	{
		std::cout << "Closing client id: " + std::to_string(*iter) << std::endl;;
		closesocket(ClientSocket[*iter]);
		ClientSocket.erase(ClientSocket.begin() + *iter);
	}
}

void socketManager::PushToClients()
{
	std::vector<int> index;
	for (auto iter = ClientSocket.begin(); iter != ClientSocket.end() - 1; iter++)
	{
		if (sendMessage(*iter) == -1)
			index.push_back(distance(ClientSocket.begin(), iter));
		std::cout << "count:" + std::to_string(count++) << std::endl;
	}
	CloseClientSockets(index);
	return;
}


int socketManager::receiveMessage(SOCKET ConnectSocket)
{
	int iResult;
	//First receive size of data that needs to be read
	memset(recvBuffer, 0, sizeof(recvBuffer));
	iResult = recv(ConnectSocket, recvBuffer, sizeof(int), 0);
	if (iResult > 0)
	{
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
				ia >> pInfo;

				std::cout << "ID: " << pInfo.playerID << std::endl;
				std::cout << "mouseX: " << pInfo.mouseX << std::endl;
				std::cout << "mouseY: " << pInfo.mouseY << std::endl << std::endl;
			}
			
		}
	}
	else if (iResult == 0)
		printf("Connection closed\n");
	else
		printf("recv failed with error: %d\n", WSAGetLastError());

	return iResult;
}

int socketManager::sendMessage(SOCKET ClientSocket)
{
	int iSendResult;
	memset(sendBuffer, 0, sizeof(sendBuffer));

	array_sink sink{ sendBuffer };
	stream<array_sink> os{ sink };

	/*bool tempBool[3];
	for (int i = 0; i < 3; i++)
		tempBool[i] = pInfo.mouseInput[i];
	int tempInt[10];
	for (int i=0;i<10;i++)
		tempInt[i] = pInfo.keyInput[i];*/
	playerInfo T(88, count, 0, pInfo.mouseInput, pInfo.keyInput);


	boost::archive::text_oarchive oa(os);
	oa << T;
	sendBuffer[strlen(sendBuffer)] = '\n';

	std::string msgLen = std::to_string(strlen(sendBuffer));
	const char* msgLenChar = msgLen.c_str();
	iSendResult = send(ClientSocket, msgLenChar, sizeof(int), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		//closesocket(ClientSocket);
		//WSACleanup();
		return -1;
	}
	iSendResult = send(ClientSocket, sendBuffer, strlen(sendBuffer), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		//closesocket(ClientSocket);
		//WSACleanup();
		return -1;
	}

	//std::cout << sendBuffer << std::endl;
	//printf("Bytes sent: %d\n", iSendResult);

	return iSendResult;
}



class playerInfo
{

public:
	friend class boost::serialization::access;

	playerInfo(int _playerId, int _mouseX, int _mouseY, bool* _mouseInput, int* _keyInput)
		:playerId(_playerId), mouseX(_mouseX), mouseY(_mouseY)
	{
		for (int i = 0; i < sizeof(_mouseInput); i++)
			mouseInput[i] = _mouseInput[i];
		for (int i = 0; i < sizeof(keyInput) / sizeof(int); i++)
			keyInput[i] = _keyInput[i];
	}

	playerInfo() {
	}

	~playerInfo() {

	}
	int playerId;
	int mouseX;
	int mouseY;

	bool mouseInput[3];
	int keyInput[10];

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar& playerId;
		ar& mouseX;
		ar& mouseY;
		ar& mouseInput;
		ar& keyInput;
	}
};
