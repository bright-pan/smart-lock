/*********************************************************************
 * Filename:      sms.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-06 09:33:10
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "sms.h"
#include "alarm.h"

rt_mq_t sms_mq;

void sms_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;  
  /* malloc a buff for process mail */
  SMS_MAIL_TYPEDEF *sms_mail_buf = (SMS_MAIL_TYPEDEF *)rt_malloc(sizeof(SMS_MAIL_TYPEDEF));
  /* initial msg queue */
  sms_mq = rt_mq_create("sms", sizeof(SMS_MAIL_TYPEDEF), \
                        SMS_MAIL_MAX_MSGS, \
                        RT_IPC_FLAG_FIFO);

  while (1)
  {
    
    /* process mail */
    result = rt_mq_recv(sms_mq, sms_mail_buf, \
                        sizeof(SMS_MAIL_TYPEDEF), \
                        RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
      rt_kprintf("receive sms mail < time: %d alarm_type: %d >\n", sms_mail_buf->time, sms_mail_buf->alarm_type);
    }
    else
    {
      /* mail receive error */
    }
  }
  rt_free(sms_mail_buf);
}
