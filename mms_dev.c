/*********************************************************************
 * Filename:    mms_dev.c
 *
 * Description:  This file realize MMC fun using hardware
 *               platfform SIM900A cmd version 1.0
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
#include "mms_dev.h"
#include "testprintf.h"

rt_sem_t mms_test_sem = RT_NULL;
volatile rt_uint32_t mms_timer_value = 0;
rt_timer_t  mms_recv_cmd_t = RT_NULL;

static void delay(rt_uint8_t t)
{
  while(t--);
}
void mms_timer_fun(void *arg)
{
  mms_timer_value++;
}

/*******************************************************************************
 * Function Name  : mms_recv_cmd_result
 * Description    : Check cmd deal result whether right
 *                  
 * Input: mms data    cmd :compare cmd wait_time:
 *                    wait_time < 0:GSM return ERROR Is believed to be correct
 *                    wait_time > 0:GSM return include 
 * Output: None
 *******************************************************************************/

#define MMS_RECV_CMD_LEN 200
void mms_recv_cmd_result(mms_dev_t mms,rt_int32_t wait_time,const char cmd[],...)
{
  rt_uint8_t*			buffer = RT_NULL;
  rt_uint8_t*			buffer_head = RT_NULL;
  char*						str_result = RT_NULL;
  rt_size_t 			size = 0;
  rt_size_t 			cnt = 0;
  rt_bool_t				deal_flag = 0;

  if(wait_time < 0)
  {
    deal_flag = 1;
    wait_time = 0 - wait_time;
  }
  buffer_head = buffer = (rt_uint8_t *)rt_malloc(sizeof(rt_uint8_t)*MMS_RECV_CMD_LEN);

  rt_memset((rt_uint8_t *)buffer,0,MMS_RECV_CMD_LEN);
	
  rt_timer_start(mms_recv_cmd_t);

  rt_device_read(mms->usart,0,buffer,MMS_RECV_CMD_LEN);

  while(mms_timer_value < wait_time)
  {		
    size = rt_device_read(mms->usart,0,buffer,1);
    if(1 == size)
    {
      buffer++;
      if((buffer - buffer_head) >= MMS_RECV_CMD_LEN)
      {
        buffer = buffer_head;
      }
      cnt++;
    }
    str_result = rt_strstr((const char *)buffer_head,cmd);
    if(str_result != RT_NULL)
    {
      rt_kprintf(">>>>cmd ok");
			
      break;
    }
    if(1 == deal_flag)
    {	
      str_result = rt_strstr((const char *)buffer_head,"ERROR");
    }
    if(str_result != RT_NULL)
    {
      rt_kprintf(">>>>cmd ok");
		
      mms->error &= ~(MMS_ERROR_FLAG(MMS_ERROR_0_CMD));
      break;
    }
  }
  if(mms_timer_value >= wait_time)
  {
    mms->error |= MMS_ERROR_FLAG(MMS_ERROR_0_CMD);
    mms->error_record[MMS_ERROR_0_CMD]++;
    rt_kprintf(">>>>cmd error");
  }
  rt_timer_stop(mms_recv_cmd_t);
  rt_kprintf("mms_timer_value = %d",mms_timer_value);
  mms_timer_value = 0;

  rt_kprintf(">>>>    %s",buffer_head);
	
  rt_kprintf(">>>>    %d error %d\n",cnt,mms->error_record[MMS_ERROR_0_CMD]);

  rt_free(buffer_head);
}
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
  int 								file_id;
  rt_uint32_t 				size = 0;
  volatile rt_uint8_t	buffer;
  rt_uint32_t 				fzise = 0;
	
  if(pic_pos < PER_MMC_PIC_MAXNUM)
  {
    rt_kprintf("%s",mms->pic_name[pic_pos]);
    file_id = open(mms->pic_name[pic_pos],O_RDONLY,0);
    if(file_id < 0)
    {
      rt_kprintf("★open file fial\n");
      return ;
    }
    do
    {
      size = read(file_id,(rt_uint8_t *)&buffer,1);
      delay(250);//delay(250);delay(250);delay(250);delay(250);delay(250);
      if(size > 0)
      {
        rt_device_write(mms->usart,0,(const char *)&buffer,size);
      }
      delay(250);//delay(250);delay(250);delay(250);delay(250);delay(250);
      fzise++;
			
    }
    while(size>0);
		
    if(close(file_id) == 0)
    {
      rt_kprintf("close file ok\n");
    }
  }
  rt_kprintf(">>>\n%d\n",fzise);
}

