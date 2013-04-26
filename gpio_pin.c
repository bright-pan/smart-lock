/*********************************************************************
 * Filename:      gpio_pin.c
 * 
 * Description:    
 *
 * Author:        wangzw <wangzw@yuettak.com>
 * Created at:    2013-04-22
 *                
 * Modify:
 *
 * 2013-04-25 Bright Pan <loststriker@gmail.com>
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "gpio_pin.h"
#include "stm32f10x_conf.h"

struct gpio_user_data
{
  const char name[RT_NAME_MAX];
  GPIO_TypeDef* gpiox;//port
  rt_uint16_t gpio_pinx;//pin
  GPIOMode_TypeDef gpio_mode;//mode
  GPIOSpeed_TypeDef gpio_speed;//speed
  rt_uint32_t gpio_clock;//clock
  rt_uint8_t gpio_port_source;
  rt_uint8_t gpio_pin_source;
  rt_uint32_t exti_line;//exti_line
  EXTIMode_TypeDef exti_mode;//exti_mode
  EXTITrigger_TypeDef exti_trigger;//exti_trigger
  rt_uint8_t nvic_channel;
  rt_uint8_t nvic_preemption_priority;
  rt_uint8_t nvic_subpriority;
  rt_err_t (*gpio_rx_indicate)(rt_device_t dev, rt_size_t size);//callback function for int
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
static void stm32_gpio_nvic_configure(gpio_device *gpio,FunctionalState new_status)
{
  NVIC_InitTypeDef nvic_initstructure;
  struct gpio_user_data* user = (struct gpio_user_data *)gpio->parent.user_data;
	
  nvic_initstructure.NVIC_IRQChannel = user->nvic_channel;
  nvic_initstructure.NVIC_IRQChannelPreemptionPriority = user->nvic_preemption_priority;
  nvic_initstructure.NVIC_IRQChannelSubPriority = user->nvic_subpriority;
  nvic_initstructure.NVIC_IRQChannelCmd = new_status;
  NVIC_Init(&nvic_initstructure);
}

static void stm32_gpio_exti_configure(gpio_device *gpio,FunctionalState new_status)
{	
  EXTI_InitTypeDef exti_initstructure;
  struct gpio_user_data* user = (struct gpio_user_data *)gpio->parent.user_data;
	
  exti_initstructure.EXTI_Line = user->exti_line;
  exti_initstructure.EXTI_Mode = user->exti_mode;
  exti_initstructure.EXTI_Trigger = user->exti_trigger;
  exti_initstructure.EXTI_LineCmd = new_status;
  EXTI_Init(&exti_initstructure);
}

static void stm32_gpio_pin_configure(gpio_device *gpio ,GPIOMode_TypeDef mode)
{
  GPIO_InitTypeDef gpio_initstructure;
  struct gpio_user_data *user = (struct gpio_user_data*)gpio->parent.user_data;

  gpio_initstructure.GPIO_Mode = mode;
  gpio_initstructure.GPIO_Pin = user->gpio_pinx;
  gpio_initstructure.GPIO_Speed = user->gpio_speed;
  GPIO_Init(user->gpiox,&gpio_initstructure);
}

static void stm32_gpio_timer_configure(gpio_device *gpio)
{
  TIM_TimeBaseInitTypeDef time_base_structure;
  TIM_OCInitTypeDef time_oc_structure;
  struct gpio_user_data *user = (struct gpio_user_data*)gpio->parent.user_data;
  
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
rt_err_t stm32_gpio_configure(gpio_device *gpio)
{	
  struct gpio_user_data *user = (struct gpio_user_data*)gpio->parent.user_data;

  RCC_APB2PeriphClockCmd(user->gpio_clock,ENABLE);
  if(RT_DEVICE_FLAG_INT_RX & gpio->parent.flag)
  {
    stm32_gpio_nvic_configure(gpio,ENABLE);	
    GPIO_EXTILineConfig(user->gpio_port_source,user->gpio_pin_source);		
    stm32_gpio_exti_configure(gpio,ENABLE);
  }
  if(RT_DEVICE_FLAG_PWM_TX & gpio->parent.flag)
  {	
    stm32_gpio_timer_configure(gpio);
  }

  stm32_gpio_pin_configure(gpio,user->gpio_mode);

  return RT_EOK;
}
rt_err_t stm32_gpio_control(gpio_device *gpio, rt_uint8_t cmd, void *arg)
{
  struct gpio_user_data *user = (struct gpio_user_data*)gpio->parent.user_data;
  
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

void stm32_gpio_out(gpio_device *gpio, rt_uint8_t data)
{
  struct gpio_user_data *user = (struct gpio_user_data*)gpio->parent.user_data;
	
  /* not is write mode */
  if((user->gpio_mode == GPIO_Mode_IN_FLOATING) || (user->gpio_mode == GPIO_Mode_IPD) 
     ||(user->gpio_mode == GPIO_Mode_IPU))
  {
    return ;
  }
  if(data)
  {
    GPIO_SetBits(user->gpiox,user->gpio_pinx);
  }
  else
  {
    GPIO_ResetBits(user->gpiox,user->gpio_pinx);
  }
}

