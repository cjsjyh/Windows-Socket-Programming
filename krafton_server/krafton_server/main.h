#pragma once
#ifndef _MAIN_H_
#define _MAIN_H_

int receiveMessage(SOCKET ConnectSocket);
int sendMessage(SOCKET ConnectSocket);

char sendBuffer[BUFFER_SIZE];
char recvBuffer[BUFFER_SIZE];
std::vector<int> delimiterIndex;

#endif