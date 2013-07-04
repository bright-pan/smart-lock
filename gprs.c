/*********************************************************************
 * Filename:      gprs.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-06 11:11:48
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "alarm.h"
#include "gprs.h"
#include "gpio_exti.h"
#include "gpio_adc.h"
#include "gsm.h"
#include "gsm_usart.h"
#include "untils.h"
#include "des.h"
#include <string.h>
#include "battery.h"
#include <rtc.h>
#include "mms_dev.h"

#define PIC_PER_PAGE_SIZE		512
#define PIC_NAME						"/2.jpg"

#define WORK_ALARM_CAMERA_IRDASENSOR (1 << 0)
#define WORK_ALARM_LOCK_SHELL (1 << 1)
#define WORK_ALARM_RFID_KEY_ERROR (1 << 2)
#define WORK_ALARM_LOCK_TEMPERATURE (1 << 3)
#define WORK_ALARM_RFID_KEY_PLUGIN (1 << 5)
#define WORK_ALARM_LOCK_GATE (1 << 6)

#define FAULT_ALARM_CAMERA_FAULT (1 << 0)
#define FAULT_ALARM_RFID_FAULT (1 << 1)
#define FAULT_ALARM_POWER_FAULT (1 << 2)
#define FAULT_ALARM_MOTOR_FAULT (1 << 6)

rt_mq_t gprs_mq;
static uint8_t gprs_order = 0;
static uint16_t work_alarm_type_map[50] = {0, };
static uint16_t fault_alarm_type_map[50] = {0, };

void work_alarm_type_map_init(void)
{
  work_alarm_type_map[ALARM_TYPE_LOCK_SHELL] = WORK_ALARM_LOCK_SHELL;
  work_alarm_type_map[ALARM_TYPE_CAMERA_IRDASENSOR] = WORK_ALARM_CAMERA_IRDASENSOR;
  work_alarm_type_map[ALARM_TYPE_RFID_KEY_ERROR] = WORK_ALARM_RFID_KEY_ERROR;
  work_alarm_type_map[ALARM_TYPE_LOCK_TEMPERATURE] = WORK_ALARM_LOCK_TEMPERATURE;
  work_alarm_type_map[ALARM_TYPE_RFID_KEY_PLUGIN] = WORK_ALARM_RFID_KEY_PLUGIN;
  work_alarm_type_map[ALARM_TYPE_LOCK_GATE] = WORK_ALARM_LOCK_GATE;
}

void fault_alarm_type_map_init(void)
{
  fault_alarm_type_map[ALARM_TYPE_CAMERA_FAULT] = FAULT_ALARM_CAMERA_FAULT;
  fault_alarm_type_map[ALARM_TYPE_RFID_FAULT] = FAULT_ALARM_RFID_FAULT;
  fault_alarm_type_map[ALARM_TYPE_POWER_FAULT] = FAULT_ALARM_POWER_FAULT;
  fault_alarm_type_map[ALARM_TYPE_MOTOR_FAULT] = FAULT_ALARM_MOTOR_FAULT;
}

void create_k1(void)
{
  uint8_t index = 0;
  time_t time;
  extern rt_device_t rtc_device;

  /* produce mail */
  while (index++ < 8)
  {
    rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
    srand(time);
    device_parameters.key1[index] = (rand() + device_parameters.key1[index]) % 0xff;
    rt_thread_delay(100);
  }
}

void gprs_mail_process_thread_entry(void *parameter)
{
  rt_err_t result;
  /* malloc a buff for process mail */
  GPRS_MAIL_TYPEDEF gprs_mail_buf;
  uint8_t error_counts = 0;

  create_k1();
  work_alarm_type_map_init();
  fault_alarm_type_map_init();
  /* initial msg queue */
  gprs_mq = rt_mq_create("gprs", sizeof(GPRS_MAIL_TYPEDEF),
                         GPRS_MAIL_MAX_MSGS,
                         RT_IPC_FLAG_FIFO);
  while (1)
  {
    /* process mail */
    memset(&gprs_mail_buf, 0, sizeof(GPRS_MAIL_TYPEDEF));
    result = rt_mq_recv(gprs_mq, &gprs_mail_buf, sizeof(GPRS_MAIL_TYPEDEF), 6000);
    /* mail receive ok */
    if (result == RT_EOK)
    {
      rt_kprintf("receive gprs mail < time: %d alarm_type: %d >\n", gprs_mail_buf.time, gprs_mail_buf.alarm_type);
      // send gprs data
      if (send_gprs_frame(gprs_mail_buf.alarm_type, gprs_mail_buf.time, gprs_mail_buf.order, RT_NULL) == 1)
      {
        rt_kprintf("\nsend gprs data success!!!\n");
        error_counts = 0;
      }
      else
      {
        rt_kprintf("\nsend gprs data failure!!!\n");
        error_counts++;
        if (error_counts > 10)
        {
          gsm_setup(DISABLE);
          error_counts = 0;
        }
      }
    }
    else
    {
      send_gprs_mail(ALARM_TYPE_GPRS_HEART, 0, 0, RT_NULL);
    }
  }
}

void send_gprs_mail(ALARM_TYPEDEF alarm_type, time_t time, uint8_t order,void *user)
{
  GPRS_MAIL_TYPEDEF buf;
  extern rt_device_t rtc_device;
  rt_err_t result;
  //send mail
  buf.alarm_type = alarm_type;
  if (!time)
  {
    rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &(buf.time));
  }
  else
  {
    buf.time = time;
  }
  buf.order = order;
  if(user != RT_NULL)
  {
    buf.user = user;
  }
  if (gprs_mq != NULL)
  {
    result = rt_mq_send(gprs_mq, &buf, sizeof(GPRS_MAIL_TYPEDEF));
    if (result == -RT_EFULL)
    {
      rt_kprintf("gprs_mq is full!!!\n");
    }
  }
  else
  {
    rt_kprintf("gprs_mq is RT_NULL!!!\n");
  }
}

