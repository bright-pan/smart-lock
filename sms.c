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
#include "untils.h"

rt_mq_t sms_mq;

/*
智能锁自动断电，将自动启动电池供电
0x667A,0x80FD,0x9501,0x81EA,0x52A8,0x65AD,0x7535,0xFF0C,0x5C06,0x81EA,0x52A8,0x542F,0x52A8,0x7535,0x6C60,0x4F9B,0x7535

智能锁电池电量还剩50%，请尽快充电
0x667A,0x80FD,0x9501,0x7535,0x6C60,0x7535,0x91CF,0x8FD8,0x5269,0x0035,0x0030,0x0025,0xFF0C,0x8BF7,0x5C3D,0x5FEB,0x5145,0x7535

智能锁电池电量还剩20%，请注意安全并尽快充电
0x667A,0x80FD,0x9501,0x7535,0x6C60,0x7535,0x91CF,0x8FD8,0x5269,0x0032,0x0030,0x0025,0xFF0C,0x8BF7,0x6CE8,0x610F,0x5B89,0x5168,0x5E76,0x5C3D,0x5FEB,0x5145,0x7535

智能锁摄像头被遮挡，请注意安全
0x667A,0x80FD,0x9501,0x6444,0x50CF,0x5934,0x88AB,0x906E,0x6321,0xFF0C,0x8BF7,0x6CE8,0x610F,0x5B89,0x5168

智能锁正在被切割开启，清注意安全
0x667A,0x80FD,0x9501,0x6B63,0x5728,0x88AB,0x5207,0x5272,0x5F00,0x542F,0xFF0C,0x6E05,0x6CE8,0x610F,0x5B89,0x5168

智能锁正被暴力开启，请注意安全
0x667A,0x80FD,0x9501,0x6B63,0x88AB,0x66B4,0x529B,0x5F00,0x542F,0xFF0C,0x8BF7,0x6CE8,0x610F,0x5B89,0x5168

智能锁正被非法开启，请注意安全
0x667A,0x80FD,0x9501,0x6B63,0x88AB,0x975E,0x6CD5,0x5F00,0x542F,0xFF0C,0x8BF7,0x6CE8,0x610F,0x5B89,0x5168

摄像头模块故障，请联系经销商或厂家
0x6444,0x50CF,0x5934,0x6A21,0x5757,0x6545,0x969C,0xFF0C,0x8BF7,0x8054,0x7CFB,0x7ECF,0x9500,0x5546,0x6216,0x5382,0x5BB6

RFID模块故障，请联系经销商或厂家
0x0052,0x0046,0x0049,0x0044,0x6A21,0x5757,0x6545,0x969C,0xFF0C,0x8BF7,0x8054,0x7CFB,0x7ECF,0x9500,0x5546,0x6216,0x5382,0x5BB6

GSM模块故障，请联系经销商或厂家
0x0047,0x0053,0x004D,0x6A21,0x5757,0x6545,0x969C,0xFF0C,0x8BF7,0x8054,0x7CFB,0x7ECF,0x9500,0x5546,0x6216,0x5382,0x5BB6

距离传感器故障，请联系经销商或厂家
0x8DDD,0x79BB,0x4F20,0x611F,0x5668,0x6545,0x969C,0xFF0C,0x8BF7,0x8054,0x7CFB,0x7ECF,0x9500,0x5546,0x6216,0x5382,0x5BB6

电源供电模块故障，请联系经销商或厂家
0x7535,0x6E90,0x4F9B,0x7535,0x6A21,0x5757,0x6545,0x969C,0xFF0C,0x8BF7,0x8054,0x7CFB,0x7ECF,0x9500,0x5546,0x6216,0x5382,0x5BB6

锁舌电机故障，请联系经销商或厂家
0x9501,0x820C,0x7535,0x673A,0x6545,0x969C,0xFF0C,0x8BF7,0x8054,0x7CFB,0x7ECF,0x9500,0x5546,0x6216,0x5382,0x5BB6

尊敬的用户，你的门没有锁好，请注意安全
0x5C0A,0x656C,0x7684,0x7528,0x6237,0xFF0C,0x4F60,0x7684,0x95E8,0x6CA1,0x6709,0x9501,0x597D,0xFF0C,0x8BF7,0x6CE8,0x610F,0x5B89,0x5168

尊敬的用户，你的门钥没有拔取，请注意安全
0x5C0A,0x656C,0x7684,0x7528,0x6237,0xFF0C,0x4F60,0x7684,0x95E8,0x94A5,0x6CA1,0x6709,0x62D4,0x53D6,0xFF0C,0x8BF7,0x6CE8,0x610F,0x5B89,0x5168
*/

