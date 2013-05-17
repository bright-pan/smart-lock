/*********************************************************************
 * Filename:      gprs.h
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-06 11:12:03
 *
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#ifndef _GPRS_H_
#define _GPRS_H_

#include "alarm.h"

#define GPRS_MAIL_MAX_MSGS 20

typedef struct
{
  time_t time;
  ALARM_TYPEDEF alarm_type;
}GPRS_MAIL_TYPEDEF;

extern rt_mq_t gprs_mq;

void gprs_mail_process_thread_entry(void *parameter);

#endif
