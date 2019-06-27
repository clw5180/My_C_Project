#ifndef __ESFILEPARSER_20190531_H__
#define __ESFILEPARSER_20190531_H__

#include <string>
#include <vector>
#include <map>
#include <qthread.h>
#include <qstring.h>
//#include "savefile.h" // for CSaveFile  clw note 20190527��д��

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
	map<string, string> m_map_nameInfo; //�洢ini�����ļ�������Ϣ����filetype=RNMXFHYC;
	map<string, vector<string> > m_map_nameVals;  //�洢ini�����ļ���վidֵ���Ӧ��txt��¼��idֵ����station1=1��#3;
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
	
	//0 ��ȡ·��path�µ������ļ���������files
	bool GetFiles(const string& path, vector<string>& vecFiles);
	
	//1 ��esfilesetting.ini�ļ��ж���������Ϣ
	bool ReadInConfigPara();

	//2 ���������ļ���idֵ��Ӧ��һ�м�¼
	bool ParseESFile(const char filepath[]);

	//2.1 �����ļ�·����ȡ�ļ���
	string GetFileNameByPath(const string& filepath);

	//   �ж�QString�е��ַ����Ƿ�Ϊ0~9�Ĵ�����
	bool IsDigitStr(QString qStr);

	//   �ҵ�ini��#id��Ӧtxt�ļ��еļ�¼λ��
	bool GetTxtPositionByIni(int nRecordID, char *pTxtBuf, char **pBegin);

	//   �ж�����ָ���м��Ƿ��������
	bool IsNumber(char* start, char* end);

	//3 ���ļ���Ϣдmysql
	bool WriteFileInfoToDB();

	//3.1 ���������͵��ļ���Ϣд��DB
	bool WriteUltraLoadFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql); //�������ڸ���Ԥ���ļ�д��DB
	bool WriteShortLoadFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql);//�����ڸ���Ԥ���ļ�д��DB
	bool WriteUltraXNYFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql);//������������Դ����Ԥ���ļ�д��DB
	bool WriteShortXNYFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql);//����������Դ����Ԥ���ļ�д��DB
	bool WriteAGCRNFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql, const string& fileType);//������AGC�ƻ��ļ�д��DB
	bool WriteAGCRQFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql, const string& fileType);//����ǰAGC�ƻ��ļ�д��DB

	//3.1.1 ���ݲ�ͬ�ļ����͵ĵ���ͬ��48/96��ʱ��
	vector<string> GetForcastTimes(const string& fileType, int nTimeInterval, int nPointNums);

	//3.2 ִ��sql���
	bool ExecSQL(const QString& sqlToExec); 

	//3.2.2 дʵʱ��������ÿ��
	bool WriteRtdbAndConfigDB(const string& fileType, const string& strPlanID);

	//3.2.1 ���ݼƻ�ID��ѯ���ÿⳡվID
	int GetStationIDByPlanID(const string& strPlanID);

	//4 �ƶ�srcDirĿ¼�µ��ļ���dstDir
	bool MoveFileToDst(string srcDir, string dstDir, bool coverFileIfExist = true /*Ĭ��ͬ������*/);

	//5 ����ɾ���ļ�
	void DeleteFileByTime(); 


#ifdef _MSC_VER
	//������־�ʹ���
	void DealWithLogs(char logLines[128], bool bCout, int outFreq);
#endif 
#ifdef Q_OS_LINUX
	//4. ������־�ʹ���
	void DealWithLogs(string logLines, bool bCout, int outFreq);
#endif 


	bool m_bExit;  //�Ƿ����ֹͣ�߳�
	static FileParser* m_pInst;

	//���ļ�ɾ������/Сʱ
	float m_nFileDeleteCycle;

	//���ļ�����ʱ��/��
	float m_nFileKeepTime;


	vector<stFileInfo> m_vecFilesInfo; //ini�����ļ���������Ϣ�����籾��ĿĿǰ��6���ļ�����Ӧ6��stFileInfo�ṹ�����

	vector<string> m_vecFiles; //������������TXT���ļ�������
	vector<string> m_vecProcessedFiles;  //�ѽ���������TXT�ļ�������
	vector<float> m_vecShortP;  // 96����ڹ���ֵ
	vector<vector<float> > m_vecStationShortPs;  //����station��96����ڹ���ֵ����
	
	vector<int> m_vecFilesInfoIndex; //�ļ�����
	QString m_strForcastTime; //Ԥ��96�����ʼʱ�䡣����Ƕ���Ԥ�⣬��ʼ��ʱ��Ϊ����00:00:00��������Ԥ����ʼ��ʱ��Ϊ����ʱ�䡣

	IUpdateRealACfg* m_pUpdateRealACfg; //wx �ӿڶ�������дʵʱ���

	//�߳�ѭ��������Ϊ�˿��Ƶ�������
	int m_circleTimes;

	//����ʱ����ز���
	QString m_timeDQ;
	QString m_timeSecondDQ;

	//��ǰ������
	int m_startTime_minCount;
};
#endif /* __ESFILEPARSER_20190531_H__ */