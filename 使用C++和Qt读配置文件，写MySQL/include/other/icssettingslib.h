// icssettingslib.h: implement for the CICSSettings class.
#ifndef _TASKMODELLIB_H_
#define _TASKMODELLIB_H_

#include "../commgr/intfobjptra.h"       // for CIntfObjPtrA
#include "../../interface/icssettings.h" // for IICSTaskModel��IICSTypeTree

//DLL����Ľӿڶ�������
class CICSSettings : public CIntfObjPtrA<IICSSettings>
{
public:
	CICSSettings() : CIntfObjPtrA<IICSSettings>(MODULE_ICSSettings)
	{
		m_pObj = CIntfObjPtr1<IICSSettings>::CreateObject(CLSID_ICSSettings, IID_ICSSettings);
		if(m_pObj) m_pObj->AddRef();//��Ϊicssettingsģ�鲻���ͷţ�����Ӵ˾䣡
	}
};

#endif 
