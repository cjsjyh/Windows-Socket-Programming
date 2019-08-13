#include "socketManager.h"
#include "textReader.h"
#include "dataCenter.h"

#include "applicationClass.h"

applicationClass::applicationClass()
{
	m_Socket = 0;
	m_tReader = 0;
}

applicationClass::~applicationClass()
{

}

void applicationClass::Initialize()
{
	m_tReader = new textReader;
	m_tReader->ReadFile("./datasheet/server_parameter.csv");
	SetDataCenter();

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

void applicationClass::SetDataCenter()
{
	dataCenter::bossHp = dataCenter::bossMaxHp = m_tReader->paramInt.find("BOSS_HP")->second;
	dataCenter::bossPhase2Hp = m_tReader->paramInt.find("BOSS_PHASE2_HP")->second;
	dataCenter::bossPhase3Hp = m_tReader->paramInt.find("BOSS_PHASE3_HP")->second;

	dataCenter::playerMaxHp = m_tReader->paramInt.find("PLAYER_HP")->second;
	for (int i = 0; i < 2; i++)
		dataCenter::playerHp[i] = dataCenter::playerMaxHp;
}