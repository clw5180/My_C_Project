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

extern bool IsMainRole();                     // �Ƿ�Ϊ����ɫ

FileParser* FileParser::m_pInst = 0;

FileParser::FileParser(): m_bExit(false)
{
	m_pUpdateRealACfg = (new CUpdateDt())->Self(); //�õ��ӿڶ����ͬʱ���Ѿ��Ѷ�̬����ؽ����ˣ�
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
	//�߳�������ʼ ��ֹ50ms
	msleep(1000);

	//1��������ʷ��
	SysConfig dbsys;
	if(dbsys.ConnectDatabas())	
	{
		printf("clw��esfileserver������ʷ�����ӳɹ���\n");
		WriteDailyLog(true, "clw����ʷ�����ӳɹ���\n");
	}
	else 
	{
		printf("clw��esfileserver������ʷ������ʧ�ܣ�\n");
		WriteDailyLog(true, "clw����ʷ������ʧ�ܣ�\n");
	}

	//2���������ÿ�
	if(dbsys.ConnectModelDB())	
	{
		printf("clw��esfileserver�������ÿ����ӳɹ���\n");
		WriteDailyLog(true, "clw�����ÿ����ӳɹ���\n");
	}
	else 
	{
		printf("clw��esfileserver�������ÿ�����ʧ�ܣ�\n");
		WriteDailyLog(true, "clw�����ÿ�����ʧ�ܣ�\n");
	}

	//3����esfilesetting.ini�ļ��ж���������Ϣ
	if(!ReadInConfigPara())
	{
		printf("clw��esfileserver�����ȡ������Ϣʧ�ܣ�\n");
		WriteDailyLog(true, "clw��esfileserver�����ȡ������Ϣʧ�ܣ�\n");
		return;
	}

	int count = 0;
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
			GetFiles(FILE_PATH, m_vecFiles);
		}
		int len = m_vecFiles.size();
		for(int i=0; i<len; ++i)
		{
			//����״̬�жϣ���״̬�Ž�������״̬�ƶ��������ط�
#ifndef WINDOWS_DEBUG
			if(IsMainRole())
			{
				if(g_IsDebug)
				{
					printf("clw������״̬������\n");
				}
#endif
				if(ParseESFile(m_vecFiles[i].c_str()))
				{
					//����ܹ���ȷ�����ļ����ٿ�ʼд���ݿ��
					if(WriteFileInfoToDB())
					{
						if(g_IsDebug)
						{
							printf("clw��д��ʷ���ݿ�ɹ���׼���ƶ��ļ�\n");
							WriteDailyLog(true, "clw��д��ʷ���ݿ�ɹ���׼���ƶ��ļ�\n");
						}
						//���д���ݿ�ɹ����ٽ��ļ��ƶ���ĳһ�ļ����£��൱�ڱ�ʶ���ļ����ѱ�������ȷ������ͬһ�ļ���������
						MoveFileToDst(m_vecFiles[i], FILE_PATH + string("/") + FILE_PROCESSED_FOLDER);
						
					}
					else
					{
						printf("clw��д��ʷ���ݿ�ʧ�ܣ�\n", m_vecFiles[i].c_str());
						printf("\n");
						WriteDailyLog(true, "clw��д��ʷ���ݿ�ʧ�ܣ�");
					}
				}
				else
				{
					printf("ParseESFile()�����ļ�%sʧ�ܣ��������������ļ�\n", m_vecFiles[i].c_str());
					printf("\n");
					string strLogInfo = "clw��ParseESFile()�����ļ�" +  m_vecFiles[i] + "ʧ�ܣ��������������ļ���";
					WriteDailyLog(true, strLogInfo.c_str());
				}
#ifndef WINDOWS_DEBUG
			}
			else //����״̬������
			{
				if(g_IsDebug)
				{
					WriteDailyLog(true, "clw������״̬������\n");
					printf("clw������״̬������\n");
				}
				//��������ʹ����ļ���ֱ���ƶ������Ѵ����ļ����£��൱�ڱ�ʶ���ļ����ѱ�������ȷ������ͬһ�ļ���������
				MoveFileToDst(m_vecFiles[i], FILE_PATH + string("/") + FILE_PROCESSED_FOLDER);
			}
#endif
			//ÿ�β����ܷ���ȷ�����ļ���֮��Ҫ���96�㹦��ֵvector<>��n��96�㹦��ֵ��vector<vector<>>����Ҫ�ļ�����vec
			vector<float>().swap(m_vecShortP);
			vector<vector<float> >().swap(m_vecStationShortPs);
			vector<int>().swap(m_vecFilesInfoIndex);
		} 

		//���������������ļ���vector<>����Ϊ��Щ�ļ��Ѿ����������	
		vector<string>().swap(m_vecFiles);  //����m_vecFiles.swap(vector<string>());	

		//���������ļ�
		count++;
		if(count >= m_nFileDeleteCycle * 120)  //��Ϊ�߳�30sѭ��һ�Σ���������1Сʱ��һ���Ƿ���ھ��ļ�����ô��Ӧ����count == 120����Ӧ60min
		{
			DeleteFileByTime();
			count = 0;
		}

		//�ȴ�30s��������µ��ļ����ٶ�������������
		msleep(THREAD_PERIOD_MS);
	}
}

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


