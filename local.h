/*********************************************************************
 * Filename:      local.h
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-06 11:53:04
 *
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#ifndef _LOCAL_H_
#define _LOCAL_H_

#include "alarm.h"

#define LOCAL_MAIL_MAX_MSGS 20

typedef struct
{
  time_t time;
  ALARM_TYPEDEF alarm_type;
}LOCAL_MAIL_TYPEDEF;

extern rt_mq_t local_mq;

void local_mail_process_thread_entry(void *parameter);

#endif
