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
#include "local.h"
#include <wchar.h>
#include <locale.h>
#include <stdlib.h>

rt_mq_t alarm_mq;
char s[512];
void alarm_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  /* malloc a buff for process mail */
  ALARM_MAIL_TYPEDEF *alarm_mail_buf = (ALARM_MAIL_TYPEDEF *)rt_malloc(sizeof(ALARM_MAIL_TYPEDEF));
  SMS_MAIL_TYPEDEF *sms_mail_buf = (SMS_MAIL_TYPEDEF *)rt_malloc(sizeof(SMS_MAIL_TYPEDEF));
  GPRS_MAIL_TYPEDEF *gprs_mail_buf = (GPRS_MAIL_TYPEDEF *)rt_malloc(sizeof(GPRS_MAIL_TYPEDEF));
  LOCAL_MAIL_TYPEDEF *local_mail_buf = (LOCAL_MAIL_TYPEDEF *)rt_malloc(sizeof(LOCAL_MAIL_TYPEDEF));
  /* initial alarm msg queue */
  alarm_mq = rt_mq_create("alarm", sizeof(ALARM_MAIL_TYPEDEF), ALARM_MAIL_MAX_MSGS, RT_IPC_FLAG_FIFO);
  
  while (1)
  {
    result = rt_mq_recv(alarm_mq, alarm_mail_buf, sizeof(ALARM_MAIL_TYPEDEF), RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
      if (alarm_mail_buf->alarm_process_flag & ALARM_PROCESS_FLAG_SMS)
      {
        /* produce mail */
        sms_mail_buf->time = alarm_mail_buf->time;
        sms_mail_buf->alarm_type = alarm_mail_buf->alarm_type;
        /* send to sms_mq */
        rt_mq_send(sms_mq, sms_mail_buf, sizeof(SMS_MAIL_TYPEDEF));
      }
      if (alarm_mail_buf->alarm_process_flag & ALARM_PROCESS_FLAG_GPRS)
      {
        /* produce mail */
        gprs_mail_buf->time = alarm_mail_buf->time;
        gprs_mail_buf->alarm_type = alarm_mail_buf->alarm_type;
        /* send to gprs_mq */
        rt_mq_send(gprs_mq, gprs_mail_buf, sizeof(GPRS_MAIL_TYPEDEF));
      }
      if (alarm_mail_buf->alarm_process_flag & ALARM_PROCESS_FLAG_LOCAL)
      {
        /* produce mail */
        local_mail_buf->time = alarm_mail_buf->time;
        local_mail_buf->alarm_type = alarm_mail_buf->alarm_type;
        /* send to local_mq */
        rt_mq_send(local_mq, local_mail_buf, sizeof(LOCAL_MAIL_TYPEDEF));
        /*        rt_kprintf("send msg in local, mail - %d, %x, %x\n", alarm_mail_buf->time, alarm_mail_buf->alarm_type, alarm_mail_buf->alarm_process_flag);
        rt_kprintf("\n%s\n", setlocale( LC_ALL, "zh_CN.gbk"));
        rt_kprintf("\n%d\n", sizeof(wchar_t));
        const wchar_t string[] = L"工作愉快！";
        //        iconv_t cd = iconv_open("UTF-8", "ISO-8859-1");
        rt_kprintf("\n%d\n", sizeof(string));
        
        rt_kprintf("\n%d\n", sizeof(string));
        int i = 0;
        while (i < sizeof(string))
        {
          rt_kprintf("%x\n",((char *)string)[i]);
          i++;
        }

        wcstombs(s, string, sizeof(string));
        i = 0;
        while (i < sizeof(string))
        {
          rt_kprintf("%x\n", s[i]);
          i++;
        }
        
        */
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
}