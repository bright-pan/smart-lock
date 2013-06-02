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
#include <ctype.h>
#include "gsm.h"
#include <string.h>
#include "gsm_usart.h"
#include <stdio.h>

rt_mq_t sms_mq;

typedef struct {
  char *data;
  uint8_t length;
}SMS_DATA_TYPEDEF;

SMS_DATA_TYPEDEF sms_data[50];
    
static void sms_data_init(SMS_DATA_TYPEDEF sms_data[])
{
  //lock shell
  sms_data[ALARM_TYPE_LOCK_SHELL].data = "";
  sms_data[ALARM_TYPE_LOCK_SHELL].length = 0;
  // lock temperature
  sms_data[ALARM_TYPE_LOCK_TEMPERATURE].data = "";
  sms_data[ALARM_TYPE_LOCK_TEMPERATURE].length = 0;
  // lock gate status
  sms_data[ALARM_TYPE_LOCK_GATE].data = "";
  sms_data[ALARM_TYPE_LOCK_GATE].length = 0;
  // rfid key detect alarm type
  sms_data[ALARM_TYPE_RFID_KEY_DETECT].data = "";
  sms_data[ALARM_TYPE_RFID_KEY_DETECT].length = 0;
  // camera irda sensor alarm
  sms_data[ALARM_TYPE_CAMERA_IRDASENSOR].data = "";
  sms_data[ALARM_TYPE_CAMERA_IRDASENSOR].length = 0;

  // battry working 20 min
  sms_data[ALARM_TYPE_BATTERY_WORKING_20M].data = "";
  sms_data[ALARM_TYPE_BATTERY_WORKING_20M].length = 0;
  // battry remain 50%
  sms_data[ALARM_TYPE_BATTERY_REMAIN_50P].data = "";
  sms_data[ALARM_TYPE_BATTERY_REMAIN_50P].length = 0;
  // battry working 20%
  sms_data[ALARM_TYPE_BATTERY_REMAIN_20P].data = "";
  sms_data[ALARM_TYPE_BATTERY_REMAIN_20P].length = 0;
  // battry working 5%
  sms_data[ALARM_TYPE_BATTERY_REMAIN_5P].data = "";
  sms_data[ALARM_TYPE_BATTERY_REMAIN_5P].length = 0;
}

void sms_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  rt_uint32_t event;
  struct tm tm_time;
  /* malloc a buff for process mail */
  SMS_MAIL_TYPEDEF *sms_mail_buf = (SMS_MAIL_TYPEDEF *)rt_malloc(sizeof(SMS_MAIL_TYPEDEF));
  /* initial msg queue */
  sms_mq = rt_mq_create("sms", sizeof(SMS_MAIL_TYPEDEF), \
                        SMS_MAIL_MAX_MSGS, \
                        RT_IPC_FLAG_FIFO);

  sms_data_init(sms_data);

  while (1)
  {
    result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND, RT_WAITING_FOREVER , &event);
    if (result == RT_EOK)
    {
      /* process mail */
      memset(sms_mail_buf, 0, sizeof(SMS_MAIL_TYPEDEF));
      result = rt_mq_recv(sms_mq, sms_mail_buf, \
                          sizeof(SMS_MAIL_TYPEDEF), \
                          100);
      if (result == RT_EOK)
      {
        rt_kprintf("\nreceive sms mail < time: %d alarm_type: %d >\n", sms_mail_buf->time, sms_mail_buf->alarm_type);

        gmtime_r(&(sms_mail_buf->time), &tm_time);
  
        tm_time.tm_year += 1900;
        tm_time.tm_mon += 1;

        rt_mutex_take(mutex_gsm_mode, RT_WAITING_FOREVER);
        if (gsm_mode_get() & EVENT_GSM_MODE_GPRS)
        {
          rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
          rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_GPRS_CMD);
          result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, 800 , &event);
          if (result != RT_EOK)
          {
            rt_kprintf("\ngsm mode switch to gprs_cmd is error, and try resend|\n");
            rt_mq_urgent(sms_mq, sms_mail_buf, sizeof(SMS_MAIL_TYPEDEF));
            rt_mutex_release(mutex_gsm_mode);
            continue;
          }
        }

        rt_kprintf("\nsend sms!!!\n");
        /*
          if (!(gsm_mode_get() & EVENT_GSM_MODE_CMD))
          {
          rt_mutex_release(mutex_gsm_mode);
          break;
          }
        


          if (!(gsm_mode_get() & EVENT_GSM_MODE_GPRS))
          {
          rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_GPRS);
          result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, 800 , &event);
          if (result != RT_EOK)
          {
          rt_kprintf("\ngsm mode switch to gprs is error\n");
          continue;
          }

          }
        */
        rt_mutex_release(mutex_gsm_mode);
      }
      else
      {
        /* mail receive error */
      }
    }
    else
    {
    }
  }
  rt_free(sms_mail_buf);
}



uint16_t NUM_UCS_MAP[16] = {

  0X3000,  0X3100, 
  0X3200,  0X3300,
  0X3400,  0X3500,
  0X3600,  0X3700,
  0X3800,  0X3900,
  0X4100,  0X4200,
  0X4300,  0X4400,
  0X4500,  0X4600,
};

static uint8_t HEX_CHAR_MAP[16] = {

  '0','1','2','3',
  '4','5','6','7',
  '8','9','A','B',
  'C','D','E','F'
};

static uint8_t NUM_MAP[10] = {

  '\x00','\x01','\x02','\x03','\x04',
  '\x05','\x06','\x07','\x08','\x09'

};

static uint8_t ALPHA_MAP[7] = {

  '\x00','\x0a','\x0b','\x0c',
  '\x0d','\x0e','\x0f'

};

