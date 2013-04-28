/*********************************************************************
 * Filename:      gpio_adc.c
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

#include "gpio_adc.h"

struct gpio_adc_user_data
{
  const char name[RT_NAME_MAX];
  /* gpio */
  GPIO_TypeDef* gpiox;//port
  rt_uint16_t gpio_pinx;//pin
  GPIOMode_TypeDef gpio_mode;//mode
  GPIOSpeed_TypeDef gpio_speed;//speed
  rt_uint32_t gpio_clock;//clock
  /* adc */
  ADC_TypeDef *adcx;//tim[1-8]
  rt_uint32_t adc_clock_div;
  rt_uint32_t adc_rcc;
  rt_uint8_t adc_channel;
  rt_uint8_t adc_channel_rank;
  rt_uint32_t adc_mode;
  FunctionalState adc_scan_conv_mode;
  FunctionalState adc_continuous_conv_mode;
  rt_uint32_t adc_external_trig_conv;
  rt_uint32_t adc_data_align;
  rt_uint8_t adc_nbr_of_channel;
  rt_uint8_t adc_sample_time;
  __IO rt_uint16_t adc_converted_value;
  /* dma */
  DMA_Channel_TypeDef *dma_channel;
  rt_uint32_t dma_rcc;
  rt_uint32_t dma_peripheral_base_address;    
  rt_uint32_t dma_memory_base_address;
  rt_uint32_t dma_dir;
  rt_uint32_t dma_buffer_size;
  rt_uint32_t dma_peripheral_inc;
  rt_uint32_t dma_memory_inc;
  rt_uint32_t dma_peripheral_data_size;
  rt_uint32_t dma_memory_data_size;
  rt_uint32_t dma_mode;
  rt_uint32_t dma_priority;
  rt_uint32_t dma_m2m;
};
/*
 *  GPIO ops function interfaces
 *
 */
static void __gpio_pin_configure(gpio_device *gpio)
{
  GPIO_InitTypeDef gpio_initstructure;
  struct gpio_adc_user_data *user = (struct gpio_adc_user_data*)gpio->parent.user_data;
  RCC_APB2PeriphClockCmd(user->gpio_clock,ENABLE);
  gpio_initstructure.GPIO_Mode = user->gpio_mode;
  gpio_initstructure.GPIO_Pin = user->gpio_pinx;
  gpio_initstructure.GPIO_Speed = user->gpio_speed;
  GPIO_Init(user->gpiox,&gpio_initstructure);
}
static void __gpio_dma_configure(gpio_device *gpio)
{
  DMA_InitTypeDef dma_init_structure;
  struct gpio_adc_user_data *user = (struct gpio_adc_user_data*)gpio->parent.user_data;
  RCC_AHBPeriphClockCmd(user->dma_rcc, ENABLE);
  /* DMA1 channel1 configuration ----------------------------------------------*/
  DMA_DeInit(user->dma_channel);
  dma_init_structure.DMA_PeripheralBaseAddr = user->dma_peripheral_base_address;
  dma_init_structure.DMA_MemoryBaseAddr = user->dma_memory_base_address;
  dma_init_structure.DMA_DIR = user->dma_dir;
  dma_init_structure.DMA_BufferSize = user->dma_buffer_size;
  dma_init_structure.DMA_PeripheralInc = user->dma_peripheral_inc;
  dma_init_structure.DMA_MemoryInc = user->dma_memory_inc;
  dma_init_structure.DMA_PeripheralDataSize = user->dma_peripheral_data_size;
  dma_init_structure.DMA_MemoryDataSize = user->dma_memory_data_size;
  dma_init_structure.DMA_Mode = user->dma_mode;
  dma_init_structure.DMA_Priority = user->dma_priority;
  dma_init_structure.DMA_M2M = user->dma_m2m;
  DMA_Init(user->dma_channel, &dma_init_structure);
}