void auth_process(GPRS_AUTH *auth)
{
  uint8_t input_buf[8], output_buf[8];
  des_context ctx_key0_enc, ctx_key1_enc;

  // process device_id
  memcpy(auth->device_id, device_parameters.device_id, 6);
  // set key for DES
  des_setkey_enc(&ctx_key0_enc, (unsigned char *)device_parameters.key0);
  des_setkey_enc(&ctx_key1_enc, (unsigned char *)device_parameters.key1);
  // DES(device_id, key1)
  memset(input_buf, 0, 8);
  memcpy(input_buf, auth->device_id, 6);
  des_crypt_ecb(&ctx_key1_enc, input_buf, output_buf);
  memcpy(auth->enc_data + 8, output_buf, 8);
  // DES(key1, key0)
  memcpy(input_buf, device_parameters.key1, 8);
  des_crypt_ecb(&ctx_key0_enc, input_buf, output_buf);
  memcpy(auth->enc_data, output_buf, 8);
  // DES(DES(device_id, key1), key0)
  memcpy(input_buf, auth->enc_data + 8, 8);
  des_crypt_ecb(&ctx_key0_enc, input_buf, output_buf);
  memcpy(auth->enc_data + 8, output_buf, 8);
}

uint8_t dec_to_bcd(uint8_t dec)
{
  uint8_t temp = dec % 100;
  return ((temp / 10)<<4) + ((temp % 10) & 0x0F); 
}

static void get_dec_to_bcd_time(uint8_t date[],time_t time)
{
  struct tm time_tm;
	
  time_tm = *localtime(&time);
  time_tm.tm_year += 1900;
  time_tm.tm_mon += 1;
  date[0] = dec_to_bcd((uint8_t)(time_tm.tm_year % 100));
  date[1] = dec_to_bcd((uint8_t)(time_tm.tm_mon % 100));
  date[2] = dec_to_bcd((uint8_t)(time_tm.tm_mday % 100));
  date[3] = dec_to_bcd((uint8_t)(time_tm.tm_hour % 100));
  date[4] = dec_to_bcd((uint8_t)(time_tm.tm_min % 100));
  date[5] = dec_to_bcd((uint8_t)(time_tm.tm_sec % 100));
}
static void get_pic_size(const char* name ,rt_uint32_t *size)
{
	struct stat status;

	if(stat(name,&status) >= 0)
	{
		*size = status.st_size;
	}
}

void lock_open_process(GPRS_LOCK_OPEN *lock_open,time_t time,rt_uint8_t key[])
{
	struct tm time_tm;
	
	strncpy((char*)lock_open->key,(const char*)key,4);
	time_tm = *localtime(&time);
  time_tm.tm_year += 1900;
  time_tm.tm_mon += 1;
	lock_open->time[0] = dec_to_bcd((uint8_t)(time_tm.tm_year % 100));
  lock_open->time[1] = dec_to_bcd((uint8_t)(time_tm.tm_mon % 100));
  lock_open->time[2] = dec_to_bcd((uint8_t)(time_tm.tm_mday % 100));
  lock_open->time[3] = dec_to_bcd((uint8_t)(time_tm.tm_hour % 100));
  lock_open->time[4] = dec_to_bcd((uint8_t)(time_tm.tm_min % 100));
  lock_open->time[5] = dec_to_bcd((uint8_t)(time_tm.tm_sec % 100));
  //get_dec_to_bcd_time(lock_open->time,time)
}

void work_alarm_process(ALARM_TYPEDEF alarm_type, GPRS_WORK_ALARM *work_alarm, time_t time)
{
  rt_device_t device_motor_status = RT_NULL;
  uint8_t dat = 0;
  struct tm tm_time;
  
  device_motor_status = rt_device_find(DEVICE_NAME_MOTOR_STATUS);
  
  work_alarm->type = work_alarm_type_map[alarm_type];

  rt_device_read(device_motor_status,0,&dat,0);
  work_alarm->status = dat;

  tm_time = *localtime(&time);
  tm_time.tm_year += 1900;
  tm_time.tm_mon += 1;

  work_alarm->time[0] = dec_to_bcd((uint8_t)(tm_time.tm_year % 100));
  work_alarm->time[1] = dec_to_bcd((uint8_t)(tm_time.tm_mon % 100));
  work_alarm->time[2] = dec_to_bcd((uint8_t)(tm_time.tm_mday % 100));
  work_alarm->time[3] = dec_to_bcd((uint8_t)(tm_time.tm_hour % 100));
  work_alarm->time[4] = dec_to_bcd((uint8_t)(tm_time.tm_min % 100));
  work_alarm->time[5] = dec_to_bcd((uint8_t)(tm_time.tm_sec % 100));
}

void fault_alarm_process(ALARM_TYPEDEF alarm_type, GPRS_FAULT_ALARM* fault_alarm, time_t time)
{
  struct tm tm_time;

  fault_alarm->type = fault_alarm_type_map[alarm_type];

  tm_time = *localtime(&time);
  tm_time.tm_year += 1900;
  tm_time.tm_mon += 1;

  fault_alarm->time[0] = dec_to_bcd((uint8_t)(tm_time.tm_year % 100));
  fault_alarm->time[1] = dec_to_bcd((uint8_t)(tm_time.tm_mon % 100));
  fault_alarm->time[2] = dec_to_bcd((uint8_t)(tm_time.tm_mday % 100));
  fault_alarm->time[3] = dec_to_bcd((uint8_t)(tm_time.tm_hour % 100));
  fault_alarm->time[4] = dec_to_bcd((uint8_t)(tm_time.tm_min % 100));
  fault_alarm->time[5] = dec_to_bcd((uint8_t)(tm_time.tm_sec % 100));
}

