/*********************************************************************
 * Filename:      gpio_pwm.c
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

#include "gpio_pwm.h"

/*
 *  GPIO ops function interfaces
 *
 */
static void __gpio_pin_configure(gpio_device *gpio)
{
  GPIO_InitTypeDef gpio_init_structure;
  struct gpio_pwm_user_data *user = (struct gpio_pwm_user_data*)gpio->parent.user_data;
  GPIO_StructInit(&gpio_init_structure);
  RCC_APB2PeriphClockCmd(user->gpio_clock,ENABLE);
  gpio_init_structure.GPIO_Mode = user->gpio_mode;
  gpio_init_structure.GPIO_Pin = user->gpio_pinx;
  gpio_init_structure.GPIO_Speed = user->gpio_speed;
  GPIO_Init(user->gpiox,&gpio_init_structure);
}
static void __gpio_nvic_configure(gpio_device *gpio,FunctionalState new_status)
{
  NVIC_InitTypeDef nvic_initstructure;
  struct gpio_pwm_user_data* user = (struct gpio_pwm_user_data *)gpio->parent.user_data;

  if (user->nvic_channel_1 != RT_NULL)
  {
    nvic_initstructure.NVIC_IRQChannel = user->nvic_channel_1;
    nvic_initstructure.NVIC_IRQChannelPreemptionPriority = user->nvic_preemption_priority;
    nvic_initstructure.NVIC_IRQChannelSubPriority = user->nvic_subpriority;
    nvic_initstructure.NVIC_IRQChannelCmd = new_status;
    NVIC_Init(&nvic_initstructure);
  }
    
  if (user->nvic_channel_2 != RT_NULL)
  {
    nvic_initstructure.NVIC_IRQChannel = user->nvic_channel_2;
    nvic_initstructure.NVIC_IRQChannelPreemptionPriority = user->nvic_preemption_priority;
    nvic_initstructure.NVIC_IRQChannelSubPriority = user->nvic_subpriority;
    nvic_initstructure.NVIC_IRQChannelCmd = new_status;
    NVIC_Init(&nvic_initstructure);
  }
}

static void __gpio_timer_configure(gpio_device *gpio)
{
  TIM_TimeBaseInitTypeDef time_base_structure;
  TIM_OCInitTypeDef time_oc_structure;
  struct gpio_pwm_user_data *user = (struct gpio_pwm_user_data*)gpio->parent.user_data;
  TIM_TimeBaseStructInit(&time_base_structure);
  TIM_OCStructInit(&time_oc_structure);
  user->tim_rcc_cmd(user->tim_rcc, ENABLE);
  /* timer base configuration */
  time_base_structure.TIM_Period = user->tim_base_reload_value;
  time_base_structure.TIM_Prescaler = (uint16_t) (SystemCoreClock / user->tim_base_clock) - 1;
  time_base_structure.TIM_ClockDivision = user->tim_base_clock_division;
  time_base_structure.TIM_CounterMode = user->tim_base_counter_mode;
  //time_base_structure.TIM_RepetitionCounter = 100;
  TIM_TimeBaseInit(user->timx, &time_base_structure);
   /* timer oc mode configuration */
  time_oc_structure.TIM_OCMode = user->tim_oc_mode;
  time_oc_structure.TIM_OutputState = user->tim_oc_output_state;
  time_oc_structure.TIM_Pulse = user->tim_oc_pulse_value;
  time_oc_structure.TIM_OCPolarity = user->tim_oc_polarity;
  //time_oc_structure.TIM_OCIdleState = TIM_OCIdleState_Reset;
  user->tim_oc_init(user->timx, &time_oc_structure);
  //TIM_OC1PreloadConfig(user->timx, TIM_OCPreload_Disable);
  /*
  if(RT_DEVICE_FLAG_ONE_PULSE & gpio->parent.flag)
  {
    TIM_SelectOnePulseMode(user->timx, TIM_OPMode_Single);
  }
  */
  if(RT_DEVICE_FLAG_INT_TX & gpio->parent.flag)
  {
    TIM_ITConfig(user->timx, user->tim_int_flag, ENABLE);
  }
  /* TIM1 Main Output Enable */
  if (IS_TIM_LIST2_PERIPH(user->timx))
  {
    TIM_CtrlPWMOutputs(user->timx, ENABLE);
  }
  //TIM_Cmd(user->timx, ENABLE);
}

