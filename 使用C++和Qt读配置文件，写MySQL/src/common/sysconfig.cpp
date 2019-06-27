// sysinfo.cpp: implementation of the PVSYS class.
//
//////////////////////////////////////////////////////////////////////

#include "sysconfig.h"
#include "staticvariable.h"
#include <qdatetime.h>
#include <qfile.h>
#include <q3textstream.h>
//Added by qt3to4:
#include <QSqlQuery>
#include <QVariant>
#include "../../include/other/icssettingslib.h"
#include <fstream>
#include <iostream>
//#include "writelog.h" // for WriteLogToFile
#include "../../include/objectbase/writedailylog.h" 
/*
clw modify 20190625��֮ǰ��д��־�������ã�
ÿ���������񶼻���һ���ļ�������������������̫�ࣻ
���Ǹĳ��ô������־����������һ����־����xxx_log��
Ȼ��ÿ�ζ�������ļ�д��д��10M��ʱ����һ���ļ���
����_��ʱ����Ϊ���֣�
*/

#include "defs.h"
using namespace std;

#define PATH_SECTION_HISDB1  "plat_global/database/hisdb/hisdb1"  //��ʷ��·��
#define PATH_SECTION_HISDB2  "plat_global/database/hisdb/hisdb2"
#define PATH_SECTION_MODELDB  "plat_global/database/modeldb"  //���ÿ�·��

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SysConfig::SysConfig()
{

}

SysConfig::~SysConfig()
{

}

bool SysConfig::IsHisDBConnected()
{
	string sql;
	if (g_dbdriver.toStdString().substr(0, 4) == "QOCI")  
	{  
		sql.assign("select * from sys.sysdual"); //only for DM ����
	}
	else if(g_dbdriver.toStdString().substr(0, 6) == "QMYSQL")
	{	
		sql.assign( "show tables" );
	}
	else
	{
		;
		WriteDailyLog(true, "clw�����ݿ����ݿ�����ӦΪQOCI��QMYSQL���ͣ�����icssetting.ini�е����ã�");
	}
	QSqlQuery query(g_pDB_His1);
	return query.exec(sql.c_str());
}

bool SysConfig::ReadModelDBCfg()
{
	bool bSuc = false;
	ifstream iscsettingFile;

	QFile file("../ini/icssetting.xml");

	if(file.exists())
	{	
		bSuc = true;
	}
	else
	{
		printf("clw���Ҳ���../ini/icssetting.xml�ļ���\n");	
		WriteDailyLog(true, "clw���Ҳ���../ini/icssetting.xml�ļ���");
		return bSuc;
	}

	if(bSuc==true)
	{
		//read configurations from icssetting.xml 
		IICSSettings* pSettings = 0;
		CICSSettings libICSSetting;
		pSettings = libICSSetting.Self();
		if(pSettings)
		{
			
			//clw modify 20190102������icssetting��ȡ"smgserver"item���ܣ������ֲ���̬����������̬��
			if(pSettings->SetCurrentSectionPath("plat_scada/detaillog/"))
			{
				g_IsDebug = pSettings->ItemReadBool("scadasrv");//�Ƿ��ڲ��Խ׶�  plat_scada/detaillog/smgserver
				if(g_IsDebug == true)
				{
					printf("clw����������ģʽ��\n");
					WriteDailyLog(true, "clw����������ģʽ��\n");
				}
			}

			char buf[BUF_LEN_MAX];
			if(true == pSettings->SetCurrentSectionPath(PATH_SECTION_MODELDB))//���õ�ǰ�����Ľڵ�·����������Ϻ��Ժ���������ڱ��ڵ����
			{
				if(pSettings->ItemReadString("driver",buf, BUF_LEN_MAX))			//virtual bool ItemReadString(const char* item, char * sValueOut,int iSizeIn ) = 0;
				{															       //\brief ���غ��������ַ����ķ��������������ֵ 
					g_dbdriver = g_Model.dbdriver = buf;	                     //@param[in] item ����������
	                                        							       //@param[in] val  Ҫ���õ�ֵ
				}
				//\return ���óɹ����� true ����ʧ�ܷ��� false
				if(pSettings->ItemReadString("username", buf, BUF_LEN_MAX))
				{
					g_Model.dbuser = buf;
					//printf("clw��g_Model.dbuser = %s\n", buf);
					//string strLogInfo = "clw��g_Model.dbuser = " + string(buf);
					//WriteDailyLog(true, strLogInfo);
				}	
				else
				{
					printf("clw��������icssetting��username����Ϣ��\n");	
					WriteDailyLog(true, "clw��������icssetting��username����Ϣ��");
				}

				if(pSettings->ItemReadString("password", buf, BUF_LEN_MAX))	
				{
					g_Model.dbpassword = buf;
					//printf("clw��g_Model.dbpassword = %s\n", buf);
				}
				if(pSettings->ItemReadString("sername", buf, BUF_LEN_MAX))	
				{
					g_Model.dbname = buf;
					//printf("clw��g_Model.dbname = %s\n", buf);
				}
				if(pSettings->ItemReadString("hostname", buf, BUF_LEN_MAX))	
				{
					g_Model.strIP = buf;
					//printf("clw��g_Model.strIP = %s\n", buf);
				}


				iscsettingFile.close();
				iscsettingFile.clear();
				bSuc = true;
			}
			else bSuc = false;
		}
		else bSuc = false;

		iscsettingFile.close();
		iscsettingFile.clear();
	}
	return bSuc;
}

