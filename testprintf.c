/*********************************************************************
 * Filename:      testprintf.c
 *
 * Description:	test file test ok after delete
 *
 * Author:        wangzw <wangzw@yuettak.com>
 * Created at:    2013-05-17 14:22:03
 *                
 * Modify:
 *
 * 
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/
#include "testprintf.h"
#include "stm32f10x.h"
//#include "gbcode.h"
#include <time.h>
#include "alarm.h"
#include "sms.h"
#include "gprs.h"
#include "mms.h"
#include "alarm.h"
#include "SPI_Flash.h"
#include "file_update.h"








void printf_write_to_file(void)
{
	static rt_uint8_t i = 0;
	
	i++;
	i %= 5;
	switch(i)
	{
		case 0:
		{
			rt_kprintf("\rWrite Data To File .    ");
			break;
		};
		case 1:
		{
			rt_kprintf("\rWrite Data To File ..   ");
			break;
		};
		case 2:
		{
			rt_kprintf("\rWrite Data To File ...  ");
			break;
		};
		case 3:
		{
			rt_kprintf("\rWrite Data To File ....");
			break;
		};
		case 4:
		{
			rt_kprintf("\rWrite Data To File .....");
			break;
		};
	}

}
void printf_camera(camera_dev_t camera)
{
	rt_kprintf("\ncamera->size    = %d\n",camera->size);
	rt_kprintf("camera->page    = %d\n",camera->page);
	rt_kprintf("camera->surplus = %d\n",camera->surplus);
	rt_kprintf("camera->addr    = %d\n",camera->addr);
	rt_kprintf("camera->error   = %d\n",camera->error);
	rt_kprintf("camera->time    = %d\n",camera->time);
	rt_kprintf("camera->device    = %s\n",camera->device->parent.name);
	rt_kprintf("camera->glint_led = %s\n",camera->glint_led->parent.name);
	rt_kprintf("camera->power     = %s\n",camera->power->parent.name);
	
}








void idle_thread_work(void)
{
	static rt_uint16_t old_temp = 0;
	rt_device_t device;
	rt_uint16_t	temp = 0;
	rt_uint8_t	len = 0;

	device = rt_device_find("IC_T");

	if(device != RT_NULL)
	{
		len = rt_device_read(device,0,&temp,1);
		temp = (0x6EE - temp)/0x05+25;
		if((old_temp != temp) && len)
		{
			
			old_temp = temp;
			rt_kprintf("stm32f103ze temperature = %d ��\n",temp);
		}
	}
}

void mms_info(mms_dev_t mms)
{
	rt_uint8_t	i = 0;

	rt_kprintf("/*****************MMS DATA*************/");
	rt_kprintf("mms->error        %d\n",mms->error);
	rt_kprintf("error0 %d error1 %d error2 %d error3 %d \n",mms->error_record[0],mms->error_record[1],mms->error_record[2],mms->error_record[3]);
	for(i = 0 ; mms->mobile_no[i] != RT_NULL;i++)
	{
		rt_kprintf("mms->mobile_no[%d]		%s\n",i,mms->mobile_no[i]);
		if(mms->number >= MOBILE_NO_NUM)
		{
			break;
		}
	}
	rt_kprintf("mms->number       %d\n",mms->number);
	for(i =0 ;mms->pic_name[i] != RT_NULL;i++)
	{
		rt_kprintf("mms->pic_name[%d]  %s size %d\n",i,mms->pic_name[i],mms->pic_size[i]);
	}
	rt_kprintf("mms->text  size %03d %s\n",mms->text.size,mms->text.string);
	rt_kprintf("mms->title size %03d %s\n",mms->title.size,mms->title.string);
	rt_kprintf("usart name   %s\n",mms->usart->parent.name);
	rt_kprintf("/**************************************/");
}

void printf_dev_data(rt_device_t dev,rt_uint8_t *buffer)
{
	rt_device_read(dev,0,buffer,100);
	rt_kprintf("%s\n",buffer);
}

void printf_loop_string(rt_uint8_t *buffer,rt_uint32_t size)
{
	rt_uint8_t* head = buffer;
	while(size--)
	{
		rt_kprintf("%02x ",*head);
		head++;
	}
}


