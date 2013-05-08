/*********************************************************************
 * Filename:      alarm.h
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-02 14:20:45
 *
 *
 * Change Log:
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#ifndef _ALARM_H_
#define _ALARM_H_

#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include <time.h>

#define ALARM_MAIL_MAX_MSGS 20

/*
 * alarm type and items
 *
 */
typedef enum
{
  ALARM_TYPE_LK_SHELL,// lock shell alarm type
  ALARM_TYPE_LK_TEMPERATRUE,// lock temperatrue
  ALARM_TYPE_LK_GATE,// lock gate status
  ALARM_TYPE_LK_PLUGIN,// lock gate plugin alarm type
  ALARM_TYPE_LK_MOTOR,
  ALARM_TYPE_CM_COVERED,// camera covered alarm type
  ALARM_TYPE_BAT_WORKING_20M,
  ALARM_TYPE_BAT_REMAIN_50P,
  ALARM_TYPE_BAT_REMAIN_20P,
  ALARM_TYPE_BAT_REMAIN_5P,  
}ALARM_TYPEDEF;

typedef enum
{
  ALARM_TYPE_RFID_FAULT,
  ALARM_TYPE_CAMERA_FAULT,
  ALARM_TYPE_MOTOR_FAULT,
}FAULT_TYPEDEF;

/*
 *
 * alarm process flag type and items
 */
typedef enum
{
  ALARM_PROCESS_FLAG_SMS = 0x01,// sms process
  ALARM_PROCESS_FLAG_GPRS = 0x02,// gprs process
  ALARM_PROCESS_FLAG_LOCAL = 0x04,// local process
}ALARM_PROCESS_FLAG_TYPEDEF;

/*
 * alarm mail type
 *   --time: occured time
 *   --alarm_type: alarm event type
 *   --alarm_process_flag: howto process alarm mail
 */
typedef struct
{
  time_t time;
  ALARM_TYPEDEF alarm_type;
  ALARM_PROCESS_FLAG_TYPEDEF alarm_process_flag;
}ALARM_MAIL_TYPEDEF;

/*
 * alarm msg queue
 */
extern rt_mq_t alarm_mq;

/*
 * alarm mail process function
 *
 */
void alarm_mail_process_thread_entry(void *parameter);

#endif
