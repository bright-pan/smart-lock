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

#define 	MMS_RESEND_NUM		3					

static const char  title[] ={0xfe,0xff,0x60,0xA6,0x5F,0xB7,0x66,0x7A,0x80,0xFD};	//ÖÇÄÜÔÃµÂ
static const char  text[] ={0xfe,0xff,0x5F,0x53,0x52,0x4D,0x64,0x44,0x50,0xCF,0x59,0x34,0x60,0xC5,0x51,0xB5};		//"²ÊÐÅok


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
//	mms->pic_name[1] = "/1.jpg";
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

void mms_mail_process_thread_entry(void *parameter)
{
  rt_err_t 					result;
  rt_uint32_t 			event;
  struct mms_dev 		mms_send_struct;			//mms send data struct 
  rt_uint8_t				i;										//loop 	variable
	extern rt_timer_t mms_recv_cmd_t;
	
  /* malloc a buff for process mail */
  MMS_MAIL_TYPEDEF *mms_mail_buf = (MMS_MAIL_TYPEDEF *)rt_malloc(sizeof(MMS_MAIL_TYPEDEF));
  /* initial msg queue */
  mms_mq = rt_mq_create("mms", sizeof(MMS_MAIL_TYPEDEF), \
                         MMS_MAIL_MAX_MSGS, \
                         RT_IPC_FLAG_FIFO);

  mms_recv_cmd_t = rt_timer_create("mms_time",mms_timer_fun,RT_NULL,10,RT_TIMER_FLAG_PERIODIC);
	if(RT_NULL == mms_recv_cmd_t)
	{
		rt_kprintf("¡ï\"mms_time\" creat fail\n",mms_recv_cmd_t);
	}
  while (1)
  {
    /* process mail */
    memset(mms_mail_buf, 0, sizeof(MMS_MAIL_TYPEDEF));
    result = rt_mq_recv(mms_mq, mms_mail_buf, sizeof(MMS_MAIL_TYPEDEF), 100);

    /* mail receive ok */
    if (result == RT_EOK)
    {
      rt_kprintf("receive mms mail < time: %d alarm_type: %d >\n", mms_mail_buf->time, mms_mail_buf->alarm_type);

      rt_mutex_take(mutex_gsm_mode, RT_WAITING_FOREVER);
      result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND, 100, &event);
      if (result == RT_EOK)
      {
        if (gsm_mode_get() & EVENT_GSM_MODE_GPRS)
        {
          rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
          rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_GPRS_CMD);
          result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_GPRS_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &event);
          if ((result == RT_EOK) && !(gsm_mode_get() & EVENT_GSM_MODE_GPRS_CMD))
          {
            rt_kprintf("\ngsm mode switch to gprs_cmd is error, and try resend|\n");
            // clear gsm setup event, do gsm check or initial for test gsm problem.
            rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
            if (!(gsm_mode_get() & EVENT_GSM_MODE_CMD))
            {
              rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);
              rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);
              result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &event);
              if ((result == RT_EOK) && !(gsm_mode_get() & EVENT_GSM_MODE_CMD))
              {
                rt_kprintf("\ngsm mode switch to cmd is error, and try resend!\n");
              }
            }
            rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);
            rt_mq_urgent(mms_mq, mms_mail_buf, sizeof(MMS_MAIL_TYPEDEF));
            rt_mutex_release(mutex_gsm_mode);
            continue;
          }
          
        }
        // send mms data
        rt_kprintf("\nsend mms data!!!\n");
			
				mms_data_init(&mms_send_struct);		//init mms function

				for(i = 0 ; i < MMS_RESEND_NUM ;i++)
				{
					mms_send_fun(&mms_send_struct);		//send mms

					/*		not A fatal error	*/						
					if(!(mms_send_struct.error &= MMS_ERROR_FLAG(MMS_ERROR_1_FATAL)))
					{
						rt_kprintf("¡ÑMMS Send ok !\n");
						break;
					}
					mms_data_init(&mms_send_struct);		//init mms function
				}
				if(i == MMS_RESEND_NUM)					//send fial
				{
					rt_kprintf("\nsend mms failure!!!\n");
					// send failure
					rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_SETUP, RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER , &event);

					if (!(gsm_mode_get() & EVENT_GSM_MODE_CMD))
					{
						rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_NO, &event);

						rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);

						result = rt_event_recv(event_gsm_mode_response, EVENT_GSM_MODE_CMD, RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &event);
						if ((result == RT_EOK) && !(gsm_mode_get() & EVENT_GSM_MODE_CMD))
						{
							rt_kprintf("\ngsm mode switch to cmd is error, and try resend|\n");
						}
					}
					rt_event_send(event_gsm_mode_request, EVENT_GSM_MODE_CMD);

					rt_mq_urgent(mms_mq, mms_mail_buf, sizeof(MMS_MAIL_TYPEDEF));
				}
      }
      else // gsm is not setup
      {
        rt_mq_urgent(mms_mq, mms_mail_buf, sizeof(MMS_MAIL_TYPEDEF));
      }

      // exit mail process
      rt_mutex_release(mutex_gsm_mode);
    }
    else
    {
      /* mail receive error */
    }
  }
  rt_free(mms_mail_buf);
}
