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
#include "gpio_pin.h"
#include "local.h"

extern rt_device_t rtc_device;

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
  NVIC_InitTypeDef nvic_init_structure;
  struct gpio_exti_user_data* user = (struct gpio_exti_user_data *)gpio->parent.user_data;
	
  nvic_init_structure.NVIC_IRQChannel = user->nvic_channel;
  nvic_init_structure.NVIC_IRQChannelPreemptionPriority = user->nvic_preemption_priority;
  nvic_init_structure.NVIC_IRQChannelSubPriority = user->nvic_subpriority;
  nvic_init_structure.NVIC_IRQChannelCmd = new_status;
  NVIC_Init(&nvic_init_structure);
}

static void __gpio_exti_configure(gpio_device *gpio,FunctionalState new_status)
{	
  EXTI_InitTypeDef exti_init_structure;
  struct gpio_exti_user_data* user = (struct gpio_exti_user_data *)gpio->parent.user_data;
  EXTI_StructInit(&exti_init_structure);
  exti_init_structure.EXTI_Line = user->exti_line;
  exti_init_structure.EXTI_Mode = user->exti_mode;
  exti_init_structure.EXTI_Trigger = user->exti_trigger;
  exti_init_structure.EXTI_LineCmd = new_status;
  EXTI_Init(&exti_init_structure);
}

static void __gpio_pin_configure(gpio_device *gpio)
{
  GPIO_InitTypeDef gpio_init_structure;
  struct gpio_exti_user_data *user = (struct gpio_exti_user_data*)gpio->parent.user_data;
  GPIO_StructInit(&gpio_init_structure);
  RCC_APB2PeriphClockCmd(user->gpio_clock,ENABLE);
  gpio_init_structure.GPIO_Mode = user->gpio_mode;
  gpio_init_structure.GPIO_Pin = user->gpio_pinx;
  gpio_init_structure.GPIO_Speed = user->gpio_speed;
  GPIO_Init(user->gpiox,&gpio_init_structure);
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

/*******************************************************
 *
 *             gpio exti device register
 *
 *******************************************************/

/* rfid_key_detect device pd10 */
rt_err_t rfid_key_detect_rx_ind(rt_device_t dev, rt_size_t size)
{
  gpio_device *gpio = RT_NULL;
  time_t time;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;
  /* produce mail */
  rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
  /* send mail */
  send_alarm_mail(ALARM_TYPE_RFID_KEY_DETECT, ALARM_PROCESS_FLAG_LOCAL, gpio->pin_value, time);
  return RT_EOK;
}

gpio_device rfid_key_detect_device;

struct gpio_exti_user_data rfid_key_detect_user_data = 
{
  DEVICE_NAME_RFID_KEY_DETECT,
  GPIOD,
  GPIO_Pin_10,
  GPIO_Mode_IPD,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOD |RCC_APB2Periph_AFIO,
  GPIO_PortSourceGPIOD,
  GPIO_PinSource10,
  EXTI_Line10,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Rising,
  EXTI15_10_IRQn,
  1,
  5,
  rfid_key_detect_rx_ind,
};

void rt_hw_rfid_key_detect_register(void)
{
  gpio_device *gpio_device = &rfid_key_detect_device;
  struct gpio_exti_user_data *gpio_user_data = &rfid_key_detect_user_data;

  gpio_device->ops = &gpio_exti_user_ops;

  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), gpio_user_data);
  rt_device_set_rx_indicate((rt_device_t)gpio_device, gpio_user_data->gpio_exti_rx_indicate);
}

/* motor_status device pd8 */
rt_err_t motor_status_rx_ind(rt_device_t dev, rt_size_t size)
{
  gpio_device *gpio = RT_NULL;
  time_t time;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;
  /* produce mail */
  rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
  /* send mail */
  send_alarm_mail(ALARM_TYPE_MOTOR_STATUS, ALARM_PROCESS_FLAG_LOCAL, gpio->pin_value, time);

  return RT_EOK;
}

gpio_device motor_status_device;

struct gpio_exti_user_data motor_status_user_data = 
{
  DEVICE_NAME_MOTOR_STATUS,
  GPIOD,
  GPIO_Pin_8,
  GPIO_Mode_IPD,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOD |RCC_APB2Periph_AFIO,
  GPIO_PortSourceGPIOD,
  GPIO_PinSource8,
  EXTI_Line8,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Rising,
  EXTI9_5_IRQn,
  1,
  5,
  motor_status_rx_ind,
};

