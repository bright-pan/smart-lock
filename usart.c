/*
 * File      : usart.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 * 2010-03-29     Bernard      remove interrupt Tx and DMA Rx mode
 */

#include "usart.h"
#include <rtdevice.h>
#include <stm32f10x_dma.h>

/*
 * Use UART1 as console output and finsh input
 * interrupt Rx and poll Tx (stream mode)
 *
 * Use UART2 with interrupt Rx and poll Tx
 * Use UART3 with DMA Tx and interrupt Rx -- DMA channel 2
 *
 * USART DMA setting on STM32
 * USART1 Tx --> DMA Channel 4
 * USART1 Rx --> DMA Channel 5
 * USART2 Tx --> DMA Channel 7
 * USART2 Rx --> DMA Channel 6
 * USART3 Tx --> DMA Channel 2
 * USART3 Rx --> DMA Channel 3
 */


struct serial_user_data
{
  USART_TypeDef *usart;
  DMA_Channel_TypeDef *usart_tx_dma_channel;
  DMA_Channel_TypeDef *usart_rx_dma_channel;
};

#define USART1_DR_Base  0x40013804
#define USART2_DR_Base  0x40004404
#define USART3_DR_Base  0x40004804

/* USART1_REMAP = 0 */
#define USART1_GPIO_TX		GPIO_Pin_9
#define USART1_GPIO_RX		GPIO_Pin_10
#define USART1_GPIO			GPIOA
#define RCC_APBPeriph_USART1	RCC_APB2Periph_USART1
#define USART1_TX_DMA		DMA1_Channel4
#define USART1_RX_DMA		DMA1_Channel5

#if defined(STM32F10X_LD) || defined(STM32F10X_MD) || defined(STM32F10X_CL)
#define USART2_GPIO_TX	    GPIO_Pin_5
#define USART2_GPIO_RX	    GPIO_Pin_6
#define USART2_GPIO	    	GPIOD
#define RCC_APBPeriph_USART2	RCC_APB1Periph_USART2
#else /* for STM32F10X_HD */
/* USART2_REMAP = 0 */
#define USART2_GPIO_TX		GPIO_Pin_2
#define USART2_GPIO_RX		GPIO_Pin_3
#define USART2_GPIO			GPIOA
#define RCC_APBPeriph_USART2	RCC_APB1Periph_USART2
#define USART2_TX_DMA		DMA1_Channel7
#define USART2_RX_DMA		DMA1_Channel6
#endif

/* USART3_REMAP[1:0] = 00 */
#define USART3_GPIO_RX		GPIO_Pin_11
#define USART3_GPIO_TX		GPIO_Pin_10
#define USART3_GPIO			GPIOB
#define RCC_APBPeriph_USART3	RCC_APB1Periph_USART3
#define USART3_TX_DMA		DMA1_Channel2
#define USART3_RX_DMA		DMA1_Channel3


#ifdef RT_USING_USART1

struct serial_user_data usart1_user_data = 
{
  USART1,
  USART1_TX_DMA,
  USART1_RX_DMA,
};
struct serial_ringbuffer usart1_int_rx;
struct serial_ringbuffer usart1_int_tx;

rt_serial_t serial_device_usart1;

#endif

#ifdef RT_USING_USART2

#endif

#ifdef RT_USING_USART3

#endif


static void RCC_Configuration(void)
{
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

#ifdef RT_USING_USART1
  /* Enable USART1 and GPIOA clocks */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
  /* DMA clock enable */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
#endif

#ifdef RT_USING_USART2

#if (defined(STM32F10X_LD) || defined(STM32F10X_MD) || defined(STM32F10X_CL))
  /* Enable AFIO and GPIOD clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOD, ENABLE);

  /* Enable the USART2 Pins Software Remapping */
  GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);
#else
  /* Enable AFIO and GPIOA clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE);
#endif

  /* Enable USART2 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
#endif

#ifdef RT_USING_USART3
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  /* Enable USART3 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

  /* DMA clock enable */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
#endif
}

static void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

