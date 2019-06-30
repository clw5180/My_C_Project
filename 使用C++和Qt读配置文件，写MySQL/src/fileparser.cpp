#include <qdatetime.h>  //for QDateTime
#include <QSqlQuery>
#include <QTextStream> //for QTextStream
#include <QSqlError>  //for query.lasterror()
#include <QVariant>

#include "fileparser.h"

#include "./common/staticvariable.h"
//#include "./common/writelog.h" // for WriteLogToFile
#include "../include/objectbase/writedailylog.h"
#include "./common/sysconfig.h"  
#include "./common/defs.h"

extern bool IsMainRole();                     // 是否为主角色

FileParser* FileParser::m_pInst = 0;

FileParser::FileParser(): m_bExit(false)
{
	m_pUpdateRealACfg = (new CUpdateDt())->Self(); //得到接口对象的同时，已经把动态库加载进来了；
	m_nFileDeleteCycle = 1;
	m_nFileKeepTime = 10;
}

FileParser::~FileParser()
{
	m_bExit = true;
	if (m_pInst)
	{
		delete m_pInst;
		m_pInst = NULL;
	}
	while(running()) 
		msleep(50);
}

FileParser* FileParser::Instance()
{
	if(NULL == m_pInst) 
	{
		m_pInst = new FileParser();
	}
	return m_pInst;
}

void FileParser::run()
{
	//线程启动初始 静止50ms
	msleep(1000);

	//1、连接历史库
	SysConfig dbsys;
	if(dbsys.ConnectDatabas())	
	{
		printf("clw：esfileserver服务历史库连接成功！\n");
		WriteDailyLog(true, "clw：历史库连接成功！\n");
	}
	else 
	{
		printf("clw：esfileserver服务历史库连接失败！\n");
		WriteDailyLog(true, "clw：历史库连接失败！\n");
	}

	//2、连接配置库
	if(dbsys.ConnectModelDB())	
	{
		printf("clw：esfileserver服务配置库连接成功！\n");
		WriteDailyLog(true, "clw：配置库连接成功！\n");
	}
	else 
	{
		printf("clw：esfileserver服务配置库连接失败！\n");
		WriteDailyLog(true, "clw：配置库连接失败！\n");
	}

	//3、从esfilesetting.ini文件中读入配置信息
	if(!ReadInConfigPara())
	{
		printf("clw：esfileserver服务读取配置信息失败！\n");
		WriteDailyLog(true, "clw：esfileserver服务读取配置信息失败！\n");
		return;
	}

	int count = 0;
	while(!m_bExit)
	{
		//3、解析文件内容
		//写短期负荷预测表或超短期负荷预测表
		//超短期负荷预测，未来4小时  
		//短期负荷预测日前已经写好

		//注意：（1）解析之后不能立即删除，需要备份一段时间，因此需要标识某个文件已经被解析过了，比如修改前缀，或者copy到另一个地方
		//       （2）应提取某一前缀（如BJ_）的文件进行解析
		//       （3）考虑1个线程or2个线程
		//       （4）设定1个时间间隔，比如一分钟找一下文件，然后解析一下

		if(m_vecFiles.empty())
		{
			GetFiles(FILE_PATH, m_vecFiles);
		}
		int len = m_vecFiles.size();
		for(int i=0; i<len; ++i)
		{
			//主备状态判断，主状态才解析，备状态移动到其他地方
#ifndef WINDOWS_DEBUG
			if(IsMainRole())
			{
				if(g_IsDebug)
				{
					printf("clw：主备状态：主机\n");
				}
#endif
				if(ParseESFile(m_vecFiles[i].c_str()))
				{
					//如果能够正确解析文件，再开始写数据库表
					if(WriteFileInfoToDB())
					{
						if(g_IsDebug)
						{
							printf("clw：写历史数据库成功！准备移动文件\n");
							WriteDailyLog(true, "clw：写历史数据库成功！准备移动文件\n");
						}
						//如果写数据库成功，再将文件移动到某一文件夹下，相当于标识该文件“已被处理”，确保不会同一文件反复解析
						MoveFileToDst(m_vecFiles[i], FILE_PATH + string("/") + FILE_PROCESSED_FOLDER);
						
					}
					else
					{
						printf("clw：写历史数据库失败！\n", m_vecFiles[i].c_str());
						printf("\n");
						WriteDailyLog(true, "clw：写历史数据库失败！");
					}
				}
				else
				{
					printf("ParseESFile()解析文件%s失败，继续解析其他文件\n", m_vecFiles[i].c_str());
					printf("\n");
					string strLogInfo = "clw：ParseESFile()解析文件" +  m_vecFiles[i] + "失败，继续解析其他文件！";
					WriteDailyLog(true, strLogInfo.c_str());
				}
#ifndef WINDOWS_DEBUG
			}
			else //主备状态：备机
			{
				if(g_IsDebug)
				{
					WriteDailyLog(true, "clw：主备状态：备机\n");
					printf("clw：主备状态：备机\n");
				}
				//无需解析和处理文件，直接移动到“已处理”文件夹下，相当于标识该文件“已被处理”，确保不会同一文件反复解析
				MoveFileToDst(m_vecFiles[i], FILE_PATH + string("/") + FILE_PROCESSED_FOLDER);
			}
#endif
			//每次不管能否正确解析文件，之后要清掉96点功率值vector<>和n个96点功率值的vector<vector<>>，还要文件索引vec
			vector<float>().swap(m_vecShortP);
			vector<vector<float> >().swap(m_vecStationShortPs);
			vector<int>().swap(m_vecFilesInfoIndex);
		} 

		//最后清掉包含所有文件的vector<>，因为这些文件已经被处理过；	
		vector<string>().swap(m_vecFiles);  //或者m_vecFiles.swap(vector<string>());	

		//定期清理文件
		count++;
		if(count >= m_nFileDeleteCycle * 120)  //因为线程30s循环一次，因此如果是1小时看一下是否存在旧文件，那么就应该是count == 120，对应60min
		{
			DeleteFileByTime();
			count = 0;
		}

		//等待30s，如果有新的文件，再读进来，解析；
		msleep(THREAD_PERIOD_MS);
	}
}

bool FileParser::GetFiles(const string& path, vector<string>& vecFiles)  
{  	
	int pathLen = path.length();
	if(pathLen >= 256)
	{
		printf("clw：文件名%s过长！\n", path.c_str());
		string strLogInfo = "clw：文件名" + path + "过长！\n";
		WriteDailyLog(true, strLogInfo.c_str());
		return false;
	}

	bool bSuc = false;
	QString tempdirPath;
	char charp[256] = {0}; //clw note：数组的列表初始化，和char charp[256] = "";等价

	int i;
	for(i = 0; i < pathLen; ++i)
	{
		//if ( path[i] == 0x0d )	//clw note：ASCII值=13对应是回车，但是文件名中好像没有回车？感觉这两句可以去掉
		//	break;	
		charp[i] = path[i];
	}
	charp[i] = '\0';
	tempdirPath.sprintf("%s", charp);
	QDir dir;
	if (!dir.exists(tempdirPath)) //把已处理过的TXT统一放到dstDir内
	{
		if(!dir.mkpath(tempdirPath)) //如果文件夹不存在，则新建文件夹
		{
			//QStringList qStrList = QString::fromStdString(filepath).split("/");
			//if(!dir.mkdir(qStrList[qStrList.length() - 1].toStdString()))
			//{
				printf("clw：创建路径%s失败，请检查！\n", tempdirPath.toLatin1().data());
				WriteDailyLog(true, "clw：创建路径失败，请检查！\n");
				return false;
			//}
		}
	}

	/*if(!dir.exists(tempdirPath))
	{	
		printf("clw：路径%s不存在！\n", path.c_str());
		string strLogInfo = "clw：路径" +  path + "不存在！\n";
		WriteDailyLog(true, strLogInfo.c_str());
		return false;
	}*/

	QStringList filtersWai;
	//定义迭代器并设置过滤器
	QDirIterator dir_iteratorWai(tempdirPath,
		filtersWai,
		QDir::Files | QDir::NoSymLinks 
		|QDir::NoDotAndDotDot /*将"."和".."目录忽略, 否则就会当成子目录处理，造成了死循环. */
		/*,QDirIterator::Subdirectories*/);

	while(dir_iteratorWai.hasNext())
	{
		dir_iteratorWai.next();
		QFileInfo file_info = dir_iteratorWai.fileInfo();
		QString absolute_file_path = file_info.absoluteFilePath();
		vecFiles.push_back(absolute_file_path.toStdString());
		bSuc = true;
	}
	sort(vecFiles.begin(), vecFiles.end());//排序
	return bSuc;
} 


