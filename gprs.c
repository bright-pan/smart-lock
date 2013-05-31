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
    result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND, RT_WAITING_FOREVER , &event);
    if (result == RT_EOK)
    {
      /* process mail */
      memset(gprs_mail_buf, 0, sizeof(GPRS_MAIL_TYPEDEF));
      result = rt_mq_recv(gprs_mq, gprs_mail_buf, sizeof(GPRS_MAIL_TYPEDEF), 100);
      /* mail receive ok */
      if (result == RT_EOK)
      {
        rt_kprintf("receive gprs mail < time: %d alarm_type: %d >\n", gprs_mail_buf->time, gprs_mail_buf->alarm_type);

        rt_mutex_take(mutex_gsm_mode, RT_WAITING_FOREVER);
        if (!(gsm_mode_get() & EVENT_GSM_MODE_GPRS))
        {
          rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
          rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_GPRS);
          rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, 800 , &event);
          if (result != RT_EOK)
          {
            rt_kprintf("\ngsm mode switch to gprs is error, and try resend|\n");
            rt_mq_urgent(gprs_mq, gprs_mail_buf, sizeof(GPRS_MAIL_TYPEDEF));
            rt_mutex_release(mutex_gsm_mode);
            continue;
          }
        }

        rt_kprintf("\nsend gprs data!!!\n");

        rt_mutex_release(mutex_gsm_mode);

      }
      else
      {
        /* mail receive error */
        rt_mutex_take(mutex_gsm_mode, RT_WAITING_FOREVER);        
        if (!(gsm_mode_get() & EVENT_GSM_MODE_GPRS))
        {
          rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
          rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_GPRS);
          result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, 800, &event);
          if (result != RT_EOK)
          {
            rt_kprintf("\ngsm mode switch to gprs is error\n");
          }
        }
        rt_mutex_release(mutex_gsm_mode);
      }
    }
    else
    {

    }
  }
  rt_free(gprs_mail_buf);
}
