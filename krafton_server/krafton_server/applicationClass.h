#pragma once
#ifndef _APPLICATION_CLASS_H_
#define _APPLICATION_CLASS_H_

class socketManager;

class applicationClass
{
public:
	applicationClass();
	~applicationClass();

	void Initialize();
	void Run();
private:
	socketManager* m_Socket;

};

#endif