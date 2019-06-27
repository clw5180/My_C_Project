#ifndef _ICFGINTF_H
#define _ICFGINTF_H

#include "icsunknown.h"

#define MODULE_ICSSettings	"icssettings"
#define IID_ICSSettings		"iidicssettings"
#define CLSID_ICSSettings	"clsidicscfgintf"

#define MAX_STRINGLEN 255

/*!
\brief �����ļ���д�ӿ�
1.��д�����ļ���ָ�����ֵ

2.����������
*/

class IICSSettings : public IICSUnknown
{
public:
	/*!
	\brief ���ַ����ķ�����ȡ�������ֵ 
	@param[in] item ����������
	@param[in] dft  Ĭ��ֵ
	\return ���ض�ȡ�����������ֵ����������ڸ��������򷵻�Ĭ��ֵ
	*/
	//virtual const char* ItemReadString(const char* item, const char* dft = "") = 0;
	
	
	/*!
	\brief ���ַ����ķ�����ȡ�������ֵ 
	@param[in]  item      ����������
	@param[out] sValueOut ����������ֵ���ַ��������ַ
	@param[out] iSizeIn   ����������ֵ���ַ�������ĳ���
	\return �����ȡ�ɹ������Ҷ�ȡ����ֵ�ĳ���С�� iSizeIn  �򽫶�ȡ����ֵ���� sValueOut ���飬ͬʱ ���� true��
	        �����ȡ�ɹ������Ҷ�ȡ����ֵ�ĳ��ȴ��� iSizeIn  ��ֻ����ȡ����ֵ��ǰ�� ISizeIn ��1 ���ַ����� sValueOut ���飬ͬʱ����false
			�����ȡʧ�ܣ��򷵻� false
	*/
	virtual bool ItemReadString(const char* item, char * sValueOut,int iSizeIn ) = 0;
	/*!
	\brief ���غ��������ַ����ķ��������������ֵ 
	@param[in] item ����������
	@param[in] val  Ҫ���õ�ֵ
	\return ���óɹ����� true ����ʧ�ܷ��� false
	*/
	virtual bool ItemWrite(const char* item, const char* val) = 0;

	/*!
	\brief �Բ������ķ�����ȡ�������ֵ 
	@param[in] item ����������
	@param[in] dft  Ĭ��ֵ
	\return ���ض�ȡ�����������ֵ����������ڸ��������򷵻�Ĭ��ֵ
	*/
	virtual const bool ItemReadBool(const char* item, const bool dft = false) = 0;
	/*!
	\brief ���غ������Բ������ķ��������������ֵ 
	@param[in] item ����������
	@param[in] val  Ҫ���õ�ֵ
	\return ���óɹ����� true ����ʧ�ܷ��� false
	*/
	virtual bool ItemWrite(const char* item, const bool val) = 0;

	/*!
	\brief ���������ķ�����ȡ�������ֵ 
	@param[in] item ����������
	@param[in] dft  Ĭ��ֵ
	\return ���ض�ȡ�����������ֵ����������ڸ��������򷵻�Ĭ��ֵ
	*/
	virtual const int ItemReadInt(const char* item, const int dft = 0) = 0;
	/*!
	\brief ���غ��������������ķ��������������ֵ 
	@param[in] item ����������
	@param[in] val  Ҫ���õ�ֵ
	\return ���óɹ����� true ����ʧ�ܷ��� false
	*/
	virtual bool ItemWrite(const char* item, const int val) = 0;

	/*!
	\brief �Ը���ֵ�ķ�����ȡ�������ֵ 
	@param[in] item ����������
	@param[in] dft  Ĭ��ֵ
	\return ���ض�ȡ�����������ֵ����������ڸ��������򷵻�Ĭ��ֵ
	*/
	virtual const double ItemReadDouble(const char* item, const double dft = 0.0) = 0;
	/*!
	\brief ���غ������Ը���ֵ�ķ��������������ֵ 
	@param[in] item ����������
	@param[in] val  Ҫ���õ�ֵ
	\return ���óɹ����� true ����ʧ�ܷ��� false
	*/
	virtual bool ItemWrite(const char* item, const double val) = 0;
	
	/*!
	\brief �ж��Ƿ����ָ����������  
	@param[in] item ����������
	\return ���ڸ�������� true �������ڸ�������� false
	*/ 
	virtual bool ExistItem(const char* item) = 0;

	/*!
	\brief ��ȡ����������ֵ  
	@param[in] item ������������
	@param[in] dft  Ĭ��ֵ
	\return ���ظû���������ֵ����������ڸñ����򷵻�Ĭ��ֵ
	*/
	virtual const char * ReadEnv(const char* envName,const char * dft = "") = 0;

	/*!
	\brief ��ȡ�ڵ��˵�� 
	@param[in] section �ڵ�����
	@param[in] dft     Ĭ��ֵ
	\return ���ظýڵ��˵������������ڸýڵ��򷵻�Ĭ��ֵ
	*/
	//virtual const char * SectionDescRead(const char* section , char * dft = "") = 0;

