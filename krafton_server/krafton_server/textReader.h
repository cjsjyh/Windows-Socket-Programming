#pragma once
#ifndef _TEXTREADER_H_
#define _TEXTREADER_H_

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>
using namespace std;

class textReader
{
public:
	textReader();
	~textReader();
	bool ReadFile(string);

	map<string, int> paramInt;
	map<string, float> paramFloat;
	map<string, bool> paramBool;
private:

};

#endif