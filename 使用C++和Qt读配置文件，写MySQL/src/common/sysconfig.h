// sysinfo.h: interface for the SYS class.
//
//////////////////////////////////////////////////////////////////////

#ifndef AFX_SYSCONFIG_H__0A802E8E_37FF_4489_8C47_235B61A59CF0__INCLUDED
#define AFX_SYSCONFIG_H__0A802E8E_37FF_4489_8C47_235B61A59CF0__INCLUDED
#include <qstring.h>

#define QT_DRIVERNAME_ORACLE QString("QOCI8")
#define QT_DRIVERNAME_SQLSER QString("QODBC3")
#define QT_DRIVERNAME_MYSQL  QString("QMYSQL3")

class SysConfig  
{
public:
	SysConfig();
	virtual ~SysConfig();

	//判断历史库是否连通
	bool IsHisDBConnected();

	//连接历史库
	bool ConnectDatabas();

	//重新连接数据库
	bool ReConnectDatabas();

	//断开数据库连接
	void DisconnectDatabase();

	//连接配置库
	bool ConnectModelDB();
	//断开配置库连接
	void DisconnectModelDB();


private:
	//读取历史库的配置
	bool ReadConfigInfo();
	//读取配置库的配置
	bool ReadModelDBCfg();

	
};

#endif // !defined(AFX_PVSYSINFO_H__0A802E8E_37FF_4489_8C47_235B61A59CF0__INCLUDED_)
