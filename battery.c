/*********************************************************************
 * Filename:      battery.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-06-08 11:17:29
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "battery.h"
#include "gpio_adc.h"
#include "gpio_exti.h"
#include "gpio_pin.h"
#include "local.h"
#include "sms.h"
#include "gprs.h"


static uint32_t battery_status = 0;

#define SAMPLE_COUNTS 10

#define SAMPLE_VALUE_BATTERY_REMAIN_5P 0xCC
#define SAMPLE_VALUE_BATTERY_REMAIN_20P 0x333
#define SAMPLE_VALUE_BATTERY_REMAIN_50P 0x7FF

#define BATTERY_ADC_VALUE_RANGE					750
#define BATTERY_WORKING_VOLTAGE					15
void battery_get_data(Battery_Data* data)
{
	rt_device_t status_dev;
	rt_device_t adc_dev;
	rt_uint16_t	range;
	float scale = 0;
	status_dev = rt_device_find(DEVICE_NAME_BATTERY_SWITCH);
	if(RT_NULL != status_dev)
	{
		rt_device_read(status_dev,0,&(data->status),1);
	}
	adc_dev = rt_device_find(DEVICE_NAME_BATTERY_ADC);
	if(RT_NULL != adc_dev)
	{
		bat_enable();

		//rt_device_read(adc_dev,0,&(data->adc_value),1);
		rt_device_control(adc_dev, RT_DEVICE_CTRL_GET_CONVERT_VALUE, (void *)&(data->adc_value));

		bat_disable();

		//To compute work time
		range = 0xfff - data->adc_value;
		scale = (float)BATTERY_WORKING_VOLTAGE/(float)BATTERY_ADC_VALUE_RANGE;
		data->work_time = BATTERY_WORKING_VOLTAGE - range*scale;//The remaining work time
		if(data->work_time < 0)
		{
			data->work_time = 0;
		}
	}
}
void battery_check_process_thread_entry(void *parameters)
{
  uint8_t dat;
  int8_t sample_counts;
  uint32_t sample_value;

  while (1)
  {
    dat = gpio_pin_input(DEVICE_NAME_BATTERY_SWITCH);
    if (dat == POWER_BATTERY)
    {
      sample_counts = SAMPLE_COUNTS;
      sample_value = 0;
      bat_enable();// start sample
      while (sample_counts-- > 0)
      {
        rt_thread_delay(50);// 50 ticks per sample
        sample_value += bat_get_value();
      }
      bat_disable();// stop sample

      sample_value /= SAMPLE_COUNTS;

      if (sample_value < SAMPLE_VALUE_BATTERY_REMAIN_5P)//battery < 5%
      {
        if (battery_status & BATTERY_STATUS_REMAIN_5P)
        {
          // already < 5%
        }
        else
        {
          // first < 5%
          battery_status |= BATTERY_STATUS_REMAIN_5P;
          send_local_mail(ALARM_TYPE_BATTERY_REMAIN_5P, 0);
        }
      }
      else // battery > 5%
      {
        battery_status &= ~BATTERY_STATUS_REMAIN_5P;
        if (sample_value < SAMPLE_VALUE_BATTERY_REMAIN_20P)//battery < 20%
        {
          if (battery_status & BATTERY_STATUS_REMAIN_20P)
          {
            // already < 20%
          }
          else
          {
            // first < 20%
            battery_status |= BATTERY_STATUS_REMAIN_20P;
            send_sms_mail(ALARM_TYPE_BATTERY_REMAIN_20P, 0);
            send_gprs_mail(ALARM_TYPE_BATTERY_REMAIN_20P, 0, 0,RT_NULL);
          }
        }
        else // battery > 20%
        {
          battery_status &= ~BATTERY_STATUS_REMAIN_20P;
          if (sample_value < SAMPLE_VALUE_BATTERY_REMAIN_50P)//battery < 50%
          {
            if (battery_status & BATTERY_STATUS_REMAIN_50P)
            {
              // already < 50%
            }
            else
            {
              // first < 50%
              battery_status |= BATTERY_STATUS_REMAIN_50P;
              send_sms_mail(ALARM_TYPE_BATTERY_REMAIN_50P, 0);
              send_gprs_mail(ALARM_TYPE_BATTERY_REMAIN_50P, 0, 0,RT_NULL);
            }
          }
          else // battery > 50%
          {
            battery_status &= ~BATTERY_STATUS_REMAIN_50P;
          }

        }
      }
    }
    else // POWER EXTERNAL
    {

    }
    rt_thread_delay(6000);// 60 seconds per check
  }
}
