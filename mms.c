/*********************************************************************
 * Filename:    mms.c
 *
 * Description:  This file realize MMC fun using hardware
 *						platfform SIM900A cmd version 1.0
 *						
 * Author:        wangzw <wangzw@yuettak.com>
 * Created at:    2013-05-29 18:20:00
 *                
 * Modify:
 *
 * 
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/
#include "mms.h"
#include "testprintf.h"


rt_sem_t mms_test_sem = RT_NULL;



/*******************************************************************************
* Function Name  : mcc_send_pic_fun
* Description    : send one pic data go to GSM modle
*                  
* Input				: mms  picture pos
* Output			: None
* Return         	: None
*******************************************************************************/
void mms_send_pic_fun(mms_dev_t mms, rt_uint8_t pic_pos)
{
	int file_id;
	rt_uint32_t size = 0;
	rt_uint8_t	*buffer;

	buffer = (rt_uint8_t *)rt_malloc(1024);

	rt_memset((rt_uint8_t *)buffer,0,1024);
	
	if(pic_pos < PER_MMC_PIC_MAXNUM)
	{
		file_id = open(mms->pic_name[pic_pos],O_RDONLY,0);
		do
		{
			size = read(file_id,buffer,1024);
			
		//	rt_device_write(mms->usart,0,buffer,size);

		}
		while(size>0);
		close(file_id);
	}
	rt_free(buffer);
}

#define MMS_BUFFER_SIZE			100
void mms_send_at_cmd(mms_dev_t mms)
{
	rt_uint8_t *buffer = RT_NULL;

	buffer = (rt_uint8_t *)rt_malloc(MMS_BUFFER_SIZE);
	
	rt_device_write(mms->usart,0,"AT+CMMSINIT\r",rt_strlen("AT+CMMSINIT\r"));	//��ʼ��
	
	rt_device_write(mms->usart,0,"at+cmmscurl=\"mmsc.monternet.com\"\r",rt_strlen("at+cmmscurl=\"mmsc.monternet.com\"\r"));	//����URL
	
	rt_thread_delay(99);
	
	rt_device_write(mms->usart,0,"at+cmmscid=1\r",rt_strlen("at+cmmscid=1\r"));	//����URL
	
	rt_thread_delay(99);
	
	rt_device_write(mms->usart,0,"at+cmmsproto=\"10.0.0.172\",80\r",rt_strlen("at+cmmsproto=\"10.0.0.172\",80\r"));//����mmsЭ������			
	
	rt_thread_delay(99);
	
	rt_device_write(mms->usart,0,"at+cmmssendcfg=6,3,0,0,2,4\r",rt_strlen("at+cmmssendcfg=6,3,0,0,2,4\r"));//����mms�ķ��Ͳ���			
	
	rt_thread_delay(99);
	
	rt_device_write(mms->usart,0,"AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r",rt_strlen("AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r"));
	
	rt_thread_delay(99);
	
	rt_device_write(mms->usart,0,"AT+SAPBR=3,1,\"APN\",\"CMWAP\"\r",rt_strlen("AT+SAPBR=3,1,\"APN\",\"CMWAP\"\r"));	 //���ݾ������������ΪCMWAP����CMNET
	
	rt_thread_delay(99);
	
	rt_device_write(mms->usart,0,"AT+SAPBR=1,1\r",rt_strlen("AT+SAPBR=1,1\r"));
	
	rt_thread_delay(99);
	
	rt_device_write(mms->usart,0,"AT+SAPBR=2,1\r",rt_strlen("AT+SAPBR=2,1\r"));
	
	rt_thread_delay(99);
	
	rt_device_write(mms->usart,0,"AT+CGATT?\r",rt_strlen("AT+CGATT?\r"));//��ѯ��ģ���Ƿ�֧��mms
	
	rt_thread_delay(99);
	
	rt_device_write(mms->usart,0,"AT+CMMSEDIT=0\r",rt_strlen("AT+CMMSEDIT=0\r"));//�رձ༭״̬��������ģ���buff�������ݽ���ɾ��	
	
	rt_thread_delay(99);
	
	rt_device_write(mms->usart,0,"AT+CMMSEDIT=1\r",rt_strlen("AT+CMMSEDIT=1\r"));//�򿪱༭״̬�����״̬�ſ��Է���mms			
	
	rt_thread_delay(99);

	//
	rt_memset(buffer,0,100);
	rt_sprintf((char *)buffer,"at+cmmsdown=\"pic\",%d,100000\r",mms->pic_size[0]);	//mms picture of size 
	
	rt_device_write(mms->usart,0,(const rt_uint8_t*)buffer,rt_strlen((const char *)buffer));//���÷��͵�ͼƬ�Ĵ�С(byte)����Ҫ���������ʱ��100000ms��4051������ִ�sscom32.exe�õ��ģ���ͼƬ��ʱ����ʾ��������
	
	rt_thread_delay(99);

	mms_send_pic_fun(mms,0);		//send picture data
	
	rt_device_write(mms->usart,0,"\r",1);//��������
	
	rt_thread_delay(199);
	
	rt_memset(buffer,0,100);
	
	rt_sprintf((char *)buffer,"at+cmmsdown=\"title\",%d,30000\r",rt_strlen(mms->title));	//mms picture of size 

	rt_device_write(mms->usart,0,(const rt_uint8_t*)buffer,rt_strlen((const char*)buffer));
//	rt_device_write(mms->usart,0,"at+cmmsdown=\"title\",3,30000\r");//���÷��͵�mms�����֣���С��ʱ��		
	rt_thread_delay(99);
	rt_device_write(mms->usart,0,mms->title,rt_strlen(mms->title));//send title data 
	rt_thread_delay(99);

	
	rt_memset(buffer,0,100);
		
	rt_sprintf((char *)buffer,"at+cmmsdown=\"text\",%d,30000\r",rt_strlen(mms->text));	
	
	rt_device_write(mms->usart,0,(const rt_uint8_t*)buffer,rt_strlen((const char*)buffer));//mms picture of size 
//	rt_device_write(mms->usart,0,"at+cmmsdown=\"text\",5,30000\r");//���÷��͵�mms���ı����ݣ�С��1000bytes			
	rt_thread_delay(99);
//	rt_device_write(mms->usart,0,"ilove");//�����ı�����			
	rt_device_write(mms->usart,0,mms->text,rt_strlen(mms->text));//send title data 

	rt_thread_delay(99);
	
	rt_memset(buffer,0,100);
		
	rt_sprintf((char *)buffer,"at+cmmsrecp=\"%s\"\r",mms->mobile_no[0]);	//mms picture of size 
	
//	rt_device_write(mms->usart,0,"at+cmmsrecp=\"13424385329\"\r");//���͵ĺ��룬�Լ��޸�		

	rt_device_write(mms->usart,0,(const rt_uint8_t*)buffer,rt_strlen((const char*)buffer));//���͵ĺ��룬�Լ��޸�		
	
	rt_thread_delay(99);
	
	rt_device_write(mms->usart,0,"at+cmmsview\r",strlen("at+cmmsview\r"));//�鿴���͵������Ƿ��Ѿ�����ģ��
	
	rt_thread_delay(99);
	
	rt_device_write(mms->usart,0,"at+cmmssend\r",strlen("at+cmmssend\r"));//����mms		
	
	rt_thread_delay(300);		

	rt_free(buffer);
	
	buffer = RT_NULL;
}

