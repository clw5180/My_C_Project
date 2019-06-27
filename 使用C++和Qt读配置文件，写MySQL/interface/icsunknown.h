#ifndef _ICSUNKNOWN__H_
#define _ICSUNKNOWN__H_

class IICSUnknown;

#define IID_IICSUnknown "IICSUnknown"
/*
IICSUnknown接口
*/
class IICSUnknown
{
public: 
	virtual bool QueryInterface(const char* InterfaceName, void** ppUnknown) = 0;
    virtual int AddRef()=0;
    virtual int Release()=0; 
public:
    //这个函数仅给智能指针模板用
    static char* GetIntfName()
    {
        return IID_IICSUnknown;
    }; 
};

#endif 
