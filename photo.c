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
#include "gprs.h"

#define IR_ACTIVE_STATUS								0			//infrared testing
#define POWR_ACTIVE_STATUS							1			//camera power 
#define LED_ACTIVE_STATUS								1			//Fill Light
#define LIGHT_ACTIVE_STATUS							1			//light
#define PHOTO_INTERVAL_TIME							200 //2	//2s
#define PIC_DATA_MAX_SIZE								70000	//70K
#define PIC_DATA_MIN_SIZE								10000	//10K

extern void photo_light_control(camera_dev_t camera,rt_uint8_t status);

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
rt_mutex_t	pic_file_mutex = RT_NULL;		//picture file operate mutex
rt_event_t	alarm_inform_event = RT_NULL;//
rt_event_t	work_flow_ok = RT_NULL;			//one work flow



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
rt_int8_t com_recv_data_analyze(rt_device_t usart,
																rt_uint32_t wait_t,
																rt_uint32_t mem_size,
																rt_uint32_t arg_num,...)
{	
	rt_uint8_t*      		buffer = RT_NULL;
	rt_uint8_t*      		tmp_buffer = RT_NULL;
	char*            		analyze_result = RT_NULL;
	volatile rt_size_t  size = 0;
	va_list          		ap;
	const char*      		ap_str = RT_NULL;
	rt_uint8_t       		i = 0;
	rt_bool_t        		jmp_flag = 0;
	
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
	rt_device_t dev;

	dev = rt_device_find(DEVICE_NAME_CAMERA_USART_TX);
	if(dev == RT_NULL)
	{
		rt_kprintf("camera tx pin device is rt_null");
		return ;
	}
	if(status == POWR_ACTIVE_STATUS)//Open camera 
	{
		/* set Tx pin input */
		rt_device_control(dev,GPIO_CMD_INIT_CONFIG,(void*)GPIO_Mode_AF_PP);
		rt_kprintf("open camrea modlue \n");
	}
	else
	{
		rt_device_control(dev,GPIO_CMD_INIT_CONFIG,(void*)GPIO_Mode_IN_FLOATING);
		rt_kprintf("close camrea modlue \n");
	}
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
}


void photo_stop_next_frame(camera_dev_t camera)
{
	rt_device_write(camera->device,0,stop_next_frame_cmd,sizeof(stop_next_frame_cmd)); 
	
	rt_thread_delay(1);
	
	rt_device_read(camera->device,0,camera->data,CM_BUFFER_LEN);
}


void photo_switch_frame(camera_dev_t camera)
{
	rt_device_write(camera->device,0,switch_frame_cmd,sizeof(switch_frame_cmd)); 
	
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
		camera->size = ((*(camera->data+5)) << 24) |((*(camera->data+6))  << 16) |\
		((*(camera->data+7))  << 8) |((*(camera->data+8)) ); 
		
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
	while((camera->size >PIC_DATA_MAX_SIZE) ||(camera->size < PIC_DATA_MIN_SIZE));
	/*   calculate receive size		*/
	//camera->size = (camera->data[5] << 24) |(camera->data[6] << 16) \
	//|(camera->data[7] << 8) |(camera->data[8]); 
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

	photo_light_control(camera,LED_ACTIVE_STATUS);//open fill-in light
	
	photo_update_frame(camera);
	
	photo_stop_cur_frame(camera);
	
	rt_thread_delay(camera->time);

	photo_switch_frame(camera);

	photo_stop_next_frame(camera);

	photo_light_control(camera,!LED_ACTIVE_STATUS);//close fill-in light

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

	photo_light_control(camera,LED_ACTIVE_STATUS);//open fill-in light
	camera->addr = 0;
	photo_update_frame(camera);
	
	photo_stop_cur_frame(camera);

	photo_light_control(camera,!LED_ACTIVE_STATUS);//close fill-in light
	
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
	//MMS_MAIL_TYPEDEF mms_mail_buf;

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
	//mms_mail_buf.alarm_type = recv_mq->alarm_type;
	//mms_mail_buf.time = recv_mq->date;
	/*  Receive timeout make no difference */
	if((CM_RUN_DEAL_OK == camera->error)||(CM_RECV_OUT_TIME & camera->error))
	{
		extern void send_mms_mail(ALARM_TYPEDEF alarm_type, time_t time, char *pic_name);
		//rt_mq_send(mms_mq, &mms_mail_buf, sizeof(MMS_MAIL_TYPEDEF));
		send_mms_mail(recv_mq->alarm_type,0,(char*)recv_mq->name2);//send mms
	}
	else
	{
		send_sms_mail(ALARM_TYPE_CAMERA_FAULT,0);
		send_gprs_mail(ALARM_TYPE_CAMERA_FAULT, 0, 0,RT_NULL);
	}
		
}


