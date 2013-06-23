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
#include "gpio_exti.h"
#include "gsm.h"
#include "gsm_usart.h"
#include "untils.h"
#include "des.h"
#include <string.h>

#define WORK_ALARM_CAMERA_IRDASENSOR (1 << 0)
#define WORK_ALARM_LOCK_SHELL (1 << 1)
#define WORK_ALARM_RFID_KEY_ERROR (1 << 2)
#define WORK_ALARM_LOCK_TEMPERATURE (1 << 3)
#define WORK_ALARM_RFID_KEY_PLUGIN (1 << 5)
#define WORK_ALARM_LOCK_GATE (1 << 6)

#define FAULT_ALARM_CAMERA_FAULT (1 << 0)
#define FAULT_ALARM_RFID_FAULT (1 << 1)
#define FAULT_ALARM_POWER_FAULT (1 << 2)
#define FAULT_ALARM_MOTOR_FAULT (1 << 6)

rt_mq_t gprs_mq;
static uint8_t gprs_order = 0;
static uint16_t work_alarm_type_map[50] = {0, };
static uint16_t fault_alarm_type_map[50] = {0, };

void work_alarm_type_map_init(void)
{
  work_alarm_type_map[ALARM_TYPE_LOCK_SHELL] = WORK_ALARM_LOCK_SHELL;
  work_alarm_type_map[ALARM_TYPE_CAMERA_IRDASENSOR] = WORK_ALARM_CAMERA_IRDASENSOR;
  work_alarm_type_map[ALARM_TYPE_RFID_KEY_ERROR] = WORK_ALARM_RFID_KEY_ERROR;
  work_alarm_type_map[ALARM_TYPE_LOCK_TEMPERATURE] = WORK_ALARM_LOCK_TEMPERATURE;
  work_alarm_type_map[ALARM_TYPE_RFID_KEY_PLUGIN] = WORK_ALARM_RFID_KEY_PLUGIN;
  work_alarm_type_map[ALARM_TYPE_LOCK_GATE] = WORK_ALARM_LOCK_GATE;
}

void fault_alarm_type_map_init(void)
{
  fault_alarm_type_map[ALARM_TYPE_CAMERA_FAULT] = FAULT_ALARM_CAMERA_FAULT;
  fault_alarm_type_map[ALARM_TYPE_RFID_FAULT] = FAULT_ALARM_RFID_FAULT;
  fault_alarm_type_map[ALARM_TYPE_POWER_FAULT] = FAULT_ALARM_POWER_FAULT;
  fault_alarm_type_map[ALARM_TYPE_MOTOR_FAULT] = FAULT_ALARM_MOTOR_FAULT;
}

int8_t recv_gprs_frame(GPRS_RECV_FRAME_TYPEDEF *recv_frame);

void create_k1(void)
{
  uint8_t index = 0;
  time_t time;
  extern rt_device_t rtc_device;

  /* produce mail */
  while (index++ < 8)
  {
    rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
    srand(time);
    device_parameters.key1[index] = (rand() + device_parameters.key1[index]) % 0xff;
    rt_thread_delay(100);
  }
}

void gprs_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  rt_uint32_t event;
  GPRS_RECV_FRAME_TYPEDEF gprs_recv_frame;
  /* malloc a buff for process mail */
  GPRS_MAIL_TYPEDEF *gprs_mail_buf = (GPRS_MAIL_TYPEDEF *)rt_malloc(sizeof(GPRS_MAIL_TYPEDEF));

  create_k1();
  work_alarm_type_map_init();
  fault_alarm_type_map_init();
  /* initial msg queue */
  gprs_mq = rt_mq_create("gprs", sizeof(GPRS_MAIL_TYPEDEF), \
                         GPRS_MAIL_MAX_MSGS, \
                         RT_IPC_FLAG_FIFO);
  while (1)
  {
    /* process mail */
    memset(gprs_mail_buf, 0, sizeof(GPRS_MAIL_TYPEDEF));
    result = rt_mq_recv(gprs_mq, gprs_mail_buf, sizeof(GPRS_MAIL_TYPEDEF), RT_WAITING_NO);
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
          }
        }
        else// gprs mode
        {
          recv_gprs_frame(&gprs_recv_frame);
          rt_thread_delay(50);
        }
      }
      else //gsm is not setup
      {
        rt_thread_delay(50);
      }
      // exit
      rt_mutex_release(mutex_gsm_mode);
    }
  }
  rt_free(gprs_mail_buf);
}