bool FileParser::GetFiles(const string& path, vector<string>& vecFiles)  
{  	
	int pathLen = path.length();
	if(pathLen >= 256)
	{
		printf("clw���ļ���%s������\n", path.c_str());
		string strLogInfo = "clw���ļ���" + path + "������\n";
		WriteDailyLog(true, strLogInfo.c_str());
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
	QDir dir;
	if (!dir.exists(tempdirPath)) //���Ѵ������TXTͳһ�ŵ�dstDir��
	{
		if(!dir.mkpath(tempdirPath)) //����ļ��в����ڣ����½��ļ���
		{
			//QStringList qStrList = QString::fromStdString(filepath).split("/");
			//if(!dir.mkdir(qStrList[qStrList.length() - 1].toStdString()))
			//{
				printf("clw������·��%sʧ�ܣ����飡\n", tempdirPath.toLatin1().data());
				WriteDailyLog(true, "clw������·��ʧ�ܣ����飡\n");
				return false;
			//}
		}
	}

	/*if(!dir.exists(tempdirPath))
	{	
		printf("clw��·��%s�����ڣ�\n", path.c_str());
		string strLogInfo = "clw��·��" +  path + "�����ڣ�\n";
		WriteDailyLog(true, strLogInfo.c_str());
		return false;
	}*/

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
		WriteDailyLog(true, "clw��δ�ҵ�esfilesetting.ini�����ļ���\n");
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
	QString strRecordID = "";
	stFileInfo iniFileInfo;

	while(!t.atEnd())
	{
		//���εõ�ini�ļ���ÿһ��
		str = codec->fromUnicode(t.readLine().stripWhiteSpace());
		//printf("clw��ini�ļ��н������е�����Ϊ%s\n", str.toLatin1().data());		

		//����ǿ���
		if(str.isEmpty())
		{
			//���strFileInfo������
			if(!iniFileInfo.m_map_nameInfo.empty() && !iniFileInfo.m_map_nameVals.empty())
			{
				//��stFileInfo�������m_vecFilesInfo��
				m_vecFilesInfo.push_back(iniFileInfo);
				//���strFileInfo�ڵ�map
				iniFileInfo.m_map_nameInfo.clear();
				iniFileInfo.m_map_nameVals.clear();
			}	
			continue; //����������һ��
		}

		//====================================================================================
		//����еȺţ�˵���������׼����ȡ
		//====================================================================================
		string::size_type nEqualSignPos = str.find('='); 
		if(nEqualSignPos != string::npos) 
		{
			left = str.left(nEqualSignPos).stripWhiteSpace();  //��������Ϊfiletype������station1֮��
			right = str.mid(nEqualSignPos + 1).stripWhiteSpace(); //posλ���ڵȺţ�ȡ�Ⱥ��ұߵ���
			right.replace(";",""); 
			//��ȡright���֣���������Ϊ"1,#3"����һ����ID����������RecordID����˳��д��һ��vector
			//unsigned int nChnCommaPos = str.find(L'��'); //clw note���������������Ķ��ţ�����һ��Ҫ��˫���ţ���find�ַ�����������õ�����''Ӧ�ǿ��ַ�wchar_t�����Lǰ׺��
			string::size_type nChnCommaPos = str.find("��"); //clw note�������unsigned int����������linux��windows����429496725������linux��string::npos��size_type���ͣ��൱��long long�����Բ���д��unsigned int
			string::size_type nEngCommaPos = str.find(',');

			//����Ҳ������ţ�˵����һ���������filetype=RNMXFHYC������m_map_nameInfo
			if(nChnCommaPos == string::npos  && nEngCommaPos == string::npos)  //clw note��
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
				strRecordID = str.mid(nComma1Pos + 1).stripWhiteSpace();
				strRecordID.replace("#","");   //����ǰ��������#3;
				strRecordID.replace(";","");

				//�洢ini�����ļ���վidֵ���Ӧ��txt��¼��idֵ����station1=1��#3;
				vector<string> tmpVec;
				tmpVec.push_back(strID.toStdString());
				tmpVec.push_back(strRecordID.toStdString());
				//printf("clw���ڶ�ȡini�����ļ�ʱ, strRecordID = %s\n", strRecordID.toLatin1().data());
				if(iniFileInfo.m_map_nameVals.end() != iniFileInfo.m_map_nameVals.find(left.toStdString()))//�ظ���Ԫ��
				{
					//printf("clw������ini����ֵ������, left = %s, right = %s\n", left.toLatin1().data(), (strID + strRecordID).toLatin1().data());
					iniFileInfo.m_map_nameVals[left.toStdString()] = tmpVec;
				} 
				else //���ظ�Ԫ�أ�����Ԫ�ػ�m_map_nameVal.empty()��һ��Ԫ��
				{
					//printf("clw������ini����ֵ������, left = %s, right = %s\n", left.toLatin1().data(), (strID + strRecordID).toLatin1().data());
					iniFileInfo.m_map_nameVals.insert(pair<string, vector<string> >(left.toStdString(), tmpVec));
				}
				bSuc = true;
			}
			else
				bSuc = false;
			//====================================================================================
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
		WriteDailyLog(true, "clw����ȡesfilesetting.ini�ļ�ʧ�ܣ�δ��ȡ����ȷ����Ϣ��\n");
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
	if(g_IsDebug)
	{
		printf("clw���ļ�����%s\n", fileName.c_str());
		WriteDailyLog(true, "clw���ļ�����%s\n", fileName.c_str());
	}
	
	int nVecFilesLen = m_vecFilesInfo.size();
	//�����ж��ļ�����

	int i;
	string fileType;
	for(i=0; i < nVecFilesLen; ++i)
	{
		fileType = m_vecFilesInfo[i].m_map_nameInfo["filetype"];
		if(fileName.find(fileType) != string::npos) //�ҵ��ļ�����
		{
			//����Ԥ���ļ���ʱ��m_strForcastTime��д���ʱ��Ҫ�õ���
			//�糬����Ԥ���ļ���20190603_1540����2019.06.03 15:40:00
			//����Ԥ���ļ���20190603����2019.06.03������ת��00:00:00
			string fileFormat = m_vecFilesInfo[i].m_map_nameInfo["fileformat"];
			string::size_type nTimeHourPos = fileFormat.find("hhmm");
			string::size_type nTimeYearPos = fileFormat.find("yyyy");
			if(nTimeHourPos != string::npos) //������ҵ�Сʱ�ͷ�����Ϣ��˵��������/������Ԥ��
			{
				m_strForcastTime = QString::fromStdString(fileName.substr(nTimeYearPos, 8)) + QString::fromStdString(fileName.substr(nTimeHourPos, 4));
				
				//m_strForcastTime = QString::fromStdString(fileName.substr(nTimeLeftPos, 13));
				//m_strForcastTime.remove("_");
			}
			else if(nTimeYearPos != string::npos) //ֻ�ҵ��꣬˵������ǰ/����Ԥ��
			{
				m_strForcastTime = QString::fromStdString(fileName.substr(nTimeYearPos, 8));
			}
			if(!IsDigitStr(m_strForcastTime)) //�����ȡ����ʱ�����зǴ����ֵ����ݣ�����0190101_����abcdefgh֮��
			{
				printf("clw����ȡ����Ԥ��ʱ��Ϊ%s������esfilesetting.ini�����õ�fileformat���ļ����Ƿ�һ�£�", m_strForcastTime.toLatin1().data());
				WriteDailyLog(true, "clw����ȡ����Ԥ��ʱ��Ϊ%s������esfilesetting.ini�����õ�fileformat���ļ����Ƿ�һ�£�", m_strForcastTime.toLatin1().data());
				return false;
			}
			break;
		}
	}
	if(i == nVecFilesLen)
	{
		//�޴��ļ�����
		printf("clw��esfilesetting.ini���޴����͵��ļ����ļ���Ϊ��%s\n", fileName.c_str());
		WriteDailyLog(true, "clw��esfilesetting.ini���޴����͵��ļ����ļ���Ϊ��%s\n", fileName.c_str());
		return false;
	}

	//��¼�����ţ���Ϊ����RNMXFHYC���͵�һ���ļ�ͬʱ��Ӧ�����ڸ���Ԥ�⼰�����ڷ���Ԥ��������
	//�����ж�Ԥ��ʱ���ʽ�Ƿ���ȷ
	for(int i = 0; i < nVecFilesLen; ++i)
	{
		//�ж�ini�ļ��Ƿ��и�TXT�ļ����ͣ�����У����ܿ�ʼ�����ļ���
		//֮ǰ��ini��ȡ�������ļ���������Ϣm_vecFilesInfo
		//printf("clw��m_vecFilesInfo[i].m_map_nameInfo['filetype'] = %s\n", m_vecFilesInfo[i].m_map_nameInfo["filetype"].c_str());
		if(m_vecFilesInfo[i].m_map_nameInfo["filetype"] == fileType)
		{
			m_vecFilesInfoIndex.push_back(i); //��¼���ļ���Ӧ���ļ��ṹ�������ţ������Ժ�ʹ��
		}	

		if(fileType == FILETYPE_ULTRALOAD || fileType == FILETYPE_ULTRAXNY || fileType == FILETYPE_AGCRN) //��201906031415
		{
			if(m_strForcastTime.size() != 12)
			{
				printf("clw������esfilesetting.ini�����õ�fileformat��ʽ�Ƿ���ȷ��fileType = %s\n", fileType.c_str());
				WriteDailyLog(true, "clw������esfilesetting.ini�����õ�fileformat��ʽ�Ƿ���ȷ��fileType = %s\n", fileType.c_str());
				return false;
			}
		}

		if(fileType == FILETYPE_SHORTLOAD || fileType == FILETYPE_SHORTLOAD1 || fileType == FILETYPE_SHORTXNY || fileType == FILETYPE_AGCRQ) //��20190603
		{
			if(m_strForcastTime.size() != 8)
			{
				printf("clw������esfilesetting.ini�����õ�fileformat��ʽ�Ƿ���ȷ��\n");
				WriteDailyLog(true, "clw������esfilesetting.ini�����õ�fileformat��ʽ�Ƿ���ȷ��\n");
				return false;
			}
		}
	}

	//���ļ�
	FILE* fp = fopen(filepath, "r");
	if(fp == NULL)
	{
		printf("clw�����ļ�%sʧ�ܣ�\n", filepath);
		WriteDailyLog(true, "clw�����ļ�%sʧ�ܣ�\n", filepath);
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

			strncpy(strRecordID, iter->second[1].c_str(), BUF_LEN_MIN - 1);//��#3��������ֻ����3

			int nRecordID = atoi(strRecordID); 
			if(nRecordID > INT_MAX)
			{
				//error1
				printf("clw��strRecordIDֵ��INT_MAX���ޣ�\n");
				WriteDailyLog(true, "clw��strRecordIDֵ��INT_MAX���ޣ�\n");
				free(pTxtBuf);
				fclose(fp);
				return false;
			}

			char* pBegin = NULL;
			//��λ��txt��#id��λ�ã�ĩβ����д��pBegin��
			if(!GetTxtPositionByIni(nRecordID, pTxtBuf, &pBegin))
			{
				printf("clw����txt�л�ȡ��¼ʧ�ܣ�\n");
				WriteDailyLog(true, "clw����txt�л�ȡ��¼ʧ�ܣ�\n");
				free(pTxtBuf);
				fclose(fp);
				return false;
			}

			//��������Ϣд���ڴ�
			char *pEnd = strchr(pBegin, '\n');
			char *pRecordBuf = (char *)malloc((pEnd - pBegin + 2) * sizeof(char));
			memset(pRecordBuf, 0, pEnd - pBegin + 2); //clw add  clw note��20190604��������Ҫд��������涼�������ݣ�������Ҫ��֤��'\0'��β�����Զ��������һ���ֽڣ���pEnd- pBegin +1���ֵ��1
			strncpy(pRecordBuf, pBegin, pEnd - pBegin + 1);  //clw note��pRecordBuf[pEnd - pBegin]��Ӧ'\n'
			if(g_IsDebug)
			{
				//WriteDailyLog(true, "clw����ӡTXT�ļ�ÿһ�е�����%s\n", pRecordBuf);//��ע�������п�����Ϊ�Ų��£����𱨴�
				printf("clw����ӡTXT�ļ�ÿһ�е�����%s\n", pRecordBuf);
			}
			pEnd = strchr(pRecordBuf, '\n');

			//��ȡ���и����й�Ԥ��ֵ��96��
			//��������ָ��������ַ���
			char *pCur = pRecordBuf;
			char *pPre = pCur;

			//==========================old method==========================
			//int nPointNums;
			//if(fileType == "AGCRN") //�����AGC���ڼƻ���ȡ48��
			//{
			//	nPointNums = 48;
			//}
			//else
			//{
			//	nPointNums = 96;
			//}
			//===============================================================
			int nPointNums = atoi(m_vecFilesInfo[m_vecFilesInfoIndex[0]].m_map_nameInfo["pointnums"].c_str());  //clw modify 20190620


			//��ʼȡ48���96������
			int count = 0;
			while(1) 
			{
				//clw add 20190604:����96�����ݻ�48�����ݾͷ��أ���Ϊ��Щ��96����󲢲�һ����\n�����п��������룬�������������
				/*����.ʯ��ɽ�糧/220kV.0��#������-�ߵ�ֵ����	0.3935  0	0.3935  0	0.3935  0	0.3935  0	
				0.3871  0	0.3808  0	0.3808  0	0.4217  0	0.4465  0	0.4636  0	0.4399  0	0.4001  0 .....
				0	q*/
				if(m_vecShortP.size() == nPointNums)
					break;

				//ע�⣺Ӧ���ǰѿո�Tab��strchr��һ�飬�ĸ�����ǰ�棬���ȴ����ĸ�
				//���ǻ���2���ո�����������Ҫ����
				//������һ���հ�
				char* pSpace = strchr(pCur, ' ');
				char* pTab = strchr(pCur, '\t');

				if(pSpace > pEnd || pTab > pEnd)  //˵���Ѿ���ȡ��ϣ�����
					break; 

				if(pSpace == NULL && pTab == NULL)  //�Ѿ��Ҳ����հף�������
				{
					if(count == nPointNums - 1) //���һ��ֵ����û�пո����Ҫ���⴦����'\n'  �ڳ���95���47���ʱ��
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


				//���pPre��pCur�м���ŵ������֣��Ͱ�pPre��pCur�м������ȡ������count+1��Ȼ�����������
				//���治Ҫ��isdigit(*(pPre + 1))����Ϊ��������ģ��п���a=-79��������isdigit��崵�
				//if((*(pPre + 1) >= DIGIT_ASCII_MIN && *(pPre + 1) <= DIGIT_ASCII_MAX || *(pPre + 1) == '-')  ||
				//	 (*(pCur - 1) >= DIGIT_ASCII_MIN && *(pCur - 1) <= DIGIT_ASCII_MAX) )  //TODO����ʱ����βָ��ǰ�������ֵľ͵�����ֵ�ˣ������걸����
				
				if(IsNumber(pPre + 1, pCur - 1))
				{
					//TODO��Ŀǰ������ֻ����ǰAGC�ƻ��ı�����޹����ݣ���Ҫ������ֻȡ�й�96��
					if(strcmp(fileType.c_str(), FILETYPE_AGCRQ) == 0) 
					{
						if(count % 2 == 0) //ֻ����count=ż�������й��������ݣ������޹�����Ԥ������
						{
							char buf[16] = {0};
							strncpy(buf, pPre + 1, pCur - pPre - 1); //clw note��ע�������+1��-1
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
			} //while(1) end


			//clw note����ȡ48/96��������ж�vec�Ƿ������96��ֵ��48��ֵ
			if(m_vecShortP.size() != nPointNums)
			{
				printf("clw����ȡTXT�ļ���%d�㣬����%d�㣬�������ݣ�\n", m_vecShortP.size(), nPointNums);
				WriteDailyLog(true, "clw����ȡTXT�ļ���%d�㣬����%d�㣬�������ݣ�\n", m_vecShortP.size(), nPointNums);
				int len = m_vecShortP.size();
				for(int i=0; i<len; ++i)
				{
					printf("%lf ", m_vecShortP[i]);
					WriteDailyLog(true, "%lf ", m_vecShortP[i]);
				}
				printf("\n");
				char buf[BUF_LEN_MIN];
				sprintf(buf, "%d", m_vecShortP.size());
				string strLogInfo = "clw����ȡTXT�ļ���" + string(buf) + "�㣬����96�㣬�������ݣ�";
				WriteDailyLog(true, strLogInfo.c_str());
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

	if(g_IsDebug)
	{
		printf("clw���й�Ԥ������TXT�ļ���ȡ��ϣ�׼��дmysql����\n");
		WriteDailyLog(true, "clw���й�Ԥ������TXT�ļ���ȡ��ϣ�׼��дmysql����\n");
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
	//��

	//��������#����ͳ�Ʒ���ֻͳ��λ�ڵ�1�е�#����)
	//char* pWellSign = pTxtBuf;  //��λ�����¿�ʼ
	//printf("clw: nRecordID = %d\n", nRecordID);
	//for(int i = 0; i < nRecordID; ++i)
	//{
	//	pWellSign = strstr(pWellSign + 1, "#");
	//	//printf("clw��*(pWellSign+1) = %d\n", *(pWellSign+1));
	//	//printf("clw��*(pWellSign-1) = %d\n", *(pWellSign-1));
	//	if(pWellSign == NULL) //����Ҫ�ҵ���#3��Ҳ���ǵ�3����¼������ʵ��txtֻ��1����¼��#��û��ô��
	//	{
	//		printf("clw��ini�ļ������õļ�¼id����TXT��¼���������޸�ini�ļ���\n");
	//		printf("\n");
	//		WriteDailyLog(true, "clw��ini�ļ������õļ�¼id����TXT��¼���������޸�ini�ļ���\n");
	//		free(pTxtBuf);
	//		fclose(fp);
	//		return false;
	//	}

	//	if(*(pWellSign-1) != '\n') //λ�ڵ�1�е�#�������ǣ�ǰһ���ַ���'\n'������һ��
	//	{
	//		i--;
	//		continue;
	//	}
	//}
	//�ж�#������ֵĵ�1�������Ƿ���nRecordID��������ǣ����Ҳ���id
	//char* pBegin = pWellSign;
	//pBegin++;
	//while(/*!isdigit(*pBegin)*/ *pBegin < DIGIT_ASCII_MIN || *pBegin > DIGIT_ASCII_MAX) //������
	//{
	//	if(*pBegin != ' ' && *pBegin != '\t')
	//	{
	//		//error2
	//		//printf("clw��*pBegin = %d\n", *pBegin);
	//		printf("clw��TXT��#id��ʽ����ȷ�������ʽ��\n");
	//		printf("\n");
	//		//printf("clw��*pBegin = %d\n", *pBegin);
	//		//printf("clw��*(pBegin+1) = %d\n", *(pBegin+1));
	//		//printf("clw��*(pBegin-1) = %d\n", *(pBegin - 1));
	//		WriteDailyLog(true, "clw��TXT��#id��ʽ����ȷ�������ʽ��\n");
	//		free(pTxtBuf);
	//		fclose(fp);
	//		return false;
	//	}
	//	pBegin++;
	//}


	//��������ֱ����#3��# 3��#  3
	char* pWellSign = pTxtBuf;  //��λ�����¿�ʼ
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
					printf("clw��txt�ļ���û��#id��Ӧ�ļ�¼��������Ӧ��txt��ini�ļ���\n");
					WriteDailyLog(true, "clw��txt�ļ���û��#id��Ӧ�ļ�¼��������Ӧ��txt��ini�ļ���\n");
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


		if(*(pWellSign-1) != '\n') //λ�ڵ�1�е�#�������ǣ�ǰһ���ַ���'\n'������һ��
		{
			pWellSign++;
		}
		else
		{
			break;
		}
	}

	//��ȡtxt�е�nRecordID
	char* pTmp = pWellSign;
	pTmp++;
	while(/*!isdigit(*pBegin)*/ *pTmp < DIGIT_ASCII_MIN || *pTmp > DIGIT_ASCII_MAX) //������
	{
		if(*pTmp != ' ' && *pTmp != '\t')
		{
			//error2
			//printf("clw��*pBegin = %d\n", *pBegin);
			printf("clw��TXT��#id��ʽ����ȷ��#����ֻ���ǿո��Tab�����֣�\n");
			//printf("clw��*pBegin = %d\n", *pBegin);
			//printf("clw��*(pBegin+1) = %d\n", *(pBegin+1));
			//printf("clw��*(pBegin-1) = %d\n", *(pBegin - 1));
			WriteDailyLog(true, "clw��TXT��#id��ʽ����ȷ��#����ֻ���ǿո��Tab�����֣�\n");
			return false;
		}
		pTmp++;
	}
	//�������¼��idֵ������#123����ônum = ((0*10+1)*10+2)*10+3
	int num = 0;
	while(*pTmp >= DIGIT_ASCII_MIN && *pTmp <= DIGIT_ASCII_MAX) //������
	{
		num = num * 10 + *pTmp - '0';
		pTmp++;
	}
	if(num != nRecordID) //�ж�ini�еļ�¼id��txt���ҵ��ļ�¼id�Ƿ���ͬ
	{
		//error3
		printf("clw��txt�еļ�¼idֵΪnum = %d����ini��nRecordID = %d���ȣ�\n", num, nRecordID);
		WriteDailyLog(true, "clw��txt�еļ�¼idֵΪnum = %d����ini��nRecordID = %d���ȣ�\n", num, nRecordID);
		return false;
	}
	*pBegin = pTmp;
	return true;

	//==================================================================================
}

//TODO����ʱ��û�ж����ֺ����ԣ�����00.0������011֮��
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
		printf("clw���������ļ���ʼԤ��ʱ���ȡʧ�ܣ������ļ����»��߸�ʽ��\n");
		WriteDailyLog(true, "clw���������ļ���ʼԤ��ʱ���ȡʧ�ܣ������ļ����»��߸�ʽ��");
		return false;
	}
	if(m_vecStationShortPs.empty())
	{
		printf("clw��δ��ȡ��96�����ݣ��޷�д���ݿ⣡\n");
		WriteDailyLog(true, "clw��δ��ȡ��96�����ݣ��޷�д���ݿ⣡\n");
		return false;
	}

	//==============================================================================================
	//�����������ݵ�mysql/����
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
		WriteDailyLog(true, "clw���Ҳ������ļ����Ͷ�Ӧ��������");//����֮ǰ˵RNMXFHYC�п��ܼȴ������ڸ���Ԥ�⣬Ҳ�������ڷ���Ԥ�⣬��ʱ�������������...
		return false;
	}

	//���ݲ�ͬ�ļ����ͣ����ɸ��Ե�sql���
	char strSQL[65536]; //TODO��ע��С�˴治�£����ٵ��У�96�� * 60ÿ������ * 4����plan��station1��2��3��4������¼
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
		printf("clw��û�д��ļ����ͣ�дMySQLʧ�ܣ�\n");
		WriteDailyLog(true, "clw��û�д��ļ����ͣ�дMySQLʧ�ܣ�\n");
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
		printf("clw���ļ�����Ϊ����ĸ�߸���Ԥ��RNMXFHYC�������Ͷ�Ӧ��table���� = %d\n", len);
		WriteDailyLog(true, "clw���ļ�����Ϊ����ĸ�߸���Ԥ��RNMXFHYC�������Ͷ�Ӧ��table���� = %d\n", len);
	}
	if(len <= 0)
	{
		WriteDailyLog(true, "clw���Ҳ��������ļ���Ϣ�����飡");
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

		for(map<string, vector<string> >::iterator iter = m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.begin();  //���ini��ͬһ�������˶�����¼����1,#1��2,#2
			iter !=  m_vecFilesInfo[m_vecFilesInfoIndex[i]].m_map_nameVals.end(); ++iter)
		{

			string strID = iter->second[0];
			int nCtrlID = atoi(strID.c_str());
			for(int i = 0; i < nPointNums /*96��1������*/; ++i)
			{
				sprintf(strSQL, strFormat, nCtrlID , vecForcastTime[i].c_str(), QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLatin1().data(), 0/*method����ʱд0*/, m_vecStationShortPs[index][i]);
				lstSql.append(strSQL);
			}

			if(count != 0)
				sqlToExec += ",";  //��һ������ǰ�治�üӶ���
			count++;
			index++;
			sqlToExec += lstSql.join(",");
			lstSql.clear();
		}
		//ִ��sql
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
		printf("clw���ļ�����Ϊ��ǰĸ�߸���Ԥ��RQMXFHYC�������Ͷ�Ӧ��table���� = %d\n", len);
		WriteDailyLog(true, "clw���ļ�����Ϊ��ǰĸ�߸���Ԥ��RQMXFHYC�������Ͷ�Ӧ��table���� = %d\n", len);
	}
	if(len <= 0)
	{
		WriteDailyLog(true, "clw���Ҳ��������ļ���Ϣ�����飡");
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

			for(int i = 0; i < nPointNums /*96��1������*/; ++i)
			{
				sprintf(strSQL, strFormat, nID , vecForcastTime[i].c_str(), QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toLatin1().data(), 0/*method����ʱд0*/, m_vecStationShortPs[index][i]);
				lstSql.append(strSQL);
			}

			if(count != 0)
				sqlToExec += ",";  //��һ������ǰ�治�üӶ���
			count++;
			index++;
			sqlToExec += lstSql.join(",");
			lstSql.clear();
		}
		//ִ��sql
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
		printf("clw���ļ�����Ϊ���ڷ�繦��Ԥ��RNFDGLYC�������Ͷ�Ӧ��table���� = %d\n", len);
		WriteDailyLog(true, "clw���ļ�����Ϊ���ڷ�繦��Ԥ��RNFDGLYC�������Ͷ�Ӧ��table���� = %d\n", len);
	}
	if(len <= 0)
	{
		WriteDailyLog(true, "clw���Ҳ��������ļ���Ϣ�����飡");
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
				sqlToExec += ",";  //��һ������ǰ�治�üӶ���
			count++;
			index++;
			sqlToExec += lstSql.join(",");
			lstSql.clear();
		}
		//ִ��sql
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
		printf("clw���ļ�����Ϊ��ǰ��繦��Ԥ��RQFDGLYC�������Ͷ�Ӧ��table���� = %d\n", len);
		WriteDailyLog(true, "clw���ļ�����Ϊ��ǰ��繦��Ԥ��RQFDGLYC�������Ͷ�Ӧ��table���� = %d\n", len);
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
				sqlToExec += ",";  //��һ������ǰ�治�üӶ���
			count++;
			index++;
			sqlToExec += lstSql.join(",");
			lstSql.clear();
		}
		//ִ��sql
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
		printf("clw���ļ�����ΪAGC���ڵ��ȼƻ�RNPLANYC�������Ͷ�Ӧ��table���� = %d\n", len);
		WriteDailyLog(true, "clw���ļ�����ΪAGC���ڵ��ȼƻ�RNPLANYC�������Ͷ�Ӧ��table���� = %d\n", len);
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
				sqlToExec += ",";  //��һ������ǰ�治�üӶ���
			count++;
			index++;
			sqlToExec += lstSql.join(",");
			lstSql.clear();

			//дʵʱ��
#ifndef WINDOWS_DEBUG
			bSuc2 = (bSuc2 && WriteRtdbAndConfigDB(fileType, strID)); //clw note��ֻ����mysqlʱ��дʵʱ����true����	
#else
			bSuc2 = true;
#endif
		
		}
		//ִ��sql
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
		printf("clw���ļ�����ΪAGC��ǰ���ȼƻ�DQPLANYC�������Ͷ�Ӧ��table���� = %d\n", len);
		WriteDailyLog(true, "clw���ļ�����ΪAGC��ǰ���ȼƻ�DQPLANYC�������Ͷ�Ӧ��table���� = %d\n", len);
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
				sqlToExec += ",";  //��һ������ǰ�治�üӶ���
			count++;
			index++;
			sqlToExec += lstSql.join(",");
			lstSql.clear();

			//дʵʱ��\	
			//printf("clw��fileType = %s\n", fileType.c_str());
			//printf("clw��strID = %s\n", strID.c_str());
#ifndef WINDOWS_DEBUG	
			bSuc2 = (bSuc2 && WriteRtdbAndConfigDB(fileType, strID));  //clw note��ֻ����mysqlʱ��дʵʱ����true����	
#else
			bSuc2 = true;
#endif
		}
		//ִ��sql
		bool bSuc1 = ExecSQL(sqlToExec);
		count = 0;

		if(!(bSuc1 && bSuc2))
			return false;
	}
	return true;
}

