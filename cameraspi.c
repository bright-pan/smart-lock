/*********************************************************************
 * Filename:      picture.c
 *
 * Description:  Camera code Photo file 			
 *		Hardware resources : 	USART5 			
 											Baud:	115200 
 											Stop: 	1
 											DataBit:8
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
#include <dfs_init.h>
#include <dfs_elm.h>
#include <dfs_fs.h>
#include "dfs_posix.h"
#include "gpio_pin.h"
#include "camera_uart.h"
#include "cameraspi.h"


#define CAMERA_BUFFER_LEN							64
//rt_uint8_t data_buffer[CAMERA_BUFFER_LEN];
struct _camera_
{
	rt_device_t device;
	rt_device_t glint_led;
	rt_device_t power;
	rt_uint32_t	surplus;
	rt_uint32_t	addr;
	rt_uint32_t	page;
	rt_base_t		size;
	rt_uint8_t		time;
	rt_uint8_t		error;
	rt_uint8_t	data[CAMERA_BUFFER_LEN];
};
typedef struct _camera_*	camera_t;

struct _picture_mb_
{
	rt_uint8_t		time;
	const char*	name1;
	const char* name2;
};
typedef struct _picture_mb_* pic_mb_t;



const rt_uint8_t reset_camrea_cmd[] =	{0x56,0x00,0x26,0x00};
const rt_uint8_t switch_frame_cmd[] = {0x56,0x00,0x36,0x01,0x03};
const rt_uint8_t update_camrea_cmd[] = 	{0x56,0x00,0x36,0x01,0x02};
const rt_uint8_t stop_cur_frame_cmd[] =	{0x56,0x00,0x36,0x01,0x00};
const rt_uint8_t stop_next_frame_cmd[] = {0x56,0x00,0x36,0x01,0x01};
rt_uint8_t get_picture_len_cmd[] 	=	{0x56,0x00,0x34,0x01,0x00};
rt_uint8_t	recv_data_respond_cmd[] = {0x76,0x00,0x32,0x00,0x00};
rt_uint8_t get_picture_fbuf_cmd[16] = {0x56,0x00,0x32,0x0C,0x00,0x0f,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00};




rt_sem_t		com2_graph_sem = RT_NULL;//photograph sem
rt_sem_t		picture_sem = RT_NULL;
rt_mq_t			picture_mq = RT_NULL;


volatile rt_uint32_t release_com2_data = 0;
rt_timer_t		pic_timer = RT_NULL;
rt_uint32_t	pic_timer_value = 0;

//test
volatile rt_uint32_t	buffer_len_test;
volatile rt_bool_t		run = 0;




static rt_err_t com2_picture_data_rx_indicate(rt_device_t device,rt_size_t len)
{
	RT_ASSERT(com2_graph_sem != RT_NULL);

	buffer_len_test = len;
	if(len == release_com2_data)
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
		rt_kprintf("%c ",data[i]);
		i++;
	}
}

void test_run(void)
{
	
	while(run);run = 1;
}


void com2_release_buffer(camera_t camera)
{
	rt_uint8_t length;
	rt_err_t	result;
	
	release_com2_data = 1;
	while(1)
	{	
		result = rt_sem_take(com2_graph_sem,50);
		if(-RT_ETIMEOUT == result)
		{	
			length = rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
				
			test_recv_data(camera->data,length);
			
			rt_kprintf("length = %d\n",length);

			release_com2_data = CAMERA_BUFFER_LEN;
			break;
		}
	}
}

void glint_light_control(camera_t camera , rt_uint8_t status)
{
	rt_device_write(camera->glint_led,0,&status,1);
}

void camera_power_control(camera_t camera,rt_uint8_t status)
{
	rt_device_write(camera->power,0,&status,1);
}

void  picture_reset(camera_t camera)
{
	rt_device_write(camera->device,0,reset_camrea_cmd,sizeof(reset_camrea_cmd));	
	rt_thread_delay(10);
	com2_release_buffer(camera);
}


void picture_update_frame(camera_t camera)
{
	rt_device_write(camera->device,0,update_camrea_cmd,sizeof(update_camrea_cmd));	
	
	rt_thread_delay(1);
	
	rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
}
void picture_stop_cur_frame(camera_t camera)
{
	rt_device_write(camera->device,0,stop_cur_frame_cmd,sizeof(stop_cur_frame_cmd)); 
	
	rt_thread_delay(1);
	
	rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);

	/* close camera glint light */
	glint_light_control(camera,0);
}


