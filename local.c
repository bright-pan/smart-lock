/*********************************************************************
 * Filename:      local.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-06 11:52:53
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "local.h"

/* local msg queue for local alarm */
rt_mq_t local_mq;

void local_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  /* malloc a buffer for local mail process */
  LOCAL_MAIL_TYPEDEF *local_mail_buf = (LOCAL_MAIL_TYPEDEF *)rt_malloc(sizeof(LOCAL_MAIL_TYPEDEF));
  /* initial msg queue for local alarm */
  local_mq = rt_mq_create("local", sizeof(LOCAL_MAIL_TYPEDEF), LOCAL_MAIL_MAX_MSGS, RT_IPC_FLAG_FIFO);
  
  while (1)
  {
    /* receive mail */
    result = rt_mq_recv(local_mq, local_mail_buf, sizeof(LOCAL_MAIL_TYPEDEF), RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
      /* process mail */
      rt_kprintf("receive local mail < time: %d alarm_type: %d >\n", local_mail_buf->time, local_mail_buf->alarm_type);
    }
    else
    {
      break;
      /* receive mail error */
    }
  }
  rt_free(local_mail_buf);
  
}
