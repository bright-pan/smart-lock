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
 * 2013-04-27 Bright Pan <loststriker@gmail.com>
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "gpio_pin.h"

struct gpio_pin_user_data
{
  const char name[RT_NAME_MAX];
  GPIO_TypeDef* gpiox;//port
  rt_uint16_t gpio_pinx;//pin
  GPIOMode_TypeDef gpio_mode;//mode
  GPIOSpeed_TypeDef gpio_speed;//speed
  rt_uint32_t gpio_clock;//clock
  rt_uint8_t gpio_default_output;
};
/*
 * gpio pin ops configure
 */
rt_err_t gpio_pin_configure(gpio_device *gpio)
{
  GPIO_InitTypeDef gpio_initstructure;
  struct gpio_pin_user_data *user = (struct gpio_pin_user_data*)gpio->parent.user_data;

  RCC_APB2PeriphClockCmd(user->gpio_clock,ENABLE);
  gpio_initstructure.GPIO_Mode = user->gpio_mode;
  gpio_initstructure.GPIO_Pin = user->gpio_pinx;
  gpio_initstructure.GPIO_Speed = user->gpio_speed;
  if (user->gpio_default_output)
  {
    GPIO_SetBits(user->gpiox,user->gpio_pinx);
  }
  else
  {
    GPIO_ResetBits(user->gpiox,user->gpio_pinx);
  }
  GPIO_Init(user->gpiox,&gpio_initstructure);

  return RT_EOK;
}

rt_err_t gpio_pin_control(gpio_device *gpio, rt_uint8_t cmd, void *arg)
{
  return RT_EOK;
}

void gpio_pin_out(gpio_device *gpio, rt_uint8_t data)
{
  struct gpio_pin_user_data *user = (struct gpio_pin_user_data*)gpio->parent.user_data;
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
    rt_kprintf("gpio pin device <%s> is can`t write! please check device flag RT_DEVICE_FLAG_WRONLY\n", user->name);
#endif
  }
}

rt_uint8_t gpio_pin_intput(gpio_device *gpio)
{
  struct gpio_pin_user_data* user = (struct gpio_pin_user_data *)gpio->parent.user_data;

  if (gpio->parent.flag & RT_DEVICE_FLAG_RDONLY)
  {
    return GPIO_ReadInputDataBit(user->gpiox,user->gpio_pinx);
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("gpio pin device <%s> is can`t read! please check device flag RT_DEVICE_FLAG_RDONLY\n", user->name);
#endif
    return 0;
  }
}

struct rt_gpio_ops gpio_pin_user_ops= 
{
  gpio_pin_configure,
  gpio_pin_control,
  gpio_pin_out,
  gpio_pin_intput
};

struct gpio_pin_user_data led1_user_data = 
{
  "led1",
  GPIOC,
  GPIO_Pin_4,
  GPIO_Mode_Out_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOC,
  1,
};
gpio_device led1_device;

void rt_hw_led1_register(void)
{
  gpio_device *led_device = &led1_device;
  struct gpio_pin_user_data *led_user_data = &led1_user_data;

  led_device->ops = &gpio_pin_user_ops;  
  rt_hw_gpio_register(led_device,led_user_data->name, (RT_DEVICE_FLAG_RDWR), led_user_data);
}
/* gsm power device */
struct gpio_pin_user_data gsm_power_user_data = 
{
  DEVICE_NAME_GSM_POWER,
  GPIOD,
  GPIO_Pin_0,
  GPIO_Mode_Out_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOD,
  1,
};
gpio_device gsm_power_device;
void rt_hw_gsm_power_register(void)
{
  gpio_device *gpio_device = &gsm_power_device;
  struct gpio_pin_user_data *gpio_user_data = &gsm_power_user_data;

  gpio_device->ops = &gpio_pin_user_ops;
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR), gpio_user_data);
}
/* gsm status device */
struct gpio_pin_user_data gsm_status_user_data = 
{
  DEVICE_NAME_GSM_STATUS,
  GPIOD,
  GPIO_Pin_7,
  GPIO_Mode_IN_FLOATING,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOD,
  1,
};
gpio_device gsm_status_device;
void rt_hw_gsm_status_register(void)
{
  gpio_device *gpio_device = &gsm_status_device;
  struct gpio_pin_user_data *gpio_user_data = &gsm_status_user_data;

  gpio_device->ops = &gpio_pin_user_ops;
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR), gpio_user_data);
}


#ifdef RT_USING_FINSH
#include <finsh.h>
void led1(const rt_uint8_t dat)
{
  rt_device_t led = RT_NULL;
  led = rt_device_find("led1");
  if (led != RT_NULL)
  {
    rt_device_write(led,0,&dat,0);
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("the device is not found!\n");
#endif
  }
}
FINSH_FUNCTION_EXPORT(led1, led1[0 1])
void gsm_power(const rt_uint8_t dat)
{
  rt_device_t device = RT_NULL;
  device = rt_device_find(DEVICE_NAME_GSM_POWER);
  if (device != RT_NULL)
  {
    rt_device_write(device,0,&dat,0);
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("the gpio device %s is not found!\n", DEVICE_NAME_GSM_POWER);
#endif
  }
}	
FINSH_FUNCTION_EXPORT(gsm_power, gsm_power[0 1])
void gsm_status(const rt_uint8_t dat)
{
  rt_device_t device = RT_NULL;
  device = rt_device_find(DEVICE_NAME_GSM_STATUS);
  if (device != RT_NULL)
  {
    rt_device_write(device,0,&dat,0);
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("the gpio device %s is not found!\n", DEVICE_NAME_GSM_STATUS);
#endif
  }
}	
FINSH_FUNCTION_EXPORT(gsm_status, gsm_status[0 1])
#endif
