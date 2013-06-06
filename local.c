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
#include "gpio_exti.h"
#include "sms.h"
#include "gpio_pwm.h"
#include "gpio_pin.h"
#include "untils.h"

/* local msg queue for local alarm */
rt_mq_t local_mq;

rt_timer_t lock_gate_timer;

static void lock_gate_timeout(void *parameters);
static int32_t counts = 0;

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

      switch (local_mail_buf->alarm_type)
      {
        case ALARM_TYPE_LOCK_SHELL : {
          motor_output(1);//lock
          voice_output(2);//send voice alarm
          //send mail to camera module
          break;
        };
        case ALARM_TYPE_LOCK_TEMPERATURE : {
          motor_output(1);//lock
          voice_output(2);//send voice alarm
          //send mail to camera module
          break;
        };
        case ALARM_TYPE_CAMERA_IRDASENSOR : {
          motor_output(1);//lock
          voice_output(2);//send voice alarm
          //send mail to camera module
          break;
        };
        case ALARM_TYPE_BATTERY_REMAIN_5P : {
          motor_output(0);//unlock
          break;
        };
        case ALARM_TYPE_GSM_RING : {
          // receive call
          // receive sms
          break;
        };
        case ALARM_TYPE_LOCK_GATE : {

          gpio_pin_output(DEVICE_NAME_LOGO_LED, 1);
          lock_gate_timer = rt_timer_create("tr_lg",
                                            lock_gate_timeout,
                                            RT_NULL,
                                            6000,
                                            RT_TIMER_FLAG_PERIODIC);
          if (device_parameters.lock_gate_alarm_time < 0 || device_parameters.lock_gate_alarm_time > 99)
          {
            counts = 30;
          }
          else
          {
            counts = device_parameters.lock_gate_alarm_time;
          }
          break;
        };
      }
    }
    else
    {
      /* receive mail error */
    }
  }
  rt_free(local_mail_buf);
  
}

static void lock_gate_timeout(void *parameters)
{
  rt_device_t device = RT_NULL;
  uint8_t data;
  SMS_MAIL_TYPEDEF sms_mail_buf;

  device = rt_device_find(DEVICE_NAME_LOCK_GATE);
  if (device == RT_NULL)
  {
    rt_timer_stop(lock_gate_timer);
    rt_timer_delete(lock_gate_timer);
    gpio_pin_output(DEVICE_NAME_LOGO_LED, 0);// close led
  }
  else
  {
    if (counts-- > 0)
    {
      rt_device_read(device,0,&data,0);
      if (!data) // gate is closed
      {
        rt_timer_stop(lock_gate_timer);
        rt_timer_delete(lock_gate_timer);
        gpio_pin_output(DEVICE_NAME_LOGO_LED, 0);// close led
        lock_gate_timer = RT_NULL;
        counts = 0;
      }
    }
    else
    {
      rt_timer_stop(lock_gate_timer);
      rt_timer_delete(lock_gate_timer);
      gpio_pin_output(DEVICE_NAME_LOGO_LED, 0);//close logo
      lock_gate_timer = RT_NULL;
      counts = 0;
      //send sms mail
      sms_mail_buf.alarm_type = ALARM_TYPE_LOCK_GATE;
      rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &(sms_mail_buf.time));
      if (alarm_mq != NULL)
      {
        rt_mq_send(sms_mq, &sms_mail_buf, sizeof(SMS_MAIL_TYPEDEF));
      }
    }
  }
}