void mms_send_exit_cmd(mms_dev_t mms,rt_int8_t time)
{
  rt_device_write(mms->usart,0,"AT+CMMSEDIT=0\r",rt_strlen("AT+CMMSEDIT=0\r"));//exit MMS edit 
  mms_recv_cmd_result(mms,time,"OK");
  rt_device_write(mms->usart,0,"AT+CMMSTERM\r",rt_strlen("AT+CMMSTERM\r"));//exit MMS function
  mms_recv_cmd_result(mms,time,"OK");
  rt_device_write(mms->usart,0,"AT+SAPBR=0,1\r",rt_strlen("AT+SAPBR=0,1\r"));//
  mms_recv_cmd_result(mms,time,"OK");
}


#define MMS_BUFFER_SIZE			120

void mms_send_pic_cmd_fun(mms_dev_t mms,rt_uint8_t* buffer,rt_uint8_t mms_pos)
{
  rt_memset(buffer,0,MMS_BUFFER_SIZE);
	
  rt_sprintf((char *)buffer,"at+cmmsdown=\"PIC\",%d,100000\r",mms->pic_size[mms_pos]);	//mms picture of size 
	
  rt_device_write(mms->usart,0,(const rt_uint8_t*)buffer,rt_strlen((const char *)buffer));//设置发送的图片的大小(byte)和需要用来传输的时间100000ms，4051这个数字从sscom32.exe得到的，打开图片的时候显示在最上面
	
  mms_recv_cmd_result(mms,10,"CONNECT");

  rt_kprintf("enter picture read \n");
  mms_send_pic_fun(mms,mms_pos);				//send picture data

  rt_device_write(mms->usart,0,"\r",1);	//结束符号
	
  mms_recv_cmd_result(mms,10,"OK");

}


void mms_send_title_cmd_fun(mms_dev_t mms,rt_uint8_t* buffer)
{
  rt_memset(buffer,0,MMS_BUFFER_SIZE);
		
  rt_sprintf((char *)buffer,"at+cmmsdown=\"title\",%d,30000\r",mms->title.size);	//mms picture of size 
	
  rt_device_write(mms->usart,0,(const rt_uint8_t*)buffer,rt_strlen((const char*)buffer));
	
		
  mms_recv_cmd_result(mms,10,"CONNECT");
		
  rt_device_write(mms->usart,0,mms->title.string,mms->title.size);//send title data 
	
  //	printf_format_string(mms->title.string,'x',8);
	
  rt_device_write(mms->usart,0,"\r",1);//结束符号
	
  mms_recv_cmd_result(mms,10,"OK");

}


void mms_send_text_cmd_fun(mms_dev_t mms,rt_uint8_t* buffer)
{
  rt_memset(buffer,0,MMS_BUFFER_SIZE);	

  rt_sprintf((char *)buffer,"at+cmmsdown=\"TEXT\",%d,30000\r",mms->text.size);	

  rt_device_write(mms->usart,0,(const rt_uint8_t*)buffer,rt_strlen((const char*)buffer));//设置发送的mms的文本内容，小于1000bytes		

  mms_recv_cmd_result(mms,10,"CONNECT");

  rt_device_write(mms->usart,0,mms->text.string,mms->text.size);//send title data 发送文本内容			
  rt_device_write(mms->usart,0,"\r",1);//结束符号

  mms_recv_cmd_result(mms,10,"OK");
}

