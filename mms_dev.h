 
	/*********************************************************************
	* Filename: 	 mms.h
	*
	* Description:	This file difine mms data structure and
	* 					 function declaration  
	*
	* Author: 			 wangzw <wangzw@yuettak.com>
	* Created at: 	 2013-05-29 18:20:00
	* 							 
	* Modify:
	*
	* 
	*
	* Copyright (C) 2013 Yuettak Co.,Ltd
	********************************************************************/
 
#ifndef	__MMS_H__
#define __MMS_H__
#include "rtthread.h"
#include "rtdevice.h"
#include <dfs_init.h>
#include <dfs_elm.h>
#include <dfs_fs.h>
#include "dfs_posix.h"
 
#define PER_MMC_PIC_MAXNUM							5
#define MOBILE_NO_NUM										11
#define MMS_ERROR_TYPE_NUM							4
#define MMS_USART_DEVICE_NAME						"g_usart"

#define MMS_ERROR_DEAL_FLAG							5
#define MMS_ERROR_FLAG(arg)							(0X01 << arg)
#define MMS_ERROR_OK										0X00
#define MMS_ERROR_0_CMD									0				
#define MMS_ERROR_1_FATAL								1

 

struct mms_string
{
 rt_int8_t	 size;
 const char* string;
};
struct mms_dev
{
 rt_device_t			 usart; 													 //mms device use usart
 rt_size_t				 pic_size[PER_MMC_PIC_MAXNUM];		 //one mms picture 1,picture 2,picture 3...of size
 const char 			 *pic_name[PER_MMC_PIC_MAXNUM]; 	 //one mms picture 1,picture 2,picture 3...of name
 struct mms_string title; 													 //one mms of title
 struct mms_string text;													 	 //one mms of text 
 rt_uint8_t 			 number;													 //per mms picture sum
 rt_uint8_t 			 *mobile_no[MOBILE_NO_NUM]; 			 //send address
 rt_uint8_t 			 error; 													 //mms error result
 rt_uint8_t 			 error_record[MMS_ERROR_TYPE_NUM]; //record querytypex error num
};
typedef struct mms_dev* mms_dev_t;



void mms_timer_fun(void *arg);						//timer 
void mms_error_record_init(mms_dev_t mms);										//init error record 
void mms_get_send_file_size(mms_dev_t mms,rt_uint8_t pos); 	//mms struct data get picture size
void mms_send_fun(mms_dev_t mms);												//mms send;
void rt_mms_thread_init(void);
 
#endif