vector<string> FileParser::GetForcastTimes(const string& fileType, int nTimeInterval, int nPointNums)
{
	//����m_strForcastTimeʱ����201905231415��˵���ļ�������FILETYPE_ULTRALOAD���������ڸ���Ԥ��,��96��ĵ�һ����¼Ԥ��ʱ��ӦΪ2019-05-23 14:15:00
	//    m_strForcastTimeʱ����20190524��˵���ļ�������FILETYPE_SHORTLOAD�������ڸ���Ԥ�⣬��96��ĵ�һ����¼Ԥ��ʱ��ӦΪ2019-06-03 00:00:00

	//14:15:00��ͨ�������m_strForcastTime�����ݶ�ȡȻ��������õ���
	vector<string> vecForcastTime; //clw note���䳤��������string�ˣ�֮��96�㴦������c_str()ת��char*
	QDateTime forcastTime;
	QString firstPointTime; //׼������ƻ����ߵ�һ�����ʱ�䣬��2019-05-30 00:00:00

	//����Ԥ�����ǰ�ƻ�����֯ʱ���ʽΪ����ʱ��+00:00:00
	if(fileType == FILETYPE_SHORTLOAD || fileType == FILETYPE_SHORTLOAD1 || fileType == FILETYPE_SHORTXNY || fileType == FILETYPE_AGCRQ)  
	{
		//forcastTime = curTime.addDays(1);//��ǰʱ��+1�죬֮����õ����������
		m_strForcastTime.append(" 00:00:15"); //ע�⣬��ǰһ���Ǵ�00:15��ʼ��24:00����96�㣻
		m_strForcastTime.insert(6, '-');
		m_strForcastTime.insert(4, '-');
		forcastTime = QDateTime::fromString(m_strForcastTime, "yyyy-MM-dd hh:mm:ss");

	}
	//������Ԥ������ڼƻ�����֯ʱ���ʽΪ����ʱ��+  hh:mm:00��mmһ����15��30��45��00����15����һ���㣻
	else if(fileType == FILETYPE_ULTRALOAD || fileType == FILETYPE_ULTRAXNY || fileType == FILETYPE_AGCRN)
	{
		m_strForcastTime.append(":00");
		m_strForcastTime.insert(10, ':');
		m_strForcastTime.insert(8, ' ');
		m_strForcastTime.insert(6, '-');
		m_strForcastTime.insert(4, '-');
		forcastTime = QDateTime::fromString(m_strForcastTime, "yyyy-MM-dd hh:mm:ss");
	}

	for(int i = 0; i < nPointNums; ++i) //nPointNumsһ��Ϊ48��96������ô����ʱ��ֵд��vec
	{
		QDateTime tempTime(forcastTime.addSecs(60 * nTimeInterval * i)); //nTimeIntervalһ��Ϊ5��15����ÿ��5min��15minΪһ��ʱ��㣻
		firstPointTime = tempTime.toString("yyyy-MM-dd hh:mm:ss");
		vecForcastTime.push_back(firstPointTime.toStdString());
	}
	return vecForcastTime;
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
		if(g_IsDebug)
		{
			printf("ִ��mysqlʧ��: %s \n", sqlToExec.toLatin1().data());
			string logInfo = "Fails to execute sql: " + sqlToExec.toStdString() + "\n" ;
			char *pSQL = logInfo.c_str();
			WriteDailyLog(true, "clw��ִ��mysql:%sʧ�ܣ�", pSQL);
			//WriteDailyLog(true, "clw��ִ��mysqlʧ��");

			QString qText = query.lastError().text();
			QByteArray ba = qText.toLocal8Bit();
			const char* s = ba.data();
			printf("[%s: %d] - %s\n", g_dbdriver.toLatin1().data(), query.lastError().number(), s); //clw note��֮ǰΪquery.lastError().text().toLocal8Bit().data()

			QString qLogInfo = "[" + g_dbdriver + ": " +  QString::number(query.lastError().number()) +  "] - " + query.lastError().text();
			WriteDailyLog(true, qLogInfo.toLatin1().data());
		}

	
		//clw note��
		//printf("clw��ִ��sqlʧ�ܵĴ�������Ϊ%d\n", query.lastError().type()); //clw note�����ݿ������ϵ�ʱ�򷵻صľ�Ȼ��2����ErrorType�������޷������ж����ݿ���ͨ��
		//����QSqlQuery��isOpen()�ܿӣ��޷��ж����ݿ���ͨ�ԣ������Լ�дIsHisDBConnected()������
		
		//����������ݿ�û���ϣ���������
		if(!g_sysModel.IsHisDBConnected())
		{
			printf("clw������������ʷ�⣡\n");
			WriteDailyLog(true, "clw������������ʷ�⣡\n");

			if(g_sysModel.ReConnectDatabas())
			{
				printf("clw�������ɹ��������ٴ�ִ��Sql��\n");
				WriteDailyLog(true, "clw�������ɹ��������ٴ�ִ��Sql��\n");
				if(!query.exec(sqlToExec))
				{
					if(g_IsDebug)
					{
						printf("ִ��mysqlʧ��: %s \n", sqlToExec.toLatin1().data());
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
					printf("clw��������ִ��Sql�ɹ���\n");
					WriteDailyLog(true, "clw��������ִ��Sql�ɹ���\n");
					return true;
				}
			}
			else
			{
				printf("clw������ʧ�ܣ�\n");
				WriteDailyLog(true, "clw������ʧ�ܣ�\n");
			}
		}
		return false;
	}
	else
	{
		if(g_IsDebug)
		{
			printf("clw��ִ��mysql�ɹ���\n");
			WriteDailyLog(true, "ִ��mysql�ɹ���\n");
		}
	}
	return true;
	//==============================================================================================
}