void battery_alarm_process(GPRS_POWER_ALARM* power_alarm,time_t time)
{
	struct tm		date;
	Battery_Data battery_dat;

	battery_get_data(&battery_dat);
	power_alarm->status = battery_dat.status;
	power_alarm->dump_energy = battery_dat.work_time;
  date = *localtime(&time);
  date.tm_year += 1900;
  date.tm_mon += 1;

  power_alarm->time[0] = dec_to_bcd((uint8_t)(date.tm_year % 100));
  power_alarm->time[1] = dec_to_bcd((uint8_t)(date.tm_mon % 100));
  power_alarm->time[2] = dec_to_bcd((uint8_t)(date.tm_mday % 100));
  power_alarm->time[3] = dec_to_bcd((uint8_t)(date.tm_hour % 100));
  power_alarm->time[4] = dec_to_bcd((uint8_t)(date.tm_min % 100));
  power_alarm->time[5] = dec_to_bcd((uint8_t)(date.tm_sec % 100));
}
void process_gprs_pic_arg(GPRS_SEND_PIC_ASK *pic_info,time_t time,void* user)
{
	struct tm		date;
	rt_uint32_t size;
	ALARM_TYPEDEF temp;

	temp = *(ALARM_TYPEDEF*)user;
	switch(temp)
	{
		case ALARM_TYPE_CAMERA_IRDASENSOR:
		{
			pic_info->type = 1;
			break;
		}
		case ALARM_TYPE_RFID_KEY_ERROR:
		{
			pic_info->type = 3;
			break;
		}
		case ALARM_TYPE_LOCK_TEMPERATURE:
		{
			pic_info->type = 4;
			break;
		}
	}
	if(pic_info->type == 0)
	{
		pic_info->type = 1;
	}
	//pic_info->type = *(rt_uint8_t *)user;
	pic_info->form = 1;
	pic_info->DPI = 1;
	get_pic_size("/2.jpg",&size);
	pic_info->pic_size[0] = (size & 0x0000ff00)>>8;
	pic_info->pic_size[1] = (size & 0x000000ff);
	pic_info->page = (size / 512);
	if((size % 512) != 0)
	{
		pic_info->page++;
	}

	rt_kprintf("time = %d\n",time);
	date = *localtime(&time);
	date.tm_year += 1900;
	date.tm_mon += 1;

	pic_info->time[0] = dec_to_bcd((uint8_t)(date.tm_year % 100));
  pic_info->time[1] = dec_to_bcd((uint8_t)(date.tm_mon % 100));
  pic_info->time[2] = dec_to_bcd((uint8_t)(date.tm_mday % 100));
  pic_info->time[3] = dec_to_bcd((uint8_t)(date.tm_hour % 100));
  pic_info->time[4] = dec_to_bcd((uint8_t)(date.tm_min % 100));
  pic_info->time[5] = dec_to_bcd((uint8_t)(date.tm_sec % 100));
}

void process_gprs_list_telephone(GPRS_SEND_LIST_TELEPHONE *list_telephone, uint16_t *length)
{
  uint8_t index = 0;
  uint8_t *buf = RT_NULL;

  list_telephone->counts = 0;
  list_telephone->result = 1;
  buf = &(list_telephone->telephone[0][0]);

  for (index = 0; index < TELEPHONE_NUMBERS; index++)
  {
    if (device_parameters.alarm_telephone[index].flag)
    {
      (list_telephone->counts)++;
      memcpy(buf, device_parameters.alarm_telephone[index].address + 2, 11);
      buf += 11;
    }
  }

  *length = 2 + 11 * list_telephone->counts;
}

void process_gprs_list_rfid_key(GPRS_SEND_LIST_RFID_KEY *list_rfid_key, uint16_t *length)
{
  uint8_t index = 0;
  uint8_t *buf = RT_NULL;

  list_rfid_key->counts = 0;
  list_rfid_key->result = 1;
  buf = &(list_rfid_key->key[0][0]);

  for (index = 0; index < RFID_KEY_NUMBERS; index++)
  {
    if (device_parameters.rfid_key[index].flag)
    {
      (list_rfid_key->counts)++;
      memcpy(buf, device_parameters.rfid_key[index].key, 4);
      buf += 4;
    }
  }

  *length = 2 + 4 * list_rfid_key->counts;
}
void process_gprs_list_user_parameters(GPRS_SEND_LIST_USER_PARAMETERS *list_user_parameters, uint16_t *length)
{
  list_user_parameters->result = 1;
  *length += 1;

  list_user_parameters->parameters = device_parameters.lock_gate_alarm_time;
  *length += 1;
}

