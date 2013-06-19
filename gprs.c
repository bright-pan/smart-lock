/*********************************************************************
 * Filename:      gprs.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-06 11:11:48
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "alarm.h"
#include "gprs.h"
#include "gsm.h"
#include <string.h>

rt_mq_t gprs_mq;

void gprs_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  rt_uint32_t event;
  /* malloc a buff for process mail */
  GPRS_MAIL_TYPEDEF *gprs_mail_buf = (GPRS_MAIL_TYPEDEF *)rt_malloc(sizeof(GPRS_MAIL_TYPEDEF));
  /* initial msg queue */
  gprs_mq = rt_mq_create("gprs", sizeof(GPRS_MAIL_TYPEDEF), \
                         GPRS_MAIL_MAX_MSGS, \
                         RT_IPC_FLAG_FIFO);
  while (1)
  {
    /* process mail */
    memset(gprs_mail_buf, 0, sizeof(GPRS_MAIL_TYPEDEF));
    result = rt_mq_recv(gprs_mq, gprs_mail_buf, sizeof(GPRS_MAIL_TYPEDEF), 100);
    /* mail receive ok */
    if (result == RT_EOK)
    {
      rt_kprintf("receive gprs mail < time: %d alarm_type: %d >\n", gprs_mail_buf->time, gprs_mail_buf->alarm_type);
      rt_mutex_take(mutex_gsm_mode, RT_WAITING_FOREVER);
      result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND, 100, &event);
      if (result == RT_EOK)
      {
        if (!(gsm_mode_get() & EVENT_GSM_MODE_GPRS))
        {
          rt_kprintf("\ngsm mode requset for gprs mode\n");
          rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
          rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_GPRS);
          rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &event);
          if ((result == RT_EOK) && !(gsm_mode_get() & EVENT_GSM_MODE_GPRS))
          {
            rt_kprintf("\ngsm mode switch to gprs is error, and try resend!\n");
            // clear gsm setup event, do gsm check or initial for test gsm problem.
            rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
            if (!(gsm_mode_get() & EVENT_GSM_MODE_CMD))
            {
              rt_kprintf("\ngsm mode requset for cmd mode\n");
              rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
              rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);
              result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &event);
              if ((result == RT_EOK) && !(gsm_mode_get() & EVENT_GSM_MODE_CMD))
              {
                rt_kprintf("\ngsm mode switch to cmd is error, and try resend!\n");
              }
            }
            rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);
            rt_mq_urgent(gprs_mq, gprs_mail_buf, sizeof(GPRS_MAIL_TYPEDEF));
            rt_mutex_release(mutex_gsm_mode);
            continue;
          }
        }
        // send gprs data
        rt_kprintf("\nsend gprs data!!!\n");
      }
      else// gsm is not setup
      {
        // resend gprs mail
        rt_mq_urgent(gprs_mq, gprs_mail_buf, sizeof(GPRS_MAIL_TYPEDEF));
      }
      // exit mail process
      rt_mutex_release(mutex_gsm_mode);
    }
    else
    {
      /* mail receive error */
      rt_mutex_take(mutex_gsm_mode, RT_WAITING_FOREVER);
      result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND, 100, &event);
      if (result == RT_EOK)
      {
        if (!(gsm_mode_get() & EVENT_GSM_MODE_GPRS))
        {
          rt_kprintf("\ngsm mode requset for gprs mode\n");
          rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
          rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_GPRS);
          result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &event);
          if ((result == RT_EOK) && !(gsm_mode_get() & EVENT_GSM_MODE_GPRS))
          {
            rt_kprintf("\ngsm mode switch to gprs is error\n");
            // clear gsm setup event, do gsm check or initial for test gsm problem.
            rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
            if (!(gsm_mode_get() & EVENT_GSM_MODE_CMD))
            {
            	rt_kprintf("\ngsm mode requset for cmd mode\n");
              rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
              rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);
              result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &event);
              if ((result == RT_EOK) && !(gsm_mode_get() & EVENT_GSM_MODE_CMD))
              {
                rt_kprintf("\ngsm mode switch to cmd is error, and try resend!\n");
              }
            }
            rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);
            rt_mutex_release(mutex_gsm_mode);
            continue;
          }
        }
      }
      else //gsm is not setup
      {
      }
      // exit
      rt_mutex_release(mutex_gsm_mode);
    }
  }
  rt_free(gprs_mail_buf);
}

void send_gprs_mail(ALARM_TYPEDEF alarm_type, time_t time)
{
  GPRS_MAIL_TYPEDEF buf;
  extern rt_device_t rtc_device;
  rt_err_t result;
  //send mail
  buf.alarm_type = alarm_type;
  if (!time)
  {
    rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &(buf.time));
  }
  else
  {
    buf.time = time;
  }
  if (gprs_mq != NULL)
  {
    result = rt_mq_send(gprs_mq, &buf, sizeof(GPRS_MAIL_TYPEDEF));
    if (result == -RT_EFULL)
    {
      rt_kprintf("gprs_mq is full!!!\n");
    }
  }
  else
  {
    rt_kprintf("gprs_mq is RT_NULL!!!\n");
  }
}

