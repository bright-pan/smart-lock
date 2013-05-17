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
#define RT_DEVICE_CTRL_SET_PULSE_VALUE  0x16    /* disable receive irq */
#define RT_DEVICE_FLAG_PWM_TX           0x1000 /* flag mask for gpio pwm mode */
//#define RT_DEVICE_FLAG_ONE_PULSE        0x2000
#define DEVICE_NAME_MOTOR_A_PULSE "mt_a"
#define DEVICE_NAME_MOTOR_B_PULSE "mt_b"
#define DEVICE_NAME_VOICE_DATA "vo_data"

struct gpio_pwm_user_data
{
  const char name[RT_NAME_MAX];
  GPIO_TypeDef* gpiox;//port
  rt_uint16_t gpio_pinx;//pin
  GPIOMode_TypeDef gpio_mode;//mode
  GPIOSpeed_TypeDef gpio_speed;//speed
  rt_uint32_t gpio_clock;//clock
  TIM_TypeDef *timx;//tim[1-8]
  rt_uint32_t tim_rcc;
  rt_uint32_t tim_base_clock;// tim clock and < system core freq
  rt_uint16_t tim_base_reload_value;
  rt_uint16_t tim_base_clock_division;
  rt_uint16_t tim_base_counter_mode;//counter up or down
  rt_uint32_t tim_oc_mode;//output compare mode
  rt_uint16_t tim_oc_output_state;//output value state
  rt_uint16_t tim_oc_pulse_value;//pulse value
  rt_uint16_t tim_oc_polarity;
  rt_uint16_t tim_oc_channel;
  rt_uint16_t tim_int_flag;
  __IO rt_uint16_t tim_pulse_counts;
  rt_uint8_t nvic_channel_1;
  rt_uint8_t nvic_channel_2;  
  rt_uint8_t nvic_preemption_priority;
  rt_uint8_t nvic_subpriority;
  void (* tim_oc_init)(TIM_TypeDef *TIMx, TIM_OCInitTypeDef *TIM_OCInitStruct);
  void (* tim_oc_set_compare)(TIM_TypeDef* TIMx, uint16_t Compare1);
  void (* tim_rcc_cmd)(uint32_t RCC_APB2Periph, FunctionalState NewState);
};

void rt_hw_pwm1_register(void);

void rt_hw_motor_a_pulse_register(void);
void rt_hw_motor_b_pulse_register(void);

void rt_hw_voice_data_register(void);

#endif
