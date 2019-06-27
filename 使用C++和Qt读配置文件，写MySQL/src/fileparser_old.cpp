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
	m_pUpdateRealACfg = (new CUpdateDt())->Self(); //�õ��ӿڶ����ͬʱ���Ѿ��Ѷ�̬����ؽ����ˣ�
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
	//�߳�������ʼ ��ֹ50ms
	msleep(50);
	g_bIsDebug = false;

	//1���������ݿ�
	SysConfig dbsys;
	if(dbsys.ConnectDatabas())	
	{
		printf("clw�����ݿ����ӳɹ���\n");
		WriteLogToFile(LOGFILENAME, "clw�����ݿ����ӳɹ���\n");
	}
	else 
	{
		printf("clw�����ݿ�����ʧ�ܣ�\n");
		WriteLogToFile(LOGFILENAME, "clw�����ݿ�����ʧ�ܣ�\n");
	}

	//2����esfilesetting.ini�ļ��ж���������Ϣ
	if(!ReadInConfigPara())
		return;

	while(!m_bExit)
	{
		//3�������ļ�����
		//д���ڸ���Ԥ���򳬶��ڸ���Ԥ���
		//�����ڸ���Ԥ�⣬δ��4Сʱ  
		//���ڸ���Ԥ����ǰ�Ѿ�д��

		//ע�⣺��1������֮��������ɾ������Ҫ����һ��ʱ�䣬�����Ҫ��ʶĳ���ļ��Ѿ����������ˣ������޸�ǰ׺������copy����һ���ط�
		//       ��2��Ӧ��ȡĳһǰ׺����BJ_�����ļ����н���
		//       ��3������1���߳�or2���߳�
		//       ��4���趨1��ʱ����������һ������һ���ļ���Ȼ�����һ��

		if(m_vecFiles.empty())
		{
			GetFiles("../file", m_vecFiles);
		}
		int len = m_vecFiles.size();
		for(int i=0; i<len; ++i)
		{
			if(ParseESFile(m_vecFiles[i].c_str()))
			{
				//����ܹ���ȷ�����ļ���д���ݿ��
				if(WriteFileInfoToDB())
				{
					//���д���ݿ�ɹ�����Դ�������ļ����б�ʶ������copy�������ط�����ȷ������ͬһ�ļ���������
					MoveFileToDst(m_vecFiles[i], "../file/processed", true);
				}
			}
			else
			{
				printf("ParseESFile()�����ļ�%sʧ�ܣ��������������ļ�\n", m_vecFiles[i].c_str());
				string strLogInfo = "clw��ParseESFile()�����ļ�" +  m_vecFiles[i] + "ʧ�ܣ��������������ļ���";
				WriteLogToFile(LOGFILENAME, strLogInfo.c_str());
			}

			//ÿ�β����ܷ���ȷ�����ļ���֮��Ҫ���96�㹦��ֵvector<>��n��96�㹦��ֵ��vector<vector<>>����Ҫ�ļ�����vec
			vector<float>().swap(m_vecShortP);
			vector<vector<float> >().swap(m_vecStationShortPs);
			vector<int>().swap(m_vecFilesInfoIndex);
		} 

		//���������������ļ���vector<>����Ϊ��Щ�ļ��Ѿ����������	
		vector<string>().swap(m_vecFiles);  //����m_vecFiles.swap(vector<string>());	

		//���������ļ�
		DeleteFileByTime();

		//�ȴ�30s��������µ��ļ����ٶ�������������
		msleep(30000);

		/* clw note��
		������vector�У����ڴ�ռ�õĿռ���ֻ�������ģ�����˵���ȷ�����10,000���ֽڣ�
		Ȼ��erase������9,999��������Ȼ��ЧԪ��ֻ��һ���������ڴ�ռ����Ϊ10,000����
		�����ڴ�ռ���vector����ʱ���ա�
		һ�㣬���Ƕ���ͨ��vector�г�Ա����clear����һЩ�����������������������е�Ԫ�أ�
		ʹvector�Ĵ�С������0��ȴ���ܼ�Сvectorռ�õ��ڴ档Ҫ����vector������������Ҫ���ڴ棬
		�����Ҫһ�ַ�����ʹ������������������������������Ҫ���������������������ķ�������Ϊ
		�����������ʣ�shrink to fit����������ѡ�ԡ�Effective STL����������������������ʡ��أ��ٺ٣�
		���Ҫȫ���̡�Swap������������ͨ�����´�������ͷŹ�ʣ��������

		��ʹ��C++�е� vector��ʱ��vector��������ڴ治���Զ��ͷţ��� push_back��ʱ��
		��� vector�ĵ�ǰ�ڴ治��ʹ�õ�ʱ��vector���Զ��Ķ��������ڴ棬���ܻᵼ������ڴ�
		���ϵ����ࡣ�������ǽ��ܼ��ֱ�����������ļ��ɡ�
		����������ͨ�� swap������ vector���ڴ�����ġ�
		swap������Ч���͵�ǰ vectorռ�õ��ڴ�������
		*/
	}
}


bool FileParser::GetFiles(const string& path, vector<string>& vecFiles)  
{  	
	int pathLen = path.length();
	if(pathLen >= 256)
	{
		printf("clw���ļ���%s������\n", path.c_str());
		string strLogInfo = "clw���ļ���" + path + "������\n";
		WriteLogToFile(LOGFILENAME, strLogInfo.c_str());
		return false;
	}

	bool bSuc = false;
	QString tempdirPath;
	char charp[256] = {0}; //clw note��������б��ʼ������char charp[256] = "";�ȼ�

	int i;
	for(i = 0; i < pathLen; ++i)
	{
		//if ( path[i] == 0x0d )	//clw note��ASCIIֵ=13��Ӧ�ǻس��������ļ����к���û�лس����о����������ȥ��
		//	break;	
		charp[i] = path[i];
	}
	charp[i] = '\0';
	tempdirPath.sprintf("%s", charp);
	QDir dir(tempdirPath);
	if(!dir.exists())
	{	
		printf("clw��·��%s�����ڣ�\n", path.c_str());
		string strLogInfo = "clw��·��" +  path + "�����ڣ�\n";
		WriteLogToFile(LOGFILENAME, strLogInfo.c_str());
		return false;
	}

	QStringList filtersWai;
	//��������������ù�����
	QDirIterator dir_iteratorWai(tempdirPath,
		filtersWai,
		QDir::Files | QDir::NoSymLinks 
		|QDir::NoDotAndDotDot /*��"."��".."Ŀ¼����, ����ͻᵱ����Ŀ¼�����������ѭ��. */
		/*,QDirIterator::Subdirectories*/);

	while(dir_iteratorWai.hasNext())
	{
		dir_iteratorWai.next();
		QFileInfo file_info = dir_iteratorWai.fileInfo();
		QString absolute_file_path = file_info.absoluteFilePath();
		vecFiles.push_back(absolute_file_path.toStdString());
		bSuc = true;
	}
	sort(vecFiles.begin(), vecFiles.end());//����
	return bSuc;
} 


