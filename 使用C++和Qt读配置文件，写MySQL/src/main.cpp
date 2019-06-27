/************************************************************************/
/*  clw write 20190527：储能文件解析，写mysql服务
/*  main.cpp 主函数
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

#include "../include/other/cfgclientobj.h" //for 主备
#include "./common/turntomainrole.h"                   // for g_toMainRoleMgr.

using namespace std;
using std::string;

#ifndef PROCESSNAME
	#define PROCESSNAME  "esfileserver"
#endif

static QMutex g_mtxToMain;
static bool g_bMainRole = false;//是否为主角色


//判断是否为主角色
bool IsMainRole()
{
	QMutexLocker locker(&g_mtxToMain);
	return g_bMainRole;
};

//设置主备角色
void SetMainRole(bool isMain)
{
	QMutexLocker locker(&g_mtxToMain);
	g_bMainRole = isMain;
};

//发送主备角色
void DoSetRole(bool bToMain)
{
	//if(bToMain)
	//{
	//	g_checkUpdate.SetWriteState(NEEDREAD); 
	//}
	//else
	//{
	//	UpdateCSFRole(false);// 发送备角色
	//	if(g_pScadaWrite) g_pScadaWrite->WriteAllTableData();
	//}
	//SetMainRole(bToMain);  // 必须放 UpdateCSFRole( ) 之后

	//clw modify psk's code: 20190624
	SetMainRole(bToMain);
};

//主备切换之后的行为
void SetRole(int role)
{
	bool bRole = (role==CFGSRV_ROLE_ENABLED);
	if(bRole)
	{
		printf("clw：收到角色变换为主\n");
		WriteDailyLog(true, "clw：收到角色变换为主。");
		if(!IsMainRole())//由备切换为主	
		{
			printf("clw：准备由备切换为主。\n");
			WriteDailyLog(true, "clw：准备由备切换为主。");
			g_toMainRoleMgr.BeginToMainRole();
		}
	}
	else
	{
		printf("clw：收到角色变换为备\n");
		WriteDailyLog(true, "clw：收到角色变换为备。");
		g_toMainRoleMgr.EndToMainRole();
		if(IsMainRole())//由主切换为备
		{
			DoSetRole(false);//发送备角色
		}
	}
};


int main( int argc, char* argv[])
{
	//设置日志
	//printf("文件生成服务esfileserver开始运行！\n");
	SetDailyLogName("esfileserver");
	//SetFileMaxLength(1024*1024*10); // 10M
	WriteDailyLog(true, "文件生成服务开始运行！");

	QApplication app( argc, argv);
	
#ifndef WINDOWS_DEBUG
	printf("clw：注册 appmgr, 参数为进程\n");
	AppmgrSyncAbort(PROCESSNAME);  // 注册 appmgr, 参数为进程名
#endif

#ifdef Q_OS_WIN
	QTextCodec *codec = QTextCodec::codecForName("GBK");
#endif
#ifdef Q_OS_LINUX
	QTextCodec *codec = QTextCodec::codecForName("UTF-8"); //clw note 20190603：应该不需要
#endif

	QTextCodec::setCodecForTr(codec);
	QTextCodec::setCodecForLocale(codec);
	QTextCodec::setCodecForCStrings(codec);

#ifndef WINDOWS_DEBUG
	//设置主备服务切换控制
	CCfgClientObj cfgCliObj;
	printf("clw：cfg注册函数返回%d\n", cfgCliObj.RegisterCfgClient(PROCESSNAME, SetRole));
	cfgCliObj.RegisterCfgClient(PROCESSNAME, SetRole);
#endif

	//启动线程。
	FileParser::Instance()->start();

	return app.exec();
}