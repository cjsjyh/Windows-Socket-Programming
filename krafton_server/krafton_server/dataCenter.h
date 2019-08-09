#pragma once
#ifndef _DATACENTER_H_
#define _DATACENTER_H_

class dataCenter
{
public:
	static int playerMaxHp;
	static int playerHp[2];
	static int playerAttackType[2];
	static int playerPassiveType[2];

	static int bossHp;	
	static int bossMaxHp;
	static int bossPhase2Hp;
	static int bossPhase3Hp;
};

#endif