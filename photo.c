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
 *2013 6 20 add ir deal thread
 * 
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/
#include "photo.h"
#include "gpio_pin.h"
#include "camera_uart.h"
#include "testprintf.h"
#include "mms.h"
#include "stdarg.h"
#include "sms.h"




const rt_uint8_t reset_camrea_cmd[] =	{0x56,0x00,0x26,0x00};
const rt_uint8_t switch_frame_cmd[] = {0x56,0x00,0x36,0x01,0x03};
const rt_uint8_t update_camrea_cmd[] = {0x56,0x00,0x36,0x01,0x02};
const rt_uint8_t stop_cur_frame_cmd[] =	{0x56,0x00,0x36,0x01,0x00};
const rt_uint8_t stop_next_frame_cmd[] = {0x56,0x00,0x36,0x01,0x01};
rt_uint8_t get_photo_len_cmd[] =	{0x56,0x00,0x34,0x01,0x00};
rt_uint8_t recv_data_respond_cmd[] = {0x76,0x00,0x32,0x00,0x00};
rt_uint8_t get_photo_fbuf_cmd[16] = {0x56,0x00,0x32,0x0C,0x00,0x0a,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00};




rt_sem_t		usart_data_sem = RT_NULL;		//photograph sem
rt_sem_t		start_work_sem = RT_NULL;		//start camrea mms gprs flow
rt_sem_t		photo_sem = RT_NULL;				//test use
rt_mq_t			photo_start_mq = RT_NULL;		//start work mq
//rt_mq_t			photo_ok_mq = RT_NULL;			//photo finish
rt_mutex_t	pic_file_mutex = RT_NULL;




volatile rt_uint32_t release_com2_data = 0;
rt_timer_t	pic_timer = RT_NULL;
rt_uint32_t	pic_timer_value = 0;

//test
volatile rt_uint32_t	buffer_len_test;
volatile rt_bool_t		run = 0;

void camera_infrared_thread_enter(void *arg);


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

/*******************************************************************************
* Function Name  : com_recv_data_analyze
* Description    : usart port recv data proceed analyze
*                  
* Input				: usart,	wait time,use  RAM size , arg num ,
*						  analyze arg
* Output			: -1pointer error 0 time, return >1of data is arg pos
*******************************************************************************/
rt_int8_t com_recv_data_analyze(rt_device_t usart,rt_uint32_t wait_t,rt_uint32_t mem_size,rt_uint32_t arg_num,...)
{	
	rt_uint8_t*      buffer = RT_NULL;
	rt_uint8_t*      tmp_buffer = RT_NULL;
	char*            analyze_result = RT_NULL;
	volatile rt_size_t        size = 0;
	va_list          ap;
	const char*      ap_str = RT_NULL;
	rt_uint8_t       i = 0;
	rt_bool_t        jmp_flag = 0;
	
	buffer = rt_malloc(sizeof(rt_uint8_t)*mem_size);
	
	rt_memset(buffer,0,sizeof(rt_uint8_t)*mem_size);
	tmp_buffer = buffer;
	if(RT_NULL == usart)
	{
#ifdef CMAERA_DEBUG_INFO_PRINTF
		rt_kprintf("photo.c \"com_recv_data_analyze\" fun usart device is RT_NULL\n ");
#endif
		return -1;
	}
	while(wait_t>0)
	{
		wait_t--;
		rt_thread_delay(1);
		
		size = rt_device_read(usart,0,tmp_buffer,1);
		if(1 == size)
		{
			if((tmp_buffer - buffer) < mem_size)
			{
				tmp_buffer++;
				va_start(ap,arg_num);
				for(i = 0; i < arg_num;i++)
				{
					ap_str = va_arg(ap,const char*);

					analyze_result = rt_strstr((const char*)buffer,(const char*)ap_str);
					if(analyze_result != RT_NULL)
					{
#ifdef CMAERA_DEBUG_INFO_PRINTF
						rt_kprintf("buffer = %s \n",buffer);
						rt_kprintf("ap_str = %s \n",ap_str);
						rt_kprintf("analyze ok\n");
#endif
						jmp_flag = i+1;
						break;
					}
				}	
				va_end(ap);
				if(jmp_flag)
				{
					rt_free(buffer);
					return jmp_flag;
				}
			}
			else
			{
				tmp_buffer = buffer;						//prevent Array Bounds Write
			}
		}
	}
	if(0 == wait_t)
	{
		rt_kprintf("wait time\n");
	}
	rt_free(buffer);
	
	return 0;
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
			release_com2_data = CM_BUFFER_LEN;
			break;
		}
		else if(RT_EOK == result)
		{
			length = rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);
			release_com2_data = CM_BUFFER_LEN;
			test_recv_data(camera->data,length);
		}
	}
