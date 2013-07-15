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
#define DEVICE_NAME_GATE_TEMPERATRUE "gt_temp"
#define DEVICE_NAME_LOCK_GATE "lk_gate"

#define DEVICE_NAME_CAMERA_COVER "cm_cover"
#define DEVICE_NAME_CAMERA_IRDASENSOR "cm_irda"

#define DEVICE_NAME_GSM_RING "g_ring"

//#define DEVICE_NAME_MOTOR_STATUS "mt_stat"

#define DEVICE_NAME_RFID_KEY_DETECT "rf_kdet"

#define DEVICE_NAME_BATTERY_SWITCH "bat_sw" /* BATTERY DEVICE NAME */

void rt_hw_lock_shell_register(void);
void rt_hw_lock_temperature_register(void);
void rt_hw_lock_gate_register(void);

void rt_hw_gate_temperature_register(void);

void rt_hw_camera_photosensor_register(void);
void rt_hw_camera_irdasensor_register(void);

void rt_hw_rfid_key_detect_register(void);

void rt_hw_gsm_ring_register(void);

void rt_hw_battery_switch_register(void);
    
void rt_hw_key2_register(void);

#endif