bool FileParser::ReadInConfigPara()
{
	bool bSuc = false;
	QFile f(PARA_INI_PATH);
	if(!f.open(QIODevice::ReadOnly))
	{
		printf("clw：未找到esfilesetting.ini配置文件！\n");
		WriteDailyLog(true, "clw：未找到esfilesetting.ini配置文件！\n");
		return bSuc;
	}

	QTextStream t(&f);
	QString str;
#ifdef Q_OS_WIN
	QTextCodec *codec=QTextCodec::codecForName("GBK"); //TODO：是否要改为win用GBK，linux用UTF8
#endif

#ifdef Q_OS_LINUX
	QTextCodec *codec=QTextCodec::codecForName("UTF-8");
#endif	

	t.setCodec(codec);

	QString left = "";
	QString right = "";
	QString strID = "";
	QString strRecordID = "";
	stFileInfo iniFileInfo;

	while(!t.atEnd())
	{
		//依次得到ini文件的每一行
		str = codec->fromUnicode(t.readLine().stripWhiteSpace());
		//printf("clw：ini文件中解析的行的内容为%s\n", str.toLatin1().data());		

		//如果是空行
		if(str.isEmpty())
		{
			//如果strFileInfo有内容
			if(!iniFileInfo.m_map_nameInfo.empty() && !iniFileInfo.m_map_nameVals.empty())
			{
				//则将stFileInfo对象存入m_vecFilesInfo；
				m_vecFilesInfo.push_back(iniFileInfo);
				//清空strFileInfo内的map
				iniFileInfo.m_map_nameInfo.clear();
				iniFileInfo.m_map_nameVals.clear();
			}	
			continue; //继续搜索下一行
		}

		//====================================================================================
		//如果有等号，说明有配置项，准备读取
		//====================================================================================
		string::size_type nEqualSignPos = str.find('='); 
		if(nEqualSignPos != string::npos) 
		{
			left = str.left(nEqualSignPos).stripWhiteSpace();  //比如内容为filetype，或者station1之类
			right = str.mid(nEqualSignPos + 1).stripWhiteSpace(); //pos位置在等号，取等号右边的数
			right.replace(";",""); 
			//提取right部分，比如内容为"1,#3"，第一个是ID，第三个是RecordID，按顺序写入一个vector
			//unsigned int nChnCommaPos = str.find(L'，'); //clw note：这里由于是中文逗号，所以一定要是双引号，即find字符串；如果是用单引号''应是宽字符wchar_t，需加L前缀；
			string::size_type nChnCommaPos = str.find("，"); //clw note：如果是unsigned int，这里结果在linux和windows都是429496725，但是linux下string::npos是size_type类型，相当于long long，所以不能写成unsigned int
			string::size_type nEngCommaPos = str.find(',');

			//如果找不到逗号，说明是一般配置项，如filetype=RNMXFHYC，存入m_map_nameInfo
			if(nChnCommaPos == string::npos  && nEngCommaPos == string::npos)  //clw note：
			{
				if(left == "filedeletecycle")
				{
					m_nFileDeleteCycle = right.toFloat();
					continue;
				}
				if(left == "filekeeptime")
				{
					m_nFileKeepTime = right.toFloat();
					continue;
				}

				if(iniFileInfo.m_map_nameInfo.end() != iniFileInfo.m_map_nameInfo.find(left.toStdString()))//重复的元素
				{
					//printf("clw：覆盖ini的一般配置项, left = %s, right = %s\n", left.toLatin1().data(), right.toLatin1().data());
					iniFileInfo.m_map_nameInfo[left.toStdString()] = right.toStdString();
				} 
				else// 非重复元素，即新元素或m_map_nameInfo.empty()第一个元素
				{
					//printf("clw：插入ini的一般配置项, left = %s, right = %s\n", left.toLatin1().data(), right.toLatin1().data());
					iniFileInfo.m_map_nameInfo.insert(pair<string,string>(left.toStdString(),right.toStdString()));
				}	
				bSuc = true;
			}
			//如果能找到逗号，说明是数值配置项，如station1=1,10,#3;
			else if(nChnCommaPos != string::npos && nEngCommaPos == string::npos ||
				nChnCommaPos == string::npos && nEngCommaPos != string::npos)
			{
				//printf("clw：能找到逗号，说明是数值配置项，准备读取\n");
				//同时支持中文和英文逗号
				wchar_t chComma;
				int nComma1Pos;
				if(nChnCommaPos != string::npos)
				{
					chComma = '，';
					nComma1Pos = nChnCommaPos;
				}
				else                              
				{
					chComma = ',';
					nComma1Pos = nEngCommaPos;
				}

				//clw note：str.right是从右边界开始往前面取x个字节，而不是从某个位置x到结尾，不要乱用。。
				strID = str.mid(nEqualSignPos + 1, nComma1Pos - nEqualSignPos - 1).stripWhiteSpace(); 
				strRecordID = str.mid(nComma1Pos + 1).stripWhiteSpace();
				strRecordID.replace("#","");   //操作前内容形如#3;
				strRecordID.replace(";","");

				//存储ini配置文件厂站id值与对应的txt记录的id值，如station1=1，#3;
				vector<string> tmpVec;
				tmpVec.push_back(strID.toStdString());
				tmpVec.push_back(strRecordID.toStdString());
				//printf("clw：在读取ini配置文件时, strRecordID = %s\n", strRecordID.toLatin1().data());
				if(iniFileInfo.m_map_nameVals.end() != iniFileInfo.m_map_nameVals.find(left.toStdString()))//重复的元素
				{
					//printf("clw：覆盖ini的数值配置项, left = %s, right = %s\n", left.toLatin1().data(), (strID + strRecordID).toLatin1().data());
					iniFileInfo.m_map_nameVals[left.toStdString()] = tmpVec;
				} 
				else //非重复元素，即新元素或m_map_nameVal.empty()第一个元素
				{
					//printf("clw：插入ini的数值配置项, left = %s, right = %s\n", left.toLatin1().data(), (strID + strRecordID).toLatin1().data());
					iniFileInfo.m_map_nameVals.insert(pair<string, vector<string> >(left.toStdString(), tmpVec));
				}
				bSuc = true;
			}
			else
				bSuc = false;
			//====================================================================================
		}
	}

	//如果strFileInfo还有内容，因为一般最后是没有空行的，而此时最后一个文件的配置信息尚未保存
	if(!iniFileInfo.m_map_nameInfo.empty() || !iniFileInfo.m_map_nameVals.empty())
	{
		//则将stFileInfo对象存入m_vecFilesInfo；
		m_vecFilesInfo.push_back(iniFileInfo);
	}	

	if(m_vecFilesInfo.empty())
	{
		printf("clw：读取esfilesetting.ini文件失败，未获取到正确的信息！\n");
		WriteDailyLog(true, "clw：读取esfilesetting.ini文件失败，未获取到正确的信息！\n");
		bSuc = false;
	}

	f.close();
	return bSuc;
}


//根据文件路径获取文件名
string FileParser::GetFileNameByPath(const string& filepath)
{
	QStringList qStrList = QString::fromStdString(filepath).split("/");
	return qStrList[qStrList.length() - 1].toStdString(); //文件名，如xxx.TXT
}


