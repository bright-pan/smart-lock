/*********************************************************************
 * Filename:    camera.c
 *
 * Description:    usart camera test code
 *						  
 *						   
 * Author:        wangzw <wangzw@yuettak.com>
 * Created at:    2013-05-8 9:00:00
 *                
 * Modify:
 *
 * 
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/


#include "camera.h"
#include <dfs_init.h>
/* dfs filesystem:ELM filesystem init */
#include <dfs_elm.h>
/* dfs Filesystem APIs */
#include <dfs_fs.h>
#include "dfs_posix.h"

#define CAMERA_READ_NUM				1000


rt_uint8_t reset_cmd[] =				{0x56,0x00,0x26,0x00};
rt_uint8_t update[] = 					{0x56,0x00,0x36,0x01,0x02};
rt_uint8_t stop_cur_update[] =	{0x56,0x00,0x36,0x01,0x00};
rt_uint8_t get_pic_len[] =			{0x56,0x00,0x34,0x01,0x00};
rt_uint8_t get_pic_data[16] = 		{0x56,0x00,0x32,0x0C,0x00,0x0a,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00};
rt_uint8_t	recv_data_cmd[] = {0x76,0x00,0x32,0x00,0x00};

rt_uint8_t timer_value = 0;

void camera_get_hex_printf(rt_uint8_t data[],rt_uint8_t len)
{
	rt_uint8_t i = 0;
	
	while((len != i) &&( len != 0))
	{
		rt_kprintf("%x ",data[i++]);
	}
//	rt_kprintf("\n");
}
rt_bool_t camera_check_cmd(rt_uint8_t data[],rt_uint8_t cmd[],rt_uint8_t cmd_len)
{
	rt_uint8_t i = 0;

//	rt_kprintf("data[] = ");
	for(i = 0; i < cmd_len; i++)
	{
	//	rt_kprintf("%x ",data[i]);
		if(data[i] != cmd[i])
		{
			rt_kprintf("cmd error\n");
			return 0;
		}
	}
//	rt_kprintf("\n");
	return 1;
}
rt_bool_t camera_receive_data_cmd(rt_device_t device,rt_uint8_t cmd_data[],rt_uint8_t cmd_len)
{
	rt_uint8_t data = 0;
	rt_uint8_t len = 0;
	rt_uint8_t cur_pos = 0;
	rt_uint8_t camera_cmd_data[20];
	
	while(1)
	{
		len = rt_device_read(device,0,&data,1);
		if( 0 != len)
		{
			camera_cmd_data[cur_pos++] = data;
		//	rt_kprintf("%x ",data);
			if(cur_pos == cmd_len)
			{
				break;
			}
		}
	}
	
	if(camera_check_cmd(camera_cmd_data,cmd_data,cmd_len))//is cmd 
	{
	//	rt_kprintf("is cmd ok\n");
		return  1;
	}
	return 0;
}
static void camera_data_transmit_timer(void *arg)
{	
	timer_value++;
}
void uint32_to_array(rt_uint8_t *array,rt_uint32_t data)
{
	*array = (data>>24) & 0x000000ff;	
	array++;
	*array = (data>>16) & 0x000000ff;
	array++;
	*array = (data>>8) & 0x000000ff;
	array++;
	*array = (data>>0) & 0x000000ff;
}
void camera_test_thread(void *arg)
{
	rt_device_t device = RT_NULL;
	rt_uint8_t	receive_data[CAMERA_READ_NUM];
	rt_uint16_t receive_len;
	int fp;
	rt_timer_t timer;

	timer = rt_timer_create("canera",camera_data_transmit_timer,RT_NULL,10,RT_TIMER_FLAG_PERIODIC);

	device = rt_device_find("usart2");
	if(device == RT_NULL)
	{
		rt_kprintf("is this cmd");
		return ;
	}
//	rt_device_write(device,0,reset_cmd,sizeof(reset_cmd));
//	rt_device_write(device,0,reset_cmd,sizeof(reset_cmd));
//	rt_thread_delay(100);
//	while(1)
	{
		receive_len = rt_device_read(device,0,receive_data,60);
		if(receive_len != 0)
		{
		}
	}
	camera_get_hex_printf(receive_data,receive_len);
	rt_device_write(device,0,update,sizeof(update));	//update
	rt_thread_delay(1);
	while(1)
	{
		static rt_uint32_t len_camera_read;
		static rt_uint32_t page;
		static rt_uint32_t surplus;
			
		receive_len = rt_device_read(device,0,receive_data,60);
		camera_get_hex_printf(receive_data,receive_len);

		rt_device_write(device,0,stop_cur_update,sizeof(stop_cur_update)); //stop
		rt_thread_delay(1);
		receive_len = rt_device_read(device,0,receive_data,60);
		camera_get_hex_printf(receive_data,receive_len);

		rt_device_write(device,0,get_pic_len,sizeof(get_pic_len)); //len
		rt_thread_delay(1);
		receive_len = rt_device_read(device,0,receive_data,60);
		camera_get_hex_printf(receive_data,receive_len);
	
		len_camera_read = (receive_data[5] << 24) |(receive_data[6] << 16) |(receive_data[7] << 8) |(receive_data[8]); 
		page = len_camera_read / CAMERA_READ_NUM;
		surplus = len_camera_read % CAMERA_READ_NUM;
		unlink("/1.jpg");
		fp = open("/1.jpg",O_CREAT |O_RDWR,0x777);
		rt_timer_start(timer);
		if(fp < 0)
		{
			rt_kprintf("file create fail\n");
		}
		while(1)
		{
			static rt_uint32_t addr_camera_read = 0;
			static rt_uint32_t num_camera_read = 0;

			uint32_to_array(&get_pic_data[6],addr_camera_read);//calculate read addr
			
			if(num_camera_read == 0)
			{
				num_camera_read = CAMERA_READ_NUM;
				uint32_to_array(&get_pic_data[10],num_camera_read);//calculate read num

			}
			rt_device_write(device,0,get_pic_data,sizeof(get_pic_data)); //write data cmd
			
			if(camera_receive_data_cmd(device,recv_data_cmd,sizeof(recv_data_cmd)))//decide cmd
			{
				rt_uint16_t cnt = CAMERA_READ_NUM;
				
				if(page == 0)
				{
					cnt = surplus;
				}
				while(cnt)
				{
					static rt_uint32_t write_num = 0;
					receive_len = rt_device_read(device,0,receive_data,cnt);
					if(receive_len != 0)
					{ 
						cnt -= receive_len;
						write(fp,receive_data,receive_len);
						write_num+=receive_len;
					}
				}
				if(page != 0)
				{
					addr_camera_read += CAMERA_READ_NUM;
				}
				else
				{
					addr_camera_read += surplus;
				}
				page--;		
				if(addr_camera_read >= len_camera_read)
				{
					close(fp);
					rt_kprintf("\nfile size = %d Byte\n",len_camera_read);
					rt_kprintf("read file size = %dByte\n",addr_camera_read);
					rt_kprintf("using time = %d 00s\n",timer_value);
					rt_kprintf("read ok\n");
					break;
				}
			}
			else
			{
				rt_kprintf("fial\n");
				break;
			}
			if(!camera_receive_data_cmd(device,recv_data_cmd,sizeof(recv_data_cmd)))
			{
				rt_kprintf("is error cmd\n");
				rt_thread_delay(500);
			}
		}
		break;
		rt_thread_delay(100);
	}
}
void camera_test(void)
{
	rt_thread_t id;

	id = rt_thread_create("camera",camera_test_thread,RT_NULL,1024*4,29,10);
	if(id != RT_NULL)
	{
		rt_thread_startup(id);
	}
}




















