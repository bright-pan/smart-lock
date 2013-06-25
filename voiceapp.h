#ifndef __VOICEAPP_H__
#define __VOICEAPP_H__
#include "gpio_pwm.h"
#include "gpio_pin.h"
extern rt_sem_t voice_start_sem;

void voice_thread_init(void);

#endif 