rt_uint8_t stm32_gpio_intput(gpio_device *gpio)
{
  struct gpio_user_data* user = (struct gpio_user_data *)gpio->parent.user_data;
  rt_uint8_t data;

  /* not is read mode */
  if((user->gpio_mode == GPIO_Mode_IN_FLOATING) || (user->gpio_mode == GPIO_Mode_IPD) 
     ||(user->gpio_mode == GPIO_Mode_IPU))
  {
    data = GPIO_ReadInputDataBit(user->gpiox,user->gpio_pinx);
  }
  else
  {
    data =  0xff;
  }
   
  return data;
}

struct rt_gpio_ops gpio_user_ops= 
{
  stm32_gpio_configure,
  stm32_gpio_control,
  stm32_gpio_out,
  stm32_gpio_intput
};

rt_err_t key1_rx_ind(rt_device_t dev, rt_size_t size)
{
  gpio_device *gpio = RT_NULL;
  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;

  rt_kprintf("key1 ok, value is 0x%x\n", gpio->pin_value);
  return RT_EOK;
}



gpio_device key1_device;

struct gpio_user_data key1_user_data = 
{	
  "key1" ,
  GPIOE,
  GPIO_Pin_5,
  GPIO_Mode_IN_FLOATING,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOE |RCC_APB2Periph_AFIO,
  GPIO_PortSourceGPIOE,
  GPIO_PinSource5,
  EXTI_Line5,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Falling,
  EXTI9_5_IRQn,
  1,
  5,
  key1_rx_ind,
};

void rt_hw_key1_register(void)
{
  gpio_device *key_device = &key1_device;
  struct gpio_user_data *key_user_data = &key1_user_data;

  key_device->ops = &gpio_user_ops;
  
  rt_hw_gpio_register(key_device,key_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), key_user_data);
  rt_device_set_rx_indicate((rt_device_t)key_device,key_user_data->gpio_rx_indicate);
}

rt_err_t key2_rx_ind(rt_device_t dev, rt_size_t size)
{
  gpio_device *gpio = RT_NULL;
  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;

  rt_kprintf("key2 ok, value is 0x%x\n", gpio->pin_value);
  return RT_EOK;
}

struct gpio_user_data key2_user_data = 
{
  "key2",
  GPIOE,
  GPIO_Pin_6,
  GPIO_Mode_IN_FLOATING,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO,
  GPIO_PortSourceGPIOE,
  GPIO_PinSource6,
  EXTI_Line6,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Falling,
  EXTI9_5_IRQn,
  1,
  4,
  key2_rx_ind,
};

gpio_device key2_device;
void rt_hw_key2_register(void)
{
  gpio_device *key_device = &key1_device;
  struct gpio_user_data *key_user_data = &key1_user_data;

  key_device->ops = &gpio_user_ops;
  
  rt_hw_gpio_register(key_device,key_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), key_user_data);
  rt_device_set_rx_indicate((rt_device_t)key_device,key_user_data->gpio_rx_indicate);
}


struct gpio_user_data led1_user_data = 
{
  "led1",
  GPIOC,
  GPIO_Pin_4,
  GPIO_Mode_AF_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOC,
  GPIO_PortSourceGPIOC,
  GPIO_PinSource4,
  EXTI_Line4,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Falling,
  EXTI4_IRQn,
  1,
  6,
  RT_NULL,
};

gpio_device led1_device;

void rt_hw_led1_register(void)
{
  gpio_device *led_device = &led1_device;
  struct gpio_user_data *led_user_data = &led1_user_data;

  led_device->ops = &gpio_user_ops;
  
  rt_hw_gpio_register(led_device,led_user_data->name, (RT_DEVICE_FLAG_RDWR), led_user_data);
}


struct gpio_user_data pwm1_user_data = 
{
  "pwm1",
  GPIOA,
  GPIO_Pin_6,
  GPIO_Mode_AF_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOA,
  GPIO_PortSourceGPIOC,
  GPIO_PinSource6,
  EXTI_Line6,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Falling,
  EXTI4_IRQn,
  1,
  6,
  RT_NULL,
  /* timer base */
  TIM3,
  RCC_APB1Periph_TIM3,
  24000000,
  65535,
  0,
  TIM_CounterMode_Up,
  /* timer base */
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
  struct gpio_user_data *pwm_user_data = &pwm1_user_data;

  pwm_device->ops = &gpio_user_ops;
  pwm_user_data->tim_oc_init = TIM_OC1Init;
  pwm_user_data->tim_oc_set_compare = TIM_SetCompare1;
  rt_hw_gpio_register(pwm_device,pwm_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_PWM_TX), pwm_user_data);
}

#ifdef RT_USING_FINSH
#include <finsh.h>

void led1(const rt_uint8_t dat)
{
  rt_device_t led = RT_NULL;
  led = rt_device_find("led1");

  rt_device_write(led,0,&dat,0);
}	
FINSH_FUNCTION_EXPORT(led1, led1[0 1])

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

void key(rt_int8_t num)
{
  rt_device_t key = RT_NULL;
  rt_int8_t dat = 0;

  if(num == 1)
  {
    key = rt_device_find("key1");
    rt_device_read(key,0,&dat,0);
    rt_kprintf("key1 = 0x%x\n",dat);
  }
  if(num == 2)
  {
    key = rt_device_find("key2");
    rt_device_read(key,0,&dat,0);
    rt_kprintf("key1 = 0x%x\n",dat);
  }
}
FINSH_FUNCTION_EXPORT(key, key[1 2] = x)
#endif
