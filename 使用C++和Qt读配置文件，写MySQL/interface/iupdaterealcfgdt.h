
#ifndef _IUPDATEREALTIMECONFIGDATA_H_
#define _IUPDATEREALTIMECONFIGDATA_H_

#include "icsunknown.h" // for IICSUnknown
#include "icsvalue.h"   // for icsvariant

#define DLL_UPDATEREALACFG	"updaterealacfg"
#define IID_UPDATEREALACFG	"IUpdateRealacfg"
#define CID_UPDATEREALACFG	"CUpdateRealacfg"

#include <vector>

class IUpdateRealACfg:  public IICSUnknown
{
public:

	/* tablename:表名；
		fieldname：字段名；
		ID：更新ID；
		data：更新数据;
		bPermissions：是否需要权限验证;
		bUpdateRtdb:是否同步更新实时库
		返回值为更新结果
	*/
	virtual bool UpdateDataInReal(const char* tablename,const char* fieldname,int ID,icsvariant& fldValue) = 0; 
	virtual bool UpdateDataInCfg(const char* tablename,const char* fieldname,int ID,icsvariant& fldValue,bool bPermissions = false,bool bUpdateRtdb = false) = 0; 

};

#endif
