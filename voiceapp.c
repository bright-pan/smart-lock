#include "voiceapp.h"


rt_sem_t voice_start_sem = RT_NULL;

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

	rt_device_write(dev,0, (void*)&status,1);

}
void voice_thread_entry(void *arg)
{
	rt_err_t result;
	
	while(1)
	{
		result =  rt_sem_take(voice_start_sem,500);
		if(RT_EOK == result)
		{
	  	Amplifier_switch(1);
	  	
			voice_play(3);
		}
		else if(result == -RT_ETIMEOUT)
		{
			Amplifier_switch(0);
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
}



