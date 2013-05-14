/*********************************************************************
 * Filename:      gpio_pwm.h
 * 
 * Description:    
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-04-27
 *                
 * Change Log:
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#ifndef _GPIO_PWM_H_
#define _GPIO_PWM_H_

#include <rthw.h>
#include <rtthread.h>
#include <stm32f10x.h>
#include "gpio.h"

#define RT_DEVICE_CTRL_SEND_PULSE       0x14    /* enable receive irq */
#define RT_DEVICE_CTRL_SET_PULSE_COUNTS 0x15    /* disable receive irq */
#define RT_DEVICE_FLAG_PWM_TX           0x1000 /* flag mask for gpio pwm mode */

void rt_hw_pwm1_register(void);

#endif
