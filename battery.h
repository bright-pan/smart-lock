/*********************************************************************
 * Filename:      battery.h
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-06-08 11:18:50
 *
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#ifndef _BATTERY_H_
#define _BATTERY_H_

#include <rtthread.h>
#include <rthw.h>
#include <stm32f10x.h>

#define BATTERY_STATUS_REMAIN_50P 0x01
#define BATTERY_STATUS_REMAIN_20P 0x02
#define BATTERY_STATUS_REMAIN_5P 0x04

#define POWER_BATTERY 0
#define POWER_EXTERNAL 1

void battery_check_process_thread_entry(void *parameters);

#endif
