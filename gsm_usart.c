/*********************************************************************
 * Filename:      gsm_usart.c
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

#include "gsm_usart.h"
#define GSM_RX_BUFFER_LEN							1024
#define GSM_TX_BUFFER_LEN							1024


/*
 * Use UART2 with interrupt Rx and dma Tx
 * 
 * USART DMA setting on STM32
 * USART2 Tx --> DMA1 Channel 7
 * USART2 Rx --> DMA1 Channel 6
 * 
 * | gsm usart | pin | description    |
 * |-----------+-----+----------------|
 * | TX        | PD5 | usart2 remap=1 |
 * | RX        | PD6 |                |
 * | CTX       | PD3 |                |
 * | RTX       | PD4 |                |
 * 
 * 
 */

#define GSM_USART_USE_HW_CONTROL

#define GSM_USART_DR_Base              0x40004404
/* USART2_REMAP = 1 */
#define GSM_USART_GPIO_PIN_CTS		GPIO_Pin_3
#define GSM_USART_GPIO_PIN_RTS		GPIO_Pin_4

#define GSM_USART_GPIO_PIN_TX		GPIO_Pin_5
#define GSM_USART_GPIO_PIN_RX		GPIO_Pin_6
#define GSM_USART_GPIO		        GPIOD
#define GSM_USART_GPIO_RCC	        RCC_APB2Periph_GPIOD

//#define GSM_USART_GPIO_PIN_TX		GPIO_Pin_2
//#define GSM_USART_GPIO_PIN_RX		GPIO_Pin_3
//#define GSM_USART_GPIO		        GPIOA
//#define GSM_USART_GPIO_RCC	        RCC_APB2Periph_GPIOA

#define GSM_USART_RCC	                RCC_APB1Periph_USART2
#define GSM_USART                       USART2
#define GSM_USART_TX_DMA_CHANNEL	DMA1_Channel7
#define GSM_USART_RX_DMA_CHANNEL	DMA1_Channel6
#define GSM_USART_DMA_RCC               RCC_AHBPeriph_DMA1

static rt_err_t gsm_usart_pos_configure(struct rt_serial_device *serial, struct serial_configure *cfg);
static rt_size_t gsm_usart_pos_dma_transmit(struct rt_serial_device *serial, const char *buf, rt_size_t size);
static int gsm_usart_pos_getc(struct rt_serial_device *serial);
static int gsm_usart_pos_putc(struct rt_serial_device *serial, char c);
static rt_err_t gsm_usart_pos_control(struct rt_serial_device *serial, int cmd, void *arg);

struct rt_uart_ops gsm_usart_pos =
{
  gsm_usart_pos_configure,
  gsm_usart_pos_control,
  gsm_usart_pos_putc,
  gsm_usart_pos_getc,
  gsm_usart_pos_dma_transmit,
};
struct serial_configure gsm_usart_default_config =
{
  BAUD_RATE_115200, /* 115200 bits/s */
  //BAUD_RATE_9600, /* 115200 bits/s */
  DATA_BITS_8,      /* 8 databits */
  STOP_BITS_1,      /* 1 stopbit */
  PARITY_NONE,      /* No parity  */
  BIT_ORDER_LSB,    /* LSB first sent */
  NRZ_NORMAL,       /* Normal mode */
  0,                                  
};


struct gsm_usart_user_data_t gsm_usart_user_data = 
{
  GSM_USART,
  GSM_USART_TX_DMA_CHANNEL,
  GSM_USART_RX_DMA_CHANNEL,
};
rt_uint8_t	gsm_rx_buffer[GSM_RX_BUFFER_LEN];
rt_uint8_t	gsm_tx_buffer[GSM_TX_BUFFER_LEN];


struct serial_ringbuffer gsm_usart_int_rx;
struct serial_ringbuffer gsm_usart_int_tx;

rt_serial_t gsm_usart_device;

static void RCC_Configuration(struct rt_serial_device *serial)
{
  /*
  struct serial_user_data *user_data;
  user_data = serial->parent.user_data;
  */
  /* Enable AFIO and GSM_USART_GPIO GSM_POWER GSM_STATUS clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | GSM_USART_GPIO_RCC, ENABLE);
  /* Enable the USART2 Pins Software Remapping */
  GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);

  /* Enable GSM_USART_RCC clock */
  RCC_APB1PeriphClockCmd(GSM_USART_RCC, ENABLE);
  /* DMA clock enable */
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    RCC_AHBPeriphClockCmd(GSM_USART_DMA_RCC, ENABLE);
  }
}

static void GPIO_Configuration(struct rt_serial_device *serial)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  /*
  struct serial_user_data *user_data;
  user_data = serial->parent.user_data;
  */
  /* Configure USART2 Rx CTS as input floating */
  GPIO_StructInit(&GPIO_InitStructure);
#ifdef GSM_USART_USE_HW_CONTROL
  GPIO_InitStructure.GPIO_Pin = GSM_USART_GPIO_PIN_RX | GSM_USART_GPIO_PIN_CTS;
#else
  GPIO_InitStructure.GPIO_Pin = GSM_USART_GPIO_PIN_RX;
#endif
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GSM_USART_GPIO, &GPIO_InitStructure);

  /* Configure USART2 Tx RTS as alternate function push-pull */
  GPIO_StructInit(&GPIO_InitStructure);
#ifdef GSM_USART_USE_HW_CONTROL
  GPIO_InitStructure.GPIO_Pin = GSM_USART_GPIO_PIN_TX | GSM_USART_GPIO_PIN_RTS;
#else
  GPIO_InitStructure.GPIO_Pin = GSM_USART_GPIO_PIN_TX;