int8_t send_gprs_auth_frame(void)
{
  uint8_t *process_buf = RT_NULL;
  uint8_t *process_buf_bk = RT_NULL;
  uint16_t process_length = 0;
  GPRS_SEND_FRAME_TYPEDEF *gprs_send_frame = RT_NULL;
  GPRS_RECV_FRAME_TYPEDEF *gprs_recv_frame = RT_NULL;
  rt_device_t device = RT_NULL;
  int8_t send_result = -1;
  rt_size_t recv_counts;
  int8_t send_counts;

  device = rt_device_find(DEVICE_NAME_GSM_USART);

  if (device == RT_NULL)
  {
    rt_kprintf("device %s is not exist!\n", DEVICE_NAME_GSM_USART);
    return -1;
  }

  gprs_send_frame = (GPRS_SEND_FRAME_TYPEDEF *)rt_malloc(sizeof(GPRS_SEND_FRAME_TYPEDEF));
  memset(gprs_send_frame, 0, sizeof(GPRS_SEND_FRAME_TYPEDEF));
  gprs_recv_frame = (GPRS_RECV_FRAME_TYPEDEF *)rt_malloc(sizeof(GPRS_RECV_FRAME_TYPEDEF));
  memset(gprs_recv_frame, 0, sizeof(GPRS_RECV_FRAME_TYPEDEF));
  // frame process
  gprs_send_frame->length = 0x16;
  gprs_send_frame->cmd = 0x11;
  gprs_send_frame->order = 0;

  auth_process(&(gprs_send_frame->data.auth));
  // send process
  process_buf = (uint8_t *)rt_malloc(sizeof(GPRS_SEND_FRAME_TYPEDEF));
  memset(process_buf, 0, sizeof(GPRS_SEND_FRAME_TYPEDEF));
  process_buf_bk = process_buf;
  process_length = 0;
  
  *process_buf_bk++ = (uint8_t)((gprs_send_frame->length) >> 8 & 0xff);
  *process_buf_bk++ = (uint8_t)(gprs_send_frame->length & 0xff);
  process_length += 2;

  *process_buf_bk++ = gprs_send_frame->cmd;
  process_length += 1;

  *process_buf_bk++ = gprs_send_frame->order;
  process_length += 1;
  memcpy(process_buf_bk, gprs_send_frame->data.auth.device_id, 6);
  memcpy(process_buf_bk+6, gprs_send_frame->data.auth.enc_data, 16);
  process_length += 22;
  //send frame
  gsm_put_hex(process_buf, process_length);
  rt_device_write(device, 0, process_buf, process_length);

  send_counts = 10;
  while (send_counts-- > 0)
  {
    rt_thread_delay(100);

    //gprs data
    recv_counts = rt_device_read(device, 0, gprs_recv_frame, sizeof(GPRS_RECV_FRAME_TYPEDEF));
    if (recv_counts)
    {
      recv_gprs_frame(gprs_recv_frame, recv_counts);
      break;
    }
  }

  rt_free(process_buf);
  rt_free(gprs_send_frame);
  rt_free(gprs_recv_frame);
  return send_result;
}
void send_picture_data(void)
{
	int 					file_id = 0;
	rt_uint8_t*		data_head_point = RT_NULL;
	rt_uint8_t		loop_cnt = 0;
	rt_uint8_t		page  = 0;
	rt_uint32_t		send_size = 0;
	rt_uint32_t 	size = 0;
	rt_uint32_t		offset = 0;
	rt_device_t		dev = RT_NULL;
	GPRS_SEND_PIC pic_data;
	GSM_MAIL_TYPEDEF gsm_mail_buf;
	rt_uint8_t		send_result;
	rt_uint8_t		result;

	dev = rt_device_find(DEVICE_NAME_GSM_USART);
	if(dev == RT_NULL)
	{
		return ;
	}
	
	data_head_point = pic_data.data = rt_malloc(PIC_PER_PAGE_SIZE+5);

	rt_memset(pic_data.data,0,PIC_PER_PAGE_SIZE+5);
	
	get_pic_size(PIC_NAME,&size);

	loop_cnt = (size / PIC_PER_PAGE_SIZE)+1;
	
	rt_kprintf("size = %d",size);
									
	pic_data.cmd = 0x0f;
	pic_data.cur_page  = 0;
	file_id = open(PIC_NAME,O_RDONLY,0);
	pic_data.order = gprs_order;
	while(loop_cnt--)
	{
		if( (size / PIC_PER_PAGE_SIZE) > 0)
		{
			size -= PIC_PER_PAGE_SIZE;
			pic_data.length = PIC_PER_PAGE_SIZE+1;
			page++;
			offset = page*PIC_PER_PAGE_SIZE;
		}
		else if(loop_cnt == 0)
		{
			pic_data.length = size % PIC_PER_PAGE_SIZE+1; 
		}
		rt_kprintf("\roffset = %d\r",offset);
		
		lseek(file_id,offset,SEEK_SET);
		
		offset = page*PIC_PER_PAGE_SIZE;
		
		read(file_id,(void*)(pic_data.data+5),pic_data.length-1);

		send_size = pic_data.length+4;
		
		*((pic_data.data)+0) = (rt_uint8_t)((pic_data.length) >> 8 & 0xff);
		*((pic_data.data)+1) = (rt_uint8_t)(pic_data.length & 0xff);
		*((pic_data.data)+2) = pic_data.cmd;
		*((pic_data.data)+3) = pic_data.order;
		*((pic_data.data)+3) = pic_data.order;
		*((pic_data.data)+4) = ++loop_cnt;
		//*((pic_data.data)+4) = pic_data.cur_page;
		pic_data.cur_page++;
		
		gsm_put_hex(pic_data.data, send_size);
		//rt_device_write(device, 0, process_buf, process_length);
		gsm_mail_buf.send_mode = GSM_MODE_GPRS;
		gsm_mail_buf.result = &send_result;
		gsm_mail_buf.result_sem = rt_sem_create("g_ret", 0, RT_IPC_FLAG_FIFO);
		gsm_mail_buf.mail_data.gprs.request = pic_data.data;
		gsm_mail_buf.mail_data.gprs.request_length = send_size;
		gsm_mail_buf.mail_data.gprs.has_response = 0;

		result = rt_mq_send(mq_gsm, &gsm_mail_buf, sizeof(GSM_MAIL_TYPEDEF));

		rt_sem_take(gsm_mail_buf.result_sem, RT_WAITING_FOREVER);
		
		//rt_thread_delay(50);
		rt_kprintf("loop_cnt = %d\n",loop_cnt);
		rt_memset(pic_data.data,0,PIC_PER_PAGE_SIZE+5);
	}
	
	close(file_id);
	free(data_head_point);
}

