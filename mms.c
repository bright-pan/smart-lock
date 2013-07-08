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
#include "mms_dev.h"
#include "testprintf.h"
#include "gprs.h"

#define MMS_RESEND_NUM 3

static const char  title[] ={0xfe,0xff,0x60,0xA6,0x5F,0xB7,0x66,0x7A,0x80,0xFD}; //ÖÇÄÜÔÃµÂ
static const char  text[] ={0xfe,0xff,0x5F,0x53,0x52,0x4D,0x64,0x44,0x50,0xCF,0x59,0x34,0x60,0xC5,0x51,0xB5}; //"²ÊÐÅok

rt_mq_t mms_mq;

void mms_telephone_init(mms_dev_t mms)
{
  rt_uint8_t telephone_num;
	
  telephone_num = TELEPHONE_NUMBERS;
  mms->number = 0;
  while (telephone_num--)
  {
    if (device_parameters.alarm_telephone[telephone_num].flag)
    {
      mms->mobile_no[mms->number] = (rt_uint8_t *)device_parameters.alarm_telephone[telephone_num].address;//copy telephon
      if(mms->number >= MOBILE_NO_NUM)	//	Array Bounds Write
      {
        rt_kprintf("¡ïTelephone number copy Serious Problems !!!\n");
        break;
      }
      mms->number++;
    }
  }
  mms->mobile_no[mms->number] = RT_NULL;
}
void mms_picture_file_init(mms_dev_t mms)
{
  mms->pic_name[0] = "/2.jpg";	//send picture path
// 	mms->pic_name[1] = "/2.jpg";
  mms->pic_name[1] = RT_NULL;

  mms_get_send_file_size(mms,0);//get picture size
//	mms_get_send_file_size(mms,1);
	
}
void mms_data_init(mms_dev_t mms)
{
  rt_device_t dev = RT_NULL;
	

  dev = rt_device_find(MMS_USART_DEVICE_NAME);
	
  if(RT_NULL != dev)
  {
    mms->usart = dev;
    rt_kprintf("mms set usart ok\n");
  }
  else
  {
    mms->usart = RT_NULL;
  }
	
  mms->error = MMS_ERROR_OK;						
  mms_error_record_init(mms);						//error record clear

  mms_telephone_init(mms);							//init telephone relevant data

  mms_picture_file_init(mms);

	
  mms->title.size = sizeof(title);
  mms->title.string = title;

  mms->text.size = sizeof(text);
  mms->text.string = text;

  mms_info(mms);
}


int8_t send_mms(void)
{
  int8_t result = -1;
  // mms initial
  if ((send_cmd_mail(AT_CMMSINIT, 50, "", 0, 0) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_CMMSCURL, 50, "", 0, 0) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_CMMSCID, 50, "", 0, 0) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_CMMSPROTO, 50, "", 0, 0) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_CMMSSENDCFG, 50, "", 0, 0) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_SAPBR_CONTYPE, 50, "", 0, 0) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_SAPBR_APN, 50, "", 0, 0) == AT_RESPONSE_OK) &&
      (send_cmd_mail(AT_SAPBR_OPEN, 50, "", 0, 0) == AT_RESPONSE_OK))
  {
    
  }
  else
  {
    result = -1;
  }
}

void mms_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  /* malloc a buff for process mail */
  MMS_MAIL_TYPEDEF *mms_mail_buf = (MMS_MAIL_TYPEDEF *)rt_malloc(sizeof(MMS_MAIL_TYPEDEF));
  /* initial msg queue */
  mms_mq = rt_mq_create("mms", sizeof(MMS_MAIL_TYPEDEF),
                        MMS_MAIL_MAX_MSGS,
                        RT_IPC_FLAG_FIFO);
  while (1)
  {
    /* process mail */
    memset(mms_mail_buf, 0, sizeof(MMS_MAIL_TYPEDEF));
    result = rt_mq_recv(mms_mq, mms_mail_buf, sizeof(MMS_MAIL_TYPEDEF), 100);
    /* mail receive ok */
    if (result == RT_EOK)
    {
      rt_kprintf("receive mms mail < time: %d alarm_type: %d >\n", mms_mail_buf->time, mms_mail_buf->alarm_type);
      send_mms();
    }
    else
    {
      /* mail receive error */
    }
  }
  rt_free(mms_mail_buf);
}


void send_mms_mail(ALARM_TYPEDEF alarm_type, time_t time)
{
  MMS_MAIL_TYPEDEF buf;
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

FINSH_FUNCTION_EXPORT(send_mms_mail, send_mms_mail[index time])
#endif
