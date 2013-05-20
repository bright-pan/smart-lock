/*********************************************************************
 * Filename:      camera_uart.c
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-08 10:08:32
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#include "camera_uart.h"

/*
 * Use UART4 with interrupt Rx and poll Tx
 *
 */

/* UART5_REMAP = 0 */
#define CAMERA_UART_GPIO_PIN_TX		GPIO_Pin_12
#define CAMERA_UART_GPIO_PIN_RX		GPIO_Pin_2
#define CAMERA_UART_GPIO_TX		        GPIOC
#define CAMERA_UART_GPIO_RX		        GPIOD
#define CAMERA_UART_GPIO_RCC_TX	        RCC_APB2Periph_GPIOC
#define CAMERA_UART_GPIO_RCC_RX	        RCC_APB2Periph_GPIOD
#define CAMERA_UART_RCC	                RCC_APB1Periph_UART5
#define CAMERA_UART                       UART5

static rt_err_t camera_uart_pos_configure(struct rt_serial_device *serial, struct serial_configure *cfg);
static rt_size_t camera_uart_pos_dma_transmit(struct rt_serial_device *serial, const char *buf, rt_size_t size);
static int camera_uart_pos_getc(struct rt_serial_device *serial);
static int camera_uart_pos_putc(struct rt_serial_device *serial, char c);
static rt_err_t camera_uart_pos_control(struct rt_serial_device *serial, int cmd, void *arg);

struct rt_uart_ops camera_uart_pos =
{
  camera_uart_pos_configure,
  camera_uart_pos_control,
  camera_uart_pos_putc,
  camera_uart_pos_getc,
  camera_uart_pos_dma_transmit,
};
struct serial_configure camera_uart_default_config = RT_SERIAL_CONFIG_DEFAULT;

struct camera_uart_user_data_t camera_uart_user_data = 
{
  CAMERA_UART,
  RT_NULL,
  RT_NULL,
};
struct serial_ringbuffer camera_uart_int_rx;
struct serial_ringbuffer camera_uart_int_tx;

rt_serial_t camera_uart_device;

static void RCC_Configuration(struct rt_serial_device *serial)
{
  /*
  struct serial_user_data *user_data;
  user_data = serial->parent.user_data;
  */
  /* Enable AFIO and CAMERA_UART_GPIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | CAMERA_UART_GPIO_RCC_TX | CAMERA_UART_GPIO_RCC_RX, ENABLE);

  /* Enable CAMERA_UART_RCC clock */
  RCC_APB1PeriphClockCmd(CAMERA_UART_RCC, ENABLE);
}

static void GPIO_Configuration(struct rt_serial_device *serial)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  /*
  struct serial_user_data *user_data;
  user_data = serial->parent.user_data;
  */
  /* Configure USART2 Rx CTS as input floating */
  //GPIO_InitStructure.GPIO_Pin = CAMERA_UART_GPIO_PIN_RX | CAMERA_UART_GPIO_PIN_CTS;
  GPIO_InitStructure.GPIO_Pin = CAMERA_UART_GPIO_PIN_RX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(CAMERA_UART_GPIO_RX, &GPIO_InitStructure);

  /* Configure USART2 Tx RTS as alternate function push-pull */
  //GPIO_InitStructure.GPIO_Pin = CAMERA_UART_GPIO_PIN_TX | CAMERA_UART_GPIO_PIN_RTS;
  GPIO_InitStructure.GPIO_Pin = CAMERA_UART_GPIO_PIN_TX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(CAMERA_UART_GPIO_TX, &GPIO_InitStructure);
}

static void NVIC_Configuration(struct rt_serial_device *serial)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  /*
  struct serial_user_data *user_data;
  user_data = serial->parent.user_data;
  */
  if ((serial->parent.flag & RT_DEVICE_FLAG_INT_RX) | (serial->parent.flag & RT_DEVICE_FLAG_INT_TX))
  {
    NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }

}



static rt_err_t camera_uart_pos_control(struct rt_serial_device *serial, int cmd, void *arg)
{
  
  
  return RT_EOK;
}
/*
 * poll send data to gsm
 *
 */

static int camera_uart_pos_putc(struct rt_serial_device *serial, char c)
{
  struct camera_uart_user_data_t *user_data;
  
  user_data = serial->parent.user_data;
  USART_SendData(user_data->usart, c);
  while (USART_GetFlagStatus(user_data->usart, USART_FLAG_TXE) == RESET)
  {
    ;
  }

  return RT_EOK;
}

/*
 * poll get data from gsm
 *
 */
static int camera_uart_pos_getc(struct rt_serial_device *serial)
{
  
  struct camera_uart_user_data_t *user_data;
  
  user_data = serial->parent.user_data;
  if (USART_GetFlagStatus(user_data->usart, USART_FLAG_RXNE) != RESET)
  {
    return USART_ReceiveData(user_data->usart);
  }
  
  return -1;
}

static rt_size_t camera_uart_pos_dma_transmit(struct rt_serial_device *serial, const char *buf, rt_size_t size)
{
  struct camera_uart_user_data_t *user_data;
  
  user_data = serial->parent.user_data;
  
  /* disable DMA */
  DMA_Cmd(user_data->usart_tx_dma_channel, DISABLE);

  /* set buffer address */
  user_data->usart_tx_dma_channel->CMAR = (rt_uint32_t)buf;
  /* set size */
  user_data->usart_tx_dma_channel->CNDTR = size;

  /* enable DMA */
  DMA_Cmd(user_data->usart_tx_dma_channel, ENABLE);
  
  return size;
}


