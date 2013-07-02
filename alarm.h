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

#define ALARM_INTERVAL 10
/*
 * alarm type and items
 *
 */
typedef enum
{
  ALARM_TYPE_LOCK_SHELL,// lock shell alarm type
  ALARM_TYPE_LOCK_TEMPERATURE,// lock temperatrue
  ALARM_TYPE_GATE_TEMPERATURE,// lock temperatrue
  ALARM_TYPE_LOCK_GATE,// lock gate status
  ALARM_TYPE_GSM_RING,// lock gate status
  ALARM_TYPE_RFID_KEY_DETECT,// rfid key detect alarm type
  ALARM_TYPE_CAMERA_PHOTOSENSOR, // camera photo sensor
  ALARM_TYPE_CAMERA_IRDASENSOR, // camera irda sensor
  ALARM_TYPE_MOTOR_STATUS, // motor status sensor
  ALARM_TYPE_BATTERY_WORKING_20M,
  ALARM_TYPE_BATTERY_REMAIN_50P,
  ALARM_TYPE_BATTERY_REMAIN_20P,
  ALARM_TYPE_BATTERY_REMAIN_5P,
  ALARM_TYPE_BATTERY_SWITCH,
  ALARM_TYPE_RFID_KEY_ERROR,// rfid key detect error alarm type 14
  ALARM_TYPE_RFID_KEY_SUCCESS,// rfid key detect success alarm type
  ALARM_TYPE_RFID_KEY_PLUGIN,// rfid key detect plugin alarm type
  ALARM_TYPE_RFID_FAULT,
  ALARM_TYPE_CAMERA_FAULT,
  ALARM_TYPE_MOTOR_FAULT,
  ALARM_TYPE_POWER_FAULT,
  ALARM_TYPE_GPRS_AUTH,
  ALARM_TYPE_GPRS_HEART,
  ALARM_TYPE_GPRS_LIST_TELEPHONE,
  ALARM_TYPE_GPRS_LIST_RFID_KEY,
  ALARM_TYPE_GPRS_SET_TELEPHONE_SUCCESS,
  ALARM_TYPE_GPRS_SET_TELEPHONE_FAILURE,
  ALARM_TYPE_GPRS_SET_RFID_KEY_SUCCESS,
  ALARM_TYPE_GPRS_SET_RFID_KEY_FAILURE,
  ALARM_TYPE_GPRS_LIST_USER_PARAMETERS,
  ALARM_TYPE_GPRS_SET_USER_PARAMETERS_SUCCESS,
  ALARM_TYPE_GPRS_SET_USER_PARAMETERS_FAILURE,//31
  ALARM_TYPE_GPRS_SET_TIME_SUCCESS,
  ALARM_TYPE_GPRS_SET_TIME_FAILURE,
  ALARM_TYPE_GPRS_SET_KEY0_SUCCESS,
  ALARM_TYPE_GPRS_SET_KEY0_FAILURE,
}ALARM_TYPEDEF;

/*
 *
 * alarm process flag type and items
 */
typedef enum
{
  ALARM_PROCESS_FLAG_SMS = 0x01,// sms process
  ALARM_PROCESS_FLAG_GPRS = 0x02,// gprs process
  ALARM_PROCESS_FLAG_LOCAL = 0x04,// local process
  ALARM_PROCESS_FLAG_MMS = 0x08,// local process
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
  rt_int8_t gpio_value;
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
void send_alarm_mail(ALARM_TYPEDEF alarm_type, ALARM_PROCESS_FLAG_TYPEDEF alarm_process_flag, rt_int8_t gpio_value, time_t time);
#endif