/*
 * gpio ops configure
 */
rt_err_t gpio_pwm_configure(gpio_device *gpio)
{	
  __gpio_pin_configure(gpio);
  if(RT_DEVICE_FLAG_INT_TX & gpio->parent.flag)
  {
    __gpio_nvic_configure(gpio, ENABLE);
  }
      
  if(RT_DEVICE_FLAG_PWM_TX & gpio->parent.flag)
  {	
    __gpio_timer_configure(gpio);
  }
  return RT_EOK;
}
rt_err_t gpio_pwm_control(gpio_device *gpio, rt_uint8_t cmd, void *arg)
{
  struct gpio_pwm_user_data *user = (struct gpio_pwm_user_data*)gpio->parent.user_data;
  
  switch (cmd)
  {
    case RT_DEVICE_CTRL_SEND_PULSE:
      {
        if (user->tim_pulse_counts > 0)
        {
          TIM_CCxCmd(user->timx, user->tim_oc_channel, TIM_CCx_Enable);
          TIM_Cmd(user->timx, ENABLE);
        }
        break;
      }
    case RT_DEVICE_CTRL_SET_PULSE_VALUE:
      {
        user->tim_oc_pulse_value = *(rt_uint16_t *)arg;
        user->tim_oc_set_compare(user->timx, user->tim_oc_pulse_value);        
        break;
      }
    case RT_DEVICE_CTRL_SET_PULSE_COUNTS:
      {
        user->tim_pulse_counts = *(rt_uint16_t *)arg;
        break;
      }
  }
  return RT_EOK;
}

void gpio_pwm_out(gpio_device *gpio, rt_uint8_t data)
{
  struct gpio_pwm_user_data *user = (struct gpio_pwm_user_data*)gpio->parent.user_data;

  if (gpio->parent.flag & RT_DEVICE_FLAG_WRONLY)
  {
    if (data)
    {
      GPIO_SetBits(user->gpiox,user->gpio_pinx);
    }
    else
    {
      GPIO_ResetBits(user->gpiox,user->gpio_pinx);
    }
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("gpio pwm device <%s> is can`t write! please check device flag RT_DEVICE_FLAG_WRONLY\n", user->name);
#endif
  }
}

rt_uint8_t gpio_pwm_intput(gpio_device *gpio)
{
  struct gpio_pwm_user_data* user = (struct gpio_pwm_user_data *)gpio->parent.user_data;

  if (gpio->parent.flag & RT_DEVICE_FLAG_RDONLY)
  {
    return GPIO_ReadInputDataBit(user->gpiox,user->gpio_pinx);
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("gpio pwm device <%s> is can`t read! please check device flag RT_DEVICE_FLAG_RDONLY\n", user->name);
#endif
    return 0;
  }
}

struct rt_gpio_ops gpio_pwm_user_ops= 
{
  gpio_pwm_configure,
  gpio_pwm_control,
  gpio_pwm_out,
  gpio_pwm_intput
};

/* voice data device register */
struct gpio_pwm_user_data voice_data_user_data = 
{
  DEVICE_NAME_VOICE_DATA,
  GPIOA,
  GPIO_Pin_11,
  GPIO_Mode_AF_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOA,
  /* timer base */
  TIM1,
  RCC_APB2Periph_TIM1,
  24000000,
  4800,//period=200us
  0,
  TIM_CounterMode_Up,
  /* timer oc */
  TIM_OCMode_PWM2,
  TIM_OutputState_Enable,
  2400,// pulse value 100us
  TIM_OCPolarity_High,
  TIM_Channel_4,
  TIM_IT_CC4 | TIM_IT_Update,
  0,// pulse counts
  TIM1_UP_IRQn,
  TIM1_CC_IRQn,
  1,
  0,
  RT_NULL,
  RT_NULL,
  RT_NULL,
};

