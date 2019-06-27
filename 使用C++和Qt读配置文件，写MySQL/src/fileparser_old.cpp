#include <qdatetime.h>  //for QDateTime
#include <QSqlQuery>
#include <QTextStream> //for QTextStream
#include <QSqlError>  //for query.lasterror()

#include "fileparser.h"

#include "./common/staticvariable.h"
#include "./common/writelog.h" // for WriteLogToFile
#include "./common/sysconfig.h"  
#include "./common/defs.h"


FileParser* FileParser::m_pInst = 0;

FileParser::FileParser(): m_bExit(false)
{
	m_pUpdateRealACfg = (new CUpdateDt())->Self(); //得到接口对象的同时，已经把动态库加载进来了；
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
	msleep(50);
	g_bIsDebug = false;

	//1、连接数据库
	SysConfig dbsys;
	if(dbsys.ConnectDatabas())	
	{
		printf("clw：数据库连接成功！\n");
		WriteLogToFile(LOGFILENAME, "clw：数据库连接成功！\n");
	}
	else 
	{
		printf("clw：数据库连接失败！\n");
		WriteLogToFile(LOGFILENAME, "clw：数据库连接失败！\n");
	}

	//2、从esfilesetting.ini文件中读入配置信息
	if(!ReadInConfigPara())
		return;

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
			GetFiles("../file", m_vecFiles);
		}
		int len = m_vecFiles.size();
		for(int i=0; i<len; ++i)
		{
			if(ParseESFile(m_vecFiles[i].c_str()))
			{
				//如果能够正确解析文件，写数据库表
				if(WriteFileInfoToDB())
				{
					//如果写数据库成功，则对处理过的文件进行标识（比如copy到其他地方），确保不会同一文件反复解析
					MoveFileToDst(m_vecFiles[i], "../file/processed", true);
				}
			}
			else
			{
				printf("ParseESFile()解析文件%s失败，继续解析其他文件\n", m_vecFiles[i].c_str());
				string strLogInfo = "clw：ParseESFile()解析文件" +  m_vecFiles[i] + "失败，继续解析其他文件！";
				WriteLogToFile(LOGFILENAME, strLogInfo.c_str());
			}

			//每次不管能否正确解析文件，之后要清掉96点功率值vector<>和n个96点功率值的vector<vector<>>，还要文件索引vec
			vector<float>().swap(m_vecShortP);
			vector<vector<float> >().swap(m_vecStationShortPs);
			vector<int>().swap(m_vecFilesInfoIndex);
		} 

		//最后清掉包含所有文件的vector<>，因为这些文件已经被处理过；	
		vector<string>().swap(m_vecFiles);  //或者m_vecFiles.swap(vector<string>());	

		//定期清理文件
		DeleteFileByTime();

		//等待30s，如果有新的文件，再读进来，解析；
		msleep(30000);

		/* clw note：
		在容器vector中，其内存占用的空间是只增不减的，比如说首先分配了10,000个字节，
		然后erase掉后面9,999个，则虽然有效元素只有一个，但是内存占用仍为10,000个。
		所有内存空间在vector析构时回收。
		一般，我们都会通过vector中成员函数clear进行一些清除操作，但它清除的是所有的元素，
		使vector的大小减少至0，却不能减小vector占用的内存。要避免vector持有它不再需要的内存，
		这就需要一种方法来使得它从曾经的容量减少至它现在需要的容量，这样减少容量的方法被称为
		“收缩到合适（shrink to fit）”。（节选自《Effective STL》）如果做到“收缩到合适”呢，嘿嘿，
		这就要全仰仗“Swap大侠”啦，即通过如下代码进行释放过剩的容量：

		在使用C++中的 vector的时候，vector的申请的内存不会自动释放，当 push_back的时候，
		如果 vector的当前内存不够使用的时候，vector会自动的二倍增长内存，可能会导致最后内存
		不断的增多。下面我们介绍几种避免这种情况的技巧。
		这个问题可以通过 swap来降低 vector对内存的消耗。
		swap可以有效降低当前 vector占用的内存总量。
		*/
	}
}


