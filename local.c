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
#include "gprs.h"
#include "gpio_pwm.h"
#include "gpio_pin.h"
#include "battery.h"
#include "untils.h"
#include "rfid_uart.h"
#include "photo.h"

/* local msg queue for local alarm */
rt_mq_t local_mq;

rt_timer_t lock_gate_timer = RT_NULL;
rt_timer_t battery_switch_timer = RT_NULL;
rt_timer_t rfid_key_detect_timer = RT_NULL;

static void lock_gate_process(void);
static void lock_gate_timeout(void *parameters);
static int32_t lock_gate_timeout_counts = 0;

static void battery_switch_timeout(void *parameters);
static void battery_switch_process(void);
static int32_t battery_switch_timeout_counts = 0;

static void rfid_key_detect_process(void);
static void rfid_key_detect_timeout(void *parameters);

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
          lock_output(GATE_LOCK);//lock
          voice_output(3);//send voice alarm
          //send mail to camera module
          camera_send_mail(local_mail_buf->alarm_type,local_mail_buf->time);
          break;
        };
        case ALARM_TYPE_LOCK_TEMPERATURE : {
          lock_output(GATE_LOCK);//lock
          voice_output(3);//send voice alarm
          //send mail to camera module
          camera_send_mail(local_mail_buf->alarm_type,local_mail_buf->time);
          break;
        };
        case ALARM_TYPE_CAMERA_IRDASENSOR : {
          lock_output(GATE_LOCK);//lock
          voice_output(3);//send voice alarm
          //send mail to camera module
         
          break;
        };
        case ALARM_TYPE_BATTERY_REMAIN_5P : {
          lock_output(GATE_UNLOCK);//unlock
          break;
        };
        case ALARM_TYPE_GSM_RING : {
          // receive call
          // receive sms
          break;
        };
        case ALARM_TYPE_BATTERY_SWITCH : {
          battery_switch_process();
          // receive sms
          break;
        };          
        case ALARM_TYPE_LOCK_GATE : {
          lock_gate_process();
          break;
        };
        case ALARM_TYPE_RFID_KEY_DETECT : {
          rfid_key_detect_process();
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

static void rfid_key_detect_process(void)
{
  rt_device_t device;
  uint8_t rfid_key[4] = {0,};
  uint8_t rfid_buf[9] = {0,};
  int8_t counts = 50;
  int8_t rfid_key_index = 0;
  int8_t rfid_error = 0;
  uint8_t dat = 0;
  uint8_t rfid_recv = 0;

  dat = gpio_pin_input(DEVICE_NAME_RFID_KEY_DETECT);
  if (dat == 1)
  {
    if (rfid_key_detect_timer == RT_NULL)
    {
      rfid_key_detect_timer = rt_timer_create("tr_rkd",
                                              rfid_key_detect_timeout,
                                              RT_NULL,
                                              6000,
                                              RT_TIMER_FLAG_PERIODIC);
      rt_timer_start(rfid_key_detect_timer);
    }
    else
    {
    	rt_timer_start(rfid_key_detect_timer);
    }
	  gpio_pin_output(DEVICE_NAME_RFID_POWER, 1);
	  device = rt_device_find(DEVICE_NAME_RFID_UART);
	  if (device != RT_NULL)
	  {
	    while (counts > 0)
	    {
	      rt_device_read(device, 0, &rfid_key, 8);// clear rfid uart cache
	      rt_device_read(device, 0, &rfid_key, 8);// clear rfid uart cache
	      rt_device_write(device, 0, "\x50\x00\x06\xD4\x07\x01\x00\x00\x00\x04\x80", 11);
	      //delay_us(100000);
	      rt_thread_delay(10);
	      rfid_recv = rt_device_read(device, 0, &rfid_buf, 9);
	      if (rfid_recv == 9)
	      {
	        rfid_key[0] = rfid_buf[4]^rfid_buf[8];
	        rfid_key[1] = rfid_buf[5]^rfid_buf[8];
	        rfid_key[2] = rfid_buf[6]^rfid_buf[8];
	        rfid_key[3] = rfid_buf[7]^rfid_buf[8];

	        rfid_key_index = RFID_KEY_NUMBERS - 1;
	        while (rfid_key_index >= 0)
	        {
	          if (device_parameters.rfid_key[rfid_key_index].flag && (*((uint32_t *)device_parameters.rfid_key[rfid_key_index].key) == *((uint32_t *)rfid_key)))
	          {
	            // success read rfid key
	            gpio_pin_output(DEVICE_NAME_RFID_POWER, 0);
	            lock_output(GATE_UNLOCK);//unlock
	            send_gprs_mail(ALARM_TYPE_RFID_KEY_SUCCESS, 0, 0);
	            return;
	          }
	          rfid_key_index--;
	        }
	      }
	      else if (rfid_recv == 0)
	      {
	        rfid_error++;
	      }
	      counts--;
	    }
	    gpio_pin_output(DEVICE_NAME_RFID_POWER, 0);
	    if (!counts)
	    {
	      // error read rfid_key
	      if (rfid_error > 20)
	      {
	        // rfid fault
	        send_sms_mail(ALARM_TYPE_RFID_FAULT, 0);
	        send_gprs_mail(ALARM_TYPE_RFID_FAULT, 0, 0);
	        return;
	      }
	      // send rfid key error mail
	      send_sms_mail(ALARM_TYPE_RFID_KEY_ERROR, 0);
	      send_gprs_mail(ALARM_TYPE_RFID_KEY_ERROR, 0, 0);
	      voice_output(3);		
	      camera_send_mail(ALARM_TYPE_RFID_KEY_ERROR,0);//camera photo 
	    }
	  }
	  else
	  {
	    rt_kprintf("device %s is not exist!\n", DEVICE_NAME_RFID_UART);
	  }
	  gpio_pin_output(DEVICE_NAME_RFID_POWER, 0);
  }
  else
  {

  }
  

}
static void rfid_key_detect_timeout(void *parameters)
{
  rt_device_t device = RT_NULL;
  uint8_t data;

  device = rt_device_find(DEVICE_NAME_RFID_KEY_DETECT);
  if (device == RT_NULL)
  {
  }
  else
  {
    rt_device_read(device,0,&data,0);
    if (data == 1) // rfid key is plugin
    {
      send_sms_mail(ALARM_TYPE_RFID_KEY_PLUGIN, 0);
      send_gprs_mail(ALARM_TYPE_RFID_KEY_PLUGIN, 0, 0);
    }
  }
  rt_timer_stop(rfid_key_detect_timer);
	//rt_timer_delete(rfid_key_detect_timer);
  //rfid_key_detect_timer = RT_NULL;
}

static void battery_switch_process(void)
{
  uint8_t dat;
  dat = gpio_pin_input(DEVICE_NAME_BATTERY_SWITCH);
  if (dat == POWER_BATTERY)
  {
    if (battery_switch_timer == RT_NULL)
    {
      battery_switch_timer = rt_timer_create("tr_bs",
                                             battery_switch_timeout,
                                             RT_NULL,
                                             6000,
                                             RT_TIMER_FLAG_PERIODIC);
      battery_switch_timeout_counts = 20;
      rt_timer_start(battery_switch_timer);
    }
    else
    {
			battery_switch_timeout_counts = 20;
			rt_timer_start(battery_switch_timer);
    }
  }
  else
  {

  }
}

static void battery_switch_timeout(void *parameters)
{
  rt_device_t device = RT_NULL;
  uint8_t data;

  device = rt_device_find(DEVICE_NAME_BATTERY_SWITCH);
  if (device == RT_NULL)
  {
    rt_timer_stop(battery_switch_timer);
    //rt_timer_delete(battery_switch_timer);
  }
  else
  {
    if (battery_switch_timeout_counts-- > 0)
    {
      rt_device_read(device,0,&data,0);
      if (data != POWER_BATTERY) // power is external
      {
        rt_timer_stop(battery_switch_timer);
        //rt_timer_delete(battery_switch_timer);
        //battery_switch_timer = RT_NULL;
        battery_switch_timeout_counts = 0;
      }
    }
    else
    {
      rt_timer_stop(battery_switch_timer);
      //rt_timer_delete(battery_switch_timer);
      //battery_switch_timer = RT_NULL;
      battery_switch_timeout_counts = 0;
      //send mail
      send_sms_mail(ALARM_TYPE_BATTERY_WORKING_20M, 0);
      send_gprs_mail(ALARM_TYPE_BATTERY_WORKING_20M, 0, 0);
    }
  }
}

static void lock_gate_process(void)
{
	gpio_pin_output(DEVICE_NAME_LOGO_LED, 1);
  if (lock_gate_timer == RT_NULL)
  {
    lock_gate_timer = rt_timer_create("tr_lg",
                                      lock_gate_timeout,
                                      RT_NULL,
                                      6000,
                                      RT_TIMER_FLAG_PERIODIC);
  }
  else
  {	
  }
  if (device_parameters.lock_gate_alarm_time < 0 || device_parameters.lock_gate_alarm_time > 99)
    {
      lock_gate_timeout_counts = 30;
    }
    else
    {
      lock_gate_timeout_counts = device_parameters.lock_gate_alarm_time;
    }
    rt_timer_start(lock_gate_timer);
}

static void lock_gate_timeout(void *parameters)
{
  rt_device_t device = RT_NULL;
  uint8_t data;

  device = rt_device_find(DEVICE_NAME_LOCK_GATE);
  if (device == RT_NULL)
  {
    rt_timer_stop(lock_gate_timer);
//    rt_timer_delete(lock_gate_timer);
    gpio_pin_output(DEVICE_NAME_LOGO_LED, 0);// close led
  }
  else
  {
    if (lock_gate_timeout_counts-- > 0)
    {
      rt_device_read(device,0,&data,0);
      if (!data) // gate is closed
      {
        rt_timer_stop(lock_gate_timer);
 //       rt_timer_delete(lock_gate_timer);
        gpio_pin_output(DEVICE_NAME_LOGO_LED, 0);// close led
        //lock_gate_timer = RT_NULL;
        lock_gate_timeout_counts = 0;
      }
    }
    else
    {
      rt_timer_stop(lock_gate_timer);
//      rt_timer_delete(lock_gate_timer);
      gpio_pin_output(DEVICE_NAME_LOGO_LED, 0);//close logo
      //lock_gate_timer = RT_NULL;
      lock_gate_timeout_counts = 0;
      //send mail
      send_sms_mail(ALARM_TYPE_LOCK_GATE, 0);
      send_gprs_mail(ALARM_TYPE_LOCK_GATE, 0, 0);
    }
  }
}

void send_local_mail(ALARM_TYPEDEF alarm_type, time_t time)
{
  LOCAL_MAIL_TYPEDEF buf;
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
  if (local_mq != NULL)
  {
    result = rt_mq_send(local_mq, &buf, sizeof(LOCAL_MAIL_TYPEDEF));
    if (result == -RT_EFULL)
    {
      rt_kprintf("local_mq is full!!!\n");
    }
  }
  else
  {
    rt_kprintf("local_mq is RT_NULL!!!\n");
  }
}

