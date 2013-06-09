/*********************************************************************
 * Filename:    spibus.h
 *
 * Description:    This file  complete spi bus configure and
 *						   spi cs device regiser requisite declare
 *						   
 * Author:        wangzw <wangzw@yuettak.com>
 * Created at:    2013-04-27 9:00:00
 *                
 * Modify:
 *
 * 
 *
 * Copyright (C) 2013 Yuettak Co.,Ltd
 ********************************************************************/
#ifndef	__SPIBUS_H__
#define		__SPIBUS_H__

#include <rtdevice.h>
#include "stm32f10x.h"
#include "board.h"

#define USING_SPI1
#define USING_SPI2
#define SPI_USE_DMA

#ifdef USING_SPI1
static struct stm32_spi_bus stm32_spi_bus_1;
#endif /* #ifdef USING_SPI1 */

#ifdef USING_SPI2
static struct stm32_spi_bus stm32_spi_bus_2;
#endif /* #ifdef USING_SPI2 */

#ifdef USING_SPI3
static struct stm32_spi_bus stm32_spi_bus_3;
#endif /* #ifdef USING_SPI3 */


struct stm32_spi_bus
{
    struct rt_spi_bus parent;
    SPI_TypeDef * SPI;
#ifdef SPI_USE_DMA
    DMA_Channel_TypeDef * DMA_Channel_TX;
    DMA_Channel_TypeDef * DMA_Channel_RX;
    uint32_t DMA_Channel_TX_FLAG_TC;
    uint32_t DMA_Channel_TX_FLAG_TE;
    uint32_t DMA_Channel_RX_FLAG_TC;
    uint32_t DMA_Channel_RX_FLAG_TE;
#endif /* SPI_USE_DMA */
};

struct stm32_spi_cs
{
    GPIO_TypeDef * GPIOx;
    uint16_t GPIO_Pin;
};

/* public function list */
rt_err_t stm32_spi_register(SPI_TypeDef * SPI,
                            struct stm32_spi_bus * stm32_spi,
                            const char * spi_bus_name);

#endif // STM32_SPI_H_INCLUDED
