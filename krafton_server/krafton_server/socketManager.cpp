#undef UNICODE

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <sstream>
#include <vector>

#include "dataCenter.h"

#include <string>
#include <ctime>
#include <stdlib.h>

#include "socketManager.h"

#define CLIENT_COUNT 1

socketManager::socketManager()
{
	count = 0;
	fadeFlag = false;
	blockInputFrame = 0;

	for (int i = 0; i < CLIENT_COUNT; i++)
	{
		SOCKET tempSocket = INVALID_SOCKET;
		clientSocket.push_back(tempSocket);

		std::queue<MsgBundle*> tempQueue;
		clientReadBuffer.push_back(tempQueue);

		std::queue<MsgBundle*> tempFrame;
		frameCount.push_back(tempFrame);

		std::mutex* tempMutex = new std::mutex();
		threadLock.push_back(tempMutex);
		
		playerReady[i] = false;
	}
}

socketManager::~socketManager()
{

}

int socketManager::Initialize()
{
	curClientCount = CLIENT_COUNT;
	srand((unsigned int)time(NULL));

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
	SendInitialParameters();

	//Make new thread for each Client
	for (int i = 0; i < CLIENT_COUNT; i++)
	{
		int opt_val = TRUE;
		setsockopt(clientSocket[i], IPPROTO_TCP, TCP_NODELAY, (const char*)&opt_val, sizeof(opt_val));
		clientThread.push_back(std::thread([&, i]() {ListenToClients(i); }));
	}
	//detach threads
	for (int i = 0; i < CLIENT_COUNT; i++)
		clientThread[i].detach();
	
	
	return 1;
}

void socketManager::SendInitialParameters()
{
	MsgBundle* tempMsgParam = new MsgBundle;
	InitialParamBundle* tempParam = new InitialParamBundle(dataCenter::playerMaxHp,
		dataCenter::bossMaxHp,
		dataCenter::bossPhase2Hp,
		dataCenter::bossPhase3Hp);
	tempMsgParam->type = PARAM_INFO;
	tempMsgParam->ptr = tempParam;
	clientSendBuffer.push(tempMsgParam);
	PushToClients();
}

