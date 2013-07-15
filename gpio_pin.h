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

#define DEVICE_NAME_GSM_POWER "g_power"
#define DEVICE_NAME_GSM_STATUS "g_stat"
#define DEVICE_NAME_GSM_LED "g_led"
#define DEVICE_NAME_GSM_DTR "g_dtr"

#define DEVICE_NAME_VOICE_RESET "vo_rst"
#define DEVICE_NAME_VOICE_SWITCH "vo_sw"
#define DEVICE_NAME_VOICE_AMP "vo_amp"

#define DEVICE_NAME_LOGO_LED "logo"

#define DEVICE_NAME_CAMERA_LED "cm_led"
#define DEVICE_NAME_CAMERA_POWER "cm_power"

#define DEVICE_NAME_RFID_POWER "rf_power"

#define DEVICE_NAME_MOTOR_STATUS "mt_stat"


void rt_hw_gsm_led_register(void);
void rt_hw_gsm_power_register(void);
void rt_hw_gsm_status_register(void);
void rt_hw_gsm_dtr_register(void);

void rt_hw_rfid_power_register(void);

void rt_hw_camera_power_register(void);
void rt_hw_camera_led_register(void);

void rt_hw_logo_led_register(void);

void rt_hw_voice_switch_register(void);
void rt_hw_voice_amp_register(void);

void rt_hw_test_register(void);

void rt_hw_motor_status_register(void);


uint8_t gpio_pin_input(char *str);
void gpio_pin_output(char *str, const rt_uint8_t dat);

#endif
