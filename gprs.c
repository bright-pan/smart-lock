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
#include <rtc.h>

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
  GPRS_RECV_FRAME_TYPEDEF *gprs_recv_frame = RT_NULL;
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
          gprs_recv_frame = (GPRS_RECV_FRAME_TYPEDEF *)rt_malloc(sizeof(GPRS_RECV_FRAME_TYPEDEF));
          recv_gprs_frame(gprs_recv_frame);
          rt_free(gprs_recv_frame);
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

void send_gprs_mail(ALARM_TYPEDEF alarm_type, time_t time, uint8_t order)
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
  buf.order = order;
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

uint8_t dec_to_bcd(uint8_t dec)
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

  work_alarm->time[0] = dec_to_bcd((uint8_t)(tm_time.tm_year % 100));
  work_alarm->time[1] = dec_to_bcd((uint8_t)(tm_time.tm_mon % 100));
  work_alarm->time[2] = dec_to_bcd((uint8_t)(tm_time.tm_mday % 100));
  work_alarm->time[3] = dec_to_bcd((uint8_t)(tm_time.tm_hour % 100));
  work_alarm->time[4] = dec_to_bcd((uint8_t)(tm_time.tm_min % 100));
  work_alarm->time[5] = dec_to_bcd((uint8_t)(tm_time.tm_sec % 100));
}

void fault_alarm_process(ALARM_TYPEDEF alarm_type, GPRS_FAULT_ALARM *fault_alarm, time_t time)
{
  struct tm tm_time;

  fault_alarm->type = fault_alarm_type_map[alarm_type];

  tm_time = *localtime(&time);
  tm_time.tm_year += 1900;
  tm_time.tm_mon += 1;

  fault_alarm->time[0] = dec_to_bcd((uint8_t)(tm_time.tm_year % 100));
  fault_alarm->time[1] = dec_to_bcd((uint8_t)(tm_time.tm_mon % 100));
  fault_alarm->time[2] = dec_to_bcd((uint8_t)(tm_time.tm_mday % 100));
  fault_alarm->time[3] = dec_to_bcd((uint8_t)(tm_time.tm_hour % 100));
  fault_alarm->time[4] = dec_to_bcd((uint8_t)(tm_time.tm_min % 100));
  fault_alarm->time[5] = dec_to_bcd((uint8_t)(tm_time.tm_sec % 100));
}

void process_gprs_list_telephone(GPRS_SEND_LIST_TELEPHONE *list_telephone, uint16_t *length)
{
  uint8_t index = 0;
  uint8_t *buf = RT_NULL;

  list_telephone->counts = 0;
  list_telephone->result = 1;
  buf = &(list_telephone->telephone[0][0]);

  for (index = 0; index < TELEPHONE_NUMBERS; index++)
  {
    if (device_parameters.alarm_telephone[index].flag)
    {
      (list_telephone->counts)++;
      memcpy(buf, device_parameters.alarm_telephone[index].address + 2, 11);
      buf += 11;
    }
  }

  *length = 2 + 11 * list_telephone->counts;
}

void process_gprs_list_rfid_key(GPRS_SEND_LIST_RFID_KEY *list_rfid_key, uint16_t *length)
{
  uint8_t index = 0;
  uint8_t *buf = RT_NULL;

  list_rfid_key->counts = 0;
  list_rfid_key->result = 1;
  buf = &(list_rfid_key->key[0][0]);

  for (index = 0; index < RFID_KEY_NUMBERS; index++)
  {
    if (device_parameters.rfid_key[index].flag)
    {
      (list_rfid_key->counts)++;
      memcpy(buf, device_parameters.rfid_key[index].key, 4);
      buf += 4;
    }
  }

  *length = 2 + 4 * list_rfid_key->counts;
}
void process_gprs_list_user_parameters(GPRS_SEND_LIST_USER_PARAMETERS *list_user_parameters, uint16_t *length)
{
  list_user_parameters->result = 1;
  *length += 1;

  list_user_parameters->parameters = device_parameters.lock_gate_alarm_time;
  *length += 1;
}