void socketManager::ListenToClients(int clientId)
{
	bool flag = true;
	while (flag)
	{
		MsgBundle* tempMsg = receiveMessage(clientId, clientSocket[clientId]);
		if (tempMsg == NULL)
		{
			printf("Receive Failed. Client Lost\n");
			flag = false;
		}
		else
		{
			threadLock[clientId]->lock();
			
			if (tempMsg->type == FRAME_INFO)
			{
				frameCount[clientId].push(tempMsg);
			}
			else
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
		for (int j=0;j<curClientCount;j++)
		{
			if (sendMessage(j, clientSocket[j], clientSendBuffer.front()) == -1)
				index.push_back(j);
		}
		clientSendBuffer.pop();
	}

	//close invalid sockets
	//CloseClientSockets(index);
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
	count++;
	blockInputFrame++;

	//Client들의 Frame 진행도 확인
	bool continueFlag = false;
	if (frameCount[0].size() > 0 && curClientCount != 0)
	{
		continueFlag = true;
		int checkFrame = ((FrameInfo*)(frameCount[0].front()->ptr))->frameNum;

		for (int i = 1; i < curClientCount; i++)
		{
			if (frameCount[i].size() <= 0)
			{
				continueFlag = false;
				break;
			}
			else {
				int tempCheckFrame = ((FrameInfo*)(frameCount[i].front()->ptr))->frameNum;
				if (tempCheckFrame != checkFrame)
					continueFlag = false;
			}
		}
	}
	//Client들이 같은 지점까지 진행되면
	if (continueFlag)
	{
		int currentFrame;
		MsgBundle* sendFrame = frameCount[0].front();

		currentFrame = ((FrameInfo*)(sendFrame->ptr))->frameNum;
		((FrameInfo*)(sendFrame->ptr))->frameNum = currentFrame + 60;
		clientSendBuffer.push(sendFrame);
		
		threadLock[0]->lock();
		frameCount[0].pop();
		threadLock[0]->unlock();

		MsgBundle* garbage;
		for (int i = 1; i < curClientCount; i++)
		{
			threadLock[i]->lock();
			garbage = frameCount[i].front();
			frameCount[i].pop();
			delete garbage->ptr;
			delete garbage;
			threadLock[i]->unlock();
		}

		bool flag = true;
		for (int i = 0; i < curClientCount; i++)
			if (!playerReady[i])
				flag = false;
		if (flag)
		{
			//SEND BOSS PATTERN
			BossInfo* bInfoPtr;
			bInfoPtr = new BossInfo;
			bInfoPtr->patternId = rand() % 5;
			bInfoPtr->frame = currentFrame + 60;
			bInfoPtr->targetid = rand() % 2;

			MsgBundle* bossMsg;
			bossMsg = new MsgBundle;
			bossMsg->type = socketManager::BOSS_INFO;
			bossMsg->ptr = bInfoPtr;
			clientSendBuffer.push(bossMsg);
		}
	}


	//Check player ready
	bool flag = true;
	for (int i = 0; i < curClientCount; i++)
		if (!playerReady[i])
			flag = false;

	//HANDLE INPUT MESSAGE
	for (int i = 0; i < curClientCount; i++)
	{
		if (clientReadBuffer[i].size() != 0)
		{
			threadLock[i]->lock();
			while (clientReadBuffer[i].size() != 0)
			{
				MsgBundle* tempMsg;
				tempMsg = clientReadBuffer[i].front();
				switch (tempMsg->type)
				{
				case PLAYER_INFO:
					clientSendBuffer.push(tempMsg);
					if (!flag && blockInputFrame > 60)
					{
						playerInput* pInfo;
						pInfo = (playerInput*)(tempMsg->ptr);
						if (IsAnyKeyPressed(pInfo))
							playerReady[pInfo->playerId] = true;
					}
					break;
				case HP_INFO:
					hpInfo* tempHp;
					tempHp = (hpInfo*)(tempMsg->ptr);
					HandleHpInfo(tempHp);
					clientSendBuffer.push(tempMsg);
					break;
				}
				clientReadBuffer[i].pop();
			}
			threadLock[i]->unlock();
		}
	}

	//restart game
	bool restartFlag = true;
	for (int i = 0; i < curClientCount; i++)
		if (dataCenter::playerHp[i] > 0)
			restartFlag = false;
	if (dataCenter::bossHp <= 0)
		restartFlag = true;

	if (restartFlag)
	{
		if (!fadeFlag)
		{
			fadeFlag = true;
			for (int i = 0; i < curClientCount; i++)
				playerReady[i] = false;
			blockInputFrame = 0;
		}
		else
		{
			fadeFlag = false;
			dataCenter::playerHp[0] = dataCenter::playerHp[1] = dataCenter::playerMaxHp;
			dataCenter::bossHp = dataCenter::bossMaxHp;
		}
		MsgBundle* tempMsg = new MsgBundle;

		hpInfo* tempHp = new hpInfo;
		tempHp->playerHp[0] = dataCenter::playerHp[0];
		tempHp->playerHp[1] = dataCenter::playerHp[1];
		tempHp->bossHp = dataCenter::bossHp;

		tempMsg->ptr = tempHp;
		tempMsg->type = HP_INFO;
		clientSendBuffer.push(tempMsg);
	}

	PushToClients();
}

void socketManager::CloseClientSockets(std::vector<int> index)
{
	for (int i = index.size() - 1; i >= 0; i--)
	{
		std::cout << "Closing client id: " + index[i] << std::endl;;
		closesocket(clientSocket[index[i]]);
		clientSocket.erase(clientSocket.begin() + i);
	}

}

MsgBundle* socketManager::receiveMessage(int id, SOCKET ConnectSocket)
{
	int iResult;

	//Receive Type of Data
	memset(recvBuffer[id], 0, sizeof(recvBuffer[id]));
	iResult = recv(ConnectSocket, recvBuffer[id], sizeof(int), 0);
	if (iResult <= 0)
	{
		printf("[ERROR] Recv Type Failed");
		return NULL;
	}
	int msgType = std::stoi(recvBuffer[id]);

	//Receive Size of Data
	memset(recvBuffer[id], 0, sizeof(recvBuffer[id]));
	iResult = recv(ConnectSocket, recvBuffer[id], sizeof(int), 0);
	if (iResult <= 0)
	{
		printf("[ERROR] Recv Len Failed");
		return NULL;
	}
	int msgLen = std::stoi(recvBuffer[id]);

	//Receive Data
	memset(recvBuffer[id], 0, sizeof(recvBuffer[id]));
	iResult = recv(ConnectSocket, recvBuffer[id], msgLen, 0);

	if (iResult > 0)
	{
		std::stringstream ss;
		ss.write(recvBuffer[id], msgLen);
		boost::archive::text_iarchive ia(ss);

		MsgBundle* msgBundle = new MsgBundle;

		
		playerInput pInfo;
		hpInfo hInfo;
		InitialParamBundle paramInfo;
		BossInfo bInfo;
		FrameInfo fInfo;
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
			ia >> bInfo;
			BossInfo* bInfoPtr;
			bInfoPtr = new BossInfo;
			CopyBossInfo(bInfoPtr, &bInfo);
			msgBundle->ptr = bInfoPtr;
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

		case FRAME_INFO:
			ia >> fInfo;
			FrameInfo* fInfoPtr;
			fInfoPtr = new FrameInfo;
			CopyFrameInfo(fInfoPtr, &fInfo);
			msgBundle->ptr = fInfoPtr;
			break;
		}
		msgBundle->type = msgType;
	
		return msgBundle;
	}

	printf("[ERROR] Recv Msg Failed\n");
	return NULL;
}