bool FileParser::ParseESFile(const char filepath[])
{
	string fileName = GetFileNameByPath(filepath);
	if(g_IsDebug)
	{
		printf("clw：文件名：%s\n", fileName.c_str());
		WriteDailyLog(true, "clw：文件名：%s\n", fileName.c_str());
	}
	
	int nVecFilesLen = m_vecFilesInfo.size();
	//首先判断文件类型

	int i;
	string fileType;
	for(i=0; i < nVecFilesLen; ++i)
	{
		fileType = m_vecFilesInfo[i].m_map_nameInfo["filetype"];
		if(fileName.find(fileType) != string::npos) //找到文件类型
		{
			//保存预测文件的时间m_strForcastTime，写表的时候要用到；
			//如超短期预测文件的20190603_1540代表2019.06.03 15:40:00
			//短期预测文件的20190603代表2019.06.03，后期转成00:00:00
			string fileFormat = m_vecFilesInfo[i].m_map_nameInfo["fileformat"];
			string::size_type nTimeHourPos = fileFormat.find("hhmm");
			string::size_type nTimeYearPos = fileFormat.find("yyyy");
			if(nTimeHourPos != string::npos) //如果能找到小时和分钟信息，说明是日内/超短期预测
			{
				m_strForcastTime = QString::fromStdString(fileName.substr(nTimeYearPos, 8)) + QString::fromStdString(fileName.substr(nTimeHourPos, 4));
				
				//m_strForcastTime = QString::fromStdString(fileName.substr(nTimeLeftPos, 13));
				//m_strForcastTime.remove("_");
			}
			else if(nTimeYearPos != string::npos) //只找到年，说明是日前/短期预测
			{
				m_strForcastTime = QString::fromStdString(fileName.substr(nTimeYearPos, 8));
			}
			if(!IsDigitStr(m_strForcastTime)) //如果提取到的时间里有非纯数字的内容，比如0190101_或者abcdefgh之类
			{
				printf("clw：提取到的预测时间为%s，请检查esfilesetting.ini中配置的fileformat与文件名是否一致！", m_strForcastTime.toLatin1().data());
				WriteDailyLog(true, "clw：提取到的预测时间为%s，请检查esfilesetting.ini中配置的fileformat与文件名是否一致！", m_strForcastTime.toLatin1().data());
				return false;
			}
			break;
		}
	}
	if(i == nVecFilesLen)
	{
		//无此文件类型
		printf("clw：esfilesetting.ini中无此类型的文件！文件名为：%s\n", fileName.c_str());
		WriteDailyLog(true, "clw：esfilesetting.ini中无此类型的文件！文件名为：%s\n", fileName.c_str());
		return false;
	}

	//记录索引号，因为比如RNMXFHYC类型的一个文件同时对应超短期负荷预测及超短期发电预测两个表
	//另外判断预测时间格式是否正确
	for(int i = 0; i < nVecFilesLen; ++i)
	{
		//判断ini文件是否含有该TXT文件类型，如果有，才能开始解析文件；
		//之前从ini获取了所有文件的配置信息m_vecFilesInfo
		//printf("clw：m_vecFilesInfo[i].m_map_nameInfo['filetype'] = %s\n", m_vecFilesInfo[i].m_map_nameInfo["filetype"].c_str());
		if(m_vecFilesInfo[i].m_map_nameInfo["filetype"] == fileType)
		{
			m_vecFilesInfoIndex.push_back(i); //记录该文件对应的文件结构体索引号，方便以后使用
		}	

		if(fileType == FILETYPE_ULTRALOAD || fileType == FILETYPE_ULTRAXNY || fileType == FILETYPE_AGCRN) //如201906031415
		{
			if(m_strForcastTime.size() != 12)
			{
				printf("clw：请检查esfilesetting.ini中配置的fileformat格式是否正确！fileType = %s\n", fileType.c_str());
				WriteDailyLog(true, "clw：请检查esfilesetting.ini中配置的fileformat格式是否正确！fileType = %s\n", fileType.c_str());
				return false;
			}
		}

		if(fileType == FILETYPE_SHORTLOAD || fileType == FILETYPE_SHORTLOAD1 || fileType == FILETYPE_SHORTXNY || fileType == FILETYPE_AGCRQ) //如20190603
		{
			if(m_strForcastTime.size() != 8)
			{
				printf("clw：请检查esfilesetting.ini中配置的fileformat格式是否正确！\n");
				WriteDailyLog(true, "clw：请检查esfilesetting.ini中配置的fileformat格式是否正确！\n");
				return false;
			}
		}
	}

	//打开文件
	FILE* fp = fopen(filepath, "r");
	if(fp == NULL)
	{
		printf("clw：打开文件%s失败！\n", filepath);
		WriteDailyLog(true, "clw：打开文件%s失败！\n", filepath);
		return false;
	}

	/*
	* int fseek(FILE *stream, long offset, int fromwhere);函数设置文件指针stream的位置。
	* 如果执行成功，stream将指向以fromwhere为基准，偏移offset(指针偏移量)个字节的位置，函数返回0。
	* 如果执行失败(比如offset取值大于等于2*1024*1024*1024，即long的正数范围2G)，则不改变stream指向的位置，函数返回一个非0值。
	* fseek函数和lseek函数类似，但lseek返回的是一个off_t数值，而fseek返回的是一个整型。
	*/
	fseek(fp, 0, SEEK_END); //让文件指针指向文件尾
	int len = ftell(fp);
	fseek(fp, 0, SEEK_SET); //把文件指针还原到文件开头


	//要保证以'\0'结尾，所以额外分配了一个字节，即len+1；读数据的时候，只读len个字节。
	char *pTxtBuf = (char*)malloc((len + 1) * sizeof(char));  //故意对malloc分配的内存进行越界访问竟然没报错（c语言）c/c++语言的危险就在这。你只要不去动操作系统保留的内存，程序就不会死。
	//有些时候内存越界还会影响程序流程，比如for循环控制变量为i，在for循环内部进行内存申请，如果控制不好就有可能无意的修改i的值，导致循环次数有错，甚至导致这个for循环死成循环。
	memset(pTxtBuf, 0, len + 1); 
	fread(pTxtBuf, sizeof(char), len, fp); //fread是一个函数，它从文件流中读数据，最多读取count个项，每个项size个字节，如果调用成功返回实际读取到的项个数（小于或等于count），如果不成功或读到文件末尾返回 0。


	//遍历m_map_nameVals
	//int len = m_vecFilesInfo[nFilesInfoIndex].m_map_nameVals.size();
	int nVecFilelen = m_vecFilesInfoIndex.size();
	for(int i = 0; i < nVecFilelen; ++i)
	{
		for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
			iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
		{
			char strRecordID[BUF_LEN_MIN] = {0};

			strncpy(strRecordID, iter->second[1].c_str(), BUF_LEN_MIN - 1);//如#3，在这里只保留3

			int nRecordID = atoi(strRecordID); 
			if(nRecordID > INT_MAX)
			{
				//error1
				printf("clw：strRecordID值超INT_MAX上限！\n");
				WriteDailyLog(true, "clw：strRecordID值超INT_MAX上限！\n");
				free(pTxtBuf);
				fclose(fp);
				return false;
			}

			char* pBegin = NULL;
			//定位到txt中#id的位置（末尾），写入pBegin；
			if(!GetTxtPositionByIni(nRecordID, pTxtBuf, &pBegin))
			{
				printf("clw：从txt中获取记录失败！\n");
				WriteDailyLog(true, "clw：从txt中获取记录失败！\n");
				free(pTxtBuf);
				fclose(fp);
				return false;
			}

			//将该行信息写入内存
			char *pEnd = strchr(pBegin, '\n');
			char *pRecordBuf = (char *)malloc((pEnd - pBegin + 2) * sizeof(char));
			memset(pRecordBuf, 0, pEnd - pBegin + 2); //clw add  clw note：20190604：这句务必要写，否则后面都是脏数据！！而且要保证以'\0'结尾，所以额外分配了一个字节，即pEnd- pBegin +1这个值加1
			strncpy(pRecordBuf, pBegin, pEnd - pBegin + 1);  //clw note：pRecordBuf[pEnd - pBegin]对应'\n'
			if(g_IsDebug)
			{
				//WriteDailyLog(true, "clw：打印TXT文件每一行的内容%s\n", pRecordBuf);//自注：这里有可能因为放不下，引起报错
				printf("clw：打印TXT文件每一行的内容%s\n", pRecordBuf);
			}
			pEnd = strchr(pRecordBuf, '\n');

			//提取该行负荷有功预测值，96点
			//两个辅助指针变量挖字符串
			char *pCur = pRecordBuf;
			char *pPre = pCur;

			//==========================old method==========================
			//int nPointNums;
			//if(fileType == "AGCRN") //如果是AGC日内计划，取48点
			//{
			//	nPointNums = 48;
			//}
			//else
			//{
			//	nPointNums = 96;
			//}
			//===============================================================
			int nPointNums = atoi(m_vecFilesInfo[m_vecFilesInfoIndex[0]].m_map_nameInfo["pointnums"].c_str());  //clw modify 20190620


			//开始取48点或96点数据
			int count = 0;
			while(1) 
			{
				//clw add 20190604:读到96点数据或48点数据就返回，因为有些行96点最后并不一定是\n，还有可能是乱码，容易误读。比如
				/*北京.石景山电厂/220kV.0甲#启备变-高等值负荷	0.3935  0	0.3935  0	0.3935  0	0.3935  0	
				0.3871  0	0.3808  0	0.3808  0	0.4217  0	0.4465  0	0.4636  0	0.4399  0	0.4001  0 .....
				0	q*/
				if(m_vecShortP.size() == nPointNums)
					break;

				//注意：应该是把空格，Tab都strchr找一遍，哪个排在前面，就先处理哪个
				//但是还有2个空格的情况，还需要考虑
				//跳过第一个空白
				char* pSpace = strchr(pCur, ' ');
				char* pTab = strchr(pCur, '\t');

				if(pSpace > pEnd || pTab > pEnd)  //说明已经提取完毕，跳出
					break; 

				if(pSpace == NULL && pTab == NULL)  //已经找不到空白，则跳出
				{
					if(count == nPointNums - 1) //最后一个值可能没有空格，因此要特殊处理，找'\n'  在出现95点或47点的时候
					{
						pCur = strchr(pCur, '\n');
					}
					else
					{
						break;
					}
				}
				else if((pSpace == NULL && pTab != NULL) || (pSpace != NULL && pTab == NULL)) //如果一个非空一个为空，取非空的那个
					pCur = max(pSpace, pTab);
				else //都不为空，取最小
					pCur = min(pSpace, pTab);


				//如果pPre和pCur中间夹着的是数字，就把pPre和pCur中间的数字取出来，count+1，然后继续往下找
				//下面不要用isdigit(*(pPre + 1))，因为如果是中文，有可能a=-79，这样用isdigit会宕掉
				//if((*(pPre + 1) >= DIGIT_ASCII_MIN && *(pPre + 1) <= DIGIT_ASCII_MAX || *(pPre + 1) == '-')  ||
				//	 (*(pCur - 1) >= DIGIT_ASCII_MIN && *(pCur - 1) <= DIGIT_ASCII_MAX) )  //TODO：暂时把首尾指针前后都是数字的就当做数值了，还不完备。。
				
				if(IsNumber(pPre + 1, pCur - 1))
				{
					//TODO：目前来看，只有日前AGC计划的表包含无功数据，需要跳过，只取有功96点
					if(strcmp(fileType.c_str(), FILETYPE_AGCRQ) == 0) 
					{
						if(count % 2 == 0) //只处理count=偶数，即有功负荷数据，跳过无功负荷预测数据
						{
							char buf[16] = {0};
							strncpy(buf, pPre + 1, pCur - pPre - 1); //clw note：注意这里的+1和-1
							float P = atof(buf);
							//printf("clw：atof转化出来P = %f\n", P);  //小数点后保留6位
							m_vecShortP.push_back(P);	
						}
					}
					else
					{
						char buf[16] = {0};
						//strncpy(buf, pPre, pCur - pPre);
						strncpy(buf, pPre + 1, pCur - pPre - 1);
						float P = atof(buf);
						m_vecShortP.push_back(P);	//
					}

					pPre = pCur;
					pCur++; //这样才能找到下一个空格，否则死循环在当前空格。。
					count++;
					continue;
				}
				pPre = pCur;
				pCur++; //这样才能找到下一个空格，否则死循环在当前空格。。
			} //while(1) end


			//clw note：读取48/96点结束，判断vec是否读到了96个值或48个值
			if(m_vecShortP.size() != nPointNums)
			{
				printf("clw：提取TXT文件是%d点，不是%d点，请检查数据！\n", m_vecShortP.size(), nPointNums);
				WriteDailyLog(true, "clw：提取TXT文件是%d点，不是%d点，请检查数据！\n", m_vecShortP.size(), nPointNums);
				int len = m_vecShortP.size();
				for(int i=0; i<len; ++i)
				{
					printf("%lf ", m_vecShortP[i]);
					WriteDailyLog(true, "%lf ", m_vecShortP[i]);
				}
				printf("\n");
				char buf[BUF_LEN_MIN];
				sprintf(buf, "%d", m_vecShortP.size());
				string strLogInfo = "clw：提取TXT文件是" + string(buf) + "点，不是96点，请检查数据！";
				WriteDailyLog(true, strLogInfo.c_str());
				free(pRecordBuf); 
				return false;
			}

			m_vecStationShortPs.push_back(m_vecShortP);
			vector<float>().swap(m_vecShortP);

			//释放为每条记录分配的内存
			if(pRecordBuf!=NULL)
			{
				free(pRecordBuf);
				pRecordBuf = NULL;
			}
		} //for end
	}

	//释放整个文件对应的str的内存
	if(pTxtBuf != NULL)
	{
		free(pTxtBuf);
		pTxtBuf = NULL;
	}

	//关闭文件
	fclose(fp);

	if(g_IsDebug)
	{
		printf("clw：有功预测数据TXT文件提取完毕，准备写mysql表！！\n");
		WriteDailyLog(true, "clw：有功预测数据TXT文件提取完毕，准备写mysql表！！\n");
	}
	return true;
}