void rt_hw_motor_status_register(void)
{
  gpio_device *gpio_device = &motor_status_device;
  struct gpio_exti_user_data *gpio_user_data = &motor_status_user_data;

  gpio_device->ops = &gpio_exti_user_ops;
  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), gpio_user_data);
  rt_device_set_rx_indicate((rt_device_t)gpio_device, gpio_user_data->gpio_exti_rx_indicate);
}
/* camera_photosensor device pb0 */
rt_err_t camera_photosensor_rx_ind(rt_device_t dev, rt_size_t size)
{
  gpio_device *gpio = RT_NULL;
  time_t time;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;
  /* produce mail */
  rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
  /* send mail */
  send_alarm_mail(ALARM_TYPE_CAMERA_PHOTOSENSOR, ALARM_PROCESS_FLAG_LOCAL, gpio->pin_value, time);

  return RT_EOK;
}

gpio_device camera_photosensor_device;

struct gpio_exti_user_data camera_photosensor_user_data = 
{
  DEVICE_NAME_CAMERA_PHOTOSENSOR,
  GPIOB,
  GPIO_Pin_0,
  GPIO_Mode_IPD,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOB |RCC_APB2Periph_AFIO,
  GPIO_PortSourceGPIOB,
  GPIO_PinSource0,
  EXTI_Line0,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Rising,
  EXTI0_IRQn,
  1,
  5,
  camera_photosensor_rx_ind,
};

void rt_hw_camera_photosensor_register(void)
{
  gpio_device *gpio_device = &camera_photosensor_device;
  struct gpio_exti_user_data *gpio_user_data = &camera_photosensor_user_data;

  gpio_device->ops = &gpio_exti_user_ops;
  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), gpio_user_data);
  rt_device_set_rx_indicate((rt_device_t)gpio_device, gpio_user_data->gpio_exti_rx_indicate);
}
#define CM_IR_SAMPLE_TIME               20
/* camera_irdasensor device pb0 */
rt_err_t camera_irdasensor_rx_ind(rt_device_t dev, rt_size_t size)
{
  gpio_device *gpio = RT_NULL;
  time_t time;
  static time_t time_old = 0;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;
  /* produce mail */
  rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
  
  if ((time - time_old) > CM_IR_SAMPLE_TIME || (0 == time_old))//0 == time_old open electrify power
  {
     rt_kprintf("&&time_old = %d time = %d\n",time_old,time);
     time_old = time;
  }
  else
  {
    rt_kprintf("time_old = %d time = %d\n",time_old,time);
  //  time_old = time;
    return RT_EOK;
  }
  /* send mail */
  send_alarm_mail(ALARM_TYPE_CAMERA_IRDASENSOR, ALARM_PROCESS_FLAG_SMS | ALARM_PROCESS_FLAG_GPRS | ALARM_PROCESS_FLAG_LOCAL, gpio->pin_value, time);

  return RT_EOK;
}

gpio_device camera_irdasensor_device;

struct gpio_exti_user_data camera_irdasensor_user_data = 
{
  DEVICE_NAME_CAMERA_IRDASENSOR,
  GPIOB,
  GPIO_Pin_1,
  GPIO_Mode_IPD,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOB |RCC_APB2Periph_AFIO,
  GPIO_PortSourceGPIOB,
  GPIO_PinSource1,
  EXTI_Line1,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Rising,
  EXTI1_IRQn,
  1,
  5,
  camera_irdasensor_rx_ind,
};

void rt_hw_camera_irdasensor_register(void)
{
  gpio_device *gpio_device = &camera_irdasensor_device;
  struct gpio_exti_user_data *gpio_user_data = &camera_irdasensor_user_data;

  gpio_device->ops = &gpio_exti_user_ops;
  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), gpio_user_data);
  rt_device_set_rx_indicate((rt_device_t)gpio_device, gpio_user_data->gpio_exti_rx_indicate);
}

/* gsm_ring device */
rt_err_t gsm_ring_rx_ind(rt_device_t dev, rt_size_t size)
{
  gpio_device *gpio = RT_NULL;
  time_t time;
  uint8_t dat;
  rt_device_t device = RT_NULL;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;
  /* produce mail */
  rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
  device = rt_device_find(DEVICE_NAME_GSM_STATUS);
  if (device != RT_NULL)
  {
    rt_device_read(device,0,&dat,0);
    if (dat == 0)
    {
      // if gsm status is 0, this alarm is ignore
      return RT_EOK;
    }
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("the gpio device %s is not found!\n", DEVICE_NAME_GSM_STATUS);
#endif
  }
  /* send mail */
  send_alarm_mail(ALARM_TYPE_GSM_RING, ALARM_PROCESS_FLAG_LOCAL, gpio->pin_value, time);

  return RT_EOK;
}

gpio_device gsm_ring_device;