void gprs_heart_process_thread_entry(void *parameters)
{
  rt_err_t result;
  rt_uint32_t event;

  while(1)
  {
    rt_mutex_take(mutex_gsm_mode, RT_WAITING_FOREVER);
    result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND, 100, &event);
    if (result == RT_EOK)
    {
      if (gsm_mode_get() & EVENT_GSM_MODE_GPRS)
      {
        //send heart
        gprs_send_heart();
      }
    }
    else //gsm is not setup
    {
      
    }
    // exit
    rt_mutex_release(mutex_gsm_mode);
    rt_thread_delay(6000);
  }
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

void auth_process(GPRS_AUTH *auth)
{
  uint8_t input_buf[8], output_buf[8];
  des_context ctx_key0_enc, ctx_key1_enc;

  // process device_id
  memcpy(auth->device_id, device_parameters.device_id, 6);
  // set key for DES
  des_setkey_enc(&ctx_key0_enc, (unsigned char *)device_parameters.key0);
  des_setkey_enc(&ctx_key1_enc, (unsigned char *)device_parameters.key1);
  // DES(device_id, key1)
  memset(input_buf, 0, 8);
  memcpy(input_buf, auth->device_id, 6);
  des_crypt_ecb(&ctx_key1_enc, input_buf, output_buf);
  memcpy(auth->enc_data + 8, output_buf, 8);
  // DES(key1, key0)
  memcpy(input_buf, device_parameters.key1, 8);
  des_crypt_ecb(&ctx_key0_enc, input_buf, output_buf);
  memcpy(auth->enc_data, output_buf, 8);
  // DES(DES(device_id, key1), key0)
  memcpy(input_buf, auth->enc_data + 8, 8);
  des_crypt_ecb(&ctx_key0_enc, input_buf, output_buf);
  memcpy(auth->enc_data + 8, output_buf, 8);
}

uint8_t bcd(uint8_t dec)
{
  uint8_t temp = dec % 100;
  return ((temp / 10)<<4) + ((temp % 10) & 0x0F); 
}

void work_alarm_process(ALARM_TYPEDEF alarm_type, GPRS_WORK_ALARM *work_alarm, time_t time)
{
  rt_device_t device_motor_status = RT_NULL;
  uint8_t dat = 0;
  struct tm tm_time;
  
  device_motor_status = rt_device_find(DEVICE_NAME_MOTOR_STATUS);
  
  work_alarm->type = work_alarm_type_map[alarm_type];

  rt_device_read(device_motor_status,0,&dat,0);
  work_alarm->status = dat;

  tm_time = *localtime(&time);
  tm_time.tm_year += 1900;
  tm_time.tm_mon += 1;

  work_alarm->time[0] = bcd((uint8_t)(tm_time.tm_year % 100));
  work_alarm->time[1] = bcd((uint8_t)(tm_time.tm_mon % 100));
  work_alarm->time[2] = bcd((uint8_t)(tm_time.tm_mday % 100));
  work_alarm->time[3] = bcd((uint8_t)(tm_time.tm_hour % 100));
  work_alarm->time[4] = bcd((uint8_t)(tm_time.tm_min % 100));
  work_alarm->time[5] = bcd((uint8_t)(tm_time.tm_sec % 100));  
}

void fault_alarm_process(ALARM_TYPEDEF alarm_type, GPRS_FAULT_ALARM *fault_alarm, time_t time)
{
  struct tm tm_time;

  fault_alarm->type = fault_alarm_type_map[alarm_type];

  tm_time = *localtime(&time);
  tm_time.tm_year += 1900;
  tm_time.tm_mon += 1;

  fault_alarm->time[0] = bcd((uint8_t)(tm_time.tm_year % 100));
  fault_alarm->time[1] = bcd((uint8_t)(tm_time.tm_mon % 100));
  fault_alarm->time[2] = bcd((uint8_t)(tm_time.tm_mday % 100));
  fault_alarm->time[3] = bcd((uint8_t)(tm_time.tm_hour % 100));
  fault_alarm->time[4] = bcd((uint8_t)(tm_time.tm_min % 100));
  fault_alarm->time[5] = bcd((uint8_t)(tm_time.tm_sec % 100));
}