static void sms_pdu_phone_address_init(uint8_t *octet, const char *phone_address)
{
  int8_t length = strlen(phone_address);
  int8_t temp = 0;
  char *buf = NULL;
  char *buf_bk = NULL;
  
  temp = (length & 0x01) ? (length + 1) : length;
  buf = (char *)rt_malloc(temp);
  buf_bk = buf;
  memcpy(buf, phone_address, length);
  
  temp >>= 1;
  //*octet_length = temp; // octect length

  while (temp-- > 0)
  {
    *octet = (*buf++ - '0') & 0x0f;
    *octet++ |= ((*buf++ - '0') << 4) & 0xf0;
  }
  if (length & 0x01)
  {
    *--octet |= 0xf0;
  }

  rt_free(buf_bk);
}

static void sms_pdu_head_init(char *smsc_address, char *dest_address, SMS_SEND_PDU_FRAME *pdu, uint8_t tp_udl)
{
  // SMSC initial
  pdu->SMSC.SMSC_Length = 0x08;
  pdu->SMSC.SMSC_Type_Of_Address = 0x91;
  sms_pdu_phone_address_init(pdu->SMSC.SMSC_Address_Value, smsc_address);

  pdu->TPDU.First_Octet = 0x11;
  pdu->TPDU.TP_MR = 0x00;
  // DA initial
  pdu->TPDU.TP_DA.TP_DA_Length = 0x0D;
  pdu->TPDU.TP_DA.TP_DA_Type_Of_Address = 0x91;
  sms_pdu_phone_address_init(pdu->TPDU.TP_DA.TP_DA_Address_Value, dest_address);
  
  pdu->TPDU.TP_PID = 0x00;
  pdu->TPDU.TP_DCS = 0x08;
  pdu->TPDU.TP_VP = 0xC2;
  pdu->TPDU.TP_UDL = tp_udl;
  
}

static void hex_to_string(char *string, uint8_t *hex, uint8_t hex_length)
{
  while (hex_length-- > 0)
  {
    *string++ = HEX_CHAR_MAP[(*hex >> 4) & 0x0f];
    *string++ = HEX_CHAR_MAP[*hex++ & 0x0f];
  }
}

int8_t sms_pdu_ucs_send(char *dest_address, char *smsc_address, uint16_t *content, uint8_t length)
{
  SMS_SEND_PDU_FRAME *send_pdu_frame;
  uint8_t *pdu_data;
  char *send_pdu_string;
  char *at_temp;
  uint8_t sms_pdu_length;
  rt_device_t device;

  device = rt_device_find(DEVICE_NAME_GSM_USART);
  if (device == RT_NULL)
  {
    return -1;
  }

  if (length > 70)
  {
    length = 70;
  }

  send_pdu_frame = (SMS_SEND_PDU_FRAME *)rt_malloc(sizeof(SMS_SEND_PDU_FRAME));
  memset(send_pdu_frame, 0, sizeof(SMS_SEND_PDU_FRAME));
  
  pdu_data = send_pdu_frame->TPDU.TP_UD;

  send_pdu_string = (char *)rt_malloc(512);
  memset(send_pdu_string, '\0', 512);

  at_temp = (char *)rt_malloc(50);
  memset(at_temp, '\0', 50);

  sms_pdu_head_init(smsc_address, dest_address, send_pdu_frame, length << 1);

  sms_pdu_length = send_pdu_frame->TPDU.TP_UDL + sizeof(send_pdu_frame->SMSC) + sizeof(send_pdu_frame->TPDU) - sizeof(send_pdu_frame->TPDU.TP_UD);
  
  while (length-- > 0)
  {
    *pdu_data++ = (uint8_t)(0x00ff & (*content >> 8));
    *pdu_data++ = (uint8_t)(0x00ff & *content++);
  }

  hex_to_string(send_pdu_string, (uint8_t *)send_pdu_frame, sms_pdu_length);

  //gsm_put_char(send_pdu_string, strlen(send_pdu_string));
  
  siprintf(at_temp,
             "AT+CMGS=%d\x0D",
             send_pdu_frame->TPDU.TP_UDL + (sizeof(send_pdu_frame->TPDU) - sizeof(send_pdu_frame->TPDU.TP_UD)));
  //gsm_put_char(at_temp, strlen(at_temp));
  rt_device_write(device, 0, "AT+CMGF=0\r", strlen(at_temp));
  rt_thread_delay(50);
  rt_device_write(device, 0, at_temp, strlen(at_temp));
  rt_thread_delay(50);
  rt_device_read(device, 0, at_temp , 50);
  rt_device_write(device, 0, send_pdu_string, strlen(send_pdu_string));
  rt_device_write(device, 0, "\x1A", 1);
  rt_thread_delay(50);
  
  rt_free(send_pdu_frame);
  rt_free(send_pdu_string);
  rt_free(at_temp);
  return 0;
}

#ifdef RT_USING_FINSH
#include <finsh.h>

static char temp[100];

uint16_t default_data[] = {0x5DE5,0x4F5C,0x6109,0x5FEB,0xFF01};

void sms(char *address, short *data, char length)
{
  rt_device_t device;
  memset(temp, '\0', 100);
  device = rt_device_find(DEVICE_NAME_GSM_USART);
  if (device != RT_NULL)
  {
    if (length == 0)
    {
      sms_pdu_ucs_send(address,smsc,default_data, 5);
    }
    else
    {
      sms_pdu_ucs_send(address,smsc,default_data, length);
    }
  }
  else
  {
    rt_kprintf("device %s is not exist!\n", DEVICE_NAME_GSM_USART);
  }
}
FINSH_FUNCTION_EXPORT(sms, sms[address data length])
#endif
