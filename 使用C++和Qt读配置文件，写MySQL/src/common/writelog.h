/*****************************************
 *writelog.h -- 写日志到文件
 *revised:
		使用std::ofstream代替QFile 
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
	//加锁
	static QMutex _ctrlLogLock;
	QMutexLocker locker(&_ctrlLogLock);
	
	//得到当前时间
	CDateTime curTime = CDateTime::GetCurrentDateTime();
	
	//设置日志内容
	//static char buf[1024];	
	static char buf[65536];	 //clw modify
	sprintf(buf, "%4d-%02d-%02d %02d:%02d:%02d: ", 
		curTime.GetYear(),curTime.GetMonth(),curTime.GetDay(),
		curTime.GetHour(),curTime.GetMinute(),curTime.GetSecond());
	//strncat(buf, content, 999);
	strncat(buf, content, 65000); //clw modify
	strcat(buf, CRLF);
	
	//得到文件名
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
	
	//打开文件
	FILE * pFile = fopen(wholeName, "ab");
	if(pFile == NULL)
	{
		pFile = fopen(wholeName, "wb");
		if(pFile == NULL) return;//文件打开失败
	}

    fseek(pFile, 0, SEEK_END);
    int fSize = ftell(pFile);

	//如果文件大于指定字节长度，进行截断处理
	if(fSize >= procSize)//如果文件大于指定字节长度，进行截断处理
	{
		fclose(pFile);
		if(bRemove) remove(newName);
		if(rename(wholeName, newName)!=0)
		{
			perror("WriteLogToFile, rename error:");
			return ;//进行重命名处理失败
		}
		bRemove = true;
		pFile = fopen(wholeName, "wb+");
		if(pFile == NULL)
		{
			perror("WriteLogToFile, fopen error:");
			return ;//文件打开失败
		}
	}
	
	//将日志内容写入文件
	fputs(buf, pFile);
	//关闭文件
	fclose(pFile);
	
};

#define WRITECTRLLOG(szLogTxt) WriteLogToFile("control_server_log", (szLogTxt))

#endif