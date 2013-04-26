/*********************************************************************
 * Filename:      gpio.c
 *
 * Description:    
 *
 * Author:        wangzw <wangzw@yuettak.com>
 * Created at:    2013-04-25 15:21:03
 *                
 * Modify:
 *
 * 2013-04-25 Bright Pan <loststriker@gmail.com>
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "gpio.h"

static rt_err_t rt_gpio_init(struct rt_device *dev)
{

  rt_err_t result = RT_EOK;
  gpio_device *gpio = RT_NULL;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;

  if (!(dev->flag & RT_DEVICE_FLAG_ACTIVATED))//uninitiated
  {
    /* apply configuration */
    if (gpio->ops->configure)
    {
      result = gpio->ops->configure(gpio);
    }
    if (result != RT_EOK)
    {
      return result;
    }
    /* set activated */
    dev->flag |= RT_DEVICE_FLAG_ACTIVATED;
  }
  return result;
}
/*
 * gpio open
 */
static rt_err_t rt_gpio_open(struct rt_device *dev,rt_uint16_t oflag)
{
  rt_err_t result = RT_EOK;
  rt_uint32_t int_flags = 0;
  gpio_device *gpio;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;

  if(dev->flag & RT_DEVICE_FLAG_INT_RX) //interrupt intpur
  {
    int_flags |= RT_DEVICE_FLAG_INT_RX;
  }
  if (int_flags)
  {
    gpio->ops->control(gpio, RT_DEVICE_CTRL_SET_INT, (void *)int_flags);
  }
	
  return result;
}
/*
 * gpio close
 */
static rt_err_t rt_gpio_close(struct rt_device *dev)
{
  gpio_device *gpio = RT_NULL;
  rt_uint32_t int_flags = 0;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;

  if (dev->flag & RT_DEVICE_FLAG_INT_RX)
  {
    int_flags |= RT_DEVICE_FLAG_INT_RX;
  }
  if (int_flags)
  {
    gpio->ops->control(gpio, RT_DEVICE_CTRL_CLR_INT, (void *)int_flags);
  }
	
  return RT_EOK;
}
/*
 * gpio read
 */
static rt_size_t rt_gpio_read(struct rt_device *dev,
                              rt_off_t          pos,
                              void             *buffer,
                              rt_size_t         size )
{
  gpio_device *gpio = RT_NULL;
  RT_ASSERT(dev != RT_NULL);

  gpio = (gpio_device *)dev;
  *(rt_uint8_t *)buffer = gpio->ops->intput(gpio);

  return RT_EOK;
}
/*
 * gpio write
 */
static rt_size_t rt_gpio_write(struct rt_device *dev,
                                 rt_off_t pos,
                                 const void *buffer,
                                 rt_size_t size)
{
  gpio_device *gpio = RT_NULL;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;
  gpio->ops->out(gpio,*(rt_uint8_t *)buffer);
	
  return RT_EOK;
}
/*
 * gpio coltrol
 */
static rt_err_t rt_gpio_control(struct rt_device *dev,
                                  rt_uint8_t cmd,
                                  void *args)
{
  gpio_device *gpio = RT_NULL;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;

  switch (cmd)
  {
    case RT_DEVICE_CTRL_SUSPEND:
      {
        /* suspend device */
        dev->flag |= RT_DEVICE_FLAG_SUSPENDED;
        break;
      }
    case RT_DEVICE_CTRL_RESUME:
      {
        /* resume device */
        dev->flag &= ~RT_DEVICE_FLAG_SUSPENDED;
        break;
      }
    case RT_DEVICE_CTRL_CONFIG:
      {
        /* configure device */
        gpio->ops->configure(gpio);
        break;
      }
    default:
      {
        /* configure device */
        gpio->ops->control(gpio, cmd, args);
        break;
      }
  }
  return RT_EOK;
}

/*
 * serial register
 */
rt_err_t rt_hw_gpio_register(gpio_device *gpio,
                             const char *name,
                             rt_uint32_t flag,
                             void *data)
{
  struct rt_device *device = RT_NULL;
  RT_ASSERT(gpio != RT_NULL);
	
  device = &(gpio->parent);						
	
  device->type        = RT_Device_Class_Char;
  device->rx_indicate = RT_NULL;
  device->tx_complete = RT_NULL;
	
  device->init 		= rt_gpio_init;
  device->open   = rt_gpio_open;
  device->close 	= rt_gpio_close;
  device->read    = rt_gpio_read;
  device->write    = rt_gpio_write;
  device->control    	= rt_gpio_control;
  device->user_data   = data;
	
  /* register a character device */
  return rt_device_register(device, name, flag);
}

/*
 * gpio interrupt input deal
 */
void rt_hw_gpio_isr(gpio_device *gpio)
{
	 
  /* interrupt mode receive */
  RT_ASSERT(gpio->parent.flag & RT_DEVICE_FLAG_INT_RX);

  gpio->pin_value = gpio->ops->intput(gpio);
  if(gpio->parent.rx_indicate != RT_NULL)
  {
    gpio->parent.rx_indicate(&(gpio->parent), 1);
  }
}


