//turntomainrole.cpp

#include "turntomainrole.h"                      // for g_toMainRoleMgr.
//#include "../include/objectbase/datetime.h"      // for CDateTime
#include "cdatetime.h"      // for CDateTime
#include "../../include/objectbase/writedailylog.h"
#include <stdio.h>

CTurntoMainRole g_toMainRoleMgr;

//����������ɫ
extern void DoSetRole(bool bToMain);

CTurntoMainRole::CTurntoMainRole()
{
	s_toMainRole = false;//�Ƿ��л�Ϊ����ɫ
	s_toMainTime = 0.0;
	//BeginToMainRole();
}

CTurntoMainRole::~CTurntoMainRole()
{
	while(running()) msleep(50);
}

void CTurntoMainRole::run()//[������ʵʱ��]
{
	while(IsToMainRole())
	{
		msleep(200);
		if(DoToMainRole())
		{
			printf("clw��DoToMainRole()����true��׼����������ɫ��\n");
			WriteDailyLog(true, "clw��DoToMainRole()����true��׼����������ɫ��");
			DoSetRole(true);//��������ɫ
			EndToMainTime();
		}
		//else
		//{
		//	printf("clw��DoToMainRole()����false��\n");
		//	WriteDailyLog(true, "clw��DoToMainRole()����false��");
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
		//s_toMainTime = (double)CDateTime::GetCurrentDateTime() + 10.0 / 86400.0; //clw note��n���ת������ɫ��Ĭ��10s
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
