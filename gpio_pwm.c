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
  rt_uint16_t tim_oc_pulse_counts;//pulse counts
  rt_uint16_t tim_oc_polarity;
  void (* tim_oc_init)(TIM_TypeDef *TIMx, TIM_OCInitTypeDef *TIM_OCInitStruct);
  void (* tim_oc_set_compare)(TIM_TypeDef* TIMx, uint16_t Compare1);
};

/*
 *  GPIO ops function interfaces
 *
 */
static void __gpio_pin_configure(gpio_device *gpio)
{
  GPIO_InitTypeDef gpio_initstructure;
  struct gpio_pwm_user_data *user = (struct gpio_pwm_user_data*)gpio->parent.user_data;
  RCC_APB2PeriphClockCmd(user->gpio_clock,ENABLE);
  gpio_initstructure.GPIO_Mode = user->gpio_mode;
  gpio_initstructure.GPIO_Pin = user->gpio_pinx;
  gpio_initstructure.GPIO_Speed = user->gpio_speed;
  GPIO_Init(user->gpiox,&gpio_initstructure);
}
static void __gpio_timer_configure(gpio_device *gpio)
{
  TIM_TimeBaseInitTypeDef time_base_structure;
  TIM_OCInitTypeDef time_oc_structure;
  struct gpio_pwm_user_data *user = (struct gpio_pwm_user_data*)gpio->parent.user_data;
  
  RCC_APB1PeriphClockCmd(user->tim_rcc, ENABLE);
  /* timer base configuration */
  time_base_structure.TIM_Period = user->tim_base_reload_value;
  time_base_structure.TIM_Prescaler = (uint16_t) (SystemCoreClock / user->tim_base_clock) - 1;
  time_base_structure.TIM_ClockDivision = user->tim_base_clock_division;
  time_base_structure.TIM_CounterMode = user->tim_base_counter_mode;
  TIM_TimeBaseInit(user->timx, &time_base_structure);
   /* timer oc mode configuration */
  time_oc_structure.TIM_OCMode = user->tim_oc_mode;
  time_oc_structure.TIM_OutputState = user->tim_oc_output_state;
  time_oc_structure.TIM_Pulse = user->tim_oc_pulse_counts;
  time_oc_structure.TIM_OCPolarity = user->tim_oc_polarity;
  user->tim_oc_init(user->timx, &time_oc_structure);
  TIM_SelectOnePulseMode(user->timx, TIM_OPMode_Single);
  TIM_Cmd(user->timx, ENABLE);
}

/*
 * gpio ops configure
 */
rt_err_t gpio_pwm_configure(gpio_device *gpio)
{	
  __gpio_pin_configure(gpio);
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
        user->tim_oc_set_compare(user->timx, user->tim_oc_pulse_counts);
        TIM_Cmd(user->timx, ENABLE);
        break;
      }
    case RT_DEVICE_CTRL_SET_PULSE_COUNTS:
      {
        user->tim_oc_pulse_counts = *(rt_uint16_t *)arg;
        
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
  rt_hw_gpio_register(pwm_device,pwm_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_PWM_TX), pwm_user_data);
}

#ifdef RT_USING_FINSH
#include <finsh.h>
void pwm1_set_pulse_counts(const rt_uint16_t time)
{
  rt_device_t device = RT_NULL;
  device = rt_device_find("pwm1");
  rt_device_control(device, RT_DEVICE_CTRL_SET_PULSE_COUNTS, (void *)&time);
}
FINSH_FUNCTION_EXPORT(pwm1_set_pulse_counts, pwm1_set_pulse_counts[x])

void pwm1_send_pulse(void)
{
  rt_device_t device = RT_NULL;
  device = rt_device_find("pwm1");
  rt_device_control(device, RT_DEVICE_CTRL_SEND_PULSE, (void *)0);
}
FINSH_FUNCTION_EXPORT(pwm1_send_pulse, pwm1_send_pulse[])
#endif