void gprs_config_IFC(rt_uint8_t mode)
{
	rt_device_t device;

	device  = rt_device_find("g_usart");
	switch(mode)
	{
		case 0:
		{
			rt_device_write(device,0,"AT+IFC=0,0\r",rt_strlen("AT+IFC=0,0\r"));
			
			break;
		}
		case 1:
		{
			rt_device_write(device,0,"AT+IFC=1,1\r",rt_strlen("AT+IFC=1,1\r"));
			
			break;
		}
		case 2:
		{
			rt_device_write(device,0,"AT+IFC=2,2\r",rt_strlen("AT+IFC=2,2\r"));
			
			break;
		}
	}
	rt_device_write(device,0,"AT&W\r",rt_strlen("AT&W\r"));
}




void gsm_cmd_cmp(rt_device_t dev,const char *cmd)
{
//	rt_device_read(dev,0,);
}
void rt_mms_data_init(mms_dev_t mms)
{
	struct stat status;
	rt_device_t dev = RT_NULL;
	const char* title = "���Ų���";
	const char* text = "����ͨ��";

	dev = rt_device_find("g_usart");
	
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
	mms->mobile_no[0] = "13544033975";
	mms->number = 2;
	mms->pic_name[0] = "/1.jpg";
	mms->pic_name[1] = "/2.jpg";
	
	stat(mms->pic_name[0],&status);
	mms->pic_size[0] = status.st_size;

	stat(mms->pic_name[1],&status);
	mms->pic_size[1] = status.st_size;

	mms->number = 2;

	mms->title = title;
	mms->text = text;
//	rt_strncpy(mms->title,"������Ϣ",rt_strlen("������Ϣ"));
//	rt_strncpy(mms->text,"���Ų���",rt_strlen("���Ų���"));
}
void rt_mms_thread_entry(void *arg)
{
	struct mms_dev mms;

	rt_mms_data_init(&mms);
	mms_info(&mms);
	while(1)
	{	
		rt_sem_take(mms_test_sem,RT_WAITING_FOREVER);

		mms_send_at_cmd(&mms);
		mms_info(&mms);
	}
}



void rt_mms_thread_init(void)
{
	rt_thread_t thread_id = RT_NULL;

	thread_id= rt_thread_create("dealpic",rt_mms_thread_entry,RT_NULL,1024*4,29,10);
	if(RT_NULL == thread_id)
	{
	rt_kprintf("\"dealpic\" create fial\n");
	return ;
	}
	rt_thread_startup(thread_id);	

	mms_test_sem = rt_sem_create("mms_sem",0,RT_IPC_FLAG_FIFO);
	if(RT_NULL == mms_test_sem)
	{
		rt_kprintf(" \"mms_sem\" sem create fail\n");

		return ;
	}
}



#ifdef RT_USING_FINSH
#include <finsh.h>

void sendMMS(void)
{
	rt_err_t result;
	
	rt_sem_release(mms_test_sem);
}

FINSH_FUNCTION_EXPORT(sendMMS, sendMMS());

void statu()
{
	struct stat status;

	stat("/1.jpg",&status);

	rt_kprintf("1.jpg size %d \n",status.st_size);
}
FINSH_FUNCTION_EXPORT(statu, statu());

#endif




