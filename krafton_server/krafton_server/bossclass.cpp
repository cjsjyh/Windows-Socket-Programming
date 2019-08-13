#include "dataCenter.h"

#include "bossclass.h"

void bossclass::Frame()
{
	hp = dataCenter::bossHp;
}

int bossclass::GetBossPhase()
{
	if (dataCenter::bossHp <= dataCenter::bossPhase3Hp)
		return 2;
	else if (dataCenter::bossHp <= dataCenter::bossPhase2Hp)
		return 1;
	return 0;
}