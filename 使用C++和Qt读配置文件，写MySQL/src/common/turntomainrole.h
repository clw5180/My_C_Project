//turntomainrole.h

#ifndef _TURNTOMAINROLE_H_
#define _TURNTOMAINROLE_H_

#include <qthread.h>
#include <qmutex.h>

class CTurntoMainRole : public QThread
{
public:
	CTurntoMainRole();
	virtual ~CTurntoMainRole();

	bool IsToMainRole();
	void BeginToMainRole();
	void EndToMainRole();
	
protected:
	virtual void run();
	bool DoToMainRole();
	void EndToMainTime();
	
private:
	QMutex s_mtxToMain;
	bool s_toMainRole;//是否切换为主角色
	double s_toMainTime;//切换为主角色的时间
};

extern CTurntoMainRole g_toMainRoleMgr;

#endif
