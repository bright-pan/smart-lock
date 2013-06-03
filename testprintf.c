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
		temp = (0x6EE-temp)/0x05+25;
		if((old_temp != temp) && len)
		{
			
			old_temp = temp;
			rt_kprintf("stm32f103ze temperature = %d ¡æ\n",temp);
		}
	}
}

void mms_info(mms_dev_t mms)
{
	rt_kprintf("mms->error        %d\n",mms->error);
	rt_kprintf("mms->mobile_no    %s\n",mms->mobile_no[0]);
	rt_kprintf("mms->number       %d\n",mms->number);
	rt_kprintf("mms->pic_name[0]  %s\n",mms->pic_name[0]);
	rt_kprintf("mms->pic_name[1]  %s\n",mms->pic_name[1]);
	rt_kprintf("mms->pic_size[0]  %d\n",mms->pic_size[0]);
	rt_kprintf("mms->pic_size[1]  %d\n",mms->pic_size[1]);
	rt_kprintf("mms->text    %s\n",mms->text);
	rt_kprintf("mms->title   %s\n",mms->title);
	rt_kprintf("usart name   %s\n",mms->usart->parent.name);
}


void printf_loop_string(rt_uint8_t *buffer,rt_uint32_t size)
{
	while(size--)
	{
		rt_kprintf("%c",*buffer);
		buffer++;
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

		rt_kprintf("stm32f103ze temperature = %d ¡æ",(0x6EE-temp)/0x05+25);
	}
}
FINSH_FUNCTION_EXPORT(ICT,ICT() -- stm32f103ve chip internal temperature);

void setT_dis(void)
{
	rt_thread_idle_sethook(idle_thread_work);
}
FINSH_FUNCTION_EXPORT(setT_dis,set ilde display temperature);

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

#define GPRS_READ_USART_LEN		1024
void ch_dev_in(const char *name)
{
	rt_device_t device;
	rt_uint8_t*	data;

	data = (rt_uint8_t *)rt_malloc(sizeof(rt_uint8_t)*GPRS_READ_USART_LEN);

	rt_memset((rt_uint8_t *)data,0,GPRS_READ_USART_LEN);
	
	device = rt_device_find(name);

//	rt_kprintf("%d",);

	rt_device_read(device,0,data,GPRS_READ_USART_LEN);
	
	rt_kprintf("%s\n",data);

	free(data);

}
FINSH_FUNCTION_EXPORT(ch_dev_in,ch_dev_in(name)--read char device printf);


void ch_dev_out(const char *name,rt_int8_t* data)
{
	rt_device_t device;
	rt_uint8_t	size;

	size = strlen((char *)data);

	rt_kprintf("size = %d\n",size);
	
	device = rt_device_find(name);
	
	rt_device_write(device,0,data,size);

}
FINSH_FUNCTION_EXPORT(ch_dev_out,(name,data)--write to data char device);

void ch_dev_rdwd(const char *name,rt_int8_t* data)
{
	rt_device_t device;
	rt_uint8_t	size;

	size = strlen((char *)data);

	rt_kprintf("size = %d\n",size);

	device = rt_device_find(name);

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


#endif