//����Ϣ
bool SysConfig::ReadConfigInfo()
{
	bool bSuc = false;
	ifstream iscsettingFile;

	QFile file("../ini/icssetting.xml");

	if(file.exists())
	{	
		bSuc = true;
	}
	else
	{
		printf("clw���Ҳ���../ini/icssetting.xml�ļ���\n");	
		WriteDailyLog(true, "clw���Ҳ���../ini/icssetting.xml�ļ���");
	}

	if(bSuc==true)
	{
		//read configurations from icssetting.xml 
		IICSSettings* pSettings = 0;
		CICSSettings libICSSetting;
		pSettings = libICSSetting.Self();
		if(pSettings)
		{
			char buf[BUF_LEN_MAX];
			if(true == pSettings->SetCurrentSectionPath(PATH_SECTION_HISDB1))//���õ�ǰ�����Ľڵ�·����������Ϻ��Ժ���������ڱ��ڵ����
			{
				if(pSettings->ItemReadString("driver", buf, BUF_LEN_MAX))			//virtual bool ItemReadString(const char* item, char * sValueOut,int iSizeIn ) = 0;
				{															//\brief ���غ��������ַ����ķ��������������ֵ 
					g_dbdriver = buf;								    //@param[in] item ����������
					g_His1.dbdriver = buf;								//@param[in] val  Ҫ���õ�ֵ
				}														//\return ���óɹ����� true ����ʧ�ܷ��� false
				if(pSettings->ItemReadString("username", buf, BUF_LEN_MAX))
				{
					g_His1.dbuser = buf;
					//printf("clw��g_His1.dbuser = %s\n", buf);
					//string strLogInfo = "clw��g_Model.dbuser = " + string(buf);
					//WriteDailyLog(true, strLogInfo);
				}	
				else
				{
					printf("clw��������icssetting��username����Ϣ��\n");
					WriteDailyLog(true, "clw��������icssetting��username����Ϣ��");
				}
				if(pSettings->ItemReadString("password", buf, BUF_LEN_MAX))	
				{
					g_His1.dbpassword = buf;
					//printf("clw��g_His1.dbpassword = %s\n", buf);
				}
				if(pSettings->ItemReadString("sername", buf, BUF_LEN_MAX))	
				{
					g_His1.dbname = buf;
					//printf("clw��g_His1.dbname = %s\n", buf);
				}
				if(pSettings->ItemReadString("hostname", buf, BUF_LEN_MAX))	
				{
					g_His1.strIP = buf;
					//printf("clw��g_His1.strIP = %s\n", buf);
				}

				iscsettingFile.close();
				iscsettingFile.clear();
				bSuc = true;
			}
			else bSuc = false;
		}
		else bSuc = false;

		iscsettingFile.close();
		iscsettingFile.clear();
	}
	return bSuc;
}

