/************************************************************************/
/*  clw write 20190527�������ļ�������дmysql����
/*  main.cpp ������
/************************************************************************/
#include <fstream>
#include <iostream>

#include <stdio.h>
#include <qapplication.h>
#include <qtextcodec.h>

#include "./common/defs.h"
#include "fileparser.h"
//#include "./common/writelog.h" // for WriteLogToFile
#include "../include/objectbase/writedailylog.h"  //for WriteDailyLog.h"
#include "../interface/appmgr/appmgr.h" // for AppmgrSyncAbort()

#include "../include/other/cfgclientobj.h" //for ����
#include "./common/turntomainrole.h"                   // for g_toMainRoleMgr.

using namespace std;
using std::string;

#ifndef PROCESSNAME
	#define PROCESSNAME  "esfileserver"
#endif

static QMutex g_mtxToMain;
static bool g_bMainRole = false;//�Ƿ�Ϊ����ɫ


//�ж��Ƿ�Ϊ����ɫ
bool IsMainRole()
{
	QMutexLocker locker(&g_mtxToMain);
	return g_bMainRole;
};

//����������ɫ
void SetMainRole(bool isMain)
{
	QMutexLocker locker(&g_mtxToMain);
	g_bMainRole = isMain;
};

//����������ɫ
void DoSetRole(bool bToMain)
{
	//if(bToMain)
	//{
	//	g_checkUpdate.SetWriteState(NEEDREAD); 
	//}
	//else
	//{
	//	UpdateCSFRole(false);// ���ͱ���ɫ
	//	if(g_pScadaWrite) g_pScadaWrite->WriteAllTableData();
	//}
	//SetMainRole(bToMain);  // ����� UpdateCSFRole( ) ֮��

	//clw modify psk's code: 20190624
	SetMainRole(bToMain);
};

//�����л�֮�����Ϊ
void SetRole(int role)
{
	bool bRole = (role==CFGSRV_ROLE_ENABLED);
	if(bRole)
	{
		printf("clw���յ���ɫ�任Ϊ��\n");
		WriteDailyLog(true, "clw���յ���ɫ�任Ϊ����");
		if(!IsMainRole())//�ɱ��л�Ϊ��	
		{
			printf("clw��׼���ɱ��л�Ϊ����\n");
			WriteDailyLog(true, "clw��׼���ɱ��л�Ϊ����");
			g_toMainRoleMgr.BeginToMainRole();
		}
	}
	else
	{
		printf("clw���յ���ɫ�任Ϊ��\n");
		WriteDailyLog(true, "clw���յ���ɫ�任Ϊ����");
		g_toMainRoleMgr.EndToMainRole();
		if(IsMainRole())//�����л�Ϊ��
		{
			DoSetRole(false);//���ͱ���ɫ
		}
	}
};


int main( int argc, char* argv[])
{
	//������־
	//printf("�ļ����ɷ���esfileserver��ʼ���У�\n");
	SetDailyLogName("esfileserver");
	//SetFileMaxLength(1024*1024*10); // 10M
	WriteDailyLog(true, "�ļ����ɷ���ʼ���У�");

	QApplication app( argc, argv);
	
#ifndef WINDOWS_DEBUG
	printf("clw��ע�� appmgr, ����Ϊ����\n");
	AppmgrSyncAbort(PROCESSNAME);  // ע�� appmgr, ����Ϊ������
#endif

#ifdef Q_OS_WIN
	QTextCodec *codec = QTextCodec::codecForName("GBK");
#endif
#ifdef Q_OS_LINUX
	QTextCodec *codec = QTextCodec::codecForName("UTF-8"); //clw note 20190603��Ӧ�ò���Ҫ
#endif

	QTextCodec::setCodecForTr(codec);
	QTextCodec::setCodecForLocale(codec);
	QTextCodec::setCodecForCStrings(codec);

#ifndef WINDOWS_DEBUG
	//�������������л�����
	CCfgClientObj cfgCliObj;
	printf("clw��cfgע�ắ������%d\n", cfgCliObj.RegisterCfgClient(PROCESSNAME, SetRole));
	cfgCliObj.RegisterCfgClient(PROCESSNAME, SetRole);
#endif

	//�����̡߳�
	FileParser::Instance()->start();

	return app.exec();
}