int socketManager::sendMessage(int id, SOCKET ClientSocket, MsgBundle* msgBundle)
{
	int iSendResult;

	memset(sendBuffer[id], 0, sizeof(sendBuffer[id]));
	array_sink sink{ sendBuffer[id] };
	stream<array_sink> os{ sink };
	boost::archive::text_oarchive oa(os);

	playerInput pinput;
	hpInfo hInfo;
	InitialParamBundle paramInfo;
	BossInfo bInfo;
	FrameInfo fInfo;
	switch (msgBundle->type)
	{
	case PLAYER_INFO:
		CopyPlayerInfo(&pinput, (playerInput*)(msgBundle->ptr));
		oa << pinput;
		break;
	case BOSS_INFO:
		CopyBossInfo(&bInfo, (BossInfo*)(msgBundle->ptr));
		oa << bInfo;
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
	case FRAME_INFO:
		CopyFrameInfo(&fInfo, (FrameInfo*)(msgBundle->ptr));
		oa << fInfo;
		break;
	}


	sendBuffer[id][strlen(sendBuffer[id])] = '\n';

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
		std::string msgLen = std::to_string(strlen(sendBuffer[id]));
		const char* msgLenChar = msgLen.c_str();
		iSendResult = send(ClientSocket, msgLenChar, sizeof(int), 0);
		if (!iSendResult)
			printf("Send Len Failed\n");
	}

	iSendResult = send(ClientSocket, sendBuffer[id], strlen(sendBuffer[id]), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("Send Msg Failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return -1;
	}

	printf("[Success] Bytes sent: %d\n", iSendResult);
	return iSendResult;
}


void socketManager::HandleHpInfo(hpInfo* tempHp)
{
	//Damage
	if (tempHp->playerHitCount > 0)
	{
		--dataCenter::playerHp[tempHp->playerId];
		
	}
	if (tempHp->bossHitCount > 0)
	{
		--dataCenter::bossHp;
	}

	//Heal
	for (int i = 0; i < 2; i++)
		dataCenter::playerHp[i] += tempHp->playerHeal[i];
	dataCenter::bossHp += tempHp->bossHeal;

	tempHp->playerHp[0] = dataCenter::playerHp[0];
	tempHp->playerHp[1] = dataCenter::playerHp[1];
	tempHp->bossHp = dataCenter::bossHp;
}

void socketManager::CopyPlayerInfo(playerInput* dest, playerInput* src)
{
	for (int i = 0; i < sizeof(src->keyInput) / sizeof(int); i++)
		dest->keyInput[i] = src->keyInput[i];
	for (int i = 0; i < sizeof(src->mouseInput); i++)
		dest->mouseInput[i] = src->mouseInput[i];
	for (int i = 0; i < 3; i++)
	{
		dest->playerPos[i] = src->playerPos[i];
		dest->mouseDirVec[i] = src->mouseDirVec[i];
	}

	dest->playerId = src->playerId;
}


void socketManager::CopyHpInfo(hpInfo* dest, hpInfo* src)
{
	dest->playerId = src->playerId;
	dest->bossHitCount = src->bossHitCount;
	dest->bossHp = src->bossHp;
	dest->playerHitCount = src->playerHitCount;
	for (int i = 0; i < sizeof(src->playerHp) / sizeof(int); i++)
	{
		dest->playerHp[i] = src->playerHp[i];
		dest->playerHeal[i] = src->playerHeal[i];
	}
	dest->bossHeal = src->bossHeal;
}

void socketManager::CopyInitialParamBundle(InitialParamBundle* dest, InitialParamBundle* src)
{
	dest->bossMaxHp = src->bossMaxHp;
	dest->bossPhase2Hp = src->bossPhase2Hp;
	dest->bossPhase3Hp = src->bossPhase3Hp;
	dest->playerMaxHp = src->playerMaxHp;
}

void socketManager::CopyItemInfo(ItemInfo* dest, ItemInfo* src)
{
	dest->itemId = src->itemId;
	dest->playerId = src->itemId;
}

void socketManager::CopyBossInfo(BossInfo* dest, BossInfo* src)
{
	dest->patternId = src->patternId;
	dest->frame = src->frame;
	dest->targetid = src->targetid;
}

void socketManager::CopyFrameInfo(FrameInfo* dest, FrameInfo* src)
{
	dest->playerId = src->playerId;
	dest->frameNum = src->frameNum;
}

bool socketManager::IsAnyKeyPressed(playerInput* pInput)
{
	for (int i = 0; i < sizeof(pInput->keyInput) / sizeof(int); i++)
		if (pInput->keyInput[i] != 0)
			return true;
	if (pInput->mouseInput[0])
		return true;
	return false;
}