bool FileParser::WriteRtdbAndConfigDB(const string& fileType, const string& strPlanID)
{
	//==============================================================================================
	//дʵʱ���
	//����wx�ӿڣ�дʵʱ��stationdispc����վ���ȿ�����Ϣ����
	//AGC��ǰ�ƻ��������ʱ�� �� AGC���������ڣ��ƻ��������ʱ��
	//==============================================================================================
	int nStationID = GetStationIDByPlanID(strPlanID);
	printf("clw��nStation = %d\n", nStationID);
	if(nStationID < 0)
	{
		//printf("clw����ѯ���ÿ��stagcplanʧ�ܣ�\n");
		WriteDailyLog(true, "clw����ѯ���ÿ��stagcplanʧ�ܣ�\n");
		return false;
	}

	icsvariant fldValue;
	fldValue.type = ft_time;
	fldValue.value.t = (double)CDateTime::GetCurrentDateTime();
	fldValue.isNull = false;
	if(fileType == FILETYPE_AGCRN)
	{
		//printf("clw��׼��д��ʵʱ�⼰���ÿ�stationdispc��\n");
		//printf("clw��nStationID = %d", nStationID);
		WriteDailyLog(true, "clw��׼��д��ʵʱ�⼰���ÿ�stationdispc��\n");
		if(m_pUpdateRealACfg->UpdateDataInCfg("stationdispc", "todayagctime", nStationID, fldValue, false, true))
		{
			//printf("clw��д��ʵʱ�⼰���ÿ�stationdispc��todayagctime�ֶγɹ���\n");
			WriteDailyLog(true, "clw��д��ʵʱ�⼰���ÿ�stationdispc��todayagctime�ֶγɹ���\n");
		}
		else
		{
			printf("clw��д��ʵʱ�⼰���ÿ�stationdispc��todayagctime�ֶ�ʧ�ܣ�\n");
			WriteDailyLog(true, "clw��д��ʵʱ�⼰���ÿ�stationdispc��todayagctime�ֶ�ʧ�ܣ�\n");
			return false;
		}
	}
	else if(fileType == FILETYPE_AGCRQ)
	{
		//printf("clw��׼��д��ʵʱ�⼰���ÿ�stationdispc��\n");
		//printf("clw��nStationID = %d", nStationID);
		WriteDailyLog(true, "clw��׼��д��ʵʱ�⼰���ÿ�stationdispc��\n");
		if(m_pUpdateRealACfg->UpdateDataInCfg("stationdispc", "nextdayagctime", nStationID, fldValue, false, true))
		{
			//printf("clw��д��ʵʱ�⼰���ÿ�stationdispc��nextdayagctime�ֶγɹ���\n");
			WriteDailyLog(true, "clw��д��ʵʱ�⼰���ÿ�stationdispc��nextdayagctime�ֶγɹ���\n");
		}
		else
		{
			printf("clw��д��ʵʱ�⼰���ÿ�stationdispc��nextdayagctime�ֶ�ʧ�ܣ�\n");
			WriteDailyLog(true, "clw��д��ʵʱ�⼰���ÿ�stationdispc��nextdayagctime�ֶ�ʧ�ܣ�\n");
			return false;
		}
	}
	return true;
	//==============================================================================================
}

