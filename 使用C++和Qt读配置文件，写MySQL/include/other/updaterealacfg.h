// onlinemodel.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _UPDATEDT_H_
#define _UPDATEDT_H_

#include "../../interface/iupdaterealcfgdt.h"
#include "../commgr/intfobjptrb.h"      // for CIntfObjPtrB<IntfObj>

class CUpdateDt: public CIntfObjPtrB<IUpdateRealACfg>
{
public:
	CUpdateDt() : CIntfObjPtrB<IUpdateRealACfg>(
		DLL_UPDATEREALACFG, CID_UPDATEREALACFG, IID_UPDATEREALACFG) {}
};

#endif
