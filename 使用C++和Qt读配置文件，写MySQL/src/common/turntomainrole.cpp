//turntomainrole.cpp

#include "turntomainrole.h"                      // for g_toMainRoleMgr.
//#include "../include/objectbase/datetime.h"      // for CDateTime
#include "cdatetime.h"      // for CDateTime
#include "../../include/objectbase/writedailylog.h"
#include <stdio.h>

CTurntoMainRole g_toMainRoleMgr;

//发送主备角色
extern void DoSetRole(bool bToMain);

CTurntoMainRole::CTurntoMainRole()
{
	s_toMainRole = false;//是否切换为主角色
	s_toMainTime = 0.0;
	//BeginToMainRole();
}

CTurntoMainRole::~CTurntoMainRole()
{
	while(running()) msleep(50);
}

void CTurntoMainRole::run()//[不访问实时库]
{
	while(IsToMainRole())
	{
		msleep(200);
		if(DoToMainRole())
		{
			printf("clw：DoToMainRole()返回true，准备发送主角色。\n");
			WriteDailyLog(true, "clw：DoToMainRole()返回true，准备发送主角色。");
			DoSetRole(true);//发送主角色
			EndToMainTime();
		}
		//else
		//{
		//	printf("clw：DoToMainRole()返回false。\n");
		//	WriteDailyLog(true, "clw：DoToMainRole()返回false。");
		//}
	}
}

bool CTurntoMainRole::IsToMainRole()
{
	QMutexLocker locker(&s_mtxToMain);
	return s_toMainRole;
}

void CTurntoMainRole::BeginToMainRole()
{
	QMutexLocker locker(&s_mtxToMain);
	if(!s_toMainRole)
	{
		s_toMainRole = true;
		//s_toMainTime = (double)CDateTime::GetCurrentDateTime() + 10.0 / 86400.0; //clw note：n秒后转入主角色，默认10s
		s_toMainTime = (double)CDateTime::GetCurrentDateTime() + 0.1 / 86400.0;
		start();
	}
}

bool CTurntoMainRole::DoToMainRole()
{
	QMutexLocker locker(&s_mtxToMain);
	return (s_toMainTime > 0.0001 && s_toMainTime <= (double)CDateTime::GetCurrentDateTime());
}

void CTurntoMainRole::EndToMainRole()
{
	QMutexLocker locker(&s_mtxToMain);
	s_toMainRole = false;
	s_toMainTime = 0.0;
}

void CTurntoMainRole::EndToMainTime()
{
	QMutexLocker locker(&s_mtxToMain);
	s_toMainTime = 0.0;
}
