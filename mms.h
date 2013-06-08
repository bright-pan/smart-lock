/*********************************************************************
 * Filename:      mms.h
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

#ifndef _MMS_H_
#define _MMS_H_

#include "alarm.h"

#define MMS_MAIL_MAX_MSGS 20

typedef struct
{
  time_t time;
  ALARM_TYPEDEF alarm_type;
}MMS_MAIL_TYPEDEF;

extern rt_mq_t mms_mq;

void mms_mail_process_thread_entry(void *parameter);

#endif