int8_t send_gprs_frame(ALARM_TYPEDEF alarm_type, time_t time, uint8_t order, void* user)
{
  uint8_t *process_buf = RT_NULL;
  uint8_t *process_buf_bk = RT_NULL;
  uint16_t process_length = 0;
  GPRS_SEND_FRAME_TYPEDEF *gprs_send_frame = RT_NULL;
  GPRS_RECV_FRAME_TYPEDEF *gprs_recv_frame = RT_NULL;
  rt_device_t device = RT_NULL;
  GSM_MAIL_TYPEDEF gsm_mail_buf;
  int8_t result = -1;
  int8_t send_result = -1;
  uint16_t recv_counts = 0;

  device = rt_device_find(DEVICE_NAME_GSM_USART);

  if (device == RT_NULL)
  {
    rt_kprintf("device %s is not exist!\n", DEVICE_NAME_GSM_USART);
    return -1;
  }

  gprs_send_frame = (GPRS_SEND_FRAME_TYPEDEF *)rt_malloc(sizeof(GPRS_SEND_FRAME_TYPEDEF));
  memset(gprs_send_frame, 0, sizeof(GPRS_SEND_FRAME_TYPEDEF));

  gsm_mail_buf.mail_data.gprs.has_response = 1;
  // frame process
  switch (alarm_type)
  {
    case ALARM_TYPE_GPRS_AUTH : {

      gprs_send_frame->length = 0x16;
      gprs_send_frame->cmd = 0x11;
      gprs_send_frame->order = gprs_order++;

      auth_process(&(gprs_send_frame->data.auth));

      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_BATTERY_WORKING_20M:
    {
    	gprs_send_frame->length = 8;
    	gprs_send_frame->cmd = 0x02;
    	gprs_send_frame->order = gprs_order++;
    	
			battery_alarm_process(&(gprs_send_frame->data.battery),time);
			
			break;
    }
    case ALARM_TYPE_RFID_KEY_SUCCESS: {

      gprs_send_frame->length = 10;
      gprs_send_frame->cmd = 0x01;
      gprs_send_frame->order = gprs_order++;

      lock_open_process(&(gprs_send_frame->data.lock_open),time,(rt_uint8_t *)user);

      break;
    };    
    case ALARM_TYPE_GPRS_HEART : {

      gprs_send_frame->length = 0x1;
      gprs_send_frame->cmd = 0x00;
      gprs_send_frame->order = gprs_order++;

      gprs_send_frame->data.heart.type = 0x02;
      break;
    };
    case ALARM_TYPE_CAMERA_IRDASENSOR: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x3;
      gprs_send_frame->order = gprs_order++;

      work_alarm_process(ALARM_TYPE_CAMERA_IRDASENSOR, &(gprs_send_frame->data.work_alarm), time);

      break;
    };
    case ALARM_TYPE_LOCK_SHELL: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x3;
      gprs_send_frame->order = gprs_order++;

      work_alarm_process(ALARM_TYPE_LOCK_SHELL, &(gprs_send_frame->data.work_alarm), time);

      break;
    };
    case ALARM_TYPE_LOCK_GATE: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x3;
      gprs_send_frame->order = gprs_order++;

      work_alarm_process(ALARM_TYPE_LOCK_GATE, &(gprs_send_frame->data.work_alarm), time);

      break;
    };
    case ALARM_TYPE_LOCK_TEMPERATURE: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x3;
      gprs_send_frame->order = gprs_order++;

      work_alarm_process(ALARM_TYPE_LOCK_TEMPERATURE, &(gprs_send_frame->data.work_alarm), time);

      break;
    };
    case ALARM_TYPE_RFID_KEY_ERROR: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x3;
      gprs_send_frame->order = gprs_order++;

      work_alarm_process(ALARM_TYPE_RFID_KEY_ERROR, &(gprs_send_frame->data.work_alarm), time);

      break;
    };
    case ALARM_TYPE_RFID_KEY_PLUGIN: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x3;
      gprs_send_frame->order = gprs_order++;

      work_alarm_process(ALARM_TYPE_RFID_KEY_PLUGIN, &(gprs_send_frame->data.work_alarm), time);

      break;
    };
    case ALARM_TYPE_RFID_FAULT: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x4;
      gprs_send_frame->order = gprs_order++;

      fault_alarm_process(ALARM_TYPE_RFID_FAULT, &(gprs_send_frame->data.fault_alarm), time);

      break;
    };
    case ALARM_TYPE_CAMERA_FAULT: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x4;
      gprs_send_frame->order = gprs_order++;

      fault_alarm_process(ALARM_TYPE_CAMERA_FAULT, &(gprs_send_frame->data.fault_alarm), time);

      break;
    };
    case ALARM_TYPE_POWER_FAULT: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x4;
      gprs_send_frame->order = gprs_order++;

      fault_alarm_process(ALARM_TYPE_POWER_FAULT, &(gprs_send_frame->data.fault_alarm), time);

      break;
    };
    case ALARM_TYPE_MOTOR_FAULT: {

      gprs_send_frame->length = 0x8;
      gprs_send_frame->cmd = 0x4;
      gprs_send_frame->order = gprs_order++;

      fault_alarm_process(ALARM_TYPE_MOTOR_FAULT, &(gprs_send_frame->data.fault_alarm), time);

      break;
    };
    case ALARM_TYPE_GPRS_UPLOAD_PIC:
    {
    	gprs_send_frame->length = 12;
      gprs_send_frame->cmd = 0x0E;
      gprs_send_frame->order = 	gprs_order++;

      process_gprs_pic_arg(&(gprs_send_frame->data.picture),time,user);

      gsm_mail_buf.mail_data.gprs.has_response = 1;
			break;
    }
    case ALARM_TYPE_GPRS_LIST_TELEPHONE: {

      gprs_send_frame->length = 0;
      gprs_send_frame->cmd = 0x06;
      gprs_send_frame->order = order;

      process_gprs_list_telephone(&(gprs_send_frame->data.list_telephone), &(gprs_send_frame->length));
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SET_TELEPHONE_SUCCESS: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x07;
      gprs_send_frame->order = order;

      gprs_send_frame->data.set_telephone.result = 1;
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SET_TELEPHONE_FAILURE: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x07;
      gprs_send_frame->order = order;

      gprs_send_frame->data.set_telephone.result = 0;
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };

    case ALARM_TYPE_GPRS_LIST_RFID_KEY: {

      gprs_send_frame->length = 0;
      gprs_send_frame->cmd = 0x08;
      gprs_send_frame->order = order;

      process_gprs_list_rfid_key(&(gprs_send_frame->data.list_rfid_key), &(gprs_send_frame->length));
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SET_RFID_KEY_SUCCESS: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x09;
      gprs_send_frame->order = order;

      gprs_send_frame->data.set_rfid_key.result = 1;
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SET_RFID_KEY_FAILURE: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x09;
      gprs_send_frame->order = order;

      gprs_send_frame->data.set_rfid_key.result = 0;
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_GPRS_LIST_USER_PARAMETERS: {

      gprs_send_frame->length = 0;
      gprs_send_frame->cmd = 0x0A;
      gprs_send_frame->order = order;

      process_gprs_list_user_parameters(&(gprs_send_frame->data.list_user_parameters), &(gprs_send_frame->length));
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SET_USER_PARAMETERS_SUCCESS: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x0B;
      gprs_send_frame->order = order;

      gprs_send_frame->data.set_user_parameters.result = 1;
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SET_USER_PARAMETERS_FAILURE: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x0B;
      gprs_send_frame->order = order;

      gprs_send_frame->data.set_user_parameters.result = 0;
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SET_TIME_SUCCESS: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x15;
      gprs_send_frame->order = order;

      gprs_send_frame->data.set_time.result = 1;
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SET_TIME_FAILURE: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x15;
      gprs_send_frame->order = order;

      gprs_send_frame->data.set_time.result = 0;
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SET_KEY0_SUCCESS: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x18;
      gprs_send_frame->order = order;

      gprs_send_frame->data.set_key0.result = 1;
      break;
    };
    case ALARM_TYPE_GPRS_SET_KEY0_FAILURE: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x18;
      gprs_send_frame->order = order;

      gprs_send_frame->data.set_key0.result = 0;
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    default : {
      break;
    };
  }

  // send process
  process_buf = (uint8_t *)rt_malloc(sizeof(GPRS_SEND_FRAME_TYPEDEF));
  memset(process_buf, 0, sizeof(GPRS_SEND_FRAME_TYPEDEF));
  process_buf_bk = process_buf;
  process_length = 0;
  
  *process_buf_bk++ = (uint8_t)((gprs_send_frame->length) >> 8 & 0xff);
  *process_buf_bk++ = (uint8_t)(gprs_send_frame->length & 0xff);
  process_length += 2;

  *process_buf_bk++ = gprs_send_frame->cmd;
  process_length += 1;

  *process_buf_bk++ = gprs_send_frame->order;
  process_length += 1;

  switch (alarm_type)
  {
    case ALARM_TYPE_GPRS_AUTH : {

      memcpy(process_buf_bk, gprs_send_frame->data.auth.device_id, 6);
      memcpy(process_buf_bk+6, gprs_send_frame->data.auth.enc_data, 16);
      process_length += 22;
      break;
    };
    case ALARM_TYPE_BATTERY_WORKING_20M:{
    	*process_buf_bk++ = gprs_send_frame->data.battery.status;
    	*process_buf_bk++ = gprs_send_frame->data.battery.dump_energy;
    	memcpy(process_buf_bk, gprs_send_frame->data.battery.time, 6);
    	process_length += 8;
			break;
    }
    case ALARM_TYPE_RFID_KEY_SUCCESS: {	
    	memcpy(process_buf_bk,gprs_send_frame->data.lock_open.key,4);
			process_buf_bk += 4;
//			process_length += 4;
			memcpy(process_buf_bk,gprs_send_frame->data.lock_open.time,6);
			process_length += 10;
			break;
    }
    
    case ALARM_TYPE_GPRS_HEART : {

      *process_buf_bk++ = gprs_send_frame->data.heart.type;
      process_length += 1;
      break;
    };
    case ALARM_TYPE_CAMERA_IRDASENSOR: {
      
      *process_buf_bk++ = (uint8_t)((gprs_send_frame->data.work_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->data.work_alarm.type & 0xff);
      process_length += 2;
      
      *process_buf_bk++ = gprs_send_frame->data.work_alarm.status;
      process_length += 1;
      
      memcpy(process_buf_bk, gprs_send_frame->data.work_alarm.time, 6);
      process_length += 6;      
      break;
    };
    case ALARM_TYPE_LOCK_SHELL: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->data.work_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->data.work_alarm.type & 0xff);
      process_length += 2;
      
      *process_buf_bk++ = gprs_send_frame->data.work_alarm.status;
      process_length += 1;
      
      memcpy(process_buf_bk, gprs_send_frame->data.work_alarm.time, 6);
      process_length += 6;
      break;
    };
    case ALARM_TYPE_LOCK_GATE: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->data.work_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->data.work_alarm.type & 0xff);
      process_length += 2;
      
      *process_buf_bk++ = gprs_send_frame->data.work_alarm.status;
      process_length += 1;
      
      memcpy(process_buf_bk, gprs_send_frame->data.work_alarm.time, 6);
      process_length += 6;      
      break;
    };
    case ALARM_TYPE_LOCK_TEMPERATURE: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->data.work_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->data.work_alarm.type & 0xff);
      process_length += 2;
      
      *process_buf_bk++ = gprs_send_frame->data.work_alarm.status;
      process_length += 1;
      
      memcpy(process_buf_bk, gprs_send_frame->data.work_alarm.time, 6);
      process_length += 6;      
      break;
    };
    case ALARM_TYPE_RFID_KEY_ERROR: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->data.work_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->data.work_alarm.type & 0xff);
      process_length += 2;
      
      *process_buf_bk++ = gprs_send_frame->data.work_alarm.status;
      process_length += 1;
      
      memcpy(process_buf_bk, gprs_send_frame->data.work_alarm.time, 6);
      process_length += 6;      
      break;
    };
    case ALARM_TYPE_RFID_KEY_PLUGIN: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->data.work_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->data.work_alarm.type & 0xff);
      process_length += 2;
      
      *process_buf_bk++ = gprs_send_frame->data.work_alarm.status;
      process_length += 1;
      
      memcpy(process_buf_bk, gprs_send_frame->data.work_alarm.time, 6);
      process_length += 6;
      break;
    };

    case ALARM_TYPE_RFID_FAULT: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->data.fault_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->data.fault_alarm.type & 0xff);
      process_length += 2;

      memcpy(process_buf_bk, gprs_send_frame->data.fault_alarm.time, 6);
      process_length += 6;
      break;
    };
    case ALARM_TYPE_CAMERA_FAULT: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->data.fault_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->data.fault_alarm.type & 0xff);
      process_length += 2;

      memcpy(process_buf_bk, gprs_send_frame->data.fault_alarm.time, 6);
      process_length += 6;
      break;
    };
    case ALARM_TYPE_POWER_FAULT: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->data.fault_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->data.fault_alarm.type & 0xff);
      process_length += 2;

      memcpy(process_buf_bk, gprs_send_frame->data.fault_alarm.time, 6);
      process_length += 6;
      break;
    };
    case ALARM_TYPE_MOTOR_FAULT: {

      *process_buf_bk++ = (uint8_t)((gprs_send_frame->data.fault_alarm.type) >> 8 & 0xff);
      *process_buf_bk++ = (uint8_t)(gprs_send_frame->data.fault_alarm.type & 0xff);
      process_length += 2;

      memcpy(process_buf_bk, gprs_send_frame->data.fault_alarm.time, 6);
      process_length += 6;
      break;
    };
    case ALARM_TYPE_GPRS_LIST_TELEPHONE: {

      *process_buf_bk++ = gprs_send_frame->data.list_telephone.result;
      *process_buf_bk++ = gprs_send_frame->data.list_telephone.counts;
      process_length += 2;

      memcpy(process_buf_bk, gprs_send_frame->data.list_telephone.telephone, 11 * gprs_send_frame->data.list_telephone.counts);
      process_length += 11 * gprs_send_frame->data.list_telephone.counts;
      break;
    };
    case ALARM_TYPE_GPRS_SET_TELEPHONE_SUCCESS: {

      *process_buf_bk++ = gprs_send_frame->data.set_telephone.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_TELEPHONE_FAILURE: {

      *process_buf_bk++ = gprs_send_frame->data.set_telephone.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_LIST_RFID_KEY: {

      *process_buf_bk++ = gprs_send_frame->data.list_rfid_key.result;
      *process_buf_bk++ = gprs_send_frame->data.list_rfid_key.counts;
      process_length += 2;

      memcpy(process_buf_bk, gprs_send_frame->data.list_rfid_key.key, 11 * gprs_send_frame->data.list_rfid_key.counts);
      process_length += 4 * gprs_send_frame->data.list_rfid_key.counts;
      break;
    };
    case ALARM_TYPE_GPRS_SET_RFID_KEY_SUCCESS: {

      *process_buf_bk++ = gprs_send_frame->data.set_rfid_key.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_RFID_KEY_FAILURE: {

      *process_buf_bk++ = gprs_send_frame->data.set_rfid_key.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_LIST_USER_PARAMETERS: {

      *process_buf_bk++ = gprs_send_frame->data.list_user_parameters.result;
      *process_buf_bk++ = gprs_send_frame->data.list_user_parameters.parameters;
      process_length += 2;

      break;
    };
    case ALARM_TYPE_GPRS_SET_USER_PARAMETERS_SUCCESS: {

      *process_buf_bk++ = gprs_send_frame->data.set_user_parameters.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_USER_PARAMETERS_FAILURE: {

      *process_buf_bk++ = gprs_send_frame->data.set_user_parameters.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_TIME_SUCCESS: {

      *process_buf_bk++ = gprs_send_frame->data.set_time.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_TIME_FAILURE: {

      *process_buf_bk++ = gprs_send_frame->data.set_time.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_KEY0_SUCCESS: {

      *process_buf_bk++ = gprs_send_frame->data.set_key0.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_KEY0_FAILURE: {

      *process_buf_bk++ = gprs_send_frame->data.set_key0.result;
      process_length += 1;

      break;
    };


    case ALARM_TYPE_GPRS_UPLOAD_PIC:
    {
    	*process_buf_bk++ = gprs_send_frame->data.picture.type;
    	*process_buf_bk++ = gprs_send_frame->data.picture.form;
    	*process_buf_bk++ = gprs_send_frame->data.picture.DPI;
    	*process_buf_bk++ = gprs_send_frame->data.picture.pic_size[0];
    	*process_buf_bk++ = gprs_send_frame->data.picture.pic_size[1];
    	*process_buf_bk++ = gprs_send_frame->data.picture.page;
    	rt_memcpy(process_buf_bk,gprs_send_frame->data.picture.time,6);
    	process_length+=12;
			break;
    }
    case ALARM_TYPE_GPRS_SEND_PIC_DATA:
    {
			send_picture_data();
			rt_free(gprs_send_frame);
  		rt_free(process_buf);
  		
  		return 1;
    }
    default : {
      break;
    };
  }

  //send frame
  gsm_put_hex(process_buf, process_length);
  //rt_device_write(device, 0, process_buf, process_length);
  gsm_mail_buf.send_mode = GSM_MODE_GPRS;
  gsm_mail_buf.result = &send_result;
  gsm_mail_buf.result_sem = rt_sem_create("g_ret", 0, RT_IPC_FLAG_FIFO);
  gsm_mail_buf.mail_data.gprs.request = process_buf;
  gsm_mail_buf.mail_data.gprs.request_length = process_length;


  if (gsm_mail_buf.mail_data.gprs.has_response == 1)
  {
    gprs_recv_frame = (GPRS_RECV_FRAME_TYPEDEF *)rt_malloc(sizeof(GPRS_RECV_FRAME_TYPEDEF));
    memset(gprs_recv_frame, 0, sizeof(GPRS_RECV_FRAME_TYPEDEF));

    gsm_mail_buf.mail_data.gprs.response = (uint8_t *)gprs_recv_frame;
    gsm_mail_buf.mail_data.gprs.response_length = &recv_counts;
  }

  if (mq_gsm != NULL)
  {
    if (alarm_type == ALARM_TYPE_GPRS_AUTH)
    {
      result = rt_mq_urgent(mq_gsm, &gsm_mail_buf, sizeof(GSM_MAIL_TYPEDEF));
    }
    else
    {
      result = rt_mq_send(mq_gsm, &gsm_mail_buf, sizeof(GSM_MAIL_TYPEDEF));
    }

    if (result == -RT_EFULL)
    {
      rt_kprintf("mq_gsm is full!!!\n");
      send_result = -1;
    }
    else
    {
      rt_sem_take(gsm_mail_buf.result_sem, RT_WAITING_FOREVER);
      if (gsm_mail_buf.mail_data.gprs.has_response == 1)
      {
        if (send_result == 1)
        {
          if (recv_gprs_frame(gprs_recv_frame, recv_counts) == 1)
          {
            if ((gprs_recv_frame->cmd == (gprs_send_frame->cmd | 0x80)) &&
                (gprs_recv_frame->order == gprs_send_frame->order))
            {
              send_result = 1;
            }
            else
            {
              send_result = -1;
            }
          }
          else
          {
            send_result = -1;
          }
        }
        else
        {
          send_result = -1;
        }
      }
      else
      {
        send_result = 1;
      }
    }
    rt_sem_delete(gsm_mail_buf.result_sem);
  }
  else
  {
    rt_kprintf("mq_gsm is RT_NULL!!!\n");
    send_result = -1;
  }

  rt_thread_delay(50);
  rt_free(gprs_send_frame);
  rt_free(gprs_recv_frame);
  rt_free(process_buf);
  return send_result;

}


void process_gprs_set_telephone(GPRS_SET_TELEPHONE *set_telephone)
{
  uint8_t index = 0;
  uint8_t *buf = RT_NULL;

  buf = &set_telephone->telephone[0][0];


  if (set_telephone->counts > TELEPHONE_NUMBERS)
  {
    return;
  }

  for (index = 0; index < TELEPHONE_NUMBERS; index++)
  {
    device_parameters.alarm_telephone[index].flag = 0;
  }

  for (index = 0; index < set_telephone->counts; index++)
  {
    device_parameters.alarm_telephone[index].flag = 1;
    memcpy(device_parameters.alarm_telephone[index].address, "86", 2);
    memcpy(device_parameters.alarm_telephone[index].address + 2, buf, 11);
    buf += 11;
  }
  // save to file
  system_file_operate(&device_parameters, 1);
}

void process_gprs_set_rfid_key(GPRS_SET_RFID_KEY *set_rfid_key)
{
  uint8_t index = 0;
  uint8_t *buf = RT_NULL;

  buf = &set_rfid_key->key[0][0];

  if (set_rfid_key->counts > RFID_KEY_NUMBERS)
  {
    return;
  }

  for (index = 0; index < RFID_KEY_NUMBERS; index++)
  {
    device_parameters.rfid_key[index].flag = 0;
  }

  for (index = 0; index < set_rfid_key->counts; index++)
  {
    device_parameters.rfid_key[index].flag = 1;
    memcpy(device_parameters.rfid_key[index].key, buf, 4);
    buf += 4;
  }
  // save to file
  system_file_operate(&device_parameters, 1);
}
void process_gprs_set_user_parameters(GPRS_SET_USER_PARAMETERS *set_user_parameters)
{
  device_parameters.lock_gate_alarm_time = set_user_parameters->parameters;
  //save to file
  system_file_operate(&device_parameters, 1);
}

uint8_t bcd_to_dec(uint8_t bcd)
{
  if (bcd >> 4 >= 0x0A)
  {
    return 0;
  }
  else
  {
    return (bcd >> 4) * 10 + (bcd & 0x0f);
  }
}
void process_gprs_set_time(GPRS_SET_TIME *gprs_set_time)
{
  extern rt_err_t set_date(rt_uint32_t year, rt_uint32_t month, rt_uint32_t day);
  extern rt_err_t set_time(rt_uint32_t hour, rt_uint32_t minute, rt_uint32_t second);
	
  set_date(bcd_to_dec(gprs_set_time->time[0]) + 2000, bcd_to_dec(gprs_set_time->time[1]), bcd_to_dec(gprs_set_time->time[2]));
  set_time(bcd_to_dec(gprs_set_time->time[3]),bcd_to_dec(gprs_set_time->time[4]),bcd_to_dec(gprs_set_time->time[5]));
}

void process_gprs_set_key0(GPRS_SET_KEY0 *gprs_set_key0)
{
  memcpy(device_parameters.key0, gprs_set_key0->key0, 8);
  //save to file
  system_file_operate(&device_parameters, 1);
}

int8_t recv_gprs_frame(GPRS_RECV_FRAME_TYPEDEF *gprs_recv_frame, uint16_t recv_counts)
{
  int8_t result = 1;

  gsm_put_hex((uint8_t *)gprs_recv_frame, recv_counts);
  /* Large end into a small side */
  gprs_recv_frame->length = __REV16(gprs_recv_frame->length);
  switch (gprs_recv_frame->cmd)
  {
    //list telephone
    case 0x86: {

      if (gprs_recv_frame->data.list_telephone.type == 0x01 && recv_counts == 4 + gprs_recv_frame->length)
      {
        send_gprs_mail(ALARM_TYPE_GPRS_LIST_TELEPHONE, 0, gprs_recv_frame->order,RT_NULL);
      }
      else
      {
        rt_kprintf("recv frame 0x%02x is error!", gprs_recv_frame->cmd);
      }
      break;
    };
    //set telephone
    case 0x87: {

      if (recv_counts == 4 + gprs_recv_frame->length)
      {
        process_gprs_set_telephone(&gprs_recv_frame->data.set_telephone);
        send_gprs_mail(ALARM_TYPE_GPRS_SET_TELEPHONE_SUCCESS, 0, gprs_recv_frame->order,RT_NULL);
      }
      else
      {
        send_gprs_mail(ALARM_TYPE_GPRS_SET_TELEPHONE_FAILURE, 0, gprs_recv_frame->order,RT_NULL);
      }
      break;
    };
    //list rfid key
    case 0x88: {
      if (gprs_recv_frame->data.list_rfid_key.type == 0x01)
      {
        send_gprs_mail(ALARM_TYPE_GPRS_LIST_RFID_KEY, 0, gprs_recv_frame->order,RT_NULL);
      }
      else
      {
        rt_kprintf("recv frame 0x%02x is error!", gprs_recv_frame->cmd);
      }
      break;
    };
    //set rfid key
    case 0x89: {

      if (recv_counts == 4 + gprs_recv_frame->length)
      {
        process_gprs_set_rfid_key(&gprs_recv_frame->data.set_rfid_key);
        send_gprs_mail(ALARM_TYPE_GPRS_SET_RFID_KEY_SUCCESS, 0, gprs_recv_frame->order,RT_NULL);
      }
      else
      {
        send_gprs_mail(ALARM_TYPE_GPRS_SET_RFID_KEY_FAILURE, 0, gprs_recv_frame->order,RT_NULL);
      }
      break;
    };
    //list user parameters
    case 0x8A: {
      if (gprs_recv_frame->data.list_user_parameters.type == 0x01)
      {
        send_gprs_mail(ALARM_TYPE_GPRS_LIST_USER_PARAMETERS, 0, gprs_recv_frame->order,RT_NULL);
      }
      else
      {
        rt_kprintf("recv frame 0x%02x is error!", gprs_recv_frame->cmd);
      }
      break;
    };
    //set user parameters
    case 0x8B: {

      if (recv_counts == 4 + gprs_recv_frame->length)
      {
        process_gprs_set_user_parameters(&gprs_recv_frame->data.set_user_parameters);
        send_gprs_mail(ALARM_TYPE_GPRS_SET_USER_PARAMETERS_SUCCESS, 0, gprs_recv_frame->order,RT_NULL);
      }
      else
      {
        send_gprs_mail(ALARM_TYPE_GPRS_SET_USER_PARAMETERS_FAILURE, 0, gprs_recv_frame->order,RT_NULL);
      }
      break;
    };
    //set key0
    case 0x98: {

      if (recv_counts == 4 + gprs_recv_frame->length)
      {
        process_gprs_set_key0(&gprs_recv_frame->data.set_key0);
        send_gprs_mail(ALARM_TYPE_GPRS_SET_KEY0_SUCCESS, 0, gprs_recv_frame->order,RT_NULL);
      }
      else
      {
        send_gprs_mail(ALARM_TYPE_GPRS_SET_KEY0_FAILURE, 0, gprs_recv_frame->order,RT_NULL);
      }
      break;
    };
    //set time
    case 0x95: {

      if (recv_counts == 4 + gprs_recv_frame->length)
      {
        process_gprs_set_time(&gprs_recv_frame->data.set_time);
        send_gprs_mail(ALARM_TYPE_GPRS_SET_TIME_SUCCESS, 0, gprs_recv_frame->order,RT_NULL);
      }
      else
      {
        send_gprs_mail(ALARM_TYPE_GPRS_SET_TIME_FAILURE, 0, gprs_recv_frame->order,RT_NULL);
      }
      break;
    };
    case 0x8e:
    {
    	//gprs_send_heart();
    	send_picture_data();
    	//send_gprs_mail(ALARM_TYPE_GPRS_SEND_PIC_DATA,0, 0,RT_NULL);
			break;
    }
    default : {
      if (recv_counts == 4 &&
          gprs_recv_frame->length == 0)
      {
        result = 1;
      }
      else
      {
        result = -1;
      }
      break;
    };
  }
  return result;
}
