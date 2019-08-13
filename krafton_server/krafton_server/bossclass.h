#pragma once
#ifndef _BOSSCLASS_H_
#define _BOSSCLASS_H_
class bossclass
{
public:
	bossclass();
	~bossclass();

	void Frame();
	int GetBossPhase();

private:
	int hp;
};

#endif