bool FileParser::IsDigitStr(QString qStr)
{
	QByteArray ba = qStr.toLatin1();
	const char *cStr = ba.data();
	while(*cStr)
	{
		if(*cStr < DIGIT_ASCII_MIN || *cStr > DIGIT_ASCII_MAX)
			return false;
		else
			cStr++;
	}
	return true;
}

bool FileParser::GetTxtPositionByIni(int nRecordID, char *pTxtBuf, char **pBegin)
{
	//==================================================================================
	//主要功能：找到nRecordId对应的行
	//不同TXT文件格式不同，有1个空格或2个空格或Tab。。但是都在第3行的位置有一个模板，
	//以@开头，记录了文件格式，如@ id name  p0015 q0015 p0030 q0030 p0045 q0045 p0100 
	//这里硬编码来处理固定某种格式的TXT文件
	//空白主暂时主要包括有1个空格或2个空格或Tab，貌似没有全角空格的概念；
	//只有全角逗号，和中文字类似，占两个字符，要用wchar_t
	//也想过通过统计#号的数量来确定是第几条记录，但是不行，比如酒仙桥/220kV.4#主变-高。。
	//或者也可以统计#，然后排除#前面不是换行符的。。。
	//==================================================================================

	//方法一：找到@
	//略

	//方法二：#数量统计法（只统计位于第1列的#数量)
	//char* pWellSign = pTxtBuf;  //定位在文章开始
	//printf("clw: nRecordID = %d\n", nRecordID);
	//for(int i = 0; i < nRecordID; ++i)
	//{
	//	pWellSign = strstr(pWellSign + 1, "#");
	//	//printf("clw：*(pWellSign+1) = %d\n", *(pWellSign+1));
	//	//printf("clw：*(pWellSign-1) = %d\n", *(pWellSign-1));
	//	if(pWellSign == NULL) //比如要找的是#3，也就是第3条记录，但是实际txt只有1条记录，#号没那么多
	//	{
	//		printf("clw：ini文件中配置的记录id大于TXT记录总数，请修改ini文件！\n");
	//		printf("\n");
	//		WriteDailyLog(true, "clw：ini文件中配置的记录id大于TXT记录总数，请修改ini文件！\n");
	//		free(pTxtBuf);
	//		fclose(fp);
	//		return false;
	//	}

	//	if(*(pWellSign-1) != '\n') //位于第1列的#的特征是，前一个字符是'\n'，即上一行
	//	{
	//		i--;
	//		continue;
	//	}
	//}
	//判断#后面出现的第1个数字是否是nRecordID，如果不是，则找不到id
	//char* pBegin = pWellSign;
	//pBegin++;
	//while(/*!isdigit(*pBegin)*/ *pBegin < DIGIT_ASCII_MIN || *pBegin > DIGIT_ASCII_MAX) //非数字
	//{
	//	if(*pBegin != ' ' && *pBegin != '\t')
	//	{
	//		//error2
	//		//printf("clw：*pBegin = %d\n", *pBegin);
	//		printf("clw：TXT的#id格式不正确，请检查格式！\n");
	//		printf("\n");
	//		//printf("clw：*pBegin = %d\n", *pBegin);
	//		//printf("clw：*(pBegin+1) = %d\n", *(pBegin+1));
	//		//printf("clw：*(pBegin-1) = %d\n", *(pBegin - 1));
	//		WriteDailyLog(true, "clw：TXT的#id格式不正确，请检查格式！\n");
	//		free(pTxtBuf);
	//		fclose(fp);
	//		return false;
	//	}
	//	pBegin++;
	//}


	//方法三：直接找#3或# 3或#  3
	char* pWellSign = pTxtBuf;  //定位在文章开始
	if(g_IsDebug)
	{
		printf("clw: nRecordID = %d\n", nRecordID);
		WriteDailyLog(true, "clw: nRecordID = %d\n", nRecordID);
	}
	char buf[BUF_LEN_MIN] = {0};
	sprintf(buf, "#%d", nRecordID);
	while(1)
	{
		if(strstr(pWellSign, buf) == NULL)
		{
			sprintf(buf, "# %d", nRecordID);
			if(strstr(pWellSign, buf) == NULL)
			{
				sprintf(buf, "#  %d", nRecordID);
				if(strstr(pWellSign, buf) == NULL)
				{
					printf("clw：txt文件中没有#id对应的记录，请检查相应的txt和ini文件！\n");
					WriteDailyLog(true, "clw：txt文件中没有#id对应的记录，请检查相应的txt和ini文件！\n");
					return false;
				}
				else
					pWellSign = strstr(pWellSign, buf);
			}
			else
				pWellSign = strstr(pWellSign, buf);
		}
		else
			pWellSign = strstr(pWellSign, buf);


		if(*(pWellSign-1) != '\n') //位于第1列的#的特征是，前一个字符是'\n'，即上一行
		{
			pWellSign++;
		}
		else
		{
			break;
		}
	}

	//获取txt中的nRecordID
	char* pTmp = pWellSign;
	pTmp++;
	while(/*!isdigit(*pBegin)*/ *pTmp < DIGIT_ASCII_MIN || *pTmp > DIGIT_ASCII_MAX) //非数字
	{
		if(*pTmp != ' ' && *pTmp != '\t')
		{
			//error2
			//printf("clw：*pBegin = %d\n", *pBegin);
			printf("clw：TXT的#id格式不正确，#后面只能是空格或Tab跟数字！\n");
			//printf("clw：*pBegin = %d\n", *pBegin);
			//printf("clw：*(pBegin+1) = %d\n", *(pBegin+1));
			//printf("clw：*(pBegin-1) = %d\n", *(pBegin - 1));
			WriteDailyLog(true, "clw：TXT的#id格式不正确，#后面只能是空格或Tab跟数字！\n");
			return false;
		}
		pTmp++;
	}
	//计算出记录的id值，比如#123，那么num = ((0*10+1)*10+2)*10+3
	int num = 0;
	while(*pTmp >= DIGIT_ASCII_MIN && *pTmp <= DIGIT_ASCII_MAX) //是数字
	{
		num = num * 10 + *pTmp - '0';
		pTmp++;
	}
	if(num != nRecordID) //判断ini中的记录id和txt中找到的记录id是否相同
	{
		//error3
		printf("clw：txt中的记录id值为num = %d，与ini中nRecordID = %d不等！\n", num, nRecordID);
		WriteDailyLog(true, "clw：txt中的记录id值为num = %d，与ini中nRecordID = %d不等！\n", num, nRecordID);
		return false;
	}
	*pBegin = pTmp;
	return true;

	//==================================================================================
}

