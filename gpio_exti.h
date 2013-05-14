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

void rt_hw_key1_register(void);
void rt_hw_key2_register(void);

#endif
