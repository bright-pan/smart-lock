/*********************************************************************
 * Filename:      rfid_uart.c
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

#include "rfid_uart.h"

/*
 * Use UART4 with interrupt Rx and dma Tx
 *
 * UART DMA setting on STM32
 * UART4 Tx --> DMA2 Channel 5
 * UART4 Rx --> DMA2 Channel 3
 */

/* USART4_REMAP = 0 */
#define RFID_UART_GPIO_PIN_TX		GPIO_Pin_10
#define RFID_UART_GPIO_PIN_RX		GPIO_Pin_11
#define RFID_UART_GPIO		        GPIOC
#define RFID_UART_GPIO_RCC	        RCC_APB2Periph_GPIOC
#define RFID_UART_RCC	                RCC_APB1Periph_UART4
#define RFID_UART                       UART4
#define RFID_UART_TX_DMA_CHANNEL	DMA2_Channel5
#define RFID_UART_RX_DMA_CHANNEL	DMA2_Channel3
#define RFID_UART_DMA_RCC               RCC_AHBPeriph_DMA2

static rt_err_t rfid_uart_pos_configure(struct rt_serial_device *serial, struct serial_configure *cfg);
static rt_size_t rfid_uart_pos_dma_transmit(struct rt_serial_device *serial, const char *buf, rt_size_t size);
static int rfid_uart_pos_getc(struct rt_serial_device *serial);
static int rfid_uart_pos_putc(struct rt_serial_device *serial, char c);
static rt_err_t rfid_uart_pos_control(struct rt_serial_device *serial, int cmd, void *arg);

struct rt_uart_ops rfid_uart_pos =
{
  rfid_uart_pos_configure,
  rfid_uart_pos_control,
  rfid_uart_pos_putc,
  rfid_uart_pos_getc,
  rfid_uart_pos_dma_transmit,
};
struct serial_configure rfid_uart_default_config = RT_SERIAL_CONFIG_DEFAULT;

struct rfid_uart_user_data_t rfid_uart_user_data = 
{
  RFID_UART,
  RFID_UART_TX_DMA_CHANNEL,
  RFID_UART_RX_DMA_CHANNEL,
};
struct serial_ringbuffer rfid_uart_int_rx;
struct serial_ringbuffer rfid_uart_int_tx;

rt_serial_t rfid_uart_device;

static void RCC_Configuration(struct rt_serial_device *serial)
{
  /*
  struct serial_user_data *user_data;
  user_data = serial->parent.user_data;
  */
  /* Enable AFIO and RFID_UART_GPIO GSM_POWER GSM_STATUS clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RFID_UART_GPIO_RCC, ENABLE);
  /* Enable the USART2 Pins Software Remapping */
  //GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);

  /* Enable RFID_UART_RCC clock */
  RCC_APB1PeriphClockCmd(RFID_UART_RCC, ENABLE);
  /* DMA clock enable */
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    RCC_AHBPeriphClockCmd(RFID_UART_DMA_RCC, ENABLE);
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
  //GPIO_InitStructure.GPIO_Pin = RFID_UART_GPIO_PIN_RX | RFID_UART_GPIO_PIN_CTS;
  GPIO_InitStructure.GPIO_Pin = RFID_UART_GPIO_PIN_RX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(RFID_UART_GPIO, &GPIO_InitStructure);

  /* Configure USART2 Tx RTS as alternate function push-pull */
  GPIO_StructInit(&GPIO_InitStructure);
  //GPIO_InitStructure.GPIO_Pin = RFID_UART_GPIO_PIN_TX | RFID_UART_GPIO_PIN_RTS;
  GPIO_InitStructure.GPIO_Pin = RFID_UART_GPIO_PIN_TX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(RFID_UART_GPIO, &GPIO_InitStructure);
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
    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }
  /* Enable the DMA1 Channel7 Interrupt for tx*/
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Channel4_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
  }

}


static void DMA_Configuration(struct rt_serial_device *serial)
{
  DMA_InitTypeDef DMA_InitStructure;
  struct rfid_uart_user_data_t *user_data;
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
    DMA_DeInit(RFID_UART_TX_DMA_CHANNEL);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (rt_int32_t)&(user_data->usart->DR);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    /* As we will set them before DMA actually enabled, the DMA_MemoryBaseAddr
     * and DMA_BufferSize are meaningless. So just set them to proper values
     * which could make DMA_Init happy.
     */
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;
    DMA_InitStructure.DMA_BufferSize = 1;
    DMA_Init(RFID_UART_TX_DMA_CHANNEL, &DMA_InitStructure);
    DMA_ITConfig(RFID_UART_TX_DMA_CHANNEL, DMA_IT_TC | DMA_IT_TE, ENABLE);
    DMA_ClearFlag(DMA2_FLAG_TC5 | DMA2_FLAG_TE5);
  }
}


static rt_err_t rfid_uart_pos_control(struct rt_serial_device *serial, int cmd, void *arg)
{
  
  
  return RT_EOK;
}
/*
 * poll send data to gsm
 *
 */

static int rfid_uart_pos_putc(struct rt_serial_device *serial, char c)
{
  struct rfid_uart_user_data_t *user_data;
  
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
static int rfid_uart_pos_getc(struct rt_serial_device *serial)
{
  
  struct rfid_uart_user_data_t *user_data;
  
  user_data = serial->parent.user_data;
  if (USART_GetFlagStatus(user_data->usart, USART_FLAG_RXNE) != RESET)
  {
    return USART_ReceiveData(user_data->usart);
  }
  
  return -1;
}

static rt_size_t rfid_uart_pos_dma_transmit(struct rt_serial_device *serial, const char *buf, rt_size_t size)
{
  struct rfid_uart_user_data_t *user_data;
  
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


static rt_err_t rfid_uart_pos_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
  USART_InitTypeDef USART_InitStructure;

  struct rfid_uart_user_data_t *user_data;
  
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
  
  if (serial->parent.flag & RT_DEVICE_FLAG_DMA_TX)
  {
    /* Enable USART2 DMA Tx request */
    USART_DMACmd(user_data->usart, USART_DMAReq_Tx , ENABLE);
  }

  return RT_EOK;
}

/*
 * Init all related hardware in here
 * rt_hw_rfid_uart_register() will register all supported GSM device
 */
void rt_hw_rfid_uart_register(void)
{
  rfid_uart_device.ops = &rfid_uart_pos;
  rfid_uart_device.int_rx = &rfid_uart_int_rx;
  rfid_uart_device.int_tx = &rfid_uart_int_tx;
  rfid_uart_device.config = rfid_uart_default_config;
  rt_hw_serial_register(&rfid_uart_device, DEVICE_NAME_RFID_UART,
                        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_TX,
                        &rfid_uart_user_data);
  rt_kprintf("register rfid usart device\n");
}

void rfid_uart_device_isr(struct rt_serial_device *serial)
{
  struct rfid_uart_user_data_t *user_data;
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

void rfid_uart(char cmd, char *str)
{
  rt_device_t device;
  memset(temp, '\xFF', 100);
  device = rt_device_find(DEVICE_NAME_RFID_UART);
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
    rt_kprintf("device %s is not exist!\n", DEVICE_NAME_RFID_UART);
  }
}
FINSH_FUNCTION_EXPORT(rfid_uart, rfid_uart[cmd parameters])
#endif
