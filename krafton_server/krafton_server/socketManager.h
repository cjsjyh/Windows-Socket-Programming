#pragma once
#ifndef _SOCKETMANAGER_H_
#define _SOCKETMANAGER_H_

#include <mutex>
#include <thread>
#include <queue>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/vector.hpp>
using namespace boost::iostreams;

#define BUFFER_SIZE 512
#define DEFAULT_PORT "27015"

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")



class playerInput
{

public:
	friend class boost::serialization::access;

	playerInput(int _playerId, int _mouseX, int _mouseY, bool* _mouseInput, int* _keyInput)
		:playerId(_playerId), mouseX(_mouseX), mouseY(_mouseY)
	{
		for (int i = 0; i < sizeof(_mouseInput); i++)
			mouseInput[i] = _mouseInput[i];
		for (int i = 0; i < sizeof(keyInput) / sizeof(int); i++)
			keyInput[i] = _keyInput[i];
	}

	playerInput() {
	}

	~playerInput() {

	}
	int playerId;
	int playerPos[3];

	int mouseX, mouseY;
	bool mouseInput[3];
	int keyInput[10];

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar& playerId;
		ar& playerPos;

		ar& mouseX;
		ar& mouseY;
		ar& mouseInput;
		ar& keyInput;
	}
};

class hpInfo {
public:
	friend class boost::serialization::access;
	hpInfo() {
		playerId = -1;
		bossHitCount = -1;
		playerHitCount = -1;
		for (int i = 0; i < sizeof(playerHp) / sizeof(int); i++)
			playerHp[i] = -1;
		bossHp = -1;
		playerMaxHp = -1;
		bossMaxHp = -1;
	}

	hpInfo(int _bossHp, int* _playerHp)
	{
		playerId = -1;
		playerMaxHp = -1;
		bossMaxHp = -1;
		bossHitCount = -1;
		playerHitCount = -1;
		bossHp = _bossHp;
		for (int i = 0; i < 2; i++)
			playerHp[i] = _playerHp[i];
	}

	int playerId;
	int playerHp[2];
	int bossHp;
	int bossHitCount;
	int playerHitCount;

	int playerMaxHp;
	int bossMaxHp;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar& playerId;
		ar& playerHp;
		ar& bossHp;
		ar& bossHitCount;
		ar& playerHitCount;
		ar& playerMaxHp;
		ar& bossMaxHp;
	}
};

class MsgBundle
{
public:
	int type;
	void* ptr;
};

class socketManager
{
public:
	enum DataType
	{
		PLAYER_INFO,
		BOSS_INFO,
		HP_INFO,
		ITEM_INFO,
	};

	socketManager();
	~socketManager();
	int Initialize();
	void Frame();
	void Shutdown();

	bool CheckNewConnection(int);
	void ListenToClients(int);
	void PushToClients();

	MsgBundle* receiveMessage(SOCKET ConnectSocket);
	int sendMessage(SOCKET ConnectSocket, MsgBundle*);
private:
	void CloseClientSockets(std::vector<int>);
	void CopyPlayerInfo(playerInput*, playerInput*);
	void CopyHpInfo(hpInfo*, hpInfo*);

	char sendBuffer[BUFFER_SIZE];
	char recvBuffer[BUFFER_SIZE];
	std::vector<int> delimiterIndex;

	std::vector<SOCKET> clientSocket;
	std::vector<std::thread> clientThread;
	std::vector<std::mutex*> threadLock;

	std::vector<std::queue<MsgBundle*>> clientReadBuffer;
	std::queue<MsgBundle*> clientSendBuffer;

	SOCKET ListenSocket;
	int count;
};

#endif