void mms_set_phone_no_fun(mms_dev_t mms,rt_uint8_t* buffer,rt_uint8_t pos)
{
  rt_memset(buffer,0,MMS_BUFFER_SIZE);

	if(*mms->mobile_no[pos] == '+')
	{
		mms->mobile_no[pos]++;
	}
	if(*mms->mobile_no[pos] == '8')
	{
		mms->mobile_no[pos]++;
	}
	if(*mms->mobile_no[pos] == '6')
	{
		mms->mobile_no[pos]++;
	}
	rt_sprintf((char *)buffer,"at+cmmsrecp=\"%s\"\r",mms->mobile_no[pos]);	//mms picture of size 

  //	rt_device_write(mms->usart,0,"at+cmmsrecp=\"13544033975\"\r",rt_strlen("at+cmmsrecp=\"13544033975\"\r"));//发送的号码，自己修改		

  rt_device_write(mms->usart,0,(const rt_uint8_t*)buffer,rt_strlen((const char*)buffer));//发送的号码，自己修改		
	
  mms_recv_cmd_result(mms,10,"OK");
}

void mms_send_data_struct_deal(mms_dev_t mms,rt_uint8_t* buffer)
{
  rt_uint8_t i;

  for(i = 0; i < PER_MMC_PIC_MAXNUM;i++)
  {
    if(RT_NULL == mms->pic_name[i])
    {
      rt_kprintf("i = %d\n",i);
			
      break;
    }
    mms_send_pic_cmd_fun(mms,buffer,i);		//send second picture
  }

  mms_send_title_cmd_fun(mms,buffer);		//send title cmd and content

  mms_send_text_cmd_fun(mms,buffer);		//send txte cmd and content 

  for(i = 0; i < MOBILE_NO_NUM;i++)
  {
    if(RT_NULL == mms->mobile_no[i])
    {
      rt_kprintf("i = %d\n",i);
			
      break;
    }
    mms_set_phone_no_fun(mms,buffer,i);			//set phone no
  }
}


rt_uint8_t mms_send_error_deal(mms_dev_t mms,rt_uint8_t deal_type)
{
  rt_uint8_t i;

  switch(deal_type)
  {
    case 0:
      {
        for(i = 0; i < MMS_ERROR_TYPE_NUM; i++)
        {
          if((mms->error_record[i] != 0)||(mms->error &= MMS_ERROR_FLAG(MMS_ERROR_0_CMD)))
          {
            return 1;
          }
        }
        break;
      }
    case 1:
      {
        rt_kprintf("enter into");
        if(mms->error_record[MMS_ERROR_0_CMD] > MMS_ERROR_DEAL_FLAG)
        {
          return 1;
        }
        break;
      }
  }
  return 0;
}
/*mms cmd-------------------------------*/
/*mms_cmd[] = 
  {
  "AT+CMMSINIT\r",
  "at+cmmscurl=\"mmsc.monternet.com\"\r",
  "at+cmmscid=1\r",
  "at+cmmsproto=\"10.0.0.172\",80\r",
  "at+cmmssendcfg=6,3,0,0,2,4\r",
  "AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r",
  "AT+SAPBR=3,1,\"APN\",\"CMWAP\"\r",
  "AT+SAPBR=1,1\r",
  "AT+SAPBR=2,1\r",
  "AT+CGATT?\r",
  "AT+CMMSEDIT=0\r",
  "AT+CMMSEDIT=1\r",
  .PIC
  .TITLE
  .TXET
  .Mobile phone number
  "at+cmmsview\r",
  "at+cmmssend\r",
  }
  ________________________________________*/
