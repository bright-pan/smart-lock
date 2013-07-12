#include "voiceapp.h"
#include "gpio_exti.h"


rt_sem_t    voice_start_sem = RT_NULL;
rt_event_t	call_event;

void voice_play(rt_uint8_t counts)
{
  rt_device_t device = RT_NULL;
  rt_device_t reset_device = RT_NULL;
  rt_int16_t dat = 0;
  
  reset_device = rt_device_find(DEVICE_NAME_VOICE_RESET);
  dat = 1;
  rt_device_control(reset_device, RT_DEVICE_CTRL_SET_PULSE_COUNTS, &dat);
  device = rt_device_find(DEVICE_NAME_VOICE_DATA);
  rt_device_control(device, RT_DEVICE_CTRL_SET_PULSE_COUNTS, (void *)&counts);
  device = rt_device_find(DEVICE_NAME_VOICE_SWITCH);
  dat = 1;
  rt_device_write(device, 0, &dat, 0);
  device = rt_device_find(DEVICE_NAME_VOICE_AMP);
  dat = 1;
  rt_device_write(device, 0, &dat, 0);
  rt_device_control(reset_device, RT_DEVICE_CTRL_SEND_PULSE, (void *)0);

}
void Amplifier_switch(rt_uint8_t status)
{
	rt_device_t dev;

	dev = rt_device_find(DEVICE_NAME_VOICE_AMP);

	if(dev != RT_NULL)
	{
		rt_device_write(dev,0, (void*)&status,1);
	}
}
void sound_channel_switch(rt_uint8_t status)
{
	rt_device_t dev;

	dev = rt_device_find(DEVICE_NAME_VOICE_SWITCH);
	
	if(dev != RT_NULL)
	{
		rt_device_write(dev,0, (void*)&status,1);
	}
}
rt_int8_t get_call_status(void)
{
	rt_uint8_t	status;
	rt_device_t dev;

	dev = rt_device_find(DEVICE_NAME_GSM_RING);

	if(dev != RT_NULL)
	{
		rt_device_read(dev,0,&status,1);

		//rt_kprintf("IR status %d\n",status);
		return status;
	}
	
	return -1;
}
void voice_thread_entry(void *arg)
{
	rt_err_t    result;
	rt_uint32_t event;
	
	while(1)
	{
		result =  rt_sem_take(voice_start_sem,100);
		if(RT_EOK == result)
		{
			result = rt_event_recv(call_event,CALL_EVENT_START,RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,RT_WAITING_NO,&event);
			if(result == RT_EOK)
			{
				rt_kprintf("open amp power\n");
				sound_channel_switch(0);
				Amplifier_switch(1);
			}
			else
			{
				sound_channel_switch(1);
				Amplifier_switch(1);
				voice_play(3);
			}
		}
		else if(result == -RT_ETIMEOUT)
		{
			if( 1 == get_call_status())
			{
				//rt_kprintf("close amp power\n");
				//Amplifier_switch(0);
			}
		}
	}
}
void voice_thread_init(void)
{
	rt_thread_t thread_id;

	thread_id = rt_thread_create("voic_t",voice_thread_entry,RT_NULL,256,103,30);
	if(RT_NULL == thread_id )
	{
		rt_kprintf("graph thread create fail\n");
		
		return ;
	}
	rt_thread_startup(thread_id);

	voice_start_sem = rt_sem_create("v_sem",0,RT_IPC_FLAG_FIFO);
	if(RT_NULL == voice_start_sem)
	{
		rt_kprintf("\"v_sem\" create fail \n");
	}
	call_event = rt_event_create("call_p",RT_IPC_FLAG_FIFO);
	if(RT_NULL == call_event)
	{
		rt_kprintf("\"call_p\" create fail\n");
	}
}



