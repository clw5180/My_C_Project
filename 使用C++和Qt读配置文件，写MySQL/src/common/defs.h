//defs.h
#ifndef _ESFILESERVER_DEFS_H
#define _ESFILESERVER_DEFS_H

#ifdef _MSC_VER
#define WINDOWS_DEBUG//clw note：windows下的调试模式宏，用于屏蔽AppmgrSyncAbort()函数，屏蔽读实时库等等
#endif

#define BUF_LEN_MIN   16
#define BUF_LEN_MAX	  256

#define PARA_INI_PATH		   "../ini/esfilesetting.ini"  //ini文件路径
#define FILE_PATH              "../file"
#define FILE_PROCESSED_FOLDER  "processed"

#define FILETYPE_SHORTLOAD    "RQMXFHYC"    //日前/短期母线负荷预测
#define FILETYPE_SHORTLOAD1   "RQZDYMXFHYC" //日前/短期中低压母线负荷预测
#define FILETYPE_ULTRALOAD    "RNMXFHYC"    //日内/超短期母线负荷预测
#define FILETYPE_SHORTXNY	  "RQFDGLYC"    //日前/短期风电功率预测
#define FILETYPE_ULTRAXNY	  "RNFDGLYC"    //日内/超短期风电功率预测
#define FILETYPE_AGCRQ        "DQPLANYC"    //日前AGC计划制定
#define FILETYPE_AGCRN	      "RNPLANYC"    //日内AGC计划制定

#define DIGIT_ASCII_MIN 48
#define DIGIT_ASCII_MAX 57

#define THREAD_PERIOD_MS 30000 //fileparser线程周期，暂定30s处理一次

#endif