#ifndef _ICFGINTF_H
#define _ICFGINTF_H

#include "icsunknown.h"

#define MODULE_ICSSettings	"icssettings"
#define IID_ICSSettings		"iidicssettings"
#define CLSID_ICSSettings	"clsidicscfgintf"

#define MAX_STRINGLEN 255

/*!
\brief 配置文件读写接口
1.读写配置文件中指定项的值

2.读环境变量
*/

class IICSSettings : public IICSUnknown
{
public:
	/*!
	\brief 以字符串的方法读取配置项的值 
	@param[in] item 配置项名称
	@param[in] dft  默认值
	\return 返回读取到的配置项的值，如果不存在该配置项则返回默认值
	*/
	//virtual const char* ItemReadString(const char* item, const char* dft = "") = 0;
	
	
	/*!
	\brief 以字符串的方法读取配置项的值 
	@param[in]  item      配置项名称
	@param[out] sValueOut 保存配置项值的字符串数组地址
	@param[out] iSizeIn   保存配置项值的字符串数组的长度
	\return 如果读取成功，并且读取到的值的长度小于 iSizeIn  则将读取到的值存入 sValueOut 数组，同时 返回 true。
	        如果读取成功，并且读取到的值的长度大于 iSizeIn  则只将读取到的值的前面 ISizeIn －1 个字符放入 sValueOut 数组，同时返回false
			如果读取失败，则返回 false
	*/
	virtual bool ItemReadString(const char* item, char * sValueOut,int iSizeIn ) = 0;
	/*!
	\brief 重载函数，以字符串的方法设置配置项的值 
	@param[in] item 配置项名称
	@param[in] val  要设置的值
	\return 设置成功返回 true 设置失败返回 false
	*/
	virtual bool ItemWrite(const char* item, const char* val) = 0;

	/*!
	\brief 以布尔量的方法读取配置项的值 
	@param[in] item 配置项名称
	@param[in] dft  默认值
	\return 返回读取到的配置项的值，如果不存在该配置项则返回默认值
	*/
	virtual const bool ItemReadBool(const char* item, const bool dft = false) = 0;
	/*!
	\brief 重载函数，以布尔量的方法设置配置项的值 
	@param[in] item 配置项名称
	@param[in] val  要设置的值
	\return 设置成功返回 true 设置失败返回 false
	*/
	virtual bool ItemWrite(const char* item, const bool val) = 0;

	/*!
	\brief 以整数量的方法读取配置项的值 
	@param[in] item 配置项名称
	@param[in] dft  默认值
	\return 返回读取到的配置项的值，如果不存在该配置项则返回默认值
	*/
	virtual const int ItemReadInt(const char* item, const int dft = 0) = 0;
	/*!
	\brief 重载函数，以整数量的方法设置配置项的值 
	@param[in] item 配置项名称
	@param[in] val  要设置的值
	\return 设置成功返回 true 设置失败返回 false
	*/
	virtual bool ItemWrite(const char* item, const int val) = 0;

	/*!
	\brief 以浮点值的方法读取配置项的值 
	@param[in] item 配置项名称
	@param[in] dft  默认值
	\return 返回读取到的配置项的值，如果不存在该配置项则返回默认值
	*/
	virtual const double ItemReadDouble(const char* item, const double dft = 0.0) = 0;
	/*!
	\brief 重载函数，以浮点值的方法设置配置项的值 
	@param[in] item 配置项名称
	@param[in] val  要设置的值
	\return 设置成功返回 true 设置失败返回 false
	*/
	virtual bool ItemWrite(const char* item, const double val) = 0;
	
	/*!
	\brief 判断是否存在指定的配置项  
	@param[in] item 配置项名称
	\return 存在该配置项返回 true ，不存在该配置项返回 false
	*/ 
	virtual bool ExistItem(const char* item) = 0;

	/*!
	\brief 读取环境变量的值  
	@param[in] item 环境变量名称
	@param[in] dft  默认值
	\return 返回该环境变量的值，如果不存在该变量则返回默认值
	*/
	virtual const char * ReadEnv(const char* envName,const char * dft = "") = 0;

	/*!
	\brief 读取节点的说明 
	@param[in] section 节点名称
	@param[in] dft     默认值
	\return 返回该节点的说明，如果不存在该节点则返回默认值
	*/
	//virtual const char * SectionDescRead(const char* section , char * dft = "") = 0;

