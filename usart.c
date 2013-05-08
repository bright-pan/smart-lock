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
#include <serial.h>
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


static rt_err_t usart_ops_configure(struct rt_serial_device *serial, struct serial_configure *cfg);
static rt_size_t usart_ops_dma_transmit(struct rt_serial_device *serial, const char *buf, rt_size_t size);
static int usart_ops_getc(struct rt_serial_device *serial);
static int usart_ops_putc(struct rt_serial_device *serial, char c);
static rt_err_t usart_ops_control(struct rt_serial_device *serial, int cmd, void *arg);

struct rt_uart_ops usart_ops =
{
  usart_ops_configure,
  usart_ops_control,
  usart_ops_putc,
  usart_ops_getc,
  usart_ops_dma_transmit,
  
};
struct serial_configure serial_device_default_config = RT_SERIAL_CONFIG_DEFAULT;
struct serial_configure serial_device_usart2_config = 
{
	38400, /* 115200 bits/s */  \
	DATA_BITS_8,			/* 8 databits */		 \
	STOP_BITS_1,			/* 1 stopbit */ 		 \
	PARITY_NONE,			/* No parity	*/		 \
	BIT_ORDER_LSB,		/* LSB first sent */ \
	NRZ_NORMAL, 			/* Normal mode */ 	 \
	0 		
};
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
struct serial_user_data usart2_user_data = 
{
  USART2,
  USART2_TX_DMA,
  USART2_RX_DMA,
};
struct serial_ringbuffer usart2_int_rx;
struct serial_ringbuffer usart2_int_tx;

rt_serial_t serial_device_usart2;

#endif

#ifdef RT_USING_USART3
struct serial_user_data usart3_user_data = 
{
  USART3,
  USART3_TX_DMA,
  USART3_RX_DMA,
};
struct serial_ringbuffer usart3_int_rx;
struct serial_ringbuffer usart3_int_tx;

rt_serial_t serial_device_usart3;

#endif

static void RCC_Configuration(struct rt_serial_device *serial)
{
  /*
  struct serial_user_data *user_data;
  user_data = serial->parent.user_data;
  */
#ifdef RT_USING_USART1
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  /* Enable USART1 and GPIOA clocks */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
  /* DMA clock enable */
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  }
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
  /* DMA clock enable */
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  }
  
#endif

#ifdef RT_USING_USART3
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  /* Enable USART3 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
  /* DMA clock enable */
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  }
#endif
}

static void GPIO_Configuration(struct rt_serial_device *serial)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  /*
  struct serial_user_data *user_data;
  user_data = serial->parent.user_data;
  */
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

static void NVIC_Configuration(struct rt_serial_device *serial)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  /*
  struct serial_user_data *user_data;
  user_data = serial->parent.user_data;
  */
#ifdef RT_USING_USART1
  /* Enable the USART1 Interrupt */
  if ((serial->parent.flag & RT_DEVICE_FLAG_INT_RX) | (serial->parent.flag & RT_DEVICE_FLAG_INT_TX))
  {
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }
  /* Enable the DMA1 Channe14 Interrupt for tx*/
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }
  /* Enable the DMA1 Channel2 Interrupt for rx
    
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_RX)
  {
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }
  */

#endif

#ifdef RT_USING_USART2
  if ((serial->parent.flag & RT_DEVICE_FLAG_INT_RX) | (serial->parent.flag & RT_DEVICE_FLAG_INT_TX))
  {
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }
  /* Enable the DMA1 Channel2 Interrupt for tx*/
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }

#endif

#ifdef RT_USING_USART3
  if ((serial->parent.flag & RT_DEVICE_FLAG_INT_RX) | (serial->parent.flag & RT_DEVICE_FLAG_INT_TX))
  {
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }
  /* Enable the DMA1 Channel2 Interrupt for tx*/
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }

#endif
}

static void DMA_Configuration(struct rt_serial_device *serial)
{
  DMA_InitTypeDef DMA_InitStructure;
  struct serial_user_data *user_data;
  user_data = serial->parent.user_data;

  
#if defined (RT_USING_USART1)
  /* fill init structure */
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {  
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    /* DMA1 Channel4 (triggered by USART1 Tx event) Config */
    DMA_DeInit(USART1_TX_DMA);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (rt_int32_t)&(user_data->usart->DR);
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
  }
#endif
#if defined (RT_USING_USART3)

  /* fill init structure */
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {  
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
  }
#endif
}

static rt_err_t usart_ops_control(struct rt_serial_device *serial, int cmd, void *arg)
{
  
  
  return RT_EOK;
}
/*
 * poll send data to usart
 *
 */

static int usart_ops_putc(struct rt_serial_device *serial, char c)
{
  struct serial_user_data *user_data;
  
  user_data = serial->parent.user_data;
  USART_SendData(user_data->usart, c);  
  while (USART_GetFlagStatus(user_data->usart, USART_FLAG_TC) != SET)
  {
    ;
  }

  return RT_EOK;
}

/*
 * poll get data from usart
 *
 */
static int usart_ops_getc(struct rt_serial_device *serial)
{
  
  struct serial_user_data *user_data;
  
  user_data = serial->parent.user_data;
  if (USART_GetFlagStatus(user_data->usart, USART_FLAG_RXNE) != RESET)
  {
    return USART_ReceiveData(user_data->usart);
  }
  
  return -1;
}

