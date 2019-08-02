#pragma once
#ifndef _PLAYERINFO_H_
#define _PLAYERINFO_H_

class playerInfo
{

public:
	friend class boost::serialization::access;

	playerInfo(int _playerID, int _mouseX, int _mouseY, bool* _mouseInput, int* _keyInput)
		:playerID(_playerID), mouseX(_mouseX), mouseY(_mouseY)
	{
		for (int i = 0; i < sizeof(mouseInput); i++)
			mouseInput[i] = _mouseInput[i];
		for (int i = 0; i < sizeof(keyInput) / sizeof(int); i++)
			keyInput[i] = _keyInput[i];
	}

	playerInfo() {
	}

	~playerInfo() {

	}
	int playerID;
	int mouseX;
	int mouseY;

	bool mouseInput[3];
	int keyInput[10];

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar& playerID;
		ar& mouseX;
		ar& mouseY;
		ar& mouseInput;
		ar& keyInput;
	}
};

#endif