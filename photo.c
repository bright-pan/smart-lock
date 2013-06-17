/*********************************************************************
 * Filename:      photo.c
 *
 * Description:  Camera code photo file 			
 *Hardware resources : 	USART5 			
 *Baud:	115200 
 *Stop: 	1
 *DataBit:8
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
#include "photo.h"
#include "gpio_pin.h"
#include "camera_uart.h"
#include "testprintf.h"
#include "mms.h"




const rt_uint8_t reset_camrea_cmd[] =	{0x56,0x00,0x26,0x00};
const rt_uint8_t switch_frame_cmd[] = {0x56,0x00,0x36,0x01,0x03};
const rt_uint8_t update_camrea_cmd[] = {0x56,0x00,0x36,0x01,0x02};
const rt_uint8_t stop_cur_frame_cmd[] =	{0x56,0x00,0x36,0x01,0x00};
const rt_uint8_t stop_next_frame_cmd[] = {0x56,0x00,0x36,0x01,0x01};
rt_uint8_t get_photo_len_cmd[] =	{0x56,0x00,0x34,0x01,0x00};
rt_uint8_t recv_data_respond_cmd[] = {0x76,0x00,0x32,0x00,0x00};
rt_uint8_t get_photo_fbuf_cmd[16] = {0x56,0x00,0x32,0x0C,0x00,0x0a,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00};




rt_sem_t		usart_data_sem = RT_NULL;		//photograph sem
rt_sem_t		photo_sem = RT_NULL;				//test use
rt_mq_t			photo_start_mq = RT_NULL;		//start work mq
//rt_mq_t			photo_ok_mq = RT_NULL;			//photo finish



volatile rt_uint32_t release_com2_data = 0;
rt_timer_t	pic_timer = RT_NULL;
rt_uint32_t	pic_timer_value = 0;

//test
volatile rt_uint32_t	buffer_len_test;
volatile rt_bool_t		run = 0;

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



static rt_err_t com2_photo_data_rx_indicate(rt_device_t device,rt_size_t len)
{
	RT_ASSERT(usart_data_sem != RT_NULL);

	buffer_len_test = len;
	if(len == release_com2_data)
	{
		rt_sem_release(usart_data_sem);
	}
	
	return RT_EOK;
}


void com2_release_buffer(camera_dev_t camera)
{
	rt_uint8_t length;
	rt_err_t	result;
	
	release_com2_data = 1;
	while(1)
	{	
		result = rt_sem_take(usart_data_sem,30);
		if(-RT_ETIMEOUT == result)
		{	
			length = rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);
				
			test_recv_data(camera->data,length);
			
			rt_kprintf("length = %d\n",length);

			release_com2_data = CM_BUFFER_LEN;
			break;
		}
		else if(RT_EOK == result)
		{
			length = rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);
			release_com2_data = CM_BUFFER_LEN;
			test_recv_data(camera->data,length);
			
			rt_kprintf("length = .%d\n",length);
		}
	}
}


void pic_timer_test(void *arg)
{
	pic_timer_value++;
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


void glint_light_control(camera_dev_t camera , rt_uint8_t status)
{
	rt_device_write(camera->glint_led,0,&status,1);
}


void camera_power_control(camera_dev_t camera,rt_uint8_t status)
{
	rt_device_write(camera->power,0,&status,1);
}


void  photo_reset(camera_dev_t camera)
{
	rt_device_write(camera->device,0,reset_camrea_cmd,sizeof(reset_camrea_cmd));	
	rt_thread_delay(10);
	com2_release_buffer(camera);
}


void photo_update_frame(camera_dev_t camera)
{
	rt_device_write(camera->device,0,update_camrea_cmd,sizeof(update_camrea_cmd));	
	
	rt_thread_delay(1);
	
	rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);
}


void photo_stop_cur_frame(camera_dev_t camera)
{
	rt_device_write(camera->device,0,stop_cur_frame_cmd,sizeof(stop_cur_frame_cmd)); 
	
	rt_thread_delay(1);
	
	rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);

	/* close camera glint light */
	glint_light_control(camera,0);
}


void photo_stop_next_frame(camera_dev_t camera)
{
	rt_device_write(camera->device,0,stop_next_frame_cmd,sizeof(stop_next_frame_cmd)); 
	
	rt_thread_delay(1);
	
	rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);

	/* close camera glint light */
	glint_light_control(camera,0);
}


