/*********************************************************************
 * Filename:    photograph.c
 *
 * Description:    usart photograph code for the module 
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

#include "photograph.h"
#include <dfs_init.h>
#include <dfs_elm.h>
#include <dfs_fs.h>
#include "dfs_posix.h"



#define CAMERA_BUFFER_LEN							1024


rt_sem_t		com2_graph_sem = RT_NULL;//photograph sem
rt_sem_t		pic_sem = RT_NULL;
rt_timer_t		pic_timer;
rt_uint32_t	pic_timer_value = 0;
rt_uint8_t		frame_pos = 0;						//cur frame pos

rt_uint8_t 		picture_recv_data[CAMERA_BUFFER_LEN];		//
rt_uint16_t	picture_recv_len = 0;
rt_uint32_t	picture_recv_data_addr = 0;
rt_uint32_t	picture_recv_data_size = 0;
rt_uint32_t	picture_data_page = 0;
rt_uint32_t	picture_data_surplus = 0;

//test 
rt_uint32_t	cur_len;


const rt_uint8_t reset_camrea_cmd[] =	{0x56,0x00,0x26,0x00};
const rt_uint8_t switch_frame_cmd[] = {0x56,0x00,0x36,0x01,0x03};
const rt_uint8_t update_camrea_cmd[] = 	{0x56,0x00,0x36,0x01,0x02};
const rt_uint8_t stop_cur_frame_cmd[] =	{0x56,0x00,0x36,0x01,0x00};
const rt_uint8_t stop_next_frame_cmd[] = {0x56,0x00,0x36,0x01,0x01};
rt_uint8_t get_picture_len_cmd[] 	=	{0x56,0x00,0x34,0x01,0x00};
rt_uint8_t	recv_data_respond_cmd[] = {0x76,0x00,0x32,0x00,0x00};
rt_uint8_t get_picture_fbuf_cmd[16] = 		{0x56,0x00,0x32,0x0C,0x00,0x0a,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00};


static rt_err_t com2_picture_data_rx_indicate(rt_device_t device,rt_size_t len)
{
	RT_ASSERT(com2_graph_sem != RT_NULL);

	cur_len = len;
	if( (len == CAMERA_BUFFER_LEN+10))
	{
		rt_sem_release(com2_graph_sem);
	}
	else if((picture_data_page == 0)&&(len == picture_data_surplus+10))
	{
		rt_sem_release(com2_graph_sem);
	}
	
	return RT_EOK;
}


static void uint32_to_array(rt_uint8_t *array,rt_uint32_t data)
{
	*array = (data>>24) & 0x000000ff;	
	array++;
	*array = (data>>16) & 0x000000ff;
	array++;
	*array = (data>>8) & 0x000000ff;
	array++;
	*array = (data>>0) & 0x000000ff;
}


void pic_timer_test(void *arg)
{
	pic_timer_value++;
}
void test_recv_data(rt_uint8_t data[],rt_uint32_t len)
{	
	 rt_uint32_t i = 0;
	
	while(i != len)
	{
		rt_kprintf("%x ",data[i]);
		i++;
	}
}

void photograph_update_frame(rt_device_t device)
{
 	volatile rt_size_t picture_recv_len = 0;
	
	rt_device_write(device,0,update_camrea_cmd,sizeof(update_camrea_cmd));	
	rt_thread_delay(10);
	picture_recv_len = rt_device_read(device,0,picture_recv_data,CAMERA_BUFFER_LEN);
}
void photograph_stop_cur_frame(rt_device_t device)
{
	rt_uint8_t picture_recv_len = 0;
	
	rt_device_write(device,0,stop_cur_frame_cmd,sizeof(stop_cur_frame_cmd)); 
	rt_thread_delay(10);
	picture_recv_len = rt_device_read(device,0,picture_recv_data,CAMERA_BUFFER_LEN);
}
void photograph_stop_next_frame(rt_device_t device)
{
	rt_uint8_t picture_recv_len = 0;

	rt_device_write(device,0,stop_next_frame_cmd,sizeof(stop_next_frame_cmd)); 
	rt_thread_delay(10);
	picture_recv_len = rt_device_read(device,0,picture_recv_data,CAMERA_BUFFER_LEN);
}
void photograph_switch_frame(rt_device_t device)
{
	rt_uint8_t picture_recv_len = 0;

	rt_device_write(device,0,switch_frame_cmd,sizeof(switch_frame_cmd)); 
	rt_thread_delay(10);
	picture_recv_len = rt_device_read(device,0,picture_recv_data,CAMERA_BUFFER_LEN);
}
rt_size_t photograph_get_picture_size(rt_device_t device,rt_uint8_t frame_flag)
{
	rt_size_t size;
	
	if(frame_flag == 0)
	{
		get_picture_len_cmd[4] = 0;
	}
	else
	{
		get_picture_len_cmd[4] = 1;
	}
	rt_device_write(device,0,get_picture_len_cmd,sizeof(get_picture_len_cmd)); 
	rt_thread_delay(10);
	picture_recv_len = rt_device_read(device,0,picture_recv_data,CAMERA_BUFFER_LEN);

	if(picture_recv_len == 9)
	{
		rt_kprintf("\npicture_recv_len = %d\n",picture_recv_len);
		test_recv_data(picture_recv_data,9);
	}
	else
	{
		rt_thread_delay(100);
		rt_kprintf("\npicture_recv_len = %d\n",picture_recv_len);
	}
	/*calculate receive size		*/
	size = (picture_recv_data[5] << 24) |(picture_recv_data[6] << 16) |(picture_recv_data[7] << 8) |(picture_recv_data[8]); 

	return size;
}


