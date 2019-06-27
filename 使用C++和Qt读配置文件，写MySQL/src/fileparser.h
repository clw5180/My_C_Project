#ifndef __ESFILEPARSER_20190531_H__
#define __ESFILEPARSER_20190531_H__

#include <string>
#include <vector>
#include <map>
#include <qthread.h>
#include <qstring.h>
//#include "savefile.h" // for CSaveFile  clw note 20190527：写表

#include <algorithm>
#ifdef _MSC_VER
#include <atlstr.h>		// for checkFile
#include <windows.h>		// for checkFile
#include <Shlwapi.h>	// for checkFile
#pragma comment(lib,"shlwapi.lib")// for checkFile 
#endif 

#include <qdir.h> // for GetFiles() 
#include <QDirIterator> // for GetFiles() 
#include <sys/stat.h> // for GetFiles() 
#include <sys/types.h> // for GetFiles() 

#include "./common/cdatetime.h"   // for CDateTime()
#include "../include/other/updaterealacfg.h" 
#include "../interface/iupdaterealcfgdt.h" //wx, for write rtdb

using namespace std;

struct stFileInfo
{
	map<string, string> m_map_nameInfo; //存储ini配置文件基本信息，如filetype=RNMXFHYC;
	map<string, vector<string> > m_map_nameVals;  //存储ini配置文件厂站id值与对应的txt记录的id值，如station1=1，#3;
};

class FileParser : public QThread
{
public:
	static FileParser* Instance();
	~FileParser();
	void Stop() { m_bExit = true; }


protected:
	virtual void run();


private:
	FileParser();
	
	//0 获取路径path下的所有文件名，存入files
	bool GetFiles(const string& path, vector<string>& vecFiles);
	
	//1 从esfilesetting.ini文件中读入配置信息
	bool ReadInConfigPara();

	//2 解析储能文件，id值对应的一行记录
	bool ParseESFile(const char filepath[]);

	//2.1 根据文件路径获取文件名
	string GetFileNameByPath(const string& filepath);

	//   判断QString中的字符串是否为0~9的纯数字
	bool IsDigitStr(QString qStr);

	//   找到ini中#id对应txt文件中的记录位置
	bool GetTxtPositionByIni(int nRecordID, char *pTxtBuf, char **pBegin);

	//   判断两个指针中间是否夹着数字
	bool IsNumber(char* start, char* end);

	//3 将文件信息写mysql
	bool WriteFileInfoToDB();

	//3.1 将各种类型的文件信息写入DB
	bool WriteUltraLoadFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql); //将超短期负荷预测文件写入DB
	bool WriteShortLoadFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql);//将短期负荷预测文件写入DB
	bool WriteUltraXNYFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql);//将超短期新能源发电预测文件写入DB
	bool WriteShortXNYFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql);//将短期新能源发电预测文件写入DB
	bool WriteAGCRNFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql, const string& fileType);//将日内AGC计划文件写入DB
	bool WriteAGCRQFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql, const string& fileType);//将日前AGC计划文件写入DB

	//3.1.1 根据不同文件类型的到不同的48/96点时间
	vector<string> GetForcastTimes(const string& fileType, int nTimeInterval, int nPointNums);

	//3.2 执行sql语句
	bool ExecSQL(const QString& sqlToExec); 

	//3.2.2 写实时库表与配置库表
	bool WriteRtdbAndConfigDB(const string& fileType, const string& strPlanID);

	//3.2.1 根据计划ID查询配置库场站ID
	int GetStationIDByPlanID(const string& strPlanID);

	//4 移动srcDir目录下的文件到dstDir
	bool MoveFileToDst(string srcDir, string dstDir, bool coverFileIfExist = true /*默认同名覆盖*/);

	//5 定期删除文件
	void DeleteFileByTime(); 


#ifdef _MSC_VER
	//处理日志和打屏
	void DealWithLogs(char logLines[128], bool bCout, int outFreq);
#endif 
#ifdef Q_OS_LINUX
	//4. 处理日志和打屏
	void DealWithLogs(string logLines, bool bCout, int outFreq);
#endif 


	bool m_bExit;  //是否可以停止线程
	static FileParser* m_pInst;

	//旧文件删除周期/小时
	float m_nFileDeleteCycle;

	//旧文件保留时长/天
	float m_nFileKeepTime;


	vector<stFileInfo> m_vecFilesInfo; //ini所有文件的配置信息（比如本项目目前有6个文件，对应6个stFileInfo结构体对象）

	vector<string> m_vecFiles; //待解析的所有TXT的文件名集合
	vector<string> m_vecProcessedFiles;  //已解析的所有TXT文件名集合
	vector<float> m_vecShortP;  // 96点短期功率值
	vector<vector<float> > m_vecStationShortPs;  //各个station的96点短期功率值集合
	
	vector<int> m_vecFilesInfoIndex; //文件索引
	QString m_strForcastTime; //预测96点的起始时间。如果是短期预测，起始点时间为次日00:00:00；超短期预测起始点时间为当日时间。

	IUpdateRealACfg* m_pUpdateRealACfg; //wx 接口对象，用于写实时库表

	//线程循环次数，为了控制弹屏数量
	int m_circleTimes;

	//上送时间相关参数
	QString m_timeDQ;
	QString m_timeSecondDQ;

	//当前分钟数
	int m_startTime_minCount;
};
#endif /* __ESFILEPARSER_20190531_H__ */