void gsm_hw_reset(void)
{
	rt_device_t dev;
	rt_uint8_t dat = 1;

	dev = rt_device_find("g_power");

	if(RT_NULL != dev)
	{
		rt_device_write(dev,0,&dat,1);

		dat = 0;
		rt_thread_delay(100);

		rt_device_write(dev,0,&dat,1);
	}
}

void printf_format_string(rt_uint8_t *addr,rt_uint8_t format,rt_uint8_t len)
{
	rt_uint8_t i = 0;

	for(i =0;i<len;i++)
	{
		switch(format)
		{
			case 'd':
			{
				rt_kprintf("%d ",*addr++);
				break;
			}
			case 'x':
			{
				rt_kprintf("%x ",*addr++);
				break;
			}
		}
	}
}



#ifdef RT_USING_FINSH
#include <finsh.h>
/*  modification terminal display  */
void terminal()
{
	rt_kprintf("\033[1;40;32m");
}
FINSH_FUNCTION_EXPORT(terminal,terminal() --  change terminal color);


void sysreset()
{
	NVIC_SystemReset();
}
FINSH_FUNCTION_EXPORT(sysreset,sysreset() -- reset stm32);

void creat_file(const char* file_name,rt_uint8_t *data,rt_uint8_t size)
{
	int file_id;

	file_id = open(file_name,O_CREAT | O_RDWR,0);

	rt_kprintf("size = %d\n",sizeof(data));
	
	write(file_id,data,size);

	close(file_id);
}
FINSH_FUNCTION_EXPORT(creat_file,creat_file(file_name,data,size) -- new creat file);

void sysstandby(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP,ENABLE);
	
	PWR_WakeUpPinCmd(ENABLE);
	
	PWR_EnterSTANDBYMode();
}
FINSH_FUNCTION_EXPORT(sysstandby,sysstandby()--system standby);
void sysstop()
{
	PWR_EnterSTOPMode(PWR_Regulator_LowPower,PWR_STOPEntry_WFI);
}
FINSH_FUNCTION_EXPORT(sysstop,sysstop()--system stop);
void syssleep(void)
{
	NVIC_SystemLPConfig(NVIC_LP_SLEEPDEEP,ENABLE);
}
FINSH_FUNCTION_EXPORT(syssleep,syssleep()--system sleep);

void ICT()
{
	rt_device_t device;
	rt_uint16_t	temp = 0;

	device = rt_device_find("IC_T");

	if(device != RT_NULL)
	{
		rt_device_read(device,0,&temp,1);

		rt_kprintf("stm32f103ze temperature = %d ��",(0x6EE - temp)/0x05+25);
	}
}
FINSH_FUNCTION_EXPORT(ICT,ICT() -- stm32f103ve chip internal temperature);

void setT_dis(void)
{
	rt_thread_idle_sethook(idle_thread_work);
}
FINSH_FUNCTION_EXPORT(setT_dis,set ilde display temperature);
/*
void cputest()
{
	extern void cpu_usage_init();
	extern void cpu_usage_get(rt_uint8_t *major, rt_uint8_t *minor);
	rt_uint8_t major,minor;

	cpu_usage_init();
	
	cpu_usage_get(&major,&minor);

	rt_kprintf("CPU Usage :%d.%d %\r\n",major,minor);
}
FINSH_FUNCTION_EXPORT(cputest,--cputest);
*/
#define GPRS_READ_USART_LEN		128
void ch_dev_in(const char *name)
{
	rt_device_t device;
	volatile rt_uint8_t	data[GPRS_READ_USART_LEN]={0,};

//	data = (rt_uint8_t *)rt_malloc(sizeof(rt_uint8_t)*GPRS_READ_USART_LEN);
	
	device = rt_device_find(name);

//	rt_kprintf("%d",);

	rt_device_read(device,0,(void*)data,GPRS_READ_USART_LEN);
	
	rt_kprintf("rcv = %s\n",data);

//	free(data);

}
FINSH_FUNCTION_EXPORT(ch_dev_in,ch_dev_in(name)--read char device printf);