#ifdef	CMAERA_DEBUG_INFO_PRINTF
	rt_kprintf("length = %d\n",length);
#endif

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
#ifdef	CMAERA_DEBUG_INFO_PRINTF
		rt_kprintf("max_cnt = %d\n",max_cnt);
#endif
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
#ifdef	CMAERA_DEBUG_INFO_PRINTF
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
#ifdef	CMAERA_DEBUG_INFO_PRINTF
		printf_write_to_file();
#endif
		camera->page--;
		if(camera->addr >= camera->size)
		{
			close(file_id);
#ifdef	CMAERA_DEBUG_INFO_PRINTF
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
	camera->infrared = rt_device_find(DEVICE_NAME_CAMERA_PHOTOSENSOR);
	if(RT_NULL == camera->infrared)
	{
		rt_kprintf("camera->infrared is RT_NULL");
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

void photo_change_pic_filename(cm_recv_mq_t recv_mq)
{
	rename(recv_mq->name1,"/pic1.jpg");
	rename(recv_mq->name2,"/pic2.jpg");
}
void photo_deal(camera_dev_t camera,cm_recv_mq_t recv_mq)
{
	//struct cm_send_mq send_mq;
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
/*	
	send_mq.error = camera->error;
	send_mq.name1 = recv_mq->name1;
	send_mq.name2 = recv_mq->name2;
	rt_mq_send(photo_ok_mq,&send_mq,sizeof(send_mq));//·¢ËÍÓÊÏä
*/
	mms_mail_buf.alarm_type = recv_mq->alarm_type;
	mms_mail_buf.time = recv_mq->date;
	/*  Receive timeout make no difference */
	if((CM_RUN_DEAL_OK == camera->error)||(CM_RECV_OUT_TIME & camera->error))
	{
//		photo_change_pic_filename(recv_mq);
		
		rt_mq_send(mms_mq, &mms_mail_buf, sizeof(MMS_MAIL_TYPEDEF));
	}
	else
	{
		send_sms_mail(ALARM_TYPE_CAMERA_FAULT,0);
	}
		
}


void photo_light_control(camera_dev_t camera,rt_uint8_t status)
{
	rt_uint8_t dat = 0;
	
	if(RT_NULL!= camera)
	{
		rt_device_read(camera->infrared,0,&dat,1);
		if(1 == status)
		{
			if(1 == dat)
			{
				rt_device_write(camera->glint_led,0,&status,1);
			}
		}
		else if(0 == status)
		{
			rt_device_write(camera->glint_led,0,&status,1);
		}
	}
}

void camera_module_self_test(camera_dev_t camera)
{
	rt_uint8_t i = 0;
	rt_uint8_t error_num = 0;
	const char result[6] = {0x76,0x00,0x36,0x00,0x00,'\0'};

	camera_power_control(camera,1);	
	/*
	if(com_recv_data_analyze(camera->device,200,300,1,"User-defined") != 1)
	{
		rt_kprintf("camrea fun problem\n");
		send_sms_mail(ALARM_TYPE_CAMERA_FAULT,0);
	}
	*/
	com_recv_data_analyze(camera->device,200,300,1,"@");
	for(i = 0; i < 3; i++)
	{
		rt_device_write(camera->device,0,update_camrea_cmd,sizeof(update_camrea_cmd));
		if(com_recv_data_analyze(camera->device,200,300,1,result) != 1)
		{
			error_num++;
			if(error_num > 2)
			{
				send_sms_mail(ALARM_TYPE_CAMERA_FAULT,0);
			}
		}
	}
	camera_power_control(camera,0); 
}


void photo_thread_entry(void *arg)
{
	struct camera_dev photo;
	struct cm_recv_mq recv_mq;
	rt_err_t	result;
	
	pic_timer = rt_timer_create("cm_time",pic_timer_test,RT_NULL,10,RT_TIMER_FLAG_PERIODIC);
	
	photo_struct_init(&photo);

	camera_power_control(&photo,1);	
	while(1)
	{
		result =  rt_mq_recv(photo_start_mq,&recv_mq,sizeof(recv_mq),24*36000);

		if(rt_sem_take(start_work_sem,1000) != RT_EOK)//start input work cycle
		{
			rt_kprintf("not photo \n\n\n");
			continue;
		}
		
		if(RT_EOK == result)								//in working order
		{
			camera_power_control(&photo,1); 	//open camera power
			
			rt_thread_delay(1);

			photo_reset(&photo);

			/*start camera job */
			rt_timer_start(pic_timer);
			
//			glint_light_control(&photo,1);
			photo_light_control(&photo,1);

			
			photo.time = recv_mq.time;
			rt_device_set_rx_indicate(photo.device,com2_photo_data_rx_indicate);
			photo.error = CM_RUN_DEAL_OK;

			
			
			com2_release_buffer(&photo);

			rt_kprintf("\n%d\n%s\n%s",recv_mq.time,recv_mq.name1,recv_mq.name2);

			rt_mutex_take(pic_file_mutex,RT_WAITING_FOREVER);
			
			photo_deal(&photo,&recv_mq);	

			rt_mutex_release(pic_file_mutex);
			
			camera_power_control(&photo,0); 	//close camera power

		}
		else if(-RT_ETIMEOUT == result)			//timeout checout module
		{
			camera_module_self_test(&photo);
			
			rt_kprintf("camera close power!!!\n");
		}
	}
}


void photo_thread_init(void)
{
	rt_thread_t	id;//threasd id

	id = rt_thread_create("cm_task",photo_thread_entry,RT_NULL,1024*4,103,30);
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
	id = rt_thread_create("cm_ir_c",camera_infrared_thread_enter,RT_NULL,256,108,100);
	if(RT_NULL == id )
	{
		rt_kprintf("graph thread create fail\n");
		
		return ;
	}
	rt_thread_startup(id);
		
	cm_ir_sem = rt_sem_create("cmirsem",0,RT_IPC_FLAG_FIFO);
	if(RT_NULL == cm_ir_sem)
	{
		rt_kprintf(" \"cmirsem\" sem create fail\n");

		return ;
	}
	pic_file_mutex = rt_mutex_create("pic_file",RT_IPC_FLAG_FIFO);
	if(RT_NULL == pic_file_mutex)
	{
		rt_kprintf(" \"pic_file\" sem create fail\n");

		return ;
	}
	start_work_sem = rt_sem_create("start_w",1,RT_IPC_FLAG_FIFO);
	if(RT_NULL == start_work_sem)
	{
		rt_kprintf(" \"start_w\" sem create fail\n");

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






/*		camera infared detection  part*/
#define CM_IR_TEST_TIME	55
#define CM_IR_ALARM_TOUCH_PERIOD	10		//10min
rt_sem_t cm_ir_sem = RT_NULL;
static rt_timer_t cm_ir_timer = RT_NULL;
static rt_timer_t cm_alarm_timer = RT_NULL;

volatile rt_uint8_t cm_ir_time_value = 0;
volatile rt_uint8_t cm_alarm_cnt_time_value = 0;

void camera_alarm_time_interval(void *arg)
{
	cm_alarm_cnt_time_value++;
	rt_kprintf("cm_alarm_cnt_time_value = %d\n",cm_alarm_cnt_time_value);
	if(cm_ir_time_value > CM_IR_ALARM_TOUCH_PERIOD)
	{
		rt_kprintf("start camare loopg!!!!!!!!!!!!!!!!!!!!!\n");
	}
}
void camera_infrared_timer(void *arg)
{
	cm_ir_time_value++;
}
void camera_infrared_thread_enter(void *arg)
{
	rt_device_t ir_dev;											//ir device
	rt_uint8_t	ir_pin_dat;
	rt_uint32_t flag = 0;										//ir pin status real-time monitoring
	rt_uint8_t	leave_flag = 0;				
	rt_err_t 		result = RT_NULL;

	cm_ir_timer = rt_timer_create("cm_ir_t",camera_infrared_timer,RT_NULL,100,\
	RT_TIMER_FLAG_PERIODIC);

	cm_alarm_timer = rt_timer_create("cm_ir_t",camera_alarm_time_interval,RT_NULL,6000,\
	RT_TIMER_FLAG_PERIODIC);

	ir_dev = rt_device_find(DEVICE_NAME_CAMERA_IRDASENSOR);
	
	while(1)
	{
		result = rt_sem_take(cm_ir_sem,200);//ir alarm touch off
		if(RT_EOK == result)
		{	
			if(2 == leave_flag) 	//touch off period not arrive
			{
				if(cm_alarm_cnt_time_value > CM_IR_ALARM_TOUCH_PERIOD)
				{
					leave_flag = 0;
					rt_timer_stop(cm_alarm_timer);
				}
				if(2 == leave_flag)
				{
					rt_kprintf("@@@not is photo time\n\n");
					continue;
				}
			}
			rt_timer_start(cm_ir_timer);
			if(ir_dev != RT_NULL)
			{
				cm_ir_time_value = 0;
				leave_flag = 0;
				flag = 0;
				while(1)
				{
					rt_device_read(ir_dev,0,&ir_pin_dat,1);
					rt_thread_delay(1);
					if(ir_pin_dat == 1)
					{
						if(flag>0)
						{
							flag--;
						} 	
					}
					else
					{
						if(flag < CM_IR_TEST_TIME)
						{
							flag++;
						}
						else
						{
							if(1 == leave_flag)
							{
								leave_flag = 2;//start	photo 
								cm_alarm_cnt_time_value = 0;
								rt_timer_start(cm_alarm_timer);//start alarm cnt timer
								camera_send_mail(ALARM_TYPE_CAMERA_IRDASENSOR,0);//send sem camera 
							}
							rt_timer_stop(cm_ir_timer);
							
							break;
						}
					}
					if(cm_ir_time_value >= 2)
					{
						rt_device_t rtc_dev;
						time_t			cur_data;
						
						cm_ir_time_value = 0;
						leave_flag = 1;
						rt_timer_stop(cm_ir_timer);
						
						rtc_dev = rt_device_find("rtc");
						
						rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &cur_data);
						//send sem alarm information
						send_alarm_mail(ALARM_TYPE_CAMERA_IRDASENSOR, ALARM_PROCESS_FLAG_SMS |\
						ALARM_PROCESS_FLAG_GPRS | ALARM_PROCESS_FLAG_LOCAL, ir_pin_dat, cur_data);
					}
				}
			}
		}
		else if(result == -RT_ETIMEOUT)
		{
			if(2 == leave_flag)
			{

			}
			else if(0 == leave_flag)
			{
				rt_device_read(ir_dev,0,&ir_pin_dat,1);
				if(ir_pin_dat == 1)
				{
					cm_ir_time_value = 2;
					rt_sem_release(cm_ir_sem);	
				}
			}
		}
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



void mq(rt_uint32_t time)//(rt_uint8_t time,rt_uint8_t *file_name)
{
	struct cm_recv_mq send_mq;

	send_mq.time = time;
	send_mq.date = 0;
	send_mq.name1 = "/1.jpg";
	send_mq.name2 = "/2.jpg";
	rt_mq_send(photo_start_mq,&send_mq,sizeof(send_mq));
}
FINSH_FUNCTION_EXPORT(mq, mq(time,name));
void s_cm_mq()
{
	camera_send_mail(ALARM_TYPE_CAMERA_IRDASENSOR,0);
}
FINSH_FUNCTION_EXPORT(s_cm_mq,);


#endif