const uint16_t NUM_UCS_MAP[16] = {

0x0030,0x0031,0x0032,0x0033,0x0034,
0x0035,0x0036,0x0037,0x0038,0x0039

};

const uint16_t sms_content_lock_shell[] = {

  0x667A,0x80FD,0x9501,0x6B63,0x88AB,
  0x66B4,0x529B,0x5F00,0x542F,0xFF0C,
  0x8BF7,0x6CE8,0x610F,0x5B89,0x5168
};
const uint16_t sms_content_lock_temperature[] = {

  0x667A,0x80FD,0x9501,0x6B63,0x5728,
  0x88AB,0x5207,0x5272,0x5F00,0x542F,
  0xFF0C,0x6E05,0x6CE8,0x610F,0x5B89,
  0x5168
};
const uint16_t sms_content_lock_gate[] = {

  0x5C0A,0x656C,0x7684,0x7528,0x6237,
  0xFF0C,0x4F60,0x7684,0x95E8,0x6CA1,
  0x6709,0x9501,0x597D,0xFF0C,0x8BF7,
  0x6CE8,0x610F,0x5B89,0x5168
};
const uint16_t sms_content_rfid_key_detect[] = {

  0x667A,0x80FD,0x9501,0x6B63,0x88AB,
  0x975E,0x6CD5,0x5F00,0x542F,0xFF0C,
  0x8BF7,0x6CE8,0x610F,0x5B89,0x5168
};
const uint16_t sms_content_camera_idrasensor[] = {

  0x667A,0x80FD,0x9501,0x6444,0x50CF,
  0x5934,0x88AB,0x906E,0x6321,0xFF0C,
  0x8BF7,0x6CE8,0x610F,0x5B89,0x5168
};

const uint16_t sms_content_battery_working_20m[] = {

  0x667A,0x80FD,0x9501,0x81EA,0x52A8,
  0x65AD,0x7535,0xFF0C,0x5C06,0x81EA,
  0x52A8,0x542F,0x52A8,0x7535,0x6C60,
  0x4F9B,0x7535
};
const uint16_t sms_content_battery_remain_50p[] = {

  0x667A,0x80FD,0x9501,0x7535,0x6C60,
  0x7535,0x91CF,0x8FD8,0x5269,0x0035,
  0x0030,0x0025,0xFF0C,0x8BF7,0x5C3D,
  0x5FEB,0x5145,0x7535
};
const uint16_t sms_content_battery_remain_20p[] = {

  0x667A,0x80FD,0x9501,0x7535,0x6C60,
  0x7535,0x91CF,0x8FD8,0x5269,0x0032,
  0x0030,0x0025,0xFF0C,0x8BF7,0x6CE8,
  0x610F,0x5B89,0x5168,0x5E76,0x5C3D,
  0x5FEB,0x5145,0x7535
};
const uint16_t sms_content_battery_remain_5p[] = {0,0};


const uint16_t sms_content_time_prefix[] = {

  0x3010
};

const uint16_t sms_content_time_suffix[] = {

  0x60A6,0x5FB7,0x667A,0x80FD,0x3011
};

typedef struct {

  const uint16_t *data;
  uint16_t length;

}SMS_DATA_TYPEDEF;

SMS_DATA_TYPEDEF sms_data[50];


/*
 *  sms content table initialing
 *
 */