gpio_device voice_data_device;

void rt_hw_voice_data_register(void)
{
  gpio_device *pwm_device = &voice_data_device;
  struct gpio_pwm_user_data *pwm_user_data = &voice_data_user_data;

  pwm_device->ops = &gpio_pwm_user_ops;
  pwm_user_data->tim_oc_init = TIM_OC4Init;
  pwm_user_data->tim_oc_set_compare = TIM_SetCompare4;
  pwm_user_data->tim_rcc_cmd = RCC_APB2PeriphClockCmd;
  rt_hw_gpio_register(pwm_device,pwm_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_PWM_TX | RT_DEVICE_FLAG_INT_TX), pwm_user_data);
}

/* Motor A device register */
struct gpio_pwm_user_data motor_a_pulse_user_data = 
{
  DEVICE_NAME_MOTOR_A_PULSE,
  GPIOC,
  GPIO_Pin_8,
  GPIO_Mode_AF_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOC,
  /* timer base */
  TIM8,
  RCC_APB2Periph_TIM8,
  24000000,
  65535,
  0,
  TIM_CounterMode_Up,
  /* timer oc */
  TIM_OCMode_PWM2,
  TIM_OutputState_Enable,
  30000,// pulse value
  TIM_OCPolarity_High,
  TIM_Channel_3,
  TIM_IT_CC3 | TIM_IT_Update,
  0,// pulse counts
  TIM8_UP_IRQn,
  TIM8_CC_IRQn,
  1,
  0,
  RT_NULL,
  RT_NULL,
  RT_NULL,
};

gpio_device motor_a_pulse_device;

void rt_hw_motor_a_pulse_register(void)
{
  gpio_device *pwm_device = &motor_a_pulse_device;
  struct gpio_pwm_user_data *pwm_user_data = &motor_a_pulse_user_data;

  pwm_device->ops = &gpio_pwm_user_ops;
  pwm_user_data->tim_oc_init = TIM_OC3Init;
  pwm_user_data->tim_oc_set_compare = TIM_SetCompare3;
  pwm_user_data->tim_rcc_cmd = RCC_APB2PeriphClockCmd;
  rt_hw_gpio_register(pwm_device,pwm_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_PWM_TX | RT_DEVICE_FLAG_INT_TX), pwm_user_data);
}
/* Motor B device register */
struct gpio_pwm_user_data motor_b_pulse_user_data = 
{
  DEVICE_NAME_MOTOR_B_PULSE,
  GPIOC,
  GPIO_Pin_9,
  GPIO_Mode_AF_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOC,
  /* timer base */
  TIM8,
  RCC_APB2Periph_TIM8,
  24000000,
  65535,
  0,
  TIM_CounterMode_Up,
  /* timer oc */
  TIM_OCMode_PWM2,
  TIM_OutputState_Enable,
  30000,// pulse value
  TIM_OCPolarity_High,
  TIM_Channel_4,
  TIM_IT_CC4 | TIM_IT_Update,
  0,// pulse counts
  TIM8_UP_IRQn,
  TIM8_CC_IRQn,
  1,
  0,
  RT_NULL,
  RT_NULL,
  RT_NULL,
};

gpio_device motor_b_pulse_device;

void rt_hw_motor_b_pulse_register(void)
{
  gpio_device *pwm_device = &motor_b_pulse_device;
  struct gpio_pwm_user_data *pwm_user_data = &motor_b_pulse_user_data;

  pwm_device->ops = &gpio_pwm_user_ops;
  pwm_user_data->tim_oc_init = TIM_OC4Init;
  pwm_user_data->tim_oc_set_compare = TIM_SetCompare4;
  pwm_user_data->tim_rcc_cmd = RCC_APB2PeriphClockCmd;
  rt_hw_gpio_register(pwm_device,pwm_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_PWM_TX | RT_DEVICE_FLAG_INT_TX), pwm_user_data);
}

