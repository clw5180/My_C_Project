
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

	/* tablename:������
		fieldname���ֶ�����
		ID������ID��
		data����������;
		bPermissions���Ƿ���ҪȨ����֤;
		bUpdateRtdb:�Ƿ�ͬ������ʵʱ��
		����ֵΪ���½��
	*/
	virtual bool UpdateDataInReal(const char* tablename,const char* fieldname,int ID,icsvariant& fldValue) = 0; 
	virtual bool UpdateDataInCfg(const char* tablename,const char* fieldname,int ID,icsvariant& fldValue,bool bPermissions = false,bool bUpdateRtdb = false) = 0; 

};

#endif