#ifdef RT_USING_USART1
  /* Configure USART1 Rx (PA.10) as input floating */
  GPIO_InitStructure.GPIO_Pin = USART1_GPIO_RX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(USART1_GPIO, &GPIO_InitStructure);

  /* Configure USART1 Tx (PA.09) as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = USART1_GPIO_TX;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(USART1_GPIO, &GPIO_InitStructure);
#endif

#ifdef RT_USING_USART2
  /* Configure USART2 Rx as input floating */
  GPIO_InitStructure.GPIO_Pin = USART2_GPIO_RX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(USART2_GPIO, &GPIO_InitStructure);

  /* Configure USART2 Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = USART2_GPIO_TX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(USART2_GPIO, &GPIO_InitStructure);
#endif

#ifdef RT_USING_USART3
  /* Configure USART3 Rx as input floating */
  GPIO_InitStructure.GPIO_Pin = USART3_GPIO_RX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(USART3_GPIO, &GPIO_InitStructure);

  /* Configure USART3 Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = USART3_GPIO_TX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(USART3_GPIO, &GPIO_InitStructure);
#endif
}

static void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

#ifdef RT_USING_USART1
  /* Enable the USART1 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  /* Enable the DMA1 Channel2 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
#endif

#ifdef RT_USING_USART2
  /* Enable the USART2 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
#endif

#ifdef RT_USING_USART3
  /* Enable the USART3 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the DMA1 Channel2 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
#endif
}

static void DMA_Configuration(void)
{
#if defined (RT_USING_USART1)
  DMA_InitTypeDef DMA_InitStructure;

  /* fill init structure */
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

  /* DMA1 Channel4 (triggered by USART1 Tx event) Config */
  DMA_DeInit(USART1_TX_DMA);
  DMA_InitStructure.DMA_PeripheralBaseAddr = USART1_DR_Base;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  /* As we will set them before DMA actually enabled, the DMA_MemoryBaseAddr
   * and DMA_BufferSize are meaningless. So just set them to proper values
   * which could make DMA_Init happy.
   */
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;
  DMA_InitStructure.DMA_BufferSize = 1;
  DMA_Init(USART1_TX_DMA, &DMA_InitStructure);
  DMA_ITConfig(USART1_TX_DMA, DMA_IT_TC | DMA_IT_TE, ENABLE);
  DMA_ClearFlag(DMA1_FLAG_TC4);
#endif
#if defined (RT_USING_USART3)
  DMA_InitTypeDef DMA_InitStructure;

  /* fill init structure */
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

  /* DMA1 Channel5 (triggered by USART3 Tx event) Config */
  DMA_DeInit(USART3_TX_DMA);
  DMA_InitStructure.DMA_PeripheralBaseAddr = USART3_DR_Base;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  /* As we will set them before DMA actually enabled, the DMA_MemoryBaseAddr
   * and DMA_BufferSize are meaningless. So just set them to proper values
   * which could make DMA_Init happy.
   */
  DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;
  DMA_InitStructure.DMA_BufferSize = 1;
  DMA_Init(USART3_TX_DMA, &DMA_InitStructure);
  DMA_ITConfig(USART3_TX_DMA, DMA_IT_TC | DMA_IT_TE, ENABLE);
  DMA_ClearFlag(DMA1_FLAG_TC2);
#endif
}
rt_err_t usart_ops_control(struct rt_serial_device *serial, int cmd, void *arg)
{
  
  
  return RT_EOK;
}
/*
 * poll send data to usart
 *
 */

int usart_ops_putc(struct rt_serial_device *serial, char c)
{
  struct serial_user_data *user_data;
  
  user_data = serial->parent.user_data;
  USART_SendData(user_data->usart, c);  
  while (USART_GetFlagStatus(user_data->usart, USART_FLAG_TXE) == RESET)
  {
    ;
  }

  return RT_EOK;
}

/*
 * poll get data from usart
 *
 */
int usart_ops_getc(struct rt_serial_device *serial)
{
  
  struct serial_user_data *user_data;
  
  user_data = serial->parent.user_data;
  if (USART_GetFlagStatus(user_data->usart, USART_FLAG_RXNE) != RESET)
  {
    return USART_ReceiveData(user_data->usart);
  }
  
  return -1;
}

rt_size_t usart_ops_dma_transmit(struct rt_serial_device *serial, const char *buf, rt_size_t size)
{
  struct serial_user_data *user_data;
  
  user_data = serial->parent.user_data;
  
  /* disable DMA */
  DMA_Cmd(user_data->usart_tx_dma_channel, DISABLE);

  /* set buffer address */
  user_data->usart_tx_dma_channel->CMAR = (uint32_t)buf;
  /* set size */
  user_data->usart_tx_dma_channel->CNDTR = size;

  /* enable DMA */
  DMA_Cmd(user_data->usart_tx_dma_channel, ENABLE);
  
  return size;
}


