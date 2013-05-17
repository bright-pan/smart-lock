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

/* camera glint light pin*/
struct gpio_pin_user_data glint_right_user_data = 
{
	"glint",
	GPIOC,
	GPIO_Pin_3,
	GPIO_Mode_Out_PP,
	GPIO_Speed_50MHz,
	RCC_APB2Periph_GPIOC,
	1,
};
gpio_device glint_device;

void rt_hw_glint_light_register(void)
{
	glint_device.ops = &gpio_pin_user_ops;
	rt_hw_gpio_register(&glint_device,glint_right_user_data.name,RT_DEVICE_FLAG_RDWR,&glint_right_user_data);
}

/* camera power pin	*/
struct gpio_pin_user_data camera_power_user_data = 
{
	"cpower",
	GPIOE,
	GPIO_Pin_12,
	GPIO_Mode_Out_PP,
	GPIO_Speed_50MHz,
	RCC_APB2Periph_GPIOE,
	0,
};
gpio_device cpower_device;

void rt_hw_camera_power_register(void)
{
	cpower_device.ops = &gpio_pin_user_ops;
	rt_hw_gpio_register(&cpower_device,camera_power_user_data.name,RT_DEVICE_FLAG_RDWR,&camera_power_user_data);
}

/* camera power pin	*/
struct gpio_pin_user_data run_led_user_data = 
{
	"ledf",
	GPIOE,
	GPIO_Pin_13,
	GPIO_Mode_Out_PP,
	GPIO_Speed_50MHz,
	RCC_APB2Periph_GPIOE,
	0,
};
gpio_device runled_device;

void rt_hw_run_led_register(void)
{
	runled_device.ops = &gpio_pin_user_ops;
	rt_hw_gpio_register(&runled_device,run_led_user_data.name,RT_DEVICE_FLAG_RDWR,&run_led_user_data);
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
FINSH_FUNCTION_EXPORT(led1, led1[0 1]);

void led_run()
{
	rt_device_t led = RT_NULL;
	static rt_uint8_t status = 1;
	static rt_uint32_t	time_glint = 0;

	time_glint++;
	if(time_glint == 0xffff)
	{
		time_glint = 0;
		status = !status;
		led = rt_device_find("ledf");
		
		rt_device_write(led,0,&status,1);
	}
}
void ledsys()
{
	rt_thread_idle_sethook(led_run);
}
FINSH_FUNCTION_EXPORT(ledsys, ledsys());


#endif