bool FileParser::GetFiles(const string& path, vector<string>& vecFiles)  
{  	
	int pathLen = path.length();
	if(pathLen >= 256)
	{
		printf("clw：文件名%s过长！\n", path.c_str());
		string strLogInfo = "clw：文件名" + path + "过长！\n";
		WriteLogToFile(LOGFILENAME, strLogInfo.c_str());
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
	QDir dir(tempdirPath);
	if(!dir.exists())
	{	
		printf("clw：路径%s不存在！\n", path.c_str());
		string strLogInfo = "clw：路径" +  path + "不存在！\n";
		WriteLogToFile(LOGFILENAME, strLogInfo.c_str());
		return false;
	}

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
		WriteLogToFile(LOGFILENAME, "clw：未找到esfilesetting.ini配置文件！\n");
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
	QString strCtrlID = "";
	QString strRecordID = "";
	stFileInfo iniFileInfo;

	while(!t.atEnd())
	{
		//依次得到ini文件的每一行
		str = codec->fromUnicode(t.readLine().stripWhiteSpace());
		//printf("clw：ini文件中解析的行的内容为%s\n", str.toLatin1().data());		

		//如果是空行代表结束，继续往下搜索
		if(str.isEmpty())
		{
			//printf("clw：ini中该行是空行！\n");	
			//如果strFileInfo有内容
			if(!iniFileInfo.m_map_nameInfo.empty() && !iniFileInfo.m_map_nameVals.empty())
			{
				//printf("clw：遇到空行结束，准备将之前读到的文件信息存入m_vecFilesInfo ! \n");
				//则将stFileInfo对象存入m_vecFilesInfo；
				m_vecFilesInfo.push_back(iniFileInfo);
				//清空strFileInfo内的map
				iniFileInfo.m_map_nameInfo.clear();
				iniFileInfo.m_map_nameVals.clear();
			}	
			continue;
		}

		////先看是不是[]，如果是[]说明是一个文件；
		//if(str.find('[') != string::npos && str.find(']') != string::npos &&)   //能找到[]
		//{
		//	;
		//}

		string::size_type nEqualSignPos = str.find('='); //
		if(nEqualSignPos != string::npos) //如果有等号，说明有配置项，准备读取
		{
			left = str.left(nEqualSignPos).stripWhiteSpace();  //比如内容为filetype，或者station1之类
			right = str.mid(nEqualSignPos + 1).stripWhiteSpace(); //pos位置在等号，取等号右边的数
			right.replace(";",""); 
			//提取right部分，比如内容为"1,10,#3"，第一个是LoadID，第二个是CtrlID，第三个是RecordID，按顺序写入一个vector
			//unsigned int nChnCommaPos = str.find(L'，'); //clw note：这里由于是中文逗号，所以一定要是双引号，即find字符串；如果是用单引号''应是宽字符wchar_t，需加L前缀；
			string::size_type nChnCommaPos = str.find("，"); //clw note：如果是unsigned int，这里结果在linux和windows都是429496725，但是linux下string::npos是size_type类型，相当于long long，所以不能写成unsigned int
			string::size_type nEngCommaPos = str.find(',');
			//printf("clw：nChnCommaPos = %u\n", nChnCommaPos ); 
			//printf("clw：nEngCommaPos = %u\n", nEngCommaPos);
			//printf("clw：nChnCommaPos = %d\n", nChnCommaPos ); 
			//printf("clw：nEngCommaPos = %d\n", nEngCommaPos);
			//printf("clw：string::npos = %d\n", string::npos); //clw note：linux下测试string::npos为-1，但是windows下为4294967295？？

			//如果找不到逗号，说明是一般配置项，如filetype=RNMXFHYC，存入m_map_nameInfo
			if(nChnCommaPos == string::npos  && nEngCommaPos == string::npos)  //clw note：
			{
				//printf("clw：找不到逗号，说明是一般配置项，准备读取\n");
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

				//int nComma2Pos = str.find(chComma, nComma1Pos + 1);
				////printf("clw：nComma1Pos = %d\n",  nComma1Pos);
				////printf("clw：nComma2Pos = %d\n",  nComma2Pos);
				////printf("clw：nEqualSignPos = %d\n",  nEqualSignPos);
				//if (nComma2Pos != -1) //找不到第二个逗号项，比如FILETYPE_AGCRQ，FILETYPE_AGCRN类型
				//{
				//	strCtrlID = str.mid(nComma1Pos + 1, nComma2Pos - nComma1Pos - 1).stripWhiteSpace();
				//	strRecordID = str.mid(nComma2Pos + 1).stripWhiteSpace();
				//}
				//else
				//{
				strRecordID = str.mid(nComma1Pos + 1).stripWhiteSpace();
				//}					
				strRecordID.replace("#","");   //操作前内容形如#3;
				strRecordID.replace(";","");

				//存储ini配置文件厂站id值与对应的txt记录的id值，如station1=1，#3;
				vector<string> tmpVec;
				tmpVec.push_back(strID.toStdString());
				if(!strCtrlID.isEmpty())
					tmpVec.push_back(strCtrlID.toStdString());
				tmpVec.push_back(strRecordID.toStdString());
				printf("clw：在读取ini配置文件时, strRecordID = %s\n", strRecordID.toLatin1().data());
				if(iniFileInfo.m_map_nameVals.end() != iniFileInfo.m_map_nameVals.find(left.toStdString()))//重复的元素
				{
					//printf("clw：覆盖ini的数值配置项, left = %s, right = %s\n", left.toLatin1().data(), (strID + strCtrlID + strRecordID).toLatin1().data());
					iniFileInfo.m_map_nameVals[left.toStdString()] = tmpVec;
				} 
				else //非重复元素，即新元素或m_map_nameVal.empty()第一个元素
				{
					//printf("clw：插入ini的数值配置项, left = %s, right = %s\n", left.toLatin1().data(), (strID + strCtrlID + strRecordID).toLatin1().data());
					iniFileInfo.m_map_nameVals.insert(pair<string, vector<string> >(left.toStdString(), tmpVec));
				}
				bSuc = true;
			}
			else
				bSuc = false;
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
		WriteLogToFile(LOGFILENAME, "clw：读取esfilesetting.ini文件失败，未获取到正确的信息！\n");
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

	string::size_type nSuffixPos = fileName.rfind('.'); //如果确定文件名只有一个.字符，那么这样写就行
	//unsigned int nSuffixPos = fileName.rfind(".txt");
	//if(nSuffixPos == string::npos) //如果找不到.txt后缀，再尝试.TXT后缀
	//	nSuffixPos = fileName.rfind(".TXT");

	string::size_type nRightUnderLinePos = fileName.rfind('_'); //准备取最后一个_到.TXT中间的内容
	string fileType = "";
	if(nSuffixPos != string::npos && nRightUnderLinePos != string::npos)
	{
		//截取文件名，比如超短期预测文件为RNMXFHYC，或短期预测文件为DQMXFHYC；
		//从某个文件BJ_20190524_DQMXFHYC.txt中截取
		fileType = fileName.substr(nRightUnderLinePos + 1, nSuffixPos - nRightUnderLinePos - 1); 


		//如果是超短期，还需要保存最后一个_前面的时间，写表的时候要用到；m_strForcastTime，如1540（代表15:40:00）
		unsigned int nLeftUnderLinePos;
		if(strcmp(fileType.c_str(), FILETYPE_ULTRA) == 0 || 
			strcmp(fileType.c_str(), FILETYPE_AGCRN) == 0) 
		{
			unsigned int nMiddleUnderLinePos;
			nMiddleUnderLinePos = fileName.rfind('_', nRightUnderLinePos - 1);
			nLeftUnderLinePos = fileName.rfind('_', nMiddleUnderLinePos - 1);
			if(nLeftUnderLinePos == string::npos)
			{
				printf("clw：TXT文件格式有误，如果是超短期文件类型RNMXFHYC，应该包含三个下划线！ \n");
				WriteLogToFile(LOGFILENAME, "clw：TXT文件格式有误，如果是超短期文件类型RNMXFHYC，应该包含三个下划线！ \n");
				return false;
			}
			m_strForcastTime = QString::fromStdString(fileName.substr(nLeftUnderLinePos + 1, nMiddleUnderLinePos - nLeftUnderLinePos - 1));
			m_strForcastTime += QString::fromStdString(fileName.substr(nMiddleUnderLinePos + 1, nRightUnderLinePos - nMiddleUnderLinePos - 1));
			if(m_strForcastTime.size() != 12) //如201905231415
			{
				printf("clw：TXT文件格式有误，下划线中间的日期格式应为201905231415！ \n");
				WriteLogToFile(LOGFILENAME, "clw：TXT文件格式有误，下划线中间的日期格式应为201905231415！ \n");
				return false;
			}
		}
		else if(strcmp(fileType.c_str(), FILETYPE_SHORT) == 0 || 
			strcmp(fileType.c_str(), FILETYPE_AGCRQ) == 0)
		{
			nLeftUnderLinePos = fileName.rfind('_', nRightUnderLinePos - 1);
			if(nLeftUnderLinePos == string::npos)
			{
				printf("clw：TXT文件格式有误，如果是超短期文件类型RNMXFHYC，应该包含三个下划线！ \n");
				WriteLogToFile(LOGFILENAME, "clw：TXT文件格式有误，如果是超短期文件类型RNMXFHYC，应该包含三个下划线！ \n");
				return false;
			}
			m_strForcastTime = QString::fromStdString(fileName.substr(nLeftUnderLinePos + 1, nRightUnderLinePos - nLeftUnderLinePos - 1));
			if(m_strForcastTime.size() != 8) //如20190524
			{
				printf("clw：TXT文件格式有误，下划线中间的日期格式应为20190524！ \n");
				WriteLogToFile(LOGFILENAME, "clw：TXT文件格式有误，下划线中间的日期格式应为20190524！ \n");
				return false;
			}
		}
	}
	else if(nRightUnderLinePos == string::npos)
	{
		printf("clw：TXT文件找不到下划线符号_ \n");
		WriteLogToFile(LOGFILENAME, "clw：TXT文件找不到下划线符号_ \n");
		return false;
	}
	else
	{
		printf("clw：找不到.txt格式的文件！\n");
		WriteLogToFile(LOGFILENAME, "clw：找不到.txt格式的文件！\n");
		return false;
	}

	int fileLen = m_vecFilesInfo.size();
	bool bFindFileName = false;

	//printf("clw：m_vecFilesInfo[i].m_map_nameInfo的fileLen = %d\n", fileLen);
	for(int i = 0; i < fileLen; ++i)
	{
		//判断ini文件是否含有该TXT文件类型，如果有，才能开始解析文件；
		//之前从ini获取了所有文件的配置信息m_vecFilesInfo
		//printf("clw：m_vecFilesInfo[i].m_map_nameInfo['filetype'] = %s\n", m_vecFilesInfo[i].m_map_nameInfo["filetype"].c_str());
		if(m_vecFilesInfo[i].m_map_nameInfo["filetype"] == fileType)
		{
			bFindFileName = true;
			m_vecFilesInfoIndex.push_back(i); //记录该文件对应的文件结构体索引号，方便以后使用
			//break;
		}		
	}
	if(!bFindFileName)
	{
		//如果这个文件名在m_vecFilesInfo中找不到，那么直接返回false
		printf("clw：ini文件中没有%s文件类型的配置信息！无法进行解析\n", fileType.c_str());
		string strLogInfo = "clw：ini文件中没有" +  fileType + "文件类型的配置信息！无法进行解析\n";
		WriteLogToFile(LOGFILENAME, strLogInfo.c_str());
		return false;
	}

	//打开文件
	FILE* fp = fopen(filepath, "r");
	if(fp == NULL)
	{
		printf("clw：打开文件%s失败！\n", filepath);
		string strLogInfo = "clw：打开文件" +  string(filepath) + "失败！\n";
		WriteLogToFile(LOGFILENAME, strLogInfo.c_str());
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
			//if(fileType == FILETYPE_SHORT || fileType == FILETYPE_ULTRA)
			//	strncpy(strRecordID, iter->second[2].c_str(), BUF_LEN_MIN - 1);//如#3，在这里只保留3
			//else if(fileType == FILETYPE_AGCRN || 
			//	fileType == FILETYPE_AGCRQ || 
			//	fileType == FILETYPE_XNYRN ||
			//	fileType == FILETYPE_XNYRQ)
			strncpy(strRecordID, iter->second[1].c_str(), BUF_LEN_MIN - 1);//如#3，在这里只保留3

			printf("clw: strRecordID = %s\n", strRecordID);
			//printf("clw:strRecordID = %s\n", strRecordID);
			int nRecordID = atoi(strRecordID); 
			printf("clw: nRecordID = %d\n", nRecordID);
			if(nRecordID > INT_MAX)
			{
				//error1
				printf("clw：strRecordID值超上限！\n");
				WriteLogToFile(LOGFILENAME, "clw：strRecordID值超上限！\n");
				free(pTxtBuf);
				fclose(fp);
				return false;
			}

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

			//方法二：#数量统计法（只统计位于第1列的#数量)
			char* pWellSign = pTxtBuf;  //定位在文章开始
			//printf("clw: nRecordID = %d\n", nRecordID);
			for(int i = 0; i < nRecordID; ++i)
			{
				pWellSign = strstr(pWellSign + 1, "#");
				//printf("clw：*(pWellSign+1) = %d\n", *(pWellSign+1));
				//printf("clw：*(pWellSign-1) = %d\n", *(pWellSign-1));

				if(*(pWellSign-1) != '\n') //位于第1列的#的特征是，前一个字符是'\n'，即上一行
				{
					i--;
					continue;
				}
			}

			//判断#后面出现的第1个数字是否是nRecordID，如果不是，则找不到id
			char* pBegin = pWellSign;
			pBegin++;
			while(/*!isdigit(*pBegin)*/ *pBegin < DIGIT_ASCII_MIN || *pBegin > DIGIT_ASCII_MAX) //非数字
			{
				if(*pBegin != ' ' && *pBegin != '\t')
				{
					//error2
					printf("clw：*pBegin = %d\n", *pBegin);
					printf("clw：TXT的#id格式不正确，请检查格式！\n");
					printf("clw：*pBegin = %d\n", *pBegin);
					printf("clw：*(pBegin+1) = %d\n", *(pBegin+1));
					printf("clw：*(pBegin-1) = %d\n", *(pBegin - 1));
					WriteLogToFile(LOGFILENAME, "clw：TXT的#id格式不正确，请检查格式！\n");
					free(pTxtBuf);
					fclose(fp);
					return false;
				}
				pBegin++;
			}

			//计算出记录的id值，比如#123，那么num = ((0*10+1)*10+2)*10+3
			int num = 0;
			while(*pBegin >= DIGIT_ASCII_MIN && *pBegin <= DIGIT_ASCII_MAX) //是数字
			{
				num = num * 10 + *pBegin - '0';
				pBegin++;
			}

			if(num != nRecordID)
			{
				//error3
				printf("计算出的记录id值为num = %d，与nRecordID = %d不等！\n", num, nRecordID);
				printf("clw：TXT找不到#id的记录！与ini中记录不符！\n");
				WriteLogToFile(LOGFILENAME, "clw：TXT找不到#id的记录！与ini中记录不符！\n");
				free(pTxtBuf);
				fclose(fp);
				return false;
			}
			//==================================================================================

			//将该行信息写入内存
			char *pEnd = strchr(pBegin, '\n');
			char *pRecordBuf = (char *)malloc((pEnd - pBegin + 2) * sizeof(char));
			memset(pRecordBuf, 0, pEnd - pBegin + 2); //clw add  clw note：20190604：这句务必要写，否则后面都是脏数据！！而且要保证以'\0'结尾，所以额外分配了一个字节，即pEnd- pBegin +1这个值加1
			strncpy(pRecordBuf, pBegin, pEnd - pBegin + 1);  //clw note：pRecordBuf[pEnd - pBegin]对应'\n'
			//printf("clw：打印TXT文件每一行的内容%s\n", pRecordBuf);


			//提取该行负荷有功预测值，96点
			//两个辅助指针变量挖字符串
			char *pCur = pRecordBuf;
			char *pPre = pCur;

			//开始取96点
			int count = 0;
			printf("clw note：开始取96点数据！\n");
			while(1) 
			{
				//clw add 20190604:读到96点数据就返回，因为有些行96点最后并不一定是\n，还有可能是乱码，容易误读。比如
				/*北京.石景山电厂/220kV.0甲#启备变-高等值负荷	0.3935  0	0.3935  0	0.3935  0	0.3935  0	
				0.3871  0	0.3808  0	0.3808  0	0.4217  0	0.4465  0	0.4636  0	0.4399  0	0.4001  0 .....
				0	q*/
				//if(m_vecShortP.size() == 96)
				//	break;

				//注意：应该是把空格，Tab都strchr找一遍，哪个排在前面，就先处理哪个
				//但是还有2个空格的情况，还需要考虑
				//跳过第一个空白
				char* pSpace = strchr(pCur, ' ');
				char* pTab = strchr(pCur, '\t');

				if(pSpace > pEnd || pTab > pEnd)  //说明已经提取完毕，跳出
					break; 

				if(pSpace == NULL && pTab == NULL)  //已经找不到空白，则跳出
				{
					if(count == 95) //最后一个值可能没有空格，因此要特殊处理，找'\n'
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


				//如果Ppre后面是数字，就把pPre和pCur中间的数字取出来，count+1，然后继续往下找
				//不要用isdigit()，因为如果是中文，有可能a=-79，这样用isdigit会宕掉
				if(/*isdigit(*(pPre + 1))*/ (*(pPre + 1) >= DIGIT_ASCII_MIN && *(pPre + 1) <= DIGIT_ASCII_MAX || *(pPre + 1) == '-'))  
				{
					//短期负荷预测的表包含无功数据，需要跳过，只取有功
					if(strcmp(fileType.c_str(), FILETYPE_SHORT) == 0 || 
						strcmp(fileType.c_str(), FILETYPE_AGCRQ) == 0)
					{
						if(count % 2 == 0) //只处理count=偶数，即有功负荷数据，跳过无功负荷预测数据
						{
							char buf[16] = {0};
							//strncpy(buf, pPre, pCur - pPre);
							strncpy(buf, pPre + 1, pCur - pPre - 1);
							//printf("clw：提取每行record的buf数值为%s\n", buf);
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
			}

			//clw note：读取96点结束，判断vec是否读到了96个值
			if(m_vecShortP.size() != 96)
			{
				printf("clw：负荷有功预测数据TXT文件是%d点，不是96点，请检查数据！\n", m_vecShortP.size());
				int len = m_vecShortP.size();
				for(int i=0;i<len;++i)
					printf("%lf ", m_vecShortP[i]);
				printf("\n");
				WriteLogToFile(LOGFILENAME, "clw：负荷有功预测数据TXT文件不是96点，请检查数据！\n");

				memset(pBegin, 0, pEnd - pBegin + 2);//因为下次for循环还要重新malloc，还是这段内存空间，所以释放之前最好是清空，否则容易有脏数据！
				//不过只要在申请内存之后memset，然后程序里读这块内存的时候注意加个判断，不要读越界就可以了。
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

	printf("clw：负荷有功预测数据TXT文件提取完毕，准备写mysql表！！\n");
	WriteLogToFile(LOGFILENAME, "clw：负荷有功预测数据TXT文件提取完毕，准备写mysql表！！\n");
	return true;
}

bool FileParser::WriteFileInfoToDB()
{
	bool bSuc = true;

	if(m_strForcastTime.isEmpty())
	{
		printf("clw：超短期文件起始预测时间获取失败，请检查文件名下划线格式！\n");
		WriteLogToFile(LOGFILENAME, "clw：超短期文件起始预测时间获取失败，请检查文件名下划线格式！");
		return false;
	}
	if(m_vecStationShortPs.empty())
	{
		printf("clw：未获取到96点数据，无法写数据库！\n");
		WriteLogToFile(LOGFILENAME, "clw：未获取到96点数据，无法写数据库！\n");
		return false;
	}


	//==============================================================================================
	//在QT中批量插入数据到mysql（达梦）之前的准备工作
	//首先判断文件类型，根据不同类型得到96点时间，得到各自的SQL语句
	//==============================================================================================
	//QString strSqlHeader = "";
	string fileType = m_vecFilesInfo[m_vecFilesInfoIndex[0]].m_map_nameInfo["filetype"];
	QDateTime curTime = QDateTime::currentDateTime();  //获取当前时间
	QString curYear = curTime.toString("yyyy"); //当前时间转化为字符串类型，格式为：年月日时分秒，如2019-05-30 14:28:15

	//根据不同文件类型的到不同的96点时间
	//比如m_strForcastTime时间是201905231415，说明文件类型是FILETYPE_ULTRA，即超短期负荷预测,则96点的第一条记录预测时间应为2019-05-23 14:15:00
	//    m_strForcastTime时间是20190524，说明文件类型是FILETYPE_SHORT，即短期负荷预测，则96点的第一条记录预测时间应为2019-06-03 00:00:00
	//14:15:00是通过下面对m_strForcastTime的内容读取然后处理操作得到的
	vector<string> vecForcastTime; //clw note：变长，还是用string了，之后96点处理再用c_str()转成char*
	QString curDateTime = curTime.toString("yyyy-MM-dd hh:mm:ss"); //当前时间转化为字符串类型，格式为：年月日时分秒，如2019-05-30 14:28:15
	QDateTime forcastTime;
	if(fileType == FILETYPE_SHORT || fileType == FILETYPE_AGCRQ)
	{
		//forcastTime = curTime.addDays(1);//当前时间+1天，之后会用到这个年月日
		m_strForcastTime.append(" 00:00:00");
	}
	else if(fileType == FILETYPE_ULTRA || fileType == FILETYPE_AGCRN)
	{
		m_strForcastTime.append(":00");
		m_strForcastTime.insert(10, ':');
		m_strForcastTime.insert(8, ' ');
	}
	m_strForcastTime.insert(6, '-');
	m_strForcastTime.insert(4, '-');
	forcastTime = QDateTime::fromString(m_strForcastTime, "yyyy-MM-dd hh:mm:ss");
	//96点时间写入vec
	QString firstPointTime = m_strForcastTime;//字符串格式，如2019-05-30 00:00:00
	for(int i = 0; i < 96; ++i)
	{
		QDateTime tempTime(forcastTime.addSecs(60 * 15 * i));
		firstPointTime = tempTime.toString("yyyy-MM-dd hh:mm:ss");
		vecForcastTime.push_back(firstPointTime.toStdString());
	}

	//根据不同文件类型，生成各自的sql语句
	QString sqlToExec;
	char strSQL[1024];
	QStringList lstSql;
	string tableName;
	/*遍历ini文件所有值类型记录，如
	station1=1,#3;
	station2=3,#6;
	station3=5,#8;
	station4=7,#9;
	这4条记录会得到96*4 = 384个点，对应mysql相应表的384条记录
	*/
	int count = 0;
	char *strFormat = NULL;
	string strID;
	int nID;
	string strCtrlID;
	int nCtrlID;
	if(fileType == FILETYPE_ULTRA)
	{
		int len = m_vecFilesInfoIndex.size();
		for(int i=0; i<len; ++i)
		{
			tableName = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameInfo["tablename"];
			if(tableName == "ultraloadhisdata")
			{
				sqlToExec = "INSERT INTO " + QString::fromStdString(tableName) + "_" + curYear + "(CTRLID, FORCASTTIME, SAVETIME, METHOD, FORCASTP) VALUES";
				for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
					iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
				{
					strFormat = "(%d, '%s', '%s', %d, %f)";
					strCtrlID = iter->second[0];
					nCtrlID = atoi(strCtrlID.c_str());

					for(int i = 0; i < 96 /*96，1测试用*/; ++i)
					{
						sprintf(strSQL, strFormat, nCtrlID , vecForcastTime[i].c_str(), curDateTime.toLatin1().data(), 0/*method，暂时写0*/, m_vecStationShortPs[count][i]);
						lstSql.append(strSQL);
					}

					if(count != 0)
						sqlToExec += ",";  //第一波数据前面不用加逗号
					count++;
					sqlToExec += lstSql.join(",");
					lstSql.clear();
				}
				//执行sql，写实时库
				bSuc = ExecSQL(sqlToExec);
				count = 0;
			}

			else if(tableName == "ultrahisdata")
			{
				sqlToExec = "INSERT INTO " + QString::fromStdString(tableName) + "_" + curYear + "(PVSTATIONID, FORCASTTIME, SAVETIME, METHOD, FORCASTP) VALUES";
				for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
					iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
				{
					strFormat = "(%d, '%s', '%s', %d, %f)";
					strID = iter->second[0];
					nID = atoi(strID.c_str());
					for(int i = 0; i < 96; ++i)
					{
						sprintf(strSQL, strFormat, nID, vecForcastTime[i].c_str(), curDateTime.toLatin1().data(), 0, m_vecStationShortPs[count][i]);
						lstSql.append(strSQL);
					}

					if(count != 0)
						sqlToExec += ",";  //第一波数据前面不用加逗号
					count++;
					sqlToExec += lstSql.join(",");
					lstSql.clear();
				}
				//执行sql，写实时库
				bSuc = ExecSQL(sqlToExec);
				count = 0;
			}
		}	//for end
	} //if end


	else if(fileType == FILETYPE_SHORT)
	{
		int len = m_vecFilesInfoIndex.size();
		for(int i=0; i<len; ++i)
		{
			tableName = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameInfo["tablename"];
			if(tableName == "shortloadhisdata")
			{
				sqlToExec = "INSERT INTO " + QString::fromStdString(tableName) + "_" + curYear + "(CTRLID, FORCASTTIME, SAVETIME, METHOD, FORCASTP) VALUES";	
			
				for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
					iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
				{
					strFormat = "(%d, '%s', '%s', %d, %f)";
					strCtrlID = iter->second[0];
					nCtrlID = atoi(strCtrlID.c_str());

					for(int i = 0; i < 96 /*96，1测试用*/; ++i)
					{
						sprintf(strSQL, strFormat, nCtrlID , vecForcastTime[i].c_str(), curDateTime.toLatin1().data(), 0/*method，暂时写0*/, m_vecStationShortPs[count][i]);
						lstSql.append(strSQL);
					}

					if(count != 0)
						sqlToExec += ",";  //第一波数据前面不用加逗号
					count++;
					sqlToExec += lstSql.join(",");
					lstSql.clear();
				}
				//执行sql，写实时库
				bSuc = ExecSQL(sqlToExec);
				count = 0;
			}

			else if(tableName == "shorthisdata")
			{
				sqlToExec = "INSERT INTO " + QString::fromStdString(tableName) + "_" + curYear + "(PVSTATIONID, FORCASTTIME, SAVETIME, METHOD, FORCASTP) VALUES";
			
				for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
					iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
				{
					strFormat = "(%d, '%s', '%s', %d, %f)";
					strID = iter->second[0];
					nID = atoi(strID.c_str());
					for(int i = 0; i < 96; ++i)
					{
						sprintf(strSQL, strFormat, nID, vecForcastTime[i].c_str(), curDateTime.toLatin1().data(), 0, m_vecStationShortPs[count][i]);
						lstSql.append(strSQL);
					}

					if(count != 0)
						sqlToExec += ",";  //第一波数据前面不用加逗号
					count++;
					sqlToExec += lstSql.join(",");
					lstSql.clear();
				}
				//执行sql，写实时库
				bSuc = ExecSQL(sqlToExec);
				count = 0;
			}
		} //for end
	} // if end


	//TODO：其他类型文件，后续补充；每种类型的时间间隔还需要修改，比如5分钟，15分钟等，点数有48点和96点；
	else if(fileType == FILETYPE_AGCRN)
	{
		int len = m_vecFilesInfoIndex.size();
		for(int i=0; i<len; ++i)
		{
			tableName = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameInfo["tablename"];
			sqlToExec = "INSERT INTO " + QString::fromStdString(tableName) + "_" + curYear + "(ID, PLANDATE, PLANVALUESQ, AUTOPLANAIM, PLANREVISEDAIM, OPRTREVISEDAIM) VALUES";
			for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
				iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
			{
				strFormat = "(%d, '%s', %d, %f, %f, %f)";
				strID = iter->second[0];
				nID = atoi(strID.c_str());
				for(int i = 0; i < 48; ++i)
				{
					sprintf(strSQL, strFormat, nID, vecForcastTime[i].c_str(), i, m_vecStationShortPs[count][i], m_vecStationShortPs[count][i], m_vecStationShortPs[count][i]);
					lstSql.append(strSQL);
				}

				if(count != 0)
					sqlToExec += ",";  //第一波数据前面不用加逗号
				count++;
				sqlToExec += lstSql.join(",");
				lstSql.clear();
			}
			//执行sql，写实时库
			bool bSuc1 = ExecSQL(sqlToExec);
			printf("clw：准备写实时库和配置库，nID = %d\n", nID);
			bool bSuc2 = WriteRtdbAndConfigDB(fileType, nID);
			bSuc = bSuc1 && bSuc2;
			count = 0;
		}
	}

	else if(fileType == FILETYPE_AGCRQ)
	{
		int len = m_vecFilesInfoIndex.size();
		for(int i=0; i<len; ++i)
		{
			tableName = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameInfo["tablename"];
			sqlToExec = "INSERT INTO " + QString::fromStdString(tableName) + "_" + curYear + "(ID, PLANDATE, PLANVALUESQ, AUTOPLANAIM, PLANREVISEDAIM, OPRTREVISEDAIM) VALUES";
		
			for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
				iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
			{
				strFormat = "(%d, '%s', %d, %f, %f, %f)";
				strID = iter->second[0]; //TODO：不一定是负荷，还可能是新能源发电计划。。。改名字
				nID = atoi(strID.c_str());
				for(int i = 0; i < 96; ++i)
				{
					sprintf(strSQL, strFormat, nID, vecForcastTime[i].c_str(), i, m_vecStationShortPs[count][i], m_vecStationShortPs[count][i], m_vecStationShortPs[count][i]);
					lstSql.append(strSQL);
				}

				if(count != 0)
					sqlToExec += ",";  //第一波数据前面不用加逗号
				count++;
				sqlToExec += lstSql.join(",");
				lstSql.clear();
			}
			//执行sql，写实时库
			bool bSuc1 = ExecSQL(sqlToExec);
			printf("clw：准备写实时库和配置库，nID = %d\n", nID);
			bool bSuc2 = WriteRtdbAndConfigDB(fileType, nID);
			bSuc = bSuc1 && bSuc2;
			count = 0;
		}
	}

	else
	{
		printf("clw：没有此文件类型！写MySQL失败！\n");
		WriteLogToFile(LOGFILENAME, "clw：没有此文件类型！写MySQL失败！\n");
		return false;
	}
	//==============================================================================================


	return bSuc;
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
		printf("执行mysql失败: %s \n", sqlToExec.toLatin1().data());
		string logInfo = "Fails to execute sql: " + sqlToExec.toStdString() + "\n" ;
		WriteLogToFile(LOGFILENAME, logInfo.c_str());

		printf("[%s: %d] - %s\n", g_dbdriver.toLatin1().data(), query.lastError().number(), 
			query.lastError().text().toLocal8Bit().data());
		QString qLogInfo = "[" + g_dbdriver + ": " +  QString::number(query.lastError().number()) +  "] - " + query.lastError().text();
		WriteLogToFile(LOGFILENAME, qLogInfo.toLatin1().data());
		return false;
	}
	else
	{
		printf("clw：执行mysql成功！\n");
		WriteLogToFile(LOGFILENAME, "执行mysql成功！\n");
	}
	return true;
	//==============================================================================================
}

bool FileParser::WriteRtdbAndConfigDB(const string& fileType, const int& nID)
{
	//==============================================================================================
	//写实时库表
	//调用wx接口，写实时库stationdispc表（场站调度控制信息）的
	//AGC日前计划最近接收时间 和 AGC滚动（日内）计划最近接收时间
	//==============================================================================================
	icsvariant fldValue;
	fldValue.type = ft_time;
	fldValue.value.t = (double)CDateTime::GetCurrentDateTime();
	fldValue.isNull = false;
	if(fileType == FILETYPE_AGCRN)
	{
		printf("clw：准备写入实时库stationdispc表：\n");
		WriteLogToFile(LOGFILENAME, "clw：准备写入实时库stationdispc表：\n");
		if(m_pUpdateRealACfg->UpdateDataInReal("stationdispc", "todayagctime", nID, fldValue))
		{
			printf("clw：写入实时库stationdispc表，todayagctime字段成功！\n");
			WriteLogToFile(LOGFILENAME, "clw：写入实时库stationdispc表，todayagctime字段成功！\n");
		}
		else
		{
			printf("clw：写入实时库stationdispc表，todayagctime字段失败！\n");
			WriteLogToFile(LOGFILENAME, "clw：写入实时库stationdispc表，todayagctime字段失败！\n");
			return false;
		}

		printf("clw：准备写入配置库stationdispc表：\n");
		WriteLogToFile(LOGFILENAME, "clw：准备写配置库stationdispc表：\n");
		if(m_pUpdateRealACfg->UpdateDataInCfg("stationdispc", "todayagctime", nID, fldValue))
		{
			printf("clw：写入配置库stationdispc表，todayagctime字段成功！\n");
			WriteLogToFile(LOGFILENAME, "clw：写入配置库stationdispc表，todayagctime字段成功！\n");
		}
		else
		{
			printf("clw：写入配置库stationdispc表，todayagctime字段失败！\n");
			WriteLogToFile(LOGFILENAME, "clw：写入配置库stationdispc表，todayagctime字段失败！\n");
			return false;
		}
	}
	else if(fileType == FILETYPE_AGCRQ)
	{
		printf("clw：准备写入实时库stationdispc表：\n");
		WriteLogToFile(LOGFILENAME, "clw：准备写入实时库stationdispc表：\n");
		if(m_pUpdateRealACfg->UpdateDataInReal("stationdispc", "nextdayagctime", nID, fldValue))
		{
			printf("clw：写入实时库stationdispc表，nextdayagctime字段成功！\n");
			WriteLogToFile(LOGFILENAME, "clw：写入实时库stationdispc表，nextdayagctime字段成功！\n");
		}
		else
		{
			printf("clw：写入实时库stationdispc表，nextdayagctime字段失败！\n");
			WriteLogToFile(LOGFILENAME, "clw：写入实时库stationdispc表，nextdayagctime字段失败！\n");
			return false;
		}

		printf("clw：准备写入配置库stationdispc表：\n");
		WriteLogToFile(LOGFILENAME, "clw：准备写配置库stationdispc表：\n");
		if(m_pUpdateRealACfg->UpdateDataInCfg("stationdispc", "nextdayagctime", nID, fldValue))
		{
			printf("clw：写入配置库stationdispc表，nextdayagctime字段成功！\n");
			WriteLogToFile(LOGFILENAME, "clw：写入配置库stationdispc表，nextdayagctime字段成功！\n");
		}
		else
		{
			printf("clw：写入配置库stationdispc表，nextdayagctime字段失败！\n");
			WriteLogToFile(LOGFILENAME, "clw：写入配置库stationdispc表，nextdayagctime字段失败！\n");
			return false;
		}
	}
	return true;
	//==============================================================================================
}


bool FileParser::MoveFileToDst(string _srcDir, string _dstDir, bool coverFileIfExist)
{
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
			printf("clw：创建路径%s失败，请检查！\n", dstDir.toLatin1().data());
			WriteLogToFile(LOGFILENAME, "clw：创建路径失败，请检查！\n");
			return false;
		}
	}//end if

	//是否覆盖
	if(coverFileIfExist)
	{
		if(dir.remove(dstDir + "/" + fileName))
		{
			QString strLogInfo = dstDir + "/" + fileName;
			printf("clw：存在旧文件%s并且成功删除，准备写入新文件\n", strLogInfo.toLatin1().data());
			WriteLogToFile(LOGFILENAME, "clw：存在旧文件%s并且成功删除，准备写入新文件\n");
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
	//TODO：定期删除文件，暂定删除超过十天的文件
	QString filePath = "../file/processed";
	QDir dir;

	vector<string> vecToDeleteFiles;
	GetFiles(filePath.toStdString(), vecToDeleteFiles);
	int len = vecToDeleteFiles.size();
	for(int i=0; i<len; ++i)
	{
		string::size_type nLeftUnderLinePos = vecToDeleteFiles[i].find('_'); 
		string::size_type nMiddleUnderLinePos = vecToDeleteFiles[i].find('_', nLeftUnderLinePos + 1); 
		QString strFileTime = QString::fromStdString(vecToDeleteFiles[i].substr(nLeftUnderLinePos + 1, nMiddleUnderLinePos - nLeftUnderLinePos - 1));
		int nFileTime = strFileTime.toInt();

		QDateTime curTime = QDateTime::currentDateTime();  //获取当前时间
		QString strCurDate = curTime.toString("yyyyMMdd"); //当前时间转化为字符串类型，格式为：年月日时分秒，如2019-05-30 14:28:15
		int nCurDate = strCurDate.toInt();

		if(nCurDate - nFileTime > 10)
		{
			if(dir.remove(QString::fromStdString(vecToDeleteFiles[i])))
			{
				printf("clw：定期删除文件，文件时间为%s\n", strFileTime.toLatin1().data());
				string strLogInfo = "clw：定期删除文件，文件时间为" + strFileTime.toStdString();
				WriteLogToFile(LOGFILENAME, strLogInfo.c_str());
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
		WriteLogToFile(LOGFILENAME, logs);
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
		WriteLogToFile(LOGFILENAME, logs);
		if (bCout == true)
		{
			printf("logs:%s\n", logs);
		}
	}
}
#endif 