struct gpio_pwm_user_data pwm1_user_data = 
{
  "pwm1",
  GPIOA,
  GPIO_Pin_6,
  GPIO_Mode_AF_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOA,
  /* timer base */
  TIM3,
  RCC_APB1Periph_TIM3,
  24000000,
  65535,
  0,
  TIM_CounterMode_Up,
  /* timer oc */
  TIM_OCMode_PWM2,
  TIM_OutputState_Enable,
  30000,
  TIM_OCPolarity_High,
  TIM_Channel_1,
  TIM_IT_CC1 | TIM_IT_Update,
  20,
  TIM3_IRQn,
  RT_NULL,
  1,
  0,
  RT_NULL,
  RT_NULL,
  RT_NULL,
};

gpio_device pwm1_device;

void rt_hw_pwm1_register(void)
{
  gpio_device *pwm_device = &pwm1_device;
  struct gpio_pwm_user_data *pwm_user_data = &pwm1_user_data;

  pwm_device->ops = &gpio_pwm_user_ops;
  pwm_user_data->tim_oc_init = TIM_OC1Init;
  pwm_user_data->tim_oc_set_compare = TIM_SetCompare1;
  pwm_user_data->tim_rcc_cmd = RCC_APB1PeriphClockCmd;
  rt_hw_gpio_register(pwm_device,pwm_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_PWM_TX | RT_DEVICE_FLAG_INT_TX), pwm_user_data);
}

void delay(rt_uint32_t counts)
{
  rt_uint32_t i = 0;
  for (; i < counts; i++)
  {
    
  }
}

#ifdef RT_USING_FINSH
#include <finsh.h>
void pwm_set_counts(char *str, rt_uint16_t counts)
{
  rt_device_t device = RT_NULL;
  device = rt_device_find(str);
  rt_device_control(device, RT_DEVICE_CTRL_SET_PULSE_COUNTS, (void *)&counts);
}
FINSH_FUNCTION_EXPORT(pwm_set_counts, pwm_set_pulse_counts[device_name x])

void pwm_set_value(char *str, rt_uint16_t time)
{
  rt_device_t device = RT_NULL;
  device = rt_device_find(str);
  rt_device_control(device, RT_DEVICE_CTRL_SET_PULSE_VALUE, (void *)&time);
}
FINSH_FUNCTION_EXPORT(pwm_set_value, pwm_set_pulse_value[device_name x])

void pwm_send_pulse(char *str)
{
  rt_device_t device = RT_NULL;
  device = rt_device_find(str);
  rt_device_control(device, RT_DEVICE_CTRL_SEND_PULSE, (void *)0);
}
FINSH_FUNCTION_EXPORT(pwm_send_pulse, pwm_send_pulse[device_name])

void voice_output(rt_uint16_t counts)
{
  rt_device_t device = RT_NULL;
  rt_device_t data_device = RT_NULL;  
  rt_int8_t dat = 0;
  data_device = rt_device_find("vo_data");
  rt_device_control(data_device, RT_DEVICE_CTRL_SET_PULSE_COUNTS, (void *)&counts);
  device = rt_device_find("vo_sw");
  dat = 1;
  rt_device_write(device, 0, &dat, 0);
  device = rt_device_find("vo_amp");
  dat = 1;
  rt_device_write(device, 0, &dat, 0);
  device = rt_device_find("vo_rst");
  dat = 1;
  rt_device_write(device, 0, &dat, 0);
  delay(1058);
  dat = 0;
  rt_device_write(device, 0, &dat, 0);
  //  delay(26450);
  delay(1058);
  rt_device_control(data_device, RT_DEVICE_CTRL_SEND_PULSE, (void *)0);
}
FINSH_FUNCTION_EXPORT(voice_output, voice_output[counts])
#endif