//TODO：暂时还没判断数字合理性，比如00.0，或者011之类
bool FileParser::IsNumber(char* start, char* end) 
{
	if(start == NULL || end == NULL || start > end)
		return false;

	while(start <= end)
	{
		if((*start < DIGIT_ASCII_MIN || *start > DIGIT_ASCII_MAX) && (*start != '-') && (*start != '.'))
			return false;
		start++;
	}
	return true;
}

bool FileParser::WriteFileInfoToDB()
{
	bool bSuc = true;

	if(m_strForcastTime.isEmpty())
	{
		printf("clw：超短期文件起始预测时间获取失败，请检查文件名下划线格式！\n");
		WriteDailyLog(true, "clw：超短期文件起始预测时间获取失败，请检查文件名下划线格式！");
		return false;
	}
	if(m_vecStationShortPs.empty())
	{
		printf("clw：未获取到96点数据，无法写数据库！\n");
		WriteDailyLog(true, "clw：未获取到96点数据，无法写数据库！\n");
		return false;
	}

	//==============================================================================================
	//批量插入数据到mysql/达梦
	//==============================================================================================
	//QString strSqlHeader = "";
	string fileType = "";
	int nTimeInterval = 0;
	int nPointNums = 0;
	if(m_vecFilesInfoIndex.size() > 0)
	{
		fileType = m_vecFilesInfo[m_vecFilesInfoIndex[0]].m_map_nameInfo["filetype"];
		nTimeInterval = atoi(m_vecFilesInfo[m_vecFilesInfoIndex[0]].m_map_nameInfo["timeinterval"].c_str());
		nPointNums = atoi(m_vecFilesInfo[m_vecFilesInfoIndex[0]].m_map_nameInfo["pointnums"].c_str());
	}
	else
	{
		WriteDailyLog(true, "clw：找不到该文件类型对应的索引！");//比如之前说RNMXFHYC有可能既代表超短期负荷预测，也代表超短期发电预测，这时候就有两个索引...
		return false;
	}

	//根据不同文件类型，生成各自的sql语句
	char strSQL[65536]; //TODO：注意小了存不下！最少得有：96点 * 60每条长度 * 4比如plan或station1、2、3、4四条记录
	QStringList lstSql;
	vector<string> vecForcastTime = GetForcastTimes(fileType, nTimeInterval, nPointNums);
	if(fileType == FILETYPE_ULTRALOAD)
	{
		bSuc = WriteUltraLoadFileToDB(nPointNums, vecForcastTime, strSQL, lstSql);
	}
	else if(fileType == FILETYPE_SHORTLOAD || fileType == FILETYPE_SHORTLOAD1)
	{
		bSuc = WriteShortLoadFileToDB(nPointNums, vecForcastTime, strSQL, lstSql);
	} 
	else if(fileType == FILETYPE_ULTRAXNY)
	{
		bSuc = WriteUltraXNYFileToDB(nPointNums, vecForcastTime, strSQL, lstSql);
	} 
	else if(fileType == FILETYPE_SHORTXNY)
	{
		bSuc = WriteShortXNYFileToDB(nPointNums, vecForcastTime, strSQL, lstSql);
	} 
	else if(fileType == FILETYPE_AGCRN)
	{
		bSuc = WriteAGCRNFileToDB(nPointNums, vecForcastTime, strSQL, lstSql, fileType);
	}
	else if(fileType == FILETYPE_AGCRQ)
	{
		bSuc = WriteAGCRQFileToDB(nPointNums, vecForcastTime, strSQL, lstSql, fileType);
	}
	else
	{
		printf("clw：没有此文件类型！写MySQL失败！\n");
		WriteDailyLog(true, "clw：没有此文件类型！写MySQL失败！\n");
		return false;
	}
	//==============================================================================================

	return bSuc;
}


bool FileParser::WriteUltraLoadFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql)
{
	int len = m_vecFilesInfoIndex.size();
	if(g_IsDebug)
	{
		printf("clw：文件类型为日内母线负荷预测RNMXFHYC，该类型对应的table个数 = %d\n", len);
		WriteDailyLog(true, "clw：文件类型为日内母线负荷预测RNMXFHYC，该类型对应的table个数 = %d\n", len);
	}
	if(len <= 0)
	{
		WriteDailyLog(true, "clw：找不到配置文件信息，请检查！");
		return false;
	}

	int count = 0;
	int index = 0;
	QString sqlToExec;
	char* strFormat = "(%d, '%s', '%s', %d, %f)";
	for(int i=0; i<len; ++i)
	{
		string tableName = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameInfo["tablename"];

		sqlToExec = "INSERT INTO " + QString::fromStdString(tableName) + "_" + QDateTime::currentDateTime().toString("yyyy") + "(ID, FORCASTTIME, SAVETIME, METHOD, FORCASTP) VALUES";

		for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin();  //如果ini中同一个表配了多条记录，如1,#1和2,#2
			iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
		{

			string strID = iter->second[0];
			int nCtrlID = atoi(strID.c_str());
			for(int i = 0; i < nPointNums /*96，1测试用*/; ++i)
			{
				sprintf(strSQL, strFormat, nCtrlID , vecForcastTime[i].c_str(), QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLatin1().data(), 0/*method，暂时写0*/, m_vecStationShortPs[index][i]);
				lstSql.append(strSQL);
			}

			if(count != 0)
				sqlToExec += ",";  //第一波数据前面不用加逗号
			count++;
			index++;
			sqlToExec += lstSql.join(",");
			lstSql.clear();
		}
		//执行sql
		if(!ExecSQL(sqlToExec))
			return false;
		count = 0;

	}
	return true;
}


