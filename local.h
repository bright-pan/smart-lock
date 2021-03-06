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

typedef struct 
{
	rt_uint8_t key[4];
}GPRS_MAIL_USER;

extern GPRS_MAIL_USER gprs_mail_user;
extern rt_mq_t local_mq;
extern rt_timer_t lock_gate_timer;
extern rt_timer_t battery_switch_timer;

void local_mail_process_thread_entry(void *parameter);

void send_local_mail(ALARM_TYPEDEF alarm_type, time_t time);

#endif
