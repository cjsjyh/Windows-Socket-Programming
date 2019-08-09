#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <sstream>
#include <vector>

#include "dataCenter.h"

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

		std::queue<MsgBundle*> tempQueue;
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

	//Set initial HP parameters
	
	/*
	MsgBundle* tempMsgHp = new MsgBundle;
	hpInfo* tempHp = new hpInfo;
	tempHp->bossHp = dataCenter::bossHp;
	for (int i = 0; i < 2; i++)
		tempHp->playerHp[i] = dataCenter::playerHp[i];
	tempMsgHp->type = HP_INFO;
	tempMsgHp->ptr = tempHp;
	clientSendBuffer.push(tempMsgHp);*/

	MsgBundle* tempMsgParam = new MsgBundle;
	InitialParamBundle* tempParam = new InitialParamBundle(dataCenter::playerMaxHp, 
															dataCenter::bossMaxHp, 
															dataCenter::bossPhase2Hp, 
															dataCenter::bossPhase3Hp);
	tempMsgParam->type = PARAM_INFO;
	tempMsgParam->ptr = tempParam;
	clientSendBuffer.push(tempMsgParam);
	
	PushToClients();

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
		MsgBundle* tempMsg = receiveMessage(clientSocket[clientId]);
		if (tempMsg == NULL)
		{
			printf("Receive Failed. terminating thread");
			flag = false;
		}
		else
		{
			threadLock[clientId]->lock();
			std::cout << "[thread]ID: " << ((playerInput*)(tempMsg->ptr))->playerId << std::endl;
			std::cout << "[thread]mouseX: " << ((playerInput*)(tempMsg->ptr))->mouseX << std::endl;
			std::cout << "[thread]mouseY: " << ((playerInput*)(tempMsg->ptr))->mouseY << std::endl;
			std::cout << "[thread]playerPos x:" << ((playerInput*)(tempMsg->ptr))->playerPos[0] << std::endl << std::endl;
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
				MsgBundle* tempMsg;
				tempMsg = clientReadBuffer[i].front();
				printf("Handling: ");
				printf("%d\n", tempMsg->type);
				switch (tempMsg->type)
				{
				case PLAYER_INFO:
					clientSendBuffer.push(tempMsg);
					break;
				case BOSS_INFO:
					
					break;
				case HP_INFO:
					hpInfo* tempHp;
					tempHp = (hpInfo*)(tempMsg->ptr);
					if (tempHp->playerHitCount > 0)
					{
						--dataCenter::playerHp[0];
						--dataCenter::playerHp[1];
					}
					if(tempHp->bossHitCount >0)
					{
						--dataCenter::bossHp;
					}
					tempHp->playerHp[0] = dataCenter::playerHp[0];
					tempHp->playerHp[1] = dataCenter::playerHp[1];
					tempHp->bossHp = dataCenter::bossHp;
					printf("Player: %d %d Boss: %d\n",tempHp->playerHp[0],tempHp->playerHp[1],tempHp->bossHp);

					clientSendBuffer.push(tempMsg);
					break;
				}
				clientReadBuffer[i].pop();
				printf("done\n");
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

MsgBundle* socketManager::receiveMessage(SOCKET ConnectSocket)
{
	int iResult;

	//Receive Type of Data
	memset(recvBuffer, 0, sizeof(recvBuffer));
	iResult = recv(ConnectSocket, recvBuffer, sizeof(int), 0);
	if (iResult <= 0)
	{
		printf("[ERROR] Recv Type Failed");
		return NULL;
	}
	int msgType = std::stoi(recvBuffer);

	//Receive Size of Data
	memset(recvBuffer, 0, sizeof(recvBuffer));
	iResult = recv(ConnectSocket, recvBuffer, sizeof(int), 0);
	if (iResult <= 0)
	{
		printf("[ERROR] Recv Len Failed");
		return NULL;
	}
	int msgLen = std::stoi(recvBuffer);

	//Receive Data
	memset(recvBuffer, 0, sizeof(recvBuffer));
	iResult = recv(ConnectSocket, recvBuffer, msgLen, 0);
	if (iResult > 0)
	{
		std::stringstream ss;
		ss.write(recvBuffer, msgLen);
		boost::archive::text_iarchive ia(ss);

		MsgBundle* msgBundle = new MsgBundle;

		
		playerInput pInfo;
		hpInfo hInfo;
		InitialParamBundle paramInfo;
		switch (msgType)
		{
		case PLAYER_INFO:
			ia >> pInfo;
			playerInput* pInfoPtr;
			pInfoPtr = new playerInput;
			CopyPlayerInfo(pInfoPtr, &pInfo);
			msgBundle->ptr = pInfoPtr;
			break;

		case BOSS_INFO:

			break;

		case ITEM_INFO:

			break;

		case HP_INFO:
			ia >> hInfo;
			hpInfo* hInfoPtr;
			hInfoPtr = new hpInfo();
			CopyHpInfo(hInfoPtr, &hInfo);
			msgBundle->ptr = hInfoPtr;
			break;
		case PARAM_INFO:
			ia >> paramInfo;
			InitialParamBundle* paramInfoPtr;
			paramInfoPtr = new InitialParamBundle;
			CopyInitialParamBundle(paramInfoPtr, &paramInfo);

			msgBundle->ptr = paramInfoPtr;
			break;
		}
		msgBundle->type = msgType;
	
		return msgBundle;
	}

	printf("[ERROR] Recv Msg Failed\n");
	return NULL;
}

int socketManager::sendMessage(SOCKET ClientSocket, MsgBundle* msgBundle)
{
	int iSendResult;

	memset(sendBuffer, 0, sizeof(sendBuffer));
	array_sink sink{ sendBuffer };
	stream<array_sink> os{ sink };
	boost::archive::text_oarchive oa(os);

	playerInput pinput;
	hpInfo hInfo;
	InitialParamBundle paramInfo;
	switch (msgBundle->type)
	{
	case PLAYER_INFO:
		CopyPlayerInfo(&pinput, (playerInput*)(msgBundle->ptr));
		oa << pinput;
		break;
	case BOSS_INFO:

		break;
	case ITEM_INFO:

		break;
	case HP_INFO:
		CopyHpInfo(&hInfo, (hpInfo*)(msgBundle->ptr));
		oa << hInfo;
		break;
	case PARAM_INFO:
		CopyInitialParamBundle(&paramInfo, (InitialParamBundle*)(msgBundle->ptr));
		oa << paramInfo;
		break;
	}

	sendBuffer[strlen(sendBuffer)] = '\n';

	iSendResult = 0;
	while (!iSendResult)
	{
		std::string msgType = std::to_string(msgBundle->type);
		const char* msgTypeChar = msgType.c_str();
		iSendResult = send(ClientSocket, msgTypeChar, sizeof(int), 0);
		if (!iSendResult)
			printf("Send Type Failed\n");
	}

	iSendResult = 0;
	while (!iSendResult)
	{
		std::string msgLen = std::to_string(strlen(sendBuffer));
		const char* msgLenChar = msgLen.c_str();
		iSendResult = send(ClientSocket, msgLenChar, sizeof(int), 0);
		if (!iSendResult)
			printf("Send Len Failed\n");
	}

	iSendResult = send(ClientSocket, sendBuffer, strlen(sendBuffer), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("Send Msg Failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return -1;
	}

	printf("[Success] Bytes sent: %d\n", iSendResult);
	return iSendResult;
}


void socketManager::CopyPlayerInfo(playerInput* dest, playerInput* src)
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
}

void socketManager::CopyHpInfo(hpInfo* dest, hpInfo* src)
{
	dest->bossHitCount = src->bossHitCount;
	dest->bossHp = src->bossHp;
	dest->playerHitCount = src->playerHitCount;
	for (int i = 0; i < sizeof(src->playerHp) / sizeof(int); i++)
		dest->playerHp[i] = src->playerHp[i];
}

void socketManager::CopyInitialParamBundle(InitialParamBundle* dest, InitialParamBundle* src)
{
	dest->bossMaxHp = src->bossMaxHp;
	dest->bossPhase2Hp = src->bossPhase2Hp;
	dest->bossPhase3Hp = src->bossPhase3Hp;
	dest->playerMaxHp = src->playerMaxHp;
}