bool FileParser::ReadInConfigPara()
{
	bool bSuc = false;
	QFile f(PARA_INI_PATH);
	if(!f.open(QIODevice::ReadOnly))
	{
		printf("clw��δ�ҵ�esfilesetting.ini�����ļ���\n");
		WriteLogToFile(LOGFILENAME, "clw��δ�ҵ�esfilesetting.ini�����ļ���\n");
		return bSuc;
	}

	QTextStream t(&f);
	QString str;
#ifdef Q_OS_WIN
	QTextCodec *codec=QTextCodec::codecForName("GBK"); //TODO���Ƿ�Ҫ��Ϊwin��GBK��linux��UTF8
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
		//���εõ�ini�ļ���ÿһ��
		str = codec->fromUnicode(t.readLine().stripWhiteSpace());
		//printf("clw��ini�ļ��н������е�����Ϊ%s\n", str.toLatin1().data());		

		//����ǿ��д��������������������
		if(str.isEmpty())
		{
			//printf("clw��ini�и����ǿ��У�\n");	
			//���strFileInfo������
			if(!iniFileInfo.m_map_nameInfo.empty() && !iniFileInfo.m_map_nameVals.empty())
			{
				//printf("clw���������н�����׼����֮ǰ�������ļ���Ϣ����m_vecFilesInfo ! \n");
				//��stFileInfo�������m_vecFilesInfo��
				m_vecFilesInfo.push_back(iniFileInfo);
				//���strFileInfo�ڵ�map
				iniFileInfo.m_map_nameInfo.clear();
				iniFileInfo.m_map_nameVals.clear();
			}	
			continue;
		}

		////�ȿ��ǲ���[]�������[]˵����һ���ļ���
		//if(str.find('[') != string::npos && str.find(']') != string::npos &&)   //���ҵ�[]
		//{
		//	;
		//}

		string::size_type nEqualSignPos = str.find('='); //
		if(nEqualSignPos != string::npos) //����еȺţ�˵���������׼����ȡ
		{
			left = str.left(nEqualSignPos).stripWhiteSpace();  //��������Ϊfiletype������station1֮��
			right = str.mid(nEqualSignPos + 1).stripWhiteSpace(); //posλ���ڵȺţ�ȡ�Ⱥ��ұߵ���
			right.replace(";",""); 
			//��ȡright���֣���������Ϊ"1,10,#3"����һ����LoadID���ڶ�����CtrlID����������RecordID����˳��д��һ��vector
			//unsigned int nChnCommaPos = str.find(L'��'); //clw note���������������Ķ��ţ�����һ��Ҫ��˫���ţ���find�ַ�����������õ�����''Ӧ�ǿ��ַ�wchar_t�����Lǰ׺��
			string::size_type nChnCommaPos = str.find("��"); //clw note�������unsigned int����������linux��windows����429496725������linux��string::npos��size_type���ͣ��൱��long long�����Բ���д��unsigned int
			string::size_type nEngCommaPos = str.find(',');
			//printf("clw��nChnCommaPos = %u\n", nChnCommaPos ); 
			//printf("clw��nEngCommaPos = %u\n", nEngCommaPos);
			//printf("clw��nChnCommaPos = %d\n", nChnCommaPos ); 
			//printf("clw��nEngCommaPos = %d\n", nEngCommaPos);
			//printf("clw��string::npos = %d\n", string::npos); //clw note��linux�²���string::nposΪ-1������windows��Ϊ4294967295����

			//����Ҳ������ţ�˵����һ���������filetype=RNMXFHYC������m_map_nameInfo
			if(nChnCommaPos == string::npos  && nEngCommaPos == string::npos)  //clw note��
			{
				//printf("clw���Ҳ������ţ�˵����һ�������׼����ȡ\n");
				if(iniFileInfo.m_map_nameInfo.end() != iniFileInfo.m_map_nameInfo.find(left.toStdString()))//�ظ���Ԫ��
				{
					//printf("clw������ini��һ��������, left = %s, right = %s\n", left.toLatin1().data(), right.toLatin1().data());
					iniFileInfo.m_map_nameInfo[left.toStdString()] = right.toStdString();
				} 
				else// ���ظ�Ԫ�أ�����Ԫ�ػ�m_map_nameInfo.empty()��һ��Ԫ��
				{
					//printf("clw������ini��һ��������, left = %s, right = %s\n", left.toLatin1().data(), right.toLatin1().data());
					iniFileInfo.m_map_nameInfo.insert(pair<string,string>(left.toStdString(),right.toStdString()));
				}	
				bSuc = true;
			}
			//������ҵ����ţ�˵������ֵ�������station1=1,10,#3;
			else if(nChnCommaPos != string::npos && nEngCommaPos == string::npos ||
				nChnCommaPos == string::npos && nEngCommaPos != string::npos)
			{
				//printf("clw�����ҵ����ţ�˵������ֵ�����׼����ȡ\n");
				//ͬʱ֧�����ĺ�Ӣ�Ķ���
				wchar_t chComma;
				int nComma1Pos;
				if(nChnCommaPos != string::npos)
				{
					chComma = '��';
					nComma1Pos = nChnCommaPos;
				}
				else                              
				{
					chComma = ',';
					nComma1Pos = nEngCommaPos;
				}

				//clw note��str.right�Ǵ��ұ߽翪ʼ��ǰ��ȡx���ֽڣ������Ǵ�ĳ��λ��x����β����Ҫ���á���
				strID = str.mid(nEqualSignPos + 1, nComma1Pos - nEqualSignPos - 1).stripWhiteSpace(); 

				//int nComma2Pos = str.find(chComma, nComma1Pos + 1);
				////printf("clw��nComma1Pos = %d\n",  nComma1Pos);
				////printf("clw��nComma2Pos = %d\n",  nComma2Pos);
				////printf("clw��nEqualSignPos = %d\n",  nEqualSignPos);
				//if (nComma2Pos != -1) //�Ҳ����ڶ������������FILETYPE_AGCRQ��FILETYPE_AGCRN����
				//{
				//	strCtrlID = str.mid(nComma1Pos + 1, nComma2Pos - nComma1Pos - 1).stripWhiteSpace();
				//	strRecordID = str.mid(nComma2Pos + 1).stripWhiteSpace();
				//}
				//else
				//{
				strRecordID = str.mid(nComma1Pos + 1).stripWhiteSpace();
				//}					
				strRecordID.replace("#","");   //����ǰ��������#3;
				strRecordID.replace(";","");

				//�洢ini�����ļ���վidֵ���Ӧ��txt��¼��idֵ����station1=1��#3;
				vector<string> tmpVec;
				tmpVec.push_back(strID.toStdString());
				if(!strCtrlID.isEmpty())
					tmpVec.push_back(strCtrlID.toStdString());
				tmpVec.push_back(strRecordID.toStdString());
				printf("clw���ڶ�ȡini�����ļ�ʱ, strRecordID = %s\n", strRecordID.toLatin1().data());
				if(iniFileInfo.m_map_nameVals.end() != iniFileInfo.m_map_nameVals.find(left.toStdString()))//�ظ���Ԫ��
				{
					//printf("clw������ini����ֵ������, left = %s, right = %s\n", left.toLatin1().data(), (strID + strCtrlID + strRecordID).toLatin1().data());
					iniFileInfo.m_map_nameVals[left.toStdString()] = tmpVec;
				} 
				else //���ظ�Ԫ�أ�����Ԫ�ػ�m_map_nameVal.empty()��һ��Ԫ��
				{
					//printf("clw������ini����ֵ������, left = %s, right = %s\n", left.toLatin1().data(), (strID + strCtrlID + strRecordID).toLatin1().data());
					iniFileInfo.m_map_nameVals.insert(pair<string, vector<string> >(left.toStdString(), tmpVec));
				}
				bSuc = true;
			}
			else
				bSuc = false;
		}
	}

	//���strFileInfo�������ݣ���Ϊһ�������û�п��еģ�����ʱ���һ���ļ���������Ϣ��δ����
	if(!iniFileInfo.m_map_nameInfo.empty() || !iniFileInfo.m_map_nameVals.empty())
	{
		//��stFileInfo�������m_vecFilesInfo��
		m_vecFilesInfo.push_back(iniFileInfo);
	}	

	if(m_vecFilesInfo.empty())
	{
		printf("clw����ȡesfilesetting.ini�ļ�ʧ�ܣ�δ��ȡ����ȷ����Ϣ��\n");
		WriteLogToFile(LOGFILENAME, "clw����ȡesfilesetting.ini�ļ�ʧ�ܣ�δ��ȡ����ȷ����Ϣ��\n");
		bSuc = false;
	}

	f.close();
	return bSuc;
}


