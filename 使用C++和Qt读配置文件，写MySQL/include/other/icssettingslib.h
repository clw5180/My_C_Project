// icssettingslib.h: implement for the CICSSettings class.
#ifndef _TASKMODELLIB_H_
#define _TASKMODELLIB_H_

#include "../commgr/intfobjptra.h"       // for CIntfObjPtrA
#include "../../interface/icssettings.h" // for IICSTaskModel、IICSTypeTree

//DLL输出的接口对象引用
class CICSSettings : public CIntfObjPtrA<IICSSettings>
{
public:
	CICSSettings() : CIntfObjPtrA<IICSSettings>(MODULE_ICSSettings)
	{
		m_pObj = CIntfObjPtr1<IICSSettings>::CreateObject(CLSID_ICSSettings, IID_ICSSettings);
		if(m_pObj) m_pObj->AddRef();//因为icssettings模块不能释放，故添加此句！
	}
};

#endif 
