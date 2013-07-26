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
#include "gpio_pin.h"
#include "gpio_pwm.h"
#include "gsm.h"
#include "gsm_usart.h"
#include "untils.h"
#include "des.h"
#include <string.h>
#include "battery.h"
#include <rtc.h>
#include "mms_dev.h"
#include "aip.h"


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

rt_mq_t gprs_mq = RT_NULL;


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
  
  while (1)
  {
    /* process mail */
    memset(&gprs_mail_buf, 0, sizeof(GPRS_MAIL_TYPEDEF));
    result = rt_mq_recv(gprs_mq, &gprs_mail_buf, sizeof(GPRS_MAIL_TYPEDEF), 6000);
    /* mail receive ok */
    if (result == RT_EOK)
    {
      rt_kprintf("receive gprs mail < time: %d alarm_type: %s >\n", gprs_mail_buf.time, alarm_help_map[gprs_mail_buf.alarm_type]);

      //send gprs data
      rt_mutex_take(mutex_gsm_mail_sequence,RT_WAITING_FOREVER);

      if (send_gprs_frame(gprs_mail_buf.alarm_type, gprs_mail_buf.time, gprs_mail_buf.order, gprs_mail_buf.user) == 1)
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
      rt_mutex_release(mutex_gsm_mail_sequence);
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

/*static void get_dec_to_bcd_time(uint8_t date[],time_t time)
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
}*/
static void get_pic_size(const char* name ,rt_uint32_t *size)
{
	struct stat status;

	if(stat(name,&status) >= 0)
	{
		*size = status.st_size;
	}
	else
	{
		rt_kprintf("get file info fail \n\n");
		*size = 0;
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
rt_int8_t process_gprs_pic_arg(GPRS_SEND_PIC_ASK *pic_info,time_t time,void* user)
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
    default:
      {
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
  get_pic_size(PIC_NAME,&size);					//get picture file size
  if(size == 0)
  {
    return -1;
  }
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
  return 1;
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
  uint32_t process_length = 0;
  GPRS_SEND_FRAME_TYPEDEF *gprs_send_frame = RT_NULL;
  GPRS_RECV_FRAME_TYPEDEF *gprs_recv_frame = RT_NULL;
  rt_device_t device = RT_NULL;
  int8_t send_result = -1;
  rt_size_t recv_counts;
  GSM_MAIL_CMD_DATA gsm_mail_cmd_data;
  int8_t recv_times = 0;

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

  gsm_mail_cmd_data.cipsend.length = process_length;
  gsm_mail_cmd_data.cipsend.buf = process_buf;
  send_result = gsm_command(AT_CIPSEND, 50, &gsm_mail_cmd_data);
  if (send_result == AT_RESPONSE_OK)
  {
    recv_times = 0;
    while (recv_times < 10)
    {
      rt_thread_delay(100);
      memset(process_buf, 0, 512);
      recv_counts = gsm_recv_frame(process_buf);
      if (recv_counts && strstr((char *)process_buf, "+RECEIVE"))
      {
        sscanf((char *)process_buf, "+RECEIVE,0,%d:", &process_length);
        recv_counts = rt_device_read(device, 0, process_buf, process_length);
        if (recv_counts == process_length)
        {
          recv_gprs_frame((GPRS_RECV_FRAME_TYPEDEF *)process_buf, process_length);
        }
        break;
      }
      recv_times++;
    }
  }

rt_free(process_buf);
  rt_free(gprs_send_frame);
  rt_free(gprs_recv_frame);
  return send_result;
}

static void get_page_pic_data(int file_id,GPRS_SEND_PIC *pic,rt_uint8_t cur_page)
{		
	rt_uint32_t				offset = 0;

	offset = cur_page * PIC_PER_PAGE_SIZE;		//read data pos
	if(lseek(file_id,offset,SEEK_SET) == -1)  //file fixed position
	{
		return;
	}
	read(file_id,(void*)(pic->data+5),pic->length-1);
	rt_kprintf("get this of data length %d\n",pic->length);
	*((pic->data)+0) = (rt_uint8_t)((pic->length) >> 8 & 0xff);
	*((pic->data)+1) = (rt_uint8_t)(pic->length & 0xff);
	*((pic->data)+2) = pic->cmd;
	*((pic->data)+3) = pic->order;
	*((pic->data)+4) = cur_page;
}

/*0 ok  1:fail*/
rt_uint8_t send_page_pic_data(int file_id,GPRS_SEND_PIC* pic,rt_uint8_t page,GSM_MAIL_TYPEDEF* gsm_mail)
{
	rt_uint32_t	pic_size;
	rt_uint32_t	cur_size;
	
	get_pic_size(PIC_NAME,&pic_size);					//get picture size
	cur_size = ((page+1)*PIC_PER_PAGE_SIZE);	//calculate cur send pos
	if(cur_size < pic_size)
	{
		pic->length = PIC_PER_PAGE_SIZE+1; 			//data length include curpage and data
		gsm_mail->mail_data.gprs.has_response = 0; 	
		rt_kprintf("no data recv\n");
	}
	else
	{  
		pic->length = pic_size % PIC_PER_PAGE_SIZE+1; 	//send data length
		gsm_mail->mail_data.gprs.has_response = 1;			//last page data need recv response
		rt_kprintf("have data recv");
	}
	get_page_pic_data(file_id,pic,page);
	gsm_mail->mail_data.gprs.request = pic->data;
	gsm_mail->mail_data.gprs.request_length = pic->length+4;
	if (mq_gsm != NULL)
	{
		rt_mq_send(mq_gsm,gsm_mail, sizeof(GSM_MAIL_TYPEDEF));
	}
	else
	{
		rt_kprintf("mq_gsm is NUll picture uploading\n");
	}
	rt_sem_take(gsm_mail->result_sem, RT_WAITING_FOREVER);
	if(*(gsm_mail->result) == 1)
	{
		rt_kprintf("send picture data ok\n");
		return 0;
	}
	else
	{
		rt_kprintf("send picture data fail\n");
		return 1;
	}
}

rt_uint8_t send_last_page_data(int file_id,GPRS_SEND_PIC* pic,GSM_MAIL_TYPEDEF* gsm_mail)
{
	rt_uint32_t pic_size;
	rt_uint8_t	last_page_pos;
	rt_uint8_t	has_response = gsm_mail->mail_data.gprs.has_response;
	
	get_pic_size(PIC_NAME,&pic_size);
	if((pic_size % PIC_PER_PAGE_SIZE) != 0)
	{
		last_page_pos = pic_size / PIC_PER_PAGE_SIZE;
		rt_kprintf("\n\nlast page data lentg %d\n",(pic_size / PIC_PER_PAGE_SIZE));
	}
	else
	{
		last_page_pos = (pic_size / PIC_PER_PAGE_SIZE)-1;
		rt_kprintf("\n\nlast page data lentg %d\n",(pic_size / PIC_PER_PAGE_SIZE)-1);	
	}
	gsm_mail->mail_data.gprs.has_response = 1; 	
	if(send_page_pic_data(file_id,pic,last_page_pos,gsm_mail) == 1)
	{
		gsm_mail->mail_data.gprs.has_response = has_response; 
		return 1;//send a page data fail
	}
	gsm_mail->mail_data.gprs.has_response = has_response; 	
	return 0;//send a page data ok
	
}
rt_int8_t send_picture_data(void)
{
	extern volatile rt_uint32_t debug_delay1;
	int               pic_file_id;			
	rt_uint8_t        page_sum;
	rt_uint8_t        page_error = 0;
	rt_uint8_t        resend_num = 0;
	rt_int8_t         send_result = 1;
	rt_uint8_t				send_last_cnt = 0;
	rt_uint16_t				delay_time = 100;
	rt_uint16_t       recv_counts;
	rt_uint32_t       pic_size;
	rt_uint32_t       send_length;
	GPRS_SEND_PIC     pic_data;
	GSM_MAIL_TYPEDEF 	gsm_mail_buf;
	GPRS_RECV_FRAME_TYPEDEF gprs_recv_frame;

	delay_time = 100;
	/* send file deal */
	get_pic_size(PIC_NAME,&pic_size);
	if(pic_size == 0)
	{
		return -1;
	}
	pic_file_id = open(PIC_NAME,O_RDWR,0);
	if(pic_file_id < 0)
	{
		rt_kprintf("file open fail\n");
		return -1;
	}
	/* picture data init */
	pic_data.data = rt_malloc(PIC_PER_PAGE_SIZE+5);
	if(pic_data.data == RT_NULL)
	{
		rt_kprintf("pic data buffer malloc fial\n");
		return -1;
	}
	rt_memset(pic_data.data,0,PIC_PER_PAGE_SIZE+5);
	pic_data.cmd = 0x0F;
	pic_data.cur_page  = 0;
	pic_data.order = gprs_order;
	
	/*Get How many packages data*/
	page_sum = pic_size / PIC_PER_PAGE_SIZE;
	if((pic_size % PIC_PER_PAGE_SIZE) != 0)							
	{
		page_sum++;
	}
	rt_kprintf("page_sum = %d\n",page_sum);
	gsm_mail_buf.result_sem = rt_sem_create("g_pic", 0, RT_IPC_FLAG_FIFO);
	if(gsm_mail_buf.result_sem == RT_NULL)
	{
		
		rt_free(pic_data.data);
		rt_sem_delete(gsm_mail_buf.result_sem);
		close(pic_file_id);		
		return -1;
	}
	gsm_mail_buf.send_mode = GSM_MODE_GPRS;
	gsm_mail_buf.result = &send_result;
	gsm_mail_buf.mail_data.gprs.response = (uint8_t *)&gprs_recv_frame;
	gsm_mail_buf.mail_data.gprs.response_length = &recv_counts;
	rt_thread_delay(200); 
	while(page_sum--)
	{
		if(page_sum > 0)//not last page data
		{
			pic_data.length = PIC_PER_PAGE_SIZE+1; //data length include curpage and data
			gsm_mail_buf.mail_data.gprs.has_response = 0;
		}
		else
		{
			pic_data.length = pic_size % PIC_PER_PAGE_SIZE+1; //send data length
			gsm_mail_buf.mail_data.gprs.has_response = 1;			//last page data need recv response
		}
		send_length = pic_data.length+4;
		get_page_pic_data(pic_file_id,&pic_data,pic_data.cur_page);
		pic_data.cur_page++;
		//gsm_put_hex(pic_data.data, send_length);
		/*padding send data sturct*/
		gsm_mail_buf.mail_data.gprs.request = pic_data.data;
		gsm_mail_buf.mail_data.gprs.request_length = send_length;
		/*padding recv data struct*/
		if(gsm_mail_buf.mail_data.gprs.has_response == 1)
		{
			gsm_mail_buf.mail_data.gprs.response = (uint8_t *)&gprs_recv_frame;
			gsm_mail_buf.mail_data.gprs.response_length = &recv_counts;
		}
		if (mq_gsm != NULL)
		{
			rt_mq_send(mq_gsm, &gsm_mail_buf, sizeof(GSM_MAIL_TYPEDEF));
		}
		else
		{
			rt_kprintf("mq_gsm is NUll picture uploading\n");
			/*send system unusual*/
			send_result = -1;
			goto EXIT_PICTURE_SEND;		
		}
		rt_sem_take(gsm_mail_buf.result_sem, RT_WAITING_FOREVER);
		if(send_result == 1)//send data ok
		{
			rt_kprintf("send picture data ok\n");
		}
		else								//send data fail
		{
			rt_kprintf("\n\nsend picture data fail\n");
			page_error++;
			if(page_error >= 3)
			{
				send_result = -1;
				goto EXIT_PICTURE_SEND;
			}
			page_sum++;
			pic_data.cur_page--;
		}
		rt_thread_delay(delay_time);										//wait server Processed data		
		if((pic_data.cur_page % 10) == 0 )
		{
CHECK_SERVER_SEND:
			if(send_last_page_data(pic_file_id,&pic_data,&gsm_mail_buf) == 1)
			{	
				/*send 10 page data all fail*/
				if(send_last_cnt < 10)
				{
					/* send last page data fail */
					page_sum++;
					pic_data.cur_page--;
					rt_thread_delay(200);
					send_last_cnt++;
					send_result = -1;
					goto CHECK_SERVER_SEND;
				}
				else
				{
					send_result = -1;
					goto EXIT_PICTURE_SEND;//send picture fail
				}
			}
			else//send data ok recv server 	Response data
			{
				recv_gprs_frame(&gprs_recv_frame, recv_counts);
				send_last_cnt = 0;
				rt_kprintf("recv_counts = %d\n",recv_counts);
			}
		}
		if(gsm_mail_buf.mail_data.gprs.has_response == 1)	//last frame picture data need recv answer
		{
			rt_int8_t result;
			rt_int8_t rsend_cnt = 0;

RSEND_LOST_PAGE:
			result = recv_gprs_frame(&gprs_recv_frame, recv_counts);
			switch(result)
			{
				case 1:	
				{
					send_result = 1;	
					break;
				}
				case -2:
				{
					rt_uint8_t  error = 0;
					rt_uint8_t	cur_page = 0;

					while(cur_page != gprs_recv_frame.data.pic_send_result.resend_num)
					{
						rt_uint8_t	result;
						result = send_page_pic_data(pic_file_id,	//resend lost data packet
																				&pic_data,
																				gprs_recv_frame.data.pic_send_result.data[cur_page++],
																				&gsm_mail_buf);	
						if((result == 1) || ((cur_page % 10 == 0)))//rsend fail
						{
							if(send_last_page_data(pic_file_id,&pic_data,&gsm_mail_buf) == 1)
							{
								rt_thread_delay(200); 	
							}
							else
							{
								recv_gprs_frame(&gprs_recv_frame, recv_counts);
								cur_page = 0;
							}
						}
						rt_thread_delay(200);		
						
					}
					if(recv_gprs_frame(&gprs_recv_frame, recv_counts) == 1)
					{
						send_result = 1;
						break;
					}
					/* send last page fail */
LASTPAGELOOP:	
					if(send_last_page_data(pic_file_id,&pic_data,&gsm_mail_buf) == 1)
					{
						error++;
						if(error < 3)
						{
							goto LASTPAGELOOP;
						}
						send_result = 1;
					}
					else
					{
						if(rsend_cnt++ < 10)
						{
							goto RSEND_LOST_PAGE;
						}
					}
					/* send last page ok */
					break;
				}
				case -3:
				{
					rt_free(pic_data.data);
					rt_sem_delete(gsm_mail_buf.result_sem);
					close(pic_file_id);
					return -1;
					//break;
				}
				case -4:	//receive unusual	send sotp
				{
					send_result = -1;
					goto EXIT_PICTURE_SEND;
					break;
				}
				default:
				{
					/*send last page picture data*/
					send_last_page_data(pic_file_id,&pic_data,&gsm_mail_buf);	
					page_sum++;
					pic_data.cur_page--;
					resend_num++;
					if(resend_num > 3)
					{
						send_result = -1;
						goto EXIT_PICTURE_SEND;
					}
					break;
				}
			}
		}
		rt_kprintf("loop_cnt = %d\n",page_sum);
		rt_memset(pic_data.data,0,PIC_PER_PAGE_SIZE+5);
		rt_memset(&gprs_recv_frame.data.pic_send_result,0,sizeof(gprs_recv_frame.data.pic_send_result));
	}
EXIT_PICTURE_SEND:
	close(pic_file_id);
	rt_thread_delay(300);
	rt_sem_delete(gsm_mail_buf.result_sem);
	rt_free(pic_data.data);
	//send_gprs_mail(ALARM_TYPE_GPRS_UPLOAD_PIC,0, 0,(void *)ALARM_TYPE_LOCK_TEMPERATURE);//test
	if(send_result)
	{
		rt_kprintf("发送成功\n");
	}
	else
	{
		rt_kprintf("发送失败\n");
	}
	return send_result;
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
  uint32_t recv_counts = 0;

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

      send_result = process_gprs_pic_arg(&(gprs_send_frame->data.picture),time,user);
      if(send_result == -1)
      {
				goto	quitgprsdatasend;
      }
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
    case ALARM_TYPE_GPRS_SET_HTTP_SUCCESS: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x19;
      gprs_send_frame->order = order;

      gprs_send_frame->data.set_http.result = 1;
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SET_HTTP_FAILURE: {

      gprs_send_frame->length = 1;
      gprs_send_frame->cmd = 0x19;
      gprs_send_frame->order = order;

      gprs_send_frame->data.set_http.result = 0;
      gsm_mail_buf.mail_data.gprs.has_response = 0;
      break;
    };
    case ALARM_TYPE_GPRS_SEND_PIC_DATA:
    {
    	extern rt_mutex_t	pic_file_mutex;
    	
    	rt_mutex_take(pic_file_mutex,RT_WAITING_FOREVER);
    	send_result = send_picture_data();
    	rt_mutex_release(pic_file_mutex);
    	gprs_order++;
    	goto	quitgprsdatasend;
    	//break;
    }
		case ALARM_TYPE_GPRS_SLEEP:
		{
			gprs_send_frame->length = 1;
      		gprs_send_frame->cmd = 0x0C;
      		gprs_send_frame->order = order;
      		gprs_send_frame->data.sleep_ack.result = 1;
      		gsm_mail_buf.mail_data.gprs.has_response = 0;
      		break;
		}
		case ALARM_TYPE_GPRS_WAKE_UP:
		{
			gprs_send_frame->length = 1;
      		gprs_send_frame->cmd = 0x0d;
      		gprs_send_frame->order = order;
      		gprs_send_frame->data.wake_up_ack.result= 1;
      		gsm_mail_buf.mail_data.gprs.has_response = 0;
			break;
		}
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
    case ALARM_TYPE_GPRS_SET_HTTP_SUCCESS: {

      *process_buf_bk++ = gprs_send_frame->data.set_http.result;
      process_length += 1;

      break;
    };
    case ALARM_TYPE_GPRS_SET_HTTP_FAILURE: {

      *process_buf_bk++ = gprs_send_frame->data.set_http.result;
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
    case ALARM_TYPE_GPRS_SLEEP://sleep mode
    {
    	*process_buf_bk++ = gprs_send_frame->data.sleep_ack.result;
      process_length += 1;
			break;
    }
    case ALARM_TYPE_GPRS_WAKE_UP:
    {
			*process_buf_bk++ = gprs_send_frame->data.wake_up_ack.result;
      process_length += 1;
			break;
    }
    default : {
      break;
    };
  }

  //send frame
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
      gsm_put_hex(gsm_mail_buf.mail_data.gprs.request, gsm_mail_buf.mail_data.gprs.request_length);
      if (gsm_mail_buf.mail_data.gprs.has_response == 1)
      {
        if (send_result == AT_RESPONSE_OK)
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
  rt_free(gprs_recv_frame);
  rt_free(process_buf);
quitgprsdatasend:  
  rt_free(gprs_send_frame);
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
void process_gprs_set_http(GPRS_SET_HTTP *gprs_set_http, uint8_t length)
{
  if(length >= 100)
  {
    length = 99;
  }
  aip_bin_http_address_length = length;
  memset(aip_bin_http_address, 0, sizeof(aip_bin_http_address));
  memcpy(aip_bin_http_address, gprs_set_http->http_address, length);
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
    //set http
    case 0x99: {

      if (recv_counts == 4 + gprs_recv_frame->length)
      {
        process_gprs_set_http(&gprs_recv_frame->data.set_http, gprs_recv_frame->length);
        send_gprs_mail(ALARM_TYPE_GPRS_SET_HTTP_SUCCESS, 0, gprs_recv_frame->order,RT_NULL);
        send_aip_mail(0,0);
      }
      else
      {
        send_gprs_mail(ALARM_TYPE_GPRS_SET_HTTP_FAILURE, 0, gprs_recv_frame->order,RT_NULL);
      }
      break;
    };
      
    case 0x8E:
    {
    	//gprs_send_heart();
    	//send_picture_data();
    	send_gprs_mail(ALARM_TYPE_GPRS_SEND_PIC_DATA,0, 0,RT_NULL);
			break;
    }
    case 0x8F:
    {
			rt_uint8_t i;

			/* RecvResponse data validity analyse */
			rt_kprintf("picture send data recv respond \n");
			rt_kprintf("%d\n",gprs_recv_frame->data.pic_send_result.resend_num);
			for(i = 0; i < gprs_recv_frame->data.pic_send_result.resend_num;i++)
			{
			  rt_kprintf("%d ",gprs_recv_frame->data.pic_send_result.data[i]);
			  if(i%10 == 0)
			  {
							rt_kprintf("\n");
			  }
			  /*if(i > 0)
			  {
			  	if(gprs_recv_frame->data.pic_send_result.data[i-1] < gprs_recv_frame->data.pic_send_result.data[i])
			  	{
						gprs_recv_frame->data.pic_send_result.resend_num = i;
			  	}
			  }*/
			}
			switch(gprs_recv_frame->data.pic_send_result.result)
				{
			      case 0:					//send ok
			        {
			          result = 1;
			          rt_kprintf("gprs data send ok\n");
			          break;
			        }
			      case 1:					//receive imperfect
			        {
			          result = -2;
			          rt_kprintf("gprs data receive imperfect\n");
			          break;
			        }case 2:				//receive	unusual resend
			        {
			          result = -3;
			          rt_kprintf("gprs data receive	unusual resend\n");
			          break;
			        }
			      case 3:					//receive unusual	send sotp
			        {
			          result = -4;
			          rt_kprintf("gprs data receive unusual	send sotp\n");
			          break;
			        }
			      default:
			        {
			          result = -1;
			          rt_kprintf("gprs data error\n");
			        }
				}
			break;
    }
    case 0x8c:
    {
    	rt_kprintf("sleep \n");
    	send_gprs_mail(ALARM_TYPE_GPRS_SLEEP, 0, gprs_recv_frame->order,RT_NULL);
    	machine_status_deal(RT_FALSE,RT_EVENT_FLAG_OR|RT_EVENT_FLAG_CLEAR,RT_WAITING_NO);
    	rt_thread_delay(200);//prior send gprs data
    	lock_output(GATE_UNLOCK);
			break;
    }
    case 0x8d:
    {
    	rt_kprintf("wake up\n");
    	send_gprs_mail(ALARM_TYPE_GPRS_WAKE_UP, 0, gprs_recv_frame->order,RT_NULL);
    	machine_status_deal(RT_TRUE,0,0);
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