void mms_send_fun(mms_dev_t mms)
{
  rt_uint8_t *buffer = RT_NULL;
	
  buffer = (rt_uint8_t *)rt_malloc(MMS_BUFFER_SIZE);
	
  rt_memset(buffer,0,MMS_BUFFER_SIZE);
	
  mms_recv_cmd_result(mms,10,"OK");			//release gsm usart data make one cmd error 
  mms->error &= ~(MMS_ERROR_FLAG(MMS_ERROR_0_CMD));	//clear this make of error
  mms->error_record[MMS_ERROR_0_CMD]--;	//	Elimination of the last error logging			
	
  mms_send_exit_cmd(mms,-20);

  rt_device_write(mms->usart,0,"AT+CMMSINIT\r",rt_strlen("AT+CMMSINIT\r"));	//初始化
	
  mms_recv_cmd_result(mms,10,"OK");
	
  rt_device_write(mms->usart,0,"at+cmmscurl=\"mmsc.monternet.com\"\r",rt_strlen("at+cmmscurl=\"mmsc.monternet.com\"\r"));	//设置URL

  mms_recv_cmd_result(mms,10,"OK");
	
  rt_device_write(mms->usart,0,"at+cmmscid=1\r",rt_strlen("at+cmmscid=1\r"));	//设置URL
	
  mms_recv_cmd_result(mms,10,"OK");

  if(mms_send_error_deal(mms,1))						//eeror  check up
  {
    mms->error |= MMS_ERROR_FLAG(MMS_ERROR_1_FATAL);
    mms->error_record[MMS_ERROR_1_FATAL]++;
		
    return ;
  }
		
  rt_device_write(mms->usart,0,"at+cmmsproto=\"10.0.0.172\",80\r",rt_strlen("at+cmmsproto=\"10.0.0.172\",80\r"));//设置mms协议属性			
	
  mms_recv_cmd_result(mms,10,"OK");
	
  rt_device_write(mms->usart,0,"at+cmmssendcfg=6,3,0,0,2,4\r",rt_strlen("at+cmmssendcfg=6,3,0,0,2,4\r"));//设置mms的发送参数			
	
  mms_recv_cmd_result(mms,10,"OK");
	
  rt_device_write(mms->usart,0,"AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r",rt_strlen("AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r"));
	
  mms_recv_cmd_result(mms,10,"OK");
	
  rt_device_write(mms->usart,0,"AT+SAPBR=3,1,\"APN\",\"CMWAP\"\r",rt_strlen("AT+SAPBR=3,1,\"APN\",\"CMWAP\"\r"));	 //根据具体情况，可设为CMWAP或者CMNET
	
  mms_recv_cmd_result(mms,10,"OK");
	
  rt_device_write(mms->usart,0,"AT+SAPBR=1,1\r",rt_strlen("AT+SAPBR=1,1\r"));
	
  mms_recv_cmd_result(mms,40,"OK");
	
  rt_device_write(mms->usart,0,"AT+SAPBR=2,1\r",rt_strlen("AT+SAPBR=2,1\r"));
	
  mms_recv_cmd_result(mms,10,"OK");
	
  rt_device_write(mms->usart,0,"AT+CGATT?\r",rt_strlen("AT+CGATT?\r"));//查询该模块是否支持mms
	
  mms_recv_cmd_result(mms,10,"OK"); 
	
  rt_device_write(mms->usart,0,"AT+CMMSEDIT=0\r",rt_strlen("AT+CMMSEDIT=0\r"));//关闭编辑状态，这样在模块的buff区的数据将被删除	
	
  mms_recv_cmd_result(mms,10,"OK");
	
  rt_device_write(mms->usart,0,"AT+CMMSEDIT=1\r",rt_strlen("AT+CMMSEDIT=1\r"));//打开编辑状态，这个状态才可以发送mms			
	
  mms_recv_cmd_result(mms,50,"OK");

  mms_send_data_struct_deal(mms,buffer);

  rt_device_write(mms->usart,0,"at+cmmsview\r",rt_strlen("at+cmmsview\r"));//查看发送的内容是否已经存入模块
	
  mms_recv_cmd_result(mms,50,"OK");

//	rt_device_write(mms->usart,0,"at+cmmssend\r",rt_strlen("at+cmmssend\r"));//发送mms		

  mms->error &= ~(MMS_ERROR_FLAG(MMS_ERROR_1_FATAL)); //Is ready to receive and send the results

//	mms_recv_cmd_result(mms,1800,"OK");		//max wait send ok time 3minute

  if(mms_send_error_deal(mms,0))						//eeror  check up
  {
    mms->error |= MMS_ERROR_FLAG(MMS_ERROR_1_FATAL);
    mms->error_record[MMS_ERROR_1_FATAL]++;
		
    return ;
  }
		
  mms_send_exit_cmd(mms,40);

  rt_free(buffer);
}




