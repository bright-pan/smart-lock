/*********************************************************************
 * Filename:      mms.c
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
#include "mms.h"
#include "gsm.h"

rt_mq_t mms_mq;

void mms_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  rt_uint32_t event;
  /* malloc a buff for process mail */
  MMS_MAIL_TYPEDEF *mms_mail_buf = (MMS_MAIL_TYPEDEF *)rt_malloc(sizeof(MMS_MAIL_TYPEDEF));
  /* initial msg queue */
  mms_mq = rt_mq_create("mms", sizeof(MMS_MAIL_TYPEDEF), \
                         MMS_MAIL_MAX_MSGS, \
                         RT_IPC_FLAG_FIFO);
  while (1)
  {
    /* process mail */
    result = rt_mq_recv(mms_mq, mms_mail_buf, sizeof(MMS_MAIL_TYPEDEF), RT_WAITING_FOREVER);

    /* mail receive ok */
    if (result == RT_EOK)
    {
      rt_kprintf("receive mms mail < time: %d alarm_type: %d >\n", mms_mail_buf->time, mms_mail_buf->alarm_type);

      rt_mutex_take(mutex_gsm_mode, RT_WAITING_FOREVER);
      if (gsm_mode_get() & EVENT_GSM_MODE_GPRS)
      {
        rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_GPRS_CMD);
        rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER , &event);
      }

      rt_kprintf("\nsend mms data!!!\n");
      
      if (!(gsm_mode_get() & EVENT_GSM_MODE_GPRS))
      {
        rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_GPRS);
        rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER , &event);
      }
      rt_mutex_release(mutex_gsm_mode);

    }
    else
    {
      /* mail receive error */
    }
  }
  rt_free(mms_mail_buf);
}
