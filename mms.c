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
#include <string.h>
#include "untils.h"
#include "gprs.h"

#define MMS_RESEND_NUM 3

static const char  mms_title[] ={0xfe,0xff,0x60,0xA6,0x5F,0xB7,0x66,0x7A,0x80,0xFD}; //ÖÇÄÜÔÃµÂ
static const char  mms_text[] ={0xfe,0xff,0x5F,0x53,0x52,0x4D,0x64,0x44,0x50,0xCF,0x59,0x34,0x60,0xC5,0x51,0xB5}; //"²ÊÐÅok

rt_mq_t mms_mq = RT_NULL;

int8_t send_mms(char *pic_name)
{
  struct stat status;
  int file_id;
  int8_t result = -1;
  uint32_t read_size = 0;
  uint8_t *process_buf = (uint8_t *)rt_malloc(512);
  uint8_t alarm_telephone_counts;
  GSM_MAIL_CMD_DATA cmd_data;

  if (stat(pic_name,&status))
  {
    rt_kprintf("\nsend mms but get picture size is error!!!\n");
    return result;
  }
  send_cmd_mail(AT_CMMSEDIT_CLOSE, 50, &cmd_data);
  send_cmd_mail(AT_SAPBR_CLOSE, 50, &cmd_data);
  send_cmd_mail(AT_CMMSTERM, 50, &cmd_data);

  //active bearer profile
  if ((send_cmd_mail(AT_SAPBR_CONTYPE, 50, &cmd_data) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_SAPBR_APN_CMWAP, 50, &cmd_data) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_SAPBR_OPEN, 50, &cmd_data) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_SAPBR_REQUEST, 50, &cmd_data) == AT_RESPONSE_OK))
  {
    // mms initial
    if ((send_cmd_mail(AT_CMMSINIT, 50, &cmd_data) == AT_RESPONSE_OK) &&
        (send_cmd_mail(AT_CMMSCURL, 50, &cmd_data) == AT_RESPONSE_OK) &&
        (send_cmd_mail(AT_CMMSCID, 50, &cmd_data) == AT_RESPONSE_OK) &&
        (send_cmd_mail(AT_CMMSPROTO, 50, &cmd_data) == AT_RESPONSE_OK) &&
        (send_cmd_mail(AT_CMMSSENDCFG, 50, &cmd_data) == AT_RESPONSE_OK))
    {
      //send mms
      //open mms edit
      if (send_cmd_mail(AT_CMMSEDIT_OPEN, 50, &cmd_data) == AT_RESPONSE_OK)
      {
        cmd_data.cmmsdown_title.length = sizeof(mms_title);
        if (send_cmd_mail(AT_CMMSDOWN_TITLE, 50, &cmd_data) == AT_RESPONSE_CONNECT_OK)
        {
          cmd_data.cmmsdown_data.buf = (uint8_t *)mms_title;
          cmd_data.cmmsdown_data.length = sizeof(mms_title);
          cmd_data.cmmsdown_data.has_complete = 1;
          if(send_cmd_mail(AT_CMMSDOWN_DATA, 50, &cmd_data) == AT_RESPONSE_OK)
          {
            cmd_data.cmmsdown_text.length = sizeof(mms_text);
            if (send_cmd_mail(AT_CMMSDOWN_TEXT, 50, &cmd_data) == AT_RESPONSE_CONNECT_OK)
            {
              cmd_data.cmmsdown_data.buf = (uint8_t *)mms_text;
              cmd_data.cmmsdown_data.length = sizeof(mms_text);
              cmd_data.cmmsdown_data.has_complete = 1;
              if (send_cmd_mail(AT_CMMSDOWN_DATA, 50, &cmd_data) == AT_RESPONSE_OK)
              {
                cmd_data.cmmsdown_pic.length = status.st_size;
                if (send_cmd_mail(AT_CMMSDOWN_PIC, 50, &cmd_data) == AT_RESPONSE_CONNECT_OK)
                {
                  extern rt_mutex_t pic_file_mutex;
                  rt_mutex_take(pic_file_mutex,RT_WAITING_FOREVER);

                  file_id = open(pic_name,O_RDONLY,0);

                  if (file_id < 0)
                  {
                    rt_kprintf("\nopen picture error!!!\n");
                    rt_mutex_release(pic_file_mutex);
                    goto process_result;
                  }
                  do {

                    memset(process_buf,0,512);
                    if (status.st_size > 512)
                    {
                      read_size = read(file_id,process_buf,512);
                    }
                    else
                    {
                      read_size = read(file_id,process_buf,status.st_size);
                    }

                    status.st_size -= read_size;

                    if (status.st_size > 0)
                    {
                      cmd_data.cmmsdown_data.buf = process_buf;
                      cmd_data.cmmsdown_data.length = read_size;
                      cmd_data.cmmsdown_data.has_complete = 0;

                      send_cmd_mail(AT_CMMSDOWN_DATA, 50, &cmd_data);
                    }
                    else
                    {
                      cmd_data.cmmsdown_data.buf = process_buf;
                      cmd_data.cmmsdown_data.length = read_size;
                      cmd_data.cmmsdown_data.has_complete = 1;

                      if (send_cmd_mail(AT_CMMSDOWN_DATA, 50, &cmd_data) == AT_RESPONSE_OK)
                      {
                      }
                    }
                  }while (status.st_size > 0);
                  if(close(file_id))
                  {
                    rt_kprintf("close file error\n");
                  }


                  rt_mutex_release(pic_file_mutex);
                  // send telephone
                  alarm_telephone_counts = 0;
                  while (alarm_telephone_counts < TELEPHONE_NUMBERS)
                  {
                    if (device_parameters.alarm_telephone[alarm_telephone_counts].flag)
                    {
                      cmd_data.cmmsrecp.buf = (uint8_t *)(device_parameters.alarm_telephone[alarm_telephone_counts].address + 2);
                      if (send_cmd_mail(AT_CMMSRECP, 50, &cmd_data) != AT_RESPONSE_OK)
                      {
                        rt_kprintf("set telephone %s error\n", (device_parameters.alarm_telephone[alarm_telephone_counts].address + 2));
                      }
                    }
                    alarm_telephone_counts++;
                  }
                  cmd_data.cmmsrecp.buf = (uint8_t *)"21255274@qq.com";
                  if (send_cmd_mail(AT_CMMSRECP, 50, &cmd_data) == AT_RESPONSE_OK)
                  {
                    if (send_cmd_mail(AT_CMMSSEND, 50, &cmd_data) == AT_RESPONSE_OK)
                    {
                      result = 0;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
process_result:
  send_cmd_mail(AT_CMMSEDIT_CLOSE, 50, &cmd_data);
  send_cmd_mail(AT_SAPBR_CLOSE, 50, &cmd_data);
  send_cmd_mail(AT_CMMSTERM, 50, &cmd_data);

  rt_free(process_buf);
  return result;
}

void mms_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  /* malloc a buff for process mail */
  MMS_MAIL_TYPEDEF mms_mail_buf;

  while (1)
  {
    /* process mail */
    memset(&mms_mail_buf, 0, sizeof(MMS_MAIL_TYPEDEF));
    result = rt_mq_recv(mms_mq, &mms_mail_buf, sizeof(MMS_MAIL_TYPEDEF), 100);
    /* mail receive ok */
    if (result == RT_EOK)
    {
      rt_kprintf("\nreceive mms mail < time: %d alarm_type: %d >\n", mms_mail_buf.time, mms_mail_buf.alarm_type);
      rt_mutex_take(mutex_gsm_mail_sequence,RT_WAITING_FOREVER);
      if (!send_mms(mms_mail_buf.pic_name))
      {
      	static ALARM_TYPEDEF alarm_type ;
      	
      	alarm_type = mms_mail_buf.alarm_type;
      	send_gprs_mail(ALARM_TYPE_GPRS_UPLOAD_PIC,0, 0,(void *)&alarm_type);
        rt_kprintf("\nsend mms success!!!\n");
      }
      else
      {
        rt_kprintf("\nsend mms failure!!!\n");
      }
      rt_mutex_release(mutex_gsm_mail_sequence);
    }
    else
    {
      /* mail receive error */
    }
  }
}


void send_mms_mail(ALARM_TYPEDEF alarm_type, time_t time, char *pic_name)
{
  MMS_MAIL_TYPEDEF buf;
  extern rt_device_t rtc_device;
  rt_err_t result;
  //send mail
  buf.alarm_type = alarm_type;
  buf.pic_name = pic_name;
  if (!time)
  {
    rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &(buf.time));
  }
  else
  {
    buf.time = time;
  }
  if (mms_mq != NULL)
  {
    result = rt_mq_send(mms_mq, &buf, sizeof(MMS_MAIL_TYPEDEF));
    if (result == -RT_EFULL)
    {
      rt_kprintf("mms_mq is full!!!\n");
    }
  }
  else
  {
    rt_kprintf("mms_mq is RT_NULL!!!\n");
  }
}


#ifdef RT_USING_FINSH
#include <finsh.h>

FINSH_FUNCTION_EXPORT(send_mms_mail, send_mms_mail[index time pic_name])
#endif