void photo_switch_frame(camera_dev_t camera)
{
	rt_device_write(camera->device,0,switch_frame_cmd,sizeof(switch_frame_cmd)); 

	glint_light_control(camera,0);
	
	rt_thread_delay(1);
	
	rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);
}


void photo_get_size(camera_dev_t camera,rt_uint8_t frame_flag)
{
	volatile rt_size_t length;
	rt_uint8_t max_cnt = 0;
	
	if(frame_flag == 0)
	{
		get_photo_len_cmd[4] = 0;
	}
	else
	{
		get_photo_len_cmd[4] = 1;
	}
	do
	{
		rt_device_write(camera->device,0,get_photo_len_cmd,sizeof(get_photo_len_cmd)); 
		
		rt_thread_delay(1);
		
		length = rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);
		
		/*    calculate receive size	*/
		camera->size = (camera->data[5] << 24) |(camera->data[6] << 16) |\
		(camera->data[7] << 8) |(camera->data[8]); 
		
		max_cnt++;
		if(max_cnt>10)
		{
			camera->error |= CM_GET_LEN_ERROR;
			break;
		}
		rt_kprintf("max_cnt = %d\n",max_cnt);
	}
	while((camera->size >60000) ||(camera->size < 0));
	/*   calculate receive size		*/
	camera->size = (camera->data[5] << 24) |(camera->data[6] << 16) \
	|(camera->data[7] << 8) |(camera->data[8]); 
	camera->page = camera->size / CM_BUFFER_LEN;
	camera->surplus = camera->size % CM_BUFFER_LEN;
	length = rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);
	printf_camera(camera);
}


void photo_data_deal(camera_dev_t camera,int file_id)
{
	volatile rt_err_t	sem_result = RT_EOK;
	volatile rt_size_t	length;
	rt_uint8_t	outtime_nun = 0;

	release_com2_data = CM_BUFFER_LEN+10;
	camera->addr = 0;
	while(outtime_nun < CM_OUTTIME_NUM)
	{
		/*		get sem photograph		*/
		sem_result = rt_sem_take(usart_data_sem,CM_SEM_BUF_WAIT_TIME);//500ms no recevie data Data sent failure		
		if((-RT_ETIMEOUT) == sem_result)
		{
#ifdef	USE_DEBUG_PRINTF
			rt_kprintf("\nsem result timer out: \n\n");
			printf_camera(camera);
#endif			
			outtime_nun++;//
			camera->error |= CM_RECV_OUT_TIME;
			rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);	
			
			rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);
			
			rt_device_write(camera->device,0,get_photo_fbuf_cmd,sizeof(get_photo_fbuf_cmd));
			
			continue;
		}
		rt_device_read(camera->device,0,recv_data_respond_cmd,5);
		if(camera->page != 0)
		{
			length = rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);
			
			rt_device_read(camera->device,0,recv_data_respond_cmd,5);
			
			camera->addr += CM_BUFFER_LEN;
			if(1 == camera->page)
			{
				release_com2_data = camera->surplus+10;
				uint32_to_array(&get_photo_fbuf_cmd[10],camera->surplus);
			}
			uint32_to_array(&get_photo_fbuf_cmd[6],camera->addr);

			rt_device_write(camera->device,0,get_photo_fbuf_cmd,sizeof(get_photo_fbuf_cmd));
		}
		else
		{
			length = rt_device_read(camera->device,0,camera->data,camera->surplus);
			
			rt_device_read(camera->device,0,recv_data_respond_cmd,5);
			camera->addr += camera->surplus;
		}
		write(file_id,camera->data,length);
#ifdef	USE_DEBUG_PRINTF
		printf_write_to_file();
#endif

		camera->page--;
		if(camera->addr >= camera->size)
		{
			close(file_id);
#ifdef	USE_DEBUG_PRINTF
			printf_camera(camera);
			rt_kprintf("\npic_timer_value = %d",pic_timer_value);
#endif
			pic_timer_value = 0;
			return ;
		}
	}
	camera->error |= CM_DEAL_DATA_ERROR;
}



void photo_set_per_read_size(camera_dev_t camera,rt_uint8_t frame_type)
{
	camera->addr = 0;
	uint32_to_array(&get_photo_fbuf_cmd[10],CM_BUFFER_LEN);
	
	uint32_to_array(&get_photo_fbuf_cmd[6],camera->addr);
	if(0 == frame_type)
	{
		get_photo_fbuf_cmd[4] = 0;
	}
	else
	{
		get_photo_fbuf_cmd[4] = 1;

	}
	rt_device_write(camera->device,0,get_photo_fbuf_cmd,sizeof(get_photo_fbuf_cmd));
}


