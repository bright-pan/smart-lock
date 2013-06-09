#ifndef	__SST25V16_H__
#define 	__SST25V16_H__
#include "rtdevice.h"
#include "rtthread.h"
#include "spibus.h"

/*  hardware platform -------------------------*/
//#define USE_WILDFIRE_TEST								//use wild fire hardware platform

/* device name  -------------------------*/
#define SPI2_CS1_NAME1									("sst25")
#define SPI2_BUS_NAME										("spi2bus")
#define FLASH1_DEVICE_NAME							("flash1")

#ifdef USE_WILDFIRE_TEST
/* hardware define pin port clock -------------*/
#define SPI2_SCK_PIN										GPIO_Pin_5
#define	 SPI2_MISO_PIN									GPIO_Pin_6
#define SPI2_MOSI_PIN										GPIO_Pin_7
#define SPI2_GPIO_PORT									GPIOA
#define SPI2_CLOCK											RCC_APB2Periph_SPI1


#define SPI2_CS1_PIN										GPIO_Pin_4
#define SPI2_CS1_PORT										GPIOA
#define SPI2_CS1_CLOCK									RCC_APB2Periph_GPIOA

#define GPIO_RCC												RCC_APB2Periph_GPIOA
#define SPI_PORT												SPI1

#else
/* hardware define pin port clock -------------*/
#define SPI2_SCK_PIN										GPIO_Pin_13
#define	 SPI2_MISO_PIN									GPIO_Pin_14
#define SPI2_MOSI_PIN										GPIO_Pin_15
#define SPI2_GPIO_PORT									GPIOB
#define SPI2_CLOCK											RCC_APB1Periph_SPI2


#define SPI2_CS1_PIN										GPIO_Pin_12
#define SPI2_CS1_PORT										GPIOB
#define SPI2_CS1_CLOCK									RCC_APB2Periph_GPIOB

#define GPIO_RCC												RCC_APB2Periph_GPIOB
#define SPI_PORT												SPI2
#endif


void rt_hw_spi_flash_init(void);




#endif