	/*!
	\brief ��ȡ�ڵ��˵�� 
	@param[in]  section �ڵ�����
	@param[out] sDescOut ����˵�����ַ��������ַ
	@param[out] iSizeIn   ����˵�����ַ�������ĳ���
	\return �����ȡ�ɹ������Ҷ�ȡ����˵���ĳ���С�� iSizeIn  �򽫶�ȡ����ֵ���� sDescOut ���飬ͬʱ ���� true��
	        �����ȡ�ɹ������Ҷ�ȡ����˵���ĳ��ȴ��� iSizeIn  ��ֻ����ȡ����ֵ��ǰ�� ISizeIn ��1 ���ַ����� sDescOut ���飬ͬʱ����false
			�����ȡʧ�ܣ��򷵻� false
	*/
	virtual bool SectionDescRead(const char* section,char * sDescOut,int iSizeIn) = 0;
	/*!
	\brief ��ȡ�������˵��
	@param[in] item ����������
	@param[in] dft  Ĭ��ֵ
	*/
	//virtual const char * ItemDescRead(const char* item ,char * dft = "") = 0;

	/*!
	\brief ��ȡ�������˵�� 
	@param[in]  item      ����������
	@param[out] sDescOut  ����˵�����ַ��������ַ
	@param[out] iSizeIn   ����˵�����ַ�������ĳ���
	\return �����ȡ�ɹ������Ҷ�ȡ����˵���ĳ���С�� iSizeIn  �򽫶�ȡ����ֵ���� sDescOut ���飬ͬʱ ���� true��
	        �����ȡ�ɹ������Ҷ�ȡ����˵���ĳ��ȴ��� iSizeIn  ��ֻ����ȡ����ֵ��ǰ�� ISizeIn ��1 ���ַ����� sDescOut ���飬ͬʱ����false
			�����ȡʧ�ܣ��򷵻� false
	*/
	virtual bool ItemDescRead(const char* item,char * sDescOut,int iSizeIn) = 0;
	/*!
	\brief ���ýڵ��˵�� 
	@param[in] section �ڵ�����
	@param[in] Desc    Ҫ���õ�ֵ
	\return ���óɹ�����true������ʧ�ܷ���false
	*/
	virtual bool SectionDescWrite(const char* section , char * Desc = "") = 0;

	/*!
	\brief �����������˵�� 
	@param[in] item ����������
	@param[in] Desc Ҫ���õ�ֵ
	\return ���óɹ�����true������ʧ�ܷ���false
	*/
	virtual bool ItemDescWrite(const char* item ,char * Desc = "") = 0;

	
	/*!
	\brief ɾ��ָ����������  
	@param[in] item ����������
	\return �ɹ�����true��ʧ�ܷ���false
	*/

	virtual bool DelItem(const char* item) = 0;

	
	/*!
	\brief ɾ��ָ���Ľڵ�  
	@param[in] section �ڵ�����
	\return �ɹ�����true��ʧ�ܷ���false
	*/

	virtual bool DelSection(const char* section) = 0;

	
	/*!
	\brief �����µ�������
	��ָ��λ�ý����µ��������������������ɰ���·���� item ����ָ����
	@param[in]   item   ������������
	@param[in]   value  ���������ֵ
	@param[in]   desc   ���������˵��
	\return �ɹ�����true��ʧ�ܷ���false
	*/

	virtual bool AddItem(const char* item , char * value ,char * desc = "") = 0;

	/*!
	\brief �����µĽڵ�
	��ָ��λ�ý����µĽڵ㡣�½ڵ�������ɰ���·���� section ����ָ����
	@param[in]   section   �½ڵ�����
	@param[in]   desc      �½ڵ��˵��
	\return �ɹ�����true��ʧ�ܷ���false
	*/
	virtual bool AddSection(const char* section , char * desc = "") = 0;

	
	/*!
	\brief       ���õ�ǰ�����Ľڵ�·����������Ϻ��Ժ���������ڱ��ڵ����
	@param[in]   section �ڵ�·��
	\return	 	 �ɹ�����true��ʧ�ܷ���false
	*/
	virtual bool SetCurrentSectionPath(const char* section) = 0;

	/*!
	\brief       ���ָ���ڵ������� ֵ�������б�
	@param[in]    section   �ڵ�·��
	@param[out]   nameList  �����б�
	@param[out]   valueList �ַ���ֵ�б�
	@param[out]   itemCount �б���ֵ����
	\return	 	 �ɹ�����true �� ʧ�ܷ��� false
	*/
	virtual bool GetItemList(const char* section,char ** &nameList , char ** &valueList,int &itemCount) = 0;

	/*!
	\brief �ж��Ƿ����ָ���Ľڵ�  
	@param[in] section �ڵ�����
	\return ���ڸýڵ㷵�� true �������ڸýڵ㷵�� false
	*/ 
	virtual bool ExistSection(const char* section) = 0;
	
	/*!
	\brief ָ�������ļ����ļ��� 
	@param[in] fileName �����ļ�������ָ�����ļ�����ʹ��Ĭ������ ��icssettings.xml�� �� 
	                    �ò���ֻ����ָ��һ�������ļ����ļ����������Ʋ�Ӧ����·�����Ը��ļ��ķ���˳��� Ĭ���ļ�һ��
	\return �ɹ����� true ��ʧ�ܷ��� false
	 */ 
	virtual bool SetCfgFileName(const char* fileName) = 0;

	/*!
	\brief       ���ָ���ڵ��������ӽڵ��б�
	@param[in]    section   �ڵ�·��
	@param[out]   nameList  �����б�
	@param[out]   descList  ˵����Ϣ�б�
	@param[out]   itemCount �б���ֵ����
	\return	 	 �ɹ�����true �� ʧ�ܷ��� false
	 2008/07/28 */ 
	virtual bool GetSectionList(const char* section,char ** &nameList , char ** &descList,int &itemCount) = 0;
	
public:
	static char* GetIntfName()
    {
        return IID_ICSSettings;
    }; 
};



#endif  //_ICFGINTF_H
