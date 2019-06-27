// dllfactory.h: implement for the CDllFactory class.
#ifndef _DLLFACTORY_H_
#define _DLLFACTORY_H_

#include <qmutex.h>     // for QMutex
#include "icsintflib.h" // for CICSIntfLib

//DLL����
class CDllFactory
{
public:
	//ʵ������
	static CDllFactory* Instance()
	{
		if(m_dllFactory == 0)
		{
			m_dllFactory = new CDllFactory();
		}
		return m_dllFactory;
	}
	
	//��ʵ����
	static void UnInstance()
	{
		if(m_dllFactory != 0)
		{
			delete m_dllFactory;
			m_dllFactory = 0;
		}
	}
	
	//����DLL�洢��
	void SetDllPool(QMap<QString, CICSIntfLib*>* pDllPool)
	{
		QMutexLocker locker(&m_mutex);
		if(pDllPool != 0 && pDllPool != m_pDllMap)
		{
			QString dllName;
			CICSIntfLib* pCurDll;
			QMap<QString, CICSIntfLib*>::Iterator it = m_pDllMap->begin();
			for(; it != m_pDllMap->end(); ++it)//�ڲ�DLL�洢��
			{
				dllName = it.key();
				pCurDll = it.data();
				if(pDllPool->find(dllName)!=pDllPool->end())//�ⲿDLL�洢��
				{
					delete pCurDll;//ɾ���ڲ������DLL
					(*m_pDllMap)[dllName] = (*pDllPool)[dllName];
					printf("Commgr: unload redundant \'%s.dll\' success. \n", dllName.ascii());
				}
				else
				{
					pDllPool->insert(dllName, pCurDll);//�ڲ�DLL�����ⲿDLL�洢��
				}
			}		
			m_pDllMap = pDllPool;//���ⲿDLL�洢���滻�ڲ�DLL�洢��
		}
	}

	//��ȡDLL�洢��
	QMap<QString, CICSIntfLib*>* GetDllPool()
	{
		return m_pDllMap;
	}
	
	//��ȡQMutex����
	QMutex* GetMutex()
	{
		return &m_mutex;
	}
	
	//��ȡ�ӿ�DLL����
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
	
	//ж�ؽӿ�DLL����
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
	//ж�����еĽӿ�DLL����
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
	static CDllFactory* m_dllFactory;       // DLL����ʵ��
	QMap<QString, CICSIntfLib*>* m_pDllMap; // DLL�洢��
	QMutex m_mutex;

};
	
#endif 