static rt_err_t camera_uart_pos_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
  USART_InitTypeDef USART_InitStructure;

  struct camera_uart_user_data_t *user_data;
  
  user_data = serial->parent.user_data;


  RCC_Configuration(serial);

  GPIO_Configuration(serial);

  NVIC_Configuration(serial);

  /* uart init */
  USART_InitStructure.USART_BaudRate = cfg->baud_rate;
  switch(cfg->data_bits)
  {
    case 8:
      {
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        break;
      }
    case 9:
      {
        USART_InitStructure.USART_WordLength = USART_WordLength_9b;
        break;
      }
    default :
      {
#ifdef RT_USING_FINSH
        rt_kprintf("data bit set error\n");
#endif
        break;
      }
  }

  switch(cfg->stop_bits)
  {
    case 0:
      {
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        break;
      }
    case 1:
      {
        USART_InitStructure.USART_StopBits = USART_StopBits_2;
        break;
      }
    case 2:
      {
        USART_InitStructure.USART_StopBits = USART_StopBits_0_5;
        break;
      }
    case 3:
      {
        USART_InitStructure.USART_StopBits = USART_StopBits_1_5;
        break;
      }
    default :
      {
#ifdef RT_USING_FINSH
        rt_kprintf("stopbits bit set error\n");
#endif
        break;
      }
  }

  switch(cfg->parity)
  {
    case 0:
      {
        USART_InitStructure.USART_Parity = USART_Parity_No;
        break;
      }
    case 1:
      {
        USART_InitStructure.USART_Parity = USART_Parity_Odd;
        break;
      }
    case 2:
      {
        USART_InitStructure.USART_Parity = USART_Parity_Even;
        break;
      }
    default :
      {
#ifdef RT_USING_FINSH
        rt_kprintf("data bit set error\n");
#endif
        break;
      }
  }
  /* set hardware flow control */
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(user_data->usart, &USART_InitStructure);
  /* enable usart */
  USART_Cmd(user_data->usart, ENABLE);
  if (serial->parent.flag & RT_DEVICE_FLAG_INT_RX)
  {
    /* enable interrupt */
    USART_ITConfig(user_data->usart, USART_IT_RXNE, ENABLE);
    USART_ITConfig(user_data->usart, USART_IT_ERR, ENABLE);
  }
  
  return RT_EOK;
}

/*
 * Init all related hardware in here
 * rt_hw_camera_uart_register() will register all supported GSM device
 */
void rt_hw_camera_uart_register(void)
{
  camera_uart_device.ops = &camera_uart_pos;
  camera_uart_device.int_rx = &camera_uart_int_rx;
  camera_uart_device.int_tx = &camera_uart_int_tx;
  camera_uart_device.config = camera_uart_default_config;
  rt_hw_serial_register(&camera_uart_device, DEVICE_NAME_CAMERA_UART,
                        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
                        &camera_uart_user_data);
  rt_kprintf("register camera usart device\n");
}

void camera_uart_device_isr(struct rt_serial_device *serial)
{
  struct camera_uart_user_data_t *user_data;
  user_data = serial->parent.user_data;

  if(USART_GetFlagStatus(user_data->usart, USART_FLAG_ORE) != RESET)
  {
    USART_ReceiveData(user_data->usart);
    while (USART_GetFlagStatus(user_data->usart, USART_FLAG_RXNE) != RESET)
    {
      USART_ReceiveData(user_data->usart);
    }
  }
  if(USART_GetFlagStatus(user_data->usart, USART_FLAG_NE) != RESET)
  {//USART_IT_NE     : Noise Error interrupt
    USART_ReceiveData(user_data->usart);
  }
  if(USART_GetFlagStatus(user_data->usart, USART_FLAG_FE) != RESET)
  {//USART_IT_FE     : Framing Error interrupt
    USART_ReceiveData(user_data->usart);
  }
  if(USART_GetFlagStatus(user_data->usart, USART_FLAG_PE) != RESET)
  {//USART_IT_PE     : Parity Error interrupt
    USART_ReceiveData(user_data->usart);
  }
  rt_hw_serial_isr(serial);
}


#ifdef RT_USING_FINSH
#include <finsh.h>

static char temp[100];

void camera_uart(char cmd, char *str)
{
  rt_device_t device;
  memset(temp, '\xFF', 100);
  device = rt_device_find(DEVICE_NAME_CAMERA_UART);
  if (device != RT_NULL)
  {
    if (cmd == 0)
    {
      rt_device_read(device, 0, temp, 20);
      temp[99] = '\0';
      rt_kprintf(temp);
    }
    else if (cmd == 1)
    {
      rt_device_write(device, 0, str, strlen(str));
    }
  }
  else
  {
    rt_kprintf("device %s is not exist!\n", DEVICE_NAME_CAMERA_UART);
  }
}
FINSH_FUNCTION_EXPORT(camera_uart, camera_uart[cmd parameters])
#endif