bool SysConfig::ConnectDatabas()
{
	if(g_pDB_His1.isOpen())
		return true;

	bool isConn = true;
	if(ReadConfigInfo())
	{
		QString curData = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");//������ǰʱ��
		g_pDB_His1 = QSqlDatabase::addDatabase(g_His1.dbdriver,"HIS1"+curData );
		//�������ݿ����ӵĻ�������

		g_pDB_His1.setUserName(g_His1.dbuser);
		g_pDB_His1.setPassword(g_His1.dbpassword);
		//if(strcmp(g_dbdriver, "QMYSQL3") == 0)

		//printf("clw��g_dbdriver.left(4)=%s\n", g_dbdriver.left(4).toLatin1().data()) ;
		//printf("clw��g_dbdriver.left(4) == QOCI �Ƿ�Ϊ��%d\n",  g_dbdriver.left(4) == "QOCI");

		if(g_dbdriver.left(4) == "QOCI")  //clw add 20190603��for ���� Database��
		{
			//if(g_His1.dbdriver.toStdString().substr(0,4) == "QOCI")
			g_pDB_His1.setDatabaseName(g_His1.strIP);
			g_pDB_His1.setPort(5236);
			//printf("clw���������ݿ⣬setDatabaseName(%s)\n", g_His1.strIP.toLatin1().data());
		}
		else if(g_dbdriver.left(6) == "QMYSQL")
		{
			g_pDB_His1.setDatabaseName(g_His1.dbname);
			g_pDB_His1.setHostName(g_His1.strIP);
			//printf("clw��mysql���ݿ⣬setDatabaseName(%s)\n", g_His1.dbname.toLatin1().data());
		}

		if ( !g_pDB_His1.open() )
		{
			printf("clw����ʷ���ʧ�ܣ�\n");
			WriteDailyLog(true, "clw����ʷ���ʧ�ܣ�\n");
			isConn = false;
		}
		else
		{
#ifdef Q_OS_WIN
			g_pDB_His1.exec("SET NAMES 'GBK'");
#endif

#ifdef Q_OS_LINUX
			g_pDB_His1.exec("SET NAMES 'UTF8'");
#endif

			//clw modify 20190116������������ʱ����
			//shortloadhisdata��ultraloadhisdata��
			QDateTime curTime = QDateTime::currentDateTime();  //��ȡ��ǰʱ��
			QString curYear = curTime.toString("yyyy"); //��ǰʱ��ת��Ϊ�ַ������ͣ���ʽΪ��������ʱ���룬��2019-05-30 14:28:15
			
			QSqlQuery query(g_pDB_His1);
			bool bSqlExec1 = true;
			bool bSqlExec2 = true;
			bool bSqlExec3 = true;
			bool bSqlExec4 = true;
			bool bSqlExec5 = true;
			bool bSqlExec6 = true;
			QString qSqlToExec = "";
			if(g_dbdriver.left(4) == "QOCI")
			{
				qSqlToExec = " \
							 CREATE TABLE " + g_His1.dbname + "." + "SHORTLOADHISDATA_" + curYear +  \
							 "( \
							 ID                   INT NOT NULL, \
							 FORCASTTIME          DATETIME NOT NULL, \
							 SAVETIME             DATETIME NOT NULL, \
							 METHOD               INT NOT NULL, \
							 FORCASTP             NUMERIC(22,4), \
							 REALTP               NUMERIC(22,4), \
							 FORCASTERR           NUMERIC(22,4), \
							 REPAIRP              NUMERIC(22,4), \
							 PRIMARY KEY (ID, FORCASTTIME, METHOD, SAVETIME) \
							 );  ";
				bSqlExec1 = query.exec(qSqlToExec); 


				qSqlToExec = "CREATE TABLE " + g_His1.dbname + "." + "ULTRALOADHISDATA_" + curYear + \
					"( \
					ID                   INT NOT NULL, \
					FORCASTTIME          DATETIME NOT NULL, \
					SAVETIME             DATETIME NOT NULL, \
					METHOD               INT NOT NULL, \
					FORCASTP             NUMERIC(22,4), \
					REALTP               NUMERIC(22,4), \
					FORCASTERR           NUMERIC(22,4), \
					REPAIRP              NUMERIC(22,4), \
					PRIMARY KEY (ID, FORCASTTIME, METHOD, SAVETIME) \
					);";
				bSqlExec2 = query.exec(qSqlToExec); 


				qSqlToExec = "CREATE TABLE " + g_His1.dbname + "." + "SHORTHISDATA_" + curYear + \
					"( \
					PVSTATIONID			INT NOT NULL, \
					FORCASTTIME			DATETIME NOT NULL, \
					SAVETIME			DATETIME NOT NULL, \
					METHOD				INT NOT NULL, \
					FORCASTP             NUMERIC(22,4), \
					REALTP               NUMERIC(22,4), \
					FORCASTERR           NUMERIC(22,4), \
					REPAIRP              NUMERIC(22,4), \
					DATAINTERVAL		 INT, \
					COST				 NUMERIC(22,4), \
					CAPACITY			 NUMERIC(22,4), \
					LIMITRATE			 NUMERIC(22,4), \
					FORCASTCP			 NUMERIC(22,4), \
					REALCP				 NUMERIC(22,4), \
					CFORCASTERR			 NUMERIC(22,4), \
					PRIMARY KEY (PVSTATIONID, FORCASTTIME, SAVETIME, METHOD) \
					);";
				bSqlExec3 = query.exec(qSqlToExec); 


				qSqlToExec = "CREATE TABLE " + g_His1.dbname + "." + "ULTRAHISDATA_" + curYear + \
					"( \
					PVSTATIONID			INT NOT NULL, \
					FORCASTTIME			DATETIME NOT NULL, \
					SAVETIME			DATETIME NOT NULL, \
					METHOD				INT NOT NULL, \
					FORCASTP             NUMERIC(22,4), \
					REALTP               NUMERIC(22,4), \
					FORCASTERR           NUMERIC(22,4), \
					REPAIRP              NUMERIC(22,4), \
					DATAINTERVAL		 INT, \
					COST				 NUMERIC(22,4), \
					CAPACITY			 NUMERIC(22,4), \
					LIMITRATE			 NUMERIC(22,4), \
					FORCASTCP			 NUMERIC(22,4), \
					REALCP				 NUMERIC(22,4), \
					CFORCASTERR			 NUMERIC(22,4), \
					PRIMARY KEY (PVSTATIONID, FORCASTTIME, SAVETIME, METHOD) \
					);";
				bSqlExec4 = query.exec(qSqlToExec); 


				qSqlToExec = "CREATE TABLE " + g_His1.dbname + "." + "STAGCNEXTDAYPLAN_" + curYear + \
					"( \
					ID                   INT NOT NULL, \
					PLANDATE                   DATETIME NOT NULL, \
					SAVETIME				   DATETIME NOT NULL, \
					PLANVALUESQ          INT NOT NULL, \
					AUTOPLANAIM                NUMERIC(22,4) NOT NULL, \
					PLANREVISEDAIM             NUMERIC(22,4) NOT NULL, \
					OPRTREVISEDAIM             NUMERIC(22,4) NOT NULL, \
					PLANUSERID               VARCHAR(32), \
					PLANREVISEDTIME           DATETIME, \
					OPRTUSERID              VARCHAR(32), \
					OPRTREVISEDTIME	         DATETIME, \
					CTRLVALUE               NUMERIC(22,4), \
					CTRLTIME                  DATETIME, \
					DIFFEREASON             VARCHAR(64), \
					PRIMARY KEY (ID, PLANDATE, SAVETIME) \
					); ";
				bSqlExec5 = query.exec(qSqlToExec); 


				qSqlToExec = "CREATE TABLE " + g_His1.dbname + "." + "STAGCTODAYPLAN_" + curYear + \
					"( \
					ID                   INT NOT NULL, \
					PLANDATE                   DATETIME NOT NULL, \
					SAVETIME					DATETIME NOT NULL, \
					PLANVALUESQ          INT NOT NULL, \
					AUTOPLANAIM                NUMERIC(22,4) NOT NULL, \
					PLANREVISEDAIM             NUMERIC(22,4) NOT NULL, \
					OPRTREVISEDAIM             NUMERIC(22,4) NOT NULL, \
					PLANUSERID               VARCHAR(32), \
					PLANREVISEDTIME           DATETIME, \
					OPRTUSERID              VARCHAR(32), \
					OPRTREVISEDTIME	         DATETIME, \
					CTRLVALUE               NUMERIC(22,4), \
					CTRLTIME                  DATETIME, \
					DIFFEREASON             VARCHAR(64), \
					PRIMARY KEY (ID, PLANDATE, SAVETIME) \
					);";
				bSqlExec6 = query.exec(qSqlToExec);
			}



			else if(g_dbdriver.left(6) == "QMYSQL")
			{
				qSqlToExec = "CREATE TABLE SHORTLOADHISDATA_" + curYear + \
					"( \
					ID                   INT NOT NULL COMMENT 'CTRLID', \
					FORCASTTIME          DATETIME NOT NULL, \
					SAVETIME             DATETIME NOT NULL, \
					METHOD               INT NOT NULL, \
					FORCASTP             DOUBLE, \
					REALTP               FLOAT, \
					FORCASTERR           DOUBLE, \
					REPAIRP              FLOAT, \
					PRIMARY KEY (ID, FORCASTTIME, METHOD, SAVETIME) \
					);  ";
				bSqlExec1 = query.exec(qSqlToExec); 


				qSqlToExec = "CREATE TABLE ULTRALOADHISDATA_" + curYear + \
					"( \
					ID                   INT NOT NULL COMMENT 'CTRLID', \
					FORCASTTIME          DATETIME NOT NULL, \
					SAVETIME             DATETIME NOT NULL, \
					METHOD               INT NOT NULL, \
					FORCASTP             DOUBLE, \
					REALTP               FLOAT, \
					FORCASTERR           DOUBLE, \
					REPAIRP              FLOAT, \
					PRIMARY KEY (ID, FORCASTTIME, METHOD, SAVETIME) \
					);";
				bSqlExec2 = query.exec(qSqlToExec); 


				qSqlToExec = "CREATE TABLE SHORTHISDATA_" + curYear + \
					"( \
					PVSTATIONID			INT NOT NULL COMMENT '��¼ID', \
					FORCASTTIME			DATETIME NOT NULL, \
					SAVETIME			DATETIME NOT NULL, \
					METHOD				INT NOT NULL, \
					FORCASTP             DOUBLE, \
					REALTP               FLOAT, \
					FORCASTERR           DOUBLE, \
					REPAIRP              FLOAT, \
					DATAINTERVAL		 INT, \
					COST				 FLOAT, \
					CAPACITY			 FLOAT, \
					LIMITRATE			 FLOAT, \
					FORCASTCP			 DOUBLE, \
					REALCP				 FLOAT, \
					CFORCASTERR			 DOUBLE, \
					PRIMARY KEY (PVSTATIONID, FORCASTTIME, SAVETIME, METHOD) \
					);";
				bSqlExec3 = query.exec(qSqlToExec); 


				qSqlToExec = "CREATE TABLE ULTRAHISDATA_" + curYear + \
					"( \
					PVSTATIONID			INT NOT NULL COMMENT '��¼ID', \
					FORCASTTIME			DATETIME NOT NULL, \
					SAVETIME			DATETIME NOT NULL, \
					METHOD				INT NOT NULL, \
					FORCASTP             DOUBLE, \
					REALTP               FLOAT, \
					FORCASTERR           DOUBLE, \
					REPAIRP              FLOAT, \
					DATAINTERVAL		 INT, \
					COST				 FLOAT, \
					CAPACITY			 FLOAT, \
					LIMITRATE			 FLOAT, \
					FORCASTCP			 DOUBLE, \
					REALCP				 FLOAT, \
					CFORCASTERR			 DOUBLE, \
					PRIMARY KEY (PVSTATIONID, FORCASTTIME, SAVETIME, METHOD) \
					);";
				bSqlExec4 = query.exec(qSqlToExec); 


				qSqlToExec = "CREATE TABLE STAGCNEXTDAYPLAN_" + curYear + \
					"( \
					ID                   INT NOT NULL COMMENT '��¼ID', \
					PLANDATE                   DATETIME NOT NULL, \
					SAVETIME                   DATETIME NOT NULL, \
					PLANVALUESQ          INT NOT NULL, \
					AUTOPLANAIM                FLOAT NOT NULL, \
					PLANREVISEDAIM             FLOAT NOT NULL, \
					OPRTREVISEDAIM             FLOAT NOT NULL, \
					PLANUSERID               VARCHAR(32), \
					PLANREVISEDTIME           DATETIME, \
					OPRTUSERID              VARCHAR(32), \
					OPRTREVISEDTIME	         DATETIME, \
					CTRLVALUE               FLOAT COMMENT '��ŵ��С' , \
					CTRLTIME                  DATETIME, \
					DIFFEREASON             VARCHAR(64), \
					PRIMARY KEY (ID, PLANDATE, SAVETIME) \
					); ";
				bSqlExec5 = query.exec(qSqlToExec); 


				qSqlToExec = "CREATE TABLE STAGCTODAYPLAN_" + curYear + \
					"( \
					ID                   INT NOT NULL COMMENT '��¼ID', \
					PLANDATE                   DATETIME NOT NULL, \
					SAVETIME                   DATETIME NOT NULL, \
					PLANVALUESQ          INT NOT NULL, \
					AUTOPLANAIM                FLOAT NOT NULL, \
					PLANREVISEDAIM             FLOAT NOT NULL, \
					OPRTREVISEDAIM             FLOAT NOT NULL, \
					PLANUSERID               VARCHAR(32), \
					PLANREVISEDTIME           DATETIME, \
					OPRTUSERID              VARCHAR(32), \
					OPRTREVISEDTIME	         DATETIME, \
					CTRLVALUE               FLOAT COMMENT '��ŵ��С' , \
					CTRLTIME                  DATETIME, \
					DIFFEREASON             VARCHAR(64), \
					PRIMARY KEY (ID, PLANDATE, SAVETIME) \
					);";
				bSqlExec6 = query.exec(qSqlToExec);
			}

			if(bSqlExec1 &&bSqlExec2 && bSqlExec3 && bSqlExec4 && bSqlExec5 && bSqlExec6)
			{
				//printf("clw������ɹ���\n");
				WriteDailyLog(true, "clw������ɹ���\n");
			}
			//else
			//{
			//	printf("clw�����Ѵ��ڻ򽨱�ʧ�ܣ����飡\n");  TODO�����Ѵ��ڿ���ͨ��SQL�鿴�����Ǵ��κ���Ƚ�����
			//	WriteDailyLog(true, "clw�����Ѵ��ڻ򽨱�ʧ�ܣ����飡\n");
			//}	
		}
	} 
	else
		isConn = false;

	return isConn;
}

