// dllfactory.h: implement for the CDllFactory class.
#ifndef _DLLFACTORY_H_
#define _DLLFACTORY_H_

#include <qmutex.h>     // for QMutex
#include "icsintflib.h" // for CICSIntfLib

//DLL工厂
class CDllFactory
{
public:
	//实例化。
	static CDllFactory* Instance()
	{
		if(m_dllFactory == 0)
		{
			m_dllFactory = new CDllFactory();
		}
		return m_dllFactory;
	}
	
	//反实例化
	static void UnInstance()
	{
		if(m_dllFactory != 0)
		{
			delete m_dllFactory;
			m_dllFactory = 0;
		}
	}
	
	//设置DLL存储池
	void SetDllPool(QMap<QString, CICSIntfLib*>* pDllPool)
	{
		QMutexLocker locker(&m_mutex);
		if(pDllPool != 0 && pDllPool != m_pDllMap)
		{
			QString dllName;
			CICSIntfLib* pCurDll;
			QMap<QString, CICSIntfLib*>::Iterator it = m_pDllMap->begin();
			for(; it != m_pDllMap->end(); ++it)//内部DLL存储池
			{
				dllName = it.key();
				pCurDll = it.data();
				if(pDllPool->find(dllName)!=pDllPool->end())//外部DLL存储池
				{
					delete pCurDll;//删除内部冗余的DLL
					(*m_pDllMap)[dllName] = (*pDllPool)[dllName];
					printf("Commgr: unload redundant \'%s.dll\' success. \n", dllName.ascii());
				}
				else
				{
					pDllPool->insert(dllName, pCurDll);//内部DLL加入外部DLL存储池
				}
			}		
			m_pDllMap = pDllPool;//用外部DLL存储池替换内部DLL存储池
		}
	}

	//获取DLL存储池
	QMap<QString, CICSIntfLib*>* GetDllPool()
	{
		return m_pDllMap;
	}
	
	//获取QMutex对象
	QMutex* GetMutex()
	{
		return &m_mutex;
	}
	
	//获取接口DLL对象
	bool GetDllLib(const char* dllName, CICSIntfLib** ppDllLib = 0)
	{
		QMutexLocker locker(&m_mutex);
		if(m_pDllMap->find(dllName) != m_pDllMap->end())
		{
			if(ppDllLib) *ppDllLib = (*m_pDllMap)[dllName];
			return true;
		}
		return false;
	}
	
	//卸载接口DLL对象
	void UnLoadDll(const char* dllName)
	{
		CICSIntfLib* pCurDll = 0;
		if(GetDllLib(dllName, &pCurDll))
		{
			delete pCurDll;
			m_mutex.lock();
			m_pDllMap->remove(dllName);
			m_mutex.unlock();
		}
	}
	
	~CDllFactory()
	{
		UnLoadDlls();
	}
	
private:
	//卸载所有的接口DLL对象
	void UnLoadDlls()
	{
		QMutexLocker locker(&m_mutex);
		QMap<QString, CICSIntfLib*>::Iterator it = m_pDllMap->end();
		for(int num = m_pDllMap->count(); num > 0; num--)
		{
			--it;
			delete it.data();
		}
		m_pDllMap->clear();
	}

	CDllFactory();
	
private:
	static CDllFactory* m_dllFactory;       // DLL工厂实例
	QMap<QString, CICSIntfLib*>* m_pDllMap; // DLL存储池
	QMutex m_mutex;

};
	
#endif 
