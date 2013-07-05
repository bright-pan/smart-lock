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

#ifndef __GNUC__
void *memmem(const void *haystack, size_t haystack_len,
                const void *needle, size_t needle_len)
{
  const char *begin = haystack;
  const char *last_possible = begin + haystack_len - needle_len;
  const char *tail = needle;
  char point;

  /*
   * The first occurrence of the empty string is deemed to occur at
   * the beginning of the string.
   */
  if (needle_len == 0)
    return (void *)begin;

  /*
   * Sanity check, otherwise the loop might search through the whole
   * memory.
   */
  if (haystack_len < needle_len)
    return NULL;

  point = *tail++;
  for (; begin <= last_possible; begin++) {
    if (*begin == point && !memcmp(begin + 1, tail, needle_len - 1))
      return (void *)begin;
  }

  return NULL;
}

#endif

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
      "\x86\x6E\xEB\x14" //866eeb14
    },
  },
  {
    "iyuet.com",
    4123
  },
  //lock gate alarm time
  30,
  //device id
  {0x01,0xA1,0x00,0x01,0x01,0x01},
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

void sys_file(rt_uint8_t status)
{
	DEVICE_PARAMETERS_TYPEDEF arg;
	rt_uint8_t i;

	if(status == 1)
	{
		system_file_operate(&device_parameters,1);
	}
	
	system_file_operate(&arg,0);

	rt_kprintf("/********************************\nbelow is system cur config file content\n\n");
	for(i =0; i< 10;i++)
	{
		rt_kprintf("arg.alarm_telephone[%d].flag    = %d \n",i,arg.alarm_telephone[i].flag);
		rt_kprintf("arg.alarm_telephone[%d].address = %s \n",i,arg.alarm_telephone[i].address);
		if(arg.alarm_telephone[i+1].address[0] == '\0' )
		{
			break;
		}
	}
	for(i =0; i< 10;i++)
	{
		rt_kprintf("arg.call_telephone[%d].flag    = %d \n",i,arg.call_telephone[i].flag);
		rt_kprintf("arg.call_telephone[%d].address = %s \n",i,arg.call_telephone[i].address);
		if(arg.call_telephone[i+1].address[0] == '\0' )
		{
			break;
		}
	}
	rt_kprintf("arg.tcp_domain.domain     = %s \n",arg.tcp_domain.domain);
	rt_kprintf("arg.tcp_domain.port       = %d \n",arg.tcp_domain.port);
	for(i =0; i< 10;i++)
	{
		rt_kprintf("arg.rfid_key[%d].flag   = %d \n",i,arg.rfid_key[i].flag);
		rt_kprintf("arg.rfid_key[%d].key    = %x %x %x %x\n",
								i,
								arg.rfid_key[i].key[0],
								arg.rfid_key[i].key[1],
								arg.rfid_key[i].key[2],
								arg.rfid_key[i].key[3]);
								
		if(arg.rfid_key[i+1].key[0] ==  '\0')
		{
			break;
		}
	}
	rt_kprintf("arg.lock_gate_alarm_time  = %d \n",arg.lock_gate_alarm_time);
	rt_kprintf("arg.device_id = %02x%02x%02x%02x%02x%02x \n",
							arg.device_id[0],
							arg.device_id[1],
							arg.device_id[2],
							arg.device_id[3],
							arg.device_id[4],
							arg.device_id[5]);
																										
	rt_kprintf("arg.key0 = %02x%02x%02x%02x%02x%02x%02x%02x \n",
							arg.key0[0],
							arg.key0[1],
							arg.key0[2],
							arg.key0[3],
							arg.key0[4],
							arg.key0[5],
							arg.key0[6],
							arg.key0[7]);
															
	rt_kprintf("arg.key1 = %02x%02x%02x%02x%02x%02x%02x%02x\n\n",
							arg.key1[0],
							arg.key1[1],
							arg.key1[2],
							arg.key1[3],
							arg.key1[4],
							arg.key1[5],
							arg.key1[6],
							arg.key1[7]);
}

FINSH_FUNCTION_EXPORT(sys_file,--system file config);

#endif