void ch_dev_out(const char *name,rt_int8_t* data)
{
	rt_device_t device;
	rt_uint8_t	size;

	size = rt_strlen((char *)data);

//	rt_kprintf("size = %d\n",size);
	
	device = rt_device_find(name);
	
	rt_device_write(device,0,data,size);

}
FINSH_FUNCTION_EXPORT(ch_dev_out,(name,data)--write to data char device);

void ch_dev_rdwd(const char *name,const char* data)
{
	rt_device_t device;
	rt_uint8_t	size;

	size = rt_strlen((char *)data);

//	rt_kprintf("size = %d\n",size);

	device = rt_device_find(name);

	if(device == RT_NULL)
	{
	  return ;
	}

	rt_device_write(device,0,data,size);

	rt_thread_delay(10);
	
	ch_dev_in(name);

}
FINSH_FUNCTION_EXPORT(ch_dev_rdwd,(name,data)--write to data char device and retuern info);
/*
rt_uint32_t gb_unicode(rt_uint8_t *gb,rt_uint16_t *get_uin)
{
	
	extern rt_uint16_t GB2Unicode(rt_uint16_t GBCode);
	rt_uint16_t tmp = 0;
	rt_uint32_t cnt = 0;
	
	while(*gb != '\0')
	{
		tmp = *gb;

		if(tmp < 128)
		{
			*get_uin = tmp;
			gb++;
		}
		else
		{
			gb++;
			tmp = (tmp<<8) | *gb;
			gb++;
			*get_uin = GB2Unicode(tmp);
		}		
		get_uin++;
		cnt++;
	}
	return cnt;
}
FINSH_FUNCTION_EXPORT(gb_unicode,(GB2313)--printf unicode2 hex);
void get_unicode(rt_uint8_t*name)
{
	rt_uint16_t unicode[10];
	int i;
	rt_uint32_t size;

	size = gb_unicode(name,unicode);

	for(i = 0; i < size ; i++)
	{
		rt_kprintf("%04x ",unicode[i]);
	}
	rt_kprintf("\n");
}
FINSH_FUNCTION_EXPORT(get_unicode,(GB2312)--printf unicode2 hex);

void unicode_gb(rt_uint16_t *get_uin,rt_uint8_t *gb,rt_uint32_t size)
{
	extern rt_uint16_t Unicode2GB(rt_uint16_t Unicode);
	rt_uint16_t tmp;

	while(size--)
	{
		tmp = Unicode2GB(*get_uin);
		if(tmp < 128)
		{
			*gb = tmp&0x00ff;
		}
		else
		{
			*gb = (tmp>>8);
			gb++;
			*gb = tmp&0x00ff;
		}
		
		gb++;
		get_uin++;
	}
}
void get_gb(void)
{
	rt_uint8_t str[200];
	rt_uint16_t unicode[100] = {0x60A6,0x5FB7,0x667A,0x80FD,0x006F,0x006B,0x0031,0x0032,0x0033,0xFF0C};
	
	unicode_gb(unicode,str,50);

	rt_kprintf("%s",str);

}
FINSH_FUNCTION_EXPORT(get_gb,(GB2313)--printf unicode2 hex);


void sizeof_test(void)
{
	rt_uint8_t len;

	len = sizeof("test len");
	rt_kprintf("get len = %d",len);
	rt_kprintf("real len is 8\n");
}
FINSH_FUNCTION_EXPORT(sizeof_test,(void)--test sizeof use);



*/
	void gsm_run()
	{
		gsm_hw_reset();
	}
	FINSH_FUNCTION_EXPORT(gsm_run,(void)--reset gsm modle);


void time_test()
{
	time_t t_v;
	struct tm t;
	
	time(&t_v);	
	t = *localtime(&t_v);

	rt_kprintf("%d,%d,%d,%d,%d,%d,\n",t.tm_year,t.tm_mon,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec);
}
FINSH_FUNCTION_EXPORT(time_test,time test);