void photograph_continuous(rt_device_t device)
{
	int file_id;
	rt_kprintf("\n");rt_kprintf("\n");rt_kprintf("\n");rt_kprintf("\n");
	
	photograph_update_frame(device);
	
	photograph_stop_cur_frame(device);
	
	photograph_switch_frame(device);

	photograph_stop_next_frame(device);

	picture_recv_data_addr = 0;
	picture_recv_data_size = photograph_get_picture_size(device,0);
	picture_data_page = picture_recv_data_size / CAMERA_BUFFER_LEN;
	picture_data_surplus = picture_recv_data_size % CAMERA_BUFFER_LEN;

	rt_kprintf("\npicture_recv_data_size = %d\n",picture_recv_data_size);
	uint32_to_array(&get_picture_fbuf_cmd[10],CAMERA_BUFFER_LEN);
	uint32_to_array(&get_picture_fbuf_cmd[6],picture_recv_data_addr);
	get_picture_fbuf_cmd[4] = 0;
	rt_device_write(device,0,get_picture_fbuf_cmd,sizeof(get_picture_fbuf_cmd));

	file_id = open("/3.jpg",O_CREAT | O_RDWR,0);
	photograph_data_deal(device,file_id);
	picture_recv_data_addr = 0;

	

	picture_recv_data_size = photograph_get_picture_size(device,1);
	picture_data_page = picture_recv_data_size / CAMERA_BUFFER_LEN;
	picture_data_surplus = picture_recv_data_size % CAMERA_BUFFER_LEN;

	rt_kprintf("\npicture_recv_data_size = %d\n",picture_recv_data_size);
	uint32_to_array(&get_picture_fbuf_cmd[10],CAMERA_BUFFER_LEN);
	uint32_to_array(&get_picture_fbuf_cmd[6],picture_recv_data_addr);
	get_picture_fbuf_cmd[4] = 1;
	rt_device_write(device,0,get_picture_fbuf_cmd,sizeof(get_picture_fbuf_cmd));

	file_id = open("/4.jpg",O_CREAT | O_RDWR,0);
	photograph_data_deal(device,file_id);
	
}

void photograph_frame_switch(rt_device_t device)
{
	frame_pos = !frame_pos;
}
void photograph_cmd_fun(rt_device_t device)
{
	photograph_update_frame(device);
	
	photograph_stop_cur_frame(device);
	
	/*Get Picture length	*/
	picture_recv_data_size = photograph_get_picture_size(device,0);
	picture_data_page = picture_recv_data_size / CAMERA_BUFFER_LEN;
	picture_data_surplus = picture_recv_data_size % CAMERA_BUFFER_LEN;

	rt_kprintf("\npicture_recv_data_size = %d\n",picture_recv_data_size);
	uint32_to_array(&get_picture_fbuf_cmd[10],CAMERA_BUFFER_LEN);
	uint32_to_array(&get_picture_fbuf_cmd[6],picture_recv_data_addr);
	rt_device_write(device,0,get_picture_fbuf_cmd,sizeof(get_picture_fbuf_cmd));
}


