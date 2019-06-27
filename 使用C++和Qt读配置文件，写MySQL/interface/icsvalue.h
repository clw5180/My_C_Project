// icsvalue.h 

#ifndef _ICSVALUE_20080611_H_
#define _ICSVALUE_20080611_H_

#define MAX_STRINGLEN		255
#define MAX_MODELTYPELEN	64				// 定义业务模型中类型字符最大长度

class IICSUnknown;

typedef unsigned char BYTE;

// 字段数据类型
typedef enum
{   ft_empty, ft_byte, ft_short, ft_int, ft_long, ft_float, 
	ft_double, ft_time, ft_string, ft_object, ft_grpstring, ft_array,
}FIELDTYPE;

// 字符串字段值结构
typedef struct 
{
	int       length;
    char*     string;
}icsstring;

// 数组字段值结构
typedef struct 
{
    FIELDTYPE type;
	int       count;
    void*     values;
}icsarray;

// 字段值定义
typedef union
{
	BYTE b;
    short s;
    int i;     
	float f;
    double d;
    double t;
	char str[256];
	icsstring strg;  
	icsarray ary;
    IICSUnknown* obj; 
}icsvalue;

typedef struct // 字段数据值
{
    FIELDTYPE type;
    icsvalue  value;
	bool      isNull;
}icsvariant;

typedef struct 
{
	unsigned int count;
	int*         values;
}intarray;

#endif
