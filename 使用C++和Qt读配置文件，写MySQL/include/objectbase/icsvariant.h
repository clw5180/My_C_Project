//icsvariant.h

#ifndef _ICSVARIANT__H_
#define _ICSVARIANT__H_

#include <qstring.h>
#include "objectbase.h"                  // for OBJECTBASE_EXPORT
#include "../../interface/icsvalue.h"    // for icsvariant

class OBJECTBASE_EXPORT CICSVariant
{
public:
    CICSVariant();
	CICSVariant(FIELDTYPE ft);

    CICSVariant(BYTE b);
    CICSVariant(short s);
    CICSVariant(int i);
    CICSVariant(float f);
    CICSVariant(double d);
    CICSVariant(const char* s);
    CICSVariant(IICSUnknown* pUnk);
	CICSVariant(icsarray ary);
    CICSVariant(icsvariant& v);
    CICSVariant(CICSVariant& v);
		
    virtual ~CICSVariant();
	
	void ReleaseIntf();
    int GetType();
	bool IsNull();
		
    operator BYTE();
    operator short();
    operator int();
    operator float();
    operator double();
    operator char*();
    operator IICSUnknown*();
    operator icsarray();
    operator icsvariant();

	icsvariant* operator&();
	
    BYTE AsByte();
    short AsShort();
    int AsInt();
    float AsFloat();
    double AsDouble();
    char* AsChar();
    IICSUnknown* AsIUnknown();
    icsarray AsArray();
    icsvariant AsVariant();

    CICSVariant& operator = (BYTE b);
    CICSVariant& operator = (short s);
    CICSVariant& operator = (int i);
    CICSVariant& operator = (float f);
    CICSVariant& operator = (double d);
    CICSVariant& operator = (const char *pchar);
    CICSVariant& operator = (IICSUnknown *pUnk);
    CICSVariant& operator = (icsarray ary);
    CICSVariant& operator = (icsvariant v);
    CICSVariant& operator = (CICSVariant v);

    bool operator == (BYTE b);
    bool operator == (short s);
    bool operator == (int i);
    bool operator == (float f);
    bool operator == (double d);
    bool operator == (const char *pchar);  
    bool operator == (IICSUnknown *pUnk);
	bool operator == (icsarray ary);
    bool operator == (const icsvariant& v);
    bool operator == (const CICSVariant& v);

    QString ToString(const char* sep=",");
	
protected:
	friend class CStreamTool;
	icsvariant m_v;	
	
    CICSVariant& ToVariant(BYTE b);
    CICSVariant& ToVariant(short s);
    CICSVariant& ToVariant(int i);
    CICSVariant& ToVariant(float f);
    CICSVariant& ToVariant(double d);
    CICSVariant& ToVariant(const char *pchar);
    CICSVariant& ToVariant(IICSUnknown *pUnk);
    CICSVariant& ToVariant(icsarray ary);
    CICSVariant& ToVariant(icsvariant v);
    CICSVariant& ToVariant(CICSVariant v);
};

#endif 
