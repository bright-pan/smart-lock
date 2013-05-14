/*********************************************************************
 * Filename:      gpio_exti.c
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

#include "gpio_exti.h"

struct gpio_exti_user_data
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
  rt_err_t (*gpio_exti_rx_indicate)(rt_device_t dev, rt_size_t size);//callback function for int
};

/*
 *  GPIO ops function interfaces
 *
 */
static void __gpio_nvic_configure(gpio_device *gpio,FunctionalState new_status)
{
  NVIC_InitTypeDef nvic_initstructure;
  struct gpio_exti_user_data* user = (struct gpio_exti_user_data *)gpio->parent.user_data;
	
  nvic_initstructure.NVIC_IRQChannel = user->nvic_channel;
  nvic_initstructure.NVIC_IRQChannelPreemptionPriority = user->nvic_preemption_priority;
  nvic_initstructure.NVIC_IRQChannelSubPriority = user->nvic_subpriority;
  nvic_initstructure.NVIC_IRQChannelCmd = new_status;
  NVIC_Init(&nvic_initstructure);
}

static void __gpio_exti_configure(gpio_device *gpio,FunctionalState new_status)
{	
  EXTI_InitTypeDef exti_initstructure;
  struct gpio_exti_user_data* user = (struct gpio_exti_user_data *)gpio->parent.user_data;
	
  exti_initstructure.EXTI_Line = user->exti_line;
  exti_initstructure.EXTI_Mode = user->exti_mode;
  exti_initstructure.EXTI_Trigger = user->exti_trigger;
  exti_initstructure.EXTI_LineCmd = new_status;
  EXTI_Init(&exti_initstructure);
}

static void __gpio_pin_configure(gpio_device *gpio)
{
  GPIO_InitTypeDef gpio_initstructure;
  struct gpio_exti_user_data *user = (struct gpio_exti_user_data*)gpio->parent.user_data;
  RCC_APB2PeriphClockCmd(user->gpio_clock,ENABLE);
  gpio_initstructure.GPIO_Mode = user->gpio_mode;
  gpio_initstructure.GPIO_Pin = user->gpio_pinx;
  gpio_initstructure.GPIO_Speed = user->gpio_speed;
  GPIO_Init(user->gpiox,&gpio_initstructure);
  GPIO_EXTILineConfig(user->gpio_port_source,user->gpio_pin_source);		
}
/*
 * gpio ops configure
 */
rt_err_t gpio_exti_configure(gpio_device *gpio)
{
  __gpio_pin_configure(gpio);
  if(RT_DEVICE_FLAG_INT_RX & gpio->parent.flag)
  {
    __gpio_nvic_configure(gpio,ENABLE);
    __gpio_exti_configure(gpio,ENABLE);
  }

  return RT_EOK;
}
rt_err_t gpio_exti_control(gpio_device *gpio, rt_uint8_t cmd, void *arg)
{
  //struct gpio_exti_user_data *user = (struct gpio_exti_user_data*)gpio->parent.user_data;
  return RT_EOK;
}

void gpio_exti_out(gpio_device *gpio, rt_uint8_t data)
{
  struct gpio_exti_user_data *user = (struct gpio_exti_user_data*)gpio->parent.user_data;

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
    rt_kprintf("gpio exti device <%s> is can`t write! please check device flag RT_DEVICE_FLAG_WRONLY\n", user->name);
#endif
  }
}

rt_uint8_t gpio_exti_intput(gpio_device *gpio)
{
  struct gpio_exti_user_data* user = (struct gpio_exti_user_data *)gpio->parent.user_data;

  if (gpio->parent.flag & RT_DEVICE_FLAG_RDONLY)
  {
    return GPIO_ReadInputDataBit(user->gpiox,user->gpio_pinx);
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("gpio exti device <%s> is can`t read! please check device flag RT_DEVICE_FLAG_RDONLY\n", user->name);
#endif
    return 0;
  }
}

struct rt_gpio_ops gpio_exti_user_ops= 
{
  gpio_exti_configure,
  gpio_exti_control,
  gpio_exti_out,
  gpio_exti_intput
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

struct gpio_exti_user_data key1_user_data = 
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
  struct gpio_exti_user_data *key_user_data = &key1_user_data;

  key_device->ops = &gpio_exti_user_ops;
  
  rt_hw_gpio_register(key_device,key_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), key_user_data);
  rt_device_set_rx_indicate((rt_device_t)key_device,key_user_data->gpio_exti_rx_indicate);
}

rt_err_t key2_rx_ind(rt_device_t dev, rt_size_t size)
{
  gpio_device *gpio = RT_NULL;
  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;

  rt_kprintf("key2 ok, value is 0x%x\n", gpio->pin_value);
  return RT_EOK;
}

struct gpio_exti_user_data key2_user_data = 
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
  gpio_device *key_device = &key2_device;
  struct gpio_exti_user_data *key_user_data = &key2_user_data;

  key_device->ops = &gpio_exti_user_ops;
  
  rt_hw_gpio_register(key_device,key_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), key_user_data);
  rt_device_set_rx_indicate((rt_device_t)key_device,key_user_data->gpio_exti_rx_indicate);
}


#ifdef RT_USING_FINSH
#include <finsh.h>
void key(rt_int8_t num)
{
  rt_device_t key = RT_NULL;
  rt_int8_t dat = 0;

  if(num == 1)
  {
    key = rt_device_find("key1");
    if (key != RT_NULL)
    {
      rt_device_read(key,0,&dat,0);
#ifdef RT_USING_FINSH
      rt_kprintf("key1 = 0x%x\n",dat);
#endif      
    }
    else
    {
#ifdef RT_USING_FINSH
      rt_kprintf("the device is not found!\n");
#endif
    }
  }
  if(num == 2)
  {
    key = rt_device_find("key2");
    if (key != RT_NULL)
    {
      rt_device_read(key,0,&dat,0);
#ifdef RT_USING_FINSH
      rt_kprintf("key2 = 0x%x\n",dat);
#endif      
    }
    else
    {
#ifdef RT_USING_FINSH
      rt_kprintf("the device is not found!\n");
#endif
    }
  }
}
FINSH_FUNCTION_EXPORT(key, key[1 2] = x)
#endif
