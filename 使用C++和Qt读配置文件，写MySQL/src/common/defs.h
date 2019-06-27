//defs.h
#ifndef _ESFILESERVER_DEFS_H
#define _ESFILESERVER_DEFS_H

#ifdef _MSC_VER
#define WINDOWS_DEBUG//clw note��windows�µĵ���ģʽ�꣬��������AppmgrSyncAbort()���������ζ�ʵʱ��ȵ�
#endif

#define BUF_LEN_MIN   16
#define BUF_LEN_MAX	  256

#define PARA_INI_PATH		   "../ini/esfilesetting.ini"  //ini�ļ�·��
#define FILE_PATH              "../file"
#define FILE_PROCESSED_FOLDER  "processed"

#define FILETYPE_SHORTLOAD    "RQMXFHYC"    //��ǰ/����ĸ�߸���Ԥ��
#define FILETYPE_SHORTLOAD1   "RQZDYMXFHYC" //��ǰ/�����е�ѹĸ�߸���Ԥ��
#define FILETYPE_ULTRALOAD    "RNMXFHYC"    //����/������ĸ�߸���Ԥ��
#define FILETYPE_SHORTXNY	  "RQFDGLYC"    //��ǰ/���ڷ�繦��Ԥ��
#define FILETYPE_ULTRAXNY	  "RNFDGLYC"    //����/�����ڷ�繦��Ԥ��
#define FILETYPE_AGCRQ        "DQPLANYC"    //��ǰAGC�ƻ��ƶ�
#define FILETYPE_AGCRN	      "RNPLANYC"    //����AGC�ƻ��ƶ�

#define DIGIT_ASCII_MIN 48
#define DIGIT_ASCII_MAX 57

#define THREAD_PERIOD_MS 30000 //fileparser�߳����ڣ��ݶ�30s����һ��

#endif