bool FileParser::WriteShortLoadFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql)
{
	int len = m_vecFilesInfoIndex.size();
	if(g_IsDebug)
	{
		printf("clw：文件类型为日前母线负荷预测RQMXFHYC，该类型对应的table个数 = %d\n", len);
		WriteDailyLog(true, "clw：文件类型为日前母线负荷预测RQMXFHYC，该类型对应的table个数 = %d\n", len);
	}
	if(len <= 0)
	{
		WriteDailyLog(true, "clw：找不到配置文件信息，请检查！");
		return false;
	}

	int count = 0;
	int index = 0;
	QString sqlToExec;
	char *strFormat = "(%d, '%s', '%s', %d, %f)";
	for(int i=0; i<len; ++i)
	{
		string tableName = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameInfo["tablename"];

		sqlToExec = "INSERT INTO " + QString::fromStdString(tableName) + "_" + QDateTime::currentDateTime().toString("yyyy") + "(ID, FORCASTTIME, SAVETIME, METHOD, FORCASTP) VALUES";	

		for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
			iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
		{

			string strID = iter->second[0];
			int nID = atoi(strID.c_str());

			for(int i = 0; i < nPointNums /*96，1测试用*/; ++i)
			{
				sprintf(strSQL, strFormat, nID , vecForcastTime[i].c_str(), QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLatin1().data(), 0/*method，暂时写0*/, m_vecStationShortPs[index][i]);
				lstSql.append(strSQL);
			}

			if(count != 0)
				sqlToExec += ",";  //第一波数据前面不用加逗号
			count++;
			index++;
			sqlToExec += lstSql.join(",");
			lstSql.clear();
		}
		//执行sql
		if(!ExecSQL(sqlToExec))
			return false;
		count = 0;

	} //for end
	return true;
}


bool FileParser::WriteUltraXNYFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql)
{
	int len = m_vecFilesInfoIndex.size();
	if(g_IsDebug)
	{
		printf("clw：文件类型为日内风电功率预测RNFDGLYC，该类型对应的table个数 = %d\n", len);
		WriteDailyLog(true, "clw：文件类型为日内风电功率预测RNFDGLYC，该类型对应的table个数 = %d\n", len);
	}
	if(len <= 0)
	{
		WriteDailyLog(true, "clw：找不到配置文件信息，请检查！");
		return false;
	}

	int count = 0;
	int index = 0;
	QString sqlToExec;
	char* strFormat = "(%d, '%s', '%s', %d, %f)";
	for(int i=0; i<len; ++i)
	{
		string tableName = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameInfo["tablename"];

		sqlToExec = "INSERT INTO " + QString::fromStdString(tableName) + "_" + QDateTime::currentDateTime().toString("yyyy") + "(PVSTATIONID, FORCASTTIME, SAVETIME, METHOD, FORCASTP) VALUES";
		for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
			iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
		{
			string strID = iter->second[0];
			int nID = atoi(strID.c_str());
			for(int i = 0; i < nPointNums; ++i)
			{
				sprintf(strSQL, strFormat, nID, vecForcastTime[i].c_str(), QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLatin1().data(), 0, m_vecStationShortPs[index][i]);
				lstSql.append(strSQL);
			}

			if(count != 0)
				sqlToExec += ",";  //第一波数据前面不用加逗号
			count++;
			index++;
			sqlToExec += lstSql.join(",");
			lstSql.clear();
		}
		//执行sql
		if(!ExecSQL(sqlToExec))
			return false;
		count = 0;

	}	//for end
	return true;
}

bool FileParser::WriteShortXNYFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql)
{
	int len = m_vecFilesInfoIndex.size();
	if(g_IsDebug)
	{
		printf("clw：文件类型为日前风电功率预测RQFDGLYC，该类型对应的table个数 = %d\n", len);
		WriteDailyLog(true, "clw：文件类型为日前风电功率预测RQFDGLYC，该类型对应的table个数 = %d\n", len);
	}
	int count = 0;
	int index = 0;
	QString sqlToExec;
	char *strFormat = "(%d, '%s', '%s', %d, %f)";
	for(int i=0; i<len; ++i)
	{
		string tableName = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameInfo["tablename"];

		sqlToExec = "INSERT INTO " + QString::fromStdString(tableName) + "_" + QDateTime::currentDateTime().toString("yyyy") + "(PVSTATIONID, FORCASTTIME, SAVETIME, METHOD, FORCASTP) VALUES";

		for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
			iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
		{
			string strID = iter->second[0];
			int nID = atoi(strID.c_str());
			for(int i = 0; i < nPointNums; ++i)
			{
				sprintf(strSQL, strFormat, nID, vecForcastTime[i].c_str(), QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLatin1().data(), 0, m_vecStationShortPs[index][i]);
				lstSql.append(strSQL);
			}

			if(count != 0)
				sqlToExec += ",";  //第一波数据前面不用加逗号
			count++;
			index++;
			sqlToExec += lstSql.join(",");
			lstSql.clear();
		}
		//执行sql
		if(!ExecSQL(sqlToExec))
			return false;
		count = 0;

	}
	return true;
}


bool FileParser::WriteAGCRNFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql, const string& fileType)
{
	int len = m_vecFilesInfoIndex.size();
	if(g_IsDebug)
	{
		printf("clw：文件类型为AGC日内调度计划RNPLANYC，该类型对应的table个数 = %d\n", len);
		WriteDailyLog(true, "clw：文件类型为AGC日内调度计划RNPLANYC，该类型对应的table个数 = %d\n", len);
	}
	int count = 0;
	int index = 0;
	QString sqlToExec;
	char *strFormat = "(%d, '%s', '%s', %d, %f, %f, %f)";
	for(int i=0; i<len; ++i)
	{
		string tableName = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameInfo["tablename"];
		sqlToExec = "INSERT INTO " + QString::fromStdString(tableName) + "_" + QDateTime::currentDateTime().toString("yyyy") + "(ID, PLANDATE, SAVETIME, PLANVALUESQ, AUTOPLANAIM, PLANREVISEDAIM, OPRTREVISEDAIM) VALUES";
		bool bSuc2 = true;

		for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
			iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
		{

			string strID = iter->second[0];
			int nID = atoi(strID.c_str());
			for(int i = 0; i < nPointNums; ++i)
			{
				sprintf(strSQL, strFormat, nID, vecForcastTime[i].c_str(), QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLatin1().data(), i, m_vecStationShortPs[index][i], m_vecStationShortPs[index][i], m_vecStationShortPs[index][i]);
				lstSql.append(strSQL);
			}

			if(count != 0)
				sqlToExec += ",";  //第一波数据前面不用加逗号
			count++;
			index++;
			sqlToExec += lstSql.join(",");
			lstSql.clear();

			//写实时库
#ifndef WINDOWS_DEBUG
			bSuc2 = (bSuc2 && WriteRtdbAndConfigDB(fileType, strID)); //clw note：只测试mysql时，写实时库用true代替	
#else
			bSuc2 = true;
#endif
		
		}
		//执行sql
		bool bSuc1 = ExecSQL(sqlToExec);
		count = 0;

		if(!(bSuc1 && bSuc2))
			return false;
	}
	return true;
}


