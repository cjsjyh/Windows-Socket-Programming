#include "socketManager.h"

#include "applicationClass.h"

applicationClass::applicationClass()
{
	m_Socket = 0;
}

applicationClass::~applicationClass()
{

}

void applicationClass::Initialize()
{
	m_Socket = new socketManager();
	m_Socket->Initialize();
}

void applicationClass::Run()
{
	while (1)
	{
		m_Socket->Frame();
	}
}