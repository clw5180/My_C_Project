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

	//�ж���ʷ���Ƿ���ͨ
	bool IsHisDBConnected();

	//������ʷ��
	bool ConnectDatabas();

	//�����������ݿ�
	bool ReConnectDatabas();

	//�Ͽ����ݿ�����
	void DisconnectDatabase();

	//�������ÿ�
	bool ConnectModelDB();
	//�Ͽ����ÿ�����
	void DisconnectModelDB();


private:
	//��ȡ��ʷ�������
	bool ReadConfigInfo();
	//��ȡ���ÿ������
	bool ReadModelDBCfg();

	
};

#endif // !defined(AFX_PVSYSINFO_H__0A802E8E_37FF_4489_8C47_235B61A59CF0__INCLUDED_)
