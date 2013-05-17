/*********************************************************************
 * Filename:      camera_uart.h
 * 
 *
 * Author:        Bright Pan <loststriker@gmail.com>
 * Created at:    2013-05-08 10:18:16
 *
 *
 *
 * Change Log:    
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/

#ifndef _CAMERA_UART_H_
#define _CAMERA_UART_H_
#include <rtdevice.h>
#include <serial.h>
#include <stm32f10x_dma.h>

struct camera_uart_user_data_t
{
  USART_TypeDef *usart;
  DMA_Channel_TypeDef *usart_tx_dma_channel;
  DMA_Channel_TypeDef *usart_rx_dma_channel;
};

#define DEVICE_NAME_CAMERA_UART "cm_uart"

void rt_hw_camera_uart_register(void);
    
#endif