void photo_create_file_two(camera_dev_t camera,const char *pathname1,const char *pathname2)
{
	int file_id = 0;
	
	photo_update_frame(camera);
	
	photo_stop_cur_frame(camera);
	
	rt_thread_delay(camera->time);

	photo_switch_frame(camera);

	photo_stop_next_frame(camera);

	/*deal photo one */
	photo_get_size(camera,0);

	if(CM_GET_LEN_ERROR == camera->error)
	{
		return ;
	}
	
	photo_set_per_read_size(camera,0);

	unlink(pathname1);

	file_id = open(pathname1,O_CREAT | O_RDWR, 0);

	photo_data_deal(camera,file_id);

	if(CM_DEAL_DATA_ERROR == camera->error)
	{
		rt_kprintf("\nphotograph fail\n");
	}


	/*deal photo two*/
	photo_get_size(camera,1);

	if(CM_GET_LEN_ERROR == camera->error)
	{
		return ;
	}
	
	photo_set_per_read_size(camera,1);
	
	unlink(pathname2);
	
	file_id = open(pathname2,O_CREAT | O_RDWR, 0);
	
	photo_data_deal(camera,file_id);

	if(CM_DEAL_DATA_ERROR == camera->error)
	{
		rt_kprintf("\nphotograph fail\n");
	}
}


void photo_create_file_one(camera_dev_t camera,const char *pathname )
{
	int file_id;

	camera->addr = 0;
	
	photo_update_frame(camera);
	
	photo_stop_cur_frame(camera);
	
	photo_get_size(camera,0);

	if(CM_GET_LEN_ERROR == camera->error)
	{
		return ;
	}
	
	uint32_to_array(&get_photo_fbuf_cmd[10],CM_BUFFER_LEN);
	
	uint32_to_array(&get_photo_fbuf_cmd[6],camera->addr);
	
	rt_device_write(camera->device,0,get_photo_fbuf_cmd,sizeof(get_photo_fbuf_cmd));
	
	unlink(pathname);
	
	file_id = open(pathname,O_CREAT | O_RDWR, 0);
	
	photo_data_deal(camera,file_id);

	if(CM_DEAL_DATA_ERROR == camera->error)
	{
		rt_kprintf("\nphotograph fail\n");
	}

}


void photo_struct_init(camera_dev_t camera)
{	
	camera->device = rt_device_find(DEVICE_NAME_CAMERA_UART);
	if(RT_NULL == camera->device)
	{
		rt_kprintf("camera->device is RT_NULL");
		camera->error = CM_DEV_INIT_ERROR;
		return ;
	}
	camera->glint_led = rt_device_find(DEVICE_NAME_CAMERA_LED);
	if(RT_NULL == camera->glint_led)
	{
		rt_kprintf("camera->glint_led is RT_NULL");
		camera->error = CM_DEV_INIT_ERROR;
		return ;
	}
	camera->power = rt_device_find(DEVICE_NAME_CAMERA_POWER);
	if(RT_NULL == camera->power)
	{
		rt_kprintf("camera->power is RT_NULL");
		camera->error = CM_DEV_INIT_ERROR;
		return ;
	}
	camera->addr = 0;
	camera->page = 0;
	camera->size = 0;
	camera->surplus = 0;
	camera->time = 0;
	camera->error = 0;
//	camera->data = data_buffer;
}


