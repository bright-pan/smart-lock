/*********************************************************************
 * Filename:      aip.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-07-17 11:11:48
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "alarm.h"
#include "aip.h"
#include "gsm.h"
#include <string.h>
#include "untils.h"
#include "gprs.h"

#define AIP_BIN_NAME "/aip.bin"

rt_mq_t aip_mq = RT_NULL;

uint8_t aip_bin_http_address[100] = "http://116.255.185.77/30k.txt";
uint8_t aip_bin_http_address_length = 0;
int aip_bin_size = 0;

int8_t get_aip(void)
{
  struct stat status;
  int file_id;
  int8_t result = -1;
  uint32_t read_size = 0;
  uint8_t *process_buf = (uint8_t *)rt_malloc(512);
  GSM_MAIL_CMD_DATA cmd_data;
  int aip_size = 0;

  send_cmd_mail(AT_SAPBR_CLOSE, 50, &cmd_data);
  send_cmd_mail(AT_HTTPTERM, 50, &cmd_data);
  // mms initial
  //active bearer profile
  if ((send_cmd_mail(AT_SAPBR_CONTYPE, 50, &cmd_data) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_SAPBR_APN_CMNET, 50, &cmd_data) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_SAPBR_OPEN, 50, &cmd_data) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_SAPBR_REQUEST, 50, &cmd_data) == AT_RESPONSE_OK))
  {
    if ((send_cmd_mail(AT_HTTPINIT, 50, &cmd_data) == AT_RESPONSE_OK) &&
        (send_cmd_mail(AT_HTTPPARA_CID, 50, &cmd_data) == AT_RESPONSE_OK))
      {
        
        cmd_data.httppara_url.buf = aip_bin_http_address;
        cmd_data.httppara_url.length = aip_bin_http_address_length;
        if ((send_cmd_mail(AT_HTTPPARA_URL, 50, &cmd_data) == AT_RESPONSE_OK) &&
            (send_cmd_mail(AT_HTTPACTION_GET, 50, &cmd_data) == AT_RESPONSE_OK))
        {
          unlink(AIP_BIN_NAME);

          file_id = open(AIP_BIN_NAME,O_CREAT | O_RDWR, 0);
          if (file_id < 0)
          {
            rt_kprintf("\nopen aip.bin error!!!\n");
            goto process_result;
          }
          cmd_data.httpread.buf = process_buf;
          cmd_data.httpread.start = 0;
          cmd_data.httpread.length = &read_size;
          aip_size = aip_bin_size;
          do {
            memset(cmd_data.httpread.buf, 0, 512);
            *cmd_data.httpread.length = 0;
            if(send_cmd_mail(AT_HTTPREAD, 50, &cmd_data) == AT_RESPONSE_OK &&
               *cmd_data.httpread.length)
            {
              gsm_put_char(process_buf, 400);
              write(file_id, process_buf, *cmd_data.httpread.length);
            }
            else
            {
              if(close(file_id))
              {
                rt_kprintf("\nclose file error\n");
              }
              goto process_result;
            }
            aip_size -= *cmd_data.httpread.length;
            cmd_data.httpread.start += *cmd_data.httpread.length;
          }while(aip_size > 0);
          if(close(file_id))
          {
            rt_kprintf("\nclose file error\n");
            goto process_result;
          }
          if (stat(AIP_BIN_NAME, &status))
          {
            rt_kprintf("\naip.bin is not exist!!!\n");
            goto process_result;
          }
          if(status.st_size == aip_bin_size)
          {
            result = 0;
          }
        }
      }
  }

process_result:
  send_cmd_mail(AT_HTTPTERM, 50, &cmd_data);
  send_cmd_mail(AT_SAPBR_CLOSE, 50, &cmd_data);  

  rt_free(process_buf);
  return result;
}

void aip_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  /* malloc a buff for process mail */
  AIP_MAIL_TYPEDEF aip_mail_buf;

  while (1)
  {
    /* process mail */
    memset(&aip_mail_buf, 0, sizeof(AIP_MAIL_TYPEDEF));
    result = rt_mq_recv(aip_mq, &aip_mail_buf, sizeof(AIP_MAIL_TYPEDEF), 100);
    /* mail receive ok */
    if (result == RT_EOK)
    {
      rt_kprintf("\nreceive aip mail < time: %d alarm_type: %d >\n", aip_mail_buf.time, aip_mail_buf.alarm_type);
      rt_mutex_take(mutex_gsm_mail_sequence,RT_WAITING_FOREVER);
      if (!get_aip())
      {
        rt_kprintf("\nget http aip success!!!\n");        
      }
      else
      {
        rt_kprintf("\nget http aip failure!!!\n");
      }
      rt_mutex_release(mutex_gsm_mail_sequence);
    }
    else
    {
      /* mail receive error */
    }
  }
}


void send_aip_mail(ALARM_TYPEDEF alarm_type, time_t time)
{
  AIP_MAIL_TYPEDEF buf;
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
  if (aip_mq != NULL)
  {
    result = rt_mq_send(aip_mq, &buf, sizeof(AIP_MAIL_TYPEDEF));
    if (result == -RT_EFULL)
    {
      rt_kprintf("aip_mq is full!!!\n");
    }
  }
  else
  {
    rt_kprintf("aip_mq is RT_NULL!!!\n");
  }
}


#ifdef RT_USING_FINSH
#include <finsh.h>

FINSH_FUNCTION_EXPORT(send_aip_mail, send_aip_mail[index time pic_name])
#endif