static rt_size_t usart_ops_dma_transmit(struct rt_serial_device *serial, const char *buf, rt_size_t size)
{
  struct serial_user_data *user_data;
  
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


static rt_err_t usart_ops_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
  USART_InitTypeDef USART_InitStructure;
  USART_ClockInitTypeDef USART_ClockInitStructure;

  struct serial_user_data *user_data;
  
  user_data = serial->parent.user_data;


  RCC_Configuration(serial);

  GPIO_Configuration(serial);

  NVIC_Configuration(serial);

  DMA_Configuration(serial);
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

  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
  USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
  USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
  USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;
  USART_Init(user_data->usart, &USART_InitStructure);
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
    USART_ITConfig(user_data->usart, USART_IT_ERR, ENABLE);
  }
  
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    /* Enable USART3 DMA Tx request */
    USART_DMACmd(user_data->usart, USART_DMAReq_Tx , ENABLE);
  }

  return RT_EOK;
}

/*
 * Init all related hardware in here
 * rt_hw_serial_init() will register all supported USART device
 */
void rt_hw_usart_init()
{
  /* register uart1 */
#ifdef RT_USING_USART1
  

  serial_device_usart1.ops = &usart_ops;
  serial_device_usart1.int_rx = &usart1_int_rx;
  serial_device_usart1.int_tx = &usart1_int_tx;
  serial_device_usart1.config = serial_device_default_config;

  rt_hw_serial_register(&serial_device_usart1, "usart1",
                        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
                        &usart1_user_data);
  rt_kprintf("register serial_device_usart1 <usart1>\n");
#endif
  /* register uart2 */
#ifdef RT_USING_USART2
  serial_device_usart2.ops = &usart_ops;
  serial_device_usart2.int_rx = &usart2_int_rx;
  serial_device_usart2.int_tx = &usart2_int_tx;
  serial_device_usart2.config = serial_device_default_config;
  rt_hw_serial_register(&serial_device_usart2, "usart2",
                        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
                        &usart2_user_data);
  rt_kprintf("register serial_device_usart2 <usart2>\n");
#endif
  /* register uart3 */
#ifdef RT_USING_USART3
  serial_device_usart3.ops = &usart_ops;
  serial_device_usart3.int_rx = &usart3_int_rx;
  serial_device_usart3.int_tx = &usart3_int_tx;
  serial_device_usart3.config = serial_device_default_config;
  rt_hw_serial_register(&serial_device_usart3, "usart3",
                        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_TX,
                        &usart3_user_data);
  rt_kprintf("register serial_device_usart3 <usart3>\n");
#endif
}

/*
void serial_device_usart_isr(struct rt_serial_device *serial)
{
  struct serial_user_data *user_data;  
  user_data = serial->parent.user_data;

  if(USART_GetFlagStatus(user_data->usart, USART_FLAG_ORE) != RESET)
  {

    USART_ReceiveData(user_data->usart);
    USART_ClearFlag(user_data->usart, USART_FLAG_ORE);
  }

  if(USART_GetFlagStatus(user_data->usart, USART_FLAG_NE) != RESET)
  {//同  @arg USART_IT_NE     : Noise Error interrupt
    USART_ClearFlag(user_data->usart, USART_FLAG_NE);
  }


  if(USART_GetFlagStatus(user_data->usart, USART_FLAG_FE) != RESET)
  {//同   @arg USART_IT_FE     : Framing Error interrupt
    USART_ClearFlag(user_data->usart, USART_FLAG_FE);
  }

  if(USART_GetFlagStatus(user_data->usart, USART_FLAG_PE) != RESET)
  {//同  @arg USART_IT_PE     : Parity Error interrupt
    USART_ClearFlag(user_data->usart, USART_FLAG_PE);
  }
  rt_hw_serial_isr(serial);
}
*/

#ifdef RT_USING_FINSH
#include <finsh.h>

#ifdef RT_USING_USART1
void usart1(rt_int8_t cmd, rt_int8_t *str)
{
  rt_device_t usart;

  usart = rt_device_find("usart1");
  if (usart != RT_NULL)
  {
    if (cmd == 0)
    {
      rt_device_read(usart,0,str,20);
    }
    else
    {
      rt_device_write(usart,0,str,20);
    }    
  }
  else
  {
    rt_kprintf("device uart1 is not exist!\n");
  }
}
FINSH_FUNCTION_EXPORT(usart1, set usart1[0 xxx] for read.)

#endif

#ifdef RT_USING_USART2
void usart2(rt_int8_t cmd, rt_int8_t *str)
{
  rt_device_t usart;

  usart = rt_device_find("usart2");
  if (usart != RT_NULL)
  {
    if (cmd == 0)
    {
      rt_device_read(usart,0,str,20);
    }
    else
    {
      rt_device_write(usart,0,str,20);
    }    
  }
  else
  {
    rt_kprintf("device usart2 is not exist!\n");
  }
}
FINSH_FUNCTION_EXPORT(usart2, set usart2[0 xxx] for read.)
#endif

#ifdef RT_USING_USART3
void usart3(rt_int8_t cmd, rt_int8_t *str)
{
  rt_device_t usart;

  usart = rt_device_find("usart3");
  if (usart != RT_NULL)
  {
    if (cmd == 0)
    {
      rt_device_read(usart,0,str,20);
    }
    else
    {
      rt_device_write(usart,0,str,20);
    }    
  }
  else
  {
    rt_kprintf("device uart3 is not exist!\n");
  }
}
FINSH_FUNCTION_EXPORT(usart3, set usart3[0 xxx] for read.)
#endif
#endif

