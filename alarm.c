/*********************************************************************
 * Filename:      alarm.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-02 14:33:16
 *
 *                
 * Change Log:
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "alarm.h"
#include "sms.h"
#include "gprs.h"
#include "mms.h"
#include "local.h"
#include <wchar.h>
#include <locale.h>
#include <stdlib.h>

rt_mq_t alarm_mq = RT_NULL;

const char *alarm_help_map[] = {
  "ALARM_TYPE_LOCK_SHELL",// lock shell alarm type
  "ALARM_TYPE_LOCK_TEMPERATURE",// lock temperatrue
  "ALARM_TYPE_GATE_TEMPERATURE",// lock temperatrue
  "ALARM_TYPE_LOCK_GATE",// lock gate status
  "ALARM_TYPE_GSM_RING",// lock gate status
  "ALARM_TYPE_RFID_KEY_DETECT",// rfid key detect alarm type
  "ALARM_TYPE_CAMERA_PHOTOSENSOR", // camera photo sensor
  "ALARM_TYPE_CAMERA_IRDASENSOR", // camera irda sensor
  "ALARM_TYPE_MOTOR_STATUS", // motor status sensor
  "ALARM_TYPE_BATTERY_WORKING_20M",
  "ALARM_TYPE_BATTERY_REMAIN_50P",
  "ALARM_TYPE_BATTERY_REMAIN_20P",
  "ALARM_TYPE_BATTERY_REMAIN_5P",
  "ALARM_TYPE_BATTERY_SWITCH",
  "ALARM_TYPE_RFID_KEY_ERROR",// rfid key detect error alarm type 14
  "ALARM_TYPE_RFID_KEY_SUCCESS",// rfid key detect success alarm type
  "ALARM_TYPE_RFID_KEY_PLUGIN",// rfid key detect plugin alarm type
  "ALARM_TYPE_RFID_FAULT",
  "ALARM_TYPE_CAMERA_FAULT",
  "ALARM_TYPE_MOTOR_FAULT",
  "ALARM_TYPE_POWER_FAULT",
  "ALARM_TYPE_GPRS_AUTH",
  "ALARM_TYPE_GPRS_HEART",
  "ALARM_TYPE_GPRS_LIST_TELEPHONE",
  "ALARM_TYPE_GPRS_LIST_RFID_KEY",
  "ALARM_TYPE_GPRS_SET_TELEPHONE_SUCCESS",
  "ALARM_TYPE_GPRS_SET_TELEPHONE_FAILURE",
  "ALARM_TYPE_GPRS_SET_RFID_KEY_SUCCESS",
  "ALARM_TYPE_GPRS_SET_RFID_KEY_FAILURE",
  "ALARM_TYPE_GPRS_LIST_USER_PARAMETERS",
  "ALARM_TYPE_GPRS_SET_USER_PARAMETERS_SUCCESS",
  "ALARM_TYPE_GPRS_SET_USER_PARAMETERS_FAILURE",//31
  "ALARM_TYPE_GPRS_SET_TIME_SUCCESS",
  "ALARM_TYPE_GPRS_SET_TIME_FAILURE",
  "ALARM_TYPE_GPRS_SET_KEY0_SUCCESS",
  "ALARM_TYPE_GPRS_SET_KEY0_FAILURE",
  "ALARM_TYPE_GPRS_SET_HTTP_SUCCESS",
  "ALARM_TYPE_GPRS_SET_HTTP_FAILURE",
  "ALARM_TYPE_GPRS_UPLOAD_PIC",
  "ALARM_TYPE_GPRS_SEND_PIC_DATA",
};
//char s[512];
void alarm_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  ALARM_MAIL_TYPEDEF alarm_mail_buf;
  SMS_MAIL_TYPEDEF sms_mail_buf;
  GPRS_MAIL_TYPEDEF gprs_mail_buf;
  LOCAL_MAIL_TYPEDEF local_mail_buf;
  //  MMS_MAIL_TYPEDEF mms_mail_buf;
  
  while (1)
  {
    result = rt_mq_recv(alarm_mq, &alarm_mail_buf, sizeof(ALARM_MAIL_TYPEDEF), RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
      if (alarm_mail_buf.alarm_process_flag & ALARM_PROCESS_FLAG_SMS)
      {
        /* produce mail */
        sms_mail_buf.time = alarm_mail_buf.time;
        sms_mail_buf.alarm_type = alarm_mail_buf.alarm_type;
        /* send to sms_mq */
        rt_mq_send(sms_mq, &sms_mail_buf, sizeof(SMS_MAIL_TYPEDEF));
      }
      if (alarm_mail_buf.alarm_process_flag & ALARM_PROCESS_FLAG_GPRS)
      {
        /* produce mail */
        gprs_mail_buf.time = alarm_mail_buf.time;
        gprs_mail_buf.alarm_type = alarm_mail_buf.alarm_type;
        /* send to gprs_mq */
        rt_mq_send(gprs_mq, &gprs_mail_buf, sizeof(GPRS_MAIL_TYPEDEF));
      }
      //if (alarm_mail_buf.alarm_process_flag & ALARM_PROCESS_FLAG_MMS)
      {
        /* produce mail */
        //mms_mail_buf.time = alarm_mail_buf.time;
        //mms_mail_buf.alarm_type = alarm_mail_buf.alarm_type;
        /* send to mms_mq */
        //rt_mq_send(mms_mq, &mms_mail_buf, sizeof(MMS_MAIL_TYPEDEF));
      }
      if (alarm_mail_buf.alarm_process_flag & ALARM_PROCESS_FLAG_LOCAL)
      {
        /* produce mail */
        local_mail_buf.time = alarm_mail_buf.time;
        local_mail_buf.alarm_type = alarm_mail_buf.alarm_type;
        rt_mq_send(local_mq, &local_mail_buf, sizeof(LOCAL_MAIL_TYPEDEF));
      }
    }
    else
    {
      /* msg receive error */
    }
  } 
}

void send_alarm_mail(ALARM_TYPEDEF alarm_type, ALARM_PROCESS_FLAG_TYPEDEF alarm_process_flag, rt_int8_t gpio_value, time_t time)
{
  extern rt_device_t rtc_device;
  ALARM_MAIL_TYPEDEF buf;
  rt_err_t result;
  buf.alarm_type = alarm_type;
  if (!time)
  {
    rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &(buf.time));
  }
  else
  {
    buf.time = time;
  }
  buf.alarm_process_flag = alarm_process_flag;
  buf.gpio_value = gpio_value;
  if (alarm_mq != NULL)
  {
    result = rt_mq_send(alarm_mq, &buf, sizeof(ALARM_MAIL_TYPEDEF));
    if (result == -RT_EFULL)
    {
      rt_kprintf("alarm_mq is full!!!\n");
    }
  }
  else
  {
    rt_kprintf("alarm_mq is RT_NULL!!!\n");
  }
}

