/*****************************************
 *writelog.h -- д��־���ļ�
 *revised:
		ʹ��std::ofstream����QFile 
 *****************************************/
#ifndef __WRITELOG_H_
#define __WRITELOG_H_

#include <qmutex.h>
#include <stdio.h>
#include <string.h>
#include "../common/cdatetime.h"// for CDateTime

#define LOGFILENAME  "esfileserver_log"

#ifdef _MSC_VER
	#define CRLF "\r\n"
#else
	#define CRLF "\n"
#endif


inline void WriteLogToFile(const char* fileName, const char* content)
{
	//����
	static QMutex _ctrlLogLock;
	QMutexLocker locker(&_ctrlLogLock);
	
	//�õ���ǰʱ��
	CDateTime curTime = CDateTime::GetCurrentDateTime();
	
	//������־����
	//static char buf[1024];	
	static char buf[65536];	 //clw modify
	sprintf(buf, "%4d-%02d-%02d %02d:%02d:%02d: ", 
		curTime.GetYear(),curTime.GetMonth(),curTime.GetDay(),
		curTime.GetHour(),curTime.GetMinute(),curTime.GetSecond());
	//strncat(buf, content, 999);
	strncat(buf, content, 65000); //clw modify
	strcat(buf, CRLF);
	
	//�õ��ļ���
	static const int procSize = 1024*1024*64;//64M
	static char wholePath[256], wholeName[256], newName[256];
	static bool bUninit = true;
	static bool bRemove = false;
	if(bUninit)
	{
		sprintf(wholePath, "../log/%s_%4d%02d%02d%02d%02d%02d", fileName, 
			curTime.GetYear(),curTime.GetMonth(),curTime.GetDay(), 
			curTime.GetHour(),curTime.GetMinute(),curTime.GetSecond());
		sprintf(wholeName, "%s.log", wholePath);
		sprintf(newName, "%s_0.log", wholePath);
		bUninit = false;
	}
	
	//���ļ�
	FILE * pFile = fopen(wholeName, "ab");
	if(pFile == NULL)
	{
		pFile = fopen(wholeName, "wb");
		if(pFile == NULL) return;//�ļ���ʧ��
	}

    fseek(pFile, 0, SEEK_END);
    int fSize = ftell(pFile);

	//����ļ�����ָ���ֽڳ��ȣ����нضϴ���
	if(fSize >= procSize)//����ļ�����ָ���ֽڳ��ȣ����нضϴ���
	{
		fclose(pFile);
		if(bRemove) remove(newName);
		if(rename(wholeName, newName)!=0)
		{
			perror("WriteLogToFile, rename error:");
			return ;//��������������ʧ��
		}
		bRemove = true;
		pFile = fopen(wholeName, "wb+");
		if(pFile == NULL)
		{
			perror("WriteLogToFile, fopen error:");
			return ;//�ļ���ʧ��
		}
	}
	
	//����־����д���ļ�
	fputs(buf, pFile);
	//�ر��ļ�
	fclose(pFile);
	
};

#define WRITECTRLLOG(szLogTxt) WriteLogToFile("control_server_log", (szLogTxt))

#endif