rt_bool_t photograph_data_deal(rt_device_t device,int file_id)
{
	rt_err_t	sem_result;
	
	while(1)
	{ 
		static rt_uint32_t num = 0;

		/*get sem photograph*/
		sem_result = rt_sem_take(com2_graph_sem,50);
		if(sem_result == -RT_ETIMEOUT)
		{
			rt_kprintf("\nsem result error");
			rt_device_read(device,0,picture_recv_data,CAMERA_BUFFER_LEN+10);
			rt_device_write(device,0,get_picture_fbuf_cmd,sizeof(get_picture_fbuf_cmd));
			continue;
		}
		
		rt_device_read(device,0,recv_data_respond_cmd,5);
		if(picture_data_page != 0)
		{
			
			picture_recv_len = rt_device_read(device,0,picture_recv_data,CAMERA_BUFFER_LEN);
			rt_device_read(device,0,recv_data_respond_cmd,5);
			picture_recv_data_addr += CAMERA_BUFFER_LEN;
			if(picture_data_page == 1)
			{
				uint32_to_array(&get_picture_fbuf_cmd[10],picture_data_surplus);
			}
			uint32_to_array(&get_picture_fbuf_cmd[6],picture_recv_data_addr);
			rt_device_write(device,0,get_picture_fbuf_cmd,sizeof(get_picture_fbuf_cmd));
		}
		else
		{
			picture_recv_len = rt_device_read(device,0,picture_recv_data,picture_data_surplus);
			rt_device_read(device,0,recv_data_respond_cmd,5);
			picture_recv_data_addr += picture_data_surplus;

		}
		{
			static	rt_uint16_t i = 0;
			write(file_id,picture_recv_data,picture_recv_len); 
			rt_kprintf("file write %d\n",i++);
		}

		picture_data_page--;
		if(picture_recv_data_addr >= picture_recv_data_size)
		{
			close(file_id);
			
			rt_kprintf("\npicture_data_page = %d",picture_data_page);
			rt_kprintf("\npicture_recv_data_addr = %d",picture_recv_data_addr);
			rt_kprintf("\npicture_recv_data_size = %d",picture_recv_data_size);
			rt_kprintf("\n num = %d",num);		
			rt_kprintf("\npic_timer_value = %d",pic_timer_value);
			pic_timer_value = 0;
			return 0;
		}
	}
	return 1;
}

void photograph_picture_file(rt_device_t device,const char *pathname )
{
	int file_id;

	picture_recv_data_addr = 0;
	photograph_cmd_fun(device);
	
	unlink(pathname);
	
	file_id = open(pathname,O_CREAT | O_RDWR, 0);
	
	if(1 == photograph_data_deal(device,file_id))
	{
		rt_kprintf("\nphotograph fail\n");
	}

}
void photograph_clear_com2_buffer(rt_device_t device)
{

}
void photograph_reset(rt_device_t device)
{
	rt_uint32_t i;
	rt_thread_delay(100);
	//rt_device_write(device,0,reset_camrea_cmd,4);
	rt_thread_delay(100);
	while(cur_len)
	{
		rt_uint8_t data;

		if(rt_device_read(device,0,&data,1))
		{
			rt_kprintf("%c",data);
			cur_len--;
		}
	}
}
void photograph_thread_entry(void *arg)
{
	rt_device_t device;
	rt_uint16_t i;
	pic_timer = rt_timer_create("pic",pic_timer_test,RT_NULL,10,RT_TIMER_FLAG_PERIODIC);
	rt_timer_start(pic_timer);
	
	device = rt_device_find("usart2");

	if(device == RT_NULL)
	{
		rt_kprintf("device error\n");
	}
	rt_device_set_rx_indicate(device,com2_picture_data_rx_indicate);
	
	photograph_reset(device);

	while(1)
	{
		rt_sem_take(pic_sem,RT_WAITING_FOREVER);
		photograph_picture_file(device,"/1.jpg");
		photograph_picture_file(device,"/2.jpg");
		photograph_continuous(device);
	}

}



void photograph_thread_init(void)
{
	rt_thread_t	id;//threasd id
//	rt_err_t	sem_result;

	
	id = rt_thread_create("graph",photograph_thread_entry,RT_NULL,1024*5,20,30);
	if(id == RT_NULL)
	{
		rt_kprintf("graph thread create fail\n");
		return ;
	}
	rt_thread_startup(id);

	com2_graph_sem = rt_sem_create("sxt",0,RT_IPC_FLAG_FIFO);
	if(com2_graph_sem == RT_NULL)
	{
		
	}
	pic_sem	= rt_sem_create("zx",0,RT_IPC_FLAG_FIFO);
	if(pic_sem == RT_NULL)
	{
		
	}
}




#ifdef RT_USING_FINSH
#include <finsh.h>
void picture()
{
	rt_sem_release(pic_sem);
}

FINSH_FUNCTION_EXPORT(picture, picture());

void reset()
{
	rt_kprintf("%d",cur_len);
}

FINSH_FUNCTION_EXPORT(reset, reset())



#endif










