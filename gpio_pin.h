/*********************************************************************
 * Filename:      gpio_pin.h
 *
 * Description:    
 *
 * Author:        wangzw <wangzw@yuettak.com>
 * Created at:    2013-04-22
 *                
 * Modify:
 *
 * 2013-04-25 Bright Pan <loststriker@gmail.com>
 * 2013-04-27 Bright Pan <loststriker@gmail.com>
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#ifndef _GPIO_PIN_H_
#define _GPIO_PIN_H_

#include <rthw.h>
#include <rtthread.h>
#include "stm32f10x.h"
#include "gpio.h"

void rt_hw_led1_register(void);
void rt_hw_glint_light_register(void);

#endif