static void sms_data_init(SMS_DATA_TYPEDEF sms_data[])
{
  //lock shell
  sms_data[ALARM_TYPE_LOCK_SHELL].data = sms_content_lock_shell;
  sms_data[ALARM_TYPE_LOCK_SHELL].length = sizeof(sms_content_lock_shell) / sizeof(uint16_t);
  // lock temperature
  sms_data[ALARM_TYPE_LOCK_TEMPERATURE].data = sms_content_lock_temperature;
  sms_data[ALARM_TYPE_LOCK_TEMPERATURE].length = sizeof(sms_content_lock_temperature) / sizeof(uint16_t);
  // lock gate status
  sms_data[ALARM_TYPE_LOCK_GATE].data = sms_content_lock_gate;
  sms_data[ALARM_TYPE_LOCK_GATE].length = sizeof(sms_content_lock_gate) / sizeof(uint16_t);
  // rfid key detect alarm type
  sms_data[ALARM_TYPE_RFID_KEY_DETECT].data = sms_content_rfid_key_detect;
  sms_data[ALARM_TYPE_RFID_KEY_DETECT].length = sizeof(sms_content_rfid_key_detect) / sizeof(uint16_t);
  // camera irda sensor alarm
  sms_data[ALARM_TYPE_CAMERA_IRDASENSOR].data = sms_content_camera_idrasensor;
  sms_data[ALARM_TYPE_CAMERA_IRDASENSOR].length = sizeof(sms_content_camera_idrasensor) / sizeof(uint16_t);

  // battry working 20 min
  sms_data[ALARM_TYPE_BATTERY_WORKING_20M].data = sms_content_battery_working_20m;
  sms_data[ALARM_TYPE_BATTERY_WORKING_20M].length = sizeof(sms_content_battery_working_20m) / sizeof(uint16_t);
  // battry remain 50%
  sms_data[ALARM_TYPE_BATTERY_REMAIN_50P].data = sms_content_battery_remain_50p;
  sms_data[ALARM_TYPE_BATTERY_REMAIN_50P].length = sizeof(sms_content_battery_remain_50p) / sizeof(uint16_t);
  // battry working 20%
  sms_data[ALARM_TYPE_BATTERY_REMAIN_20P].data = sms_content_battery_remain_20p;
  sms_data[ALARM_TYPE_BATTERY_REMAIN_20P].length = sizeof(sms_content_battery_remain_20p) / sizeof(uint16_t);
  // battry working 5%
  sms_data[ALARM_TYPE_BATTERY_REMAIN_5P].data = sms_content_battery_remain_5p;
  sms_data[ALARM_TYPE_BATTERY_REMAIN_5P].length = sizeof(sms_content_battery_remain_5p) / sizeof(uint16_t);
}


/*
 * convert time to ucs for sms
 *
 */
uint16_t *sms_time_ucs(const struct tm *tm_time,
                       const uint16_t *prefix, uint8_t prefix_length,
                       const uint16_t *suffix, uint8_t suffix_length,
                       uint8_t *length)
{
  uint16_t *time_ucs = (uint16_t *)rt_malloc(256);
  uint16_t year;
  uint16_t *time_ucs_bk = time_ucs;
  
  *length += prefix_length;
  while (prefix_length-- > 0)
  {
    *time_ucs++ = *prefix++;
  }
  // year
  year = tm_time->tm_year;
  year %= 10000;
  *time_ucs++ = NUM_UCS_MAP[year / 1000];
  year %= 1000;
  *time_ucs++ = NUM_UCS_MAP[year / 100];
  year %= 100;
  *time_ucs++ = NUM_UCS_MAP[year / 10];
  *time_ucs++ = NUM_UCS_MAP[year % 10];
  *time_ucs++ = 0x002D;// -
  // month
  *time_ucs++ = NUM_UCS_MAP[tm_time->tm_mon / 10];
  *time_ucs++ = NUM_UCS_MAP[tm_time->tm_mon % 10];
  *time_ucs++ = 0x002D;// -
  // day
  *time_ucs++ = NUM_UCS_MAP[tm_time->tm_mday / 10];
  *time_ucs++ = NUM_UCS_MAP[tm_time->tm_mday % 10];
  *time_ucs++ = 0x0020;// 0x0020
  // hour
  *time_ucs++ = NUM_UCS_MAP[tm_time->tm_hour / 10];
  *time_ucs++ = NUM_UCS_MAP[tm_time->tm_hour % 10];
  *time_ucs++ = 0x003A;// 0x003A
  // minute
  *time_ucs++ = NUM_UCS_MAP[tm_time->tm_min / 10];
  *time_ucs++ = NUM_UCS_MAP[tm_time->tm_min % 10];
  *time_ucs++ = 0x003A;// 0x003A
  // second
  *time_ucs++ = NUM_UCS_MAP[tm_time->tm_sec / 10];
  *time_ucs++ = NUM_UCS_MAP[tm_time->tm_sec % 10];
  *time_ucs++ = 0x0020;// 0x0020

  *length += 20;

  *length += suffix_length;
  while (suffix_length-- > 0)
  {
    *time_ucs++ = *suffix++;
  }

  return time_ucs_bk;
}