void picture_stop_next_frame(camera_t camera)
{
	rt_device_write(camera->device,0,stop_next_frame_cmd,sizeof(stop_next_frame_cmd)); 
	
	rt_thread_delay(1);
	
	rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);

	/* close camera glint light */
	glint_light_control(camera,0);
}


void picture_switch_frame(camera_t camera)
{
	rt_device_write(camera->device,0,switch_frame_cmd,sizeof(switch_frame_cmd)); 

	glint_light_control(camera,0);
	
	rt_thread_delay(1);
	
	rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
}
void picture_get_size(camera_t camera,rt_uint8_t frame_flag)
{
	rt_size_t length;
	rt_uint8_t cnt = 0;
	
	if(frame_flag == 0)
	{
		get_picture_len_cmd[4] = 0;
	}
	else
	{
		get_picture_len_cmd[4] = 1;
	}
	do
	{
		rt_device_write(camera->device,0,get_picture_len_cmd,sizeof(get_picture_len_cmd)); 
		
		rt_thread_delay(10);
		
		length = rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
		
		/*calculate receive size		*/
		camera->size = (camera->data[5] << 24) |(camera->data[6] << 16) |(camera->data[7] << 8) |(camera->data[8]); 
		
		cnt++;
		if(cnt>10)
		{
			camera->error = 1;
			break;
		}
		rt_kprintf("cnt = %d\n",cnt);
	}
	while((camera->size >60000) ||(camera->size < 0));
	/*calculate receive size		*/
	camera->size = (camera->data[5] << 24) |(camera->data[6] << 16) |(camera->data[7] << 8) |(camera->data[8]); 
	camera->page = camera->size / CAMERA_BUFFER_LEN;
	camera->surplus = camera->size % CAMERA_BUFFER_LEN;
//	test_run();
	length = rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
}


rt_bool_t picture_data_deal(camera_t camera,int file_id)
{
	rt_device_t device;
	rt_uint32_t	len = CAMERA_BUFFER_LEN;
	rt_uint32_t   num;
	int i = 0;

	rt_kprintf("\ncamera->size = %d",camera->size);
	
	len = CAMERA_BUFFER_LEN;
	while(len--)
	{
		camera->data[len] = 0;
	}
	device  = rt_device_find("camera");

	{
		rt_device_read(device,0,camera->data,CAMERA_BUFFER_LEN);

		len = CAMERA_BUFFER_LEN;
		while(len--)
		{
			rt_kprintf("%x ",camera->data[CAMERA_BUFFER_LEN - len]);
		}

	}

	while(1);
	num = rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
	if(num == 0)
	{
		rt_thread_delay(1000);
	}
	while(num--)
	{
		rt_kprintf("%x ",camera->data[CAMERA_BUFFER_LEN -num]);
	}
	 rt_device_read(camera->device,0,camera->data,CAMERA_BUFFER_LEN);
}

void picture_set_per_read_size(camera_t camera,rt_uint8_t frame_type)
{
	rt_uint32_t length;
	
	camera->addr = 0;
	
	uint32_to_array(&get_picture_fbuf_cmd[10],CAMERA_BUFFER_LEN);
	
	uint32_to_array(&get_picture_fbuf_cmd[6],camera->addr);
	if(0 == frame_type)
	{
		get_picture_fbuf_cmd[4] = 0;
	}
	else
	{
		get_picture_fbuf_cmd[4] = 1;

	}
	rt_device_write(camera->device,0,get_picture_fbuf_cmd,sizeof(get_picture_fbuf_cmd));
	
	
}
void picture_create_file_two(camera_t camera,const char *pathname1,const char *pathname2)
{
	int file_id = 0;
	
	picture_update_frame(camera);
	
	picture_stop_cur_frame(camera);
	
	rt_thread_delay(camera->time);

	picture_switch_frame(camera);

	picture_stop_next_frame(camera);

	/*deal picture one */
	picture_get_size(camera,0);

	if(camera->error == 1)
	{
		return ;
	}
	
	picture_set_per_read_size(camera,0);

	unlink(pathname1);

	file_id = open(pathname1,O_CREAT | O_RDWR, 0);

	if(1 == picture_data_deal(camera,file_id))
	{
		rt_kprintf("\nphotograph fail\n");
	}


	/*deal picture two*/
	picture_get_size(camera,1);

	if(camera->error == 1)
	{
		return ;
	}
	
	picture_set_per_read_size(camera,1);
	
	unlink(pathname2);
	
	file_id = open(pathname2,O_CREAT | O_RDWR, 0);
	
	if(1 == picture_data_deal(camera,file_id))
	{
		rt_kprintf("\nphotograph fail\n");
	}
}