//�����ļ�·����ȡ�ļ���
string FileParser::GetFileNameByPath(const string& filepath)
{
	QStringList qStrList = QString::fromStdString(filepath).split("/");
	return qStrList[qStrList.length() - 1].toStdString(); //�ļ�������xxx.TXT
}


bool FileParser::ParseESFile(const char filepath[])
{
	string fileName = GetFileNameByPath(filepath);

	string::size_type nSuffixPos = fileName.rfind('.'); //���ȷ���ļ���ֻ��һ��.�ַ�����ô����д����
	//unsigned int nSuffixPos = fileName.rfind(".txt");
	//if(nSuffixPos == string::npos) //����Ҳ���.txt��׺���ٳ���.TXT��׺
	//	nSuffixPos = fileName.rfind(".TXT");

	string::size_type nRightUnderLinePos = fileName.rfind('_'); //׼��ȡ���һ��_��.TXT�м������
	string fileType = "";
	if(nSuffixPos != string::npos && nRightUnderLinePos != string::npos)
	{
		//��ȡ�ļ��������糬����Ԥ���ļ�ΪRNMXFHYC�������Ԥ���ļ�ΪDQMXFHYC��
		//��ĳ���ļ�BJ_20190524_DQMXFHYC.txt�н�ȡ
		fileType = fileName.substr(nRightUnderLinePos + 1, nSuffixPos - nRightUnderLinePos - 1); 


		//����ǳ����ڣ�����Ҫ�������һ��_ǰ���ʱ�䣬д���ʱ��Ҫ�õ���m_strForcastTime����1540������15:40:00��
		unsigned int nLeftUnderLinePos;
		if(strcmp(fileType.c_str(), FILETYPE_ULTRA) == 0 || 
			strcmp(fileType.c_str(), FILETYPE_AGCRN) == 0) 
		{
			unsigned int nMiddleUnderLinePos;
			nMiddleUnderLinePos = fileName.rfind('_', nRightUnderLinePos - 1);
			nLeftUnderLinePos = fileName.rfind('_', nMiddleUnderLinePos - 1);
			if(nLeftUnderLinePos == string::npos)
			{
				printf("clw��TXT�ļ���ʽ��������ǳ������ļ�����RNMXFHYC��Ӧ�ð��������»��ߣ� \n");
				WriteLogToFile(LOGFILENAME, "clw��TXT�ļ���ʽ��������ǳ������ļ�����RNMXFHYC��Ӧ�ð��������»��ߣ� \n");
				return false;
			}
			m_strForcastTime = QString::fromStdString(fileName.substr(nLeftUnderLinePos + 1, nMiddleUnderLinePos - nLeftUnderLinePos - 1));
			m_strForcastTime += QString::fromStdString(fileName.substr(nMiddleUnderLinePos + 1, nRightUnderLinePos - nMiddleUnderLinePos - 1));
			if(m_strForcastTime.size() != 12) //��201905231415
			{
				printf("clw��TXT�ļ���ʽ�����»����м�����ڸ�ʽӦΪ201905231415�� \n");
				WriteLogToFile(LOGFILENAME, "clw��TXT�ļ���ʽ�����»����м�����ڸ�ʽӦΪ201905231415�� \n");
				return false;
			}
		}
		else if(strcmp(fileType.c_str(), FILETYPE_SHORT) == 0 || 
			strcmp(fileType.c_str(), FILETYPE_AGCRQ) == 0)
		{
			nLeftUnderLinePos = fileName.rfind('_', nRightUnderLinePos - 1);
			if(nLeftUnderLinePos == string::npos)
			{
				printf("clw��TXT�ļ���ʽ��������ǳ������ļ�����RNMXFHYC��Ӧ�ð��������»��ߣ� \n");
				WriteLogToFile(LOGFILENAME, "clw��TXT�ļ���ʽ��������ǳ������ļ�����RNMXFHYC��Ӧ�ð��������»��ߣ� \n");
				return false;
			}
			m_strForcastTime = QString::fromStdString(fileName.substr(nLeftUnderLinePos + 1, nRightUnderLinePos - nLeftUnderLinePos - 1));
			if(m_strForcastTime.size() != 8) //��20190524
			{
				printf("clw��TXT�ļ���ʽ�����»����м�����ڸ�ʽӦΪ20190524�� \n");
				WriteLogToFile(LOGFILENAME, "clw��TXT�ļ���ʽ�����»����м�����ڸ�ʽӦΪ20190524�� \n");
				return false;
			}
		}
	}
	else if(nRightUnderLinePos == string::npos)
	{
		printf("clw��TXT�ļ��Ҳ����»��߷���_ \n");
		WriteLogToFile(LOGFILENAME, "clw��TXT�ļ��Ҳ����»��߷���_ \n");
		return false;
	}
	else
	{
		printf("clw���Ҳ���.txt��ʽ���ļ���\n");
		WriteLogToFile(LOGFILENAME, "clw���Ҳ���.txt��ʽ���ļ���\n");
		return false;
	}

	int fileLen = m_vecFilesInfo.size();
	bool bFindFileName = false;

	//printf("clw��m_vecFilesInfo[i].m_map_nameInfo��fileLen = %d\n", fileLen);
	for(int i = 0; i < fileLen; ++i)
	{
		//�ж�ini�ļ��Ƿ��и�TXT�ļ����ͣ�����У����ܿ�ʼ�����ļ���
		//֮ǰ��ini��ȡ�������ļ���������Ϣm_vecFilesInfo
		//printf("clw��m_vecFilesInfo[i].m_map_nameInfo['filetype'] = %s\n", m_vecFilesInfo[i].m_map_nameInfo["filetype"].c_str());
		if(m_vecFilesInfo[i].m_map_nameInfo["filetype"] == fileType)
		{
			bFindFileName = true;
			m_vecFilesInfoIndex.push_back(i); //��¼���ļ���Ӧ���ļ��ṹ�������ţ������Ժ�ʹ��
			//break;
		}		
	}
	if(!bFindFileName)
	{
		//�������ļ�����m_vecFilesInfo���Ҳ�������ôֱ�ӷ���false
		printf("clw��ini�ļ���û��%s�ļ����͵�������Ϣ���޷����н���\n", fileType.c_str());
		string strLogInfo = "clw��ini�ļ���û��" +  fileType + "�ļ����͵�������Ϣ���޷����н���\n";
		WriteLogToFile(LOGFILENAME, strLogInfo.c_str());
		return false;
	}

	//���ļ�
	FILE* fp = fopen(filepath, "r");
	if(fp == NULL)
	{
		printf("clw�����ļ�%sʧ�ܣ�\n", filepath);
		string strLogInfo = "clw�����ļ�" +  string(filepath) + "ʧ�ܣ�\n";
		WriteLogToFile(LOGFILENAME, strLogInfo.c_str());
		return false;
	}

	/*
	* int fseek(FILE *stream, long offset, int fromwhere);���������ļ�ָ��stream��λ�á�
	* ���ִ�гɹ���stream��ָ����fromwhereΪ��׼��ƫ��offset(ָ��ƫ����)���ֽڵ�λ�ã���������0��
	* ���ִ��ʧ��(����offsetȡֵ���ڵ���2*1024*1024*1024����long��������Χ2G)���򲻸ı�streamָ���λ�ã���������һ����0ֵ��
	* fseek������lseek�������ƣ���lseek���ص���һ��off_t��ֵ����fseek���ص���һ�����͡�
	*/
	fseek(fp, 0, SEEK_END); //���ļ�ָ��ָ���ļ�β
	int len = ftell(fp);
	fseek(fp, 0, SEEK_SET); //���ļ�ָ�뻹ԭ���ļ���ͷ


	//Ҫ��֤��'\0'��β�����Զ��������һ���ֽڣ���len+1�������ݵ�ʱ��ֻ��len���ֽڡ�
	char *pTxtBuf = (char*)malloc((len + 1) * sizeof(char));  //�����malloc������ڴ����Խ����ʾ�Ȼû����c���ԣ�c/c++���Ե�Σ�վ����⡣��ֻҪ��ȥ������ϵͳ�������ڴ棬����Ͳ�������
	//��Щʱ���ڴ�Խ�绹��Ӱ��������̣�����forѭ�����Ʊ���Ϊi����forѭ���ڲ������ڴ����룬������Ʋ��þ��п���������޸�i��ֵ������ѭ�������д������������forѭ������ѭ����
	memset(pTxtBuf, 0, len + 1); 
	fread(pTxtBuf, sizeof(char), len, fp); //fread��һ�������������ļ����ж����ݣ�����ȡcount���ÿ����size���ֽڣ�������óɹ�����ʵ�ʶ�ȡ�����������С�ڻ����count����������ɹ�������ļ�ĩβ���� 0��


	//����m_map_nameVals
	//int len = m_vecFilesInfo[nFilesInfoIndex].m_map_nameVals.size();
	int nVecFilelen = m_vecFilesInfoIndex.size();
	for(int i = 0; i < nVecFilelen; ++i)
	{
		for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin(); 
			iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
		{
			char strRecordID[BUF_LEN_MIN] = {0};
			//if(fileType == FILETYPE_SHORT || fileType == FILETYPE_ULTRA)
			//	strncpy(strRecordID, iter->second[2].c_str(), BUF_LEN_MIN - 1);//��#3��������ֻ����3
			//else if(fileType == FILETYPE_AGCRN || 
			//	fileType == FILETYPE_AGCRQ || 
			//	fileType == FILETYPE_XNYRN ||
			//	fileType == FILETYPE_XNYRQ)
			strncpy(strRecordID, iter->second[1].c_str(), BUF_LEN_MIN - 1);//��#3��������ֻ����3

			printf("clw: strRecordID = %s\n", strRecordID);
			//printf("clw:strRecordID = %s\n", strRecordID);
			int nRecordID = atoi(strRecordID); 
			printf("clw: nRecordID = %d\n", nRecordID);
			if(nRecordID > INT_MAX)
			{
				//error1
				printf("clw��strRecordIDֵ�����ޣ�\n");
				WriteLogToFile(LOGFILENAME, "clw��strRecordIDֵ�����ޣ�\n");
				free(pTxtBuf);
				fclose(fp);
				return false;
			}

			//==================================================================================
			//��Ҫ���ܣ��ҵ�nRecordId��Ӧ����
			//��ͬTXT�ļ���ʽ��ͬ����1���ո��2���ո��Tab�������Ƕ��ڵ�3�е�λ����һ��ģ�壬
			//��@��ͷ����¼���ļ���ʽ����@ id name  p0015 q0015 p0030 q0030 p0045 q0045 p0100 
			//����Ӳ����������̶�ĳ�ָ�ʽ��TXT�ļ�
			//�հ�����ʱ��Ҫ������1���ո��2���ո��Tab��ò��û��ȫ�ǿո�ĸ��
			//ֻ��ȫ�Ƕ��ţ������������ƣ�ռ�����ַ���Ҫ��wchar_t
			//Ҳ���ͨ��ͳ��#�ŵ�������ȷ���ǵڼ�����¼�����ǲ��У����������/220kV.4#����-�ߡ���
			//����Ҳ����ͳ��#��Ȼ���ų�#ǰ�治�ǻ��з��ġ�����
			//==================================================================================

			//����һ���ҵ�@

			//��������#����ͳ�Ʒ���ֻͳ��λ�ڵ�1�е�#����)
			char* pWellSign = pTxtBuf;  //��λ�����¿�ʼ
			//printf("clw: nRecordID = %d\n", nRecordID);
			for(int i = 0; i < nRecordID; ++i)
			{
				pWellSign = strstr(pWellSign + 1, "#");
				//printf("clw��*(pWellSign+1) = %d\n", *(pWellSign+1));
				//printf("clw��*(pWellSign-1) = %d\n", *(pWellSign-1));

				if(*(pWellSign-1) != '\n') //λ�ڵ�1�е�#�������ǣ�ǰһ���ַ���'\n'������һ��
				{
					i--;
					continue;
				}
			}

			//�ж�#������ֵĵ�1�������Ƿ���nRecordID��������ǣ����Ҳ���id
			char* pBegin = pWellSign;
			pBegin++;
			while(/*!isdigit(*pBegin)*/ *pBegin < DIGIT_ASCII_MIN || *pBegin > DIGIT_ASCII_MAX) //������
			{
				if(*pBegin != ' ' && *pBegin != '\t')
				{
					//error2
					printf("clw��*pBegin = %d\n", *pBegin);
					printf("clw��TXT��#id��ʽ����ȷ�������ʽ��\n");
					printf("clw��*pBegin = %d\n", *pBegin);
					printf("clw��*(pBegin+1) = %d\n", *(pBegin+1));
					printf("clw��*(pBegin-1) = %d\n", *(pBegin - 1));
					WriteLogToFile(LOGFILENAME, "clw��TXT��#id��ʽ����ȷ�������ʽ��\n");
					free(pTxtBuf);
					fclose(fp);
					return false;
				}
				pBegin++;
			}

			//�������¼��idֵ������#123����ônum = ((0*10+1)*10+2)*10+3
			int num = 0;
			while(*pBegin >= DIGIT_ASCII_MIN && *pBegin <= DIGIT_ASCII_MAX) //������
			{
				num = num * 10 + *pBegin - '0';
				pBegin++;
			}

			if(num != nRecordID)
			{
				//error3
				printf("������ļ�¼idֵΪnum = %d����nRecordID = %d���ȣ�\n", num, nRecordID);
				printf("clw��TXT�Ҳ���#id�ļ�¼����ini�м�¼������\n");
				WriteLogToFile(LOGFILENAME, "clw��TXT�Ҳ���#id�ļ�¼����ini�м�¼������\n");
				free(pTxtBuf);
				fclose(fp);
				return false;
			}
			//==================================================================================

			//��������Ϣд���ڴ�
			char *pEnd = strchr(pBegin, '\n');
			char *pRecordBuf = (char *)malloc((pEnd - pBegin + 2) * sizeof(char));
			memset(pRecordBuf, 0, pEnd - pBegin + 2); //clw add  clw note��20190604��������Ҫд��������涼�������ݣ�������Ҫ��֤��'\0'��β�����Զ��������һ���ֽڣ���pEnd- pBegin +1���ֵ��1
			strncpy(pRecordBuf, pBegin, pEnd - pBegin + 1);  //clw note��pRecordBuf[pEnd - pBegin]��Ӧ'\n'
			//printf("clw����ӡTXT�ļ�ÿһ�е�����%s\n", pRecordBuf);


			//��ȡ���и����й�Ԥ��ֵ��96��
			//��������ָ��������ַ���
			char *pCur = pRecordBuf;
			char *pPre = pCur;

			//��ʼȡ96��
			int count = 0;
			printf("clw note����ʼȡ96�����ݣ�\n");
			while(1) 
			{
				//clw add 20190604:����96�����ݾͷ��أ���Ϊ��Щ��96����󲢲�һ����\n�����п��������룬�������������
				/*����.ʯ��ɽ�糧/220kV.0��#������-�ߵ�ֵ����	0.3935  0	0.3935  0	0.3935  0	0.3935  0	
				0.3871  0	0.3808  0	0.3808  0	0.4217  0	0.4465  0	0.4636  0	0.4399  0	0.4001  0 .....
				0	q*/
				//if(m_vecShortP.size() == 96)
				//	break;

				//ע�⣺Ӧ���ǰѿո�Tab��strchr��һ�飬�ĸ�����ǰ�棬���ȴ����ĸ�
				//���ǻ���2���ո�����������Ҫ����
				//������һ���հ�
				char* pSpace = strchr(pCur, ' ');
				char* pTab = strchr(pCur, '\t');

				if(pSpace > pEnd || pTab > pEnd)  //˵���Ѿ���ȡ��ϣ�����
					break; 

				if(pSpace == NULL && pTab == NULL)  //�Ѿ��Ҳ����հף�������
				{
					if(count == 95) //���һ��ֵ����û�пո����Ҫ���⴦����'\n'
					{
						pCur = strchr(pCur, '\n');
					}
					else
					{
						break;
					}
				}
				else if((pSpace == NULL && pTab != NULL) || (pSpace != NULL && pTab == NULL)) //���һ���ǿ�һ��Ϊ�գ�ȡ�ǿյ��Ǹ�
					pCur = max(pSpace, pTab);
				else //����Ϊ�գ�ȡ��С
					pCur = min(pSpace, pTab);


				//���Ppre���������֣��Ͱ�pPre��pCur�м������ȡ������count+1��Ȼ�����������
				//��Ҫ��isdigit()����Ϊ��������ģ��п���a=-79��������isdigit��崵�
				if(/*isdigit(*(pPre + 1))*/ (*(pPre + 1) >= DIGIT_ASCII_MIN && *(pPre + 1) <= DIGIT_ASCII_MAX || *(pPre + 1) == '-'))  
				{
					//���ڸ���Ԥ��ı�����޹����ݣ���Ҫ������ֻȡ�й�
					if(strcmp(fileType.c_str(), FILETYPE_SHORT) == 0 || 
						strcmp(fileType.c_str(), FILETYPE_AGCRQ) == 0)
					{
						if(count % 2 == 0) //ֻ����count=ż�������й��������ݣ������޹�����Ԥ������
						{
							char buf[16] = {0};
							//strncpy(buf, pPre, pCur - pPre);
							strncpy(buf, pPre + 1, pCur - pPre - 1);
							//printf("clw����ȡÿ��record��buf��ֵΪ%s\n", buf);
							float P = atof(buf);
							//printf("clw��atofת������P = %f\n", P);  //С�������6λ
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
					pCur++; //���������ҵ���һ���ո񣬷�����ѭ���ڵ�ǰ�ո񡣡�
					count++;
					continue;
				}
				pPre = pCur;
				pCur++; //���������ҵ���һ���ո񣬷�����ѭ���ڵ�ǰ�ո񡣡�
			}

			//clw note����ȡ96��������ж�vec�Ƿ������96��ֵ
			if(m_vecShortP.size() != 96)
			{
				printf("clw�������й�Ԥ������TXT�ļ���%d�㣬����96�㣬�������ݣ�\n", m_vecShortP.size());
				int len = m_vecShortP.size();
				for(int i=0;i<len;++i)
					printf("%lf ", m_vecShortP[i]);
				printf("\n");
				WriteLogToFile(LOGFILENAME, "clw�������й�Ԥ������TXT�ļ�����96�㣬�������ݣ�\n");

				memset(pBegin, 0, pEnd - pBegin + 2);//��Ϊ�´�forѭ����Ҫ����malloc����������ڴ�ռ䣬�����ͷ�֮ǰ�������գ����������������ݣ�
				//����ֻҪ�������ڴ�֮��memset��Ȼ������������ڴ��ʱ��ע��Ӹ��жϣ���Ҫ��Խ��Ϳ����ˡ�
				free(pRecordBuf); 
				return false;
			}
			m_vecStationShortPs.push_back(m_vecShortP);
			vector<float>().swap(m_vecShortP);

			//�ͷ�Ϊÿ����¼������ڴ�
			if(pRecordBuf!=NULL)
			{
				free(pRecordBuf);
				pRecordBuf = NULL;
			}
		} //for end
	}

	//�ͷ������ļ���Ӧ��str���ڴ�
	if(pTxtBuf != NULL)
	{
		free(pTxtBuf);
		pTxtBuf = NULL;
	}

	//�ر��ļ�
	fclose(fp);

	printf("clw�������й�Ԥ������TXT�ļ���ȡ��ϣ�׼��дmysql����\n");
	WriteLogToFile(LOGFILENAME, "clw�������й�Ԥ������TXT�ļ���ȡ��ϣ�׼��дmysql����\n");
	return true;
}

bool FileParser::WriteFileInfoToDB()
{
	bool bSuc = true;

	if(m_strForcastTime.isEmpty())
	{
		printf("clw���������ļ���ʼԤ��ʱ���ȡʧ�ܣ������ļ����»��߸�ʽ��\n");
		WriteLogToFile(LOGFILENAME, "clw���������ļ���ʼԤ��ʱ���ȡʧ�ܣ������ļ����»��߸�ʽ��");
		return false;
	}
	if(m_vecStationShortPs.empty())
	{
		printf("clw��δ��ȡ��96�����ݣ��޷�д���ݿ⣡\n");
		WriteLogToFile(LOGFILENAME, "clw��δ��ȡ��96�����ݣ��޷�д���ݿ⣡\n");
		return false;
	}


	//==============================================================================================
	//��QT�������������ݵ�mysql�����Σ�֮ǰ��׼������
	//�����ж��ļ����ͣ����ݲ�ͬ���͵õ�96��ʱ�䣬�õ����Ե�SQL���
	//==============================================================================================
	//QString strSqlHeader = "";
	string fileType = m_vecFilesInfo[m_vecFilesInfoIndex[0]].m_map_nameInfo["filetype"];
	QDateTime curTime = QDateTime::currentDateTime();  //��ȡ��ǰʱ��
	QString curYear = curTime.toString("yyyy"); //��ǰʱ��ת��Ϊ�ַ������ͣ���ʽΪ��������ʱ���룬��2019-05-30 14:28:15

	//���ݲ�ͬ�ļ����͵ĵ���ͬ��96��ʱ��
	//����m_strForcastTimeʱ����201905231415��˵���ļ�������FILETYPE_ULTRA���������ڸ���Ԥ��,��96��ĵ�һ����¼Ԥ��ʱ��ӦΪ2019-05-23 14:15:00
	//    m_strForcastTimeʱ����20190524��˵���ļ�������FILETYPE_SHORT�������ڸ���Ԥ�⣬��96��ĵ�һ����¼Ԥ��ʱ��ӦΪ2019-06-03 00:00:00
	//14:15:00��ͨ�������m_strForcastTime�����ݶ�ȡȻ��������õ���
	vector<string> vecForcastTime; //clw note���䳤��������string�ˣ�֮��96�㴦������c_str()ת��char*
	QString curDateTime = curTime.toString("yyyy-MM-dd hh:mm:ss"); //��ǰʱ��ת��Ϊ�ַ������ͣ���ʽΪ��������ʱ���룬��2019-05-30 14:28:15
	QDateTime forcastTime;
	if(fileType == FILETYPE_SHORT || fileType == FILETYPE_AGCRQ)
	{
		//forcastTime = curTime.addDays(1);//��ǰʱ��+1�죬֮����õ����������
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
	//96��ʱ��д��vec
	QString firstPointTime = m_strForcastTime;//�ַ�����ʽ����2019-05-30 00:00:00
	for(int i = 0; i < 96; ++i)
	{
		QDateTime tempTime(forcastTime.addSecs(60 * 15 * i));
		firstPointTime = tempTime.toString("yyyy-MM-dd hh:mm:ss");
		vecForcastTime.push_back(firstPointTime.toStdString());
	}

	//���ݲ�ͬ�ļ����ͣ����ɸ��Ե�sql���
	QString sqlToExec;
	char strSQL[1024];
	QStringList lstSql;
	string tableName;
	/*����ini�ļ�����ֵ���ͼ�¼����
	station1=1,#3;
	station2=3,#6;
	station3=5,#8;
	station4=7,#9;
	��4����¼��õ�96*4 = 384���㣬��Ӧmysql��Ӧ���384����¼
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

					for(int i = 0; i < 96 /*96��1������*/; ++i)
					{
						sprintf(strSQL, strFormat, nCtrlID , vecForcastTime[i].c_str(), curDateTime.toLatin1().data(), 0/*method����ʱд0*/, m_vecStationShortPs[count][i]);
						lstSql.append(strSQL);
					}

					if(count != 0)
						sqlToExec += ",";  //��һ������ǰ�治�üӶ���
					count++;
					sqlToExec += lstSql.join(",");
					lstSql.clear();
				}
				//ִ��sql��дʵʱ��
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
						sqlToExec += ",";  //��һ������ǰ�治�üӶ���
					count++;
					sqlToExec += lstSql.join(",");
					lstSql.clear();
				}
				//ִ��sql��дʵʱ��
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

					for(int i = 0; i < 96 /*96��1������*/; ++i)
					{
						sprintf(strSQL, strFormat, nCtrlID , vecForcastTime[i].c_str(), curDateTime.toLatin1().data(), 0/*method����ʱд0*/, m_vecStationShortPs[count][i]);
						lstSql.append(strSQL);
					}

					if(count != 0)
						sqlToExec += ",";  //��һ������ǰ�治�üӶ���
					count++;
					sqlToExec += lstSql.join(",");
					lstSql.clear();
				}
				//ִ��sql��дʵʱ��
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
						sqlToExec += ",";  //��һ������ǰ�治�üӶ���
					count++;
					sqlToExec += lstSql.join(",");
					lstSql.clear();
				}
				//ִ��sql��дʵʱ��
				bSuc = ExecSQL(sqlToExec);
				count = 0;
			}
		} //for end
	} // if end


	//TODO�����������ļ����������䣻ÿ�����͵�ʱ��������Ҫ�޸ģ�����5���ӣ�15���ӵȣ�������48���96�㣻
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
					sqlToExec += ",";  //��һ������ǰ�治�üӶ���
				count++;
				sqlToExec += lstSql.join(",");
				lstSql.clear();
			}
			//ִ��sql��дʵʱ��
			bool bSuc1 = ExecSQL(sqlToExec);
			printf("clw��׼��дʵʱ������ÿ⣬nID = %d\n", nID);
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
				strID = iter->second[0]; //TODO����һ���Ǹ��ɣ�������������Դ����ƻ�������������
				nID = atoi(strID.c_str());
				for(int i = 0; i < 96; ++i)
				{
					sprintf(strSQL, strFormat, nID, vecForcastTime[i].c_str(), i, m_vecStationShortPs[count][i], m_vecStationShortPs[count][i], m_vecStationShortPs[count][i]);
					lstSql.append(strSQL);
				}

				if(count != 0)
					sqlToExec += ",";  //��һ������ǰ�治�üӶ���
				count++;
				sqlToExec += lstSql.join(",");
				lstSql.clear();
			}
			//ִ��sql��дʵʱ��
			bool bSuc1 = ExecSQL(sqlToExec);
			printf("clw��׼��дʵʱ������ÿ⣬nID = %d\n", nID);
			bool bSuc2 = WriteRtdbAndConfigDB(fileType, nID);
			bSuc = bSuc1 && bSuc2;
			count = 0;
		}
	}

	else
	{
		printf("clw��û�д��ļ����ͣ�дMySQLʧ�ܣ�\n");
		WriteLogToFile(LOGFILENAME, "clw��û�д��ļ����ͣ�дMySQLʧ�ܣ�\n");
		return false;
	}
	//==============================================================================================


	return bSuc;
}