struct gpio_exti_user_data gsm_ring_user_data = 
{
  DEVICE_NAME_GSM_RING,
  GPIOD,
  GPIO_Pin_13,
  GPIO_Mode_IPU,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOD |RCC_APB2Periph_AFIO,
  GPIO_PortSourceGPIOD,
  GPIO_PinSource13,
  EXTI_Line13,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Falling,
  EXTI15_10_IRQn,
  1,
  5,
  gsm_ring_rx_ind,
};

void rt_hw_gsm_ring_register(void)
{
  gpio_device *gpio_device = &gsm_ring_device;
  struct gpio_exti_user_data *gpio_user_data = &gsm_ring_user_data;

  gpio_device->ops = &gpio_exti_user_ops;
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), gpio_user_data);
  rt_device_set_rx_indicate((rt_device_t)gpio_device, gpio_user_data->gpio_exti_rx_indicate);
}
/* lock_gate device */
rt_err_t lock_gate_rx_ind(rt_device_t dev, rt_size_t size)
{
  gpio_device *gpio = RT_NULL;
  time_t time;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;
  /* produce mail */
  rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
  if (lock_gate_timer != RT_NULL)
  {
    return RT_EOK;
  }
  /* send mail */
  send_alarm_mail(ALARM_TYPE_LOCK_GATE, ALARM_PROCESS_FLAG_GPRS | ALARM_PROCESS_FLAG_LOCAL, gpio->pin_value, time);

  return RT_EOK;
}

gpio_device lock_gate_device;

struct gpio_exti_user_data lock_gate_user_data = 
{
  DEVICE_NAME_LOCK_GATE,
  GPIOD,
  GPIO_Pin_12,
  GPIO_Mode_IPD,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOD |RCC_APB2Periph_AFIO,
  GPIO_PortSourceGPIOD,
  GPIO_PinSource12,
  EXTI_Line12,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Rising,
  EXTI15_10_IRQn,
  1,
  5,
  lock_gate_rx_ind,
};

void rt_hw_lock_gate_register(void)
{
  gpio_device *gpio_device = &lock_gate_device;
  struct gpio_exti_user_data *gpio_user_data = &lock_gate_user_data;

  gpio_device->ops = &gpio_exti_user_ops;
  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), gpio_user_data);
  rt_device_set_rx_indicate((rt_device_t)gpio_device, gpio_user_data->gpio_exti_rx_indicate);
}

/* lock_shell device */
rt_err_t lock_shell_rx_ind(rt_device_t dev, rt_size_t size)
{
  static time_t time_old = 0;
  gpio_device *gpio = RT_NULL;
  time_t time;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;
  /* produce mail */
  rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
  if ((time - time_old) > ALARM_INTERVAL)
  {
    time_old = time;
  }
  else
  {
    time_old = time;
    return RT_EOK;
  }
  /* send mail */
  send_alarm_mail(ALARM_TYPE_LOCK_SHELL, ALARM_PROCESS_FLAG_SMS | ALARM_PROCESS_FLAG_GPRS | ALARM_PROCESS_FLAG_LOCAL, gpio->pin_value, time);

  return RT_EOK;
}

gpio_device lock_shell_device;

struct gpio_exti_user_data lock_shell_user_data = 
{
  DEVICE_NAME_LOCK_SHELL,
  GPIOD,
  GPIO_Pin_11,
  GPIO_Mode_IPD,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOD |RCC_APB2Periph_AFIO,
  GPIO_PortSourceGPIOD,
  GPIO_PinSource11,
  EXTI_Line11,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Rising,
  EXTI15_10_IRQn,
  1,
  5,
  lock_shell_rx_ind,
};

void rt_hw_lock_shell_register(void)
{
  gpio_device *gpio_device = &lock_shell_device;
  struct gpio_exti_user_data *gpio_user_data = &lock_shell_user_data;

  gpio_device->ops = &gpio_exti_user_ops;
  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), gpio_user_data);
  rt_device_set_rx_indicate((rt_device_t)gpio_device, gpio_user_data->gpio_exti_rx_indicate);
}

/* gate temperature device register */
rt_err_t gate_temperature_rx_ind(rt_device_t dev, rt_size_t size)
{
  static time_t time_old = 0;
  gpio_device *gpio = RT_NULL;
  time_t time;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;
  /* produce mail */
  rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
  if ((time - time_old) > ALARM_INTERVAL)
  {
    time_old = time;
  }
  else
  {
    time_old = time;
    return RT_EOK;
  }
  /* send mail */
  send_alarm_mail(ALARM_TYPE_GATE_TEMPERATURE, ALARM_PROCESS_FLAG_SMS | ALARM_PROCESS_FLAG_GPRS | ALARM_PROCESS_FLAG_LOCAL, gpio->pin_value, time);

  return RT_EOK;
}