void photo_light_control(camera_dev_t camera,rt_uint8_t status)
{
	rt_uint8_t dat = 0;
	
	if(RT_NULL!= camera)
	{
		rt_device_read(camera->infrared,0,&dat,1);
		if(LED_ACTIVE_STATUS== status)
		{
			if(LIGHT_ACTIVE_STATUS == dat)
			{
				rt_device_write(camera->glint_led,0,&status,1);
			}
		}
		else if(!LED_ACTIVE_STATUS == status)
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

	camera_power_control(camera,POWR_ACTIVE_STATUS);	
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
	camera_power_control(camera,!POWR_ACTIVE_STATUS); 
}

rt_err_t work_flow_status(void)
{
	rt_err_t result = RT_EOK;
	rt_uint32_t	event;

	result = rt_event_recv(work_flow_ok,
												 FLOW_OK_FLAG,
												 RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR,
												 RT_WAITING_NO,
												 &event);
	if(RT_EOK == result)
	{
		return result;
	}
	return result;
}
void photo_thread_entry(void *arg)
{
	struct camera_dev photo;
	struct cm_recv_mq recv_mq;
	rt_err_t	result;
	
	pic_timer = rt_timer_create("cm_time",
															pic_timer_test,RT_NULL,
															10,
															RT_TIMER_FLAG_PERIODIC);
	photo_struct_init(&photo);

	camera_power_control(&photo,!POWR_ACTIVE_STATUS);	
	while(1)
	{
		result =  rt_mq_recv(photo_start_mq,&recv_mq,sizeof(recv_mq),24*360000);//3600sx24h is a day

		/*if(work_flow_status() == -RT_ETIMEOUT)
		{
#ifdef	CMAERA_DEBUG_INFO_PRINTF
			rt_kprintf("one work flow not finsh\n\n");
#endif
			continue;
		}
		*/
		if(RT_EOK == result)								//in working order
		{
			camera_power_control(&photo,POWR_ACTIVE_STATUS); 	//open camera power
			
			rt_thread_delay(1);

			photo.data = (rt_uint8_t*)rt_malloc(CM_BUFFER_LEN);
			rt_memset(photo.data,0,CM_BUFFER_LEN);
			photo_reset(&photo);

			/*start camera job */
			rt_timer_start(pic_timer);
			
			//glint_light_control(&photo,1);
			//photo_light_control(&photo,LED_ACTIVE_STATUS);

			
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
		rt_thread_delay(50);								//wait picture send finish
		
		rt_free(photo.data);
	}
}


void photo_thread_init(void)
{
	rt_thread_t	id = RT_NULL;

	/*camera com port recv data use sem*/
	usart_data_sem = rt_sem_create("cmcom",
																0,
																RT_IPC_FLAG_FIFO);
	if(RT_NULL == usart_data_sem)
	{
		rt_kprintf(" \"cmcom\" sem create fail\n");
		return ;
	}

	/*camera start work mq*/
	photo_start_mq = rt_mq_create("cmwork",
																64,
																8,
																RT_IPC_FLAG_FIFO);
	if(RT_NULL == photo_start_mq)
	{
		rt_kprintf(" \"cmwork\" mq create fial\n\n");
		return ;
	}

	/*ir deal use sem */
	cm_ir_sem = rt_sem_create("cmirdeal",0,RT_IPC_FLAG_FIFO);
	if(RT_NULL == cm_ir_sem)
	{
		rt_kprintf(" \"cmirdeal\" sem create fail\n\n");
		return ;
	}

	/*picture file mutex*/
	pic_file_mutex = rt_mutex_create("pic_file",RT_IPC_FLAG_FIFO);
	if(RT_NULL == pic_file_mutex)
	{
		rt_kprintf(" \"pic_file\" sem create fail\n\n");
		return ;
	}

	/*camera start work flow event */
	work_flow_ok  = rt_event_create("workflow",RT_IPC_FLAG_FIFO);
	if(RT_NULL == work_flow_ok)
	{
		rt_kprintf(" \"workflow\" sem create fail\n\n");
		return ;
	}
	rt_event_send(work_flow_ok,FLOW_OK_FLAG);
	
	/*enable work event creat*/
	alarm_inform_event = rt_event_create("infosend",RT_IPC_FLAG_FIFO);
	if(RT_NULL == alarm_inform_event)
	{
		rt_kprintf(" \"infosend\" sem create fail\n\n");
		return;
	}
	rt_event_send(alarm_inform_event,INFO_SEND_SMS | INFO_SEND_MMS);

	/*camera data deal thread*/
	id = rt_thread_create("cm_task",
												photo_thread_entry,
												RT_NULL,
												800,
												103,
												30);
	if(RT_NULL == id )
	{
		rt_kprintf("graph thread create fail\n\n");
		
		return ;
	}
	rt_thread_startup(id);

	/*ir deal thread */
	id = rt_thread_create("cmirtask",
												camera_infrared_thread_enter,
												RT_NULL,
												256,
												108,
												100);
	if(RT_NULL == id )
	{
		rt_kprintf("graph thread create fail\n");
		
		return ;
	}
	rt_thread_startup(id);

}


void camera_send_mail(ALARM_TYPEDEF alarm_type, time_t time)
{
	struct cm_recv_mq send_mq;
	
	send_mq.time = PHOTO_INTERVAL_TIME;											//photograph time interval
	send_mq.name1 = "/1.jpg";
	send_mq.name2 = "/2.jpg";
	send_mq.date = time;									//send information photogragh time 
	send_mq.alarm_type = alarm_type;		

	if(photo_start_mq != RT_NULL)
	{
		rt_mq_send(photo_start_mq,&send_mq,sizeof(send_mq));
	}
	else
	{
		rt_kprintf("photo_start_mq is RT_NULL\n");
	}
}






/*camera infared detection  part*/
#define CM_IR_TEST_TIME						55	//leave ir check up num
#define CM_IR_ALARM_TOUCH_PERIOD	1		//10min

rt_sem_t 		cm_ir_sem = RT_NULL;			//ir keep out 2s send sem
/*ir of timer*/
rt_timer_t 	cm_alarm_timer = RT_NULL;	
static rt_timer_t cm_ir_timer = RT_NULL;
/*timer count flag*/
volatile rt_uint8_t cm_ir_time_value = 0;
volatile rt_uint8_t cm_alarm_cnt_time_value = 0;





void camera_alarm_time_interval(void *arg)
{
	cm_alarm_cnt_time_value++;
	if(cm_alarm_cnt_time_value > CM_IR_ALARM_TOUCH_PERIOD)
	{
		rt_timer_stop(cm_alarm_timer);

		/*send alarm info event*/
		rt_event_send(alarm_inform_event,INFO_SEND_MMS | INFO_SEND_SMS);
		
		rt_kprintf("start camare loopg!!!!!!!!!!!!!!!!!!!!!\n\n");
	}
}
void camera_infrared_timer(void *arg)
{
	cm_ir_time_value++;
}
void camera_infrared_thread_enter(void *arg)
{
	rt_device_t ir_dev;									//ir device
	rt_uint8_t	ir_pin_dat;									
	rt_uint32_t flag = 0;								//ir pin status real-time monitoring
	rt_uint8_t	leave_flag = 0;				
	rt_err_t 		result = RT_NULL;
	rt_uint32_t	recv_e;

	cm_ir_timer = rt_timer_create("cm_ir_t",
																camera_infrared_timer,
																RT_NULL,100,
																RT_TIMER_FLAG_PERIODIC);

	cm_alarm_timer = rt_timer_create("alam_t",
																		camera_alarm_time_interval,
																		RT_NULL,3000,
																		RT_TIMER_FLAG_PERIODIC);

	ir_dev = rt_device_find(DEVICE_NAME_CAMERA_IRDASENSOR);
	
	while(1)
	{
		/*check up machine work status(sleep or wake up)*/
  	machine_status_deal(RT_FALSE,RT_EVENT_FLAG_OR,RT_WAITING_FOREVER);
		result = rt_sem_take(cm_ir_sem,200);//ir alarm touch off
		if(RT_EOK == result)
		{			
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
					if(ir_pin_dat == IR_ACTIVE_STATUS)//have  shield
					{
						if(flag>0)
						{
							flag--;
						} 	
					}
					else
					{
						if(flag < CM_IR_TEST_TIME)//not have shield
						{
							flag++;
						}
						else
						{
							if(1 == leave_flag)//man leave
							{
								leave_flag = 2;//start	photo 
								cm_alarm_cnt_time_value = 0;
								rt_timer_start(cm_alarm_timer);//start alarm cnt timer
								camera_send_mail(ALARM_TYPE_CAMERA_IRDASENSOR,0);//send sem camera 
							}
							else	
							{
								//rt_event_send(alarm_inform_event,INFO_SEND_MMS);//send alarm info event
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
						rt_timer_stop(cm_ir_timer);
						
						rtc_dev = rt_device_find("rtc");
						
						rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &cur_data);
						/* recv work event*/
						result = rt_event_recv(alarm_inform_event,
																	INFO_SEND_MMS,
																	RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR,
																	RT_WAITING_NO,
																	&recv_e);
																	
						if(result == RT_EOK)
						{
							rt_kprintf("send mms sem\n");
							leave_flag = 1;							//have man leave
							//send sem alarm information
							send_alarm_mail(ALARM_TYPE_CAMERA_IRDASENSOR,
														 ALARM_PROCESS_FLAG_SMS |
														 ALARM_PROCESS_FLAG_GPRS | 
														 ALARM_PROCESS_FLAG_LOCAL,
														 ir_pin_dat, 
														 cur_data);
						}
						
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
				if(ir_pin_dat == IR_ACTIVE_STATUS)
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

