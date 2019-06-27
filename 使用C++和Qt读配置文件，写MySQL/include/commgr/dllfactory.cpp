// dllfactory.cpp: initialization of the CDllFactory::m_dllFactory.
// ���ڲ����о�̬DLL��Ӧ�ó��򣨶�̬DLL����ִ�г���

#include "dllfactory.h" // for CDllFactory
#include "setdllpool.h" // for SetDllPool()

QMap<QString, CICSIntfLib*> g_dllPool;

CDllFactory* CDllFactory::m_dllFactory = 0;

CDllFactory::CDllFactory()
{
	g_dllPool.clear();
	m_pDllMap = &g_dllPool;
}

void SetDllPool(QMap<QString, CICSIntfLib*>* pDllPool)
{
	CDllFactory::Instance()->SetDllPool(pDllPool);
}