void picture_create_file_one(camera_t camera,const char *pathname )
{
	int file_id;

	camera->addr = 0;
	
	picture_update_frame(camera);
	
	picture_stop_cur_frame(camera);
	
	picture_get_size(camera,0);

	if(camera->error == 1)
	{
		return ;
	}
	
	uint32_to_array(&get_picture_fbuf_cmd[10],CAMERA_BUFFER_LEN);
	
	uint32_to_array(&get_picture_fbuf_cmd[6],camera->addr);
	
	rt_device_write(camera->device,0,get_picture_fbuf_cmd,sizeof(get_picture_fbuf_cmd));
	
	unlink(pathname);
	
	file_id = open(pathname,O_CREAT | O_RDWR, 0);
	if(1 == picture_data_deal(camera,file_id))
	{
		rt_kprintf("\nphotograph fail\n");
	}

}

void picture_struct_init(camera_t camera)
{	
	camera->device = rt_device_find(DEVICE_NAME_CAMERA_UART);
	if(RT_NULL == camera->device)
	{
		rt_kprintf("camera usart device error\n");

		
	}
	camera->glint_led = rt_device_find(DEVICE_NAME_CAMERA_LED);
	if(RT_NULL == camera->glint_led)
	{

	}
	camera->power = rt_device_find(DEVICE_NAME_CAMERA_POWER);
	if(RT_NULL == camera->power)
	{

	}
	camera->addr = 0;
	camera->page = 0;
	camera->size = 0;
	camera->surplus = 0;
	camera->time = 0;
	camera->error = 0;
//	camera->data = data_buffer;
}

void picture_thread_entry(void *arg)
{
	struct _camera_	 picture;
	struct _picture_mb_ recv_mq;
	rt_int32_t timeout = 500;
	rt_err_t	result;
	
	pic_timer = rt_timer_create("cm_time",pic_timer_test,RT_NULL,10,RT_TIMER_FLAG_PERIODIC);
	
	picture_struct_init(&picture);

	camera_power_control(&picture,1);	
	while(1)
	{
			result =  rt_mq_recv(picture_mq,&recv_mq,sizeof(recv_mq),timeout);
			rt_device_set_rx_indicate(picture.device,com2_picture_data_rx_indicate);
			picture_create_file_two(&picture,"/1.jpg","/1.jpg");
	}
}


void picture_thread_init(void)
{
	rt_thread_t	id;//threasd id

	
	id = rt_thread_create("cm_task",picture_thread_entry,RT_NULL,1024*6,20,30);
	if(RT_NULL == id )
	{
		rt_kprintf("graph thread create fail\n");
		return ;
	}
	rt_thread_startup(id);

	com2_graph_sem = rt_sem_create("cm_sem",0,RT_IPC_FLAG_FIFO);
	if(RT_NULL == com2_graph_sem)
	{
		rt_kprintf(" \"sxt\" sem create fail\n");
	}
	
	picture_mq = rt_mq_create("cm_mq",64,8,RT_IPC_FLAG_FIFO);
	if(RT_NULL == picture_mq)
	{
		rt_kprintf(" \"pic_mq\" mq create fial\n");
	}
}



#ifdef RT_USING_FINSH
#include <finsh.h>
void reset()
{
	rt_kprintf("%d",buffer_len_test);
	run = 0;
}

FINSH_FUNCTION_EXPORT(reset, reset());



void mq()//(rt_uint8_t time,rt_uint8_t *file_name)
{
	struct _picture_mb_ send_mq = {0,"/1.jpg","/2.jpg"};
	
	rt_mq_send(picture_mq,&send_mq,sizeof(send_mq));
}
FINSH_FUNCTION_EXPORT(mq, mq(time,name));



void rled(int data)
{
	rt_device_t device;

	device = rt_device_find("ledf");

	rt_device_write(device,0,&data,1);
	
}
FINSH_FUNCTION_EXPORT(rled, rled());




void chardevieo(const char *name,rt_int8_t data)
{
	rt_device_t device;

	device = rt_device_find(name);
	
	rt_device_write(device,0,&data,1);
}
FINSH_FUNCTION_EXPORT(chardevieo,chardevieo(name,data)--output char device);

#endif