int8_t send_gprs_frame(ALARM_TYPEDEF alarm_type, time_t time)
{

  uint8_t *process_buf = RT_NULL;
  uint8_t *process_buf_bk = RT_NULL;
  uint16_t process_length = 0;
  GPRS_SEND_FRAME_TYPEDEF *gprs_send_frame = RT_NULL;
  rt_device_t device = RT_NULL;

  device = rt_device_find(DEVICE_NAME_GSM_USART);

  if (device == RT_NULL)
  {
    rt_kprintf("device %s is not exist!\n", DEVICE_NAME_GSM_USART);
    return -1;
  }

  gprs_send_frame = (GPRS_SEND_FRAME_TYPEDEF *)rt_malloc(sizeof(GPRS_SEND_FRAME_TYPEDEF));
  memset(gprs_send_frame, 0, sizeof(GPRS_SEND_FRAME_TYPEDEF));
  // frame process
  switch (alarm_type)
  {
    case ALARM_TYPE_GPRS_AUTH : {

      gprs_send_frame->length = 0x16;
      gprs_send_frame->cmd = 0x11;
      gprs_send_frame->order = gprs_order++;

      auth_process(&(gprs_send_frame->auth));

      break;
    };
    case ALARM_TYPE_GPRS_HEART : {

      gprs_send_frame->length = 0x1;
      gprs_send_frame->cmd = 0x00;
      gprs_send_frame->order = gprs_order++;

      gprs_send_frame->heart.type = 0x02;

      break;
    };
    case ALARM_TYPE_CAMERA_IRDASENSOR: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x3;
      gprs_send_frame->order = gprs_order++;

      work_alarm_process(ALARM_TYPE_CAMERA_IRDASENSOR, &(gprs_send_frame->work_alarm), time);

      break;
    };
    case ALARM_TYPE_LOCK_SHELL: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x3;
      gprs_send_frame->order = gprs_order++;

      work_alarm_process(ALARM_TYPE_LOCK_SHELL, &(gprs_send_frame->work_alarm), time);

      break;
    };
    case ALARM_TYPE_LOCK_GATE: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x3;
      gprs_send_frame->order = gprs_order++;

      work_alarm_process(ALARM_TYPE_LOCK_GATE, &(gprs_send_frame->work_alarm), time);

      break;
    };
    case ALARM_TYPE_LOCK_TEMPERATURE: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x3;
      gprs_send_frame->order = gprs_order++;

      work_alarm_process(ALARM_TYPE_LOCK_TEMPERATURE, &(gprs_send_frame->work_alarm), time);

      break;
    };
    case ALARM_TYPE_RFID_KEY_ERROR: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x3;
      gprs_send_frame->order = gprs_order++;

      work_alarm_process(ALARM_TYPE_RFID_KEY_ERROR, &(gprs_send_frame->work_alarm), time);

      break;
    };
    case ALARM_TYPE_RFID_KEY_PLUGIN: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x3;
      gprs_send_frame->order = gprs_order++;

      work_alarm_process(ALARM_TYPE_RFID_KEY_PLUGIN, &(gprs_send_frame->work_alarm), time);

      break;
    };
    case ALARM_TYPE_RFID_FAULT: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x4;
      gprs_send_frame->order = gprs_order++;

      fault_alarm_process(ALARM_TYPE_RFID_FAULT, &(gprs_send_frame->fault_alarm), time);

      break;
    };
    case ALARM_TYPE_CAMERA_FAULT: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x4;
      gprs_send_frame->order = gprs_order++;

      fault_alarm_process(ALARM_TYPE_CAMERA_FAULT, &(gprs_send_frame->fault_alarm), time);

      break;
    };
    case ALARM_TYPE_POWER_FAULT: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x4;
      gprs_send_frame->order = gprs_order++;

      fault_alarm_process(ALARM_TYPE_POWER_FAULT, &(gprs_send_frame->fault_alarm), time);

      break;
    };
    case ALARM_TYPE_MOTOR_FAULT: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x4;
      gprs_send_frame->order = gprs_order++;

      fault_alarm_process(ALARM_TYPE_MOTOR_FAULT, &(gprs_send_frame->fault_alarm), time);

      break;
    };

  }

  // send process
  process_buf = (uint8_t *)rt_malloc(sizeof(GPRS_SEND_FRAME_TYPEDEF));
  memset(process_buf, 0, sizeof(GPRS_SEND_FRAME_TYPEDEF));
  process_buf_bk = process_buf;
  process_length = 0;
  
  *process_buf_bk++ = (uint8_t)((gprs_send_frame->length) >> 8 & 0xff);
  *process_buf_bk++ = (uint8_t)(gprs_send_frame->length & 0xff);
  process_length += 2;

  *process_buf_bk++ = gprs_send_frame->cmd;
  process_length += 1;

  *process_buf_bk++ = gprs_send_frame->order;
  process_length += 1;

  switch (alarm_type)
  {
    case ALARM_TYPE_GPRS_AUTH : {

      memcpy(process_buf_bk, gprs_send_frame->auth.device_id, 6);
      memcpy(process_buf_bk+6, gprs_send_frame->auth.enc_data, 16);
      process_length += 22;
      break;
    };
    case ALARM_TYPE_GPRS_HEART : {

      *process_buf_bk++ = gprs_send_frame->heart.type;
      process_length += 1;
      break;
    };
    case ALARM_TYPE_CAMERA_IRDASENSOR: {
      
      *process_buf_bk++ = (uint8_t)((gprs_send_frame->work_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->work_alarm.type & 0xff);
      process_length += 2;
      
      *process_buf_bk++ = gprs_send_frame->work_alarm.status;
      process_length += 1;
      
      memcpy(process_buf_bk, gprs_send_frame->work_alarm.time, 6);
      process_length += 6;      
      break;
    };
    case ALARM_TYPE_LOCK_SHELL: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->work_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->work_alarm.type & 0xff);
      process_length += 2;
      
      *process_buf_bk++ = gprs_send_frame->work_alarm.status;
      process_length += 1;
      
      memcpy(process_buf_bk, gprs_send_frame->work_alarm.time, 6);
      process_length += 6;      
      break;
    };
    case ALARM_TYPE_LOCK_GATE: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->work_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->work_alarm.type & 0xff);
      process_length += 2;
      
      *process_buf_bk++ = gprs_send_frame->work_alarm.status;
      process_length += 1;
      
      memcpy(process_buf_bk, gprs_send_frame->work_alarm.time, 6);
      process_length += 6;      
      break;
    };
    case ALARM_TYPE_LOCK_TEMPERATURE: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->work_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->work_alarm.type & 0xff);
      process_length += 2;
      
      *process_buf_bk++ = gprs_send_frame->work_alarm.status;
      process_length += 1;
      
      memcpy(process_buf_bk, gprs_send_frame->work_alarm.time, 6);
      process_length += 6;      
      break;
    };
    case ALARM_TYPE_RFID_KEY_ERROR: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->work_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->work_alarm.type & 0xff);
      process_length += 2;
      
      *process_buf_bk++ = gprs_send_frame->work_alarm.status;
      process_length += 1;
      
      memcpy(process_buf_bk, gprs_send_frame->work_alarm.time, 6);
      process_length += 6;      
      break;
    };
    case ALARM_TYPE_RFID_KEY_PLUGIN: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->work_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->work_alarm.type & 0xff);
      process_length += 2;
      
      *process_buf_bk++ = gprs_send_frame->work_alarm.status;
      process_length += 1;
      
      memcpy(process_buf_bk, gprs_send_frame->work_alarm.time, 6);
      process_length += 6;      
      break;
    };

    case ALARM_TYPE_RFID_FAULT: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->fault_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->fault_alarm.type & 0xff);
      process_length += 2;

      memcpy(process_buf_bk, gprs_send_frame->fault_alarm.time, 6);
      process_length += 6;
      break;
    };
    case ALARM_TYPE_CAMERA_FAULT: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->fault_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->fault_alarm.type & 0xff);
      process_length += 2;

      memcpy(process_buf_bk, gprs_send_frame->fault_alarm.time, 6);
      process_length += 6;
      break;
    };
    case ALARM_TYPE_POWER_FAULT: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->fault_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->fault_alarm.type & 0xff);
      process_length += 2;

      memcpy(process_buf_bk, gprs_send_frame->fault_alarm.time, 6);
      process_length += 6;
      break;
    };
    case ALARM_TYPE_MOTOR_FAULT: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->fault_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->fault_alarm.type & 0xff);
      process_length += 2;

      memcpy(process_buf_bk, gprs_send_frame->fault_alarm.time, 6);
      process_length += 6;
      break;
    };
  }

  //send frame
  gsm_put_hex(process_buf, process_length);
  rt_device_write(device, 0, process_buf, process_length);

  rt_free(gprs_send_frame);
  rt_free(process_buf);
  return 1;
}

