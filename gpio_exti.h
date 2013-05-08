/*********************************************************************
 * Filename:      gpio_exti.h
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

#ifndef _GPIO_EXTI_H_
#define _GPIO_EXTI_H_

#include <rthw.h>
#include <rtthread.h>
#include <stm32f10x.h>
#include "gpio.h"
#include "alarm.h"

#define DEVICE_NAME_LOCK_SHELL "lk_shell"// length <= 8
#define DEVICE_NAME_LOCK_TEMPERATRUE "lk_temp"
#define DEVICE_NAME_LOCK_GATE "lk_gate"
#define DEVICE_NAME_LOCK_PLUGIN "lk_plug"
#define DEVICE_NAME_CAMERA_COVER "cm_cover"

void rt_hw_lk_shell_register(void);
void rt_hw_lk_temp_register(void);
void rt_hw_key2_register(void);

#endif