gpio_device gate_temperature_device;

struct gpio_exti_user_data gate_temperature_user_data = 
{
  DEVICE_NAME_GATE_TEMPERATRUE,
  GPIOD,
  GPIO_Pin_15,
  GPIO_Mode_IPD,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO,
  GPIO_PortSourceGPIOD,
  GPIO_PinSource15,
  EXTI_Line15,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Rising,
  EXTI15_10_IRQn,
  1,
  5,
  gate_temperature_rx_ind,
};

void rt_hw_gate_temperature_register(void)
{
  gpio_device *gpio_device = &gate_temperature_device;
  struct gpio_exti_user_data *gpio_user_data = &gate_temperature_user_data;

  gpio_device->ops = &gpio_exti_user_ops;
  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), gpio_user_data);
  rt_device_set_rx_indicate((rt_device_t)gpio_device, gpio_user_data->gpio_exti_rx_indicate);
}
/* lock temperature device register */
rt_err_t lock_temperature_rx_ind(rt_device_t dev, rt_size_t size)
{
  static time_t time_old = 0;
  gpio_device *gpio = RT_NULL;
  time_t time;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;
  /* produce mail */
  rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
  if ((time - time_old) > ALARM_INTERVAL)
  {
    time_old = time;
  }
  else
  {
    time_old = time;
    return RT_EOK;
  }
  /* send mail */
  send_alarm_mail(ALARM_TYPE_LOCK_TEMPERATURE, ALARM_PROCESS_FLAG_SMS | ALARM_PROCESS_FLAG_GPRS | ALARM_PROCESS_FLAG_LOCAL, gpio->pin_value, time);

  return RT_EOK;
}

gpio_device lock_temperature_device;

struct gpio_exti_user_data lock_temperature_user_data = 
{
  DEVICE_NAME_LOCK_TEMPERATRUE,
  GPIOD,
  GPIO_Pin_14,
  GPIO_Mode_IPD,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO,
  GPIO_PortSourceGPIOD,
  GPIO_PinSource14,
  EXTI_Line14,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Rising,
  EXTI15_10_IRQn,
  1,
  5,
  lock_temperature_rx_ind,
};

void rt_hw_lock_temperature_register(void)
{
  gpio_device *gpio_device = &lock_temperature_device;
  struct gpio_exti_user_data *gpio_user_data = &lock_temperature_user_data;

  gpio_device->ops = &gpio_exti_user_ops;
  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), gpio_user_data);
  rt_device_set_rx_indicate((rt_device_t)gpio_device, gpio_user_data->gpio_exti_rx_indicate);
}
/* battery switch device register */
rt_err_t battery_switch_rx_ind(rt_device_t dev, rt_size_t size)
{
  static time_t time_old = 0;
  gpio_device *gpio = RT_NULL;
  time_t time;

  RT_ASSERT(dev != RT_NULL);
  gpio = (gpio_device *)dev;
  /* produce mail */
  rt_device_control(rtc_device, RT_DEVICE_CTRL_RTC_GET_TIME, &time);
  if ((time - time_old) > ALARM_INTERVAL)
  {
    time_old = time;
  }
  else
  {
    time_old = time;
    return RT_EOK;
  }
  /* send mail */
  send_alarm_mail(ALARM_TYPE_BATTERY_SWITCH, ALARM_PROCESS_FLAG_LOCAL, gpio->pin_value, time);

  return RT_EOK;
}

gpio_device battery_switch_device;

struct gpio_exti_user_data battery_switch_user_data = 
{
  DEVICE_NAME_BATTERY_SWITCH,
  GPIOD,
  GPIO_Pin_9,
  GPIO_Mode_IPD,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO,
  GPIO_PortSourceGPIOD,
  GPIO_PinSource9,
  EXTI_Line9,
  EXTI_Mode_Interrupt,
  EXTI_Trigger_Rising_Falling,
  EXTI9_5_IRQn,
  1,
  5,
  battery_switch_rx_ind,
};

void rt_hw_battery_switch_register(void)
{
  gpio_device *gpio_device = &battery_switch_device;
  struct gpio_exti_user_data *gpio_user_data = &battery_switch_user_data;

  gpio_device->ops = &gpio_exti_user_ops;
  
  rt_hw_gpio_register(gpio_device, gpio_user_data->name, (RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX), gpio_user_data);
  rt_device_set_rx_indicate((rt_device_t)gpio_device, gpio_user_data->gpio_exti_rx_indicate);
}

/* key2 device */
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
  EXTI_Trigger_Rising,
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