rt_err_t usart_ops_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
  struct serial_user_data *user_data;
  
  user_data = serial->parent.user_data;

  USART_InitTypeDef USART_InitStructure;
  USART_ClockInitTypeDef USART_ClockInitStructure;

  RCC_Configuration();

  GPIO_Configuration();

  NVIC_Configuration();

  DMA_Configuration();
  /* uart init */
  USART_InitStructure.USART_BaudRate = cfg->baud_rate;
  switch(cfg->data_bits)
  {
    case 8:
      {
        USART_InitStructure.USART_WordLength 	= USART_WordLength_8b;
        break;
      }
    case 9:
      {
        USART_InitStructure.USART_WordLength 	= USART_WordLength_9b;
        break;
      }
    default :
      {
#ifndef RT_USING_FINSH
        rt_kprintf("data bit set error\n");
#endif
        break;
      }
  }

  switch(cfg->stop_bits)
  {
    case 0:
      {
        USART_InitStructure.USART_StopBits 	= USART_StopBits_1;
        break;
      }
    case 1:
      {
        USART_InitStructure.USART_StopBits 	= USART_StopBits_2;
        break;
      }
    case 2:
      {
        USART_InitStructure.USART_StopBits 	= USART_StopBits_0_5;
        break;
      }
    case 3:
      {
        USART_InitStructure.USART_StopBits 	= USART_StopBits_1_5;
        break;
      }
    default :
      {
#ifndef RT_USING_FINSH
        rt_kprintf("stopbits bit set error\n");
#endif
        break;
      }
  }

  switch(cfg->parity)
  {
    case 0:
      {
        USART_InitStructure.USART_Parity 	= USART_Parity_No;
        break;
      }
    case 1:
      {
        USART_InitStructure.USART_Parity 	= USART_Parity_Odd;
        break;
      }
    case 2:
      {
        USART_InitStructure.USART_Parity 	= USART_Parity_Even;
        break;
      }
    default :
      {
#ifndef RT_USING_FINSH
        rt_kprintf("data bit set error\n");
#endif
        break;
      }
  }

  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
  USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
  USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
  USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;
  USART_Init(USART1, &USART_InitStructure);
  USART_ClockInit(user_data->usart, &USART_ClockInitStructure);

  /* register uart1 */
  /*  rt_hw_serial_register(&uart1_device, "uart1",
      RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
      &uart1);*/
  USART_Cmd(user_data->usart, ENABLE);
  if (serial->parent.flag & RT_DEVICE_FLAG_INT_RX)
  {
    /* enable interrupt */
    USART_ITConfig(user_data->usart, USART_IT_RXNE, ENABLE);
  }
  
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    /* Enable USART3 DMA Tx request */
    USART_DMACmd(user_data->usart, USART_DMAReq_Tx , ENABLE);
  }

  return RT_EOK;
}

struct serial_configure serial_configure_default = RT_SERIAL_CONFIG_DEFAULT;

struct rt_uart_ops usart_ops =
{
  usart_ops_configure,
  usart_ops_control,
  usart_ops_putc,
  usart_ops_getc,
  usart_ops_dma_transmit,
  
};



/*
 * Init all related hardware in here
 * rt_hw_serial_init() will register all supported USART device
 */
void rt_hw_usart_init()
{
#ifdef RT_USING_USART1
  /* register uart1 */
  serial_device_usart1.ops = &usart_ops;
  serial_device_usart1.int_rx = &usart1_int_rx;
  serial_device_usart1.int_rx = &usart1_int_tx;
  serial_device_usart1.config = serial_configure_default;
  rt_hw_serial_register(&serial_device_usart1, "uart1",
                        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_TX,
                        &usart1_user_data);
#endif

#ifdef RT_USING_USART2
  /* register uart2 */
  /*rt_hw_serial_register(&uart2_device, "uart2",
    RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
    &uart2);*/
#endif

#ifdef RT_USING_USART3
  /* register uart3 */
  /*rt_hw_serial_register(&uart3_device, "uart3",
    RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_TX,
    &uart3);*/

#endif
}

#ifdef RT_USING_FINSH
#include <finsh.h>

void usartx(char *str)
{
  rt_device_t usart;

  usart = rt_device_find("uart2");
  rt_device_write(usart,0,str,20);
}
FINSH_FUNCTION_EXPORT(usartx, set led[0 - 1] on[1] or off[0].)
#endif

