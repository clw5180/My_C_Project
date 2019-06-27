//writedailylog.h  // for WriteDailyLog(doWrite, context, ...)
#ifndef _WRITEDAILYLOG_H_
#define _WRITEDAILYLOG_H_

#include "objectbase.h" // for OBJECTBASE_EXPORT

typedef enum
{
	BFPM_Nothing, //������
	BFPM_Truncate,//�ضϴ���
	BFPM_Rename   //���������ݴ���
}BigFileProcessMode;//���ļ�����ģʽ

void OBJECTBASE_EXPORT SetDailyLogName(const char* logName);
void OBJECTBASE_EXPORT SetBigFileProcessMode(const BigFileProcessMode& procMode);
void OBJECTBASE_EXPORT SetFileMaxLength(int fileSize);
bool OBJECTBASE_EXPORT WriteDailyLog(bool doWrite, const char* context, ...);

#endif
