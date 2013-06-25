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

rt_mq_t alarm_mq;
//char s[512];
void alarm_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  /* malloc a buff for process mail */
  ALARM_MAIL_TYPEDEF *alarm_mail_buf = (ALARM_MAIL_TYPEDEF *)rt_malloc(sizeof(ALARM_MAIL_TYPEDEF));
  SMS_MAIL_TYPEDEF *sms_mail_buf = (SMS_MAIL_TYPEDEF *)rt_malloc(sizeof(SMS_MAIL_TYPEDEF));
  GPRS_MAIL_TYPEDEF *gprs_mail_buf = (GPRS_MAIL_TYPEDEF *)rt_malloc(sizeof(GPRS_MAIL_TYPEDEF));
  LOCAL_MAIL_TYPEDEF *local_mail_buf = (LOCAL_MAIL_TYPEDEF *)rt_malloc(sizeof(LOCAL_MAIL_TYPEDEF));
  MMS_MAIL_TYPEDEF *mms_mail_buf = (MMS_MAIL_TYPEDEF *)rt_malloc(sizeof(MMS_MAIL_TYPEDEF));
  /* initial alarm msg queue */
  alarm_mq = rt_mq_create("alarm", sizeof(ALARM_MAIL_TYPEDEF), ALARM_MAIL_MAX_MSGS, RT_IPC_FLAG_FIFO);
  
  while (1)
  {
    result = rt_mq_recv(alarm_mq, alarm_mail_buf, sizeof(ALARM_MAIL_TYPEDEF), RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
      if (alarm_mail_buf->alarm_process_flag & ALARM_PROCESS_FLAG_SMS)
      {
        send_sms_mail(alarm_mail_buf->alarm_type, alarm_mail_buf->time);
      }
      if (alarm_mail_buf->alarm_process_flag & ALARM_PROCESS_FLAG_GPRS)
      {
				send_gprs_mail(alarm_mail_buf->alarm_type, alarm_mail_buf->time, 0);
      }
      if (alarm_mail_buf->alarm_process_flag & ALARM_PROCESS_FLAG_MMS)
      {
      }
      if (alarm_mail_buf->alarm_process_flag & ALARM_PROCESS_FLAG_LOCAL)
      {
        send_local_mail(alarm_mail_buf->alarm_type, alarm_mail_buf->time);
      }
    }
    else
    {
      /* msg receive error */
    }
  } 
  /* free process buf */
  rt_free(alarm_mail_buf);
  rt_free(sms_mail_buf);
  rt_free(gprs_mail_buf);
  rt_free(local_mail_buf);
  rt_free(mms_mail_buf);
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

