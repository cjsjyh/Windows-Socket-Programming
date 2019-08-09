#pragma once
#ifndef _APPLICATION_CLASS_H_
#define _APPLICATION_CLASS_H_

class socketManager;
class textReader;

class applicationClass
{
public:
	applicationClass();
	~applicationClass();

	void Initialize();
	void Run();
private:
	void SetDataCenter();
	
	socketManager* m_Socket;
	textReader* m_tReader;


};

#endif