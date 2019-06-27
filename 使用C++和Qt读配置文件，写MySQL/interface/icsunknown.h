#ifndef _ICSUNKNOWN__H_
#define _ICSUNKNOWN__H_

class IICSUnknown;

#define IID_IICSUnknown "IICSUnknown"
/*
IICSUnknown�ӿ�
*/
class IICSUnknown
{
public: 
	virtual bool QueryInterface(const char* InterfaceName, void** ppUnknown) = 0;
    virtual int AddRef()=0;
    virtual int Release()=0; 
public:
    //���������������ָ��ģ����
    static char* GetIntfName()
    {
        return IID_IICSUnknown;
    }; 
};

#endif 