int8_t send_gprs_frame(ALARM_TYPEDEF alarm_type, time_t time, uint8_t order)
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
    case ALARM_TYPE_GPRS_LIST_TELEPHONE: {

      gprs_send_frame->length = 0;
      gprs_send_frame->cmd = 0x06;
      gprs_send_frame->order = order;

      process_gprs_list_telephone(&(gprs_send_frame->list_telephone), &(gprs_send_frame->length));
      break;
    };
    case ALARM_TYPE_GPRS_SET_TELEPHONE_SUCCESS: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x07;
      gprs_send_frame->order = order;

      gprs_send_frame->set_telephone.result = 1;
      break;
    };
    case ALARM_TYPE_GPRS_SET_TELEPHONE_FAILURE: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x07;
      gprs_send_frame->order = order;

      gprs_send_frame->set_telephone.result = 0;
      break;
    };

    case ALARM_TYPE_GPRS_LIST_RFID_KEY: {

      gprs_send_frame->length = 0;
      gprs_send_frame->cmd = 0x08;
      gprs_send_frame->order = order;

      process_gprs_list_rfid_key(&(gprs_send_frame->list_rfid_key), &(gprs_send_frame->length));
      break;
    };
    case ALARM_TYPE_GPRS_SET_RFID_KEY_SUCCESS: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x09;
      gprs_send_frame->order = order;

      gprs_send_frame->set_rfid_key.result = 1;
      break;
    };
    case ALARM_TYPE_GPRS_SET_RFID_KEY_FAILURE: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x09;
      gprs_send_frame->order = order;

      gprs_send_frame->set_rfid_key.result = 0;
      break;
    };
    case ALARM_TYPE_GPRS_LIST_USER_PARAMETERS: {

      gprs_send_frame->length = 0;
      gprs_send_frame->cmd = 0x0A;
      gprs_send_frame->order = order;

      process_gprs_list_user_parameters(&(gprs_send_frame->list_user_parameters), &(gprs_send_frame->length));
      break;
    };
    case ALARM_TYPE_GPRS_SET_USER_PARAMETERS_SUCCESS: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x0B;
      gprs_send_frame->order = order;

      gprs_send_frame->set_user_parameters.result = 1;
      break;
    };
    case ALARM_TYPE_GPRS_SET_USER_PARAMETERS_FAILURE: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x0B;
      gprs_send_frame->order = order;

      gprs_send_frame->set_user_parameters.result = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SET_TIME_SUCCESS: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x15;
      gprs_send_frame->order = order;

      gprs_send_frame->set_time.result = 1;
      break;
    };
    case ALARM_TYPE_GPRS_SET_TIME_FAILURE: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x15;
      gprs_send_frame->order = order;

      gprs_send_frame->set_time.result = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SET_KEY0_SUCCESS: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x18;
      gprs_send_frame->order = order;

      gprs_send_frame->set_key0.result = 1;
      break;
    };
    case ALARM_TYPE_GPRS_SET_KEY0_FAILURE: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x18;
      gprs_send_frame->order = order;

      gprs_send_frame->set_key0.result = 0;
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
    case ALARM_TYPE_GPRS_LIST_TELEPHONE: {

      *process_buf_bk++ = gprs_send_frame->list_telephone.result;
      *process_buf_bk++ = gprs_send_frame->list_telephone.counts;
      process_length += 2;

      memcpy(process_buf_bk, gprs_send_frame->list_telephone.telephone, 11 * gprs_send_frame->list_telephone.counts);
      process_length += 11 * gprs_send_frame->list_telephone.counts;
      break;
    };
    case ALARM_TYPE_GPRS_SET_TELEPHONE_SUCCESS: {

      *process_buf_bk++ = gprs_send_frame->set_telephone.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_TELEPHONE_FAILURE: {

      *process_buf_bk++ = gprs_send_frame->set_telephone.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_LIST_RFID_KEY: {

      *process_buf_bk++ = gprs_send_frame->list_rfid_key.result;
      *process_buf_bk++ = gprs_send_frame->list_rfid_key.counts;
      process_length += 2;

      memcpy(process_buf_bk, gprs_send_frame->list_rfid_key.key, 11 * gprs_send_frame->list_rfid_key.counts);
      process_length += 4 * gprs_send_frame->list_rfid_key.counts;
      break;
    };
    case ALARM_TYPE_GPRS_SET_RFID_KEY_SUCCESS: {

      *process_buf_bk++ = gprs_send_frame->set_rfid_key.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_RFID_KEY_FAILURE: {

      *process_buf_bk++ = gprs_send_frame->set_rfid_key.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_LIST_USER_PARAMETERS: {

      *process_buf_bk++ = gprs_send_frame->list_user_parameters.result;
      *process_buf_bk++ = gprs_send_frame->list_user_parameters.parameters;
      process_length += 2;

      break;
    };
    case ALARM_TYPE_GPRS_SET_USER_PARAMETERS_SUCCESS: {

      *process_buf_bk++ = gprs_send_frame->set_user_parameters.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_USER_PARAMETERS_FAILURE: {

      *process_buf_bk++ = gprs_send_frame->set_user_parameters.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_TIME_SUCCESS: {

      *process_buf_bk++ = gprs_send_frame->set_time.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_TIME_FAILURE: {

      *process_buf_bk++ = gprs_send_frame->set_time.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_KEY0_SUCCESS: {

      *process_buf_bk++ = gprs_send_frame->set_key0.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_KEY0_FAILURE: {

      *process_buf_bk++ = gprs_send_frame->set_key0.result;
      process_length += 1;

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

void process_gprs_set_telephone(GPRS_SET_TELEPHONE *set_telephone)
{
  uint8_t index = 0;
  uint8_t *buf = RT_NULL;

  buf = &set_telephone->telephone[0][0];


  if (set_telephone->counts > TELEPHONE_NUMBERS)
  {
    return;
  }

  for (index = 0; index < TELEPHONE_NUMBERS; index++)
  {
    device_parameters.alarm_telephone[index].flag = 0;
  }

  for (index = 0; index < set_telephone->counts; index++)
  {
    device_parameters.alarm_telephone[index].flag = 1;
    memcpy(device_parameters.alarm_telephone[index].address, "86", 2);
    memcpy(device_parameters.alarm_telephone[index].address + 2, buf, 11);
    buf += 11;
  }
  // save to file
  system_file_operate(&device_parameters, 1);
}

void process_gprs_set_rfid_key(GPRS_SET_RFID_KEY *set_rfid_key)
{
  uint8_t index = 0;
  uint8_t *buf = RT_NULL;

  buf = &set_rfid_key->key[0][0];


  if (set_rfid_key->counts > RFID_KEY_NUMBERS)
  {
    return;
  }

  for (index = 0; index < RFID_KEY_NUMBERS; index++)
  {
    device_parameters.rfid_key[index].flag = 0;
  }

  for (index = 0; index < set_rfid_key->counts; index++)
  {
    device_parameters.rfid_key[index].flag = 1;
    memcpy(device_parameters.rfid_key[index].key, buf, 4);
    buf += 4;
  }
  // save to file
  system_file_operate(&device_parameters, 1);
}
void process_gprs_set_user_parameters(GPRS_SET_USER_PARAMETERS *set_user_parameters)
{
  device_parameters.lock_gate_alarm_time = set_user_parameters->parameters;
  //save to file
  system_file_operate(&device_parameters, 1);
}

uint8_t bcd_to_dec(uint8_t bcd)
{
  if (bcd >> 4 >= 0x0A)
  {
    return 0;
  }
  else
  {
    return (bcd >> 4) * 10 + (bcd & 0x0f);
  }
}
void process_gprs_set_time(GPRS_SET_TIME *gprs_set_time)
{
  set_date(bcd_to_dec(gprs_set_time->time[0]) + 2000, bcd_to_dec(gprs_set_time->time[1]), bcd_to_dec(gprs_set_time->time[2]));
  set_time(bcd_to_dec(gprs_set_time->time[3]),bcd_to_dec(gprs_set_time->time[4]),bcd_to_dec(gprs_set_time->time[5]));
}

void process_gprs_set_key0(GPRS_SET_KEY0 *gprs_set_key0)
{
  memcpy(device_parameters.key0, gprs_set_key0->key0, 8);
  //save to file
  system_file_operate(&device_parameters, 1);
}

int8_t recv_gprs_frame(GPRS_RECV_FRAME_TYPEDEF *recv_frame)
{
  uint8_t *process_buf = RT_NULL;
  GPRS_RECV_FRAME_TYPEDEF *gprs_recv_frame = RT_NULL;
  rt_device_t device = RT_NULL;
  uint16_t recv_counts = 0;

  device = rt_device_find(DEVICE_NAME_GSM_USART);

  if (device == RT_NULL)
  {
    rt_kprintf("device %s is not exist!\n", DEVICE_NAME_GSM_USART);
    return -1;
  }

  // recv process
  process_buf = (uint8_t *)rt_malloc(sizeof(GPRS_RECV_FRAME_TYPEDEF));
  memset(process_buf, 0, sizeof(GPRS_RECV_FRAME_TYPEDEF));
  recv_counts = rt_device_read(device, 0, process_buf, sizeof(GPRS_RECV_FRAME_TYPEDEF));
  if (recv_counts == 0)
  {
    rt_free(process_buf);
    return -1;
  }
  gsm_put_hex(process_buf, recv_counts);
  gprs_recv_frame = (GPRS_RECV_FRAME_TYPEDEF *)process_buf;

  switch (gprs_recv_frame->cmd)
  {
    //list telephone
    case 0x86: {

      if (gprs_recv_frame->list_telephone.type == 0x01 && recv_counts == 4 + gprs_recv_frame->length)
      {
        send_gprs_mail(ALARM_TYPE_GPRS_LIST_TELEPHONE, 0, gprs_recv_frame->order);
      }
      else
      {
        rt_kprintf("recv frame 0x%02x is error!", gprs_recv_frame->cmd);
      }
      break;
    };
    //set telephone
    case 0x87: {

      if (recv_counts == 4 + gprs_recv_frame->length)
      {
        process_gprs_set_telephone(&gprs_recv_frame->set_telephone);
        send_gprs_mail(ALARM_TYPE_GPRS_SET_TELEPHONE_SUCCESS, 0, gprs_recv_frame->order);
      }
      else
      {
        send_gprs_mail(ALARM_TYPE_GPRS_SET_TELEPHONE_FAILURE, 0, gprs_recv_frame->order);
      }
      break;
    };
    //list rfid key
    case 0x88: {
      if (gprs_recv_frame->list_rfid_key.type == 0x01)
      {
        send_gprs_mail(ALARM_TYPE_GPRS_LIST_RFID_KEY, 0, gprs_recv_frame->order);
      }
      else
      {
        rt_kprintf("recv frame 0x%02x is error!", gprs_recv_frame->cmd);
      }
      break;
    };
    //set rfid key
    case 0x89: {

      if (recv_counts == 4 + gprs_recv_frame->length)
      {
        process_gprs_set_rfid_key(&gprs_recv_frame->set_rfid_key);
        send_gprs_mail(ALARM_TYPE_GPRS_SET_RFID_KEY_SUCCESS, 0, gprs_recv_frame->order);
      }
      else
      {
        send_gprs_mail(ALARM_TYPE_GPRS_SET_RFID_KEY_FAILURE, 0, gprs_recv_frame->order);
      }
      break;
    };
    //list user parameters
    case 0x8A: {
      if (gprs_recv_frame->list_user_parameters.type == 0x01)
      {
        send_gprs_mail(ALARM_TYPE_GPRS_LIST_USER_PARAMETERS, 0, gprs_recv_frame->order);
      }
      else
      {
        rt_kprintf("recv frame 0x%02x is error!", gprs_recv_frame->cmd);
      }
      break;
    };
    //set user parameters
    case 0x8B: {

      if (recv_counts == 4 + gprs_recv_frame->length)
      {
        process_gprs_set_user_parameters(&gprs_recv_frame->set_user_parameters);
        send_gprs_mail(ALARM_TYPE_GPRS_SET_USER_PARAMETERS_SUCCESS, 0, gprs_recv_frame->order);
      }
      else
      {
        send_gprs_mail(ALARM_TYPE_GPRS_SET_USER_PARAMETERS_FAILURE, 0, gprs_recv_frame->order);
      }
      break;
    };
    //set key0
    case 0x98: {

      if (recv_counts == 4 + gprs_recv_frame->length)
      {
        process_gprs_set_key0(&gprs_recv_frame->set_key0);
        send_gprs_mail(ALARM_TYPE_GPRS_SET_KEY0_SUCCESS, 0, gprs_recv_frame->order);
      }
      else
      {
        send_gprs_mail(ALARM_TYPE_GPRS_SET_KEY0_FAILURE, 0, gprs_recv_frame->order);
      }
      break;
    };
  }

  *recv_frame = *gprs_recv_frame;

  // frame process
  rt_free(process_buf);
  return 1;
}