bool SysConfig::ReConnectDatabas()
{
	//(1)��ɾ������
	g_pDB_His1.close();

	//(2)����
	bool ConnectCondition =true;
	ConnectCondition = ConnectDatabas();
	return ConnectCondition;
}

void SysConfig::DisconnectDatabase()
{
	g_pDB_His1.close();
	// 	QString name;
	// 	if(g_pDB_His1.isOpen())
	// 	{
	// 		name = g_pDB_His1.databaseName();
	// 		QSqlDatabase::removeDatabase(name);
	// 	}
}


//�������ÿ�
bool SysConfig::ConnectModelDB()
{
	bool isConn = true;
	if (g_pDB_Model.isOpen())
	{
		isConn =true;
	}
	else 
	{
		if ( !g_pDB_Model.open() )
		{
			if(ReadModelDBCfg())
			{
				QString curData = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");//������ǰʱ��
				g_pDB_Model = QSqlDatabase::addDatabase(g_Model.dbdriver,"MODEL"+curData );
				//�������ÿ����ӵĻ�������
				g_pDB_Model.setUserName(g_Model.dbuser);
				g_pDB_Model.setPassword(g_Model.dbpassword);
				//if(strcmp(g_dbdriver, "QMYSQL3") == 0)  

				if(g_dbdriver.left(4) == "QOCI")
				{
					//if(g_His1.dbdriver.toStdString().substr(0,4) == "QOCI")
					g_pDB_Model.setDatabaseName(g_His1.strIP);
					g_pDB_Model.setPort(5236);
					//printf("clw���������ÿ⣬setDatabaseName(%s)\n", g_His1.strIP.toLatin1().data());
				}
				else if(g_dbdriver.left(6) == "QMYSQL")
				{
					g_pDB_Model.setDatabaseName(g_His1.dbname);
					g_pDB_Model.setHostName(g_His1.strIP);
					//printf("clw��mysql���ÿ⣬setDatabaseName(%s)\n", g_His1.dbname.toLatin1().data());
				}

				if ( !g_pDB_Model.open() )
				{
					printf("clw�����ÿ��ʧ�ܣ�\n");
					WriteDailyLog(true, "clw�����ÿ��ʧ�ܣ�\n");
					isConn = false ;
				}
				else
				{
#ifdef Q_OS_WIN
					g_pDB_His1.exec("SET NAMES 'GBK'");
#endif

#ifdef Q_OS_LINUX
					g_pDB_His1.exec("SET NAMES 'UTF8'");
#endif
				}
			} 
			else
				isConn = false;

		}	
	}

	return isConn;
}

//�Ͽ����ÿ�����
void SysConfig::DisconnectModelDB()
{
	g_pDB_Model.close();

	//  	QString name;
	//  	if(g_pDB_Model.isOpen())
	//  	{
	//  		name = g_pDB_Model.databaseName();
	//  		QSqlDatabase::removeDatabase(name);
	// 
	//  	}
}