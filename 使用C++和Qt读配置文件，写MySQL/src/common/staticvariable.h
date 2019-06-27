//StaticVariable.h
#ifndef _STATICVARIABLE_H_
#define _STATICVARIABLE_H_

#include <qsqldatabase.h>
#include <qstring.h>
//#include "../srccfg/csfint64define.h"  //clw modify 20190520
//#include "../srccfg/mycentertable.h"
#include "sysconfig.h"

#ifdef _MSC_VER //Windows版本
#include <windows.h>
#else //Unix版本
#include <stdio.h>
#endif

//历史库配置
typedef struct{

	QString dbdriver;
	QString dbname;
	QString dbuser;
	QString dbpassword;
	QString strIP;
}DB;

//应用系统
extern QString g_family;
// 全局数据库连接  
extern	QSqlDatabase g_pDB_His1;
extern	QSqlDatabase g_pDB_His2;
extern	QSqlDatabase g_pDB_Model;

extern	DB g_Model;
extern	DB g_His1;
extern	DB g_His2;
extern QString g_dbdriver;
extern  bool g_IsDebug;

//以秒为单位睡眠一段时间
extern void SleepInSecond(int sec);

extern SysConfig g_sysModel;

#endif