	/*!
	\brief 读取节点的说明 
	@param[in]  section 节点名称
	@param[out] sDescOut 保存说明的字符串数组地址
	@param[out] iSizeIn   保存说明的字符串数组的长度
	\return 如果读取成功，并且读取到的说明的长度小于 iSizeIn  则将读取到的值存入 sDescOut 数组，同时 返回 true。
	        如果读取成功，并且读取到的说明的长度大于 iSizeIn  则只将读取到的值的前面 ISizeIn －1 个字符放入 sDescOut 数组，同时返回false
			如果读取失败，则返回 false
	*/
	virtual bool SectionDescRead(const char* section,char * sDescOut,int iSizeIn) = 0;
	/*!
	\brief 读取配置项的说明
	@param[in] item 配置项名称
	@param[in] dft  默认值
	*/
	//virtual const char * ItemDescRead(const char* item ,char * dft = "") = 0;

	/*!
	\brief 读取配置项的说明 
	@param[in]  item      配置项名称
	@param[out] sDescOut  保存说明的字符串数组地址
	@param[out] iSizeIn   保存说明的字符串数组的长度
	\return 如果读取成功，并且读取到的说明的长度小于 iSizeIn  则将读取到的值存入 sDescOut 数组，同时 返回 true。
	        如果读取成功，并且读取到的说明的长度大于 iSizeIn  则只将读取到的值的前面 ISizeIn －1 个字符放入 sDescOut 数组，同时返回false
			如果读取失败，则返回 false
	*/
	virtual bool ItemDescRead(const char* item,char * sDescOut,int iSizeIn) = 0;
	/*!
	\brief 设置节点的说明 
	@param[in] section 节点名称
	@param[in] Desc    要设置的值
	\return 设置成功返回true，设置失败返回false
	*/
	virtual bool SectionDescWrite(const char* section , char * Desc = "") = 0;

	/*!
	\brief 设置配置项的说明 
	@param[in] item 配置项名称
	@param[in] Desc 要设置的值
	\return 设置成功返回true，设置失败返回false
	*/
	virtual bool ItemDescWrite(const char* item ,char * Desc = "") = 0;

	
	/*!
	\brief 删除指定的配置项  
	@param[in] item 配置项名称
	\return 成功返回true，失败返回false
	*/

	virtual bool DelItem(const char* item) = 0;

	
	/*!
	\brief 删除指定的节点  
	@param[in] section 节点名称
	\return 成功返回true，失败返回false
	*/

	virtual bool DelSection(const char* section) = 0;

	
	/*!
	\brief 建立新的配置项
	在指定位置建立新的配置项。新配置项的名称由包含路径的 item 参数指定。
	@param[in]   item   新配置项名称
	@param[in]   value  新配置项的值
	@param[in]   desc   新配置项的说明
	\return 成功返回true，失败返回false
	*/

	virtual bool AddItem(const char* item , char * value ,char * desc = "") = 0;

	/*!
	\brief 建立新的节点
	在指定位置建立新的节点。新节点的名称由包含路径的 section 参数指定。
	@param[in]   section   新节点名称
	@param[in]   desc      新节点的说明
	\return 成功返回true，失败返回false
	*/
	virtual bool AddSection(const char* section , char * desc = "") = 0;

	
	/*!
	\brief       设置当前操作的节点路径，设置完毕后，以后操作都基于本节点进行
	@param[in]   section 节点路径
	\return	 	 成功返回true，失败返回false
	*/
	virtual bool SetCurrentSectionPath(const char* section) = 0;

	/*!
	\brief       获得指定节点下属的 值和名称列表
	@param[in]    section   节点路径
	@param[out]   nameList  名称列表
	@param[out]   valueList 字符串值列表
	@param[out]   itemCount 列表中值个数
	\return	 	 成功返回true ， 失败返回 false
	*/
	virtual bool GetItemList(const char* section,char ** &nameList , char ** &valueList,int &itemCount) = 0;

	/*!
	\brief 判断是否存在指定的节点  
	@param[in] section 节点名称
	\return 存在该节点返回 true ，不存在该节点返回 false
	*/ 
	virtual bool ExistSection(const char* section) = 0;
	
	/*!
	\brief 指定配置文件的文件名 
	@param[in] fileName 配置文件名，不指定该文件名会使用默认名称 “icssettings.xml” 。 
	                    该参数只重新指定一个配置文件的文件名，该名称不应包含路径。对该文件的访问顺序和 默认文件一致
	\return 成功返回 true ，失败返回 false
	 */ 
	virtual bool SetCfgFileName(const char* fileName) = 0;

	/*!
	\brief       获得指定节点下属的子节点列表
	@param[in]    section   节点路径
	@param[out]   nameList  名称列表
	@param[out]   descList  说明信息列表
	@param[out]   itemCount 列表中值个数
	\return	 	 成功返回true ， 失败返回 false
	 2008/07/28 */ 
	virtual bool GetSectionList(const char* section,char ** &nameList , char ** &descList,int &itemCount) = 0;
	
public:
	static char* GetIntfName()
    {
        return IID_ICSSettings;
    }; 
};



#endif  //_ICFGINTF_H
