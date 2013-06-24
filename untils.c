/*********************************************************************
 * Filename:      untils.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-22 09:25:39
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "untils.h"
#include <dfs_init.h>
#include <dfs_elm.h>
#include <dfs_fs.h>
#include "dfs_posix.h"

#define SYSTEM_CONFIG_FILE_NAME					"/config"

void delay_us(uint32_t time)
{
  uint8_t nCount;
  while(time--)
  {
    for(nCount = 6 ; nCount != 0; nCount--);
  }
}


DEVICE_PARAMETERS_TYPEDEF device_parameters = {
  //alarm telephone
  {
    {
      0,
      "8613316975697"
    },
    {
      1,
      "8613544033975"
    },
  },
  //call telephone
  {
    {
      1,
      "8613316975697"
    },
    {
      0,
      "8613316975697"
    },
  },
  // rfid key
  {
    {
      1,
      "\x86\x6E\xEB\x14"
    },
  },
  {
    "iyuet.com",
    6800
  },
  //lock gate alarm time
  30,
  //device id
  {0x01,0xA1,0x00,0x01,0x01,0x02},
  //key0
  {0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31},
  //key1
  {0x00,0x00,0xCB,0x17,0x62,0x2F,0x7A,0xC5},
};


/*******************************************************************************
* Function Name  : system_file_operate
* Description    :  system config file operate
*                  
* Input				: flag :1>>write ; 0>>read
* Output			: None
* Return         	: None
*******************************************************************************/
void system_file_operate(DEVICE_PARAMETERS_TYPEDEF *arg,rt_uint8_t flag)
{
	int file_id;
	
	file_id = open(SYSTEM_CONFIG_FILE_NAME,O_CREAT | O_RDWR,0);
	if(file_id < 0)
	{
		rt_kprintf("config file not open\n");
		return ;
	}
	if(flag)//write 
	{
		write(file_id,(const void*)arg,sizeof(DEVICE_PARAMETERS_TYPEDEF));
	}
	else//read
	{
		read(file_id,arg,sizeof(DEVICE_PARAMETERS_TYPEDEF));
	}
	close(file_id);
}

#ifdef RT_USING_FINSH
#include <finsh.h>

void sys_file(void)
{
	DEVICE_PARAMETERS_TYPEDEF arg;
	rt_uint8_t i;
	
	system_file_operate(&device_parameters,1);
	
	system_file_operate(&arg,0);

	for(i =0; i< 10;i++)
	{
		rt_kprintf("arg.call_telephone[%d].flag = %d \n",i,arg.alarm_telephone[i].flag);
		rt_kprintf("arg.call_telephone[%d].address =%s \n",i,arg.alarm_telephone[i].address);
	}
	for(i =0; i< 10;i++)
	{
		rt_kprintf("arg.call_telephone[%d].flag = %d \n",i,arg.call_telephone[i].flag);
		rt_kprintf("arg.call_telephone[%d].address =%s \n",i,arg.call_telephone[i].address);
	}
	rt_kprintf("arg.tcp_domain.domain =%s \n",arg.tcp_domain.domain);
	rt_kprintf("arg.tcp_domain.port = %d \n",arg.tcp_domain.port);
	for(i =0; i< 10;i++)
	{
		rt_kprintf("arg.rfid_key[%d].flag = %d \n",i,arg.rfid_key[i].flag);
		rt_kprintf("arg.rfid_key[%d].key = %s \n",i,arg.rfid_key[i].key);
	}
	rt_kprintf("arg.lock_gate_alarm_time = %d \n",arg.lock_gate_alarm_time);
	rt_kprintf("arg.device_id = %s \n",arg.device_id);
	rt_kprintf("arg.key0 = %s \n",arg.key0);
	rt_kprintf("arg.key1 = %s \n",arg.key1);
}

FINSH_FUNCTION_EXPORT(sys_file,--system file config);

#endif