void sms_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  rt_uint32_t event;
  struct tm tm_time;
  uint16_t *time_ucs, *sms_ucs, *sms_ucs_bk;
  const uint16_t *temp_ucs;
  uint8_t time_ucs_length, sms_ucs_length, temp_ucs_length;
  int8_t resend_counts = 0;
  int8_t alarm_telephone_counts = 0;

  /* malloc a buff for process mail */
  SMS_MAIL_TYPEDEF *sms_mail_buf = (SMS_MAIL_TYPEDEF *)rt_malloc(sizeof(SMS_MAIL_TYPEDEF));
  /* initial msg queue */
  sms_mq = rt_mq_create("sms", sizeof(SMS_MAIL_TYPEDEF), \
                        SMS_MAIL_MAX_MSGS, \
                        RT_IPC_FLAG_FIFO);

  sms_data_init(sms_data);

  while (1)
  {
    /* process mail */
    memset(sms_mail_buf, 0, sizeof(SMS_MAIL_TYPEDEF));
    result = rt_mq_recv(sms_mq, sms_mail_buf, \
                        sizeof(SMS_MAIL_TYPEDEF), \
                        100);
    if (result == RT_EOK)
    {
      rt_kprintf("\nreceive sms mail < time: %d alarm_type: %d >\n", sms_mail_buf->time, sms_mail_buf->alarm_type);

      // sms content process
      sms_ucs = (uint16_t *)rt_malloc(256);
      sms_ucs_bk = sms_ucs;
      sms_ucs_length = 0;

      temp_ucs = sms_data[sms_mail_buf->alarm_type].data;
      temp_ucs_length = sms_data[sms_mail_buf->alarm_type].length;
      sms_ucs_length += temp_ucs_length;
      while (temp_ucs_length-- > 0)
      {
        *sms_ucs_bk++ = *temp_ucs++;
      }

      // sms time process
      gmtime_r(&(sms_mail_buf->time), &tm_time);

      tm_time.tm_year += 1900;
      tm_time.tm_mon += 1;
      time_ucs_length = 0;
      time_ucs = sms_time_ucs(&tm_time,
                              sms_content_time_prefix, sizeof(sms_content_time_prefix)/sizeof(uint16_t),
                              sms_content_time_suffix, sizeof(sms_content_time_suffix)/sizeof(uint16_t),
                              &time_ucs_length);
      temp_ucs = time_ucs;
      sms_ucs_length += time_ucs_length;
      while (time_ucs_length-- > 0)
      {
        *sms_ucs_bk++ = *temp_ucs++;
      }
      rt_free(time_ucs);

      rt_mutex_take(mutex_gsm_mode, RT_WAITING_FOREVER);
      result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND, 100, &event);
      if (result == RT_EOK)
      {
        if (gsm_mode_get() & EVENT_GSM_MODE_GPRS)
        {
          rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
          rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_GPRS_CMD);
          result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, 800 , &event);
          if (result != RT_EOK)
          {
            rt_kprintf("\ngsm mode gprs switch to gprs_cmd is error, and try resend|\n");
            // clear gsm setup event, do gsm check or initial for test gsm problem.
            rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
            if (!(gsm_mode_get() & EVENT_GSM_MODE_CMD))
            {
              rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
              rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);
              result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, 800 , &event);
              if (result != RT_EOK)
              {
                rt_kprintf("\ngsm mode switch to cmd is error, and try resend|\n");
              }
            }
            rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);
            rt_mq_urgent(sms_mq, sms_mail_buf, sizeof(SMS_MAIL_TYPEDEF));
            rt_free(sms_ucs);
            rt_mutex_release(mutex_gsm_mode);
            continue;
          }
        }
        // send sms
        rt_kprintf("\nsend sms!!!\n");
        alarm_telephone_counts = TELEPHONE_NUMBERS - 1;
        while (alarm_telephone_counts >= 0)
        {
          if (device_parameters.alarm_telephone[alarm_telephone_counts].flag)
          {
            rt_kprintf((char *)(device_parameters.alarm_telephone[alarm_telephone_counts].address));
            resend_counts = 5;
            while (resend_counts > 0)
            {
              if (!sms_pdu_ucs_send(device_parameters.alarm_telephone[alarm_telephone_counts].address, smsc, sms_ucs, sms_ucs_length))
              {
                break;
              }
              resend_counts--;
            }
            rt_kprintf("\nresend counts :%d\n", resend_counts);
            if (!resend_counts)
            {
              rt_kprintf("\nsend sms failure!!!\n");
              // send failure
              rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER , &event);
              if (!(gsm_mode_get() & EVENT_GSM_MODE_CMD))
              {
                rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
                rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);
                result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, 800 , &event);
                if (result != RT_EOK)
                {
                  rt_kprintf("\ngsm mode switch to cmd is error, and try resend|\n");
                }
              }
              rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);
              rt_mq_urgent(sms_mq, sms_mail_buf, sizeof(SMS_MAIL_TYPEDEF));
              break;
            }
          }
          alarm_telephone_counts--;
        }
      }
      else // gsm is not setup
      {
        rt_mq_urgent(sms_mq, sms_mail_buf, sizeof(SMS_MAIL_TYPEDEF));
      }
      // exit mail process
      rt_free(sms_ucs);
      rt_mutex_release(mutex_gsm_mode);
    }
    else
    {
      /* mail receive error */
    }
    /*
    result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND, RT_WAITING_FOREVER , &event);
    if (result == RT_EOK)
    {
    }
    else
    {
    }
    */
  }
  rt_free(sms_mail_buf);
}