static void __gpio_adc_configure(gpio_device *gpio)
{
  ADC_InitTypeDef adc_init_structure;
  struct gpio_adc_user_data *user = (struct gpio_adc_user_data*)gpio->parent.user_data;
  
  RCC_ADCCLKConfig(user->adc_clock_div);
  RCC_APB2PeriphClockCmd(user->adc_rcc, ENABLE);
  
  adc_init_structure.ADC_Mode = user->adc_mode;
  adc_init_structure.ADC_ScanConvMode = user->adc_scan_conv_mode;
  adc_init_structure.ADC_ContinuousConvMode = user->adc_continuous_conv_mode;
  adc_init_structure.ADC_ExternalTrigConv = user->adc_external_trig_conv;
  adc_init_structure.ADC_DataAlign = user->adc_data_align;
  adc_init_structure.ADC_NbrOfChannel = user->adc_nbr_of_channel;
  ADC_Init(user->adcx, &adc_init_structure);
  /* ADC1 regular channel14 configuration */
  ADC_RegularChannelConfig(user->adcx, user->adc_channel, user->adc_channel_rank, user->adc_sample_time);
}

/*
 * gpio ops configure
 */
rt_err_t gpio_adc_configure(gpio_device *gpio)
{	
  __gpio_pin_configure(gpio);
  if (gpio->parent.flag & RT_DEVICE_FLAG_DMA_RX)
  {
    __gpio_dma_configure(gpio);
  }
  __gpio_adc_configure(gpio);
  return RT_EOK;
}
rt_err_t gpio_adc_control(gpio_device *gpio, rt_uint8_t cmd, void *arg)
{
  static uint8_t calibration_flag = 0;
  struct gpio_adc_user_data *user = (struct gpio_adc_user_data*)gpio->parent.user_data;
  
  switch (cmd)
  {
    case RT_DEVICE_CTRL_ENABLE_CONVERT:
      {
        if (gpio->parent.flag & RT_DEVICE_FLAG_DMA_RX)
        {
          /* Enable DMA channel */
          DMA_Cmd(user->dma_channel, ENABLE);        
          /* Enable ADC1 DMA */
          ADC_DMACmd(user->adcx, ENABLE);
        }
        /* Enable ADC1 */
        ADC_Cmd(user->adcx, ENABLE);
        if (calibration_flag == 0)
        {
          /* Enable ADC1 reset calibration register */   
          ADC_ResetCalibration(user->adcx);
          /* Check the end of ADC1 reset calibration register */
          while(ADC_GetResetCalibrationStatus(user->adcx));
          /* Start ADC1 calibration */
          ADC_StartCalibration(user->adcx);
          /* Check the end of ADC1 calibration */
          while(ADC_GetCalibrationStatus(user->adcx));
          //ADC1采样开始
          ADC_SoftwareStartConvCmd(user->adcx, ENABLE);
          calibration_flag = 1;
        }
        break;
      }
    case RT_DEVICE_CTRL_DISABLE_CONVERT:
      {
        if (gpio->parent.flag & RT_DEVICE_FLAG_DMA_RX)
        {
          DMA_Cmd(user->dma_channel, DISABLE);
          /* Enable ADC1 DMA */
          ADC_DMACmd(user->adcx, DISABLE);
        }
        /* Enable ADC1 */
        ADC_Cmd(user->adcx, DISABLE);
        calibration_flag = 0;
        break;
      }
    case RT_DEVICE_CTRL_GET_CONVERT_VALUE:
      {
        if (gpio->parent.flag & RT_DEVICE_FLAG_DMA_RX)
        {
          *(uint16_t *)arg = user->adc_converted_value;
        }
        else
        {
          *(uint16_t *)arg = ADC_GetConversionValue(user->adcx);
        }
        break;
      }
    default:
      {
#ifdef RT_USING_FINSH
        rt_kprintf("gpio adc device <%s> is not support this command!\n", user->name);
#endif
        break;
      }
  }
  return RT_EOK;
}

void gpio_adc_out(gpio_device *gpio, rt_uint8_t data)
{
  struct gpio_adc_user_data* user = (struct gpio_adc_user_data *)gpio->parent.user_data;
#ifdef RT_USING_FINSH
    rt_kprintf("gpio adc device <%s> is can`t write! please check device flag RT_DEVICE_FLAG_WRONLY\n", user->name);
#endif
}

