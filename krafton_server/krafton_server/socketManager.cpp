#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <sstream>
#include <vector>

#include <string>



#include "socketManager.h"

#define CLIENT_COUNT 1

socketManager::socketManager()
{
	count = 0;

	for (int i = 0; i < CLIENT_COUNT; i++)
	{
		SOCKET tempSocket = INVALID_SOCKET;
		clientSocket.push_back(tempSocket);

		std::queue<playerInfo*> tempQueue;
		clientReadBuffer.push_back(tempQueue);

		std::mutex* tempMutex = new std::mutex();
		threadLock.push_back(tempMutex);
	}
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

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 0;
	}

	printf("Listening\n");
	//Connect with Clients. Retry on failure
	for (int i = 0; i < CLIENT_COUNT; i++)
		if (!CheckNewConnection(i))
			--i;

	//Make new thread for each Client
	for (int i = 0; i < CLIENT_COUNT; i++)
		clientThread.push_back(std::thread([&,i]() {ListenToClients(i);}));

	//detach threads
	for (int i = 0; i < CLIENT_COUNT; i++)
		clientThread[i].detach();
	
	
	return 1;
}

void socketManager::ListenToClients(int clientId)
{
	bool flag = true;
	while (flag)
	{
		playerInfo* tempMsg = receiveMessage(clientSocket[clientId]);
		if (tempMsg == NULL)
		{
			printf("Receive Failed. terminating thread");
			flag = false;
		}
		else
		{
			threadLock[clientId]->lock();
			std::cout << "[thread]ID: " << tempMsg->playerId << std::endl;
			std::cout << "[thread]mouseX: " << tempMsg->mouseX << std::endl;
			std::cout << "[thread]mouseY: " << tempMsg->mouseY << std::endl;
			std::cout << "[thread]playerPos x:" << tempMsg->playerPos[0] << std::endl << std::endl;
			clientReadBuffer[clientId].push(tempMsg);
			threadLock[clientId]->unlock();
		}
	}
}

void socketManager::PushToClients()
{
	std::vector<int> index;

	int size = clientSendBuffer.size();
	for (int i = 0; i < size; i++)
	{
		for (auto iter = clientSocket.begin(); iter != clientSocket.end(); iter++)
		{
			if (sendMessage(*iter, clientSendBuffer.front()) == -1)
				index.push_back(distance(clientSocket.begin(), iter));
		}
		clientSendBuffer.pop();
	}

	//close invalid sockets
	CloseClientSockets(index);
	return;
}


bool socketManager::CheckNewConnection(int index)
{
	//Accept a client socket
	clientSocket[index] = accept(ListenSocket, NULL, NULL);
	
	//Socket Connection Failed
	if (clientSocket[index] == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		return false;
	}
	//Socket Connection Successful
	else {
		std::string clientId = std::to_string(index);
		const char* clientIdChar = clientId.c_str();
		int iSendResult = send(clientSocket[index], clientIdChar, sizeof(int), 0);
		printf("Client ID Sent\n");

		if (iSendResult > 0)
			return true;
		//If failed to send client id, close and reconnect
		closesocket(clientSocket[index]);
		return false;
	}
}

void socketManager::Shutdown()
{
	closesocket(ListenSocket);
	for(auto iter = clientSocket.begin(); iter != clientSocket.end(); iter++)
		closesocket(*iter);
	WSACleanup();
}


void socketManager::Frame()
{
	for (int i = 0; i < CLIENT_COUNT; i++)
	{
		if (clientReadBuffer[i].size() != 0)
		{
			threadLock[i]->lock();
			while (clientReadBuffer[i].size() != 0)
			{
				clientSendBuffer.push(clientReadBuffer[i].front());
				clientReadBuffer[i].pop();
			}
			threadLock[i]->unlock();
		}
	}

	PushToClients();
}

void socketManager::CloseClientSockets(std::vector<int> index)
{
	for (auto iter = index.end(); iter != index.begin(); iter--)
	{
		std::cout << "Closing client id: " + std::to_string(*iter) << std::endl;;
		closesocket(clientSocket[*iter]);
		clientSocket.erase(clientSocket.begin() + *iter);
	}
}

playerInfo* socketManager::receiveMessage(SOCKET ConnectSocket)
{
	int iResult;
	playerInfo* pInfoPtr = new playerInfo;
	playerInfo pInfo;

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
				ss.write(&(recvBuffer[prevEnd + 1]), delimiterIndex[i] - (prevEnd + 1) );
				boost::archive::text_iarchive ia(ss);
				prevEnd = delimiterIndex[i] + 1;
				
				//playerInfo pInfo;
				ia >> pInfo;
				CopyPlayerInfo(pInfoPtr, &pInfo);
				std::cout << "[recv]ID: " << pInfoPtr->playerId << std::endl;
				std::cout << "[recv]mouseX: " << pInfoPtr->mouseX << std::endl;
				std::cout << "[recv]mouseY: " << pInfoPtr->mouseY << std::endl << std::endl;
			}
		}
		return pInfoPtr;
	}
	else
	{
		printf("recv failed with error: %d\n", WSAGetLastError());
		return NULL;
	}

	return NULL;
}

int socketManager::sendMessage(SOCKET clientSocket, playerInfo* pInfoPtr)
{
	int iSendResult;
	memset(sendBuffer, 0, sizeof(sendBuffer));

	array_sink sink{ sendBuffer };
	stream<array_sink> os{ sink };

	playerInfo pInfo;
	CopyPlayerInfo(&pInfo, pInfoPtr);
	
	boost::archive::text_oarchive oa(os);
	oa << pInfo;
	sendBuffer[strlen(sendBuffer)] = '\n';

	std::string msgLen = std::to_string(strlen(sendBuffer));
	const char* msgLenChar = msgLen.c_str();
	iSendResult = send(clientSocket, msgLenChar, sizeof(int), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		//closesocket(clientSocket);
		//WSACleanup();
		return -1;
	}
	iSendResult = send(clientSocket, sendBuffer, strlen(sendBuffer), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		return -1;
	}

	return iSendResult;
}

void socketManager::CopyPlayerInfo(playerInfo* dest, playerInfo* src)
{
	for (int i = 0; i < sizeof(src->keyInput) / sizeof(int); i++)
		dest->keyInput[i] = src->keyInput[i];
	for (int i = 0; i < sizeof(src->mouseInput); i++)
		dest->mouseInput[i] = src->mouseInput[i];
	for (int i = 0; i < 3; i++)
		dest->playerPos[i] = src->playerPos[i];
	dest->mouseX = src->mouseX;
	dest->mouseY = src->mouseY;
	dest->playerId = src->playerId;
	dest->bossHitCount = src->bossHitCount;
	dest->playerHitCount = src->playerHitCount;
}