static const uint8_t HEX_CHAR_MAP[16] = {

  '0','1','2','3',
  '4','5','6','7',
  '8','9','A','B',
  'C','D','E','F'
};

static const uint8_t NUM_MAP[10] = {

  '\x00','\x01','\x02','\x03','\x04',
  '\x05','\x06','\x07','\x08','\x09'

};

static const uint8_t ALPHA_MAP[7] = {

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
  char *at_temp, *process_buf;
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

  at_temp = (char *)rt_malloc(512);
  memset(at_temp, '\0', 512);
  process_buf = (char *)rt_malloc(512);
  memset(process_buf, '\0', 512);

  sms_pdu_head_init(smsc_address, dest_address, send_pdu_frame, length << 1);

  sms_pdu_length = send_pdu_frame->TPDU.TP_UDL + sizeof(send_pdu_frame->SMSC) + sizeof(send_pdu_frame->TPDU) - sizeof(send_pdu_frame->TPDU.TP_UD);
  
  while (length-- > 0)
  {
    *pdu_data++ = (uint8_t)(0x00ff & (*content >> 8));
    *pdu_data++ = (uint8_t)(0x00ff & *content++);
  }

  hex_to_string(send_pdu_string, (uint8_t *)send_pdu_frame, sms_pdu_length);
  /*
  rt_device_write(device, 0, "AT+CMGF=0\r", strlen("AT+CMGF=0\r"));
  rt_thread_delay(50);
  rt_device_read(device, 0, process_buf, 512);

  gsm_put_char(process_buf, strlen(process_buf));
  if (sscanf(process_buf, "%*[^\r]\r\r%[^\r]\r\n", at_temp) == 1)
  {
    gsm_put_char(at_temp, strlen(at_temp));
    //success send
  }
  else
  {
    gsm_put_char("AT+CMFG=0 is error\n", strlen("AT+CMFG=0 is error\n"));
    //error send
    //goto send_error;
  }
  */


  memset(at_temp, '\0', 512);
  memset(process_buf, '\0', 512);
  siprintf(at_temp,"AT+CMGS=%d\x0D",
           send_pdu_frame->TPDU.TP_UDL + (sizeof(send_pdu_frame->TPDU) - sizeof(send_pdu_frame->TPDU.TP_UD)));
  rt_device_write(device, 0, at_temp, strlen(at_temp));
  rt_thread_delay(50);
  rt_device_read(device, 0,  process_buf, 50);

  gsm_put_char(process_buf, strlen(process_buf));
  if (sscanf(process_buf, "%*[^>]%[>]", at_temp) == 1)
  {
    gsm_put_char(at_temp, strlen(at_temp));
    //success send
  }
  else
  {
    //error send
    gsm_put_char("AT+CMGS is error\n", strlen("AT+CMGS is error\n"));
    //goto send_error;
  }

  memset(at_temp, '\0', 512);
  memset(process_buf, '\0', 512);
  rt_device_write(device, 0, send_pdu_string, strlen(send_pdu_string));
  rt_device_write(device, 0, "\x1A", 1);
  rt_thread_delay(600);
  rt_device_read(device, 0, process_buf, 512);

  gsm_put_char(process_buf, strlen(process_buf));
  if (sscanf(process_buf, "%*[^:]:%*[^\r]\r\n%[^\r]", at_temp) == 1)
  {
    gsm_put_char(at_temp, strlen(at_temp));
    //success send
    
  }
  else
  {
    gsm_put_char("SEND SMS is error\n", strlen("SEND SMS is error\n"));
    //error send
    goto send_error;
  }

  rt_free(send_pdu_frame);
  rt_free(send_pdu_string);
  rt_free(at_temp);
  rt_free(process_buf);
  return 0;

send_error:
  rt_free(send_pdu_frame);
  rt_free(send_pdu_string);
  rt_free(at_temp);
  rt_free(process_buf);
  return -1;

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
