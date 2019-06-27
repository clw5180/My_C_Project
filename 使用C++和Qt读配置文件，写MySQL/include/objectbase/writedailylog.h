//writedailylog.h  // for WriteDailyLog(doWrite, context, ...)
#ifndef _WRITEDAILYLOG_H_
#define _WRITEDAILYLOG_H_

#include "objectbase.h" // for OBJECTBASE_EXPORT

typedef enum
{
	BFPM_Nothing, //不敏感
	BFPM_Truncate,//截断处理
	BFPM_Rename   //重命名备份处理
}BigFileProcessMode;//大文件处理模式

void OBJECTBASE_EXPORT SetDailyLogName(const char* logName);
void OBJECTBASE_EXPORT SetBigFileProcessMode(const BigFileProcessMode& procMode);
void OBJECTBASE_EXPORT SetFileMaxLength(int fileSize);
bool OBJECTBASE_EXPORT WriteDailyLog(bool doWrite, const char* context, ...);

#endif