bool FileParser::ExecSQL(const QString& sqlToExec)
{
	//==============================================================================================
	//clw note��ִ��sql���
	//TODO�����ִ��ʧ�ܣ��ж��Ƿ����ݿ�Ͽ�����������
	//������������ϣ�Ӧ���Ȱ�sql�Զ����Ƹ�ʽ��.DAT�����ڴ�����
	//==============================================================================================
	QSqlQuery query = QSqlQuery(g_pDB_His1);
	if(!query.exec(sqlToExec))
	{
		printf("ִ��mysqlʧ��: %s \n", sqlToExec.toLatin1().data());
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
		printf("clw��ִ��mysql�ɹ���\n");
		WriteLogToFile(LOGFILENAME, "ִ��mysql�ɹ���\n");
	}
	return true;
	//==============================================================================================
}

bool FileParser::WriteRtdbAndConfigDB(const string& fileType, const int& nID)
{
	//==============================================================================================
	//дʵʱ���
	//����wx�ӿڣ�дʵʱ��stationdispc����վ���ȿ�����Ϣ����
	//AGC��ǰ�ƻ��������ʱ�� �� AGC���������ڣ��ƻ��������ʱ��
	//==============================================================================================
	icsvariant fldValue;
	fldValue.type = ft_time;
	fldValue.value.t = (double)CDateTime::GetCurrentDateTime();
	fldValue.isNull = false;
	if(fileType == FILETYPE_AGCRN)
	{
		printf("clw��׼��д��ʵʱ��stationdispc��\n");
		WriteLogToFile(LOGFILENAME, "clw��׼��д��ʵʱ��stationdispc��\n");
		if(m_pUpdateRealACfg->UpdateDataInReal("stationdispc", "todayagctime", nID, fldValue))
		{
			printf("clw��д��ʵʱ��stationdispc��todayagctime�ֶγɹ���\n");
			WriteLogToFile(LOGFILENAME, "clw��д��ʵʱ��stationdispc��todayagctime�ֶγɹ���\n");
		}
		else
		{
			printf("clw��д��ʵʱ��stationdispc��todayagctime�ֶ�ʧ�ܣ�\n");
			WriteLogToFile(LOGFILENAME, "clw��д��ʵʱ��stationdispc��todayagctime�ֶ�ʧ�ܣ�\n");
			return false;
		}

		printf("clw��׼��д�����ÿ�stationdispc��\n");
		WriteLogToFile(LOGFILENAME, "clw��׼��д���ÿ�stationdispc��\n");
		if(m_pUpdateRealACfg->UpdateDataInCfg("stationdispc", "todayagctime", nID, fldValue))
		{
			printf("clw��д�����ÿ�stationdispc��todayagctime�ֶγɹ���\n");
			WriteLogToFile(LOGFILENAME, "clw��д�����ÿ�stationdispc��todayagctime�ֶγɹ���\n");
		}
		else
		{
			printf("clw��д�����ÿ�stationdispc��todayagctime�ֶ�ʧ�ܣ�\n");
			WriteLogToFile(LOGFILENAME, "clw��д�����ÿ�stationdispc��todayagctime�ֶ�ʧ�ܣ�\n");
			return false;
		}
	}
	else if(fileType == FILETYPE_AGCRQ)
	{
		printf("clw��׼��д��ʵʱ��stationdispc��\n");
		WriteLogToFile(LOGFILENAME, "clw��׼��д��ʵʱ��stationdispc��\n");
		if(m_pUpdateRealACfg->UpdateDataInReal("stationdispc", "nextdayagctime", nID, fldValue))
		{
			printf("clw��д��ʵʱ��stationdispc��nextdayagctime�ֶγɹ���\n");
			WriteLogToFile(LOGFILENAME, "clw��д��ʵʱ��stationdispc��nextdayagctime�ֶγɹ���\n");
		}
		else
		{
			printf("clw��д��ʵʱ��stationdispc��nextdayagctime�ֶ�ʧ�ܣ�\n");
			WriteLogToFile(LOGFILENAME, "clw��д��ʵʱ��stationdispc��nextdayagctime�ֶ�ʧ�ܣ�\n");
			return false;
		}

		printf("clw��׼��д�����ÿ�stationdispc��\n");
		WriteLogToFile(LOGFILENAME, "clw��׼��д���ÿ�stationdispc��\n");
		if(m_pUpdateRealACfg->UpdateDataInCfg("stationdispc", "nextdayagctime", nID, fldValue))
		{
			printf("clw��д�����ÿ�stationdispc��nextdayagctime�ֶγɹ���\n");
			WriteLogToFile(LOGFILENAME, "clw��д�����ÿ�stationdispc��nextdayagctime�ֶγɹ���\n");
		}
		else
		{
			printf("clw��д�����ÿ�stationdispc��nextdayagctime�ֶ�ʧ�ܣ�\n");
			WriteLogToFile(LOGFILENAME, "clw��д�����ÿ�stationdispc��nextdayagctime�ֶ�ʧ�ܣ�\n");
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
	QString fileName = qStrList[qStrList.length() - 1]; //�ļ�������xxx.TXT

	if (srcDir == dstDir)
	{
		return true;
	}
	if (!QFile::exists(srcDir))
	{
		return false;
	}
	QDir dir;
	if (!dir.exists(dstDir)) //���Ѵ������TXTͳһ�ŵ�dstDir��
	{
		if(!dir.mkdir(dstDir)) //����ļ��в����ڣ����½��ļ���
		{
			printf("clw������·��%sʧ�ܣ����飡\n", dstDir.toLatin1().data());
			WriteLogToFile(LOGFILENAME, "clw������·��ʧ�ܣ����飡\n");
			return false;
		}
	}//end if

	//�Ƿ񸲸�
	if(coverFileIfExist)
	{
		if(dir.remove(dstDir + "/" + fileName))
		{
			QString strLogInfo = dstDir + "/" + fileName;
			printf("clw�����ھ��ļ�%s���ҳɹ�ɾ����׼��д�����ļ�\n", strLogInfo.toLatin1().data());
			WriteLogToFile(LOGFILENAME, "clw�����ھ��ļ�%s���ҳɹ�ɾ����׼��д�����ļ�\n");
		}
		//���û��ͬ���ľ��ļ�����ɾ������false����Ӱ��
	}

	//���Ƶ��µ�Ŀ¼
	if(!QFile::copy(srcDir, dstDir + "/" + fileName))
	{
		return false;
	}

	//ɾ�����ļ�
	if(!QFile::remove(srcDir))
	{
		return false;
	}

	return true;
}

void FileParser::DeleteFileByTime()
{
	//TODO������ɾ���ļ����ݶ�ɾ������ʮ����ļ�
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

		QDateTime curTime = QDateTime::currentDateTime();  //��ȡ��ǰʱ��
		QString strCurDate = curTime.toString("yyyyMMdd"); //��ǰʱ��ת��Ϊ�ַ������ͣ���ʽΪ��������ʱ���룬��2019-05-30 14:28:15
		int nCurDate = strCurDate.toInt();

		if(nCurDate - nFileTime > 10)
		{
			if(dir.remove(QString::fromStdString(vecToDeleteFiles[i])))
			{
				printf("clw������ɾ���ļ����ļ�ʱ��Ϊ%s\n", strFileTime.toLatin1().data());
				string strLogInfo = "clw������ɾ���ļ����ļ�ʱ��Ϊ" + strFileTime.toStdString();
				WriteLogToFile(LOGFILENAME, strLogInfo.c_str());
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
//4. ������־�ʹ���
void FileParser::DealWithLogs(char* logLines, bool bCout, int outFreq)
{
	if (m_circleTimes % outFreq == 1 || m_circleTimes == 0)
	{
		char logs[128];
		sprintf(logs, "��FileParser�� circleTimes=%d, ", m_circleTimes);
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
//4. ������־�ʹ���
void FileParser::DealWithLogs(string logLine, bool bCout, int outFreq)
{
	char* cLogLines = const_cast<char*>(logLine.c_str());
	if (m_circleTimes%outFreq == 1 || m_circleTimes == 0)
	{
		char logs[128];
		sprintf(logs, "��FileParser�� circleTimes=%d, ", m_circleTimes);
		strcat(logs, cLogLines);
		WriteLogToFile(LOGFILENAME, logs);
		if (bCout == true)
		{
			printf("logs:%s\n", logs);
		}
	}
}
#endif 