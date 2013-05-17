/*********************************************************************
 * Filename:      gsm_usart.h
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

#ifndef _GSM_USART_H_
#define _GSM_USART_H_
#include <rtdevice.h>
//#include serial.h>
#include <stm32f10x_dma.h>

struct gsm_usart_user_data_t
{
  USART_TypeDef *usart;
  DMA_Channel_TypeDef *usart_tx_dma_channel;
  DMA_Channel_TypeDef *usart_rx_dma_channel;
};

#define RT_DEVICE_CTRL_GSM_POWER    0x14    /* gsm power control */
#define RT_DEVICE_CTRL_GSM_GET_STATUS 0x15    /* get gsm status */

#define DEVICE_NAME_GSM_USART "g_usart"

void rt_hw_gsm_usart_register(void); 
    
#endif
