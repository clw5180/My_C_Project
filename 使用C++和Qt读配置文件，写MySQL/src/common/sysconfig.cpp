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
clw modify 20190625：之前的写日志函数不好，
每次重启服务都会另开一个文件，这样积累起来数量太多；
还是改成用大彭的日志函数，设置一个日志名字xxx_log，
然后每次都往这个文件写，写到10M的时候，另开一个文件，
增加_和时间作为区分；
*/

#include "defs.h"
using namespace std;

#define PATH_SECTION_HISDB1  "plat_global/database/hisdb/hisdb1"  //历史库路径
#define PATH_SECTION_HISDB2  "plat_global/database/hisdb/hisdb2"
#define PATH_SECTION_MODELDB  "plat_global/database/modeldb"  //配置库路径

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
		sql.assign("select * from sys.sysdual"); //only for DM 达梦
	}
	else if(g_dbdriver.toStdString().substr(0, 6) == "QMYSQL")
	{	
		sql.assign( "show tables" );
	}
	else
	{
		;
		WriteDailyLog(true, "clw：数据库数据库驱动应为QOCI或QMYSQL类型，请检查icssetting.ini中的配置！");
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
		printf("clw：找不到../ini/icssetting.xml文件！\n");	
		WriteDailyLog(true, "clw：找不到../ini/icssetting.xml文件！");
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
			
			//clw modify 20190102：增加icssetting读取"smgserver"item功能，来区分测试态与正常运行态；
			if(pSettings->SetCurrentSectionPath("plat_scada/detaillog/"))
			{
				g_IsDebug = pSettings->ItemReadBool("scadasrv");//是否处于测试阶段  plat_scada/detaillog/smgserver
				if(g_IsDebug == true)
				{
					printf("clw：开启调试模式！\n");
					WriteDailyLog(true, "clw：开启调试模式！\n");
				}
			}

			char buf[BUF_LEN_MAX];
			if(true == pSettings->SetCurrentSectionPath(PATH_SECTION_MODELDB))//设置当前操作的节点路径，设置完毕后，以后操作都基于本节点进行
			{
				if(pSettings->ItemReadString("driver",buf, BUF_LEN_MAX))			//virtual bool ItemReadString(const char* item, char * sValueOut,int iSizeIn ) = 0;
				{															       //\brief 重载函数，以字符串的方法设置配置项的值 
					g_dbdriver = g_Model.dbdriver = buf;	                     //@param[in] item 配置项名称
	                                        							       //@param[in] val  要设置的值
				}
				//\return 设置成功返回 true 设置失败返回 false
				if(pSettings->ItemReadString("username", buf, BUF_LEN_MAX))
				{
					g_Model.dbuser = buf;
					//printf("clw：g_Model.dbuser = %s\n", buf);
					//string strLogInfo = "clw：g_Model.dbuser = " + string(buf);
					//WriteDailyLog(true, strLogInfo);
				}	
				else
				{
					printf("clw：读不到icssetting的username等信息！\n");	
					WriteDailyLog(true, "clw：读不到icssetting的username等信息！");
				}

				if(pSettings->ItemReadString("password", buf, BUF_LEN_MAX))	
				{
					g_Model.dbpassword = buf;
					//printf("clw：g_Model.dbpassword = %s\n", buf);
				}
				if(pSettings->ItemReadString("sername", buf, BUF_LEN_MAX))	
				{
					g_Model.dbname = buf;
					//printf("clw：g_Model.dbname = %s\n", buf);
				}
				if(pSettings->ItemReadString("hostname", buf, BUF_LEN_MAX))	
				{
					g_Model.strIP = buf;
					//printf("clw：g_Model.strIP = %s\n", buf);
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

//读信息
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
		printf("clw：找不到../ini/icssetting.xml文件！\n");	
		WriteDailyLog(true, "clw：找不到../ini/icssetting.xml文件！");
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
			if(true == pSettings->SetCurrentSectionPath(PATH_SECTION_HISDB1))//设置当前操作的节点路径，设置完毕后，以后操作都基于本节点进行
			{
				if(pSettings->ItemReadString("driver", buf, BUF_LEN_MAX))			//virtual bool ItemReadString(const char* item, char * sValueOut,int iSizeIn ) = 0;
				{															//\brief 重载函数，以字符串的方法设置配置项的值 
					g_dbdriver = buf;								    //@param[in] item 配置项名称
					g_His1.dbdriver = buf;								//@param[in] val  要设置的值
				}														//\return 设置成功返回 true 设置失败返回 false
				if(pSettings->ItemReadString("username", buf, BUF_LEN_MAX))
				{
					g_His1.dbuser = buf;
					//printf("clw：g_His1.dbuser = %s\n", buf);
					//string strLogInfo = "clw：g_Model.dbuser = " + string(buf);
					//WriteDailyLog(true, strLogInfo);
				}	
				else
				{
					printf("clw：读不到icssetting的username等信息！\n");
					WriteDailyLog(true, "clw：读不到icssetting的username等信息！");
				}
				if(pSettings->ItemReadString("password", buf, BUF_LEN_MAX))	
				{
					g_His1.dbpassword = buf;
					//printf("clw：g_His1.dbpassword = %s\n", buf);
				}
				if(pSettings->ItemReadString("sername", buf, BUF_LEN_MAX))	
				{
					g_His1.dbname = buf;
					//printf("clw：g_His1.dbname = %s\n", buf);
				}
				if(pSettings->ItemReadString("hostname", buf, BUF_LEN_MAX))	
				{
					g_His1.strIP = buf;
					//printf("clw：g_His1.strIP = %s\n", buf);
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
		QString curData = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");//读到当前时间
		g_pDB_His1 = QSqlDatabase::addDatabase(g_His1.dbdriver,"HIS1"+curData );
		//设置数据库联接的基本参数

		g_pDB_His1.setUserName(g_His1.dbuser);
		g_pDB_His1.setPassword(g_His1.dbpassword);
		//if(strcmp(g_dbdriver, "QMYSQL3") == 0)

		//printf("clw：g_dbdriver.left(4)=%s\n", g_dbdriver.left(4).toLatin1().data()) ;
		//printf("clw：g_dbdriver.left(4) == QOCI 是否为真%d\n",  g_dbdriver.left(4) == "QOCI");

		if(g_dbdriver.left(4) == "QOCI")  //clw add 20190603：for 达梦 Database；
		{
			//if(g_His1.dbdriver.toStdString().substr(0,4) == "QOCI")
			g_pDB_His1.setDatabaseName(g_His1.strIP);
			g_pDB_His1.setPort(5236);
			//printf("clw：达梦数据库，setDatabaseName(%s)\n", g_His1.strIP.toLatin1().data());
		}
		else if(g_dbdriver.left(6) == "QMYSQL")
		{
			g_pDB_His1.setDatabaseName(g_His1.dbname);
			g_pDB_His1.setHostName(g_His1.strIP);
			//printf("clw：mysql数据库，setDatabaseName(%s)\n", g_His1.dbname.toLatin1().data());
		}

		if ( !g_pDB_His1.open() )
		{
			printf("clw：历史库打开失败！\n");
			WriteDailyLog(true, "clw：历史库打开失败！\n");
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

			//clw modify 20190116：服务启动的时候建立
			//shortloadhisdata，ultraloadhisdata，
			QDateTime curTime = QDateTime::currentDateTime();  //获取当前时间
			QString curYear = curTime.toString("yyyy"); //当前时间转化为字符串类型，格式为：年月日时分秒，如2019-05-30 14:28:15
			
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
					PVSTATIONID			INT NOT NULL COMMENT '记录ID', \
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
					PVSTATIONID			INT NOT NULL COMMENT '记录ID', \
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
					ID                   INT NOT NULL COMMENT '记录ID', \
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
					CTRLVALUE               FLOAT COMMENT '充放电大小' , \
					CTRLTIME                  DATETIME, \
					DIFFEREASON             VARCHAR(64), \
					PRIMARY KEY (ID, PLANDATE, SAVETIME) \
					); ";
				bSqlExec5 = query.exec(qSqlToExec); 


				qSqlToExec = "CREATE TABLE STAGCTODAYPLAN_" + curYear + \
					"( \
					ID                   INT NOT NULL COMMENT '记录ID', \
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
					CTRLVALUE               FLOAT COMMENT '充放电大小' , \
					CTRLTIME                  DATETIME, \
					DIFFEREASON             VARCHAR(64), \
					PRIMARY KEY (ID, PLANDATE, SAVETIME) \
					);";
				bSqlExec6 = query.exec(qSqlToExec);
			}

			if(bSqlExec1 &&bSqlExec2 && bSqlExec3 && bSqlExec4 && bSqlExec5 && bSqlExec6)
			{
				//printf("clw：建表成功！\n");
				WriteDailyLog(true, "clw：建表成功！\n");
			}
			//else
			//{
			//	printf("clw：表已存在或建表失败，请检查！\n");  TODO：表已存在可以通过SQL查看，但是达梦好像比较特殊
			//	WriteDailyLog(true, "clw：表已存在或建表失败，请检查！\n");
			//}	
		}
	} 
	else
		isConn = false;

	return isConn;
}

