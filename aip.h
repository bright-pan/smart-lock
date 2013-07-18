/*********************************************************************
 * Filename:      aip.h
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-07-17 11:12:03
 *
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#ifndef _AIP_H_
#define _AIP_H_

#include <rthw.h>
#include <rtthread.h>
#include "stm32f10x.h"
#include "rtdevice.h"
#include "alarm.h"
#include <dfs_init.h>
#include <dfs_elm.h>
#include <dfs_fs.h>
#include "dfs_posix.h"

#define AIP_MAIL_MAX_MSGS 20

typedef struct
{
  time_t time;
  ALARM_TYPEDEF alarm_type;
}AIP_MAIL_TYPEDEF;

extern rt_mq_t aip_mq;
extern uint8_t aip_bin_http_address[100];
extern uint8_t aip_bin_http_address_length;
extern int aip_bin_size;
void aip_mail_process_thread_entry(void *parameter);

#endif