bool FileParser::WriteAGCRQFileToDB(int nPointNums, const vector<string>& vecForcastTime, char* strSQL, QStringList& lstSql, const string& fileType)
{
	
	int len = m_vecFilesInfoIndex.size();
	if(g_IsDebug)
	{
		printf("clw：文件类型为AGC日前调度计划DQPLANYC，该类型对应的table个数 = %d\n", len);
		WriteDailyLog(true, "clw：文件类型为AGC日前调度计划DQPLANYC，该类型对应的table个数 = %d\n", len);
	}
	int count = 0;
	int index = 0;
	QString sqlToExec;
	
	for(int i=0; i<len; ++i)
	{
		string tableName = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameInfo["tablename"];
		sqlToExec = "INSERT INTO " + QString::fromStdString(tableName) + "_" + QDateTime::currentDateTime().toString("yyyy") + "(ID, PLANDATE, SAVETIME, PLANVALUESQ, AUTOPLANAIM, PLANREVISEDAIM, OPRTREVISEDAIM) VALUES";

		bool bSuc2 = true;

		for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
			iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
		{
			char *strFormat = "(%d, '%s', '%s', %d, %f, %f, %f)";
			string strID = iter->second[0]; 
			int nID = atoi(strID.c_str());
			for(int i = 0; i < nPointNums; ++i)
			{
				sprintf(strSQL, strFormat, nID, vecForcastTime[i].c_str(), QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLatin1().data(), i, m_vecStationShortPs[index][i], m_vecStationShortPs[index][i], m_vecStationShortPs[index][i]);
				lstSql.append(strSQL);
			}

			if(count != 0)
				sqlToExec += ",";  //第一波数据前面不用加逗号
			count++;
			index++;
			sqlToExec += lstSql.join(",");
			lstSql.clear();

			//写实时库\	
			//printf("clw：fileType = %s\n", fileType.c_str());
			//printf("clw：strID = %s\n", strID.c_str());
#ifndef WINDOWS_DEBUG	
			bSuc2 = (bSuc2 && WriteRtdbAndConfigDB(fileType, strID));  //clw note：只测试mysql时，写实时库用true代替	
#else
			bSuc2 = true;
#endif
		}
		//执行sql
		bool bSuc1 = ExecSQL(sqlToExec);
		count = 0;

		if(!(bSuc1 && bSuc2))
			return false;
	}
	return true;
}

vector<string> FileParser::GetForcastTimes(const string& fileType, int nTimeInterval, int nPointNums)
{
	//比如m_strForcastTime时间是201905231415，说明文件类型是FILETYPE_ULTRALOAD，即超短期负荷预测,则96点的第一条记录预测时间应为2019-05-23 14:15:00
	//    m_strForcastTime时间是20190524，说明文件类型是FILETYPE_SHORTLOAD，即短期负荷预测，则96点的第一条记录预测时间应为2019-06-03 00:00:00

	//14:15:00是通过下面对m_strForcastTime的内容读取然后处理操作得到的
	vector<string> vecForcastTime; //clw note：变长，还是用string了，之后96点处理再用c_str()转成char*
	QDateTime forcastTime;
	QString firstPointTime; //准备存入计划曲线第一个点的时间，如2019-05-30 00:00:00

	//短期预测或日前计划，组织时间格式为当天时间+00:00:00
	if(fileType == FILETYPE_SHORTLOAD || fileType == FILETYPE_SHORTLOAD1 || fileType == FILETYPE_SHORTXNY || fileType == FILETYPE_AGCRQ)  
	{
		//forcastTime = curTime.addDays(1);//当前时间+1天，之后会用到这个年月日
		m_strForcastTime.append(" 00:00:15"); //注意，日前一般是从00:15开始到24:00，共96点；
		m_strForcastTime.insert(6, '-');
		m_strForcastTime.insert(4, '-');
		forcastTime = QDateTime::fromString(m_strForcastTime, "yyyy-MM-dd hh:mm:ss");

	}
	//超短期预测或日内计划，组织时间格式为当天时间+  hh:mm:00，mm一般是15，30，45或00，即15分钟一个点；
	else if(fileType == FILETYPE_ULTRALOAD || fileType == FILETYPE_ULTRAXNY || fileType == FILETYPE_AGCRN)
	{
		m_strForcastTime.append(":00");
		m_strForcastTime.insert(10, ':');
		m_strForcastTime.insert(8, ' ');
		m_strForcastTime.insert(6, '-');
		m_strForcastTime.insert(4, '-');
		forcastTime = QDateTime::fromString(m_strForcastTime, "yyyy-MM-dd hh:mm:ss");
	}

	for(int i = 0; i < nPointNums; ++i) //nPointNums一般为48或96，将这么多点的时间值写入vec
	{
		QDateTime tempTime(forcastTime.addSecs(60 * nTimeInterval * i)); //nTimeInterval一般为5或15，即每隔5min或15min为一个时间点；
		firstPointTime = tempTime.toString("yyyy-MM-dd hh:mm:ss");
		vecForcastTime.push_back(firstPointTime.toStdString());
	}
	return vecForcastTime;
}


bool FileParser::ExecSQL(const QString& sqlToExec)
{
	//==============================================================================================
	//clw note：执行sql语句
	//TODO：如果执行失败，判断是否数据库断开，尝试重连
	//并且如果连不上，应该先把sql以二进制格式如.DAT，存在磁盘中
	//==============================================================================================
	QSqlQuery query = QSqlQuery(g_pDB_His1);
	if(!query.exec(sqlToExec))
	{
		if(g_IsDebug)
		{
			printf("执行mysql失败: %s \n", sqlToExec.toLatin1().data());
			string logInfo = "Fails to execute sql: " + sqlToExec.toStdString() + "\n" ;
			char *pSQL = logInfo.c_str();
			WriteDailyLog(true, "clw：执行mysql:%s失败！", pSQL);
			//WriteDailyLog(true, "clw：执行mysql失败");

			QString qText = query.lastError().text();
			QByteArray ba = qText.toLocal8Bit();
			const char* s = ba.data();
			printf("[%s: %d] - %s\n", g_dbdriver.toLatin1().data(), query.lastError().number(), s); //clw note：之前为query.lastError().text().toLocal8Bit().data()

			QString qLogInfo = "[" + g_dbdriver + ": " +  QString::number(query.lastError().number()) +  "] - " + query.lastError().text();
			WriteDailyLog(true, qLogInfo.toLatin1().data());
		}

	
		//clw note：
		//printf("clw：执行sql失败的错误类型为%d\n", query.lastError().type()); //clw note：数据库连不上的时候返回的居然是2，与ErrorType不符，无法用于判断数据库连通性
		//另外QSqlQuery的isOpen()很坑，无法判断数据库连通性，考虑自己写IsHisDBConnected()方法；
		
		//如果发现数据库没连上，尝试重连
		if(!g_sysModel.IsHisDBConnected())
		{
			printf("clw：尝试重连历史库！\n");
			WriteDailyLog(true, "clw：尝试重连历史库！\n");

			if(g_sysModel.ReConnectDatabas())
			{
				printf("clw：重连成功，尝试再次执行Sql！\n");
				WriteDailyLog(true, "clw：重连成功，尝试再次执行Sql！\n");
				if(!query.exec(sqlToExec))
				{
					if(g_IsDebug)
					{
						printf("执行mysql失败: %s \n", sqlToExec.toLatin1().data());
						string logInfo = "Fails to execute sql: " + sqlToExec.toStdString() + "\n" ;
						WriteDailyLog(true, logInfo.c_str());

						printf("[%s: %d] - %s\n", g_dbdriver.toLatin1().data(), query.lastError().number(), 
							query.lastError().text().toLocal8Bit().data());
						QString qLogInfo = "[" + g_dbdriver + ": " +  QString::number(query.lastError().number()) +  "] - " + query.lastError().text();
						WriteDailyLog(true, qLogInfo.toLatin1().data());
					}
				}
				else
				{
					printf("clw：重连后执行Sql成功！\n");
					WriteDailyLog(true, "clw：重连后执行Sql成功！\n");
					return true;
				}
			}
			else
			{
				printf("clw：重连失败！\n");
				WriteDailyLog(true, "clw：重连失败！\n");
			}
		}
		return false;
	}
	else
	{
		if(g_IsDebug)
		{
			printf("clw：执行mysql成功！\n");
			WriteDailyLog(true, "执行mysql成功！\n");
		}
	}
	return true;
	//==============================================================================================
}