void mms_get_send_file_size(mms_dev_t mms,rt_uint8_t pos)
{
  struct stat status;
	
  if(pos < PER_MMC_PIC_MAXNUM)
  {
  	if(mms->pic_name[pos] != RT_NULL)
  	{
			stat(mms->pic_name[pos],&status);
			mms->pic_size[pos] = status.st_size;	
  	}
    else
    {
			rt_kprintf("mms send picture name file %d is RT_NULL\n",pos);
    }
  }
  else
  {	
    rt_kprintf("error operate\n");
  }
}

void mms_error_record_init(mms_dev_t mms)
{
  rt_uint8_t i;

  for(i = 0; i < MMS_ERROR_TYPE_NUM ; i++)
  {
    mms->error_record[i] = 0;
  }
}

static const char  title[] ={0xfe,0xff,0x60,0xA6,0x5F,0xB7,0x66,0x7A,0x80,0xFD};	//智能悦德
static const char  text[] ={0xfe,0xff,0x5F,0x53,0x52,0x4D,0x64,0x44,0x50,0xCF,0x59,0x34,0x60,0xC5,0x51,0xB5};		//"彩信ok

void rt_mms_data_init(mms_dev_t mms)
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
  mms_error_record_init(mms);

	
  mms->mobile_no[0] = "1678764298@qq.com";
  mms->mobile_no[1] = "939463111@qq.com";
  mms->mobile_no[2] = RT_NULL;
	
  mms->number = 1;
  mms->pic_name[0] = "/2.jpg";	//send picture path
	mms->pic_name[1] = "/1.jpg";
  mms->pic_name[1] = RT_NULL;

  mms_get_send_file_size(mms,0);//get picture size
	mms_get_send_file_size(mms,1);
	
  mms->number = 2;

  mms->title.size = sizeof(title);
  mms->title.string = title;

  mms->text.size = sizeof(text);
  mms->text.string = text;
}
void rt_mms_thread_entry(void *arg)
{
  struct mms_dev mms;

  mms_recv_cmd_t = rt_timer_create("mms_time",mms_timer_fun,RT_NULL,10,RT_TIMER_FLAG_PERIODIC);
  if(RT_NULL == mms_recv_cmd_t)
  {
    rt_kprintf("\"mms_time\" creat fail\n",mms_recv_cmd_t);
  }

  rt_mms_data_init(&mms);
	
  mms_info(&mms);
  while(1)
  {	
    rt_sem_take(mms_test_sem,RT_WAITING_FOREVER);

    mms_send_fun(&mms);

	//rt_thread_delay(100);

	//rt_sem_release(mms_test_sem);
		
    mms_info(&mms);
  }
}



void rt_mms_thread_init(void)
{
  rt_thread_t thread_id = RT_NULL;

  thread_id= rt_thread_create("dealpic",rt_mms_thread_entry,RT_NULL,1024*4,107,10);
  if(RT_NULL == thread_id)
  {
    rt_kprintf("\"dealpic\" create fial\n");
    return ;
  }
  rt_thread_startup(thread_id);	

  mms_test_sem = rt_sem_create("mms_dev",0,RT_IPC_FLAG_FIFO);
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