#endif
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GSM_USART_GPIO, &GPIO_InitStructure);
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
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }
  /* Enable the DMA1 Channel7 Interrupt for tx*/
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }
}

static void DMA_Configuration(struct rt_serial_device *serial)
{
  DMA_InitTypeDef DMA_InitStructure;
  struct gsm_usart_user_data_t *user_data;
  user_data = serial->parent.user_data;

  /* fill init structure */
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    DMA_StructInit(&DMA_InitStructure);
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    /* DMA1 Channel7 (triggered by USART2 Tx event) Config */
    DMA_DeInit(GSM_USART_TX_DMA_CHANNEL);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (rt_int32_t)&(user_data->usart->DR);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    /* As we will set them before DMA actually enabled, the DMA_MemoryBaseAddr
     * and DMA_BufferSize are meaningless. So just set them to proper values
     * which could make DMA_Init happy.
     */
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;
    DMA_InitStructure.DMA_BufferSize = 1;
    DMA_Init(GSM_USART_TX_DMA_CHANNEL, &DMA_InitStructure);
    DMA_ITConfig(GSM_USART_TX_DMA_CHANNEL, DMA_IT_TC | DMA_IT_TE, ENABLE);
    DMA_ClearFlag(DMA1_FLAG_TC7| DMA1_FLAG_TE7);
  }
}

static rt_err_t gsm_usart_pos_control(struct rt_serial_device *serial, int cmd, void *arg)
{
  
  
  return RT_EOK;
}
/*
 * poll send data to gsm
 *
 */

static int gsm_usart_pos_putc(struct rt_serial_device *serial, char c)
{
  struct gsm_usart_user_data_t *user_data;
  
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
static int gsm_usart_pos_getc(struct rt_serial_device *serial)
{
  
  struct gsm_usart_user_data_t *user_data;
  
  user_data = serial->parent.user_data;
  if (USART_GetFlagStatus(user_data->usart, USART_FLAG_RXNE) != RESET)
  {
    return USART_ReceiveData(user_data->usart);
  }
  
  return -1;
}

static rt_size_t gsm_usart_pos_dma_transmit(struct rt_serial_device *serial, const char *buf, rt_size_t size)
{
  struct gsm_usart_user_data_t *user_data;
  
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


static rt_err_t gsm_usart_pos_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
  USART_InitTypeDef USART_InitStructure;
  USART_ClockInitTypeDef USART_ClockInitStructure;

  struct gsm_usart_user_data_t *user_data;
  
  user_data = serial->parent.user_data;


  RCC_Configuration(serial);

  GPIO_Configuration(serial);

  NVIC_Configuration(serial);

  DMA_Configuration(serial);

  /* uart init */
  USART_StructInit(&USART_InitStructure);
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
#ifdef GSM_USART_USE_HW_CONTROL
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
#else
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
#endif
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_ClockStructInit(&USART_ClockInitStructure);
  USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
  USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
  USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
  USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;
  USART_Init(user_data->usart, &USART_InitStructure);
  USART_ClockInit(user_data->usart, &USART_ClockInitStructure);
  /* enable usart */
  USART_Cmd(user_data->usart, ENABLE);
  if (serial->parent.flag & RT_DEVICE_FLAG_INT_RX)
  {
    /* enable interrupt */
    USART_ITConfig(user_data->usart, USART_IT_RXNE, ENABLE);
    USART_ITConfig(user_data->usart, USART_IT_ERR, ENABLE);
  }
  
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    /* Enable USART2 DMA Tx request */
    USART_DMACmd(user_data->usart, USART_DMAReq_Tx , ENABLE);
  }

  return RT_EOK;
}

/*
 * Init all related hardware in here
 * rt_hw_gsm_init() will register all supported GSM device
 */
void rt_hw_gsm_usart_register(void)
{
  gsm_usart_device.ops = &gsm_usart_pos;
  gsm_usart_int_rx.buffer = gsm_rx_buffer;
  gsm_usart_int_tx.buffer = gsm_tx_buffer;
  gsm_usart_int_rx.size = GSM_RX_BUFFER_LEN;
  gsm_usart_int_tx.size = GSM_TX_BUFFER_LEN;
  gsm_usart_device.int_rx = &gsm_usart_int_rx;
  gsm_usart_device.int_tx = &gsm_usart_int_tx;
  gsm_usart_device.config = gsm_usart_default_config;
  rt_hw_serial_register(&gsm_usart_device, DEVICE_NAME_GSM_USART,
                        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_TX,
                        &gsm_usart_user_data);
  rt_kprintf("register gsm usart device\n");
}

void gsm_usart_device_isr(struct rt_serial_device *serial)
{
  struct gsm_usart_user_data_t *user_data;
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

void gsm_usart(rt_int8_t cmd, const char *str)
{
  rt_uint8_t i = 0;
  rt_device_t device;
  memset(temp, '\0', 100);
  device = rt_device_find(DEVICE_NAME_GSM_USART);
  if (device != RT_NULL)
  {
    if (cmd == 0)
    {
      
      rt_device_read(device, 0, temp, 20);
      temp[99] = '\0';
      while(i < 20)
      {
        rt_kprintf("%c", temp[i++]);
      }
      rt_kprintf("\n");
      i = 0;
      while(i < 20)
      {
        rt_kprintf("%02X ", temp[i++]);
      }
      rt_kprintf("\n");
    }
    else if (cmd == 1)
    {
      rt_device_write(device, 0, str, strlen(str));
    }
  }
  else
  {
    rt_kprintf("device %s is not exist!\n", DEVICE_NAME_GSM_USART);
  }
}
FINSH_FUNCTION_EXPORT(gsm_usart, gsm_usart[cmd parameters])
#endif
