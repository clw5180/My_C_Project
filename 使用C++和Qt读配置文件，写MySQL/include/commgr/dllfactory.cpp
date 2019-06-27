// dllfactory.cpp: initialization of the CDllFactory::m_dllFactory.
// 用于不含有静态DLL的应用程序（动态DLL、可执行程序）

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