void photo_deal(camera_dev_t camera,cm_recv_mq_t recv_mq)
{
	struct cm_send_mq send_mq;
	MMS_MAIL_TYPEDEF mms_mail_buf;

	if('\0' == *(recv_mq->name2))
	{
		photo_create_file_one(camera,recv_mq->name1);	
	}
	else if(camera->time < CM_CONTINUOUS_PHOTO_TIME)
	{
		photo_create_file_two(camera,recv_mq->name1,recv_mq->name2);	
	}
	else
	{
		photo_create_file_one(camera,recv_mq->name1);	
		rt_thread_delay(camera->time);
		photo_create_file_one(camera,recv_mq->name2);	
	}
	/* camera woker finish send Message Queuing */
/*	send_mq.error = camera->error;
	send_mq.name1 = recv_mq->name1;
	send_mq.name2 = recv_mq->name2;
*/
//	rt_mq_send(photo_ok_mq,&send_mq,sizeof(send_mq));//·¢ËÍÓÊÏä
	mms_mail_buf.alarm_type = recv_mq->alarm_type;
	mms_mail_buf.time = recv_mq->date;
	if((CM_RUN_DEAL_OK == camera->error)||(CM_RECV_OUT_TIME | camera->error))//Receive timeout make no difference
	{
		rt_mq_send(mms_mq, &mms_mail_buf, sizeof(MMS_MAIL_TYPEDEF));
	}
		
}
void photo_thread_entry(void *arg)
{
	struct camera_dev photo;
	struct cm_recv_mq recv_mq;
//	rt_int32_t timeout = 1000;
	rt_err_t	result;
	
	pic_timer = rt_timer_create("cm_time",pic_timer_test,RT_NULL,10,RT_TIMER_FLAG_PERIODIC);
	
	photo_struct_init(&photo);

	camera_power_control(&photo,1);	
	while(1)
	{
		result =  rt_mq_recv(photo_start_mq,&recv_mq,sizeof(recv_mq),1000);

		if(RT_EOK == result)								//in working order
		{
			camera_power_control(&photo,1); 	//open camera power
			
			rt_thread_delay(1);

			photo_reset(&photo);

			/*start camera job */
			rt_timer_start(pic_timer);
			
			glint_light_control(&photo,1);
			
			photo.time = recv_mq.time;
			rt_device_set_rx_indicate(photo.device,com2_photo_data_rx_indicate);
			photo.error = CM_RUN_DEAL_OK;

			
			
			com2_release_buffer(&photo);

			rt_kprintf("\n%d\n%s\n%s",recv_mq.time,recv_mq.name1,recv_mq.name2);
			
			photo_deal(&photo,&recv_mq);	

			//camera_power_control(&photo,0); 	//close camera power

		}
		else if(-RT_ETIMEOUT == result)			//timeout checout module
		{
			//camera_power_control(&photo,0); 
//			timeout = RT_WAITING_FOREVER;
			rt_kprintf("close power\n");
			continue;
		}
	}
}


void photo_thread_init(void)
{
	rt_thread_t	id;//threasd id

	id = rt_thread_create("cm_task",photo_thread_entry,RT_NULL,1024*5,100,30);
	if(RT_NULL == id )
	{
		rt_kprintf("graph thread create fail\n");
		
		return ;
	}
	rt_thread_startup(id);

	usart_data_sem = rt_sem_create("cm_sem",0,RT_IPC_FLAG_FIFO);
	if(RT_NULL == usart_data_sem)
	{
		rt_kprintf(" \"sxt\" sem create fail\n");

		return ;
	}
	
	photo_start_mq = rt_mq_create("cmsend",64,8,RT_IPC_FLAG_FIFO);
	if(RT_NULL == photo_start_mq)
	{
		rt_kprintf(" \"cmsend\" mq create fial\n");

		return ;
	}
/*
	photo_ok_mq = rt_mq_create("cmok",64,8,RT_IPC_FLAG_FIFO);
	if(RT_NULL == photo_ok_mq)
	{
		rt_kprintf(" \"cmok\" mq create fial\n");

		return ;
	}
*/
}


void camera_send_mail(ALARM_TYPEDEF alarm_type, time_t time)
{
	struct cm_recv_mq send_mq;

	send_mq.time = 0;
	send_mq.name1 = "/1.jpg";
	send_mq.name2 = "/2.jpg";
	send_mq.date = time;
	send_mq.alarm_type = alarm_type;

	rt_mq_send(photo_start_mq,&send_mq,sizeof(send_mq));
}


















#ifdef RT_USING_FINSH
#include <finsh.h>
void reset()
{
	rt_kprintf("%d",buffer_len_test);
	run = 0;
}

FINSH_FUNCTION_EXPORT(reset, reset());



void mq(rt_uint32_t time)//(rt_uint8_t time,rt_uint8_t *file_name)
{
	struct cm_recv_mq send_mq = {0,0,0,"/1.jpg","/2.jpg"};

	send_mq.time = time;
	rt_mq_send(photo_start_mq,&send_mq,sizeof(send_mq));
}
FINSH_FUNCTION_EXPORT(mq, mq(time,name));
void s_cm_mq()
{
	camera_send_mail(ALARM_TYPE_CAMERA_IRDASENSOR,0);
}
FINSH_FUNCTION_EXPORT(s_cm_mq,);

#endif

