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
  GPIO_InitTypeDef gpio_init_structure;
  struct gpio_pin_user_data *user = (struct gpio_pin_user_data*)gpio->parent.user_data;
  GPIO_StructInit(&gpio_init_structure);
  RCC_APB2PeriphClockCmd(user->gpio_clock,ENABLE);
  gpio_init_structure.GPIO_Mode = user->gpio_mode;
  gpio_init_structure.GPIO_Pin = user->gpio_pinx;
  gpio_init_structure.GPIO_Speed = user->gpio_speed;
  if (user->gpio_default_output)
  {
    GPIO_SetBits(user->gpiox,user->gpio_pinx);
  }
  else
  {
    GPIO_ResetBits(user->gpiox,user->gpio_pinx);
  }
  GPIO_Init(user->gpiox,&gpio_init_structure);

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

/**************************************************
 *           gpio pin device register
 *
 **************************************************/

/* RFID power device register */
struct gpio_pin_user_data rfid_power_user_data = 
{
  DEVICE_NAME_RFID_POWER,
  GPIOE,
  GPIO_Pin_11,
  GPIO_Mode_Out_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOE,
  0,//default value
};
gpio_device rfid_power_device;

void rt_hw_rfid_power_register(void)
{
  gpio_device *gpio_device = &rfid_power_device;
  struct gpio_pin_user_data *gpio_user_data = &rfid_power_user_data;

  gpio_device->ops = &gpio_pin_user_ops;  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR), gpio_user_data);
  }

/* camera led device */
struct gpio_pin_user_data camera_led_user_data = 
{
  DEVICE_NAME_CAMERA_LED,
  GPIOE,
  GPIO_Pin_9,
  GPIO_Mode_Out_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOE,
  0,
};
gpio_device camera_led_device;

void rt_hw_camera_led_register(void)
{
  gpio_device *gpio_device = &camera_led_device;
  struct gpio_pin_user_data *gpio_user_data = &camera_led_user_data;

  gpio_device->ops = &gpio_pin_user_ops;  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR), gpio_user_data);
}
/* camera power device */
struct gpio_pin_user_data camera_power_user_data = 
{
  DEVICE_NAME_CAMERA_POWER,
  GPIOE,
  GPIO_Pin_12,
  GPIO_Mode_Out_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOE,
  0,
};
gpio_device camera_power_device;

void rt_hw_camera_power_register(void)
{
  gpio_device *gpio_device = &camera_power_device;
  struct gpio_pin_user_data *gpio_user_data = &camera_power_user_data;

  gpio_device->ops = &gpio_pin_user_ops;  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR), gpio_user_data);
}

/* logo led device */
struct gpio_pin_user_data logo_led_user_data = 
{
  DEVICE_NAME_LOGO_LED,
  GPIOE,
  GPIO_Pin_10,
  GPIO_Mode_Out_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOE,
  0,
};
gpio_device logo_led_device;

void rt_hw_logo_led_register(void)
{
  gpio_device *gpio_device = &logo_led_device;
  struct gpio_pin_user_data *gpio_user_data = &logo_led_user_data;

  gpio_device->ops = &gpio_pin_user_ops;  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR), gpio_user_data);
}

/* gsm led device */
struct gpio_pin_user_data gsm_led_user_data = 
{
  DEVICE_NAME_GSM_LED,
  GPIOE,
  GPIO_Pin_13,
  GPIO_Mode_Out_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOE,
  0,
};
gpio_device gsm_led_device;

void rt_hw_gsm_led_register(void)
{
  gpio_device *gpio_device = &gsm_led_device;
  struct gpio_pin_user_data *gpio_user_data = &gsm_led_user_data;

  gpio_device->ops = &gpio_pin_user_ops;  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR), gpio_user_data);
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
  0,
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
  RCC_APB2Periph_GPIOE,
  0,
};
gpio_device gsm_status_device;
void rt_hw_gsm_status_register(void)
{
  gpio_device *gpio_device = &gsm_status_device;
  struct gpio_pin_user_data *gpio_user_data = &gsm_status_user_data;

  gpio_device->ops = &gpio_pin_user_ops;
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR), gpio_user_data);
}

/* voice reset device */
struct gpio_pin_user_data voice_reset_user_data = 
{
  DEVICE_NAME_VOICE_RESET,
  GPIOA,
  GPIO_Pin_12,
  GPIO_Mode_Out_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOA,
  0,
};
gpio_device voice_reset_device;

void rt_hw_voice_reset_register(void)
{
  gpio_device *gpio_device = &voice_reset_device;
  struct gpio_pin_user_data *gpio_user_data = &voice_reset_user_data;

  gpio_device->ops = &gpio_pin_user_ops;  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR), gpio_user_data);
}
/* voice switch device */
struct gpio_pin_user_data voice_switch_user_data = 
{
  DEVICE_NAME_VOICE_SWITCH,
  GPIOC,
  GPIO_Pin_7,
  GPIO_Mode_Out_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOC,
  1,
};
gpio_device voice_switch_device;

void rt_hw_voice_switch_register(void)
{
  gpio_device *gpio_device = &voice_switch_device;
  struct gpio_pin_user_data *gpio_user_data = &voice_switch_user_data;

  gpio_device->ops = &gpio_pin_user_ops;  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR), gpio_user_data);
}
/* voice amp device */
struct gpio_pin_user_data voice_amp_user_data = 
{
  DEVICE_NAME_VOICE_AMP,
  GPIOC,
  GPIO_Pin_6,
  GPIO_Mode_Out_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOC,
  0,
};
gpio_device voice_amp_device;

void rt_hw_voice_amp_register(void)
{
  gpio_device *gpio_device = &voice_amp_device;
  struct gpio_pin_user_data *gpio_user_data = &voice_amp_user_data;

  gpio_device->ops = &gpio_pin_user_ops;  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR), gpio_user_data);
}

/* gsm led device */
struct gpio_pin_user_data test_user_data =
{
  "test",
  GPIOC,
  GPIO_Pin_8,
  GPIO_Mode_Out_PP,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOC,
  1,
};
gpio_device test_device;

void rt_hw_test_register(void)
{
  gpio_device *gpio_device = &test_device;
  struct gpio_pin_user_data *gpio_user_data = &test_user_data;

  gpio_device->ops = &gpio_pin_user_ops;
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR), gpio_user_data);
}

void gpio_pin_output(char *str, const rt_uint8_t dat)
{
  rt_device_t device = RT_NULL;
  device = rt_device_find(str);
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

uint8_t gpio_pin_input(char *str)
{
  rt_device_t device = RT_NULL;
  rt_uint8_t dat;
  device = rt_device_find(str);
  if (device != RT_NULL)
  {
    rt_device_read(device,0,&dat,0);
    rt_kprintf("the gpio pin value is %d\n", dat);
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("the gpio device %s is not found!\n", DEVICE_NAME_GSM_STATUS);
#endif
  }
  return dat;
}	

#ifdef RT_USING_FINSH
#include <finsh.h>
void led(const char *str, const rt_uint8_t dat)
{
  rt_device_t led = RT_NULL;
  led = rt_device_find(str);
  if (led != RT_NULL)
  {
    rt_device_write(led,0,&dat,0);
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("the led device <%s>is not found!\n", str);
#endif
  }
}
FINSH_FUNCTION_EXPORT(led, led[device_name 0/1])

FINSH_FUNCTION_EXPORT(gpio_pin_output, [device_name <0 1>])
FINSH_FUNCTION_EXPORT(gpio_pin_input, [device_name result])
#endif