int FileParser::GetStationIDByPlanID(const string& strPlanID)
{
	/* �����ý������
	SET FOREIGN_KEY_CHECKS=0;
	-- ----------------------------
	-- Table structure for strgcrvplan
	-- ----------------------------
	CREATE TABLE `stagcplan` (
	`ID` int(11) NOT NULL,
	`stationid` int(11) NOT NULL,
	PRIMARY KEY (`ID`)
	) ENGINE=InnoDB DEFAULT CHARSET=gbk COMMENT='AGC���ȼƻ�������Ϣ';
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
		printf("clw����ʼ�ƶ��ļ�%s\n", _srcDir.c_str());
		printf("\n");
		printf("\n");
		WriteDailyLog(true, "clw����ʼ�ƶ��ļ�%s\n", _srcDir.c_str());
	}

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
			//printf("clw������·��%sʧ�ܣ����飡\n", dstDir.toLatin1().data());
			WriteDailyLog(true, "clw������·��ʧ�ܣ����飡\n");
			return false;
		}
	}

	//�Ƿ񸲸�
	if(coverFileIfExist)
	{
		if(dir.remove(dstDir + "/" + fileName))
		{
			if(g_IsDebug)
			{
				QString strLogInfo = dstDir + "/" + fileName;
				printf("clw�����ھ��ļ�%s���ҳɹ�ɾ����׼��д�����ļ�\n", strLogInfo.toLatin1().data());
				WriteDailyLog(true, "clw�����ھ��ļ�%s���ҳɹ�ɾ����׼��д�����ļ�\n", strLogInfo.toLatin1().data());
			}
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
	//����ɾ��n��ǰ���ļ�����ini�п���
	QString filePath = "../file/processed";
	QDir dir;

	vector<string> vecToDeleteFiles;
	GetFiles(filePath.toStdString(), vecToDeleteFiles);
	int len = vecToDeleteFiles.size();
	for(int i=0; i<len; ++i)
	{
		string::size_type fileNameLen = vecToDeleteFiles[i].size();
		string::size_type nLeftUnderLinePos = vecToDeleteFiles[i].find('_'); 
		//printf("clw���»��ߺ����ַ�Ϊ%c\n", vecToDeleteFiles[i][nLeftUnderLinePos+1]);
		while(nLeftUnderLinePos < fileNameLen && 
			(vecToDeleteFiles[i][nLeftUnderLinePos+1] < DIGIT_ASCII_MIN || vecToDeleteFiles[i][nLeftUnderLinePos+1] > DIGIT_ASCII_MAX) /*�»��ߺ�������������֣��ͼ�������һ���»���*/)
		{
			nLeftUnderLinePos = vecToDeleteFiles[i].find('_', nLeftUnderLinePos+1); 
		}

		string::size_type nMiddleUnderLinePos = vecToDeleteFiles[i].find('_', nLeftUnderLinePos + 1); 
		QString strFileTime;
		if(nMiddleUnderLinePos - nLeftUnderLinePos - 1 >= 8)
			strFileTime = QString::fromStdString(vecToDeleteFiles[i].substr(nLeftUnderLinePos + 1, nMiddleUnderLinePos - nLeftUnderLinePos - 1).substr(0, 8)); //��yyyyMMddhhmmת��ΪyyyyMMdd
		else
		{
			WriteDailyLog(true, "clw��ɾ���ļ�ʱ�����ļ���ʱ���ʽ����ȷ�������ʽΪyyyyMMdd��yyyyMMddhhmm��\n");
			printf("clw��ɾ���ļ�ʱ�����ļ���ʱ���ʽ����ȷ�������ʽΪyyyyMMdd��yyyyMMddhhmm��\n");
			continue;
		}
		long long nFileTime = strFileTime.toLongLong();
		QDateTime curTime = QDateTime::currentDateTime();  //��ȡ��ǰʱ��
		QString strCurDate = curTime.toString("yyyyMMdd"); //��ǰʱ��ת��Ϊ�ַ������ͣ���ʽΪ��������ʱ���룬��2019-05-30 14:28:15
		int nCurDate = strCurDate.toInt();

		if(nCurDate - nFileTime > m_nFileKeepTime) //����m_nFileKeepTime = 10����10����ǰ���ļ����ᱻɾ��������ʱ���ȡ��day�����Ӻ������Ϣ������
		{
			if(dir.remove(QString::fromStdString(vecToDeleteFiles[i])))
			{
				//printf("clw������ɾ���ļ����ļ�ʱ��Ϊ%s\n", strFileTime.toLatin1().data());
				string strLogInfo = "clw������ɾ���ļ����ļ�ʱ��Ϊ" + strFileTime.toStdString();
				WriteDailyLog(true, strLogInfo.c_str());
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
//4. ������־�ʹ���
void FileParser::DealWithLogs(string logLine, bool bCout, int outFreq)
{
	char* cLogLines = const_cast<char*>(logLine.c_str());
	if (m_circleTimes%outFreq == 1 || m_circleTimes == 0)
	{
		char logs[128];
		sprintf(logs, "��FileParser�� circleTimes=%d, ", m_circleTimes);
		strcat(logs, cLogLines);
		WriteDailyLog(true, logs);
		if (bCout == true)
		{
			printf("logs:%s\n", logs);
		}
	}
}
#endif 