void smms(void)
{
	time_t now_t;
	extern rt_mq_t mms_mq;
	MMS_MAIL_TYPEDEF *mms_mail_buf = (MMS_MAIL_TYPEDEF *)rt_malloc(sizeof(MMS_MAIL_TYPEDEF));

	time(&now_t);
	mms_mail_buf->time = now_t;
        mms_mail_buf->alarm_type = ALARM_TYPE_CAMERA_IRDASENSOR;

	rt_mq_send(mms_mq,mms_mail_buf,sizeof(MMS_MAIL_TYPEDEF));
}
FINSH_FUNCTION_EXPORT(smms,send mq mms_mq);

void f_n_l(const char *name)
{
	int file_id;

	if(RT_NULL!= name)
	{
		file_id = open(name,O_CREAT | O_RDWR,0);

		write(file_id,"13544033975",sizeof("13544033975"));

		close(file_id);	
	}
}
FINSH_FUNCTION_EXPORT(f_n_l,send mq mms_mq);
void gprs_pic(void)
{
	static ALARM_TYPEDEF type = ALARM_TYPE_LOCK_TEMPERATURE;
	
	send_gprs_mail(ALARM_TYPE_GPRS_UPLOAD_PIC,0, 0,(void *)&type);
}
FINSH_FUNCTION_EXPORT(gprs_pic,gprs send picture);

volatile rt_uint32_t debug_delay1 = 500;

void set_delay(rt_uint32_t t)
{
	debug_delay1 = t;
}

FINSH_FUNCTION_EXPORT(set_delay,set delay time);

void light_ir_test(rt_uint8_t	status)
{
	rt_uint8_t dat = 0;
	rt_device_t dev = RT_NULL;
	rt_device_t	led = RT_NULL;
	rt_uint32_t	loop = 0xffffffff;

	dev = rt_device_find("cm_photo");
	led = rt_device_find("cm_led");
	if((RT_NULL!= dev)||(RT_NULL != led))
	{
		while(loop--)
		{
			rt_device_read(dev,0,&dat,1);
			if(1 == status)
			{
				if(1 == dat)
				{
					rt_device_write(led,0,&status,1);
				}
			}
			else if(0 == status)
			{
				rt_device_write(led,0,&status,1);
			}
		}
	}
}
FINSH_FUNCTION_EXPORT(light_ir_test,Infrared and optical test);

void flashtest(void)
{
	rt_base_t temp;
	rt_uint8_t	buffer[20] = "wang zhou wang";

	SST25_W_BLOCK(FLASH_START_WRITE_ADDR,buffer,20);

	SST25_R_BLOCK(FLASH_START_WRITE_ADDR,buffer,20);
	temp = rt_hw_interrupt_disable();

	rt_kprintf("read flash data : %s\n\n",buffer);
	rt_hw_interrupt_enable(temp);
}
FINSH_FUNCTION_EXPORT(flashtest,test flash intterupt);

void file_move_flash(const char *file_name)
{

	rt_uint32_t i = 0; 
	rt_uint8_t	data;
	struct  stat  Info;
	extern rt_uint8_t file_write_flash_addr(const char *file_name,
                                         rt_uint32_t addr);
	stat(file_name,&Info);	
	if(file_write_flash_addr(file_name,FLASH_START_WRITE_ADDR) == 1)
	{
		rt_kprintf("File move fail\n");
	}

	rt_thread_delay(500);
	for(i = FLASH_START_WRITE_ADDR; i <FLASH_START_WRITE_ADDR + Info.st_size;i++)
	{
		SST25_R_BLOCK(i,&data,1);

		rt_kprintf("%c",data);
	}
}
FINSH_FUNCTION_EXPORT(file_move_flash,file system file move flash start addr);

void gprs_test_timer_entry(void *arg)
{
	static rt_uint32_t	cnt = 0;
	cnt++;
	if((cnt%15) == 0)//15 min send gprs picture
	{
		gprs_pic();
	}
}
void test_gprs_pic(void)
{	
	rt_timer_t gprs_test_timer;

	gprs_test_timer = rt_timer_create("g_test",
																		gprs_test_timer_entry,
																		RT_NULL,
																		6000,
																		RT_TIMER_FLAG_PERIODIC);
	rt_timer_start(gprs_test_timer);
}
FINSH_FUNCTION_EXPORT(test_gprs_pic,succession send gprs pictuer data);

#endif