bool SysConfig::ReConnectDatabas()
{
	//(1)先删除连接
	g_pDB_His1.close();

	//(2)连接
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


//连接配置库
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
				QString curData = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");//读到当前时间
				g_pDB_Model = QSqlDatabase::addDatabase(g_Model.dbdriver,"MODEL"+curData );
				//设置配置库联接的基本参数
				g_pDB_Model.setUserName(g_Model.dbuser);
				g_pDB_Model.setPassword(g_Model.dbpassword);
				//if(strcmp(g_dbdriver, "QMYSQL3") == 0)  

				if(g_dbdriver.left(4) == "QOCI")
				{
					//if(g_His1.dbdriver.toStdString().substr(0,4) == "QOCI")
					g_pDB_Model.setDatabaseName(g_His1.strIP);
					g_pDB_Model.setPort(5236);
					//printf("clw：达梦配置库，setDatabaseName(%s)\n", g_His1.strIP.toLatin1().data());
				}
				else if(g_dbdriver.left(6) == "QMYSQL")
				{
					g_pDB_Model.setDatabaseName(g_His1.dbname);
					g_pDB_Model.setHostName(g_His1.strIP);
					//printf("clw：mysql配置库，setDatabaseName(%s)\n", g_His1.dbname.toLatin1().data());
				}

				if ( !g_pDB_Model.open() )
				{
					printf("clw：配置库打开失败！\n");
					WriteDailyLog(true, "clw：配置库打开失败！\n");
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

//断开配置库连接
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