rt_uint8_t gpio_adc_intput(gpio_device *gpio)
{
  struct gpio_adc_user_data* user = (struct gpio_adc_user_data *)gpio->parent.user_data;

  if (gpio->parent.flag & RT_DEVICE_FLAG_RDONLY)
  {
    return 1;
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("gpio adc device <%s> is can`t read! please check device flag RT_DEVICE_FLAG_RDONLY\n", user->name);
#endif
    return 0;
  }
}

struct rt_gpio_ops gpio_adc_user_ops= 
{
  gpio_adc_configure,
  gpio_adc_control,
  gpio_adc_out,
  gpio_adc_intput
};
struct gpio_adc_user_data adc11_user_data = 
{
  "adc11",
  /* gpio */
  GPIOC,
  GPIO_Pin_1,
  GPIO_Mode_AIN,
  GPIO_Speed_50MHz,
  RCC_APB2Periph_GPIOC,
  /* adc */
  ADC1,
  RCC_PCLK2_Div8,
  RCC_APB2Periph_ADC1,
  ADC_Channel_11,
  1, //adc_channel_rank
  ADC_Mode_Independent,
  DISABLE,
  ENABLE,
  ADC_ExternalTrigConv_None,
  ADC_DataAlign_Right,
  1, //adc_nbr_of_channel
  ADC_SampleTime_55Cycles5,
  0, //adc_converted_value
  /* dma */
  DMA1_Channel1,
  RCC_AHBPeriph_DMA1,
  (uint32_t)&(ADC1->DR),
  0,
  DMA_DIR_PeripheralSRC,
  1,//dma_buffer_size
  DMA_PeripheralInc_Disable,
  DMA_MemoryInc_Disable,
  DMA_PeripheralDataSize_HalfWord,
  DMA_MemoryDataSize_HalfWord,
  DMA_Mode_Circular,//dma_mode
  DMA_Priority_High,//dma_priority
  DMA_M2M_Disable,//dma_m2m
};

gpio_device adc11_device;

void rt_hw_adc11_register(void)
{
  gpio_device *adc_device = &adc11_device;
  struct gpio_adc_user_data *adc_user_data = &adc11_user_data;
  adc_user_data->dma_memory_base_address = (uint32_t)&(adc_user_data->adc_converted_value);
  adc_device->ops = &gpio_adc_user_ops;
  rt_hw_gpio_register(adc_device,adc_user_data->name, (RT_DEVICE_FLAG_RDONLY | RT_DEVICE_FLAG_DMA_RX), adc_user_data);
}

#ifdef RT_USING_FINSH
#include <finsh.h>
void adc11_get_value(void)
{
  rt_uint16_t adc_value;
  rt_device_t device = RT_NULL;
  device = rt_device_find("adc11");

  if (device != RT_NULL)
  {
    rt_device_control(device, RT_DEVICE_CTRL_GET_CONVERT_VALUE, (void *)&adc_value);
#ifdef RT_USING_FINSH
    rt_kprintf("adc11_value = 0x%x\n",adc_value);
#endif
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("the device is not found!\n");
#endif
  }
}
FINSH_FUNCTION_EXPORT(adc11_get_value, get adc11 converted value)

void adc11_enable(void)
{
  rt_device_t device = RT_NULL;
  device = rt_device_find("adc11");
  if (device != RT_NULL)
  {
    rt_device_control(device, RT_DEVICE_CTRL_ENABLE_CONVERT, (void *)0);
#ifdef RT_USING_FINSH
    rt_kprintf("adc11 is starting convert!\n");
#endif  
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("the device is not found!\n");
#endif
  }
}
FINSH_FUNCTION_EXPORT(adc11_enable, enable adc11 convert)

void adc11_disable(void)
{
  rt_device_t device = RT_NULL;
  device = rt_device_find("adc11");
  if (device != RT_NULL)
  {
    rt_device_control(device, RT_DEVICE_CTRL_DISABLE_CONVERT, (void *)0);
#ifdef RT_USING_FINSH
    rt_kprintf("adc11 is stoped convert!\n");
#endif  
  }
  else
  {
#ifdef RT_USING_FINSH
    rt_kprintf("the device is not found!\n");
#endif
  }
}
FINSH_FUNCTION_EXPORT(adc11_disable, disable adc11 convert)
#endif