bool FileParser::WriteRtdbAndConfigDB(const string& fileType, const string& strPlanID)
{
	//==============================================================================================
	//写实时库表
	//调用wx接口，写实时库stationdispc表（场站调度控制信息）的
	//AGC日前计划最近接收时间 和 AGC滚动（日内）计划最近接收时间
	//==============================================================================================
	int nStationID = GetStationIDByPlanID(strPlanID);
	printf("clw：nStation = %d\n", nStationID);
	if(nStationID < 0)
	{
		//printf("clw：查询配置库表stagcplan失败！\n");
		WriteDailyLog(true, "clw：查询配置库表stagcplan失败！\n");
		return false;
	}

	icsvariant fldValue;
	fldValue.type = ft_time;
	fldValue.value.t = (double)CDateTime::GetCurrentDateTime();
	fldValue.isNull = false;
	if(fileType == FILETYPE_AGCRN)
	{
		//printf("clw：准备写入实时库及配置库stationdispc表：\n");
		//printf("clw：nStationID = %d", nStationID);
		WriteDailyLog(true, "clw：准备写入实时库及配置库stationdispc表：\n");
		if(m_pUpdateRealACfg->UpdateDataInCfg("stationdispc", "todayagctime", nStationID, fldValue, false, true))
		{
			//printf("clw：写入实时库及配置库stationdispc表，todayagctime字段成功！\n");
			WriteDailyLog(true, "clw：写入实时库及配置库stationdispc表，todayagctime字段成功！\n");
		}
		else
		{
			printf("clw：写入实时库及配置库stationdispc表，todayagctime字段失败！\n");
			WriteDailyLog(true, "clw：写入实时库及配置库stationdispc表，todayagctime字段失败！\n");
			return false;
		}
	}
	else if(fileType == FILETYPE_AGCRQ)
	{
		//printf("clw：准备写入实时库及配置库stationdispc表：\n");
		//printf("clw：nStationID = %d", nStationID);
		WriteDailyLog(true, "clw：准备写入实时库及配置库stationdispc表：\n");
		if(m_pUpdateRealACfg->UpdateDataInCfg("stationdispc", "nextdayagctime", nStationID, fldValue, false, true))
		{
			//printf("clw：写入实时库及配置库stationdispc表，nextdayagctime字段成功！\n");
			WriteDailyLog(true, "clw：写入实时库及配置库stationdispc表，nextdayagctime字段成功！\n");
		}
		else
		{
			printf("clw：写入实时库及配置库stationdispc表，nextdayagctime字段失败！\n");
			WriteDailyLog(true, "clw：写入实时库及配置库stationdispc表，nextdayagctime字段失败！\n");
			return false;
		}
	}
	return true;
	//==============================================================================================
}

int FileParser::GetStationIDByPlanID(const string& strPlanID)
{
	/* 测试用建表语句
	SET FOREIGN_KEY_CHECKS=0;
	-- ----------------------------
	-- Table structure for strgcrvplan
	-- ----------------------------
	CREATE TABLE `stagcplan` (
	`ID` int(11) NOT NULL,
	`stationid` int(11) NOT NULL,
	PRIMARY KEY (`ID`)
	) ENGINE=InnoDB DEFAULT CHARSET=gbk COMMENT='AGC调度计划基本信息';
	*/

	QString sql = QString::fromStdString("SELECT STATIONID FROM stagcplan where ID = " + strPlanID);
	QSqlQuery query = QSqlQuery(g_pDB_Model);
	if(!query.exec(sql))
	{
		return -1;
	}
	query.next();
	return query.value(0).toInt();
}

bool FileParser::MoveFileToDst(string _srcDir, string _dstDir, bool coverFileIfExist)
{
	if(g_IsDebug)
	{
		printf("clw：开始移动文件%s\n", _srcDir.c_str());
		printf("\n");
		printf("\n");
		WriteDailyLog(true, "clw：开始移动文件%s\n", _srcDir.c_str());
	}

	QString srcDir = QString::fromStdString(_srcDir);
	QString dstDir = QString::fromStdString(_dstDir);
	//srcDir.replace("\\","/");
	//dstDir.replace("\\","/");
	QStringList qStrList = srcDir.split("/");
	QString fileName = qStrList[qStrList.length() - 1]; //文件名，如xxx.TXT
	if (srcDir == dstDir)
	{
		return true;
	}
	if (!QFile::exists(srcDir))
	{
		return false;
	}

	QDir dir;
	if (!dir.exists(dstDir)) //把已处理过的TXT统一放到dstDir内
	{
		if(!dir.mkdir(dstDir)) //如果文件夹不存在，则新建文件夹
		{
			//printf("clw：创建路径%s失败，请检查！\n", dstDir.toLatin1().data());
			WriteDailyLog(true, "clw：创建路径失败，请检查！\n");
			return false;
		}
	}

	//是否覆盖
	if(coverFileIfExist)
	{
		if(dir.remove(dstDir + "/" + fileName))
		{
			if(g_IsDebug)
			{
				QString strLogInfo = dstDir + "/" + fileName;
				printf("clw：存在旧文件%s并且成功删除，准备写入新文件\n", strLogInfo.toLatin1().data());
				WriteDailyLog(true, "clw：存在旧文件%s并且成功删除，准备写入新文件\n", strLogInfo.toLatin1().data());
			}
		}
		//如果没有同名的旧文件，则删除返回false，不影响
	}

	//复制到新的目录
	if(!QFile::copy(srcDir, dstDir + "/" + fileName))
	{
		return false;
	}

	//删除旧文件
	if(!QFile::remove(srcDir))
	{
		return false;
	}

	return true;
}

void FileParser::DeleteFileByTime()
{
	//定期删除n天前的文件，在ini中可配
	QString filePath = "../file/processed";
	QDir dir;

	vector<string> vecToDeleteFiles;
	GetFiles(filePath.toStdString(), vecToDeleteFiles);
	int len = vecToDeleteFiles.size();
	for(int i=0; i<len; ++i)
	{
		string::size_type fileNameLen = vecToDeleteFiles[i].size();
		string::size_type nLeftUnderLinePos = vecToDeleteFiles[i].find('_'); 
		//printf("clw：下划线后面字符为%c\n", vecToDeleteFiles[i][nLeftUnderLinePos+1]);
		while(nLeftUnderLinePos < fileNameLen && 
			(vecToDeleteFiles[i][nLeftUnderLinePos+1] < DIGIT_ASCII_MIN || vecToDeleteFiles[i][nLeftUnderLinePos+1] > DIGIT_ASCII_MAX) /*下划线后面如果不是数字，就继续找下一个下划线*/)
		{
			nLeftUnderLinePos = vecToDeleteFiles[i].find('_', nLeftUnderLinePos+1); 
		}

		string::size_type nMiddleUnderLinePos = vecToDeleteFiles[i].find('_', nLeftUnderLinePos + 1); 
		QString strFileTime;
		if(nMiddleUnderLinePos - nLeftUnderLinePos - 1 >= 8)
			strFileTime = QString::fromStdString(vecToDeleteFiles[i].substr(nLeftUnderLinePos + 1, nMiddleUnderLinePos - nLeftUnderLinePos - 1).substr(0, 8)); //把yyyyMMddhhmm转化为yyyyMMdd
		else
		{
			WriteDailyLog(true, "clw：删除文件时发现文件名时间格式不正确，请检查格式为yyyyMMdd或yyyyMMddhhmm！\n");
			printf("clw：删除文件时发现文件名时间格式不正确，请检查格式为yyyyMMdd或yyyyMMddhhmm！\n");
			continue;
		}
		long long nFileTime = strFileTime.toLongLong();
		QDateTime curTime = QDateTime::currentDateTime();  //获取当前时间
		QString strCurDate = curTime.toString("yyyyMMdd"); //当前时间转化为字符串类型，格式为：年月日时分秒，如2019-05-30 14:28:15
		int nCurDate = strCurDate.toInt();

		if(nCurDate - nFileTime > m_nFileKeepTime) //比如m_nFileKeepTime = 10，则10天以前的文件都会被删除；这里时间截取到day，分钟和秒的信息被忽略
		{
			if(dir.remove(QString::fromStdString(vecToDeleteFiles[i])))
			{
				//printf("clw：定期删除文件，文件时间为%s\n", strFileTime.toLatin1().data());
				string strLogInfo = "clw：定期删除文件，文件时间为" + strFileTime.toStdString();
				WriteDailyLog(true, strLogInfo.c_str());
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
//4. 处理日志和打屏
void FileParser::DealWithLogs(char* logLines, bool bCout, int outFreq)
{
	if (m_circleTimes % outFreq == 1 || m_circleTimes == 0)
	{
		char logs[128];
		sprintf(logs, "【FileParser】 circleTimes=%d, ", m_circleTimes);
		strcat(logs, logLines);
		WriteDailyLog(true, logs);
		if (bCout == true)
		{
			printf("logs:%s\n", logs);
		}
	}
}
#endif 
///////////////////////////////////////////////////////////////////////
#ifdef Q_OS_LINUX
//4. 处理日志和打屏
void FileParser::DealWithLogs(string logLine, bool bCout, int outFreq)
{
	char* cLogLines = const_cast<char*>(logLine.c_str());
	if (m_circleTimes%outFreq == 1 || m_circleTimes == 0)
	{
		char logs[128];
		sprintf(logs, "【FileParser】 circleTimes=%d, ", m_circleTimes);
		strcat(logs, cLogLines);
		WriteDailyLog(true, logs);
		if (bCout == true)
		{
			printf("logs:%s\n", logs);
		}
	}
}
#endif 
