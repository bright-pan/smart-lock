#ifndef __VOICEAPP_H__
#define __VOICEAPP_H__
#include "gpio_pwm.h"
#include "gpio_pin.h"

#define     CALL_EVENT_START			(0X01<<0)


extern rt_sem_t   voice_start_sem;
extern rt_event_t	call_event;

void voice_thread_init(void);

#endif 