int8_t recv_gprs_frame(GPRS_RECV_FRAME_TYPEDEF *recv_frame)
{
  uint8_t *process_buf = RT_NULL;
  GPRS_RECV_FRAME_TYPEDEF *gprs_recv_frame = RT_NULL;
  rt_device_t device = RT_NULL;

  device = rt_device_find(DEVICE_NAME_GSM_USART);

  if (device == RT_NULL)
  {
    rt_kprintf("device %s is not exist!\n", DEVICE_NAME_GSM_USART);
    return -1;
  }

  // recv process
  process_buf = (uint8_t *)rt_malloc(sizeof(GPRS_RECV_FRAME_TYPEDEF));
  memset(process_buf, 0, sizeof(GPRS_RECV_FRAME_TYPEDEF));
  if (rt_device_read(device, 0, process_buf, sizeof(GPRS_RECV_FRAME_TYPEDEF)) == 0)
  {
    rt_free(process_buf);
    return -1;
  }
  gsm_put_hex(process_buf, sizeof(GPRS_RECV_FRAME_TYPEDEF));
  gprs_recv_frame = (GPRS_RECV_FRAME_TYPEDEF *)process_buf;

  *recv_frame = *gprs_recv_frame;

  // frame process
  rt_free(process_buf);
  return 1;
}

